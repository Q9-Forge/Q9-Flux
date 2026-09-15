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

/* Matches Q9_BOARD_RTC_BASE in Q9-Flux-68k/src/kernel/q9board.h. */
#define Q9BOARD_RTC_BASE 0xFFFFD000

/* Matches Q9_BOARD_TIRQ_OFF_BASE in Q9-Flux-68k/src/kernel/q9board.h (the
 * ON window is the upper half of the same 0x1000-byte device region, see
 * Q9-Flux-68kQEMU/devices/timer_irq/q9_timer_irq.c). */
#define Q9BOARD_TIMER_IRQ_BASE 0xFFFF9000

/* Matches Q9_BOARD_UART_BASE in Q9-Flux-68k/src/kernel/q9board.h. */
#define Q9BOARD_UART_BASE 0xFFFFF000

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

    /* TODO (future sessions): CF, QUICC, remap trigger, nettty,
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
