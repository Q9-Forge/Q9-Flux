//════════════════════════════════════════════════════════════════════════════════════════════════
// File:   08_test_devschema.c                                                             Ver. 1.50
// Owner:  Claudia
// Desc.:  6.7-Pilot: Rauchtest fuer devschema.h/.c -- kein Board, keine CPU, reine Datenstruktur-
//         Pruefung (Registry-Lookup, Int-Grenzen, Enum-Mitgliedschaft, Feld-Suche).
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
    const q9_devschema_t *cf;
    char err[128];
    int idx;

    printf("=== devschema: Registry ===\n");
    cf = q9_devschema_lookup("cf");
    if (!cf) {
        printf("    FAIL Schema 'cf' nicht gefunden\n");
        return 1;
    }
    printf("    OK   Schema 'cf' gefunden (%d Felder)\n", cf->field_count);
    check("unbekannter Typ liefert NULL", q9_devschema_lookup("gibtsnicht") == NULL ? 0 : -1, 1);

    printf("=== devschema: Feld-Suche ===\n");
    idx = q9_devschema_find_field(cf, "start_sector");
    check("start_sector gefunden", idx >= 0 ? 0 : -1, 1);
    idx = q9_devschema_find_field(cf, "unbekanntesfeld");
    check("unbekanntes Feld liefert -1", idx == -1 ? 0 : -1, 1);

    printf("=== devschema: Int-Grenzen (start_sector, 0..0xFFFFFFFF) ===\n");
    idx = q9_devschema_find_field(cf, "start_sector");
    check("0 ist gueltig", q9_devschema_check_int(&cf->fields[idx], 0, err, sizeof(err)), 1);
    check("1000 ist gueltig", q9_devschema_check_int(&cf->fields[idx], 1000, err, sizeof(err)), 1);
    check("-1 ist ungueltig", q9_devschema_check_int(&cf->fields[idx], -1, err, sizeof(err)), 0);

    printf("=== devschema: Enum-Mitgliedschaft (type: auto|rbf|pcf|fat) ===\n");
    idx = q9_devschema_find_field(cf, "type");
    check("'rbf' ist gueltig", q9_devschema_check_enum(&cf->fields[idx], "rbf", err, sizeof(err)), 1);
    check("'RBF' (Grossschreibung) ist ungueltig",
          q9_devschema_check_enum(&cf->fields[idx], "RBF", err, sizeof(err)), 0);
    check("'xml' ist ungueltig", q9_devschema_check_enum(&cf->fields[idx], "xml", err, sizeof(err)), 0);
    if (q9_devschema_check_enum(&cf->fields[idx], "xml", err, sizeof(err)) != 0) {
        printf("    (Meldung: %s)\n", err);
    }

    printf("=== devschema: Pflichtfeld-Kennzeichnung ===\n");
    idx = q9_devschema_find_field(cf, "image");
    check("image ist Pflichtfeld", cf->fields[idx].required ? 0 : -1, 1);
    idx = q9_devschema_find_field(cf, "base");
    check("base ist optional", cf->fields[idx].required ? -1 : 0, 1);

    printf("=== devschema: Ground-Truth gegen ALLE echten .q9-Dateien im Repo (2026-08-14) ===\n");
    printf("    (per grep ermittelte tatsaechlich verwendete [cfN]-Schluessel/Werte-Paare --\n");
    printf("     faengt genau die Art Schema/Parser-Drift, die diese Korrektur ausgeloest hat)\n");
    {
        /* bus: alle real genutzten Werte (onboard, secondary -- rc2014/cf/sc145 sind Synonyme,
           die kein reales File nutzt, aber der Parser akzeptiert -- s. devschema.c-Kommentar). */
        static const char *const real_bus[]  = { "onboard", "secondary", NULL };
        static const char *const real_unit[] = { "master", NULL };            /* kein File nutzt slave/0/1 */
        static const char *const real_type[] = { "rbf", "pcf", NULL };
        int i;

        idx = q9_devschema_find_field(cf, "bus");
        for (i = 0; real_bus[i]; i++) {
            char label[64];
            snprintf(label, sizeof(label), "bus='%s' (real genutzt) ist gueltig", real_bus[i]);
            check(label, q9_devschema_check_enum(&cf->fields[idx], real_bus[i], err, sizeof(err)), 1);
        }
        idx = q9_devschema_find_field(cf, "unit");
        for (i = 0; real_unit[i]; i++) {
            char label[64];
            snprintf(label, sizeof(label), "unit='%s' (real genutzt) ist gueltig", real_unit[i]);
            check(label, q9_devschema_check_enum(&cf->fields[idx], real_unit[i], err, sizeof(err)), 1);
        }
        idx = q9_devschema_find_field(cf, "type");
        for (i = 0; real_type[i]; i++) {
            char label[64];
            snprintf(label, sizeof(label), "type='%s' (real genutzt) ist gueltig", real_type[i]);
            check(label, q9_devschema_check_enum(&cf->fields[idx], real_type[i], err, sizeof(err)), 1);
        }
    }

    printf("=== devschema: descriptor-Konflikt geloest (2026-08-14, projektweit Bool) ===\n");
    {
        int desc_idx, name_idx;
        desc_idx = q9_devschema_find_field(cf, "descriptor");
        check("cf.descriptor ist Q9_FIELD_BOOL", cf->fields[desc_idx].kind == Q9_FIELD_BOOL ? 0 : -1, 1);
        check("cf.descriptor: 'yes' ist gueltig",
              q9_devschema_check_bool(&cf->fields[desc_idx], "yes", err, sizeof(err)), 1);

        name_idx = q9_devschema_find_field(cf, "descriptorName");
        check("cf.descriptorName gefunden", name_idx >= 0 ? 0 : -1, 1);
        check("cf.descriptorName ist Q9_FIELD_STR", cf->fields[name_idx].kind == Q9_FIELD_STR ? 0 : -1, 1);

        printf("=== devschema: q9_devschema_field_relevant (depends_on) ===\n");
        check("descriptorName relevant wenn descriptor='yes'",
              q9_devschema_field_relevant(&cf->fields[name_idx], "yes") ? 0 : -1, 1);
        check("descriptorName NICHT relevant wenn descriptor='no'",
              q9_devschema_field_relevant(&cf->fields[name_idx], "no") ? -1 : 0, 1);
        check("descriptorName NICHT relevant wenn aktueller Wert unbekannt (NULL)",
              q9_devschema_field_relevant(&cf->fields[name_idx], NULL) ? -1 : 0, 1);
        check("image (kein depends_on) ist IMMER relevant, unabhaengig vom uebergebenen Wert",
              q9_devschema_field_relevant(&cf->fields[q9_devschema_find_field(cf, "image")], NULL) ? 0 : -1, 1);
    }

    printf("=== devschema: cf.useSlot/slot (I/O-Tabellenplatz-Wahl, 5.18-Fortsetzung) ===\n");
    {
        int use_idx, slot_idx;
        use_idx = q9_devschema_find_field(cf, "useSlot");
        check("cf.useSlot gefunden", use_idx >= 0 ? 0 : -1, 1);
        check("cf.useSlot ist Q9_FIELD_BOOL", cf->fields[use_idx].kind == Q9_FIELD_BOOL ? 0 : -1, 1);

        slot_idx = q9_devschema_find_field(cf, "slot");
        check("cf.slot gefunden", slot_idx >= 0 ? 0 : -1, 1);
        check("cf.slot: 0 ist gueltig", q9_devschema_check_int(&cf->fields[slot_idx], 0, err, sizeof(err)), 1);
        check("cf.slot: 255 ist gueltig", q9_devschema_check_int(&cf->fields[slot_idx], 255, err, sizeof(err)), 1);
        check("cf.slot: 256 ist ungueltig (nur 0-255)",
              q9_devschema_check_int(&cf->fields[slot_idx], 256, err, sizeof(err)), 0);
        check("cf.slot relevant wenn useSlot='yes'",
              q9_devschema_field_relevant(&cf->fields[slot_idx], "yes") ? 0 : -1, 1);
        check("cf.slot NICHT relevant wenn useSlot='no'",
              q9_devschema_field_relevant(&cf->fields[slot_idx], "no") ? -1 : 0, 1);
    }

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
// EOF 08_test_devschema.c                                                                 Ver. 1.50
//────────────────────────────────────────────────────────────────────────────────────────────────
