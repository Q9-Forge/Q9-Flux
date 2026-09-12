//════════════════════════════════════════════════════════════════════════════════════════════════
// File:   devdesc.c                                                                       Ver. 1.40
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
// 26-08-21│ 1.10 │ quicc/mc6845/framebuf/clut dazu (bereits eigene Dateien seit "6.6", nur    │ Cld
//         │      │ der devdesc-Eintrag war neu) -- DEVDESC_SRC (Makefile) fasst die dadurch    │
//         │      │ noetigen Link-Abhaengigkeiten an einer Stelle zusammen                      │
// 26-08-21│ 1.20 │ duart68681/rtc72421/timer_irq dazu -- letzte drei Typen, die noch in         │ Cld
//         │      │ q9board.c gebuendelt waren, jetzt ebenfalls eigene Dateien. Damit sind acht   │
//         │      │ von neun heutigen Hardware-Typen registriert -- nur nettty (m68krt.c,         │
//         │      │ EIN Geraet fuer alle acht Netz-Terminal-Kanaele) fehlt noch                   │
// 26-08-21│ 1.30 │ nettty dazu -- letzter Typ, damit sind ALLE NEUN heutigen Hardware-Typen       │ Cld
//         │      │ registriert. nettty.c bewusst OHNE Musashi-Abhaengigkeit gehalten (Funktions-  │
//         │      │ zeiger-Hook statt direktem m68k_set_irq()), sonst haette DEVDESC_SRC jeden      │
//         │      │ Aufrufer (Editor, leichte Testziele) gezwungen, die volle CPU-Kernobjekte       │
//         │      │ mitzulinken -- s. nettty.c-Kopfkommentar                                        │
// 26-08-21│ 1.40 │ remap dazu (der REMAP-Trigger, vormals Sonderfall in q9board.c) -- Andreas'    │ Cld
//         │      │ Idee, auch die "Adressraum-Topologie" als Geraet zu modellieren. NUR der        │
//         │      │ Trigger selbst, NICHT die RAM/ROM-Interpretation (bleibt Performance-Fast-Path)│
//═════════╧══════╧════════════════════════════════════════════════════════════════════════╧══════
#include "devdesc.h"
#include "../devices/cf/cf.h"
#include "../devices/quicc/quicc.h"
#include "../devices/mc6845/mc6845.h"
#include "../devices/framebuf/framebuf.h"
#include "../devices/clut/clut.h"
#include "../devices/duart68681/duart68681.h"
#include "../devices/rtc72421/rtc72421.h"
#include "../devices/timer_irq/timer_irq.h"
#include "../devices/nettty/nettty.h"
#include "../devices/remap/remap.h"
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
    &q9_devdesc_quicc,
    &q9_devdesc_mc6845,
    &q9_devdesc_framebuf,
    &q9_devdesc_clut,
    &q9_devdesc_duart68681,
    &q9_devdesc_rtc72421,
    &q9_devdesc_timer_irq,
    &q9_devdesc_nettty,
    &q9_devdesc_remap,
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
// EOF devdesc.c                                                                           Ver. 1.40
//────────────────────────────────────────────────────────────────────────────────────────────────
