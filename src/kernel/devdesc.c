//════════════════════════════════════════════════════════════════════════════════════════════════
// File:   devdesc.c                                                                       Ver. 1.00
// Owner:  Cld
// Desc.:  Implementierung, siehe devdesc.h. Enthaelt NUR die Registry-Tabelle -- die einzelnen
//         q9_devdesc_t-Instanzen (z.B. q9_devdesc_cf) sind in den jeweiligen Pro-Typ-Dateien
//         definiert (s. devdesc.h-Kopfkommentar), genau wie devreg.c's g_device_types[] seine
//         Vtables aus den Pro-Typ-Dateien bezieht statt sie selbst zu definieren.
//
// Edition History
//─────────┬──────┬────────────────────────────────────────────────────────────────────────┬──────
// Date    │ Ver. │ Description                                                            │ By
//─────────┼──────┼────────────────────────────────────────────────────────────────────────┬──────
// 26-08-20│ 1.00 │ Hardware-Vereinheitlichung, Pilot "cf": Erster Wurf                      │ Cld
//═════════╧══════╧════════════════════════════════════════════════════════════════════════╧══════
#include "devdesc.h"
#include "../devices/cf/cf.h"
#include <string.h>

/* Descriptor/DescriptorName -- EINMAL definiert (s. devdesc.h-Kopfkommentar), Inhalt entspricht
   1:1 den bisher in devschema.c's "cf"- und "memory"-Schemata jeweils separat wiederholten Eintraegen
   (s. dortige Historie 2026-08-14: "descriptor" projektweit einheitlich Q9_FIELD_BOOL). */
const q9_field_schema_t q9_devschema_common_fields[2] = {
    {
        .name = "descriptor", .kind = Q9_FIELD_BOOL,
        .desc = "braucht dieses Geraet einen OS-9-Descriptor",
    },
    {
        .name = "descriptorName", .kind = Q9_FIELD_STR,
        .depends_on = "descriptor", .depends_on_value = "yes",
        .desc = "Descriptorname -- nur relevant wenn descriptor=yes",
    },
};

//────────────────────────────────────────────────────────────────────────────────────────────────
// Registry (s. devdesc.h) — waechst mit jedem Migrationsschritt um einen Eintrag, analog
// devreg.c's g_device_types[]. Pilot-Stand: nur "cf".
//────────────────────────────────────────────────────────────────────────────────────────────────
static const q9_devdesc_t *const g_devdesc_registry[] = {
    &q9_devdesc_cf,
};
#define Q9_DEVDESC_COUNT (int)(sizeof(g_devdesc_registry) / sizeof(g_devdesc_registry[0]))

const q9_devdesc_t *q9_devdesc_lookup(const char *type)
{
    int i;
    if (!type) {
        return NULL;
    }
    for (i = 0; i < Q9_DEVDESC_COUNT; i++) {
        if (strcmp(g_devdesc_registry[i]->type, type) == 0) {
            return g_devdesc_registry[i];
        }
    }
    return NULL;
}

int q9_devdesc_count(void)
{
    return Q9_DEVDESC_COUNT;
}

const q9_devdesc_t *q9_devdesc_get(int index)
{
    if (index < 0 || index >= Q9_DEVDESC_COUNT) {
        return NULL;
    }
    return g_devdesc_registry[index];
}

//────────────────────────────────────────────────────────────────────────────────────────────────
// EOF devdesc.c                                                                           Ver. 1.00
//────────────────────────────────────────────────────────────────────────────────────────────────
