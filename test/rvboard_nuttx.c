//════════════════════════════════════════════════════════════════════════════════════════════════
// File:   rvboard_nuttx.c                                                                 Ver. 1.00
// Owner:  Claudia
// Desc.:  Stufe 3 des RISC-V-Bring-up: allgemeiner Board-Treiber fuer ein Gastsystem mit echten
//         Tastatureingaben -- RAM + 16550-UART + CLINT + PLIC, Speicherkarte nach QEMU "virt".
//         Zuerst gegen NuttX (rv-virt:nsh) verifiziert, aber nicht NuttX-spezifisch: jedes ELF,
//         das dieselbe Geraeteauswahl erwartet, laeuft hier.
//
//         Warum ueberhaupt PLIC (anders als Stufe 2/2b): NuttX's 16550-Treiber haengt sich per
//         irq_attach() in den Interrupt-Pfad -- ohne PLIC bootet das System zwar bis zum
//         Shell-Prompt (TX ist gepollt), aber KEINE Tastatureingabe kommt an (leer geprueft:
//         identischer Zyklenzaehler mit/ohne Eingabe ueber stdin). Mit PLIC bestaetigt.
//
//         SET_MIP-DISZIPLIN (aus der CLINT-Sturm-Lehre von Stufe 2b uebernommen, nicht neu
//         erfunden): jede Aenderung, die q9_plic_context_pending() beeinflussen koennte --
//         MMIO-Zugriff auf PLIC ODER UART, sowie der periodische Poll auf neue Eingabe --
//         zieht SOFORT einen Abgleich von MIP_MEIP nach sich, nicht nur einmal pro Abschnitt.
//
// Call:   build/<platform>/rvboard_nuttx <programm.elf>     (gebaut per "make test-rvnuttx")
//════════════════════════════════════════════════════════════════════════════════════════════════

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#include "cutils.h"
#include "iomem.h"
#include "riscv_cpu.h"

#include "rvelf.h"
#include "uart16550.h"
#include "clint.h"
#include "plic.h"

#define RAM_BASE     0x80000000ULL
#define RAM_SIZE     (32u * 1024u * 1024u)   /* NuttX rv-virt:nsh verlangt CONFIG_RAM_SIZE=32M */
#define BOOT_BASE    0x1000ULL
#define BOOT_SIZE    0x1000u
#define UART0_BASE   0x10000000ULL
#define CLINT_BASE   0x02000000ULL
#define PLIC_BASE    0x0c000000ULL
/* Q9_PLIC-QUELLENZAHL FUER UART0: 10, NICHT die 37 aus CONFIG_16550_UART0_IRQ=37 in der
   .config! NuttX zaehlt IRQ-Nummern DURCHGEHEND -- erst die internen CSR-Ausnahmen/Traps
   (RISCV_MAX_EXCEPTION=15, plus 1 = RISCV_IRQ_ASYNC=16 als erste asynchrone Nummer), dann die
   externen PLIC-Quellen ab RISCV_IRQ_MEXT = RISCV_IRQ_ASYNC+11 = 27 (M-Mode; im S-Mode waere es
   RISCV_IRQ_SEXT = RISCV_IRQ_ASYNC+9 = 25). Der tatsaechliche PLIC-Quellenindex ist
   irq - RISCV_IRQ_EXT, also 37-27 = 10 -- bestaetigt per Testausgabe: NuttX schreibt beim
   Freischalten der Konsole tatsaechlich Bit 10 in ENABLE1 (0x00000400), nicht Bit 5 (waere
   Quelle 37 gewesen). Ohne diese Umrechnung bleibt die UART-Anforderung fuer immer unbeachtet,
   weil PLIC eine Quelle freischaltet, die niemand jemals anhebt (level[37] statt level[10]). */
#define UART0_IRQ    10u
#define PLIC_CTX_M   0u           /* Machine-Mode Hart 0, s. plic.h */

#define CYCLES_PER_SLICE  2000
#define MAX_SLICES        2000000   /* 4 Mrd. Zyklen Vorrat -- ein interaktiver Gast wartet die
                                        meiste Zeit im Leerlauf, "wfi"-Abschnitte sind billig */

//───────────────────────────────────────────────────────────────────────────────────────────────
// Gemeinsamer Zustand fuer alle Geraete-Rueckrufe: jeder MMIO-Zugriff auf UART oder PLIC, und
// jeder periodische Eingabe-Poll, muss danach MIP_MEIP neu abgleichen koennen (s. Kopf).
//───────────────────────────────────────────────────────────────────────────────────────────────

typedef struct {
    q9_uart16550_t uart;
    q9_plic_t      plic;
    q9_clint_t     clint;
    RISCVCPUState *cpu;          /* bleibt NULL bis nach riscv_cpu_init(), s. rvboard_timer.c */
    int            meip_set;
    int            mtip_set;
} board_t;

static void sync_meip(board_t *b)
{
    q9_plic_set_level(&b->plic, UART0_IRQ, q9_uart16550_irq_pending(&b->uart));
    if (q9_plic_context_pending(&b->plic, PLIC_CTX_M)) {
        if (!b->meip_set) { riscv_cpu_set_mip(b->cpu, MIP_MEIP); b->meip_set = 1; }
    } else {
        if (b->meip_set) { riscv_cpu_reset_mip(b->cpu, MIP_MEIP); b->meip_set = 0; }
    }
}

static void sync_mtip(board_t *b)
{
    if (q9_clint_timer_pending(&b->clint)) {
        if (!b->mtip_set) { riscv_cpu_set_mip(b->cpu, MIP_MTIP); b->mtip_set = 1; }
    } else {
        if (b->mtip_set) { riscv_cpu_reset_mip(b->cpu, MIP_MTIP); b->mtip_set = 0; }
    }
}

//───────────────────────────────────────────────────────────────────────────────────────────────
// UART: Konsole auf stdio. RX per fcntl-O_NONBLOCK, damit das Fehlen von Eingabe kein Blockieren
// verursacht -- der Gast wartet ohnehin per "wfi", nicht der Wirt.
//───────────────────────────────────────────────────────────────────────────────────────────────

static void uart_tx_to_stdout(void *opaque, uint8_t ch)
{
    (void)opaque;
    fputc((int)ch, stdout);
    fflush(stdout);
}

static int uart_rx_from_stdin(void *opaque)
{
    unsigned char c;
    ssize_t n;
    (void)opaque;
    n = read(0, &c, 1);
    return (n == 1) ? (int)c : -1;
}

static uint32_t uart_read(void *opaque, uint32_t offset, int size_log2)
{
    board_t *b = (board_t *)opaque;
    uint32_t v;
    (void)size_log2;
    v = q9_uart16550_read8(&b->uart, offset);
    sync_meip(b);   /* ein RBR-Lesen kann den Vorschaupuffer leeren -> Anforderung faellt weg */
    return v;
}

static void uart_write(void *opaque, uint32_t offset, uint32_t val, int size_log2)
{
    board_t *b = (board_t *)opaque;
    (void)size_log2;
    q9_uart16550_write8(&b->uart, offset, (uint8_t)(val & 0xffu));
    sync_meip(b);   /* IER kann die Anforderung erst freischalten oder sperren */
}

//───────────────────────────────────────────────────────────────────────────────────────────────
// CLINT
//───────────────────────────────────────────────────────────────────────────────────────────────

static uint32_t clint_read(void *opaque, uint32_t offset, int size_log2)
{
    board_t *b = (board_t *)opaque;
    (void)size_log2;
    return q9_clint_read32(&b->clint, offset);
}

static void clint_write(void *opaque, uint32_t offset, uint32_t val, int size_log2)
{
    board_t *b = (board_t *)opaque;
    (void)size_log2;
    q9_clint_write32(&b->clint, offset, val);
    sync_mtip(b);   /* dieselbe Sofort-Disziplin wie in rvboard_timer.c, s. dortige Begruendung */
}

//───────────────────────────────────────────────────────────────────────────────────────────────
// PLIC
//───────────────────────────────────────────────────────────────────────────────────────────────

static uint32_t plic_read(void *opaque, uint32_t offset, int size_log2)
{
    board_t *b = (board_t *)opaque;
    (void)size_log2;
    return q9_plic_read32(&b->plic, offset);   /* CLAIM-Lesen aendert den Anspruchszustand */
}

static void plic_write(void *opaque, uint32_t offset, uint32_t val, int size_log2)
{
    board_t *b = (board_t *)opaque;
    (void)size_log2;
    q9_plic_write32(&b->plic, offset, val);
    sync_meip(b);   /* ENABLE/THRESHOLD/COMPLETE koennen die Anforderung veraendern */
}

//───────────────────────────────────────────────────────────────────────────────────────────────

int main(int argc, char **argv)
{
    rvelf_t ef;
    PhysMemoryMap *map;
    board_t b;
    int slice;

    if (argc < 2) {
        fprintf(stderr, "Aufruf: %s <programm.elf>\n", argv[0]);
        return 2;
    }
    if (rvelf_read(argv[1], &ef) != 0) return 1;

    memset(&b, 0, sizeof(b));
    fcntl(0, F_SETFL, fcntl(0, F_GETFL) | O_NONBLOCK);

    map = phys_mem_map_init();
    cpu_register_ram(map, RAM_BASE,  RAM_SIZE,  0);
    cpu_register_ram(map, BOOT_BASE, BOOT_SIZE, 0);

    q9_uart16550_init(&b.uart, uart_tx_to_stdout, uart_rx_from_stdin, NULL);
    cpu_register_device(map, UART0_BASE, Q9_UART16550_SIZE, &b,
                        uart_read, uart_write, DEVIO_SIZE8 | DEVIO_SIZE16 | DEVIO_SIZE32);

    q9_clint_init(&b.clint);
    cpu_register_device(map, CLINT_BASE, Q9_CLINT_SIZE, &b,
                        clint_read, clint_write, DEVIO_SIZE32);

    q9_plic_init(&b.plic);
    cpu_register_device(map, PLIC_BASE, Q9_PLIC_SIZE, &b,
                        plic_read, plic_write, DEVIO_SIZE32);

    if (rvelf_load_segments(&ef, map) != 0)                    goto fail;
    if (rvelf_place_boot_stub(map, BOOT_BASE, ef.entry) != 0)   goto fail;

    b.cpu = riscv_cpu_init(map, 32);
    if (!b.cpu) { fprintf(stderr, "  CPU laesst sich nicht anlegen\n"); goto fail; }

    for (slice = 0; slice < MAX_SLICES; slice++) {
        q9_clint_advance(&b.clint, CYCLES_PER_SLICE);
        sync_mtip(&b);
        q9_uart16550_poll(&b.uart);   /* neue Eingabe uebernehmen, falls welche ansteht */
        sync_meip(&b);
        riscv_cpu_interp(b.cpu, CYCLES_PER_SLICE);
    }

    riscv_cpu_end(b.cpu);
    phys_mem_map_end(map);
    rvelf_free(&ef);
    return 0;

fail:
    phys_mem_map_end(map);
    rvelf_free(&ef);
    return 1;
}

// EOF rvboard_nuttx.c                                                                       Ver. 1.00
