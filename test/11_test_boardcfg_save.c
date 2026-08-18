//════════════════════════════════════════════════════════════════════════════════════════════════
// File:   11_test_boardcfg_save.c                                                         Ver. 1.00
// Owner:  Claudia
// Desc.:  Q9FLUX_EDITOR_de.md 4.7 (Andreas: "Speichern-Funktion"): Regressionsabsicherung fuer
//         q9_board_cfg_save() (boardcfg.h/.c) -- reine Datenstruktur-/Serialisierungs-Pruefung,
//         kein Board/keine CPU noetig. Kernpruefung: ein Load+Save+Load-Zyklus IN DERSELBEN
//         Datei/demselben Verzeichnis liefert dieselben Werte zurueck (vollstaendiger Roundtrip,
//         inkl. [cfN]-Abschnitte und relativer Pfade -- die knifflige Stelle, s.
//         cfg_relativize()-Kommentar in boardcfg.c). Ausserdem: vmnet_*-Default-Unterdrueckung,
//         Fehlerfall (nicht schreibbares Verzeichnis).
//
// Call:   build/<platform>/test_boardcfg_save
//════════════════════════════════════════════════════════════════════════════════════════════════
#include <stdio.h>
#include <string.h>

#include "../src/kernel/boardcfg.h"

static int g_fails = 0;

static void check_int(const char *label, int got, int want)
{
    if (got == want) {
        printf("    OK   %s (%d)\n", label, got);
    } else {
        printf("    FAIL %s -- erwartet %d, bekommen %d\n", label, want, got);
        g_fails++;
    }
}

static void check_str(const char *label, const char *got, const char *want)
{
    if (strcmp(got, want) == 0) {
        printf("    OK   %s ('%s')\n", label, got);
    } else {
        printf("    FAIL %s -- erwartet '%s', bekommen '%s'\n", label, want, got);
        g_fails++;
    }
}

static void check_true(const char *label, int cond)
{
    if (cond) {
        printf("    OK   %s\n", label);
    } else {
        printf("    FAIL %s\n", label);
        g_fails++;
    }
}

/* Liest die Datei komplett in buf (NUL-terminiert) -- fuer den Blick auf den tatsaechlich
   geschriebenen Text (vmnet_*-Default-Unterdrueckung etc.). */
static void slurp(const char *path, char *buf, unsigned buf_max)
{
    FILE *f = fopen(path, "r");
    unsigned n = 0;
    buf[0] = '\0';
    if (!f) { return; }
    n = (unsigned)fread(buf, 1, buf_max - 1, f);
    buf[n] = '\0';
    fclose(f);
}

int main(void)
{
    q9_board_cfg_t cfg1, cfg2;
    char err[256];
    char text[4096];
    const char *src = "build_test_boardcfg_save_src.q9";
    const char *dst = "build_test_boardcfg_save_dst.q9";

    printf("=== q9_board_cfg_save: voller Roundtrip inkl. [cfN] und relativer Pfade ===\n");
    {
        FILE *f = fopen(src, "w");
        check_true("Scratch-Quelldatei angelegt", f != 0);
        if (f) {
            fprintf(f,
                "[board]\n"
                "name = Roundtrip-Test\n"
                "rom  = roms/test.bin\n"
                "net  = vmnet\n"
                "cpu  = 68030\n"
                "\n"
                "[cf0]\n"
                "type = rbf\n"
                "bus  = onboard\n"
                "unit = master\n"
                "image = OS9SYS.hda\n"
                "\n"
                "[cf1]\n"
                "type = pcf\n"
                "bus  = rc2014\n"
                "unit = slave\n"
                "image = data.img\n"
                "useSlot = yes\n"
                "slot = 12\n");
            fclose(f);
        }

        check_int("Quelldatei laedt erfolgreich", q9_board_cfg_load(&cfg1, src, err, sizeof(err)), 0);

        /* Selbes Verzeichnis (".") wie die Quelldatei -- relative Pfade muessen relativ bleiben. */
        check_int("Save nach dst gelingt", q9_board_cfg_save(&cfg1, dst, err, sizeof(err)), 0);
        check_int("dst laedt wieder erfolgreich", q9_board_cfg_load(&cfg2, dst, err, sizeof(err)), 0);

        check_str("name bleibt erhalten",     cfg2.name,     cfg1.name);
        check_str("rom_path bleibt relativ",  cfg2.rom_path, "roms/test.bin");
        check_str("net_mode bleibt erhalten", cfg2.net_mode, cfg1.net_mode);
        check_str("cpu bleibt erhalten",      cfg2.cpu,      cfg1.cpu);
        check_int("cf_count bleibt 2",        cfg2.cf_count, 2);
        if (cfg2.cf_count == 2) {
            check_str("cf[0].path bleibt relativ", cfg2.cf[0].path, "OS9SYS.hda");
            check_int("cf[0].bus bleibt onboard",  cfg2.cf[0].bus, Q9_CFG_BUS_ONBOARD);
            check_int("cf[0].unit bleibt master",  cfg2.cf[0].unit, 0);
            check_int("cf[0].format bleibt rbf",   cfg2.cf[0].format, cfg1.cf[0].format);
            check_str("cf[1].path bleibt relativ", cfg2.cf[1].path, "data.img");
            check_int("cf[1].bus bleibt rc2014",   cfg2.cf[1].bus, Q9_CFG_BUS_RC2014);
            check_int("cf[1].unit bleibt slave",   cfg2.cf[1].unit, 1);
            check_int("cf[1].use_slot bleibt 1",   cfg2.cf[1].use_slot, 1);
            check_int("cf[1].slot bleibt 12",      cfg2.cf[1].slot, 12);
        }
    }

    printf("=== q9_board_cfg_save: vmnet_*-Keys nur bei Abweichung vom Default ===\n");
    {
        q9_board_cfg_t cfg;
        q9_board_cfg_default(&cfg);
        strcpy(cfg.name, "Defaulttest");
        /* vmnet_ip/_gateway/_netmask/_dhcp_end bleiben auf den Default-Werten (s.
           q9_board_cfg_default()) -- duerfen NICHT in der gespeicherten Datei auftauchen. */
        check_int("Save (nur Defaults) gelingt", q9_board_cfg_save(&cfg, dst, err, sizeof(err)), 0);
        slurp(dst, text, sizeof(text));
        check_true("vmnet_ip NICHT geschrieben (Default)", strstr(text, "vmnet_ip") == 0);
        check_true("vmnet_gateway NICHT geschrieben (Default)", strstr(text, "vmnet_gateway") == 0);

        strcpy(cfg.vmnet_ip, "10.0.0.5");
        check_int("Save (abweichende vmnet_ip) gelingt", q9_board_cfg_save(&cfg, dst, err, sizeof(err)), 0);
        slurp(dst, text, sizeof(text));
        check_true("vmnet_ip WIRD geschrieben (abweichend)", strstr(text, "10.0.0.5") != 0);
    }

    printf("=== q9_board_cfg_save: Fehlerfall (Verzeichnis existiert nicht) ===\n");
    {
        q9_board_cfg_t cfg;
        q9_board_cfg_default(&cfg);
        strcpy(cfg.name, "X");
        check_int("Save in nicht existierendes Verzeichnis scheitert",
                  q9_board_cfg_save(&cfg, "kein/solches/verzeichnis/x.q9", err, sizeof(err)), -1);
        printf("    (Meldung: %s)\n", err);
    }

    remove(src);
    remove(dst);

    printf("\n=== Zusammenfassung ===\n");
    printf("  Gesamt: %d Checks fehlgeschlagen\n", g_fails);
    return g_fails ? 1 : 0;
}

//────────────────────────────────────────────────────────────────────────────────────────────────
// EOF 11_test_boardcfg_save.c                                                             Ver. 1.00
//────────────────────────────────────────────────────────────────────────────────────────────────
