//════════════════════════════════════════════════════════════════════════════════════════════════
// File:   plic.c                                                                          Ver. 1.00
// Owner:  Claudia
// Desc.:  Umsetzung des PLIC-Geraets, s. plic.h fuer Registerlage und die beiden wichtigen
//         Anmerkungen (Pegelsteuerung, die set_mip-Falle beim Aufrufer).
//════════════════════════════════════════════════════════════════════════════════════════════════

#include <string.h>
#include "plic.h"

#define OFF_PRIORITY   0x000000u
#define OFF_PENDING    0x001000u
#define OFF_ENABLE     0x002000u
#define ENABLE_CTX_STRIDE  0x80u
#define OFF_CTX        0x200000u
#define CTX_STRIDE     0x1000u
#define OFF_THRESHOLD  0u          /* relativ zum jeweiligen Kontext-Block bei OFF_CTX + ctx*CTX_STRIDE */
#define OFF_CLAIM      4u

void q9_plic_init(q9_plic_t *p)
{
    memset(p, 0, sizeof(*p));
}

void q9_plic_set_level(q9_plic_t *p, unsigned source, int level)
{
    if (source == 0 || source >= Q9_PLIC_NUM_SOURCES) return;
    p->level[source] = level ? 1 : 0;
}

static int enabled_for_ctx(const q9_plic_t *p, unsigned ctx, unsigned source)
{
    unsigned word = source / 32u, bit = source % 32u;
    return (p->enable[ctx][word] & (1u << bit)) != 0;
}

/* Findet die anspruchsberechtigte Quelle mit der hoechsten Prioritaet fuer Kontext ctx
   (bei Gleichstand die niedrigste Quellennummer, wie in der Spezifikation gefordert).
   "Anspruchsberechtigt" heisst: liegt an (level=1), ist fuer diesen Kontext freigegeben, hat eine
   Prioritaet ueber der Schwelle, und ist nicht bereits von DIESEM Kontext uebernommen. 0 = keine. */
static unsigned find_claimable(const q9_plic_t *p, unsigned ctx)
{
    unsigned best = 0;
    uint32_t best_prio = p->threshold[ctx];
    unsigned s;
    for (s = 1; s < Q9_PLIC_NUM_SOURCES; s++) {
        if (!p->level[s]) continue;
        if (!enabled_for_ctx(p, ctx, s)) continue;
        if ((int)s == p->claimed[ctx]) continue;
        if (p->priority[s] <= best_prio) continue;   /* muss STRENG groesser als die Schwelle sein */
        best_prio = p->priority[s];
        best = s;
    }
    return best;
}

int q9_plic_context_pending(const q9_plic_t *p, unsigned ctx)
{
    if (ctx >= Q9_PLIC_NUM_CONTEXTS) return 0;
    return find_claimable(p, ctx) != 0;
}

uint32_t q9_plic_read32(q9_plic_t *p, uint32_t offset)
{
    if (offset >= OFF_PRIORITY && offset < OFF_PRIORITY + 4u * Q9_PLIC_NUM_SOURCES) {
        unsigned s = (offset - OFF_PRIORITY) / 4u;
        return p->priority[s];
    }
    if (offset >= OFF_PENDING && offset < OFF_PENDING + 8u) {
        /* informativ, wird von NuttX nicht gelesen -- der Vollstaendigkeit halber richtig */
        unsigned word = (offset - OFF_PENDING) / 4u, s, bits = 0;
        for (s = word * 32u; s < word * 32u + 32u && s < Q9_PLIC_NUM_SOURCES; s++)
            if (p->level[s]) bits |= 1u << (s % 32u);
        return bits;
    }
    if (offset >= OFF_ENABLE && offset < OFF_CTX) {
        unsigned rel = offset - OFF_ENABLE;
        unsigned ctx = rel / ENABLE_CTX_STRIDE;
        unsigned word = (rel % ENABLE_CTX_STRIDE) / 4u;
        if (ctx < Q9_PLIC_NUM_CONTEXTS && word < 2u) return p->enable[ctx][word];
        return 0;
    }
    if (offset >= OFF_CTX) {
        unsigned rel = offset - OFF_CTX;
        unsigned ctx = rel / CTX_STRIDE;
        unsigned sub = rel % CTX_STRIDE;
        if (ctx >= Q9_PLIC_NUM_CONTEXTS) return 0;
        if (sub == OFF_THRESHOLD) return p->threshold[ctx];
        if (sub == OFF_CLAIM) {
            /* Lesen = uebernehmen: die gefundene Quelle merken, damit COMPLETE (Schreiben
               derselben Nummer) sie wieder freigibt, und zurueckgeben. */
            unsigned s = find_claimable(p, ctx);
            p->claimed[ctx] = (int)s;
            return s;
        }
    }
    return 0;
}

void q9_plic_write32(q9_plic_t *p, uint32_t offset, uint32_t val)
{
    if (offset >= OFF_PRIORITY && offset < OFF_PRIORITY + 4u * Q9_PLIC_NUM_SOURCES) {
        unsigned s = (offset - OFF_PRIORITY) / 4u;
        if (s != 0) p->priority[s] = val;   /* Quelle 0 gibt es nicht, s. Kopf */
        return;
    }
    if (offset >= OFF_PENDING && offset < OFF_PENDING + 8u) {
        return;   /* nur lesbar */
    }
    if (offset >= OFF_ENABLE && offset < OFF_CTX) {
        unsigned rel = offset - OFF_ENABLE;
        unsigned ctx = rel / ENABLE_CTX_STRIDE;
        unsigned word = (rel % ENABLE_CTX_STRIDE) / 4u;
        if (ctx < Q9_PLIC_NUM_CONTEXTS && word < 2u) p->enable[ctx][word] = val;
        return;
    }
    if (offset >= OFF_CTX) {
        unsigned rel = offset - OFF_CTX;
        unsigned ctx = rel / CTX_STRIDE;
        unsigned sub = rel % CTX_STRIDE;
        if (ctx >= Q9_PLIC_NUM_CONTEXTS) return;
        if (sub == OFF_THRESHOLD) { p->threshold[ctx] = val; return; }
        if (sub == OFF_CLAIM) {
            /* Complete: nur wirksam, wenn genau die zuvor uebernommene Quelle zurueckgemeldet
               wird -- entspricht der Spezifikation (eine falsche Nummer wird ignoriert). Die
               Quelle bleibt in p->level[] anliegend, falls das Geraet die Ursache noch nicht
               behoben hat -- s. Pegelsteuerung im Kopf. */
            if ((int)val == p->claimed[ctx]) p->claimed[ctx] = 0;
            return;
        }
    }
}

// EOF plic.c                                                                               Ver. 1.00
