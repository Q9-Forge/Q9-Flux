//════════════════════════════════════════════════════════════════════════════════════════════════
// File:   filelist_selftest.c                                                             Ver. 1.00
// Owner:  Claudia
// Desc.:  Automatischer Nachweis fuer q9_filelist.h/.c -- legt ein echtes Scratch-Verzeichnis mit
//         Testdateien an (verschiedene Endungen/Groessen), scannt es, prueft Filterung, alphabet-
//         ische Sortierung, versteckte Dateien werden uebersprungen, Groessen-Kurzform, und dass
//         ein nicht existierendes Verzeichnis sauber -1 liefert statt abzustuerzen.
//
// Call:   build/filelist_selftest
//════════════════════════════════════════════════════════════════════════════════════════════════
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>

#include "../src/q9_filelist.h"

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

static void check_true(const char *label, int cond)
{
    if (cond) {
        printf("    OK   %s\n", label);
    } else {
        printf("    FAIL %s\n", label);
        g_fails++;
    }
}

static int find_entry(const q9_filelist_t *fl, const char *name)
{
    int i;
    for (i = 0; i < fl->count; i++) {
        if (strcmp(fl->entry[i].name, name) == 0) { return i; }
    }
    return -1;
}

static void write_file(const char *path, int byte_count)
{
    FILE *f = fopen(path, "wb");
    int i;
    if (!f) { return; }
    for (i = 0; i < byte_count; i++) { fputc('x', f); }
    fclose(f);
}

int main(void)
{
    const char *dir = "build_filelist_scratch";
    char path[256];
    q9_filelist_t fl;
    int n, idx;

    /* Scratch-Verzeichnis frisch anlegen (falls von einem vorherigen Lauf noch Reste da sind,
       stoert das nicht -- die Testdateien werden hier deterministisch neu geschrieben; mkdir()
       liefert bei bereits existierendem Verzeichnis einfach einen (ignorierten) Fehler zurueck). */
    mkdir(dir, 0755);

    snprintf(path, sizeof(path), "%s/alpha.q9", dir);
    write_file(path, 100);                                   /* < 1K -> "100B" */
    snprintf(path, sizeof(path), "%s/beta.q9", dir);
    write_file(path, 2048);                                  /* 2K */
    snprintf(path, sizeof(path), "%s/gamma.img", dir);
    write_file(path, 3 * 1024 * 1024);                        /* 3M */
    snprintf(path, sizeof(path), "%s/.hidden.q9", dir);
    write_file(path, 10);                                     /* versteckt -- darf NICHT auftauchen */

    printf("=== q9_filelist_scan: alle Dateien (kein Filter) ===\n");
    n = q9_filelist_scan(dir, NULL, &fl);
    check_int("3 sichtbare Dateien gefunden (nicht 4 -- .hidden.q9 uebersprungen)", n, 3);
    check_int("fl.count stimmt mit Rueckgabe ueberein", fl.count, n);

    printf("=== q9_filelist_scan: alphabetisch sortiert ===\n");
    check_true("alpha.q9 vor beta.q9", find_entry(&fl, "alpha.q9") < find_entry(&fl, "beta.q9"));
    check_true("beta.q9 vor gamma.img", find_entry(&fl, "beta.q9") < find_entry(&fl, "gamma.img"));

    printf("=== q9_filelist_scan: Groessen-Kurzform ===\n");
    idx = find_entry(&fl, "alpha.q9");
    check_true("alpha.q9 (100 Byte) -> '100B'", idx >= 0 && strcmp(fl.entry[idx].size, "100B") == 0);
    idx = find_entry(&fl, "beta.q9");
    check_true("beta.q9 (2048 Byte) -> '2K'", idx >= 0 && strcmp(fl.entry[idx].size, "2K") == 0);
    idx = find_entry(&fl, "gamma.img");
    check_true("gamma.img (3 MB) -> '3.0M'", idx >= 0 && strcmp(fl.entry[idx].size, "3.0M") == 0);

    printf("=== q9_filelist_scan: Datum ist gesetzt (nicht leer/'-') ===\n");
    idx = find_entry(&fl, "alpha.q9");
    check_true("Datumsfeld nicht leer", idx >= 0 && fl.entry[idx].date[0] != '\0'
                                          && strcmp(fl.entry[idx].date, "-") != 0);

    printf("=== q9_filelist_scan: Extension-Filter (case-insensitiv, mit/ohne Punkt) ===\n");
    n = q9_filelist_scan(dir, ".q9", &fl);
    check_int("Filter '.q9' -> 2 Dateien (alpha, beta)", n, 2);
    check_true("gamma.img NICHT dabei", find_entry(&fl, "gamma.img") < 0);

    n = q9_filelist_scan(dir, "Q9", &fl);                     /* ohne Punkt, Grossschreibung */
    check_int("Filter 'Q9' (ohne Punkt, Grossschreibung) -> ebenfalls 2 Dateien", n, 2);

    n = q9_filelist_scan(dir, "img", &fl);
    check_int("Filter 'img' -> 1 Datei (gamma)", n, 1);
    check_true("gamma.img gefunden", find_entry(&fl, "gamma.img") >= 0);

    n = q9_filelist_scan(dir, "xyz", &fl);
    check_int("Filter 'xyz' (kein Treffer) -> 0 Dateien", n, 0);

    n = q9_filelist_scan(dir, "*.*", &fl);
    check_int("Filter '*.*' -> alle 3 Dateien", n, 3);

    printf("=== q9_filelist_scan: nicht existierendes Verzeichnis -> -1, kein Absturz ===\n");
    n = q9_filelist_scan("build_filelist_scratch_gibtsnicht", NULL, &fl);
    check_int("Rueckgabe -1", n, -1);
    check_int("fl.count auf 0 zurueckgesetzt (kein undefinierter Zustand)", fl.count, 0);

    printf("=== q9_filelist_scan: NULL-Verzeichnis/NULL-Zeiger, kein Absturz ===\n");
    n = q9_filelist_scan(NULL, NULL, &fl);
    check_int("dir==NULL -> -1", n, -1);
    n = q9_filelist_scan(dir, NULL, NULL);
    check_int("out==NULL -> -1", n, -1);

    /* Aufraeumen */
    remove("build_filelist_scratch/alpha.q9");
    remove("build_filelist_scratch/beta.q9");
    remove("build_filelist_scratch/gamma.img");
    remove("build_filelist_scratch/.hidden.q9");
    rmdir("build_filelist_scratch");

    printf("\n=== Zusammenfassung ===\n");
    printf("  Gesamt: %d Checks fehlgeschlagen\n", g_fails);
    return g_fails ? 1 : 0;
}

//────────────────────────────────────────────────────────────────────────────────────────────────
// EOF filelist_selftest.c                                                                 Ver. 1.00
//────────────────────────────────────────────────────────────────────────────────────────────────
