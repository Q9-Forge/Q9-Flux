//════════════════════════════════════════════════════════════════════════════════════════════════
// File:   filedialog_selftest.c                                                          Ver. 1.10
// Owner:  Claudia
// Desc.:  Automatischer Nachweis fuer q9_filedialog.h/.c -- reine Zustandslogik (Fokus-Zyklus,
//         Filter-Zyklus, Filter-Aufklapp-Menue, Escape/Enter-Verhalten, "eingefroren nach
//         done!=0") mit einem echten Scratch-Verzeichnis (wie filelist_selftest.c). Rendering
//         selbst (q9_filedialog_render) wird NICHT geprueft (kein automatisierter Sichtvergleich,
//         gleiches Muster wie widgets_selftest/listview_selftest -- die pruefen auch nur die
//         Geometrie-/Zustandslogik, nicht das tatsaechliche Bildschirmbild).
//
// Call:   build/filedialog_selftest
//
// Edition History
//─────────┬──────┬────────────────────────────────────────────────────────────────────────┬──────
// Date    │ Ver. │ Description                                                            │ By
//─────────┼──────┼────────────────────────────────────────────────────────────────────────┬──────
// 26-08-17│ 1.00 │ Erster Wurf                                                              │ Cld
// 26-08-17│ 1.10 │ Filter-Aufklapp-Menue-Checks dazu (oeffnen per Enter/Pfeil-runter,        │ Cld
//         │      │ navigieren, uebernehmen, Escape schliesst nur das Popup)                 │
//═════════╧══════╧════════════════════════════════════════════════════════════════════════╧══════
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "../src/q9_filedialog.h"

static int g_fails = 0;

static void check_int(const char *label, int got, int want)
{
    if (got == want) { printf("    OK   %s (%d)\n", label, got); }
    else              { printf("    FAIL %s -- erwartet %d, bekommen %d\n", label, want, got); g_fails++; }
}

static void check_str(const char *label, const char *got, const char *want)
{
    if (strcmp(got, want) == 0) { printf("    OK   %s ('%s')\n", label, got); }
    else                         { printf("    FAIL %s -- erwartet '%s', bekommen '%s'\n", label, want, got); g_fails++; }
}

static void write_file(const char *path, int byte_count)
{
    FILE *f = fopen(path, "wb");
    int i;
    if (!f) { return; }
    for (i = 0; i < byte_count; i++) { fputc('x', f); }
    fclose(f);
}

static q9_key_t key(q9_key_kind_t kind)
{
    q9_key_t k;
    k.kind = kind;
    k.ch = 0;
    return k;
}

static void make_palette(q9_filedialog_palette_t *p)
{
    /* Konkrete Farbwerte sind fuer die Logik-Tests hier egal, nur damit init() nicht NULL bekommt. */
    memset(p, 0, sizeof(*p));
    p->focus_bg_r = 200; p->focus_bg_g = 160; p->focus_bg_b = 30;
}

int main(void)
{
    const char *dir = "build_filedialog_scratch";
    char path[256];
    static const char *const filters[] = { "*.*", ".q9", ".img" };
    q9_filedialog_palette_t pal;
    q9_filedialog_t dlg;
    char name[Q9_FILELIST_NAME_MAX];
    int r;

    mkdir(dir, 0755);
    snprintf(path, sizeof(path), "%s/alpha.q9", dir);
    write_file(path, 10);
    snprintf(path, sizeof(path), "%s/beta.q9", dir);
    write_file(path, 20);
    snprintf(path, sizeof(path), "%s/gamma.img", dir);
    write_file(path, 30);

    make_palette(&pal);

    printf("=== q9_filedialog_init: Grundzustand ===\n");
    r = q9_filedialog_init(&dlg, 2, 2, 16, 50, "Konfigurationsauswahl", dir, filters, 3, &pal);
    check_int("init() liefert 0", r, 0);
    check_int("Fokus startet auf der Liste", (int)dlg.focus, (int)Q9_FILEDIALOG_FOCUS_LIST);
    check_int("erster Filter ist '*.*' -> alle 3 Dateien", dlg.files.count, 3);
    check_int("done startet auf 0 (offen)", dlg.done, 0);

    printf("=== q9_filedialog_init: ungueltige Argumente -> -1 ===\n");
    check_int("dlg==NULL -> -1", q9_filedialog_init(NULL, 0,0,10,10, "T", dir, filters, 3, &pal), -1);
    check_int("filters==NULL -> -1", q9_filedialog_init(&dlg, 0,0,10,10, "T", dir, NULL, 3, &pal), -1);
    check_int("filter_count<1 -> -1", q9_filedialog_init(&dlg, 0,0,10,10, "T", dir, filters, 0, &pal), -1);

    printf("=== TAB: zyklisch Liste -> Filter -> OK -> Abbrechen -> Liste ===\n");
    q9_filedialog_init(&dlg, 2, 2, 16, 50, "Konfigurationsauswahl", dir, filters, 3, &pal);
    q9_filedialog_handle_key(&dlg, key(Q9_KEY_TAB));
    check_int("1x TAB -> FILTER", (int)dlg.focus, (int)Q9_FILEDIALOG_FOCUS_FILTER);
    q9_filedialog_handle_key(&dlg, key(Q9_KEY_TAB));
    check_int("2x TAB -> OK", (int)dlg.focus, (int)Q9_FILEDIALOG_FOCUS_OK);
    q9_filedialog_handle_key(&dlg, key(Q9_KEY_TAB));
    check_int("3x TAB -> CANCEL", (int)dlg.focus, (int)Q9_FILEDIALOG_FOCUS_CANCEL);
    q9_filedialog_handle_key(&dlg, key(Q9_KEY_TAB));
    check_int("4x TAB -> wieder LIST (Rundlauf)", (int)dlg.focus, (int)Q9_FILEDIALOG_FOCUS_LIST);

    printf("=== Shift-TAB: rueckwaerts, auch ueber die 0 hinaus ===\n");
    q9_filedialog_handle_key(&dlg, key(Q9_KEY_SHIFT_TAB));
    check_int("Shift-TAB von LIST -> CANCEL (rueckwaerts ueber die 0 hinaus)",
              (int)dlg.focus, (int)Q9_FILEDIALOG_FOCUS_CANCEL);

    printf("=== Filter-Zyklus (Pfeil rechts) rescanned das Verzeichnis ===\n");
    q9_filedialog_init(&dlg, 2, 2, 16, 50, "T", dir, filters, 3, &pal);
    q9_filedialog_handle_key(&dlg, key(Q9_KEY_TAB));           /* -> FILTER */
    q9_filedialog_handle_key(&dlg, key(Q9_KEY_RIGHT));
    check_str("nach 1x Pfeil-rechts: Filter '.q9'", dlg.filters[dlg.filter_index], ".q9");
    check_int("Filter '.q9' -> 2 Dateien (alpha, beta)", dlg.files.count, 2);
    q9_filedialog_handle_key(&dlg, key(Q9_KEY_RIGHT));
    check_str("nach 2x Pfeil-rechts: Filter '.img'", dlg.filters[dlg.filter_index], ".img");
    check_int("Filter '.img' -> 1 Datei (gamma)", dlg.files.count, 1);
    q9_filedialog_handle_key(&dlg, key(Q9_KEY_RIGHT));
    check_str("nach 3x Pfeil-rechts: wieder '*.*' (Rundlauf)", dlg.filters[dlg.filter_index], "*.*");
    check_int("Filter '*.*' -> wieder 3 Dateien", dlg.files.count, 3);
    q9_filedialog_handle_key(&dlg, key(Q9_KEY_LEFT));
    check_str("Pfeil-links vom Rundlauf-Anfang -> letzter Filter '.img'",
              dlg.filters[dlg.filter_index], ".img");

    printf("=== Pfeile ausserhalb ihres Fokus tun nichts ===\n");
    q9_filedialog_init(&dlg, 2, 2, 16, 50, "T", dir, filters, 3, &pal);
    q9_filedialog_handle_key(&dlg, key(Q9_KEY_RIGHT));         /* Fokus ist LIST, nicht FILTER */
    check_str("Pfeil-rechts bei Fokus LIST -> Filter unveraendert", dlg.filters[dlg.filter_index], "*.*");
    q9_filedialog_handle_key(&dlg, key(Q9_KEY_TAB));           /* -> FILTER */
    q9_filedialog_handle_key(&dlg, key(Q9_KEY_UP));
    check_int("Pfeil-hoch bei Fokus FILTER -> Auswahl unveraendert (0)", dlg.list.selected, 0);

    printf("=== Enter auf der Liste == OK ===\n");
    q9_filedialog_init(&dlg, 2, 2, 16, 50, "T", dir, filters, 3, &pal);
    r = q9_filedialog_handle_key(&dlg, key(Q9_KEY_ENTER));
    check_int("Enter auf LIST -> done == 1 (OK)", r, 1);
    check_int("dlg.done == 1", dlg.done, 1);
    r = q9_filedialog_selected_name(&dlg, name, sizeof(name));
    check_int("selected_name() liefert 0", r, 0);
    check_str("ausgewaehlte Datei ist alpha.q9 (alphabetisch erste)", name, "alpha.q9");

    printf("=== Escape == IMMER Abbruch, unabhaengig vom Fokus ===\n");
    q9_filedialog_init(&dlg, 2, 2, 16, 50, "T", dir, filters, 3, &pal);
    q9_filedialog_handle_key(&dlg, key(Q9_KEY_TAB));           /* -> FILTER, Escape soll trotzdem gehen */
    r = q9_filedialog_handle_key(&dlg, key(Q9_KEY_ESCAPE));
    check_int("Escape -> done == -1 (Abbruch)", r, -1);

    printf("=== Einmal entschieden (done!=0) bleibt eingefroren ===\n");
    q9_filedialog_init(&dlg, 2, 2, 16, 50, "T", dir, filters, 3, &pal);
    q9_filedialog_handle_key(&dlg, key(Q9_KEY_ENTER));         /* -> done = 1 (OK) */
    r = q9_filedialog_handle_key(&dlg, key(Q9_KEY_ESCAPE));    /* darf das NICHT mehr umbiegen */
    check_int("Escape NACH bereits erfolgtem OK aendert nichts mehr", r, 1);
    check_int("Fokus bleibt ebenfalls unveraendert (kein Seiteneffekt mehr)",
              (int)dlg.focus, (int)Q9_FILEDIALOG_FOCUS_LIST);

    printf("=== Filter-Aufklapp-Menue: oeffnen, navigieren, uebernehmen ===\n");
    q9_filedialog_init(&dlg, 2, 2, 16, 50, "T", dir, filters, 3, &pal);
    q9_filedialog_handle_key(&dlg, key(Q9_KEY_TAB));           /* -> FILTER */
    check_int("Popup startet geschlossen", dlg.filter_popup_open, 0);
    q9_filedialog_handle_key(&dlg, key(Q9_KEY_ENTER));
    check_int("Enter auf FILTER oeffnet das Popup (nicht mehr Pfeil-rechts-Ersatz)",
              dlg.filter_popup_open, 1);
    check_int("Popup-Auswahl startet beim aktuellen Filter (Index 0, '*.*')", dlg.filter_popup_index, 0);
    check_str("Filter selbst noch unveraendert waehrend das Popup offen ist",
              dlg.filters[dlg.filter_index], "*.*");
    q9_filedialog_handle_key(&dlg, key(Q9_KEY_DOWN));
    check_int("Pfeil runter im Popup -> Index 1", dlg.filter_popup_index, 1);
    q9_filedialog_handle_key(&dlg, key(Q9_KEY_DOWN));
    check_int("Pfeil runter im Popup -> Index 2 (.img)", dlg.filter_popup_index, 2);
    q9_filedialog_handle_key(&dlg, key(Q9_KEY_DOWN));
    check_int("Pfeil runter am Ende des Popups -> bleibt auf Index 2 (kein Rundlauf)",
              dlg.filter_popup_index, 2);
    q9_filedialog_handle_key(&dlg, key(Q9_KEY_UP));
    check_int("Pfeil hoch im Popup -> Index 1", dlg.filter_popup_index, 1);
    r = q9_filedialog_handle_key(&dlg, key(Q9_KEY_ENTER));
    check_int("Enter im Popup -> weiterhin offen (0), Popup schliesst NICHT den Dialog", r, 0);
    check_int("Popup jetzt geschlossen", dlg.filter_popup_open, 0);
    check_str("Filter uebernommen (Index 1 -> '.q9')", dlg.filters[dlg.filter_index], ".q9");
    check_int("Verzeichnis mit dem neuen Filter neu gescannt -> 2 Dateien", dlg.files.count, 2);

    printf("=== Filter-Aufklapp-Menue: Pfeil runter (statt Enter) oeffnet es ebenfalls ===\n");
    q9_filedialog_init(&dlg, 2, 2, 16, 50, "T", dir, filters, 3, &pal);
    q9_filedialog_handle_key(&dlg, key(Q9_KEY_TAB));
    q9_filedialog_handle_key(&dlg, key(Q9_KEY_DOWN));
    check_int("Pfeil runter auf FILTER oeffnet das Popup", dlg.filter_popup_open, 1);

    printf("=== Filter-Aufklapp-Menue: Escape schliesst NUR das Popup, nicht den Dialog ===\n");
    q9_filedialog_handle_key(&dlg, key(Q9_KEY_DOWN));          /* Index 1, aber noch NICHT bestaetigt */
    r = q9_filedialog_handle_key(&dlg, key(Q9_KEY_ESCAPE));
    check_int("Escape im Popup -> Dialog bleibt offen (0)", r, 0);
    check_int("Popup jetzt geschlossen", dlg.filter_popup_open, 0);
    check_str("Filter UNVERAENDERT (Auswahl verworfen, '*.*' bleibt aktiv)",
              dlg.filters[dlg.filter_index], "*.*");

    printf("=== Filter-Aufklapp-Menue: waehrend es offen ist, ignoriert alles ausser Pfeil/Enter/Escape ===\n");
    q9_filedialog_handle_key(&dlg, key(Q9_KEY_ENTER));         /* Popup wieder oeffnen (Fokus: FILTER) */
    q9_filedialog_handle_key(&dlg, key(Q9_KEY_TAB));
    check_int("TAB waehrend Popup offen -> Fokus bleibt unveraendert (FILTER)",
              (int)dlg.focus, (int)Q9_FILEDIALOG_FOCUS_FILTER);
    check_int("Popup bleibt offen", dlg.filter_popup_open, 1);
    q9_filedialog_handle_key(&dlg, key(Q9_KEY_ESCAPE));        /* aufraeumen fuer die naechsten Checks */

    printf("=== Enter auf leerer Liste bestaetigt nicht ===\n");
    {
        q9_filedialog_t dlg_empty;
        r = q9_filedialog_init(&dlg_empty, 2, 2, 16, 50, "T", dir, filters, 3, &pal);
        q9_filedialog_handle_key(&dlg_empty, key(Q9_KEY_TAB));  /* -> FILTER */
        q9_filedialog_handle_key(&dlg_empty, key(Q9_KEY_RIGHT)); /* -> '.q9' */
        q9_filedialog_handle_key(&dlg_empty, key(Q9_KEY_RIGHT)); /* -> '.img' */
        q9_filedialog_handle_key(&dlg_empty, key(Q9_KEY_RIGHT)); /* -> '*.*' */
        /* stattdessen: Filter auf etwas, das NICHTS trifft */
        {
            static const char *const filters_none[] = { "xyz" };
            q9_filedialog_init(&dlg_empty, 2, 2, 16, 50, "T", dir, filters_none, 1, &pal);
        }
        check_int("Filter 'xyz' -> 0 Dateien", dlg_empty.files.count, 0);
        r = q9_filedialog_handle_key(&dlg_empty, key(Q9_KEY_ENTER));
        check_int("Enter auf leerer Liste -> weiterhin offen (0)", r, 0);
        r = q9_filedialog_selected_name(&dlg_empty, name, sizeof(name));
        check_int("selected_name() bei leerer Liste -> -1", r, -1);
    }

    /* Aufraeumen */
    remove("build_filedialog_scratch/alpha.q9");
    remove("build_filedialog_scratch/beta.q9");
    remove("build_filedialog_scratch/gamma.img");
    rmdir("build_filedialog_scratch");

    printf("\n=== Zusammenfassung ===\n");
    printf("  Gesamt: %d Checks fehlgeschlagen\n", g_fails);
    return g_fails ? 1 : 0;
}

//────────────────────────────────────────────────────────────────────────────────────────────────
// EOF filedialog_selftest.c                                                              Ver. 1.10
//────────────────────────────────────────────────────────────────────────────────────────────────
