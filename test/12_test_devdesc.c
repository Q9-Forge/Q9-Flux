//════════════════════════════════════════════════════════════════════════════════════════════════
// File:   12_test_devdesc.c                                                              Ver. 1.00
// Owner:  Claudia
// Desc.:  Hardware-Vereinheitlichung (2026-08-20), Pilot "cf": Rauchtest fuer devdesc.h/.c --
//         Registry-Lookup, gemeinsame Basisfelder (q9_devschema_common_fields), und die
//         typspezifischen "cf"-Felder (q9_devdesc_cf.extra_fields, s. src/devices/cf/cf.c).
//         Groesstenteils die "cf"-Testfaelle aus test/08_test_devschema.c (vor 2026-08-20 dort),
//         hierher umgezogen und auf q9_devdesc_lookup()/extra_fields umgestellt -- inhaltlich
//         dieselben Pruefungen, nur descriptor/descriptorName jetzt gegen die gemeinsame Tabelle
//         statt gegen ein wiederholtes Typfeld.
//
// Call:   build/<platform>/test_devdesc
//════════════════════════════════════════════════════════════════════════════════════════════════
#include <stdio.h>
#include <string.h>
#include "../src/kernel/devdesc.h"

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
    const q9_devdesc_t *cf;
    q9_devschema_t      cf_view;                       /* Sicht auf extra_fields[], damit die
                                                            bestehenden q9_devschema_*-Helfer
                                                            unveraendert wiederverwendet werden
                                                            koennen (kein neuer Feld-Suche-Pfad
                                                            noetig, s. Kopfkommentar) */
    char err[128];
    int idx;

    printf("=== devdesc: Registry ===\n");
    cf = q9_devdesc_lookup("cf");
    if (!cf) {
        printf("    FAIL Typ 'cf' nicht gefunden\n");
        return 1;
    }
    printf("    OK   Typ 'cf' gefunden (%d Zusatzfelder)\n", cf->extra_field_count);
    check("unbekannter Typ liefert NULL", q9_devdesc_lookup("gibtsnicht") == NULL ? 0 : -1, 1);
    check("cf.vt gesetzt", cf->vt != NULL ? 0 : -1, 1);
    check("cf.use_table_default == 1 (liegt im Fast-Table-Cluster)", cf->use_table_default == 1 ? 0 : -1, 1);

    cf_view.type = cf->type;
    cf_view.fields = cf->extra_fields;
    cf_view.field_count = cf->extra_field_count;

    printf("=== devdesc: Feld-Suche (extra_fields) ===\n");
    idx = q9_devschema_find_field(&cf_view, "start_sector");
    check("start_sector gefunden", idx >= 0 ? 0 : -1, 1);
    idx = q9_devschema_find_field(&cf_view, "unbekanntesfeld");
    check("unbekanntes Feld liefert -1", idx == -1 ? 0 : -1, 1);
    check("descriptor NICHT in extra_fields (jetzt gemeinsam, s.u.)",
          q9_devschema_find_field(&cf_view, "descriptor") == -1 ? 0 : -1, 1);

    printf("=== devdesc: Int-Grenzen (start_sector, 0..0xFFFFFFFF) ===\n");
    idx = q9_devschema_find_field(&cf_view, "start_sector");
    check("0 ist gueltig", q9_devschema_check_int(&cf_view.fields[idx], 0, err, sizeof(err)), 1);
    check("1000 ist gueltig", q9_devschema_check_int(&cf_view.fields[idx], 1000, err, sizeof(err)), 1);
    check("-1 ist ungueltig", q9_devschema_check_int(&cf_view.fields[idx], -1, err, sizeof(err)), 0);

    printf("=== devdesc: Enum-Mitgliedschaft (type: auto|rbf|pcf|fat) ===\n");
    idx = q9_devschema_find_field(&cf_view, "type");
    check("'rbf' ist gueltig", q9_devschema_check_enum(&cf_view.fields[idx], "rbf", err, sizeof(err)), 1);
    check("'RBF' (Grossschreibung) ist ungueltig",
          q9_devschema_check_enum(&cf_view.fields[idx], "RBF", err, sizeof(err)), 0);
    check("'xml' ist ungueltig", q9_devschema_check_enum(&cf_view.fields[idx], "xml", err, sizeof(err)), 0);

    printf("=== devdesc: Pflichtfeld-Kennzeichnung ===\n");
    idx = q9_devschema_find_field(&cf_view, "image");
    check("image ist Pflichtfeld", cf_view.fields[idx].required ? 0 : -1, 1);
    idx = q9_devschema_find_field(&cf_view, "base");
    check("base ist optional", cf_view.fields[idx].required ? -1 : 0, 1);

    printf("=== devdesc: Ground-Truth gegen ALLE echten .q9-Dateien im Repo (Stand 2026-08-14) ===\n");
    {
        static const char *const real_bus[]  = { "onboard", "secondary", NULL };
        static const char *const real_unit[] = { "master", NULL };
        static const char *const real_type[] = { "rbf", "pcf", NULL };
        int i;

        idx = q9_devschema_find_field(&cf_view, "bus");
        for (i = 0; real_bus[i]; i++) {
            char label[64];
            snprintf(label, sizeof(label), "bus='%s' (real genutzt) ist gueltig", real_bus[i]);
            check(label, q9_devschema_check_enum(&cf_view.fields[idx], real_bus[i], err, sizeof(err)), 1);
        }
        idx = q9_devschema_find_field(&cf_view, "unit");
        for (i = 0; real_unit[i]; i++) {
            char label[64];
            snprintf(label, sizeof(label), "unit='%s' (real genutzt) ist gueltig", real_unit[i]);
            check(label, q9_devschema_check_enum(&cf_view.fields[idx], real_unit[i], err, sizeof(err)), 1);
        }
        idx = q9_devschema_find_field(&cf_view, "type");
        for (i = 0; real_type[i]; i++) {
            char label[64];
            snprintf(label, sizeof(label), "type='%s' (real genutzt) ist gueltig", real_type[i]);
            check(label, q9_devschema_check_enum(&cf_view.fields[idx], real_type[i], err, sizeof(err)), 1);
        }
    }

    printf("=== devdesc: q9_devschema_common_fields (descriptor/descriptorName) ===\n");
    {
        const q9_field_schema_t *desc_f = &q9_devschema_common_fields[0];
        const q9_field_schema_t *name_f = &q9_devschema_common_fields[1];

        check("[0] heisst 'descriptor'", strcmp(desc_f->name, "descriptor") == 0 ? 0 : -1, 1);
        check("descriptor ist Q9_FIELD_BOOL", desc_f->kind == Q9_FIELD_BOOL ? 0 : -1, 1);
        check("descriptor: 'yes' ist gueltig", q9_devschema_check_bool(desc_f, "yes", err, sizeof(err)), 1);

        check("[1] heisst 'descriptorName'", strcmp(name_f->name, "descriptorName") == 0 ? 0 : -1, 1);
        check("descriptorName ist Q9_FIELD_STR", name_f->kind == Q9_FIELD_STR ? 0 : -1, 1);

        printf("=== devdesc: q9_devschema_field_relevant (depends_on) ===\n");
        check("descriptorName relevant wenn descriptor='yes'",
              q9_devschema_field_relevant(name_f, "yes") ? 0 : -1, 1);
        check("descriptorName NICHT relevant wenn descriptor='no'",
              q9_devschema_field_relevant(name_f, "no") ? -1 : 0, 1);
        check("descriptorName NICHT relevant wenn aktueller Wert unbekannt (NULL)",
              q9_devschema_field_relevant(name_f, NULL) ? -1 : 0, 1);
        idx = q9_devschema_find_field(&cf_view, "image");
        check("image (kein depends_on) ist IMMER relevant, unabhaengig vom uebergebenen Wert",
              q9_devschema_field_relevant(&cf_view.fields[idx], NULL) ? 0 : -1, 1);
    }

    printf("=== devdesc: cf.useSlot/slot (I/O-Tabellenplatz-Wahl, 5.18-Fortsetzung) ===\n");
    {
        int use_idx, slot_idx;
        use_idx = q9_devschema_find_field(&cf_view, "useSlot");
        check("useSlot gefunden", use_idx >= 0 ? 0 : -1, 1);
        check("useSlot ist Q9_FIELD_BOOL", cf_view.fields[use_idx].kind == Q9_FIELD_BOOL ? 0 : -1, 1);

        slot_idx = q9_devschema_find_field(&cf_view, "slot");
        check("slot gefunden", slot_idx >= 0 ? 0 : -1, 1);
        check("slot: 0 ist gueltig", q9_devschema_check_int(&cf_view.fields[slot_idx], 0, err, sizeof(err)), 1);
        check("slot: 255 ist gueltig", q9_devschema_check_int(&cf_view.fields[slot_idx], 255, err, sizeof(err)), 1);
        check("slot: 256 ist ungueltig (nur 0-255)",
              q9_devschema_check_int(&cf_view.fields[slot_idx], 256, err, sizeof(err)), 0);
        check("slot relevant wenn useSlot='yes'",
              q9_devschema_field_relevant(&cf_view.fields[slot_idx], "yes") ? 0 : -1, 1);
        check("slot NICHT relevant wenn useSlot='no'",
              q9_devschema_field_relevant(&cf_view.fields[slot_idx], "no") ? -1 : 0, 1);
    }

    printf("\n=== Zusammenfassung ===\n");
    printf("  Gesamt: %d Checks fehlgeschlagen\n", g_fails);
    return g_fails ? 1 : 0;
}

//────────────────────────────────────────────────────────────────────────────────────────────────
// EOF 12_test_devdesc.c                                                                  Ver. 1.00
//────────────────────────────────────────────────────────────────────────────────────────────────
