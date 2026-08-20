//════════════════════════════════════════════════════════════════════════════════════════════════
// File:   devdesc.h                                                                       Ver. 1.00
// Owner:  Cld
// Desc.:  Hardware-Vereinheitlichung (2026-08-20, Pilot "cf", s. Q9FLUX_EDITOR_de.md): Ein
//         "Device-Deskriptor" q9_devdesc_t buendelt pro Geraetetyp das, was bisher ueber drei Stellen
//         verteilt war -- Laufzeit-Vtable (bisher nur devreg.h/q9_devtype_lookup, praktisch unbenutzt),
//         Feldbeschreibung fuers UI/Config (bisher devschema.h, nicht verdrahtet) und die neue
//         Fast-Table-Teilnahme (devreg.h q9_device_t.use_table) -- an EINEM Ort. Andreas' Vorgabe
//         (2026-08-19): "Ich faende es am besten wenn wir pro Hardware ein eigenes Sourcefile
//         bekommen wuerden" -- ein q9_devdesc_t lebt deshalb NICHT hier, sondern jeweils in der
//         Pro-Typ-Datei selbst (z.B. src/devices/cf/cf.c: q9_devdesc_cf), genau wie devreg.h's
//         Typ-Registry ihre Vtables aus den Pro-Typ-Dateien bezieht (kein Linker-Trick -- ein neuer
//         Typ traegt sich hier per Hand in g_devdesc_registry[] ein, s. devdesc.c).
//
//         Gemeinsame Basisfelder (Name/Basisadresse/Endadresse) sind bewusst NICHT Teil dieser
//         Tabelle -- sie sind schon strukturelle Member von q9_device_t (base/size, devreg.h) und
//         werden dort einmal, nicht pro Typ wiederholt, gepflegt. Die zwei verbleibenden, wirklich
//         GEMEINSAMEN Editor-Felder (descriptor/descriptorName, s. devschema.c-Historie 2026-08-14 --
//         "descriptor" ist projektweit einheitlich ein Q9_FIELD_BOOL) sind hier EINMAL als
//         q9_devschema_common_fields[] definiert, damit kein Typ sie erneut abtippen muss -- ein
//         Aufrufer (Editor/boardcfg.c), der die vollstaendige Feldliste eines Typs braucht, haengt
//         extra_fields[] gedanklich HINTER die gemeinsamen Felder.
//
// Call:   const q9_devdesc_t *d = q9_devdesc_lookup("cf");
//         if (d->use_table_default) { ... }
//         for (i = 0; i < d->extra_field_count; i++) { ... d->extra_fields[i] ... }
//
// Edition History
//─────────┬──────┬────────────────────────────────────────────────────────────────────────┬──────
// Date    │ Ver. │ Description                                                            │ By
//─────────┼──────┼────────────────────────────────────────────────────────────────────────┼──────
// 26-08-20│ 1.00 │ Hardware-Vereinheitlichung, Pilot "cf": Erster Wurf                      │ Cld
//═════════╧══════╧════════════════════════════════════════════════════════════════════════╧══════
#ifndef Q9_DEVDESC_H
#define Q9_DEVDESC_H

#include "devreg.h"
#include "devschema.h"

/* Zwei Felder, die JEDER Geraetetyp im Editor gleich anbietet (s. Kopfkommentar) -- einmal definiert
   in devdesc.c, hier nur deklariert. Inhalt entspricht 1:1 dem, was bisher in devschema.c's "cf"-
   und "memory"-Schemata jeweils separat stand. */
extern const q9_field_schema_t q9_devschema_common_fields[2];   /* [0]=descriptor, [1]=descriptorName */

typedef struct {
    const char                *type;              /* z.B. "cf" -- gleicher Name wie in q9_device_t.type
                                                       und in der devreg-Typ-Registry                 */
    const char                *desc;               /* Kurzbeschreibung fuer den Editor                */
    const q9_device_vtable_t  *vt;                 /* Laufzeit-Dispatch (devreg.h)                    */
    /* Default fuer q9_device_t.use_table bei der Registrierung dieses Typs (s. devreg.h) -- ein
       Aufrufer KANN das je Instanz ueberschreiben (z.B. Config-Feld "useSlot"=no fuer eine bestimmte
       Instanz), dieser Wert ist nur der sinnvolle Vorbelegungswert fuer den Typ als Ganzes. */
    int                         use_table_default;
    /* Typspezifische Zusatzfelder (devschema-Stil), OHNE die gemeinsamen Basisfelder (s.o.) --
       NULL/0, wenn ein Typ keine Zusatzfelder braucht. */
    const q9_field_schema_t   *extra_fields;
    int                         extra_field_count;
} q9_devdesc_t;

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_devdesc_lookup
// Desc.:    Registry-Zugriff, analog q9_devtype_lookup (devreg.h) / q9_devschema_lookup (devschema.h)
//           -- aber vereinheitlicht: EIN Lookup liefert Vtable UND Feldbeschreibung UND Fast-Table-
//           Default zusammen. NULL, wenn der Typname (noch) keinen Deskriptor hat.
//════════════════════════════════════════════════════════════════════════════════════════════════
const q9_devdesc_t *q9_devdesc_lookup(const char *type);
int                  q9_devdesc_count(void);
const q9_devdesc_t  *q9_devdesc_get(int index);          /* NULL wenn ausserhalb */

#endif /* Q9_DEVDESC_H */
//────────────────────────────────────────────────────────────────────────────────────────────────
// EOF devdesc.h                                                                           Ver. 1.00
//────────────────────────────────────────────────────────────────────────────────────────────────
