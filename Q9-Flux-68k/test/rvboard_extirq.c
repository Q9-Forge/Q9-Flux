//════════════════════════════════════════════════════════════════════════════════════════════════
// File:   rvboard_extirq.c                                                                Ver. 1.00
// Owner:  Claudia
// Desc.:  Eigenstaendige Regressionsabsicherung fuer den Stufe-3-Kernfehler (mie-Schreibmaske
//         ohne MIP_MEIP, s. test/riscv/extirq/extirq_test.c und docs/RISCV.md). BEWUSST OHNE
//         CLINT -- dieses Board isoliert genau den externen Interruptpfad (PLIC + mie.MEIE),
//         nichts sonst. Braucht nur die normale riscv64-elf-gcc-Toolchain, nicht NuttX' schwere
//         Werkzeugkette (xPack, kconfig-tweak, genromfs, flock) -- lauffaehig auch dort, wo diese
//         fehlen.
//
// Call:   build/<platform>/rvboard_extirq <programm.elf>   (gebaut per "make test-rvextirq")
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
#include "plic.h"

#define RAM_BASE    0x80000000ULL
#define RAM_SIZE    (2u * 1024u * 1024u)
#define BOOT_BASE   0x1000ULL
#define BOOT_SIZE   0x1000u
#define UART0_BASE  0x10000000ULL
#define PLIC_BASE   0x0c000000ULL
#define UART_PLIC_SOURCE  1u    /* muss zu extirq_test.c passen */
#define PLIC_CTX_M  0u

#define CYCLES_PER_SLICE  2000
#define MAX_SLICES        200000   /* 400 Mio. Zyklen -- reichlich fuer 5 Zeichen */

typedef struct {
    q9_uart16550_t uart;
    q9_plic_t      plic;
    RISCVCPUState *cpu;
    int            meip_set;
} board_t;

static void sync_meip(board_t *b)
{
    q9_plic_set_level(&b->plic, UART_PLIC_SOURCE, q9_uart16550_irq_pending(&b->uart));
    if (q9_plic_context_pending(&b->plic, PLIC_CTX_M)) {
        if (!b->meip_set) { riscv_cpu_set_mip(b->cpu, MIP_MEIP); b->meip_set = 1; }
    } else {
        if (b->meip_set) { riscv_cpu_reset_mip(b->cpu, MIP_MEIP); b->meip_set = 0; }
    }
}

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
    sync_meip(b);
    return v;
}

static void uart_write(void *opaque, uint32_t offset, uint32_t val, int size_log2)
{
    board_t *b = (board_t *)opaque;
    (void)size_log2;
    q9_uart16550_write8(&b->uart, offset, (uint8_t)(val & 0xffu));
    sync_meip(b);
}

static uint32_t plic_read(void *opaque, uint32_t offset, int size_log2)
{
    board_t *b = (board_t *)opaque;
    (void)size_log2;
    return q9_plic_read32(&b->plic, offset);
}

static void plic_write(void *opaque, uint32_t offset, uint32_t val, int size_log2)
{
    board_t *b = (board_t *)opaque;
    (void)size_log2;
    q9_plic_write32(&b->plic, offset, val);
    sync_meip(b);
}

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

    q9_plic_init(&b.plic);
    cpu_register_device(map, PLIC_BASE, Q9_PLIC_SIZE, &b,
                        plic_read, plic_write, DEVIO_SIZE32);

    if (rvelf_load_segments(&ef, map) != 0)                    goto fail;
    if (rvelf_place_boot_stub(map, BOOT_BASE, ef.entry) != 0)   goto fail;

    b.cpu = riscv_cpu_init(map, 32);
    if (!b.cpu) { fprintf(stderr, "  CPU laesst sich nicht anlegen\n"); goto fail; }

    for (slice = 0; slice < MAX_SLICES; slice++) {
        q9_uart16550_poll(&b.uart);
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

// EOF rvboard_extirq.c                                                                     Ver. 1.00
