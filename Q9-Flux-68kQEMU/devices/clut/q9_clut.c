/*
 * Q9 board: colour lookup table (CLUT) for indexed video modes.
 *
 * Ported from the Musashi-based implementation in
 * Q9-Flux-68k/src/devices/clut/clut.c -- register map and semantics are
 * unchanged: offset 0 is the index register (R/W, same idea as MC6845's
 * address register), offsets 1/2/3 are R/G/B of the selected entry.
 * Every write to R/G/B bumps `generation` unconditionally (no compare
 * against the old value) -- the video bridge only ever needs "may have
 * changed since last send", not an exact diff, s. the original's own
 * header comment. No IRQ. Reset state is the identity ramp
 * (r=g=b=index), matching q9_clut_init().
 *
 * This file is not (yet) part of Q9-Flux's own MIT-licensed code base --
 * it lives inside the vendored QEMU source tree and is licensed under
 * QEMU's own terms (GPL), like the rest of this directory.
 */

#include "qemu/osdep.h"
#include "hw/core/sysbus.h"
#include "qom/object.h"

#define TYPE_Q9_CLUT "q9-clut"
OBJECT_DECLARE_SIMPLE_TYPE(Q9ClutState, Q9_CLUT)

/* Matches Q9_CLUT_BASE/_TOP/_ENTRIES in
 * Q9-Flux-68k/src/devices/clut/clut.h. */
#define Q9_CLUT_WINDOW_SIZE 4u
#define Q9_CLUT_ENTRIES     256u

struct Q9ClutState {
    SysBusDevice parent_obj;
    MemoryRegion iomem;

    uint8_t  r[Q9_CLUT_ENTRIES];
    uint8_t  g[Q9_CLUT_ENTRIES];
    uint8_t  b[Q9_CLUT_ENTRIES];
    uint8_t  index;
    uint32_t generation;
};

static uint64_t q9_clut_read(void *opaque, hwaddr addr, unsigned size)
{
    Q9ClutState *s = opaque;

    switch (addr) {
    case 0: return s->index;
    case 1: return s->r[s->index];
    case 2: return s->g[s->index];
    case 3: return s->b[s->index];
    default: return 0;
    }
}

static void q9_clut_write(void *opaque, hwaddr addr, uint64_t val,
                           unsigned size)
{
    Q9ClutState *s = opaque;

    switch (addr) {
    case 0:
        s->index = (uint8_t)val;
        break;
    case 1:
        s->r[s->index] = (uint8_t)val;
        s->generation++;
        break;
    case 2:
        s->g[s->index] = (uint8_t)val;
        s->generation++;
        break;
    case 3:
        s->b[s->index] = (uint8_t)val;
        s->generation++;
        break;
    default:
        break;
    }
}

static const MemoryRegionOps q9_clut_ops = {
    .read = q9_clut_read,
    .write = q9_clut_write,
    .valid.min_access_size = 1,
    .valid.max_access_size = 1,
    .endianness = DEVICE_BIG_ENDIAN,
};

static void q9_clut_realize(DeviceState *dev, Error **errp)
{
    Q9ClutState *s = Q9_CLUT(dev);
    unsigned i;

    /* Reset state = identity ramp (r=g=b=index), matching q9_clut_init()
     * in the original -- everything else (generation, index) is already
     * zero from QOM's own object allocation. */
    for (i = 0; i < Q9_CLUT_ENTRIES; i++) {
        s->r[i] = s->g[i] = s->b[i] = (uint8_t)i;
    }

    memory_region_init_io(&s->iomem, OBJECT(dev), &q9_clut_ops, s,
                           TYPE_Q9_CLUT, Q9_CLUT_WINDOW_SIZE);
    sysbus_init_mmio(SYS_BUS_DEVICE(dev), &s->iomem);
}

static void q9_clut_class_init(ObjectClass *oc, const void *data)
{
    DeviceClass *dc = DEVICE_CLASS(oc);

    dc->desc = "Q9 board CLUT (colour lookup table for indexed video modes)";
    dc->realize = q9_clut_realize;
    set_bit(DEVICE_CATEGORY_DISPLAY, dc->categories);
}

static const TypeInfo q9_clut_info = {
    .name          = TYPE_Q9_CLUT,
    .parent        = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(Q9ClutState),
    .class_init    = q9_clut_class_init,
};

static void q9_clut_register_types(void)
{
    type_register_static(&q9_clut_info);
}

type_init(q9_clut_register_types)
