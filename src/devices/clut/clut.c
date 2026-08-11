//════════════════════════════════════════════════════════════════════════════════════════════════
// File:   clut.c                                                                          Ver. 1.00
// Owner:  AF
// Desc.:  5.29-Nachtrag: Implementierung, s. clut.h fuer das Adressmodell.
//
// Edition History
//─────────┬──────┬────────────────────────────────────────────────────────────────────────┬──────
// Date    │ Ver. │ Description                                                            │ By
//─────────┼──────┼────────────────────────────────────────────────────────────────────────┼──────
// 26-08-10│ 1.00 │ 5.29-Nachtrag: Erster Wurf (Claude)                                     │ Claude
//═════════╧══════╧════════════════════════════════════════════════════════════════════════╧══════
#include "clut.h"
#include <string.h>

void q9_clut_init(q9_clut_t *c)
{
    int i;
    memset(c, 0, sizeof(*c));
    for (i = 0; i < Q9_CLUT_ENTRIES; i++) {
        c->r[i] = c->g[i] = c->b[i] = (uint8_t)i;
    }
    c->generation = 0;
}

//────────────────────────────────────────────────────────────────────────────────────────────────
// Function: clut_dev_* / q9_devtype_clut
// Desc.:    5.29-Nachtrag: Vtable-Adapter (devreg.h). +0 Index (R/W, wie MC6845s Adressregister),
//           +1/+2/+3 R/G/B des gewaehlten Eintrags -- jeder Schreibzugriff auf +1/+2/+3 erhoeht
//           `generation` sofort (auch wenn der geschriebene Wert zufaellig identisch zum
//           bisherigen ist -- kein Vergleich vor dem Zaehlen, haelt den Pfad einfach und die
//           Video-Bridge braucht ohnehin nur "hat sich seit dem letzten Senden etwas GEAENDERT
//           HABEN KOENNEN", kein exaktes Diff).
//────────────────────────────────────────────────────────────────────────────────────────────────
static uint8_t clut_dev_read8(q9_device_t *dev, uint32_t addr)
{
    q9_clut_t *c = (q9_clut_t *)dev->state;
    uint32_t off = addr - dev->base;
    switch (off) {
        case 0:  return c->index;
        case 1:  return c->r[c->index];
        case 2:  return c->g[c->index];
        case 3:  return c->b[c->index];
        default: return 0;
    }
}

static void clut_dev_write8(q9_device_t *dev, uint32_t addr, uint8_t val)
{
    q9_clut_t *c = (q9_clut_t *)dev->state;
    uint32_t off = addr - dev->base;
    switch (off) {
        case 0: c->index = val; break;
        case 1: c->r[c->index] = val; c->generation++; break;
        case 2: c->g[c->index] = val; c->generation++; break;
        case 3: c->b[c->index] = val; c->generation++; break;
        default: break;
    }
}

const q9_device_vtable_t q9_devtype_clut = {
    .read8         = clut_dev_read8,
    .write8        = clut_dev_write8,
    .read16        = NULL,
    .write16       = NULL,
    .read32        = NULL,
    .write32       = NULL,
    .poll          = NULL,
    .irq_pending   = NULL,
    .reset         = NULL,
    .irq_vector_fn = NULL,
};
//────────────────────────────────────────────────────────────────────────────────────────────────
// EOF clut.c                                                                              Ver. 1.00
//────────────────────────────────────────────────────────────────────────────────────────────────
