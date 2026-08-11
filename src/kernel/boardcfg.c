//════════════════════════════════════════════════════════════════════════════════════════════════
// File:   boardcfg.c                                                                      Ver. 1.10
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
//═════════╧══════╧════════════════════════════════════════════════════════════════════════╧══════
#include "boardcfg.h"
#include "q9board.h"                                     /* Q9_CF_FMT_*                            */
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

static int cfg_parse_u32(const char *v, uint32_t *out)
{
    char *end;
    unsigned long n = strtoul(v, &end, 0);
    if (end == v || *cfg_trim(end) != '\0' || n > 0xFFFFFFFFul) return -1;
    *out = (uint32_t)n;
    return 0;
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
                /* Defaults je CF-Abschnitt: Onboard-Master, Format automatisch erkennen. */
                cur_cf->bus    = Q9_CFG_BUS_ONBOARD;
                cur_cf->unit   = 0;
                cur_cf->format = Q9_CF_FMT_AUTO;
                cur_cf->base = 0;
                cur_cf->start_sector = 0;
                cur_cf->length_sectors = 0;
                cur_cf->descriptor_lsn = 0;
                cur_cf->path[0] = '\0';
                cur_cf->descriptor[0] = '\0';
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
            } else {
                snprintf(err, err_max, "Zeile %d: unbekannter [board]-Key '%s'", lineno, key);
                fclose(f);
                return -1;
            }
        } else if (sec == SEC_CF && cur_cf) {
            if (cfg_ieq(key, "image") || cfg_ieq(key, "file")) {
                cfg_resolve_rel(dir, val, cur_cf->path, sizeof(cur_cf->path));
            } else if (cfg_ieq(key, "descriptor")) {
                cfg_resolve_rel(dir, val, cur_cf->descriptor, sizeof(cur_cf->descriptor));
            } else if (cfg_ieq(key, "type") || cfg_ieq(key, "format")) {
                int fmt = cfg_parse_format(val);
                if (fmt < 0) {
                    snprintf(err, err_max, "Zeile %d: unbekannter CF-Typ '%s' (rbf|pcf)", lineno, val);
                    fclose(f);
                    return -1;
                }
                cur_cf->format = fmt;
            } else if (cfg_ieq(key, "bus")) {
                int bus = cfg_parse_bus(val);
                if (bus < 0) {
                    snprintf(err, err_max, "Zeile %d: unbekannter CF-Bus '%s' (onboard|rc2014)",
                             lineno, val);
                    fclose(f);
                    return -1;
                }
                cur_cf->bus = bus;
            } else if (cfg_ieq(key, "unit") || cfg_ieq(key, "drive")) {
                int unit = cfg_parse_unit(val);
                if (unit < 0) {
                    snprintf(err, err_max, "Zeile %d: unbekannte CF-Unit '%s' (master|slave)",
                             lineno, val);
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

    /* Nachvalidierung: jeder CF-Abschnitt braucht ein Image; Bus/Unit-Kollision abfangen. */
    for (int i = 0; i < cfg->cf_count; i++) {
        if (cfg->cf[i].path[0] == '\0') {
            snprintf(err, err_max, "CF-Abschnitt #%d ohne 'image ='", i + 1);
            return -1;
        }
        for (int j = 0; j < i; j++) {
            uint32_t bj = cfg->cf[j].base ? cfg->cf[j].base :
                          (cfg->cf[j].bus == Q9_CFG_BUS_RC2014 ? 0xFFFFC010u : 0xFFFFE000u);
            uint32_t bi = cfg->cf[i].base ? cfg->cf[i].base :
                          (cfg->cf[i].bus == Q9_CFG_BUS_RC2014 ? 0xFFFFC010u : 0xFFFFE000u);
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
// EOF boardcfg.c                                                                          Ver. 1.10
//────────────────────────────────────────────────────────────────────────────────────────────────
