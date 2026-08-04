//════════════════════════════════════════════════════════════════════════════════════════════════
// File:   framebuf.c                                                                     Ver. 1.00
// Owner:  AF
// Desc.:  5.26: Implementierung, s. framebuf.h fuer Adressmodell und Dirty-Tracking-Design.
//
// Edition History
//─────────┬──────┬────────────────────────────────────────────────────────────────────────┬──────
// Date    │ Ver. │ Description                                                            │ By
//─────────┼──────┼────────────────────────────────────────────────────────────────────────┼──────
// 26-08-03│ 1.00 │ 5.26: Erster Wurf                                                       │ Ada
//═════════╧══════╧════════════════════════════════════════════════════════════════════════╧══════
#include "framebuf.h"
#include <string.h>

int q9_framebuf_init(q9_framebuf_t *fb, uint8_t *vram, uint32_t vram_cap, uint32_t size,
                      const q9_mc6845_t *crtc)
{
    if (!fb || !vram || size == 0 || size > vram_cap) {
        return -1;
    }
    memset(fb, 0, sizeof(*fb));
    fb->vram = vram;
    fb->size = size;
    fb->crtc = crtc;
    memset(vram, 0, size);
    return 0;
}

uint8_t *q9_framebuf_vram(q9_framebuf_t *fb)
{
    return fb->vram;
}

uint32_t q9_framebuf_size(const q9_framebuf_t *fb)
{
    return fb->size;
}

//────────────────────────────────────────────────────────────────────────────────────────────────
// Function: rects_overlap / union_into / add_dirty_rect
// Desc.:    1:1 dasselbe Verfahren wie Q9-Frame tests/dummy_server.cpp (dort gegen eine einzelne
//           Bounding-Box gemessen: 13x weniger Netzwerklast) -- gedeckelte Liste mit Ueberlapp-
//           Merge, bei voller Liste Fallback auf eine Bounding-Box ueber alle Rechtecke.
//────────────────────────────────────────────────────────────────────────────────────────────────
static int rects_overlap(const q9_fb_dirty_rect_t *a, const q9_fb_dirty_rect_t *b)
{
    return a->x0 < b->x1 && a->x1 > b->x0 && a->y0 < b->y1 && a->y1 > b->y0;
}

static void union_into(q9_fb_dirty_rect_t *a, const q9_fb_dirty_rect_t *b)
{
    if (b->x0 < a->x0) a->x0 = b->x0;
    if (b->y0 < a->y0) a->y0 = b->y0;
    if (b->x1 > a->x1) a->x1 = b->x1;
    if (b->y1 > a->y1) a->y1 = b->y1;
}

static void add_dirty_rect(q9_framebuf_t *fb, q9_fb_dirty_rect_t r)
{
    int i;
    for (i = 0; i < fb->dirty_count; i++) {
        if (rects_overlap(&fb->dirty[i], &r)) {
            union_into(&fb->dirty[i], &r);
            return;
        }
    }
    if (fb->dirty_count < Q9_FRAMEBUF_MAX_DIRTY_RECTS) {
        fb->dirty[fb->dirty_count++] = r;
        return;
    }
    {
        q9_fb_dirty_rect_t merged = r;
        for (i = 0; i < fb->dirty_count; i++) union_into(&merged, &fb->dirty[i]);
        fb->dirty[0] = merged;
        fb->dirty_count = 1;
    }
}

//────────────────────────────────────────────────────────────────────────────────────────────────
// Function: q9_framebuf_dirty_mark
// Desc.:    Byte-Bereich [offset, offset+len) in (Zeile, Byte-Spalte)-Rechtecke umrechnen (s.
//           Dateikopf) -- ein Zugriff kann ueber eine Zeilengrenze laufen (z.B. ein LONG-Schreiben
//           kurz vor Zeilenende), daher die Schleife statt eines einzelnen Rechtecks.
//────────────────────────────────────────────────────────────────────────────────────────────────
void q9_framebuf_dirty_mark(q9_framebuf_t *fb, uint32_t offset, uint32_t len)
{
    uint32_t stride = fb->crtc ? q9_mc6845_stride(fb->crtc) : 0;
    if (stride == 0) {
        stride = fb->size;                             /* keine Zeilenlaenge programmiert -> eine Zeile */
    }
    while (len > 0) {
        uint32_t row   = offset / stride;
        uint32_t col0  = offset % stride;
        uint32_t take  = stride - col0;
        q9_fb_dirty_rect_t r;
        if (take > len) take = len;
        r.x0 = (int)col0;
        r.y0 = (int)row;
        r.x1 = (int)(col0 + take);
        r.y1 = (int)(row + 1u);
        add_dirty_rect(fb, r);
        offset += take;
        len    -= take;
    }
}

int q9_framebuf_dirty_count(const q9_framebuf_t *fb)
{
    return fb->dirty_count;
}

q9_fb_dirty_rect_t q9_framebuf_dirty_rect(const q9_framebuf_t *fb, int index)
{
    static const q9_fb_dirty_rect_t zero = { 0, 0, 0, 0 };
    if (index < 0 || index >= fb->dirty_count) {
        return zero;
    }
    return fb->dirty[index];
}

void q9_framebuf_dirty_clear(q9_framebuf_t *fb)
{
    fb->dirty_count = 0;
}

//────────────────────────────────────────────────────────────────────────────────────────────────
// Function: fb_dev_* / q9_devtype_framebuf
// Desc.:    5.26: Vtable-Adapter fuer die Geraete-Registry (devreg.h). off ist per Konstruktion
//           immer < dev->size = fb->size (s. q9_device_hit in devreg.c) -- die zusaetzlichen
//           Grenzchecks bei 16/32-Bit-Zugriffen fangen nur den Randfall ab, dass ein mehrbytiger
//           Zugriff GENAU am letzten Byte des Fensters beginnt (Folgebyte laege sonst ausserhalb).
//           Eigene 16/32-Pfade statt Synthese aus write8 (devreg.c-Default): so bleibt ein
//           mehrbytiger Schreibzugriff EIN zusammenhaengendes Dirty-Rechteck statt zwei einzelner,
//           die sich (angrenzend, aber nicht ueberlappend) NICHT von selbst zusammenfassen wuerden.
//────────────────────────────────────────────────────────────────────────────────────────────────
static uint8_t fb_dev_read8(q9_device_t *dev, uint32_t addr)
{
    q9_framebuf_t *fb = (q9_framebuf_t *)dev->state;
    uint32_t off = addr - dev->base;
    return fb->vram[off];
}

static void fb_dev_write8(q9_device_t *dev, uint32_t addr, uint8_t val)
{
    q9_framebuf_t *fb = (q9_framebuf_t *)dev->state;
    uint32_t off = addr - dev->base;
    fb->vram[off] = val;
    q9_framebuf_dirty_mark(fb, off, 1);
}

static uint16_t fb_dev_read16(q9_device_t *dev, uint32_t addr)
{
    q9_framebuf_t *fb = (q9_framebuf_t *)dev->state;
    uint32_t off = addr - dev->base;
    uint16_t hi = fb->vram[off];
    uint16_t lo = (off + 1u < fb->size) ? fb->vram[off + 1u] : 0;
    return (uint16_t)((hi << 8) | lo);
}

static void fb_dev_write16(q9_device_t *dev, uint32_t addr, uint16_t val)
{
    q9_framebuf_t *fb = (q9_framebuf_t *)dev->state;
    uint32_t off = addr - dev->base;
    uint32_t n = 0;
    fb->vram[off] = (uint8_t)(val >> 8);
    n++;
    if (off + 1u < fb->size) {
        fb->vram[off + 1u] = (uint8_t)val;
        n++;
    }
    q9_framebuf_dirty_mark(fb, off, n);
}

static uint32_t fb_dev_read32(q9_device_t *dev, uint32_t addr)
{
    q9_framebuf_t *fb = (q9_framebuf_t *)dev->state;
    uint32_t off = addr - dev->base;
    uint32_t b0 = fb->vram[off];
    uint32_t b1 = (off + 1u < fb->size) ? fb->vram[off + 1u] : 0;
    uint32_t b2 = (off + 2u < fb->size) ? fb->vram[off + 2u] : 0;
    uint32_t b3 = (off + 3u < fb->size) ? fb->vram[off + 3u] : 0;
    return (b0 << 24) | (b1 << 16) | (b2 << 8) | b3;
}

static void fb_dev_write32(q9_device_t *dev, uint32_t addr, uint32_t val)
{
    q9_framebuf_t *fb = (q9_framebuf_t *)dev->state;
    uint32_t off = addr - dev->base;
    uint32_t n = 0;
    fb->vram[off] = (uint8_t)(val >> 24);
    n++;
    if (off + 1u < fb->size) { fb->vram[off + 1u] = (uint8_t)(val >> 16); n++; }
    if (off + 2u < fb->size) { fb->vram[off + 2u] = (uint8_t)(val >> 8);  n++; }
    if (off + 3u < fb->size) { fb->vram[off + 3u] = (uint8_t)val;         n++; }
    q9_framebuf_dirty_mark(fb, off, n);
}

const q9_device_vtable_t q9_devtype_framebuf = {
    .read8         = fb_dev_read8,
    .write8        = fb_dev_write8,
    .read16        = fb_dev_read16,
    .write16       = fb_dev_write16,
    .read32        = fb_dev_read32,
    .write32       = fb_dev_write32,
    .poll          = NULL,
    .irq_pending   = NULL,
    .reset         = NULL,
    .irq_vector_fn = NULL,
};
//────────────────────────────────────────────────────────────────────────────────────────────────
// EOF framebuf.c                                                                          Ver. 1.00
//────────────────────────────────────────────────────────────────────────────────────────────────
