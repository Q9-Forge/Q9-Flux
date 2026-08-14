//════════════════════════════════════════════════════════════════════════════════════════════════
// File:   10_test_useslot.c                                                               Ver. 1.00
// Owner:  Claudia
// Desc.:  5.18-Fortsetzung: Regressionsabsicherung fuer useSlot/slot (boardcfg.h/.c) -- reine
//         Datenstruktur-/Parser-Pruefung, kein Board/keine CPU noetig (q9_cfg_cf_effective_base()
//         und q9_board_cfg_load() sind beide reine boardcfg.c-Funktionen).
//
//         Ergaenzt die manuelle End-to-End-Verifikation (echter q9.exe-Lauf mit vier Szenarien:
//         gueltiger Slot bootet, Slot-Kollision mit RTC bricht sauber ab, useSlot ohne slot=
//         scheitert beim Parsen, slot ausserhalb 0-255 scheitert beim Parsen -- alle vier beim
//         Bau dieses Schritts bestaetigt, s. ARBEITSPLAN 5.18) -- der eigentliche Abbruch bei
//         echter Adressueberlappung (q9_board_validate_no_overlap, q9boardrun.c) braucht die
//         volle Geraete-Registry und wird hier bewusst NICHT nachgebaut (waere derselbe Aufwand
//         wie test/09_test_io_dispatch.c fuer eine bereits manuell verifizierte, einfache
//         O(n^2)-Bereichspruefung) -- dieser Test deckt die boardcfg.c-Seite ab: Adressberechnung
//         und Parser-Fehlerpfade.
//
// Call:   build/<platform>/test_useslot
//════════════════════════════════════════════════════════════════════════════════════════════════
#include <stdio.h>
#include <string.h>

#include "../src/kernel/boardcfg.h"

static int g_fails = 0;

static void check_u32(const char *label, uint32_t got, uint32_t want)
{
    if (got == want) {
        printf("    OK   %s (0x%08X)\n", label, got);
    } else {
        printf("    FAIL %s -- erwartet 0x%08X, bekommen 0x%08X\n", label, want, got);
        g_fails++;
    }
}

static void check_int(const char *label, int got, int want)
{
    if (got == want) {
        printf("    OK   %s (%d)\n", label, got);
    } else {
        printf("    FAIL %s -- erwartet %d, bekommen %d\n", label, want, got);
        g_fails++;
    }
}

/* Schreibt eine kleine, wegwerfbare .q9-Datei nach path und laedt sie -- fuer die Parser-
   Fehlerpfade, die nur ueber q9_board_cfg_load() (also eine echte Datei) erreichbar sind.
   Pfad bewusst relativ im Build-Verzeichnis (nicht /tmp) -- portabel, von "make clean" erfasst. */
static int load_scratch_cf(const char *path, const char *extra_cf_lines,
                            q9_board_cfg_t *cfg, char *err, unsigned err_max)
{
    FILE *f = fopen(path, "w");
    if (!f) {
        return -2;                                        /* Testinfrastruktur-Fehler, kein Config-Fehler */
    }
    fprintf(f, "[board]\nrom = dummy.bin\n\n[c0]\ntype = rbf\nbus = onboard\nunit = master\n"
               "image = dummy.img\ndescriptor = no\n%s\n", extra_cf_lines);
    fclose(f);
    return q9_board_cfg_load(cfg, path, err, err_max);
}

int main(void)
{
    q9_board_cfg_t cfg;
    char err[256];
    const char *scratch = "build_test_useslot_scratch.q9";

    printf("=== useSlot/slot: q9_cfg_cf_effective_base() Adressberechnung ===\n");
    {
        q9_cfg_cf_t cf;
        memset(&cf, 0, sizeof(cf));

        cf.use_slot = 0;
        cf.base = 0;
        cf.bus = Q9_CFG_BUS_ONBOARD;
        check_u32("use_slot=0, base=0, bus=onboard -> Onboard-Default", q9_cfg_cf_effective_base(&cf), 0xFFFFE000u);

        cf.bus = Q9_CFG_BUS_RC2014;
        check_u32("use_slot=0, base=0, bus=rc2014 -> RC2014-Default", q9_cfg_cf_effective_base(&cf), 0xFFFFC010u);

        cf.base = 0x12345600u;
        check_u32("use_slot=0, base gesetzt -> base gewinnt", q9_cfg_cf_effective_base(&cf), 0x12345600u);

        cf.use_slot = 1;
        cf.slot = 0;
        check_u32("use_slot=1, slot=0 -> $FFFF0000", q9_cfg_cf_effective_base(&cf), 0xFFFF0000u);

        cf.slot = 0x80;
        check_u32("use_slot=1, slot=0x80 -> $FFFF8000", q9_cfg_cf_effective_base(&cf), 0xFFFF8000u);

        cf.slot = 255;
        check_u32("use_slot=1, slot=255 -> $FFFFFF00", q9_cfg_cf_effective_base(&cf), 0xFFFFFF00u);

        cf.slot = -1;                                       /* Sentinel "nicht gesetzt" */
        check_u32("use_slot=1, slot=-1 (Sentinel) -> defensiv wie slot=0", q9_cfg_cf_effective_base(&cf), 0xFFFF0000u);
    }

    printf("=== useSlot/slot: Parser-Erfolgsfall ===\n");
    {
        int rc = load_scratch_cf(scratch, "useSlot = yes\nslot = 80\n", &cfg, err, sizeof(err));
        check_int("gueltiger useSlot/slot laedt erfolgreich", rc, 0);
        if (rc == 0 && cfg.cf_count > 0) {
            check_int("cfg.cf[0].use_slot == 1", cfg.cf[0].use_slot, 1);
            check_int("cfg.cf[0].slot == 80", cfg.cf[0].slot, 80);
            check_u32("effektive Basis == $FFFF5000", q9_cfg_cf_effective_base(&cfg.cf[0]), 0xFFFF5000u);
        }
    }

    printf("=== useSlot/slot: Parser-Fehlerpfade ===\n");
    {
        int rc = load_scratch_cf(scratch, "useSlot = yes\n", &cfg, err, sizeof(err));
        check_int("useSlot=yes ohne slot= scheitert", rc, -1);
        if (rc != 0) {
            printf("    (Meldung: %s)\n", err);
        }
    }
    {
        int rc = load_scratch_cf(scratch, "useSlot = yes\nslot = 300\n", &cfg, err, sizeof(err));
        check_int("slot=300 (ausserhalb 0-255) scheitert", rc, -1);
        if (rc != 0) {
            printf("    (Meldung: %s)\n", err);
        }
    }
    {
        int rc = load_scratch_cf(scratch, "useSlot = maybe\nslot = 5\n", &cfg, err, sizeof(err));
        check_int("useSlot='maybe' (kein yes/no) scheitert", rc, -1);
        if (rc != 0) {
            printf("    (Meldung: %s)\n", err);
        }
    }
    {
        /* Ohne useSlot bleibt slot= einfach unbenutzt -- kein Fehler, auch wenn beide Keys
           bunt gemischt in der Datei stehen (devschema.c depends_on beschreibt Relevanz fuers
           EDITOR-Ausgrauen, ist keine harte Parser-Pflicht, s. dortiger Kommentar). */
        int rc = load_scratch_cf(scratch, "slot = 5\n", &cfg, err, sizeof(err));
        check_int("slot= ohne useSlot ist kein Fehler (bleibt unbenutzt)", rc, 0);
        if (rc == 0 && cfg.cf_count > 0) {
            check_int("use_slot bleibt 0 (Default)", cfg.cf[0].use_slot, 0);
            check_u32("effektive Basis ignoriert slot, nutzt Onboard-Default",
                      q9_cfg_cf_effective_base(&cfg.cf[0]), 0xFFFFE000u);
        }
    }

    remove(scratch);

    printf("\n=== Zusammenfassung ===\n");
    printf("  Gesamt: %d Checks fehlgeschlagen\n", g_fails);
    return g_fails ? 1 : 0;
}

//────────────────────────────────────────────────────────────────────────────────────────────────
// EOF 10_test_useslot.c                                                                   Ver. 1.00
//────────────────────────────────────────────────────────────────────────────────────────────────
