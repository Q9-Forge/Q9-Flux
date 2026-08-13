//════════════════════════════════════════════════════════════════════════════════════════════════
// File:   devschema.c                                                                     Ver. 1.20
// Owner:  Claudia
// Desc.:  Implementierung, siehe devschema.h. Pilot-Schema fuer "cf" -- die Feldnamen/Wertebereiche
//         entsprechen 1:1 q9_cfg_cf_t (boardcfg.h) und Q9_CF_FMT_*/Q9_CFG_BUS_* (q9board.h/
//         boardcfg.h), nur jetzt als geprueftes, selbstbeschreibendes Datum statt implizitem
//         Wissen im Parser.
//
// Edition History
//─────────┬──────┬────────────────────────────────────────────────────────────────────────┬──────
// Date    │ Ver. │ Description                                                            │ By
//─────────┼──────┼────────────────────────────────────────────────────────────────────────┬──────
// 26-08-13│ 1.00 │ 6.7-Pilot: Erster Wurf                                                  │ Cld
// 26-08-13│ 1.10 │ q9_devschema_check_bool + Schema "memory" (RAM/ROM/NVRAM, Andreas'      │ Cld
//         │      │ Editor-Beispiel, noch ohne C-Struct-Gegenstueck -- vorausschauend)       │
// 26-08-14│ 1.20 │ Bugfix "cf"-Schema: Feldnamen auf tatsaechliche .q9-Schluesselwoerter    │ Cld
//         │      │ (image/type statt path/format) korrigiert, Bus/Unit/Format-Synonyme      │
//         │      │ ergaenzt (secondary/0/1/fat) -- beides per grep gegen alle *.q9-Dateien    │
//         │      │ im Repo verifiziert (s. dortiger Fund im Kommentar bei g_cf_bus_values)   │
//═════════╧══════╧════════════════════════════════════════════════════════════════════════╧══════
#include "devschema.h"
#include <string.h>
#include <stdio.h>

/* Enum-Werte entsprechen den .q9-seitigen Bezeichnern, nicht den internen Zahlenwerten
   (Q9_CF_FMT_.., Q9_CFG_BUS_.. in q9board.h/boardcfg.h) -- die Zuordnung Text<->Zahl bleibt
   Aufgabe des Aufrufers (boardcfg.c), dieses Modul kennt nur die gueltigen Texte.

   KORREKTUR (2026-08-14, beim Anschluss an boardcfg.c gefunden): der urspruengliche Pilot-Stand
   kannte nur die KANONISCHEN Werte, nicht die Synonyme, die boardcfg.c's cfg_parse_bus/unit/format
   TATSAECHLICH akzeptieren -- ein Schema-Validator haette damit echte, im Repo vorhandene Configs
   falsch abgelehnt (emu.claude-work.q9 u.a. nutzen z.B. "bus = secondary", nicht "rc2014").
   ELEMENT [0] JEDER LISTE IST DER KANONISCHE WERT (Konvention ab jetzt: ein kuenftiger Editor
   zeigt/schreibt immer nur diesen, akzeptiert aber beim Einlesen bestehender Dateien alle).

   Bekannte, bewusst NICHT behobene Vereinfachung: boardcfg.c vergleicht diese Werte
   GROSS-/KLEINSCHREIBUNGS-UNABHAENGIG (cfg_ieq), waehrend q9_devschema_check_enum() unten exakt
   (Case-sensitiv) vergleicht -- ein Editor schreibt ohnehin immer kanonische Kleinschreibung, echte
   Repo-Dateien tun das ebenfalls (verifiziert), daher aktuell nur ein theoretisches, kein
   beobachtetes Auseinanderklaffen. Nicht "repariert", um Case-sensitive Enums nicht generell fuer
   alle kuenftigen Schemata aufzuweichen. */
static const char *const g_cf_bus_values[]    = { "onboard", "cf", "rc2014", "sc145", "secondary", NULL };
static const char *const g_cf_unit_values[]   = { "master", "0", "slave", "1", NULL };
static const char *const g_cf_format_values[] = { "auto", "rbf", "pcf", "fat", NULL };

static const q9_field_schema_t g_cf_fields[] = {
    /* KORREKTUR (2026-08-14): Feldnamen sind jetzt die .q9-DATEI-Schluesselwoerter (wie sie in
       JEDER echten Config im Repo tatsaechlich stehen -- verifiziert per grep ueber alle *.q9),
       NICHT mehr die internen C-Struct-Feldnamen aus q9_cfg_cf_t (boardcfg.h). Der urspruengliche
       Pilot-Stand hatte hier "path"/"format" -- die tatsaechliche .q9-Syntax ist "image"/"type" (s.
       cfg_ieq(key,"image")||cfg_ieq(key,"file") bzw. cfg_ieq(key,"type")||cfg_ieq(key,"format") in
       boardcfg.c). Ein Schema, das die INTERNEN Namen zeigt, waere fuer einen Config-Editor (der
       ja genau DIESE Datei-Syntax anzeigen/erzeugen soll) irrefuehrend gewesen. boardcfg.c
       akzeptiert zusaetzlich die synonymen Schluesselnamen "file"/"format"/"drive"/
       "offset_sector"/"part_size"/"lsn_offset" -- die sind hier NICHT als Alternative modelliert
       (kein Feldname-Synonym-Mechanismus in q9_field_schema_t, anders als bei Enum-WERTEN), weil
       keine einzige reale Config im Repo sie nutzt; bewusste Vereinfachung, kein Anspruch auf
       Vollstaendigkeit der Parser-Akzeptanz. */
    { "image", Q9_FIELD_STR, 1, 0, 0, 0, NULL,
      "Pfad zum Image (relativ zur Config-Datei aufgeloest)" },
    { "descriptor", Q9_FIELD_STR, 0, 0, 0, 0, NULL,
      "optionaler OS-9-Descriptorname fuer den ROM-Generator" },
    { "bus", Q9_FIELD_ENUM, 0, 0, 0, 0, g_cf_bus_values,
      "welches emulierte CF-Interface (Default onboard)" },
    { "unit", Q9_FIELD_ENUM, 0, 0, 0, 0, g_cf_unit_values,
      "Master/Slave am ATA-Bus (Default master)" },
    { "type", Q9_FIELD_ENUM, 0, 0, 0, 0, g_cf_format_values,
      "Image-Format, auto erkennt RBF/PCF an der Groesse (Default auto)" },
    { "base", Q9_FIELD_INT, 0, 1, 0, 0xFFFFFFFFL, NULL,
      "ATA-Basisadresse; 0 = Standard-Base anhand von bus" },
    { "start_sector", Q9_FIELD_INT, 0, 1, 0, 0xFFFFFFFFL, NULL,
      "Host-Startsektor, der als Gast-LBA 0 erscheint (Default 0)" },
    { "length_sectors", Q9_FIELD_INT, 0, 1, 0, 0xFFFFFFFFL, NULL,
      "logische Partitionslaenge fuer Descriptor/Pruefung" },
    { "descriptor_lsn", Q9_FIELD_INT, 0, 1, 0, 0xFFFFFFFFL, NULL,
      "PD_LSNOffs im OS-9-Descriptor -- fuer spaeteren Descriptor-Abgleich" },
};
#define Q9_CF_FIELD_COUNT (int)(sizeof(g_cf_fields) / sizeof(g_cf_fields[0]))

/* "memory": Andreas' Beispiel-Feldsatz aus der Editor-Planung (docs/Q9FLUX_EDITOR_de.md) fuer
   RAM/ROM/NVRAM-Bereiche -- entspricht noch keinem bestehenden C-Struct (boardcfg.h hat bisher
   keine Speicher-Config, s. ARBEITSPLAN 5.19 "Speicher-/Geraete-Abschnitte", haengt an 5.18) --
   rein vorausschauend beschrieben, wie schon "cf" additiv und ohne Parser-Anschluss. RAM/ROM ist
   EIN Typ mit einem "writable"-Bool statt zwei getrennter Typen (Andreas: "Schreibzugriff:
   Ja/Nein fuer RAM-Simulation sonst ROM"). */
static const q9_field_schema_t g_memory_fields[] = {
    { "name", Q9_FIELD_STR, 1, 0, 0, 0, NULL,
      "Instanzname (z.B. \"sysram\")" },
    { "description", Q9_FIELD_STR, 0, 0, 0, 0, NULL,
      "Kurzbeschreibung, z.B. \"Systemspeicher mit direktem CPU-Zugriff\"" },
    { "start_address", Q9_FIELD_INT, 1, 1, 0, 0xFFFFFFFFL, NULL,
      "Startadresse des Fensters (32 Bit)" },
    { "end_address", Q9_FIELD_INT, 1, 1, 0, 0xFFFFFFFFL, NULL,
      "Endadresse des Fensters (32 Bit, einschliesslich)" },
    { "color_id", Q9_FIELD_INT, 0, 1, 0, 15, NULL,
      "Farb-ID fuer OS-9s MemList (ARBEITSPLAN 5.19 \"colored RAM\")" },
    { "writable", Q9_FIELD_BOOL, 1, 0, 0, 0, NULL,
      "yes = RAM-Simulation (beschreibbar), no = ROM-Simulation (nur lesend)" },
    { "init_image", Q9_FIELD_STR, 0, 0, 0, 0, NULL,
      "Preload-Image fuer ROM-Simulation (Pfad relativ zur Config-Datei)" },
    { "save_after_session", Q9_FIELD_BOOL, 0, 0, 0, 0, NULL,
      "yes = Inhalt nach Sitzungsende zuruecksichern (NVRAM-Simulation)" },
    { "descriptor", Q9_FIELD_BOOL, 0, 0, 0, 0, NULL,
      "wird fuer dieses Geraet ein OS-9-Descriptor gebraucht -- reine Info, bei Speicher meist no" },
};
#define Q9_MEMORY_FIELD_COUNT (int)(sizeof(g_memory_fields) / sizeof(g_memory_fields[0]))

static const q9_devschema_t g_device_schemas[] = {
    { "cf",     g_cf_fields,     Q9_CF_FIELD_COUNT },
    { "memory", g_memory_fields, Q9_MEMORY_FIELD_COUNT },
};
#define Q9_DEVSCHEMA_COUNT (int)(sizeof(g_device_schemas) / sizeof(g_device_schemas[0]))

const q9_devschema_t *q9_devschema_lookup(const char *type)
{
    int i;
    if (!type) {
        return NULL;
    }
    for (i = 0; i < Q9_DEVSCHEMA_COUNT; i++) {
        if (strcmp(g_device_schemas[i].type, type) == 0) {
            return &g_device_schemas[i];
        }
    }
    return NULL;
}

int q9_devschema_count(void)
{
    return Q9_DEVSCHEMA_COUNT;
}

const q9_devschema_t *q9_devschema_get(int index)
{
    if (index < 0 || index >= Q9_DEVSCHEMA_COUNT) {
        return NULL;
    }
    return &g_device_schemas[index];
}

int q9_devschema_find_field(const q9_devschema_t *schema, const char *name)
{
    int i;
    if (!schema || !name) {
        return -1;
    }
    for (i = 0; i < schema->field_count; i++) {
        if (strcmp(schema->fields[i].name, name) == 0) {
            return i;
        }
    }
    return -1;
}

int q9_devschema_check_int(const q9_field_schema_t *f, long val, char *err, unsigned err_max)
{
    if (!f) {
        return -1;
    }
    if (f->has_range && (val < f->min || val > f->max)) {
        if (err && err_max) {
            snprintf(err, err_max, "%s: %ld ausserhalb [%ld..%ld]", f->name, val, f->min, f->max);
        }
        return -1;
    }
    return 0;
}

int q9_devschema_check_enum(const q9_field_schema_t *f, const char *val, char *err, unsigned err_max)
{
    int i;
    if (!f || !val) {
        return -1;
    }
    if (f->enum_values) {
        for (i = 0; f->enum_values[i] != NULL; i++) {
            if (strcmp(f->enum_values[i], val) == 0) {
                return 0;
            }
        }
    }
    if (err && err_max) {
        snprintf(err, err_max, "%s: '%s' ist kein gueltiger Wert", f->name, val);
    }
    return -1;
}

int q9_devschema_check_bool(const q9_field_schema_t *f, const char *val, char *err, unsigned err_max)
{
    if (!f || !val) {
        return -1;
    }
    if (strcmp(val, "yes") == 0 || strcmp(val, "no") == 0) {
        return 0;
    }
    if (err && err_max) {
        snprintf(err, err_max, "%s: '%s' ist kein gueltiger Bool-Wert (erwartet yes/no)", f->name, val);
    }
    return -1;
}

//────────────────────────────────────────────────────────────────────────────────────────────────
// EOF devschema.c                                                                         Ver. 1.10
//────────────────────────────────────────────────────────────────────────────────────────────────
