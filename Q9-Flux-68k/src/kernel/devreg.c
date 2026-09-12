//════════════════════════════════════════════════════════════════════════════════════════════════
// File:   devreg.c                                                                        Ver. 1.10
// Owner:  AF
// Desc.:  Implementierung der Geraete-Registry, siehe devreg.h.
//
// Edition History
//─────────┬──────┬────────────────────────────────────────────────────────────────────────┬──────
// Date    │ Ver. │ Description                                                            │ By
//─────────┼──────┼────────────────────────────────────────────────────────────────────────┼──────
// 26-07-14│ 1.00 │ 5.17: Erster Wurf — Instanz-Registry + generische Zugriffs-Helfer;       │ CF
//         │      │ Typ-Registry startet mit "duart68681" (waechst mit jedem Migrationsschritt) │
// 26-08-21│ 1.10 │ Hardware-Vereinheitlichung: q9_devtype_duart68681 kommt jetzt ueber          │ Cld
//         │      │ duart68681.h statt transitiv ueber q9board.h (das re-exportiert es nicht    │
//         │      │ mehr, s. dortiger Kommentar) -- reine Include-Anpassung                       │
//═════════╧══════╧════════════════════════════════════════════════════════════════════════╧══════
#include "devreg.h"
#include "../devices/duart68681/duart68681.h"          /* 2026-08-21: q9_devtype_duart68681, s.u. */
#include <string.h>

//────────────────────────────────────────────────────────────────────────────────────────────────
// Instanz-Registry
//────────────────────────────────────────────────────────────────────────────────────────────────
static q9_device_t g_devreg[Q9_DEVREG_MAX];
static int         g_devreg_count;

void q9_devreg_clear(void)
{
    memset(g_devreg, 0, sizeof(g_devreg));
    g_devreg_count = 0;
}

int q9_devreg_add(q9_device_t dev)
{
    if (g_devreg_count >= Q9_DEVREG_MAX) {
        return -1;
    }
    g_devreg[g_devreg_count++] = dev;
    return 0;
}

int q9_devreg_count(void)
{
    return g_devreg_count;
}

q9_device_t *q9_devreg_get(int index)
{
    if (index < 0 || index >= g_devreg_count) {
        return NULL;
    }
    return &g_devreg[index];
}

//────────────────────────────────────────────────────────────────────────────────────────────────
// Generische Zugriffs-Helfer
//────────────────────────────────────────────────────────────────────────────────────────────────
int q9_device_hit(const q9_device_t *dev, uint32_t addr)
{
    if (!dev || dev->size == 0) {
        return 0;
    }
    return addr >= dev->base && addr <= dev->base + dev->size - 1u;
}

uint8_t q9_device_read8(q9_device_t *dev, uint32_t addr)
{
    if (!dev || !dev->vt || !dev->vt->read8) {
        return 0;
    }
    return dev->vt->read8(dev, addr);
}

void q9_device_write8(q9_device_t *dev, uint32_t addr, uint8_t val)
{
    if (!dev || !dev->vt || !dev->vt->write8) {
        return;
    }
    dev->vt->write8(dev, addr, val);
}

uint16_t q9_device_read16(q9_device_t *dev, uint32_t addr)
{
    if (!dev || !dev->vt) {
        return 0;
    }
    if (dev->vt->read16) {
        return dev->vt->read16(dev, addr);
    }
    /* Synthese aus zwei Byte-Zugriffen, big-endian wie der 68k (s. m68krt.c) */
    {
        uint16_t hi = q9_device_read8(dev, addr);
        uint16_t lo = q9_device_read8(dev, addr + 1u);
        return (uint16_t)((hi << 8) | lo);
    }
}

void q9_device_write16(q9_device_t *dev, uint32_t addr, uint16_t val)
{
    if (!dev || !dev->vt) {
        return;
    }
    if (dev->vt->write16) {
        dev->vt->write16(dev, addr, val);
        return;
    }
    q9_device_write8(dev, addr,      (uint8_t)(val >> 8));
    q9_device_write8(dev, addr + 1u, (uint8_t)val);
}

uint32_t q9_device_read32(q9_device_t *dev, uint32_t addr)
{
    if (!dev || !dev->vt) {
        return 0;
    }
    if (dev->vt->read32) {
        return dev->vt->read32(dev, addr);
    }
    {
        uint32_t b0 = q9_device_read8(dev, addr);
        uint32_t b1 = q9_device_read8(dev, addr + 1u);
        uint32_t b2 = q9_device_read8(dev, addr + 2u);
        uint32_t b3 = q9_device_read8(dev, addr + 3u);
        return (b0 << 24) | (b1 << 16) | (b2 << 8) | b3;
    }
}

void q9_device_write32(q9_device_t *dev, uint32_t addr, uint32_t val)
{
    if (!dev || !dev->vt) {
        return;
    }
    if (dev->vt->write32) {
        dev->vt->write32(dev, addr, val);
        return;
    }
    q9_device_write8(dev, addr,      (uint8_t)(val >> 24));
    q9_device_write8(dev, addr + 1u, (uint8_t)(val >> 16));
    q9_device_write8(dev, addr + 2u, (uint8_t)(val >> 8));
    q9_device_write8(dev, addr + 3u, (uint8_t)val);
}

void q9_device_poll(q9_device_t *dev, uint32_t now_ms)
{
    if (dev && dev->vt && dev->vt->poll) {
        dev->vt->poll(dev, now_ms);
    }
}

int q9_device_irq_pending(q9_device_t *dev)
{
    if (!dev || !dev->vt || !dev->vt->irq_pending) {
        return 0;
    }
    return dev->vt->irq_pending(dev);
}

void q9_device_reset(q9_device_t *dev)
{
    if (dev && dev->vt && dev->vt->reset) {
        dev->vt->reset(dev);
    }
}

int q9_device_irq_vector(q9_device_t *dev)
{
    if (!dev) {
        return -1;
    }
    if (dev->vt && dev->vt->irq_vector_fn) {
        return dev->vt->irq_vector_fn(dev);
    }
    return dev->irq_vector;
}

//────────────────────────────────────────────────────────────────────────────────────────────────
// Typ-Registry (s. devreg.h) — waechst mit jedem 5.17-Migrationsschritt um einen Eintrag.
//────────────────────────────────────────────────────────────────────────────────────────────────
static const q9_device_type_entry_t g_device_types[] = {
    { "duart68681", &q9_devtype_duart68681 },
};
#define Q9_DEVTYPE_COUNT (int)(sizeof(g_device_types) / sizeof(g_device_types[0]))

const q9_device_vtable_t *q9_devtype_lookup(const char *type)
{
    int i;
    if (!type) {
        return NULL;
    }
    for (i = 0; i < Q9_DEVTYPE_COUNT; i++) {
        if (strcmp(g_device_types[i].type, type) == 0) {
            return g_device_types[i].vt;
        }
    }
    return NULL;
}

int q9_devtype_count(void)
{
    return Q9_DEVTYPE_COUNT;
}

const q9_device_type_entry_t *q9_devtype_get(int index)
{
    if (index < 0 || index >= Q9_DEVTYPE_COUNT) {
        return NULL;
    }
    return &g_device_types[index];
}

//────────────────────────────────────────────────────────────────────────────────────────────────
// EOF devreg.c                                                                            Ver. 1.10
//────────────────────────────────────────────────────────────────────────────────────────────────
