//════════════════════════════════════════════════════════════════════════════════════════════════
// File:   clint.c                                                                        Ver. 1.00
// Owner:  Claudia
// Desc.:  Umsetzung des CLINT-Geraets, s. clint.h fuer Registerlage und die beiden wichtigen
//         Anmerkungen (zyklenbasierte mtime, die set_mip-Falle beim Aufrufer).
//════════════════════════════════════════════════════════════════════════════════════════════════

#include <string.h>
#include "clint.h"

void q9_clint_init(q9_clint_t *c)
{
    memset(c, 0, sizeof(*c));
    c->mtimecmp = UINT64_MAX;   /* nichts anfordern, bis der Gast einen Wert setzt */
}

uint32_t q9_clint_read32(q9_clint_t *c, uint32_t offset)
{
    switch (offset) {
    case Q9_CLINT_OFF_MSIP:              return c->msip & 1u;
    case Q9_CLINT_OFF_MTIMECMP:          return (uint32_t)(c->mtimecmp & 0xffffffffu);
    case Q9_CLINT_OFF_MTIMECMP + 4:      return (uint32_t)(c->mtimecmp >> 32);
    case Q9_CLINT_OFF_MTIME:             return (uint32_t)(c->mtime & 0xffffffffu);
    case Q9_CLINT_OFF_MTIME + 4:         return (uint32_t)(c->mtime >> 32);
    default:                             return 0;
    }
}

void q9_clint_write32(q9_clint_t *c, uint32_t offset, uint32_t val)
{
    switch (offset) {
    case Q9_CLINT_OFF_MSIP:
        c->msip = val & 1u;
        return;
    case Q9_CLINT_OFF_MTIMECMP:
        c->mtimecmp = (c->mtimecmp & 0xffffffff00000000ULL) | (uint64_t)val;
        return;
    case Q9_CLINT_OFF_MTIMECMP + 4:
        c->mtimecmp = (c->mtimecmp & 0x00000000ffffffffULL) | ((uint64_t)val << 32);
        return;
    default:
        return;   /* mtime ist nur lesend, s. Kopf von clint.h */
    }
}

void q9_clint_advance(q9_clint_t *c, uint64_t cycles)
{
    c->mtime += cycles;
}

int q9_clint_timer_pending(const q9_clint_t *c)
{
    return c->mtime >= c->mtimecmp;
}

// EOF clint.c                                                                             Ver. 1.00
