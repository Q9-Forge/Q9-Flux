//════════════════════════════════════════════════════════════════════════════════════════════════
// File:   remap.c                                                                         Ver. 1.00
// Owner:  AF
// Desc.:  Implementierung, siehe remap.h. Reine Verschiebung aus src/kernel/q9board.c (2026-08-21,
//         Hardware-Vereinheitlichung) -- Verhalten UNVERAENDERT (Lesen wie Schreiben setzen
//         remapped=1, Lesewert immer 0). Neu ist nur q9_devdesc_remap am Dateiende.
//
// Edition History
//─────────┬──────┬────────────────────────────────────────────────────────────────────────┬──────
// Date    │ Ver. │ Description                                                            │ By
//─────────┼──────┼────────────────────────────────────────────────────────────────────────┬──────
// 25/26-xx│ 1.xx │ 5.2a/5.3: urspruenglich Teil von q9board.c, s. dortige Historie          │ CF
// 26-08-21│ 1.00 │ Hardware-Vereinheitlichung: aus q9board.c hierher verschoben, neu         │ Cld
//         │      │ q9_devdesc_remap; devreg dispatcht jetzt VOR dem alten Board-Fallback --  │
//         │      │ board_read_byte/write_byte brauchen den REMAP-Check nicht mehr (s. dort)  │
//═════════╧══════╧════════════════════════════════════════════════════════════════════════╧══════
#include "remap.h"

//────────────────────────────────────────────────────────────────────────────────────────────────
// Function: remap_dev_* / q9_devtype_remap
// Desc.:    Reiner Adress-Trigger (kein Datenwert): Lesen wie Schreiben schalten das Board in den
//           Remap-Zustand, unabhaengig vom gelesenen/geschriebenen Wert. Lesewert immer 0 (kein
//           echtes Register dahinter). Kein IRQ, kein poll noetig.
//────────────────────────────────────────────────────────────────────────────────────────────────
static uint8_t remap_dev_read8(q9_device_t *dev, uint32_t addr)
{
    (void)addr;
    ((q9_board_t *)dev->state)->remapped = 1;
    return 0;
}

static void remap_dev_write8(q9_device_t *dev, uint32_t addr, uint8_t val)
{
    (void)addr; (void)val;
    ((q9_board_t *)dev->state)->remapped = 1;
}

const q9_device_vtable_t q9_devtype_remap = {
    .read8         = remap_dev_read8,
    .write8        = remap_dev_write8,
    .read16        = NULL,
    .write16       = NULL,
    .read32        = NULL,
    .write32       = NULL,
    .poll          = NULL,
    .irq_pending   = NULL,
    .reset         = NULL,
    .irq_vector_fn = NULL,
};

/* 2026-08-21 (Hardware-Vereinheitlichung): q9_devdesc_remap -- noch OHNE extra_fields (kein
   Config-Schema, immer hartkodiert instanziiert, s. m68krt.c q9_m68krt_attach_board -- eigener,
   spaeterer Schritt). */
const q9_devdesc_t q9_devdesc_remap = {
    .type              = "remap",
    .desc              = "REMAP-Trigger (ROM-Spiegel -> RAM-Umschaltung)",
    .vt                = &q9_devtype_remap,
    .use_table_default = 1,                            /* liegt im Fast-Table-Cluster              */
    .extra_fields      = NULL,
    .extra_field_count = 0,
};

//────────────────────────────────────────────────────────────────────────────────────────────────
// EOF remap.c                                                                            Ver. 1.00
//────────────────────────────────────────────────────────────────────────────────────────────────
