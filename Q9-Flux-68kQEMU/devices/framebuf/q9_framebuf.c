/*
 * Q9 board: VRAM framebuffer (Q9 Frame / host video bridge).
 *
 * Ported from the Musashi-based implementation in
 * Q9-Flux-68k/src/devices/framebuf/framebuf.c -- semantics unchanged:
 * a flat byte-addressed VRAM window with dirty-rectangle tracking (byte
 * columns x0/x1 within a row, row index y0/y1), row width taken from
 * the CRTC's stride register (Q9_MC6845_R_HDISP via
 * q9_mc6845_get_stride(), s. devices/mc6845/q9_mc6845.c) or the whole
 * buffer as one row if no CRTC is linked. Wide (16/32-bit) accesses get
 * their own read/write path rather than QEMU's usual per-size byte
 * synthesis, for the same reason as the original: a multi-byte write
 * should become ONE dirty rectangle, not several adjacent-but-not-
 * overlapping ones that wouldn't auto-merge. Capped dirty-rect list
 * (16 entries) with overlap-merge, falling back to one bounding box
 * over everything once full -- 1:1 the same scheme as the original
 * (itself matching Q9-Frame's own dummy_server.cpp). No IRQ.
 *
 * This file is not (yet) part of Q9-Flux's own MIT-licensed code base --
 * it lives inside the vendored QEMU source tree and is licensed under
 * QEMU's own terms (GPL), like the rest of this directory.
 */

#include "qemu/osdep.h"
#include "qapi/error.h"
#include "hw/core/sysbus.h"
#include "hw/core/qdev-properties.h"
#include "qom/object.h"

#define TYPE_Q9_FRAMEBUF "q9-framebuf"
OBJECT_DECLARE_SIMPLE_TYPE(Q9FramebufState, Q9_FRAMEBUF)

/* Matches Q9_FRAMEBUF_BASE/_DEFAULT_SIZE/_MAX_SIZE/_MAX_DIRTY_RECTS in
 * Q9-Flux-68k/src/devices/framebuf/framebuf.h. */
#define Q9_FRAMEBUF_DEFAULT_SIZE (1u * 1024u * 1024u)
#define Q9_FRAMEBUF_MAX_SIZE     (16u * 1024u * 1024u)
#define Q9_FRAMEBUF_MAX_DIRTY_RECTS 16

/* devices/mc6845/q9_mc6845.c, s. there for why this is a plain function
 * rather than a qdev property. */
uint32_t q9_mc6845_get_stride(DeviceState *dev);

typedef struct {
    int x0, y0, x1, y1;
} Q9FbDirtyRect;

struct Q9FramebufState {
    SysBusDevice parent_obj;
    MemoryRegion iomem;

    uint8_t *vram;
    uint32_t size;
    DeviceState *crtc;         /* optional "mc6845" link, may be NULL */

    Q9FbDirtyRect dirty[Q9_FRAMEBUF_MAX_DIRTY_RECTS];
    int dirty_count;

    uint32_t size_prop;        /* qdev property, s. Dateikopf */
};

static bool rects_overlap(const Q9FbDirtyRect *a, const Q9FbDirtyRect *b)
{
    return a->x0 < b->x1 && a->x1 > b->x0 && a->y0 < b->y1 && a->y1 > b->y0;
}

static void union_into(Q9FbDirtyRect *a, const Q9FbDirtyRect *b)
{
    if (b->x0 < a->x0) { a->x0 = b->x0; }
    if (b->y0 < a->y0) { a->y0 = b->y0; }
    if (b->x1 > a->x1) { a->x1 = b->x1; }
    if (b->y1 > a->y1) { a->y1 = b->y1; }
}

static void add_dirty_rect(Q9FramebufState *s, Q9FbDirtyRect r)
{
    int i;

    for (i = 0; i < s->dirty_count; i++) {
        if (rects_overlap(&s->dirty[i], &r)) {
            union_into(&s->dirty[i], &r);
            return;
        }
    }
    if (s->dirty_count < Q9_FRAMEBUF_MAX_DIRTY_RECTS) {
        s->dirty[s->dirty_count++] = r;
        return;
    }
    {
        Q9FbDirtyRect merged = r;
        for (i = 0; i < s->dirty_count; i++) {
            union_into(&merged, &s->dirty[i]);
        }
        s->dirty[0] = merged;
        s->dirty_count = 1;
    }
}

static void q9_framebuf_dirty_mark(Q9FramebufState *s, uint32_t offset,
                                    uint32_t len)
{
    uint32_t stride = q9_mc6845_get_stride(s->crtc);

    if (stride == 0) {
        stride = s->size;   /* no stride programmed -> treat as one row */
    }
    while (len > 0) {
        uint32_t row  = offset / stride;
        uint32_t col0 = offset % stride;
        uint32_t take = stride - col0;
        Q9FbDirtyRect r;

        if (take > len) {
            take = len;
        }
        r.x0 = (int)col0;
        r.y0 = (int)row;
        r.x1 = (int)(col0 + take);
        r.y1 = (int)(row + 1u);
        add_dirty_rect(s, r);
        offset += take;
        len -= take;
    }
}

static uint64_t q9_framebuf_read(void *opaque, hwaddr addr, unsigned size)
{
    Q9FramebufState *s = opaque;
    uint64_t val = 0;
    unsigned i;

    for (i = 0; i < size; i++) {
        uint8_t b = (addr + i < s->size) ? s->vram[addr + i] : 0;
        val = (val << 8) | b;
    }
    return val;
}

static void q9_framebuf_write(void *opaque, hwaddr addr, uint64_t val,
                               unsigned size)
{
    Q9FramebufState *s = opaque;
    unsigned i, n = 0;

    for (i = 0; i < size; i++) {
        if (addr + i < s->size) {
            s->vram[addr + i] = (uint8_t)(val >> (8 * (size - 1 - i)));
            n++;
        }
    }
    if (n > 0) {
        q9_framebuf_dirty_mark(s, (uint32_t)addr, n);
    }
}

static const MemoryRegionOps q9_framebuf_ops = {
    .read = q9_framebuf_read,
    .write = q9_framebuf_write,
    .valid.min_access_size = 1,
    .valid.max_access_size = 4,
    .impl.min_access_size = 1,
    .impl.max_access_size = 4,
    .endianness = DEVICE_BIG_ENDIAN,
};

static void q9_framebuf_realize(DeviceState *dev, Error **errp)
{
    Q9FramebufState *s = Q9_FRAMEBUF(dev);

    if (s->size_prop == 0 || s->size_prop > Q9_FRAMEBUF_MAX_SIZE) {
        error_setg(errp, "q9-framebuf: 'size' must be 1..%u bytes",
                   Q9_FRAMEBUF_MAX_SIZE);
        return;
    }
    s->size = s->size_prop;
    s->vram = g_malloc0(s->size);

    memory_region_init_io(&s->iomem, OBJECT(dev), &q9_framebuf_ops, s,
                           TYPE_Q9_FRAMEBUF, s->size);
    sysbus_init_mmio(SYS_BUS_DEVICE(dev), &s->iomem);
}

static const Property q9_framebuf_properties[] = {
    DEFINE_PROP_LINK("mc6845", Q9FramebufState, crtc, TYPE_DEVICE, DeviceState *),
    DEFINE_PROP_UINT32("size", Q9FramebufState, size_prop,
                        Q9_FRAMEBUF_DEFAULT_SIZE),
};

static void q9_framebuf_class_init(ObjectClass *oc, const void *data)
{
    DeviceClass *dc = DEVICE_CLASS(oc);

    dc->desc = "Q9 board VRAM framebuffer";
    dc->realize = q9_framebuf_realize;
    device_class_set_props(dc, q9_framebuf_properties);
    set_bit(DEVICE_CATEGORY_DISPLAY, dc->categories);
}

static const TypeInfo q9_framebuf_info = {
    .name          = TYPE_Q9_FRAMEBUF,
    .parent        = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(Q9FramebufState),
    .class_init    = q9_framebuf_class_init,
};

static void q9_framebuf_register_types(void)
{
    type_register_static(&q9_framebuf_info);
}

type_init(q9_framebuf_register_types)
