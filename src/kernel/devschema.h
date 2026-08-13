//════════════════════════════════════════════════════════════════════════════════════════════════
// File:   devschema.h                                                                     Ver. 1.00
// Owner:  Claudia
// Desc.:  6.7-Pilot: Selbstbeschreibende Feld-Schemata je Geraetetyp (Feldname, Typ, Min/Max oder
//         Enum-Werte, Freitext-Beschreibung) -- Andreas' Idee (2026-08-13): "ein kleines
//         Beschreibungsfile ... welche Felder das Simulationsmodul braucht, welcher Typ, min/max
//         Werte, oder vorgegebene Enums, vielleicht sogar eine Beschreibung".
//
//         Bewusst KEIN Laufzeit-JSON: boardcfg.c ist ausdruecklich "INI-artig, C99-Parser ohne
//         Fremdbibliothek" (s. dortiger Kopfkommentar) -- ein JSON-Parser waere eine neue
//         Abhaengigkeit nur fuer diesen Zweck. Die Idee (Feld/Typ/Min/Max/Enum/Beschreibung, im
//         Prinzip JSON-Schema-Form) bleibt erhalten, nur als statisch kompilierte C-Tabelle, genau
//         im Stil der bestehenden Typ-Registry (devreg.h: q9_devtype_lookup) -- KEINE Linker-
//         Magie, portabel, gleiche Bauweise.
//
//         Reine Beschreibungs-/Validierungs-Infrastruktur, bewusst additiv: boardcfg.c's
//         tatsaechliches Parsing/Verhalten ist von diesem Schritt NOCH NICHT betroffen (kommt erst,
//         wenn Andreas den Ansatz freigibt). Pilot-Abdeckung: nur "cf" (reichhaltigstes Beispiel:
//         Enum-Felder bus/unit/format, Int-Felder mit Min/Max, Pflicht-/Optionalfelder).
//
//         ZWEITER, staerkerer Verwendungszweck (Andreas, 2026-08-13): nicht nur Validierung beim
//         Einlesen, sondern Grundlage fuer einen KUENFTIGEN CONFIG-EDITOR -- der kann dann rein aus
//         dieser Tabelle generisch ableiten, was er anzeigen/abfragen muss (Enum -> Auswahlliste,
//         Int -> Eingabe mit Min/Max-Pruefung, desc -> Hilfetext/Tooltip), ohne dass ein neuer
//         Geraetetyp eine Aenderung am Editor selbst braucht -- nur ein neues Schema hier.
//
//         Bewusst KEIN JSON, auch nicht als Export-Zwischenschritt (Andreas, 2026-08-13, nach
//         eigener Ueberlegung verworfen: C ist nicht dynamisch, ein Editor koennte die Objekte aus
//         JSON ohnehin nur umstaendlich UND unvollstaendig zugreifbar rekonstruieren -- lohnt den
//         Aufwand nicht). Der kuenftige Editor liest diese Tabelle direkt (verlinkt gegen
//         devschema.c) oder ueber eine noch zu bauende C-API, NICHT ueber eine Zwischendatei.
//
// Call:   const q9_devschema_t *s = q9_devschema_lookup("cf");
//         int idx = q9_devschema_find_field(s, "start_sector");
//         q9_devschema_check_int(&s->fields[idx], 42, err, sizeof(err));
//
// Edition History
//─────────┬──────┬────────────────────────────────────────────────────────────────────────┬──────
// Date    │ Ver. │ Description                                                            │ By
//─────────┼──────┼────────────────────────────────────────────────────────────────────────┼──────
// 26-08-13│ 1.00 │ 6.7-Pilot: Erster Wurf -- Schema-Typen, Registry, Validierung fuer "cf"  │ Cld
//═════════╧══════╧════════════════════════════════════════════════════════════════════════╧══════
#ifndef Q9_DEVSCHEMA_H
#define Q9_DEVSCHEMA_H

/* INT: Ganzzahl, optional mit Min/Max (has_range=0 -> unbegrenzt). STR: freier Text (z.B. Pfad,
   keine weitere Pruefung ausser "vorhanden, wenn required"). ENUM: einer von enum_values
   (NULL-terminiertes Array von Strings, Vergleich Case-sensitiv wie der Rest von boardcfg.c). */
typedef enum {
    Q9_FIELD_INT,
    Q9_FIELD_STR,
    Q9_FIELD_ENUM
} q9_field_kind_t;

typedef struct {
    const char            *name;          /* Feldname, wie er in der .q9-Datei stehen wuerde     */
    q9_field_kind_t        kind;
    int                     required;      /* 1 = Pflichtfeld, 0 = optional                       */
    int                     has_range;     /* nur Q9_FIELD_INT: 1 = min/max gelten                */
    long                    min, max;      /* nur wenn has_range                                   */
    const char *const     *enum_values;   /* nur Q9_FIELD_ENUM: NULL-terminiertes Array           */
    const char             *desc;          /* Freitext -- Diagnose, spaeter evtl. Doku-Generierung */
} q9_field_schema_t;

typedef struct {
    const char               *type;         /* z.B. "cf" -- derselbe Name wie in der Typ-Registry
                                                (devreg.h q9_devtype_lookup), aber EIGENE Tabelle:
                                                Schema (Config-Form) und Vtable (Laufzeit-
                                                Verhalten) sind bewusst getrennte Zustaendigkeiten */
    const q9_field_schema_t  *fields;
    int                        field_count;
} q9_devschema_t;

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_devschema_lookup
// Desc.:    Schema-Registry, analog zu q9_devtype_lookup (devreg.h). NULL wenn der Typname kein
//           Schema hat (Pilot-Stand: nur "cf").
//════════════════════════════════════════════════════════════════════════════════════════════════
const q9_devschema_t *q9_devschema_lookup(const char *type);
int                    q9_devschema_count(void);
const q9_devschema_t  *q9_devschema_get(int index);   /* NULL wenn ausserhalb */

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_devschema_find_field
// Desc.:    Sucht ein Feld per Name im Schema. Gibt den Index zurueck oder -1, wenn das Feld im
//           Schema nicht vorkommt (fuer boardcfg.c spaeter: "unbekanntes Feld in [device]-Block").
//════════════════════════════════════════════════════════════════════════════════════════════════
int q9_devschema_find_field(const q9_devschema_t *schema, const char *name);

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_devschema_check_int / q9_devschema_check_enum
// Desc.:    Prueft einen Wert gegen sein Schema-Feld. Gibt 0 bei Erfolg zurueck, sonst -1 und eine
//           erklaerende Meldung in err (max err_max Byte). check_int ignoriert min/max, wenn
//           f->has_range == 0. Beide pruefen NICHT f->kind selbst (falscher Aufruf ist ein
//           Programmierfehler, kein Config-Fehler) -- der Aufrufer waehlt anhand f->kind.
//════════════════════════════════════════════════════════════════════════════════════════════════
int q9_devschema_check_int (const q9_field_schema_t *f, long val,        char *err, unsigned err_max);
int q9_devschema_check_enum(const q9_field_schema_t *f, const char *val, char *err, unsigned err_max);

#endif /* Q9_DEVSCHEMA_H */
//────────────────────────────────────────────────────────────────────────────────────────────────
// EOF devschema.h                                                                         Ver. 1.00
//────────────────────────────────────────────────────────────────────────────────────────────────
