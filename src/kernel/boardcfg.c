//════════════════════════════════════════════════════════════════════════════════════════════════
// File:   boardcfg.c                                                                      Ver. 1.60
// Owner:  AF
// Desc.:  Implementierung des Board-Config-Parsers, siehe boardcfg.h. INI-artig, C99, ohne
//         Fremdbibliothek. Bewusst schlank: nur die Abschnitte/Keys, die 5.19a heute braucht
//         (ROM, Netz, mehrere CF-Images rbf/pcf) — spaetere Ausbaustufen (Speicher-/Geraete-
//         Abschnitte aus docs/HWCONFIG.md) koennen hier andocken.
//
// Edition History
//─────────┬──────┬────────────────────────────────────────────────────────────────────────┬──────
// Date    │ Ver. │ Description                                                            │ By
//─────────┼──────┼────────────────────────────────────────────────────────────────────────┬──────
// 26-07-16│ 1.00 │ 5.19: Erster Wurf                                                        │ CF
// 26-08-07│ 1.10 │ 5.14: net_hostfwd-Key (Host->Gast-Portweiterleitung fuer net=slirp)       │ AF
// 26-08-14│ 1.20 │ descriptor-Key jetzt Bool (yes/no) statt Pfad, neuer descriptorName-Key   │ Cld
//         │      │ (Nachtrag: Versionskopf war beim urspruenglichen Commit vergessen worden) │
// 26-08-14│ 1.30 │ useSlot/slot-Keys (5.18-Fortsetzung): waehlen einen automatisch           │ Cld
//         │      │ zugeteilten I/O-Tabellenplatz statt einer freien "base". Neue Funktion    │
//         │      │ q9_cfg_cf_effective_base() -- einzige Stelle fuer diese Berechnung, ersetzt│
//         │      │ drei bisher duplizierte Inline-Berechnungen (hier + zweimal              │
//         │      │ q9boardrun.c). Nachvalidierung: useSlot=yes ohne slot= ist jetzt ein       │
//         │      │ Parse-Fehler                                                              │
// 26-08-15│ 1.40 │ Q9FLUX_EDITOR_de.md 4.1: neuer [board]-Key "cpu" (Whitelist-Validierung),  │ Cld
//         │      │ macht die CPU-Typ-Wahl aus m68krt.h q9_cpu_type_t config-steuerbar         │
// 26-08-18│ 1.50 │ q9_board_cfg_save() NEU (Gegenstueck zu q9_board_cfg_load(), Q9FLUX_EDITOR_ │ Cld
//         │      │ de.md, Andreas: "Speichern-Funktion") + Hilfsfunktion cfg_relativize()      │
//         │      │ (Umkehrung von cfg_resolve_rel())                                           │
// 26-08-20│ 1.60 │ Hardware-Vereinheitlichung, Pilot "cf": cfg_schema_confirm_invalid_enum()    │ Cld
//         │      │ NEU -- reine Diagnose-Gegenprobe auf dem bereits-ungueltig-Pfad von          │
//         │      │ type/bus/unit gegen q9_devdesc_lookup("cf") (devdesc.h), beweist dass das    │
//         │      │ Schema jetzt load-bearing ist. KEINE Aenderung am eigentlichen Parse-/        │
//         │      │ Fehlerpfad (Rueckgabewert/err-Text unveraendert)                              │
//═════════╧══════╧════════════════════════════════════════════════════════════════════════╧══════
#include "boardcfg.h"
#include "q9board.h"                                     /* Q9_CF_FMT_*                            */
#include "devdesc.h"                                     /* 2026-08-20: Schema-Gegenprobe, s.u.    */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

//────────────────────────────────────────────────────────────────────────────────────────────────
// Kleine String-Helfer (kein strdup/keine dynamische Allokation — Q9-Grundsatz).
//────────────────────────────────────────────────────────────────────────────────────────────────
static void cfg_copy(char *dst, unsigned dst_max, const char *src)
{
    unsigned i = 0;
    if (dst_max == 0) {
        return;
    }
    for (; src[i] && i + 1u < dst_max; i++) {
        dst[i] = src[i];
    }
    dst[i] = '\0';
}

static char *cfg_trim(char *s)
{
    char *end;
    while (*s == ' ' || *s == '\t' || *s == '\r' || *s == '\n') {
        s++;
    }
    end = s + strlen(s);
    while (end > s) {
        char c = end[-1];
        if (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
            end--;
        } else {
            break;
        }
    }
    *end = '\0';
    return s;
}

static int cfg_ieq(const char *a, const char *b)
{
    while (*a && *b) {
        char ca = *a, cb = *b;
        if (ca >= 'A' && ca <= 'Z') ca = (char)(ca - 'A' + 'a');
        if (cb >= 'A' && cb <= 'Z') cb = (char)(cb - 'A' + 'a');
        if (ca != cb) {
            return 0;
        }
        a++; b++;
    }
    return *a == '\0' && *b == '\0';
}

//────────────────────────────────────────────────────────────────────────────────────────────────
// Function: cfg_dir_of
// Desc.:    Schreibt das Verzeichnis von cfg_path (mit abschliessendem '/') nach out; leer, wenn
//           cfg_path keinen Pfadtrenner enthaelt (Config liegt im CWD). Behandelt '/' und '\\'.
//────────────────────────────────────────────────────────────────────────────────────────────────
static void cfg_dir_of(const char *cfg_path, char *out, unsigned out_max)
{
    const char *slash = 0;
    const char *p;
    unsigned    len;

    for (p = cfg_path; *p; p++) {
        if (*p == '/' || *p == '\\') {
            slash = p;
        }
    }
    if (!slash) {
        out[0] = '\0';
        return;
    }
    len = (unsigned)(slash - cfg_path) + 1u;             /* inkl. Trenner selbst                  */
    if (len >= out_max) {
        len = out_max - 1u;
    }
    memcpy(out, cfg_path, len);
    out[len] = '\0';
}

//────────────────────────────────────────────────────────────────────────────────────────────────
// Function: cfg_resolve_rel
// Desc.:    Loest value relativ zum Config-Verzeichnis auf (docs/HWCONFIG.md: Pfade in der Datei
//           sind relativ zur CONFIG-DATEI). Absolute Pfade (fuehrender '/' oder Windows "X:")
//           bleiben unveraendert.
//────────────────────────────────────────────────────────────────────────────────────────────────
static void cfg_resolve_rel(const char *dir, const char *value, char *out, unsigned out_max)
{
    int absolute = (value[0] == '/' || value[0] == '\\' ||
                    (value[0] && value[1] == ':'));      /* "C:\..." unter Windows                */
    if (absolute || dir[0] == '\0') {
        cfg_copy(out, out_max, value);
        return;
    }
    if (snprintf(out, out_max, "%s%s", dir, value) < 0) {
        cfg_copy(out, out_max, value);
    }
}

//────────────────────────────────────────────────────────────────────────────────────────────────
// Function: cfg_relativize
// Desc.:    Umkehrung von cfg_resolve_rel() fuer q9_board_cfg_save() -- beginnt path mit genau dem
//           Verzeichnis dir (das cfg_resolve_rel() beim Laden vorangestellt hat), wird dieser
//           Praefix wieder abgeschnitten (path bleibt relativ). Passt path NICHT (bleibt absolut
//           im out), wenn dir leer ist oder path NICHT mit dir beginnt (z.B. weil der Nutzer beim
//           Laden bereits einen absoluten Pfad angegeben hatte, s. cfg_resolve_rel()).
//────────────────────────────────────────────────────────────────────────────────────────────────
static void cfg_relativize(const char *dir, const char *path, char *out, unsigned out_max)
{
    unsigned dlen = (unsigned)strlen(dir);
    if (dlen > 0 && strncmp(path, dir, dlen) == 0) {
        cfg_copy(out, out_max, path + dlen);
    } else {
        cfg_copy(out, out_max, path);
    }
}

void q9_board_cfg_default(q9_board_cfg_t *cfg)
{
    memset(cfg, 0, sizeof(*cfg));
    /* Bestehende OS-9-Netzkonfiguration: nur die Config kann diese Werte ueberschreiben. */
    cfg_copy(cfg->vmnet_ip,       sizeof(cfg->vmnet_ip),       "192.168.200.2");
    cfg_copy(cfg->vmnet_gateway,  sizeof(cfg->vmnet_gateway),  "192.168.200.1");
    cfg_copy(cfg->vmnet_netmask,  sizeof(cfg->vmnet_netmask),  "255.255.255.0");
    cfg_copy(cfg->vmnet_dhcp_end, sizeof(cfg->vmnet_dhcp_end), "192.168.200.254");
}

void q9_board_cfg_resolve_path(const char *arg, char *out, unsigned out_max)
{
    const char *base = arg;
    const char *p;
    int         has_dot = 0;

    /* letztes Pfadsegment isolieren, damit ein Punkt im Verzeichnisnamen nicht als Extension zaehlt */
    for (p = arg; *p; p++) {
        if (*p == '/' || *p == '\\') {
            base = p + 1;
        }
    }
    for (p = base; *p; p++) {
        if (*p == '.') {
            has_dot = 1;
        }
    }
    if (has_dot) {
        cfg_copy(out, out_max, arg);
    } else if (snprintf(out, out_max, "%s.q9", arg) < 0) {
        cfg_copy(out, out_max, arg);
    }
}

//────────────────────────────────────────────────────────────────────────────────────────────────
// Function: cfg_parse_format / cfg_parse_bus / cfg_parse_unit
// Desc.:    Wandeln die typspezifischen CF-Keys in die numerischen Konstanten. -1 = unbekannt.
//────────────────────────────────────────────────────────────────────────────────────────────────
static int cfg_parse_format(const char *v)
{
    if (cfg_ieq(v, "rbf"))               return Q9_CF_FMT_RBF;
    if (cfg_ieq(v, "pcf"))               return Q9_CF_FMT_PCF;
    if (cfg_ieq(v, "fat"))               return Q9_CF_FMT_PCF;   /* Synonym                       */
    if (cfg_ieq(v, "auto") || v[0] == 0) return Q9_CF_FMT_AUTO;
    return -1;
}

static int cfg_parse_bus(const char *v)
{
    if (cfg_ieq(v, "onboard") || cfg_ieq(v, "cf"))     return Q9_CFG_BUS_ONBOARD;
    if (cfg_ieq(v, "rc2014")  || cfg_ieq(v, "sc145") || cfg_ieq(v, "secondary"))
        return Q9_CFG_BUS_RC2014;
    return -1;
}

static int cfg_parse_unit(const char *v)
{
    if (cfg_ieq(v, "master") || cfg_ieq(v, "0")) return 0;
    if (cfg_ieq(v, "slave")  || cfg_ieq(v, "1")) return 1;
    return -1;
}

/* 2026-08-14: fuer den "descriptor"-Key (jetzt Bool statt Pfad, s. boardcfg.h). BEWUSST NUR
   "yes"/"no" -- exakt die Werte, die devschema.c's q9_devschema_check_bool() als gueltig kennt.
   Keine zusaetzlichen Synonyme (true/false/1/0) ohne konkreten Bedarf: genau SO ein
   unbegruendetes Auseinanderdriften zwischen Parser-Akzeptanz und Schema-Beschreibung war der
   Fehler, der in dieser Session bei bus/unit/type gefunden und behoben wurde (s. devschema.c
   Kommentar bei g_cf_bus_values) -- hier von Anfang an vermieden statt spaeter wieder einzufangen. */
static int cfg_parse_bool(const char *v, int *out)
{
    if (cfg_ieq(v, "yes")) { *out = 1; return 0; }
    if (cfg_ieq(v, "no"))  { *out = 0; return 0; }
    return -1;
}

/* 2026-08-20 (Hardware-Vereinheitlichung, Pilot "cf"): reine Diagnose-Gegenprobe -- wird NUR
   aufgerufen, NACHDEM cfg_parse_bus/unit/format() einen Wert bereits als ungueltig verworfen hat
   (der eigentliche Fehlerpfad/die zurueckgegebene Meldung bleiben unveraendert, s. Aufrufstellen).
   Bestaetigt per q9_devdesc_lookup("cf") + q9_devschema_check_enum(), dass das jetzt in
   src/devices/cf/cf.c gepflegte Schema denselben Wert ebenfalls ablehnt -- ein Auseinanderlaufen
   waere ein Bug (Schema kopiert dieselben Wertelisten wie cfg_parse_bus/unit/format, s. cf.c
   g_cf_bus/unit/format_values), koennte aber unbemerkt bleiben, wenn niemand je danach sucht.
   Case-sensitiv (anders als cfg_ieq) -- deshalb bewusst NUR auf dem bereits-ungueltig-Pfad
   verwendet, nie auf dem Erfolgspfad (dort waere Grossschreibung ein falscher Alarm, s. cf.c-
   Kommentar bei g_cf_bus_values "bekannte, bewusst nicht behobene Vereinfachung"). */
static void cfg_schema_confirm_invalid_enum(const char *field, const char *val)
{
    const q9_devdesc_t *cf = q9_devdesc_lookup("cf");
    q9_devschema_t view;
    int idx;
    char schema_err[128];

    if (!cf) {
        return;
    }
    view.type = cf->type;
    view.fields = cf->extra_fields;
    view.field_count = cf->extra_field_count;
    idx = q9_devschema_find_field(&view, field);
    if (idx < 0) {
        return;
    }
    if (q9_devschema_check_enum(&view.fields[idx], val, schema_err, sizeof(schema_err)) != 0) {
        fprintf(stderr, "[boardcfg] Schema bestaetigt: %s\n", schema_err);
    }
}

static int cfg_parse_u32(const char *v, uint32_t *out)
{
    char *end;
    unsigned long n = strtoul(v, &end, 0);
    if (end == v || *cfg_trim(end) != '\0' || n > 0xFFFFFFFFul) return -1;
    *out = (uint32_t)n;
    return 0;
}

uint32_t q9_cfg_cf_effective_base(const q9_cfg_cf_t *cf)
{
    if (cf->use_slot) {
        int slot = (cf->slot >= 0) ? cf->slot : 0;    /* defensiv, s. Kopfkommentar in boardcfg.h */
        return 0xFFFF0000u + (uint32_t)slot * 256u;
    }
    return cf->base ? cf->base : (cf->bus == Q9_CFG_BUS_RC2014 ? Q9_BOARD_CF2_BASE : Q9_BOARD_CF_BASE);
}

//────────────────────────────────────────────────────────────────────────────────────────────────
// Function: q9_board_cfg_load
//────────────────────────────────────────────────────────────────────────────────────────────────
int q9_board_cfg_load(q9_board_cfg_t *cfg, const char *cfg_path, char *err, unsigned err_max)
{
    FILE *f;
    char  line[Q9_CFG_PATH_MAX + 128];
    char  dir[Q9_CFG_PATH_MAX];
    int   lineno = 0;
    enum { SEC_NONE, SEC_BOARD, SEC_CF } sec = SEC_NONE;
    q9_cfg_cf_t *cur_cf = 0;                             /* aktueller [cfN]-Abschnitt              */

    q9_board_cfg_default(cfg);
    cfg_dir_of(cfg_path, dir, sizeof(dir));

    f = fopen(cfg_path, "r");
    if (!f) {
        snprintf(err, err_max, "Config-Datei '%s' nicht lesbar", cfg_path);
        return -1;
    }

    while (fgets(line, (int)sizeof(line), f)) {
        char *s, *eq, *key, *val;
        lineno++;

        /* Kommentar (';' oder '#') abschneiden, dann trimmen. */
        for (s = line; *s; s++) {
            if (*s == ';' || *s == '#') {
                *s = '\0';
                break;
            }
        }
        s = cfg_trim(line);
        if (*s == '\0') {
            continue;
        }

        /* Abschnittskopf [name]. */
        if (*s == '[') {
            char *close = strchr(s, ']');
            char *sec_name;
            if (!close) {
                snprintf(err, err_max, "Zeile %d: Abschnitt ohne ']'", lineno);
                fclose(f);
                return -1;
            }
            *close = '\0';
            sec_name = cfg_trim(s + 1);
            if (cfg_ieq(sec_name, "board")) {
                sec = SEC_BOARD;
            } else if ((strncmp(sec_name, "cf", 2) == 0 || strncmp(sec_name, "CF", 2) == 0 ||
                        ((sec_name[0] == 'c' || sec_name[0] == 'C' ||
                        sec_name[0] == 'd' || sec_name[0] == 'D' ||
                        sec_name[0] == 'e' || sec_name[0] == 'E' ||
                        sec_name[0] == 'f' || sec_name[0] == 'F') &&
                       sec_name[1] >= '0' && sec_name[1] <= '9'))) {
                if (cfg->cf_count >= Q9_CFG_MAX_CF) {
                    snprintf(err, err_max, "Zeile %d: mehr als %d CF-Abschnitte",
                             lineno, Q9_CFG_MAX_CF);
                    fclose(f);
                    return -1;
                }
                cur_cf = &cfg->cf[cfg->cf_count++];
                /* Defaults je CF-Abschnitt: Onboard-Master, Format automatisch erkennen. 2026-08-14:
                   has_descriptor Default 1 (yes) -- CF-Geraete brauchen im Regelfall einen
                   Descriptor (Andreas: "alle andere brauchen im Regelfall einen", im Unterschied
                   zu memory-Geraeten, deren kuenftiger Default no waere). */
                cur_cf->bus    = Q9_CFG_BUS_ONBOARD;
                cur_cf->unit   = 0;
                cur_cf->format = Q9_CF_FMT_AUTO;
                cur_cf->base = 0;
                cur_cf->use_slot = 0;
                cur_cf->slot = -1;                        /* -1 = nicht gesetzt, s. boardcfg.h */
                cur_cf->start_sector = 0;
                cur_cf->length_sectors = 0;
                cur_cf->descriptor_lsn = 0;
                cur_cf->path[0] = '\0';
                cur_cf->has_descriptor = 1;
                cur_cf->descriptor_name[0] = '\0';
                sec = SEC_CF;
            } else {
                snprintf(err, err_max, "Zeile %d: unbekannter Abschnitt '[%s]'", lineno, sec_name);
                fclose(f);
                return -1;
            }
            continue;
        }

        /* key = value */
        eq = strchr(s, '=');
        if (!eq) {
            snprintf(err, err_max, "Zeile %d: '=' erwartet ('%s')", lineno, s);
            fclose(f);
            return -1;
        }
        *eq = '\0';
        key = cfg_trim(s);
        val = cfg_trim(eq + 1);

        if (sec == SEC_BOARD) {
            if (cfg_ieq(key, "name")) {
                cfg_copy(cfg->name, sizeof(cfg->name), val);
            } else if (cfg_ieq(key, "rom")) {
                cfg_resolve_rel(dir, val, cfg->rom_path, sizeof(cfg->rom_path));
            } else if (cfg_ieq(key, "net")) {
                cfg_copy(cfg->net_mode, sizeof(cfg->net_mode), val);
            } else if (cfg_ieq(key, "vmnet_ip") || cfg_ieq(key, "vmnet_guest_ip")) {
                cfg_copy(cfg->vmnet_ip, sizeof(cfg->vmnet_ip), val);
            } else if (cfg_ieq(key, "vmnet_gateway")) {
                cfg_copy(cfg->vmnet_gateway, sizeof(cfg->vmnet_gateway), val);
            } else if (cfg_ieq(key, "vmnet_netmask")) {
                cfg_copy(cfg->vmnet_netmask, sizeof(cfg->vmnet_netmask), val);
            } else if (cfg_ieq(key, "vmnet_dhcp_end")) {
                cfg_copy(cfg->vmnet_dhcp_end, sizeof(cfg->vmnet_dhcp_end), val);
            } else if (cfg_ieq(key, "net_hostfwd")) {
                cfg_copy(cfg->net_hostfwd, sizeof(cfg->net_hostfwd), val);
            } else if (cfg_ieq(key, "cpu")) {
                /* Q9FLUX_EDITOR_de.md 4.1: Whitelist statt freiem String -- Tippfehler sollen
                   beim Parsen auffallen, nicht erst als "bootet nicht" beim Aufrufer. Bewusst
                   KEIN #include von m68krt.h hier (boardcfg.c bleibt schlank/eigenstaendig, s.
                   Kopfkommentar) -- q9boardrun.c macht die eigentliche String->q9_cpu_type_t-
                   Abbildung, hier wird nur validiert + roh gespeichert. */
                static const char *cpu_names[] = {
                    "68000", "68010", "68020", "68ec020", "68030", "68ec030",
                    "68040", "68ec040", "68lc040"
                };
                unsigned i, n = sizeof(cpu_names) / sizeof(cpu_names[0]);
                int ok = 0;
                for (i = 0; i < n; i++) {
                    if (cfg_ieq(val, cpu_names[i])) { ok = 1; break; }
                }
                if (!ok) {
                    snprintf(err, err_max,
                             "Zeile %d: ungueltiger cpu-Wert '%s' (68000|68010|68020|68ec020|"
                             "68030|68ec030|68040|68ec040|68lc040)", lineno, val);
                    fclose(f);
                    return -1;
                }
                cfg_copy(cfg->cpu, sizeof(cfg->cpu), val);
            } else {
                snprintf(err, err_max, "Zeile %d: unbekannter [board]-Key '%s'", lineno, key);
                fclose(f);
                return -1;
            }
        } else if (sec == SEC_CF && cur_cf) {
            if (cfg_ieq(key, "image") || cfg_ieq(key, "file")) {
                cfg_resolve_rel(dir, val, cur_cf->path, sizeof(cur_cf->path));
            } else if (cfg_ieq(key, "descriptor")) {
                /* 2026-08-14: jetzt Bool statt Pfad (s. boardcfg.h) -- der bisherige Pfad-Wert
                   heisst jetzt descriptorName (naechster Zweig unten). */
                if (cfg_parse_bool(val, &cur_cf->has_descriptor) != 0) {
                    snprintf(err, err_max, "Zeile %d: ungueltiger descriptor-Wert '%s' (yes|no)",
                             lineno, val);
                    fclose(f);
                    return -1;
                }
            } else if (cfg_ieq(key, "descriptorName")) {
                /* .q9-seitiger Schluessel bewusst camelCase (Andreas' Vorgabe 2026-08-14, s.
                   devschema.c-Kommentar) -- der C-Struct-Feldname bleibt descriptor_name, passend
                   zum sonstigen snake_case-C-Stil dieser Codebasis (z.B. start_sector); beide
                   Namensraeume duerfen bewusst auseinanderlaufen, das eine ist Datei-Syntax, das
                   andere Implementierungsdetail. */
                cfg_resolve_rel(dir, val, cur_cf->descriptor_name, sizeof(cur_cf->descriptor_name));
            } else if (cfg_ieq(key, "type") || cfg_ieq(key, "format")) {
                int fmt = cfg_parse_format(val);
                if (fmt < 0) {
                    snprintf(err, err_max, "Zeile %d: unbekannter CF-Typ '%s' (rbf|pcf)", lineno, val);
                    cfg_schema_confirm_invalid_enum("type", val);
                    fclose(f);
                    return -1;
                }
                cur_cf->format = fmt;
            } else if (cfg_ieq(key, "bus")) {
                int bus = cfg_parse_bus(val);
                if (bus < 0) {
                    snprintf(err, err_max, "Zeile %d: unbekannter CF-Bus '%s' (onboard|rc2014)",
                             lineno, val);
                    cfg_schema_confirm_invalid_enum("bus", val);
                    fclose(f);
                    return -1;
                }
                cur_cf->bus = bus;
            } else if (cfg_ieq(key, "unit") || cfg_ieq(key, "drive")) {
                int unit = cfg_parse_unit(val);
                if (unit < 0) {
                    snprintf(err, err_max, "Zeile %d: unbekannte CF-Unit '%s' (master|slave)",
                             lineno, val);
                    cfg_schema_confirm_invalid_enum("unit", val);
                    fclose(f);
                    return -1;
                }
                cur_cf->unit = unit;
            } else if (cfg_ieq(key, "base")) {
                if (cfg_parse_u32(val, &cur_cf->base) != 0) {
                    snprintf(err, err_max, "Zeile %d: ungueltige CF-Base '%s'", lineno, val);
                    fclose(f);
                    return -1;
                }
            } else if (cfg_ieq(key, "useSlot")) {
                /* 2026-08-14 (ARBEITSPLAN 5.18-Fortsetzung): waehlt zwischen einem automatisch
                   zugeteilten I/O-Tabellenplatz und der freien "base" oben -- gewinnt bei yes
                   ueber "base", s. cf_entry_base() in q9boardrun.c und boardcfg.h-Kommentar. */
                if (cfg_parse_bool(val, &cur_cf->use_slot) != 0) {
                    snprintf(err, err_max, "Zeile %d: ungueltiger useSlot-Wert '%s' (yes|no)", lineno, val);
                    fclose(f);
                    return -1;
                }
            } else if (cfg_ieq(key, "slot")) {
                uint32_t v;
                if (cfg_parse_u32(val, &v) != 0 || v > 255u) {
                    snprintf(err, err_max, "Zeile %d: ungueltiger slot-Wert '%s' (0-255)", lineno, val);
                    fclose(f);
                    return -1;
                }
                cur_cf->slot = (int)v;
            } else if (cfg_ieq(key, "start_sector") || cfg_ieq(key, "offset_sector")) {
                if (cfg_parse_u32(val, &cur_cf->start_sector) != 0) {
                    snprintf(err, err_max, "Zeile %d: ungueltiger Startsektor '%s'", lineno, val);
                    fclose(f);
                    return -1;
                }
            } else if (cfg_ieq(key, "length_sectors") || cfg_ieq(key, "part_size")) {
                if (cfg_parse_u32(val, &cur_cf->length_sectors) != 0) {
                    snprintf(err, err_max, "Zeile %d: ungueltige Partitionslaenge '%s'", lineno, val);
                    fclose(f);
                    return -1;
                }
            } else if (cfg_ieq(key, "descriptor_lsn") || cfg_ieq(key, "lsn_offset")) {
                if (cfg_parse_u32(val, &cur_cf->descriptor_lsn) != 0) {
                    snprintf(err, err_max, "Zeile %d: ungueltiger Descriptor-LSN-Offset '%s'", lineno, val);
                    fclose(f);
                    return -1;
                }
            } else {
                snprintf(err, err_max, "Zeile %d: unbekannter CF-Key '%s'", lineno, key);
                fclose(f);
                return -1;
            }
        } else {
            snprintf(err, err_max, "Zeile %d: Key '%s' ausserhalb eines Abschnitts", lineno, key);
            fclose(f);
            return -1;
        }
    }
    fclose(f);

    /* Nachvalidierung: jeder CF-Abschnitt braucht ein Image; useSlot ohne slot abfangen;
       Bus/Unit-Kollision abfangen (nutzt jetzt q9_cfg_cf_effective_base() -- beruecksichtigt
       useSlot/slot, s. dortiger Kommentar). */
    for (int i = 0; i < cfg->cf_count; i++) {
        if (cfg->cf[i].path[0] == '\0') {
            snprintf(err, err_max, "CF-Abschnitt #%d ohne 'image ='", i + 1);
            return -1;
        }
        if (cfg->cf[i].use_slot && cfg->cf[i].slot < 0) {
            snprintf(err, err_max, "CF-Abschnitt #%d: useSlot=yes aber 'slot =' fehlt", i + 1);
            return -1;
        }
        for (int j = 0; j < i; j++) {
            uint32_t bj = q9_cfg_cf_effective_base(&cfg->cf[j]);
            uint32_t bi = q9_cfg_cf_effective_base(&cfg->cf[i]);
            if (bj == bi && cfg->cf[j].unit == cfg->cf[i].unit &&
                strcmp(cfg->cf[j].path, cfg->cf[i].path) != 0) {
                snprintf(err, err_max,
                         "CF-Abschnitte #%d und #%d belegen dieselbe Einheit (bus/unit)",
                         j + 1, i + 1);
                return -1;
            }
        }
    }
    return 0;
}

//────────────────────────────────────────────────────────────────────────────────────────────────
// Function: q9_board_cfg_save
//────────────────────────────────────────────────────────────────────────────────────────────────
int q9_board_cfg_save(const q9_board_cfg_t *cfg, const char *cfg_path, char *err, unsigned err_max)
{
    /* Defaults exakt wie q9_board_cfg_default() -- vmnet_*-Keys werden nur geschrieben, wenn sie
       davon abweichen (s. Kopfkommentar in boardcfg.h). */
    static const char *const def_vmnet_ip       = "192.168.200.2";
    static const char *const def_vmnet_gateway  = "192.168.200.1";
    static const char *const def_vmnet_netmask  = "255.255.255.0";
    static const char *const def_vmnet_dhcp_end = "192.168.200.254";
    FILE *f;
    char  dir[Q9_CFG_PATH_MAX];
    char  rel[Q9_CFG_PATH_MAX];
    int   i;

    cfg_dir_of(cfg_path, dir, sizeof(dir));

    f = fopen(cfg_path, "w");
    if (!f) {
        snprintf(err, err_max, "Config-Datei '%s' nicht schreibbar", cfg_path);
        return -1;
    }

    fprintf(f, "[board]\n");
    if (cfg->name[0])     { fprintf(f, "name = %s\n", cfg->name); }
    if (cfg->rom_path[0]) {
        cfg_relativize(dir, cfg->rom_path, rel, sizeof(rel));
        fprintf(f, "rom  = %s\n", rel);
    }
    if (cfg->net_mode[0]) { fprintf(f, "net  = %s\n", cfg->net_mode); }
    if (strcmp(cfg->vmnet_ip, def_vmnet_ip) != 0) {
        fprintf(f, "vmnet_ip       = %s\n", cfg->vmnet_ip);
    }
    if (strcmp(cfg->vmnet_gateway, def_vmnet_gateway) != 0) {
        fprintf(f, "vmnet_gateway  = %s\n", cfg->vmnet_gateway);
    }
    if (strcmp(cfg->vmnet_netmask, def_vmnet_netmask) != 0) {
        fprintf(f, "vmnet_netmask  = %s\n", cfg->vmnet_netmask);
    }
    if (strcmp(cfg->vmnet_dhcp_end, def_vmnet_dhcp_end) != 0) {
        fprintf(f, "vmnet_dhcp_end = %s\n", cfg->vmnet_dhcp_end);
    }
    if (cfg->net_hostfwd[0]) { fprintf(f, "net_hostfwd = %s\n", cfg->net_hostfwd); }
    if (cfg->cpu[0])         { fprintf(f, "cpu  = %s\n", cfg->cpu); }

    for (i = 0; i < cfg->cf_count; i++) {
        const q9_cfg_cf_t *cf = &cfg->cf[i];
        fprintf(f, "\n[cf%d]\n", i);
        fprintf(f, "type = %s\n", cf->format == Q9_CF_FMT_RBF ? "rbf" :
                                   cf->format == Q9_CF_FMT_PCF ? "pcf" : "auto");
        fprintf(f, "bus  = %s\n", cf->bus == Q9_CFG_BUS_RC2014 ? "rc2014" : "onboard");
        fprintf(f, "unit = %s\n", cf->unit ? "slave" : "master");
        cfg_relativize(dir, cf->path, rel, sizeof(rel));
        fprintf(f, "image = %s\n", rel);
        if (!cf->has_descriptor) { fprintf(f, "descriptor = no\n"); }
        if (cf->descriptor_name[0]) {
            cfg_relativize(dir, cf->descriptor_name, rel, sizeof(rel));
            fprintf(f, "descriptorName = %s\n", rel);
        }
        if (cf->use_slot) {
            fprintf(f, "useSlot = yes\n");
            fprintf(f, "slot = %d\n", cf->slot);
        } else if (cf->base) {
            fprintf(f, "base = 0x%08X\n", cf->base);
        }
        if (cf->start_sector)   { fprintf(f, "start_sector = %u\n", cf->start_sector); }
        if (cf->length_sectors) { fprintf(f, "length_sectors = %u\n", cf->length_sectors); }
        if (cf->descriptor_lsn) { fprintf(f, "descriptor_lsn = %u\n", cf->descriptor_lsn); }
    }

    if (fclose(f) != 0) {
        snprintf(err, err_max, "Config-Datei '%s': Fehler beim Schreiben", cfg_path);
        return -1;
    }
    return 0;
}

//────────────────────────────────────────────────────────────────────────────────────────────────
// EOF boardcfg.c                                                                          Ver. 1.50
//────────────────────────────────────────────────────────────────────────────────────────────────
