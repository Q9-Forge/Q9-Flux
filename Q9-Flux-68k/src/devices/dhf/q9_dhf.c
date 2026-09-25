//════════════════════════════════════════════════════════════════════════════════════════════════
// File:   q9_dhf.c                                                                        Ver. 2.00
// Owner:  Claude
// Desc.:  Implementierung, siehe q9_dhf.h. Reiner devreg-Vtable-Adapter -- die eigentliche Arbeit
//         (Kommando-Dispatch, Host-Dateisystem-Zugriff, Gast-RAM-Aufloesung ueber A0/A1) macht
//         dhf_emu_device.c/dhf_host_fs.c unveraendert, s. dortige Kopfkommentare.
//
// Edition History
//─────────┬──────┬────────────────────────────────────────────────────────────────────────┬──────
// Date    │ Ver. │ Description                                                            │ By
//─────────┼──────┼────────────────────────────────────────────────────────────────────────┬──────
// 26-09-25│ 2.00 │ Neufassung als duenner Adapter (s. q9_dhf.h)                             │ Cld
//═════════╧══════╧════════════════════════════════════════════════════════════════════════╧══════
#include "q9_dhf.h"
#include <string.h>

void q9_dhf_init(q9_dhf_t *state, const char *basepath, uint8_t *ram, size_t ram_len)
{
    memset(state, 0, sizeof(*state));
    dhf_emu_device_init_local(&state->dev, &state->shared, basepath);
    dhf_emu_device_set_ram(&state->dev, ram, ram_len);
}

//────────────────────────────────────────────────────────────────────────────────────────────────
// Function: dhf_dev_read8 / dhf_dev_write8 / ... / q9_devtype_dhf
// Desc.:    Adapter (q9_device_t*, absolute Adresse) -> (dhf_emu_device_t*, fensterrelativer
//           Offset), analog cf_dev_read8/rtc_dev_read8. dhf_emu_device_write8 loest bei einem
//           Schreibzugriff auf das "command"-Feld (Offset 1, s. dhf_shared.h) selbst schon
//           dhf_emu_device_process() aus -- hier nichts weiter zu tun.
//────────────────────────────────────────────────────────────────────────────────────────────────
static uint8_t dhf_dev_read8(q9_device_t *dev, uint32_t addr)
{
    q9_dhf_t *s = (q9_dhf_t *)dev->state;
    return dhf_emu_device_read8(&s->dev, addr - dev->base);
}

static void dhf_dev_write8(q9_device_t *dev, uint32_t addr, uint8_t val)
{
    q9_dhf_t *s = (q9_dhf_t *)dev->state;
    dhf_emu_device_write8(&s->dev, addr - dev->base, val);
}

static uint16_t dhf_dev_read16(q9_device_t *dev, uint32_t addr)
{
    q9_dhf_t *s = (q9_dhf_t *)dev->state;
    return dhf_emu_device_read16(&s->dev, addr - dev->base);
}

static void dhf_dev_write16(q9_device_t *dev, uint32_t addr, uint16_t val)
{
    q9_dhf_t *s = (q9_dhf_t *)dev->state;
    dhf_emu_device_write16(&s->dev, addr - dev->base, val);
}

static uint32_t dhf_dev_read32(q9_device_t *dev, uint32_t addr)
{
    q9_dhf_t *s = (q9_dhf_t *)dev->state;
    return dhf_emu_device_read32(&s->dev, addr - dev->base);
}

static void dhf_dev_write32(q9_device_t *dev, uint32_t addr, uint32_t val)
{
    q9_dhf_t *s = (q9_dhf_t *)dev->state;
    dhf_emu_device_write32(&s->dev, addr - dev->base, val);
}

const q9_device_vtable_t q9_devtype_dhf = {
    .read8         = dhf_dev_read8,
    .write8        = dhf_dev_write8,
    .read16        = dhf_dev_read16,
    .write16       = dhf_dev_write16,
    .read32        = dhf_dev_read32,
    .write32       = dhf_dev_write32,
    .poll          = NULL,
    .irq_pending   = NULL,
    .reset         = NULL,
    .irq_vector_fn = NULL,
};

const q9_devdesc_t q9_devdesc_dhf = {
    .type              = "dhf",
    .desc              = "DHF Host-Passthrough-Dateisystem (Zero-Copy MMIO-Bruecke, Q9-DHFDRV-68k-Protokoll)",
    .vt                = &q9_devtype_dhf,
    .use_table_default = 1,
    .extra_fields      = NULL,     /* 2026-09-25: Basepath noch hartkodiert, s. m68krt.c attach --
                                       Config-Feld "hostpath" folgt als eigener Schritt (wie "cf"s
                                       "image"-Feld). */
    .extra_field_count = 0,
};

//────────────────────────────────────────────────────────────────────────────────────────────────
// EOF q9_dhf.c                                                                            Ver. 2.00
//────────────────────────────────────────────────────────────────────────────────────────────────
