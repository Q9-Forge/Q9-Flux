/*
 * Q9 board emulation.
 *
 * Board assembly for the planned migration of the Q9-Flux 68K board
 * model from Musashi to QEMU. Peripheral devices live separately under
 * Q9-Flux-68kQEMU/devices/ (see qemu-mapping.conf for how they land in
 * this QEMU tree) -- this file only wires them together, the same role
 * Q9-Flux-68k/src/kernel/q9board.c/boardcfg.c play in the Musashi
 * branch. See Q9-Flux-68kQEMU/docs/HOSTFS_MANAGER.md and
 * QEMU_DEVICE_MODEL.md for the full design background.
 *
 * This file is not (yet) part of Q9-Flux's own MIT-licensed code base --
 * it lives inside the vendored QEMU source tree and is licensed under
 * QEMU's own terms (GPL), like the rest of this directory.
 */

#include "qemu/osdep.h"
#include "qapi/error.h"
#include "cpu.h"
#include "hw/core/boards.h"
#include "hw/core/loader.h"
#include "hw/core/sysbus.h"
#include "hw/core/qdev-properties.h"
#include "elf.h"
#include "qemu/error-report.h"
#include "system/qtest.h"
#include "system/system.h"

#define Q9BOARD_KERNEL_LOAD_ADDR 0x10000

/* Matches Q9_BOARD_ROM_MIRROR_TOP/_ROM_REMAP_BASE/_TOP in
 * Q9-Flux-68k/src/kernel/q9board.h. The ROM/RAM address-space topology
 * these implement is deliberately kept here rather than in
 * devices/remap/q9_remap.c, matching remap.h's own header comment: it's
 * the board's address-space layout, not window peripherality. Only
 * created when a ROM/firmware image is actually given via "-bios" --
 * without one, RAM stays directly visible at address 0 as before (s.
 * q9board_init below for why: unlike the real board, which never runs
 * without a boot ROM, every -kernel-based device test so far loads
 * straight into RAM at Q9BOARD_KERNEL_LOAD_ADDR and jumps there
 * directly, bypassing ROM/reset semantics entirely -- shadowing address
 * 0 unconditionally would silently break all of that). */
#define Q9BOARD_ROM_MIRROR_SIZE 0xFF000000u  /* 0..Q9_BOARD_ROM_MIRROR_TOP incl. */
#define Q9BOARD_ROM_REMAP_BASE  0xFE000000u
#define Q9BOARD_ROM_REMAP_SIZE  0x00080000u  /* Q9_BOARD_ROM_REMAP_TOP - _BASE + 1 */

/* Matches Q9_BOARD_REMAP_REG_BASE in
 * Q9-Flux-68k/src/devices/remap/remap.h. */
#define Q9BOARD_REMAP_REG_BASE 0xFFFF8000u

/* devices/remap/q9_remap.c -- no shared header in this QEMU-side
 * devices/ tree (every device so far is a single self-contained .c),
 * so this one cross-device call is declared directly where it's used,
 * like q9board.c's other qdev property wiring just above it. */
void q9_remap_set_targets(DeviceState *dev, MemoryRegion *rom_mirror,
                           MemoryRegion *rom_window);

typedef struct {
    const uint8_t *rom;
    uint32_t rom_len;
} Q9RomCtx;

static uint64_t q9board_rom_mirror_read(void *opaque, hwaddr addr, unsigned size)
{
    Q9RomCtx *ctx = opaque;
    uint64_t val = 0;
    unsigned i;

    if (ctx->rom_len == 0) {
        return 0;
    }
    for (i = 0; i < size; i++) {
        val = (val << 8) | ctx->rom[(addr + i) % ctx->rom_len];
    }
    return val;
}

static uint64_t q9board_rom_window_read(void *opaque, hwaddr addr, unsigned size)
{
    Q9RomCtx *ctx = opaque;
    uint64_t val = 0;
    unsigned i;

    for (i = 0; i < size; i++) {
        uint8_t b = (addr + i < ctx->rom_len) ? ctx->rom[addr + i] : 0;
        val = (val << 8) | b;
    }
    return val;
}

/* Both ROM regions are read-only in both board states (s. Dateikopf and
 * the original's board_write_byte, which never writes to either the
 * mirror or the fixed remap window) -- writes are simply discarded. */
static void q9board_rom_write_discard(void *opaque, hwaddr addr, uint64_t val,
                                       unsigned size)
{
    (void)opaque;
    (void)addr;
    (void)val;
    (void)size;
}

static const MemoryRegionOps q9board_rom_mirror_ops = {
    .read = q9board_rom_mirror_read,
    .write = q9board_rom_write_discard,
    .valid.min_access_size = 1,
    .valid.max_access_size = 4,
    .impl.min_access_size = 1,
    .impl.max_access_size = 4,
    .endianness = DEVICE_BIG_ENDIAN,
};

static const MemoryRegionOps q9board_rom_window_ops = {
    .read = q9board_rom_window_read,
    .write = q9board_rom_write_discard,
    .valid.min_access_size = 1,
    .valid.max_access_size = 4,
    .impl.min_access_size = 1,
    .impl.max_access_size = 4,
    .endianness = DEVICE_BIG_ENDIAN,
};

/* Matches Q9_BOARD_RTC_BASE in Q9-Flux-68k/src/kernel/q9board.h. */
#define Q9BOARD_RTC_BASE 0xFFFFD000

/* Matches Q9_BOARD_TIRQ_OFF_BASE in Q9-Flux-68k/src/kernel/q9board.h (the
 * ON window is the upper half of the same 0x1000-byte device region, see
 * Q9-Flux-68kQEMU/devices/timer_irq/q9_timer_irq.c). */
#define Q9BOARD_TIMER_IRQ_BASE 0xFFFF9000

/* Matches Q9_BOARD_UART_BASE in Q9-Flux-68k/src/kernel/q9board.h. */
#define Q9BOARD_UART_BASE 0xFFFFF000

/* Matches Q9_BOARD_CF_BASE in Q9-Flux-68k/src/devices/cf/cf.h. */
#define Q9BOARD_CF_BASE 0xFFFFE000

static void q9board_init(MachineState *machine)
{
    ram_addr_t ram_size = machine->ram_size;
    const char *kernel_filename = machine->kernel_filename;
    M68kCPU *cpu;
    CPUM68KState *env;
    int kernel_size;
    uint64_t elf_entry;
    hwaddr entry;
    MemoryRegion *address_space_mem = get_system_memory();

    cpu = M68K_CPU(cpu_create(machine->cpu_type));
    env = &cpu->env;
    env->vbr = 0;

    /* RAM at address zero, matching the real board's reset convention
     * (initial SSP/PC read from address 0/4). */
    memory_region_add_subregion(address_space_mem, 0, machine->ram);

    /* ROM mirror / remap window (s. #define block above), REMAP trigger
     * is the fifth ported peripheral -- see
     * Q9-Flux-68kQEMU/devices/remap/q9_remap.c. Only set up when a ROM/
     * firmware image is given via "-bios"; otherwise skipped entirely
     * (RAM stays directly visible at 0, s. the #define block's own
     * comment for why). */
    {
        DeviceState *remap = qdev_new("q9-remap");
        MemoryRegion *rom_mirror = NULL;
        MemoryRegion *rom_window = NULL;

        if (machine->firmware) {
            gchar *rom_data = NULL;
            gsize  rom_len  = 0;
            Q9RomCtx *ctx;

            if (!g_file_get_contents(machine->firmware, &rom_data, &rom_len,
                                      NULL)) {
                error_report("Could not load ROM image '%s'", machine->firmware);
                exit(1);
            }

            ctx = g_new0(Q9RomCtx, 1);
            ctx->rom = (const uint8_t *)rom_data;   /* g_file_get_contents: never freed, s.o. */
            ctx->rom_len = (uint32_t)rom_len;

            rom_mirror = g_new0(MemoryRegion, 1);
            memory_region_init_io(rom_mirror, OBJECT(remap),
                                   &q9board_rom_mirror_ops, ctx,
                                   "q9board.rom-mirror", Q9BOARD_ROM_MIRROR_SIZE);
            memory_region_add_subregion_overlap(address_space_mem, 0,
                                                 rom_mirror, 1);

            rom_window = g_new0(MemoryRegion, 1);
            memory_region_init_io(rom_window, OBJECT(remap),
                                   &q9board_rom_window_ops, ctx,
                                   "q9board.rom-window", Q9BOARD_ROM_REMAP_SIZE);
            memory_region_add_subregion_overlap(address_space_mem,
                                                 Q9BOARD_ROM_REMAP_BASE,
                                                 rom_window, 2);
            memory_region_set_enabled(rom_window, false);
        }

        q9_remap_set_targets(remap, rom_mirror, rom_window);
        sysbus_realize_and_unref(SYS_BUS_DEVICE(remap), &error_fatal);
        memory_region_add_subregion(
            address_space_mem, Q9BOARD_REMAP_REG_BASE,
            sysbus_mmio_get_region(SYS_BUS_DEVICE(remap), 0));
    }

    /* RTC72421 real-time clock, first ported peripheral -- see
     * Q9-Flux-68kQEMU/devices/rtc72421/q9_rtc72421.c. */
    {
        DeviceState *rtc = qdev_new("q9-rtc72421");
        sysbus_realize_and_unref(SYS_BUS_DEVICE(rtc), &error_fatal);
        memory_region_add_subregion(
            address_space_mem, Q9BOARD_RTC_BASE,
            sysbus_mmio_get_region(SYS_BUS_DEVICE(rtc), 0));
    }

    /* Timer/IRQ3 address-trigger, second ported peripheral -- see
     * Q9-Flux-68kQEMU/devices/timer_irq/q9_timer_irq.c. Needs the CPU
     * object to raise/lower the level-6 autovector interrupt itself,
     * passed via the standard "m68k-cpu" link property (same pattern as
     * upstream's an5206.c -> mcf5206_init()). */
    {
        DeviceState *tirq = qdev_new("q9-timer-irq");
        object_property_set_link(OBJECT(tirq), "m68k-cpu", OBJECT(cpu),
                                  &error_abort);
        sysbus_realize_and_unref(SYS_BUS_DEVICE(tirq), &error_fatal);
        memory_region_add_subregion(
            address_space_mem, Q9BOARD_TIMER_IRQ_BASE,
            sysbus_mmio_get_region(SYS_BUS_DEVICE(tirq), 0));
    }

    /* 68681 DUART (console, channel A only), third ported peripheral --
     * see Q9-Flux-68kQEMU/devices/duart68681/q9_duart68681.c. Needs the
     * CPU object (same "m68k-cpu" link pattern as timer_irq) to raise/
     * lower its level-3 vectored interrupt, plus a chardev backend for
     * the host-facing console -- serial_hd(0) is whatever "-serial"
     * selects on the command line (defaults to the QEMU console when
     * none is given). */
    {
        DeviceState *duart = qdev_new("q9-duart68681");
        object_property_set_link(OBJECT(duart), "m68k-cpu", OBJECT(cpu),
                                  &error_abort);
        qdev_prop_set_chr(duart, "chardev", serial_hd(0));
        sysbus_realize_and_unref(SYS_BUS_DEVICE(duart), &error_fatal);
        memory_region_add_subregion(
            address_space_mem, Q9BOARD_UART_BASE,
            sysbus_mmio_get_region(SYS_BUS_DEVICE(duart), 0));
    }

    /* Compact-Flash interface (master unit only, s. q9_cf.c Dateikopf),
     * fourth ported peripheral -- see Q9-Flux-68kQEMU/devices/cf/q9_cf.c.
     * No CPU/chardev wiring needed (no IRQ). Attach a backing image via
     * e.g. "-global q9-cf.image=/path/to/image.hda" (optionally also
     * "-global q9-cf.format=rbf|pcf|auto" and
     * "-global q9-cf.start-sector=N"). Without an image the unit behaves
     * like an empty slot -- every ATA command answers ERR. */
    {
        DeviceState *cf = qdev_new("q9-cf");
        sysbus_realize_and_unref(SYS_BUS_DEVICE(cf), &error_fatal);
        memory_region_add_subregion(
            address_space_mem, Q9BOARD_CF_BASE,
            sysbus_mmio_get_region(SYS_BUS_DEVICE(cf), 0));
    }

    /* TODO (future sessions): QUICC, nettty,
     * MC6845/framebuf/CLUT/videobridge -- ported from
     * Q9-Flux-68k/src/devices/, one at a time. */

    if (!kernel_filename) {
        if (qtest_enabled()) {
            return;
        }
        error_report("Kernel image must be specified");
        exit(1);
    }

    kernel_size = load_elf(kernel_filename, NULL, NULL, NULL, &elf_entry,
                            NULL, NULL, NULL, ELFDATA2MSB, EM_68K, 0, 0);
    entry = elf_entry;
    if (kernel_size < 0) {
        kernel_size = load_image_targphys(kernel_filename,
                                           Q9BOARD_KERNEL_LOAD_ADDR,
                                           ram_size - Q9BOARD_KERNEL_LOAD_ADDR,
                                           NULL);
        entry = Q9BOARD_KERNEL_LOAD_ADDR;
    }
    if (kernel_size < 0) {
        error_report("Could not load kernel '%s'", kernel_filename);
        exit(1);
    }

    env->pc = entry;
}

static void q9board_machine_init(MachineClass *mc)
{
    mc->desc = "Q9 board (skeleton, CPU+RAM only)";
    mc->init = q9board_init;
    mc->default_cpu_type = M68K_CPU_TYPE_NAME("m68030");
    mc->default_ram_id = "q9board.ram";
    mc->default_ram_size = 32 * 1024 * 1024;
}

DEFINE_MACHINE("q9board", q9board_machine_init)
