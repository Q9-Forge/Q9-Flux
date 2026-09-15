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

/* Matches Q9_BOARD_CF2_BASE in Q9-Flux-68k/src/devices/cf/cf.h
 * (RC2014-SC145 second interface). */
#define Q9BOARD_CF2_BASE 0xFFFFC010

/* Matches Q9_MC6845_BASE in Q9-Flux-68k/src/devices/mc6845/mc6845.h. */
#define Q9BOARD_MC6845_BASE 0xFFFFA000

/* Matches Q9_CLUT_BASE in Q9-Flux-68k/src/devices/clut/clut.h. */
#define Q9BOARD_CLUT_BASE 0xFFFFA010

/* Matches Q9_FRAMEBUF_BASE in Q9-Flux-68k/src/devices/framebuf/framebuf.h. */
#define Q9BOARD_FRAMEBUF_BASE 0xFD000000

/* Matches Q9_QUICC_BASE in Q9-Flux-68k/src/devices/quicc/quicc.h. */
#define Q9BOARD_QUICC_BASE 0xFFFF2000

/* devices/quicc/q9_quicc.c, s. there for why this is a plain function
 * rather than a qdev property. */
void q9_quicc_set_ram(DeviceState *dev, uint8_t *ram, uint32_t ram_len);

/* Matches Q9_BOARD_NET_BASE in Q9-Flux-68k/src/devices/nettty/nettty.h. */
#define Q9BOARD_NETTTY_BASE 0xFFFF1000

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

    /* Compact-Flash interface, onboard, fourth ported peripheral -- see
     * Q9-Flux-68kQEMU/devices/cf/q9_cf.c. No CPU/chardev wiring needed
     * (no IRQ). Attach a backing image via e.g.
     * "-global q9-cf.image=/path/to/image.hda" (optionally also
     * "-global q9-cf.format=rbf|pcf|auto", "-global
     * q9-cf.start-sector=N", and the "slave-image"/"slave-format"/
     * "slave-start-sector" equivalents for the second, slave unit on
     * this same interface). Without an image a unit behaves like an
     * empty slot -- every ATA command answers ERR. */
    {
        DeviceState *cf = qdev_new("q9-cf");
        sysbus_realize_and_unref(SYS_BUS_DEVICE(cf), &error_fatal);
        memory_region_add_subregion(
            address_space_mem, Q9BOARD_CF_BASE,
            sysbus_mmio_get_region(SYS_BUS_DEVICE(cf), 0));
    }

    /* Compact-Flash interface, RC2014-SC145 second interface -- same
     * implementation as above (registered a second time under the
     * distinct QOM type name "q9-cf2", s. q9_cf.c's own end-of-file
     * comment), just a second instance at its own base address (matches
     * the original's own q9_cf_attach(), which is likewise interface-
     * agnostic). A distinct type name, rather than a second instance of
     * plain "q9-cf", is what lets "-global" address the two interfaces
     * independently ("-global" keys off the QOM type, not the instance
     * -- two same-typed instances could not otherwise be configured
     * differently from the command line). Configure via
     * "-global q9-cf2.image=..." etc. (same property names as "q9-cf",
     * s. above). */
    {
        DeviceState *cf2 = qdev_new("q9-cf2");
        sysbus_realize_and_unref(SYS_BUS_DEVICE(cf2), &error_fatal);
        memory_region_add_subregion(
            address_space_mem, Q9BOARD_CF2_BASE,
            sysbus_mmio_get_region(SYS_BUS_DEVICE(cf2), 0));
    }

    /* MC6845 CRT controller (GDP framebuffer geometry base), sixth
     * ported peripheral -- see Q9-Flux-68kQEMU/devices/mc6845/q9_mc6845.c.
     * No CPU/chardev/IRQ wiring needed. */
    DeviceState *mc6845 = qdev_new("q9-mc6845");
    {
        sysbus_realize_and_unref(SYS_BUS_DEVICE(mc6845), &error_fatal);
        memory_region_add_subregion(
            address_space_mem, Q9BOARD_MC6845_BASE,
            sysbus_mmio_get_region(SYS_BUS_DEVICE(mc6845), 0));
    }

    /* CLUT (colour lookup table for indexed video modes), seventh
     * ported peripheral -- see Q9-Flux-68kQEMU/devices/clut/q9_clut.c.
     * No CPU/chardev/IRQ wiring needed. */
    DeviceState *clut = qdev_new("q9-clut");
    {
        sysbus_realize_and_unref(SYS_BUS_DEVICE(clut), &error_fatal);
        memory_region_add_subregion(
            address_space_mem, Q9BOARD_CLUT_BASE,
            sysbus_mmio_get_region(SYS_BUS_DEVICE(clut), 0));
    }

    /* VRAM framebuffer, eighth ported peripheral -- see
     * Q9-Flux-68kQEMU/devices/framebuf/q9_framebuf.c. Linked to the
     * MC6845 above (its "mc6845" property) purely to read its stride
     * register (R1) for dirty-rectangle row mapping -- no IRQ. Size
     * defaults to the original's own 1 MiB default
     * (Q9_FRAMEBUF_DEFAULT_SIZE); override with e.g.
     * "-global q9-framebuf.size=4194304" (max 16 MiB, s. q9_framebuf.c). */
    DeviceState *fb = qdev_new("q9-framebuf");
    {
        object_property_set_link(OBJECT(fb), "mc6845", OBJECT(mc6845),
                                  &error_abort);
        sysbus_realize_and_unref(SYS_BUS_DEVICE(fb), &error_fatal);
        /* $FD000000 falls inside the ROM mirror's 0..$FEFFFFFF span (s.
         * the #define block above) -- unlike every earlier device's
         * window, which all sit above it. A plain add_subregion() would
         * assert on that overlap; add it with priority clearly above
         * the mirror's (1) and window's (2) instead, so the framebuffer
         * always wins whether or not "-bios" is given -- matching the
         * original, where devreg-registered devices are dispatched
         * before the board's own ROM/RAM fallback is ever consulted
         * (s. remap.c's own history comment). */
        memory_region_add_subregion_overlap(
            address_space_mem, Q9BOARD_FRAMEBUF_BASE,
            sysbus_mmio_get_region(SYS_BUS_DEVICE(fb), 0), 10);
    }

    /* QUICC Ethernet (MC68360 SCC1), ninth ported peripheral -- see
     * Q9-Flux-68kQEMU/devices/quicc/q9_quicc.c. Needs the CPU object
     * (same "m68k-cpu" link pattern as timer_irq/duart) for its level-5
     * IRQ, and a host pointer into guest RAM for the buffer-descriptor
     * rings' SDMA-style frame transfer (q9_quicc_set_ram(), same
     * rationale as q9_mc6845_get_stride()). This is a standard QEMU NIC
     * frontend -- give it a network with the usual "-netdev"/"-nic"
     * machinery, e.g.
     * "-netdev user,id=net0 -global q9-quicc.netdev=net0" (see
     * q9_quicc.c's own header comment for why this replaces the
     * original's four hand-rolled host network backends outright). */
    {
        DeviceState *quicc = qdev_new("q9-quicc");
        object_property_set_link(OBJECT(quicc), "m68k-cpu", OBJECT(cpu),
                                  &error_abort);
        sysbus_realize_and_unref(SYS_BUS_DEVICE(quicc), &error_fatal);
        q9_quicc_set_ram(quicc, memory_region_get_ram_ptr(machine->ram),
                          (uint32_t)memory_region_size(machine->ram));
        memory_region_add_subregion(
            address_space_mem, Q9BOARD_QUICC_BASE,
            sysbus_mmio_get_region(SYS_BUS_DEVICE(quicc), 0));
    }

    /* Network terminals (8 channels /x1../x8), tenth ported peripheral --
     * see Q9-Flux-68kQEMU/devices/nettty/q9_nettty.c. Needs the CPU
     * object (same "m68k-cpu" link pattern as timer_irq/duart/quicc) for
     * its shared level-4 IRQ. Each channel is its own "chardevN" (N=0-7)
     * property, attached the usual QEMU way, e.g.
     * "-chardev socket,id=x1,port=2001,server=on,wait=off,telnet=on
     *  -global q9-nettty.chardev0=x1" (repeat per channel/port; see
     * q9_nettty.c's own header comment for why this replaces the
     * original's single dynamic-port dispatcher). Channels with no
     * chardev attached simply never connect (client_fd stays unset). */
    {
        DeviceState *nettty = qdev_new("q9-nettty");
        object_property_set_link(OBJECT(nettty), "m68k-cpu", OBJECT(cpu),
                                  &error_abort);
        sysbus_realize_and_unref(SYS_BUS_DEVICE(nettty), &error_fatal);
        memory_region_add_subregion(
            address_space_mem, Q9BOARD_NETTTY_BASE,
            sysbus_mmio_get_region(SYS_BUS_DEVICE(nettty), 0));
    }

    /* Host video bridge (Q9 Frame protocol), eleventh and final ported
     * peripheral -- see Q9-Flux-68kQEMU/devices/videobridge/
     * q9_videobridge.c. Not a guest-visible device at all (no
     * MemoryRegion, s. its own header comment) -- a host-side background
     * service linked to the framebuffer/CRTC/CLUT above, streaming their
     * state to an external Q9 Frame viewer over TCP+UDP (ports
     * overridable via "-global q9-videobridge.tcp-port=..."/"...udp-
     * port=..."). "clut" is intentionally optional (falls back to a
     * grayscale ramp, matching the original). */
    {
        DeviceState *vb = qdev_new("q9-videobridge");
        object_property_set_link(OBJECT(vb), "framebuf", OBJECT(fb),
                                  &error_abort);
        object_property_set_link(OBJECT(vb), "mc6845", OBJECT(mc6845),
                                  &error_abort);
        object_property_set_link(OBJECT(vb), "clut", OBJECT(clut),
                                  &error_abort);
        qdev_realize_and_unref(vb, NULL, &error_fatal);
    }

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
