/*
 * Q9 board: MC6845 CRT controller (GDP framebuffer geometry base).
 *
 * Ported from the Musashi-based implementation in
 * Q9-Flux-68k/src/devices/mc6845/mc6845.c -- register map and semantics
 * are unchanged: a classic index/data register pair (offset 0 selects
 * R0-R19, offset 1 reads/writes the selected register), all 20
 * registers freely read/write (deliberate simplification vs. real
 * hardware, s. the original's own header comment), no IRQ. R0-R15 match
 * the real MC6845 layout; R16/R17 (VideoClk kHz) and R18 (video mode,
 * s. Q9_MC6845_MODE_* in the original) and R19 (net update Hz) are this
 * board's own additions, not real 6845 registers.
 *
 * This file is not (yet) part of Q9-Flux's own MIT-licensed code base --
 * it lives inside the vendored QEMU source tree and is licensed under
 * QEMU's own terms (GPL), like the rest of this directory.
 */

#include "qemu/osdep.h"
#include "hw/core/sysbus.h"
#include "qom/object.h"

#define TYPE_Q9_MC6845 "q9-mc6845"
OBJECT_DECLARE_SIMPLE_TYPE(Q9MC6845State, Q9_MC6845)

/* Matches Q9_MC6845_BASE/_TOP and Q9_MC6845_NUM_REGS in
 * Q9-Flux-68k/src/devices/mc6845/mc6845.h. */
#define Q9_MC6845_WINDOW_SIZE 2u
#define Q9_MC6845_NUM_REGS    20u

struct Q9MC6845State {
    SysBusDevice parent_obj;
    MemoryRegion iomem;

    uint8_t reg[Q9_MC6845_NUM_REGS];
    uint8_t addr_ptr;
};

/* Cross-device accessor for devices/framebuf/q9_framebuf.c (dirty-rect
 * row mapping needs the CRTC's stride, R1 = Q9_MC6845_R_HDISP) -- a
 * plain C function rather than a qdev property, same rationale as
 * devices/remap/q9_remap.c's own q9_remap_set_targets(): this QEMU-side
 * devices/ tree has no shared headers yet. Returns 0 (matching the
 * original's own "no CRTC" fallback in q9_framebuf_dirty_mark, s. there)
 * if dev is NULL. */
uint32_t q9_mc6845_get_stride(DeviceState *dev);

uint32_t q9_mc6845_get_stride(DeviceState *dev)
{
    Q9MC6845State *s;

    if (!dev) {
        return 0;
    }
    s = Q9_MC6845(dev);
    return s->reg[1];   /* Q9_MC6845_R_HDISP */
}

/* Q9_MC6845_MODE_* -> bits/pixel, s. Q9-Flux-68k/src/devices/mc6845/
 * mc6845.c's own q9_mc6845_bpp() -- identical table, needed by
 * devices/videobridge/q9_videobridge.c too so kept as a small standalone
 * helper rather than duplicated inline there. */
static int q9_mc6845_bpp_for_mode(int mode)
{
    switch (mode) {
    case 0: return 1;    /* INDEXED1 */
    case 1: return 2;    /* INDEXED2 */
    case 2: return 4;    /* INDEXED4 */
    case 3: return 8;    /* INDEXED8 */
    case 4: return 16;   /* RGB565   */
    case 5: return 16;   /* RGB555I  */
    case 6: return 24;   /* RGB888   */
    default: return 8;
    }
}

/* Cross-device accessor for devices/videobridge/q9_videobridge.c --
 * everything it needs from the CRTC in one call, same rationale as
 * q9_mc6845_get_stride() above. */
void q9_mc6845_get_info(DeviceState *dev, uint32_t *stride, uint32_t *height,
                         int *mode, int *bpp, uint32_t *width_px,
                         uint32_t *net_hz);

void q9_mc6845_get_info(DeviceState *dev, uint32_t *stride, uint32_t *height,
                         int *mode, int *bpp, uint32_t *width_px,
                         uint32_t *net_hz)
{
    Q9MC6845State *s = Q9_MC6845(dev);
    uint32_t st = s->reg[1];    /* R1 HDISP  */
    uint32_t ht = s->reg[6];    /* R6 VDISP  */
    int m = s->reg[18];         /* R18 mode  */
    int bp = q9_mc6845_bpp_for_mode(m);
    uint32_t hz = s->reg[19];   /* R19 net update Hz */

    if (stride) { *stride = st; }
    if (height) { *height = ht; }
    if (mode) { *mode = m; }
    if (bpp) { *bpp = bp; }
    if (width_px) {
        *width_px = bp < 8 ? st * (uint32_t)(8 / bp) : st / (uint32_t)(bp / 8);
    }
    if (net_hz) { *net_hz = hz ? hz : 30u; /* Q9_MC6845_NET_HZ_DEFAULT */ }
}

static uint64_t q9_mc6845_read(void *opaque, hwaddr addr, unsigned size)
{
    Q9MC6845State *s = opaque;

    if (addr == 0) {
        return s->addr_ptr;
    }
    return s->reg[s->addr_ptr];
}

static void q9_mc6845_write(void *opaque, hwaddr addr, uint64_t val,
                             unsigned size)
{
    Q9MC6845State *s = opaque;

    if (addr == 0) {
        s->addr_ptr = (uint8_t)(val < Q9_MC6845_NUM_REGS
                                 ? val : Q9_MC6845_NUM_REGS - 1);
    } else {
        s->reg[s->addr_ptr] = (uint8_t)val;
    }
}

static const MemoryRegionOps q9_mc6845_ops = {
    .read = q9_mc6845_read,
    .write = q9_mc6845_write,
    .valid.min_access_size = 1,
    .valid.max_access_size = 1,
    .endianness = DEVICE_BIG_ENDIAN,
};

static void q9_mc6845_realize(DeviceState *dev, Error **errp)
{
    Q9MC6845State *s = Q9_MC6845(dev);

    memory_region_init_io(&s->iomem, OBJECT(dev), &q9_mc6845_ops, s,
                           TYPE_Q9_MC6845, Q9_MC6845_WINDOW_SIZE);
    sysbus_init_mmio(SYS_BUS_DEVICE(dev), &s->iomem);
}

static void q9_mc6845_class_init(ObjectClass *oc, const void *data)
{
    DeviceClass *dc = DEVICE_CLASS(oc);

    dc->desc = "Q9 board MC6845 CRT controller (GDP framebuffer base)";
    dc->realize = q9_mc6845_realize;
    set_bit(DEVICE_CATEGORY_DISPLAY, dc->categories);
}

static const TypeInfo q9_mc6845_info = {
    .name          = TYPE_Q9_MC6845,
    .parent        = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(Q9MC6845State),
    .class_init    = q9_mc6845_class_init,
};

static void q9_mc6845_register_types(void)
{
    type_register_static(&q9_mc6845_info);
}

type_init(q9_mc6845_register_types)
