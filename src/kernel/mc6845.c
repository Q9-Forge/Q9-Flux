//════════════════════════════════════════════════════════════════════════════════════════════════
// File:   mc6845.c                                                                        Ver. 1.03
// Owner:  AF
// Desc.:  5.24: Implementierung, s. mc6845.h fuer das Adressmodell und die Geometrie-Vereinfachung.
//
// Edition History
//─────────┬──────┬────────────────────────────────────────────────────────────────────────┬──────
// Date    │ Ver. │ Description                                                            │ By
//─────────┼──────┼────────────────────────────────────────────────────────────────────────┼──────
// 26-08-03│ 1.00 │ 5.24: Erster Wurf                                                       │ Ada
// 26-08-03│ 1.01 │ 5.25: q9_mc6845_videoclk_khz() fuer R16/17 (Si5351A-Ersatz)              │ Ada
// 26-08-03│ 1.02 │ 5.27: q9_mc6845_bpp/width_px fuer die Host-Video-Bridge                 │ Ada
// 26-08-03│ 1.03 │ 5.27: R19 Netz-Update-Rate (Hz), q9_mc6845_net_update_hz()              │ Ada
//═════════╧══════╧════════════════════════════════════════════════════════════════════════╧══════
#include "mc6845.h"
#include <string.h>

void q9_mc6845_init(q9_mc6845_t *c)
{
    memset(c, 0, sizeof(*c));
}

uint32_t q9_mc6845_stride(const q9_mc6845_t *c)
{
    return c->reg[Q9_MC6845_R_HDISP];
}

uint32_t q9_mc6845_height(const q9_mc6845_t *c)
{
    return c->reg[Q9_MC6845_R_VDISP];
}

int q9_mc6845_mode(const q9_mc6845_t *c)
{
    return c->reg[Q9_MC6845_R_VIDEOMODE];
}

uint32_t q9_mc6845_videoclk_khz(const q9_mc6845_t *c)
{
    return ((uint32_t)c->reg[Q9_MC6845_R_VIDEOCLKH] << 8) | c->reg[Q9_MC6845_R_VIDEOCLKL];
}

int q9_mc6845_bpp(int mode)
{
    switch (mode) {
        case Q9_MC6845_MODE_INDEXED1: return 1;
        case Q9_MC6845_MODE_INDEXED2: return 2;
        case Q9_MC6845_MODE_INDEXED4: return 4;
        case Q9_MC6845_MODE_INDEXED8: return 8;
        case Q9_MC6845_MODE_RGB565:   return 16;
        case Q9_MC6845_MODE_RGB555I:  return 16;
        case Q9_MC6845_MODE_RGB888:   return 24;
        default:                      return 8;
    }
}

uint32_t q9_mc6845_width_px(const q9_mc6845_t *c)
{
    uint32_t stride = q9_mc6845_stride(c);
    int      bpp    = q9_mc6845_bpp(q9_mc6845_mode(c));
    return bpp < 8 ? stride * (uint32_t)(8 / bpp) : stride / (uint32_t)(bpp / 8);
}

uint32_t q9_mc6845_net_update_hz(const q9_mc6845_t *c)
{
    uint32_t hz = c->reg[Q9_MC6845_R_NETHZ];
    return hz ? hz : Q9_MC6845_NET_HZ_DEFAULT;
}

//────────────────────────────────────────────────────────────────────────────────────────────────
// Function: mc6845_dev_* / q9_devtype_mc6845
// Desc.:    5.24/5.25/5.27: Vtable-Adapter fuer die Geraete-Registry (devreg.h). Klassisches Index/
//           Daten-Registerpaar: +0 waehlt per SCHREIBEN das aktive Register (0-19, ausserhalb wird auf
//           den gueltigen Bereich begrenzt statt undefiniert zu bleiben); LESEN von +0 liefert den
//           zuletzt gewaehlten Index zurueck (praktisch fuers Debugging/Tuning-Tool aus 5.31, beim
//           echten Chip nicht immer moeglich, hier bewusst grosszuegiger). +1 liest/schreibt das
//           gewaehlte Register -- ALLE Register sind frei R/W (Vereinfachung, s. mc6845.h). Kein
//           IRQ (der 6845 selbst loest hier keinen Interrupt aus, s. ARBEITSPLAN 5.24).
//────────────────────────────────────────────────────────────────────────────────────────────────
static uint8_t mc6845_dev_read8(q9_device_t *dev, uint32_t addr)
{
    q9_mc6845_t *c = (q9_mc6845_t *)dev->state;
    uint32_t off = addr - dev->base;
    if (off == 0) {
        return c->addr_ptr;
    }
    return c->reg[c->addr_ptr];
}

static void mc6845_dev_write8(q9_device_t *dev, uint32_t addr, uint8_t val)
{
    q9_mc6845_t *c = (q9_mc6845_t *)dev->state;
    uint32_t off = addr - dev->base;
    if (off == 0) {
        c->addr_ptr = (uint8_t)(val < Q9_MC6845_NUM_REGS ? val : Q9_MC6845_NUM_REGS - 1);
    } else {
        c->reg[c->addr_ptr] = val;
    }
}

const q9_device_vtable_t q9_devtype_mc6845 = {
    .read8         = mc6845_dev_read8,
    .write8        = mc6845_dev_write8,
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
// EOF mc6845.c                                                                            Ver. 1.03
//────────────────────────────────────────────────────────────────────────────────────────────────
