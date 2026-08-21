//════════════════════════════════════════════════════════════════════════════════════════════════
// File:   08_test_devschema.c                                                             Ver. 1.60
// Owner:  Claudia
// Desc.:  6.7-Pilot: Rauchtest fuer devschema.h/.c -- kein Board, keine CPU, reine Datenstruktur-
//         Pruefung (Registry-Lookup, Int-Grenzen, Enum-Mitgliedschaft, Feld-Suche).
//
//         2026-08-20: die "cf"-Testfaelle sind nach test/12_test_devdesc.c umgezogen (Hardware-
//         Vereinheitlichung, Pilot "cf" -- s. dort). Diese Datei deckt nur noch die hier
//         verbliebenen Schemata "memory"/"board" ab.
//
// Call:   build/<platform>/test_devschema
//════════════════════════════════════════════════════════════════════════════════════════════════
#include <stdio.h>
#include <string.h>
#include "../src/kernel/devschema.h"

static int g_fails = 0;

static void check(const char *label, int got_ok, int want_ok)
{
    if ((got_ok == 0) == want_ok) {
        printf("    OK   %s\n", label);
    } else {
        printf("    FAIL %s (erwartet %s, bekommen %s)\n", label,
               want_ok ? "gueltig" : "ungueltig", got_ok == 0 ? "gueltig" : "ungueltig");
        g_fails++;
    }
}

int main(void)
{
    char err[128];
    int idx;

    /* 2026-08-20: das "cf"-Schema (Registry-Lookup, Feld-Suche, Int-Grenzen, Enum-Mitgliedschaft,
       Ground-Truth, descriptor-Konflikt, useSlot/slot) ist nach test/12_test_devdesc.c umgezogen --
       "cf" selbst lebt jetzt in src/devices/cf/cf.c (q9_devdesc_cf), nicht mehr in devschema.c. */
    printf("=== devschema: Registry ===\n");
    check("unbekannter Typ liefert NULL", q9_devschema_lookup("gibtsnicht") == NULL ? 0 : -1, 1);

    printf("=== devschema: Schema 'memory' (RAM/ROM/NVRAM) ===\n");
    {
        const q9_devschema_t *mem = q9_devschema_lookup("memory");
        if (!mem) {
            printf("    FAIL Schema 'memory' nicht gefunden\n");
            g_fails++;
        } else {
            printf("    OK   Schema 'memory' gefunden (%d Felder)\n", mem->field_count);

            idx = q9_devschema_find_field(mem, "writable");
            check("writable ist Q9_FIELD_BOOL", mem->fields[idx].kind == Q9_FIELD_BOOL ? 0 : -1, 1);
            check("'yes' ist gueltig", q9_devschema_check_bool(&mem->fields[idx], "yes", err, sizeof(err)), 1);
            check("'no' ist gueltig", q9_devschema_check_bool(&mem->fields[idx], "no", err, sizeof(err)), 1);
            check("'Yes' (Grossschreibung) ist ungueltig",
                  q9_devschema_check_bool(&mem->fields[idx], "Yes", err, sizeof(err)), 0);
            check("'1' ist ungueltig (kein yes/no)",
                  q9_devschema_check_bool(&mem->fields[idx], "1", err, sizeof(err)), 0);

            idx = q9_devschema_find_field(mem, "color_id");
            check("color_id: 15 ist gueltig", q9_devschema_check_int(&mem->fields[idx], 15, err, sizeof(err)), 1);
            check("color_id: 16 ist ungueltig", q9_devschema_check_int(&mem->fields[idx], 16, err, sizeof(err)), 0);

            idx = q9_devschema_find_field(mem, "start_address");
            check("start_address ist Pflichtfeld", mem->fields[idx].required ? 0 : -1, 1);

            idx = q9_devschema_find_field(mem, "descriptor");
            check("memory.descriptor ist Q9_FIELD_BOOL (dieselbe Bedeutung wie bei cf)",
                  mem->fields[idx].kind == Q9_FIELD_BOOL ? 0 : -1, 1);
            {
                int name_idx = q9_devschema_find_field(mem, "descriptorName");
                check("memory.descriptorName gefunden (\"fuer alle\" -- Andreas' Vorgabe)",
                      name_idx >= 0 ? 0 : -1, 1);
                check("memory.descriptorName nur relevant wenn descriptor='yes'",
                      q9_devschema_field_relevant(&mem->fields[name_idx], "yes") ? 0 : -1, 1);
            }
        }
    }

    printf("=== devschema: Schema 'board' (cpu, Q9FLUX_EDITOR_de.md 4.1) ===\n");
    {
        const q9_devschema_t *brd = q9_devschema_lookup("board");
        if (!brd) {
            printf("    FAIL Schema 'board' nicht gefunden\n");
            g_fails++;
        } else {
            printf("    OK   Schema 'board' gefunden (%d Felder)\n", brd->field_count);

            idx = q9_devschema_find_field(brd, "cpu");
            check("cpu gefunden", idx >= 0 ? 0 : -1, 1);
            check("cpu ist Q9_FIELD_ENUM", brd->fields[idx].kind == Q9_FIELD_ENUM ? 0 : -1, 1);
            check("'68030' ist gueltig", q9_devschema_check_enum(&brd->fields[idx], "68030", err, sizeof(err)), 1);
            check("'68000' ist gueltig", q9_devschema_check_enum(&brd->fields[idx], "68000", err, sizeof(err)), 1);
            check("'68lc040' ist gueltig", q9_devschema_check_enum(&brd->fields[idx], "68lc040", err, sizeof(err)), 1);
            check("'68030' (Grossschreibung waere 'ungueltig', hier klein) bleibt gueltig",
                  q9_devschema_check_enum(&brd->fields[idx], "68030", err, sizeof(err)), 1);
            check("'68030X' ist ungueltig", q9_devschema_check_enum(&brd->fields[idx], "68030X", err, sizeof(err)), 0);
            check("'SCC68070' ist ungueltig (andere CPU-Familie, bewusst nicht aufgenommen)",
                  q9_devschema_check_enum(&brd->fields[idx], "SCC68070", err, sizeof(err)), 0);
        }
    }

    printf("\n=== Zusammenfassung ===\n");
    printf("  Gesamt: %d Checks fehlgeschlagen\n", g_fails);
    return g_fails ? 1 : 0;
}

//────────────────────────────────────────────────────────────────────────────────────────────────
// EOF 08_test_devschema.c                                                                 Ver. 1.60
//────────────────────────────────────────────────────────────────────────────────────────────────
