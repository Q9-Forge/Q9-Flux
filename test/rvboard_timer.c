//════════════════════════════════════════════════════════════════════════════════════════════════
// File:   rvboard_timer.c                                                                 Ver. 1.00
// Owner:  Claudia
// Desc.:  Stufe 2b des RISC-V-Bring-up: Board mit RAM + 16550-UART + CLINT, treibt den Gast aus
//         test/riscv/timer/ und prueft, dass Timer-Interrupts sauber ankommen.
//
//         Der wichtige Unterschied zu rvboard_hello.c (Stufe 2) liegt in der Hauptschleife: dort
//         genuegte "interp() aufrufen, bis wfi anhaelt". Hier muss die Schleife zusaetzlich JEDEN
//         Abschnitt die CLINT-Uhr voranstellen und bei Bedarf riscv_cpu_set_mip()/reset_mip()
//         aufrufen -- s. die "set_mip-Falle" in src/devices/clint/clint.h. Ohne das wuerde ein
//         "wfi" im Gast fuer immer haengen, obwohl die Zeit laengst abgelaufen ist: der Kern
//         wertet ein blosses Veraendern der Zeit nicht von selbst aus.
//
// Call:   build/<platform>/rvboard_timer <programm.elf>     (gebaut per "make test-rvtimer")
//════════════════════════════════════════════════════════════════════════════════════════════════

#include <stdio.h>
#include <string.h>

#include "cutils.h"
#include "iomem.h"
#include "riscv_cpu.h"

#include "rvelf.h"
#include "uart16550.h"
#include "clint.h"

#define RAM_BASE    0x80000000ULL
#define RAM_SIZE    (16u * 1024u * 1024u)
#define BOOT_BASE   0x1000ULL
#define BOOT_SIZE   0x1000u
#define UART0_BASE  0x10000000ULL
#define CLINT_BASE  0x02000000ULL

#define CYCLES_PER_SLICE  2000       /* kleiner als in rvboard_hello.c: DELTA im Gast ist 5000
                                         Zyklen, wir wollen die CLINT-Uhr feiner nachfuehren als
                                         eine ganze Interruptperiode auf einen Schlag */
#define MAX_SLICES        20000

/* GENAU 20000*2000 = 40 Mio. simulierte Zyklen Vorrat, der Gast braucht real nur ~30000 (5
   Interrupts a 5000 Zyklen + etwas Rechenzeit) -- reichlich Luft, ohne den Test spuerbar zu
   verlangsamen: ein "power_down"-Abschnitt kostet praktisch nur einen Funktionsaufruf. */

static void uart_tx_to_stdout(void *opaque, uint8_t ch)
{
    (void)opaque;
    fputc((int)ch, stdout);
    fflush(stdout);
}

static uint32_t uart_read(void *opaque, uint32_t offset, int size_log2)
{
    (void)size_log2;
    return q9_uart16550_read8((q9_uart16550_t *)opaque, offset);
}

static void uart_write(void *opaque, uint32_t offset, uint32_t val, int size_log2)
{
    (void)size_log2;
    q9_uart16550_write8((q9_uart16550_t *)opaque, offset, (uint8_t)(val & 0xffu));
}

/* Verbindet das Geraet (kennt den Kern nicht, s. clint.h) mit dem Kern (kennt das Geraet nicht) --
   noetig fuer den Sync-Kunstgriff unten. cpu bleibt bis nach riscv_cpu_init() NULL; das ist
   sicher, weil vor dem ersten Schleifendurchlauf keine MMIO-Zugriffe stattfinden koennen. */
typedef struct {
    q9_clint_t     *clint;
    RISCVCPUState  *cpu;
    int             mtip_set;
} clint_ctx_t;

/* Gleicht mip.MTIP im Kern SOFORT an den CLINT-Zustand an. Auf echter Hardware ist mip.MTIP ein
   KOMBINATORISCHES Signal (mtime >= mtimecmp, jederzeit aktuell) -- in diesem Kern dagegen ein
   Zwischenspeicher, den nur set_mip()/reset_mip() aendern (s. clint.h-Kopf). Wird dieser Abgleich
   NUR periodisch (einmal pro Abschnitt der Hauptschleife) gemacht, bleibt MTIP zwischen zwei
   Abschnitten gesetzt -- und der Kern prueft mip&mie vor JEDER Instruktion neu, feuert den
   Trap-Handler also wiederholt, bis der naechste Abschnittsabgleich endlich reset_mip() aufruft.
   GEFUNDEN beim ersten Testlauf: 33 statt der erwarteten 5 Interrupts, ein echter
   Interrupt-Sturm -- der Trap-Handler braucht nur ~40-60 Zyklen, ein Abschnitt hier 2000, macht
   also reichlich Platz fuer Nachfeuern. Der Fix ist NICHT ein kleinerer Abschnitt (das
   verschiebt das Problem nur, kostet aber Laufzeit), sondern der Abgleich GENAU an der Stelle,
   die es auf echter Hardware "kombinatorisch" macht: sofort nach jedem Schreibzugriff auf
   mtimecmp, zusaetzlich zum periodischen Abgleich fuer die "Zeit ist abgelaufen"-Richtung. */
static void clint_sync(clint_ctx_t *ctx)
{
    int pending = q9_clint_timer_pending(ctx->clint);
    if (pending && !ctx->mtip_set) {
        riscv_cpu_set_mip(ctx->cpu, MIP_MTIP);
        ctx->mtip_set = 1;
    } else if (!pending && ctx->mtip_set) {
        riscv_cpu_reset_mip(ctx->cpu, MIP_MTIP);
        ctx->mtip_set = 0;
    }
}

static uint32_t clint_read(void *opaque, uint32_t offset, int size_log2)
{
    clint_ctx_t *ctx = (clint_ctx_t *)opaque;
    (void)size_log2;    /* mtimecmp/mtime werden vom Gast ausschliesslich 32-Bit-weise
                            angesprochen (der Kern kennt keine 64-Bit-MMIO, s. iomem.h) */
    return q9_clint_read32(ctx->clint, offset);
}

static void clint_write(void *opaque, uint32_t offset, uint32_t val, int size_log2)
{
    clint_ctx_t *ctx = (clint_ctx_t *)opaque;
    (void)size_log2;
    q9_clint_write32(ctx->clint, offset, val);
    clint_sync(ctx);    /* der eigentliche Fix, s. Kommentar bei clint_sync() */
}

int main(int argc, char **argv)
{
    rvelf_t ef;
    PhysMemoryMap *map;
    RISCVCPUState *cpu;
    q9_uart16550_t uart;
    q9_clint_t clint;
    clint_ctx_t clint_ctx;
    int slice, halted = 0;

    if (argc < 2) {
        fprintf(stderr, "Aufruf: %s <programm.elf>\n", argv[0]);
        return 2;
    }

    if (rvelf_read(argv[1], &ef) != 0) return 1;

    map = phys_mem_map_init();
    cpu_register_ram(map, RAM_BASE,  RAM_SIZE,  0);
    cpu_register_ram(map, BOOT_BASE, BOOT_SIZE, 0);

    q9_uart16550_init(&uart, uart_tx_to_stdout, NULL, NULL);
    cpu_register_device(map, UART0_BASE, Q9_UART16550_SIZE, &uart,
                        uart_read, uart_write, DEVIO_SIZE8 | DEVIO_SIZE16 | DEVIO_SIZE32);

    q9_clint_init(&clint);
    clint_ctx.clint = &clint;
    clint_ctx.cpu = NULL;         /* erst nach riscv_cpu_init() gueltig, s. Typ-Kommentar */
    clint_ctx.mtip_set = 0;
    cpu_register_device(map, CLINT_BASE, Q9_CLINT_SIZE, &clint_ctx,
                        clint_read, clint_write, DEVIO_SIZE32);

    if (rvelf_load_segments(&ef, map) != 0)                    goto fail;
    if (rvelf_place_boot_stub(map, BOOT_BASE, ef.entry) != 0)   goto fail;

    cpu = riscv_cpu_init(map, 32);
    if (!cpu) { fprintf(stderr, "  CPU laesst sich nicht anlegen\n"); goto fail; }
    clint_ctx.cpu = cpu;

    /* KEIN frueher Ausstieg bei power_down: "wfi" heisst hier nur "gerade nichts zu tun", nicht
       "Programm fertig" -- der Gast wartet insgesamt FUENFMAL, zwischen jedem Mal wacht er durch
       den naechsten Timer-Interrupt wieder auf. Ein Ausstieg beim ERSTEN wfi (wie es
       rvboard_hello.c fuer sein echtes Einmal-Programm tun darf) haette hier nur den Start der
       ersten Wartezeit als "fertig" missverstanden -- genau der Fehler, an dem dieser Testlauf
       beim ersten Versuch scheiterte (Abbruch nach 393 statt der noetigen rund 30000 Zyklen).
       Der volle Zyklenvorrat wird daher immer ausgeschoepft; ein power_down-Abschnitt kostet den
       Wirt praktisch nur einen Funktionsaufruf, das Budget ist grosszuegig genug (s. oben). */
    for (slice = 0; slice < MAX_SLICES; slice++) {
        q9_clint_advance(&clint, CYCLES_PER_SLICE);
        /* Periodischer Abgleich deckt die Richtung "Zeit ist abgelaufen" ab (mtime steigt nur
           hier, ausserhalb jeder MMIO-Aktion). Die Gegenrichtung -- "der Gast hat mtimecmp neu
           gesetzt" -- deckt clint_write() bereits SOFORT ab, s. dortigen Kommentar. */
        clint_sync(&clint_ctx);
        riscv_cpu_interp(cpu, CYCLES_PER_SLICE);
    }
    halted = riscv_cpu_get_power_down(cpu) ? 1 : 0;

    printf("\n  %s nach %llu Zyklen\n",
           halted ? "am Ende im Stromsparzustand (wfi)"
                  : "am Ende NICHT im Stromsparzustand -- haengt vermutlich in einer Rechenschleife",
           (unsigned long long)riscv_cpu_get_cycles(cpu));

    riscv_cpu_end(cpu);
    phys_mem_map_end(map);
    rvelf_free(&ef);
    return halted ? 0 : 1;

fail:
    phys_mem_map_end(map);
    rvelf_free(&ef);
    return 1;
}

// EOF rvboard_timer.c                                                                      Ver. 1.00
