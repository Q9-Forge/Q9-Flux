//════════════════════════════════════════════════════════════════════════════════════════════════
// File:   devschema.c                                                                     Ver. 1.50
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
// 26-08-14│ 1.30 │ descriptor-Konflikt geloest: projektweit einheitlich Q9_FIELD_BOOL +      │ Cld
//         │      │ neues descriptorName (depends_on descriptor=yes). Feldtabellen auf       │
//         │      │ Designated Initializers umgestellt (Projektstil, s. Vtables) -- bei 9      │
//         │      │ Feldern je Eintrag wurden positionelle Initializer unuebersichtlich/       │
//         │      │ fehleranfaellig fuer kuenftige Erweiterungen                               │
// 26-08-14│ 1.40 │ "cf": useSlot (Bool) + slot (Int 0-255) ergaenzt -- Config-seitige Wahl    │ Cld
//         │      │ zwischen automatisch zugeteiltem I/O-Tabellenplatz (5.18 g_io_table) und   │
//         │      │ freier Adresse, vorausschauend, noch ohne boardcfg.c-Anschluss             │
// 26-08-15│ 1.50 │ Neues Schema "board" (cpu-Feld, ENUM) -- Q9FLUX_EDITOR_de.md 4.1, ECHTES   │ Cld
//         │      │ boardcfg.c-Schluesselwort (anders als "memory" oben, das noch vorausschauend│
//         │      │ ohne Parser-Anschluss ist)                                                  │
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

/* descriptor/descriptorName (2026-08-14, mit Andreas geklaert): "descriptor" ist PROJEKTWEIT
   einheitlich Q9_FIELD_BOOL ("braucht dieses Geraet einen Descriptor" -- bei cf i.d.R. yes). Der
   bisherige String-Wert (Descriptor-NAME fuer den ROM-Generator) heisst jetzt "descriptorName"
   und ist nur relevant, wenn descriptor=yes (depends_on). ECHTES boardcfg.c-Schluesselwort, s.
   dortige Kommentare -- alle 9 betroffenen .q9-Dateien im Repo wurden mitmigriert. */
static const q9_field_schema_t g_cf_fields[] = {
    /* KORREKTUR (2026-08-14): Feldnamen sind die .q9-DATEI-Schluesselwoerter (wie sie in JEDER
       echten Config im Repo tatsaechlich stehen -- verifiziert per grep ueber alle *.q9), NICHT
       die internen C-Struct-Feldnamen aus q9_cfg_cf_t (boardcfg.h). Der urspruengliche Pilot-Stand
       hatte hier "path"/"format" -- die tatsaechliche .q9-Syntax ist "image"/"type" (s.
       cfg_ieq(key,"image")||cfg_ieq(key,"file") bzw. cfg_ieq(key,"type")||cfg_ieq(key,"format") in
       boardcfg.c). boardcfg.c akzeptiert zusaetzlich die synonymen Schluesselnamen "file"/
       "format"/"drive"/"offset_sector"/"part_size"/"lsn_offset" -- die sind hier NICHT als
       Alternative modelliert (kein Feldname-Synonym-Mechanismus in q9_field_schema_t, anders als
       bei Enum-WERTEN), weil keine einzige reale Config im Repo sie nutzt; bewusste
       Vereinfachung, kein Anspruch auf Vollstaendigkeit der Parser-Akzeptanz. */
    {
        .name = "image", .kind = Q9_FIELD_STR, .required = 1,
        .desc = "Pfad zum Image (relativ zur Config-Datei aufgeloest)",
    },
    {
        .name = "descriptor", .kind = Q9_FIELD_BOOL,
        .desc = "braucht dieses Geraet einen OS-9-Descriptor (Default: yes)",
    },
    {
        .name = "descriptorName", .kind = Q9_FIELD_STR,
        .depends_on = "descriptor", .depends_on_value = "yes",
        .desc = "Descriptorname fuer den ROM-Generator -- nur relevant wenn descriptor=yes",
    },
    {
        .name = "bus", .kind = Q9_FIELD_ENUM, .enum_values = g_cf_bus_values,
        .desc = "welches emulierte CF-Interface (Default onboard)",
    },
    {
        .name = "unit", .kind = Q9_FIELD_ENUM, .enum_values = g_cf_unit_values,
        .desc = "Master/Slave am ATA-Bus (Default master)",
    },
    {
        .name = "type", .kind = Q9_FIELD_ENUM, .enum_values = g_cf_format_values,
        .desc = "Image-Format, auto erkennt RBF/PCF an der Groesse (Default auto)",
    },
    /* 2026-08-14 (Andreas' Vorschlag, Fortsetzung von ARBEITSPLAN 5.18 "Config-seitig bewusst
       BEIDES anbieten"): Wahl zwischen einem automatisch zugeteilten I/O-Tabellenplatz (schneller
       Dispatch, s. m68krt.c g_io_table) und einer frei gewaehlten Adresse. Nur additiv beschrieben,
       NOCH KEIN boardcfg.c-Anschluss (die eigentliche Config-gesteuerte Instanziierung ueber die
       Tabelle ist selbst noch nicht gebaut, s. 5.18 "Weiterhin offen"). Gehoert nur zu Geraeten im
       festen 64-KB-I/O-Cluster ($FFFF0000-$FFFFFFFF) -- NICHT zu "memory" (RAM liegt ausserhalb). */
    {
        .name = "useSlot", .kind = Q9_FIELD_BOOL,
        .desc = "vordefinierten I/O-Tabellenplatz nutzen (schneller Dispatch, 256-Byte-Raster ab "
                "$FFFF0000) statt einer frei gewaehlten Adresse -- Default no",
    },
    {
        .name = "slot", .kind = Q9_FIELD_INT, .has_range = 1, .min = 0, .max = 255,
        .depends_on = "useSlot", .depends_on_value = "yes",
        .desc = "I/O-Tabellenplatz-Nummer (0-255); Adresse = $FFFF0000 + slot*256 -- nur relevant "
                "wenn useSlot=yes",
    },
    {
        .name = "base", .kind = Q9_FIELD_INT, .has_range = 1, .min = 0, .max = 0xFFFFFFFFL,
        .desc = "ATA-Basisadresse; 0 = Standard-Base anhand von bus. Wenn useSlot=yes wird dieser "
                "Wert automatisch aus slot berechnet (Editor: nur anzeigen, nicht eingeben lassen)",
    },
    {
        .name = "start_sector", .kind = Q9_FIELD_INT, .has_range = 1, .min = 0, .max = 0xFFFFFFFFL,
        .desc = "Host-Startsektor, der als Gast-LBA 0 erscheint (Default 0)",
    },
    {
        .name = "length_sectors", .kind = Q9_FIELD_INT, .has_range = 1, .min = 0, .max = 0xFFFFFFFFL,
        .desc = "logische Partitionslaenge fuer Descriptor/Pruefung",
    },
    {
        .name = "descriptor_lsn", .kind = Q9_FIELD_INT, .has_range = 1, .min = 0, .max = 0xFFFFFFFFL,
        .desc = "PD_LSNOffs im OS-9-Descriptor -- fuer spaeteren Descriptor-Abgleich",
    },
};
#define Q9_CF_FIELD_COUNT (int)(sizeof(g_cf_fields) / sizeof(g_cf_fields[0]))

/* "memory": Andreas' Beispiel-Feldsatz aus der Editor-Planung (docs/Q9FLUX_EDITOR_de.md) fuer
   RAM/ROM/NVRAM-Bereiche -- entspricht noch keinem bestehenden C-Struct (boardcfg.h hat bisher
   keine Speicher-Config, s. ARBEITSPLAN 5.19 "Speicher-/Geraete-Abschnitte", haengt an 5.18) --
   rein vorausschauend beschrieben, wie schon "cf" additiv und ohne Parser-Anschluss. RAM/ROM ist
   EIN Typ mit einem "writable"-Bool statt zwei getrennter Typen (Andreas: "Schreibzugriff:
   Ja/Nein fuer RAM-Simulation sonst ROM"). */
static const q9_field_schema_t g_memory_fields[] = {
    {
        .name = "name", .kind = Q9_FIELD_STR, .required = 1,
        .desc = "Instanzname (z.B. \"sysram\")",
    },
    {
        .name = "description", .kind = Q9_FIELD_STR,
        .desc = "Kurzbeschreibung, z.B. \"Systemspeicher mit direktem CPU-Zugriff\"",
    },
    {
        .name = "start_address", .kind = Q9_FIELD_INT, .required = 1, .has_range = 1,
        .min = 0, .max = 0xFFFFFFFFL,
        .desc = "Startadresse des Fensters (32 Bit)",
    },
    {
        .name = "end_address", .kind = Q9_FIELD_INT, .required = 1, .has_range = 1,
        .min = 0, .max = 0xFFFFFFFFL,
        .desc = "Endadresse des Fensters (32 Bit, einschliesslich)",
    },
    {
        .name = "color_id", .kind = Q9_FIELD_INT, .has_range = 1, .min = 0, .max = 15,
        .desc = "Farb-ID fuer OS-9s MemList (ARBEITSPLAN 5.19 \"colored RAM\")",
    },
    {
        .name = "writable", .kind = Q9_FIELD_BOOL, .required = 1,
        .desc = "yes = RAM-Simulation (beschreibbar), no = ROM-Simulation (nur lesend)",
    },
    {
        .name = "init_image", .kind = Q9_FIELD_STR,
        .desc = "Preload-Image fuer ROM-Simulation (Pfad relativ zur Config-Datei)",
    },
    {
        .name = "save_after_session", .kind = Q9_FIELD_BOOL,
        .desc = "yes = Inhalt nach Sitzungsende zuruecksichern (NVRAM-Simulation)",
    },
    {
        .name = "descriptor", .kind = Q9_FIELD_BOOL,
        .desc = "braucht dieses Geraet einen OS-9-Descriptor (Default: no -- Speicher braucht i.d.R. keinen)",
    },
    {
        .name = "descriptorName", .kind = Q9_FIELD_STR,
        .depends_on = "descriptor", .depends_on_value = "yes",
        .desc = "Descriptorname -- nur relevant wenn descriptor=yes",
    },
};
#define Q9_MEMORY_FIELD_COUNT (int)(sizeof(g_memory_fields) / sizeof(g_memory_fields[0]))

/* "board": Q9FLUX_EDITOR_de.md 4.1 (CPU-Auswahl) -- im Unterschied zu "memory" oben entspricht
   dieses Schema bereits einem ECHTEN, geparsten .q9-Schluesselwort (boardcfg.c [board] "cpu",
   s. dortige Kommentare). Nur EIN Feld bisher -- die uebrigen [board]-Grundkonfiguration-Punkte
   (Name/Kurzbeschreibung/Netzwerk) sind laut Editor-Plan noch nicht so weit entschieden, dass sie
   sich sauber schematisieren liessen (Netzwerk-Backend-Frage insbesondere, s. dortiger Abschnitt
   8a). Kein Synonym-Mechanismus noetig: boardcfg.c akzeptiert fuer "cpu" ausschliesslich diese
   neun kanonischen Tokens (kein bus/unit-artiges Synonym-Bedarf wie bei "cf"). */
static const char *const g_board_cpu_values[] = {
    "68030", "68000", "68010", "68020", "68ec020", "68ec030", "68040", "68ec040", "68lc040", NULL
};

static const q9_field_schema_t g_board_fields[] = {
    {
        .name = "cpu", .kind = Q9_FIELD_ENUM, .enum_values = g_board_cpu_values,
        .desc = "CPU-Typ (Musashi-Emulation, Default 68030 = echte Q9-Hardware) -- nicht jede "
                "Wahl bootet das echte OS-9-Image erfolgreich (OS-9/68030 braucht eine PMMU, die "
                "z.B. 68000/68010 nicht haben)",
    },
};
#define Q9_BOARD_FIELD_COUNT (int)(sizeof(g_board_fields) / sizeof(g_board_fields[0]))

static const q9_devschema_t g_device_schemas[] = {
    { "cf",     g_cf_fields,     Q9_CF_FIELD_COUNT },
    { "memory", g_memory_fields, Q9_MEMORY_FIELD_COUNT },
    { "board",  g_board_fields,  Q9_BOARD_FIELD_COUNT },
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

int q9_devschema_field_relevant(const q9_field_schema_t *f, const char *other_field_current_value)
{
    if (!f) {
        return 0;
    }
    if (!f->depends_on) {
        return 1;                                          /* keine Bedingung -- immer relevant */
    }
    if (!other_field_current_value) {
        return 0;                                          /* Bedingung existiert, Wert unbekannt -> vorsichtig "nicht relevant" */
    }
    return strcmp(other_field_current_value, f->depends_on_value) == 0;
}

//────────────────────────────────────────────────────────────────────────────────────────────────
// EOF devschema.c                                                                         Ver. 1.50
//────────────────────────────────────────────────────────────────────────────────────────────────
