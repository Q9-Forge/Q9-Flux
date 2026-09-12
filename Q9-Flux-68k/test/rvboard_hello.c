//════════════════════════════════════════════════════════════════════════════════════════════════
// File:   rvboard_hello.c                                                                 Ver. 1.00
// Owner:  Claudia
// Desc.:  Lebenszeichen auf einem minimalen RISC-V-Board: RAM + ein 16550-UART, sonst nichts.
//         Zweck ist NICHT, ein Programm laufen zu lassen -- das kann der ISA-Prueflauf laengst --
//         sondern zwei bisher UNGEPRUEFTE Wege zu belegen:
//           1. den GERAETE-Rueckrufpfad des Kerns (cpu_register_device mit eigenen read/write-
//              Funktionen). Der ISA-Prueflauf braucht nur RAM und hat ihn nie angefasst.
//           2. dass wir eigene Gastprogramme durchgaengig bauen koennen (Quelle -> Toolchain ->
//              ELF -> laeuft), statt nur fertige Binaerdateien der Suite abzuspielen.
//
//         Speicherkarte nach QEMU "virt" -- die De-facto-Standardmaschine der RISC-V-Welt:
//           0x0000_1000  Reset-PC des Kerns, traegt den Sprungbefehl (s. rvelf_place_boot_stub)
//           0x1000_0000  UART0, NS16550A, Register im Abstand 1 Byte
//           0x8000_0000  RAM
//         Wer diese Lage einhaelt, kann spaeter dieselben Abbilder benutzen wie
//         qemu-system-riscv32 -M virt -- und gegen QEMU als unabhaengiges Orakel gegenpruefen.
//
// Call:   build/<platform>/rvboard_hello <programm.elf>     (gebaut per "make test-rvboard")
//════════════════════════════════════════════════════════════════════════════════════════════════

#include <stdio.h>
#include <string.h>

#include "cutils.h"
#include "iomem.h"
#include "riscv_cpu.h"

#include "rvelf.h"
#include "uart16550.h"

#define RAM_BASE    0x80000000ULL
#define RAM_SIZE    (16u * 1024u * 1024u)
#define BOOT_BASE   0x1000ULL
#define BOOT_SIZE   0x1000u
#define UART0_BASE  0x10000000ULL

#define CYCLES_PER_SLICE  100000
#define MAX_SLICES        2000

//───────────────────────────────────────────────────────────────────────────────────────────────
// Ankopplung des UART an den Kern. Der Kern liefert Byte-Offsets innerhalb des Fensters und
// eine Groessenangabe als Zweierlogarithmus (size_log2: 0=8, 1=16, 2=32 Bit).
//───────────────────────────────────────────────────────────────────────────────────────────────

static void uart_tx_to_stdout(void *opaque, uint8_t ch)
{
    (void)opaque;
    fputc((int)ch, stdout);
    fflush(stdout);          /* sofort sichtbar -- sonst haengt die Ausgabe im Puffer, wenn
                                der Gast in eine Schleife laeuft */
}

static uint32_t uart_read(void *opaque, uint32_t offset, int size_log2)
{
    (void)size_log2;         /* alle 16550-Register sind ein Byte breit */
    return q9_uart16550_read8((q9_uart16550_t *)opaque, offset);
}

static void uart_write(void *opaque, uint32_t offset, uint32_t val, int size_log2)
{
    (void)size_log2;
    q9_uart16550_write8((q9_uart16550_t *)opaque, offset, (uint8_t)(val & 0xffu));
}

//───────────────────────────────────────────────────────────────────────────────────────────────

int main(int argc, char **argv)
{
    rvelf_t ef;
    PhysMemoryMap *map;
    RISCVCPUState *cpu;
    q9_uart16550_t uart;
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

    if (rvelf_load_segments(&ef, map) != 0)                    goto fail;
    if (rvelf_place_boot_stub(map, BOOT_BASE, ef.entry) != 0)   goto fail;

    cpu = riscv_cpu_init(map, 32);
    if (!cpu) { fprintf(stderr, "  CPU laesst sich nicht anlegen\n"); goto fail; }

    for (slice = 0; slice < MAX_SLICES; slice++) {
        riscv_cpu_interp(cpu, CYCLES_PER_SLICE);
        if (riscv_cpu_get_power_down(cpu)) { halted = 1; break; }   /* wfi im Gast = fertig */
    }

    printf("\n  %s nach %llu Zyklen\n",
           halted ? "angehalten (wfi)" : "Zyklenvorrat erschoepft -- kein wfi erreicht",
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

// EOF rvboard_hello.c                                                                      Ver. 1.00
