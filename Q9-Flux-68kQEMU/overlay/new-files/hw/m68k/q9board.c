/*
 * Q9 board emulation (skeleton).
 *
 * Minimal starting point for the planned migration of the Q9-Flux 68K
 * board model from Musashi to QEMU. Currently: CPU + RAM only, no
 * peripherals yet. See Q9-Flux-68kQEMU/docs/HOSTFS_MANAGER.md and
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
#include "elf.h"
#include "qemu/error-report.h"
#include "system/qtest.h"

#define Q9BOARD_KERNEL_LOAD_ADDR 0x10000

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

    /* TODO (future sessions): CF, QUICC, RTC72421, 68681 DUART, timer
     * IRQ3, remap trigger, nettty, MC6845/framebuf/CLUT/videobridge --
     * ported from Q9-Flux-68k/src/devices/, one at a time. */

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
