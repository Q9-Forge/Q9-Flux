//════════════════════════════════════════════════════════════════════════════════════════════════
// File:   procspawn_selftest.c                                                            Ver. 1.00
// Owner:  Claudia
// Desc.:  Automatischer Nachweis fuer q9_procspawn.h/.c: Exit-Code kommt korrekt durch, ein
//         nicht-existierender Pfad liefert einen Fehler statt abzustuerzen, der Aufrufer blockiert
//         tatsaechlich bis das Kind fertig ist. Bewusst OHNE Abhaengigkeit von Systembinaries
//         (kein /bin/true o.ae. -- unter Windows gaebe es das so nicht) -- das Testprogramm startet
//         sich stattdessen SELBST erneut mit einem Sonderflag, das sofort mit einem gewuenschten
//         Code beendet (etabliertes, portables Selbsttest-Muster fuer Subprozess-Code).
//
// Call:   build/procspawn_selftest                     (Selbsttest)
//         build/procspawn_selftest --exit-code N        (interner Helfer, beendet sofort mit Code N)
//════════════════════════════════════════════════════════════════════════════════════════════════
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../src/q9_procspawn.h"

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

int main(int argc, char **argv)
{
    /* Interner Helfer-Modus -- vom Selbsttest unten per q9_procspawn_run() aufgerufen. */
    if (argc > 2 && strcmp(argv[1], "--exit-code") == 0) {
        return atoi(argv[2]);
    }

    printf("=== q9_procspawn: Exit-Code eines normal beendeten Kindes kommt exakt durch ===\n");
    {
        char *const child_argv[] = { argv[0], (char *)"--exit-code", (char *)"0", NULL };
        check_int("Exit-Code 0", q9_procspawn_run(argv[0], child_argv), 0);
    }
    {
        char *const child_argv[] = { argv[0], (char *)"--exit-code", (char *)"7", NULL };
        check_int("Exit-Code 7", q9_procspawn_run(argv[0], child_argv), 7);
    }
    {
        char *const child_argv[] = { argv[0], (char *)"--exit-code", (char *)"42", NULL };
        check_int("Exit-Code 42", q9_procspawn_run(argv[0], child_argv), 42);
    }

    printf("=== q9_procspawn: nicht existierender Pfad liefert einen Fehler, kein Absturz ===\n");
    {
        char *const child_argv[] = { (char *)"/pfad/der/ganz/sicher/nicht/existiert/q9dummy", NULL };
        int code = q9_procspawn_run("/pfad/der/ganz/sicher/nicht/existiert/q9dummy", child_argv);
        /* POSIX: das Kind laeuft kurz an (fork() gelingt), execv() schlaegt fehl -> Kind beendet
           sich selbst mit 127 (Shell-Konvention) -- q9_procspawn_run() liefert also den durchaus
           GUELTIGEN Exit-Code 127 zurueck, keinen negativen Spawn-Fehler. Windows' CreateProcessA
           scheitert dagegen schon beim Start selbst -> -1. Beide Ergebnisse zeigen "hat nicht
           geklappt", nur auf unterschiedliche Art -- der Test akzeptiert beide. */
        check_true("nicht-existierender Pfad -> Fehler erkennbar (127 oder -1), kein Crash",
                   code == 127 || code < 0);
    }

    printf("\n=== Zusammenfassung ===\n");
    printf("  Gesamt: %d Checks fehlgeschlagen\n", g_fails);
    return g_fails ? 1 : 0;
}

//────────────────────────────────────────────────────────────────────────────────────────────────
// EOF procspawn_selftest.c                                                                Ver. 1.00
//────────────────────────────────────────────────────────────────────────────────────────────────
