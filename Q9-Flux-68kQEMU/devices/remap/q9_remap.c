/*
 * Q9 board: REMAP register (pure address trigger).
 *
 * Ported from the Musashi-based implementation in
 * Q9-Flux-68k/src/devices/remap/remap.c -- a read or write anywhere in
 * $FFFF8000-$FFFF8FFF switches the board from the reset state (ROM
 * mirrored at address 0) to the remapped state (RAM at address 0, ROM
 * once at $FE000000-$FE07FFFF), unconditionally and irreversibly. Read
 * value is always 0 (no real register behind it); writes have no data
 * effect either -- only the access itself matters.
 *
 * The original deliberately keeps the actual ROM/RAM address-space
 * topology OUT of this file (s. remap.h's own header comment): that
 * belongs to the board, not to a window peripheral. This port keeps the
 * same split -- this device only knows it must flip two MemoryRegions
 * when triggered, not what they contain. q9board.c creates those two
 * regions (the ROM mirror and the ROM's remapped-position window) and
 * hands them over with q9_remap_set_targets() before the machine starts
 * running; either may be NULL (no ROM/firmware image was given via
 * "-bios"), in which case triggering the register is a harmless no-op,
 * same as the original with rom_len==0.
 *
 * No IRQ, matching the original.
 *
 * This file is not (yet) part of Q9-Flux's own MIT-licensed code base --
 * it lives inside the vendored QEMU source tree and is licensed under
 * QEMU's own terms (GPL), like the rest of this directory.
 */

#include "qemu/osdep.h"
#include "hw/core/sysbus.h"
#include "qom/object.h"

#define TYPE_Q9_REMAP "q9-remap"
OBJECT_DECLARE_SIMPLE_TYPE(Q9RemapState, Q9_REMAP)

/* Matches Q9_BOARD_REMAP_REG_BASE/_TOP in
 * Q9-Flux-68k/src/devices/remap/remap.h. */
#define Q9_REMAP_WINDOW_SIZE 0x1000

struct Q9RemapState {
    SysBusDevice parent_obj;
    MemoryRegion iomem;

    MemoryRegion *rom_mirror;   /* board-owned, s. Dateikopf; may be NULL */
    MemoryRegion *rom_window;   /* board-owned, s. Dateikopf; may be NULL */
    bool triggered;
};

void q9_remap_set_targets(DeviceState *dev, MemoryRegion *rom_mirror,
                           MemoryRegion *rom_window);

void q9_remap_set_targets(DeviceState *dev, MemoryRegion *rom_mirror,
                           MemoryRegion *rom_window)
{
    Q9RemapState *s = Q9_REMAP(dev);

    s->rom_mirror = rom_mirror;
    s->rom_window = rom_window;
}

static void q9_remap_trigger(Q9RemapState *s)
{
    if (s->triggered) {
        return;   /* one-way latch, s. Dateikopf -- further accesses are no-ops */
    }
    s->triggered = true;
    if (s->rom_mirror) {
        memory_region_set_enabled(s->rom_mirror, false);
    }
    if (s->rom_window) {
        memory_region_set_enabled(s->rom_window, true);
    }
}

static uint64_t q9_remap_read(void *opaque, hwaddr addr, unsigned size)
{
    (void)addr;
    (void)size;
    q9_remap_trigger(opaque);
    return 0;
}

static void q9_remap_write(void *opaque, hwaddr addr, uint64_t val,
                            unsigned size)
{
    (void)addr;
    (void)val;
    (void)size;
    q9_remap_trigger(opaque);
}

static const MemoryRegionOps q9_remap_ops = {
    .read = q9_remap_read,
    .write = q9_remap_write,
    .valid.min_access_size = 1,
    .valid.max_access_size = 4,
    .impl.min_access_size = 1,
    .impl.max_access_size = 4,
    .endianness = DEVICE_BIG_ENDIAN,
};

static void q9_remap_realize(DeviceState *dev, Error **errp)
{
    Q9RemapState *s = Q9_REMAP(dev);

    memory_region_init_io(&s->iomem, OBJECT(dev), &q9_remap_ops, s,
                           TYPE_Q9_REMAP, Q9_REMAP_WINDOW_SIZE);
    sysbus_init_mmio(SYS_BUS_DEVICE(dev), &s->iomem);
}

static void q9_remap_class_init(ObjectClass *oc, const void *data)
{
    DeviceClass *dc = DEVICE_CLASS(oc);

    dc->desc = "Q9 board REMAP register (ROM-mirror -> RAM switch trigger)";
    dc->realize = q9_remap_realize;
    set_bit(DEVICE_CATEGORY_MISC, dc->categories);
}

static const TypeInfo q9_remap_info = {
    .name          = TYPE_Q9_REMAP,
    .parent        = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(Q9RemapState),
    .class_init    = q9_remap_class_init,
};

static void q9_remap_register_types(void)
{
    type_register_static(&q9_remap_info);
}

type_init(q9_remap_register_types)
