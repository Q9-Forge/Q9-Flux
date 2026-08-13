//════════════════════════════════════════════════════════════════════════════════════════════════
// File:   ansi_selftest.c                                                                 Ver. 1.00
// Owner:  Claudia
// Desc.:  Automatischer Nachweis, dass q9_ansi.h/.c Zeilen-/Spalten-/Farbwerte MECHANISCH und
//         VERLUSTFREI in die erzeugten Escape-Bytes uebernimmt -- das war der eigentliche
//         Streitpunkt am gescheiterten Turbo-Vision-Prototypen (docs/Q9FLUX_EDITOR_de.md Abschnitt
//         0): "keine Kontrolle ueber Farbe/Lage". Kein echter Terminal noetig -- jede Funktion
//         schreibt nur in einen Puffer (s. q9_ansi.h), dieser Test liest die erzeugten Bytes exakt
//         nach (String-Vergleich UND Ruecklese-Parse per sscanf: "wenn ich Zeile 5 anfordere,
//         steht dann auch wirklich 5 in den erzeugten Bytes").
//
//         Default (ohne Argument): Selbsttest, druckt OK/FAIL, Exit-Code wie die anderen Q9-Tests.
//         --demo: schreibt DIREKT auf stdout ein Drei-Zeilen-Beispiel (verschiedene Position/Farbe)
//         -- fuer Andreas, um es in einem echten Terminal anzuschauen und Verschiebe-Anweisungen
//         ("3 Zeilen tiefer, 4 Zeichen nach links") zum Nachpruefen zu geben. NICHT Teil von
//         "make test" (braucht ein echtes Terminal, kein automatisierbarer Vergleich).
//
// Call:   build/ansi_selftest          (Selbsttest)
//         build/ansi_selftest --demo   (interaktive Sichtpruefung)
//════════════════════════════════════════════════════════════════════════════════════════════════
#include <stdio.h>
#include <string.h>

#include "../src/q9_ansi.h"

static int g_fails = 0;

static void check_str(const char *label, const char *got, const char *want)
{
    if (strcmp(got, want) == 0) {
        printf("    OK   %s\n", label);
    } else {
        printf("    FAIL %s -- erwartet %s, bekommen %s\n", label, want, got);
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

int main(int argc, char **argv)
{
    char buf[64];
    unsigned n;

    if (argc > 1 && strcmp(argv[1], "--demo") == 0) {
        /* Interaktive Sichtpruefung -- drei Zeilen, unterschiedliche Position/Farbe, exakt das
           Beispiel aus der Planungsrunde mit Andreas (2026-08-13/14). Direkt auf stdout, kein
           Puffer -- das ist bewusst NICHT der zu testende Pfad (der ist oben, gepuffert), sondern
           die eigentliche visuelle Demonstration. */
        n = 0;
        n += q9_ansi_clear(buf + n, sizeof(buf) - n);
        n += q9_ansi_move(buf + n, sizeof(buf) - n, 1, 1);
        fwrite(buf, 1, n, stdout);

        n = 0;
        n += q9_ansi_move(buf + n, sizeof(buf) - n, 3, 5);
        n += q9_ansi_fg_rgb(buf + n, sizeof(buf) - n, 255, 140, 0);   /* Orange */
        fwrite(buf, 1, n, stdout);
        fputs("Zeile A (Reihe 3, Spalte 5, orange)", stdout);

        n = 0;
        n += q9_ansi_move(buf + n, sizeof(buf) - n, 6, 10);
        n += q9_ansi_fg_rgb(buf + n, sizeof(buf) - n, 0, 200, 0);     /* Gruen */
        fwrite(buf, 1, n, stdout);
        fputs("Zeile B (Reihe 6, Spalte 10, gruen)", stdout);

        n = 0;
        n += q9_ansi_move(buf + n, sizeof(buf) - n, 9, 15);
        n += q9_ansi_fg_rgb(buf + n, sizeof(buf) - n, 80, 160, 255);  /* Blau */
        fwrite(buf, 1, n, stdout);
        fputs("Zeile C (Reihe 9, Spalte 15, blau)", stdout);

        n = 0;
        n += q9_ansi_reset(buf + n, sizeof(buf) - n);
        n += q9_ansi_move(buf + n, sizeof(buf) - n, 12, 1);
        fwrite(buf, 1, n, stdout);
        fputs("\n", stdout);
        fflush(stdout);
        return 0;
    }

    printf("=== q9_ansi: exakte Byte-Sequenzen (String-Vergleich gegen den ANSI/VT100-Standard) ===\n");
    n = q9_ansi_move(buf, sizeof(buf), 5, 10);
    buf[n] = '\0';
    check_str("CUP Zeile 5, Spalte 10", buf, "\x1b[5;10H");

    n = q9_ansi_move(buf, sizeof(buf), 1, 1);
    buf[n] = '\0';
    check_str("CUP Zeile 1, Spalte 1 (obere linke Ecke)", buf, "\x1b[1;1H");

    n = q9_ansi_fg_rgb(buf, sizeof(buf), 255, 140, 0);
    buf[n] = '\0';
    check_str("SGR Vordergrund RGB(255,140,0)", buf, "\x1b[38;2;255;140;0m");

    n = q9_ansi_bg_rgb(buf, sizeof(buf), 0, 0, 0);
    buf[n] = '\0';
    check_str("SGR Hintergrund RGB(0,0,0)", buf, "\x1b[48;2;0;0;0m");

    n = q9_ansi_reset(buf, sizeof(buf));
    buf[n] = '\0';
    check_str("SGR Reset", buf, "\x1b[0m");

    n = q9_ansi_clear(buf, sizeof(buf));
    buf[n] = '\0';
    check_str("ED Bildschirm loeschen", buf, "\x1b[2J");

    n = q9_ansi_hide_cursor(buf, sizeof(buf));
    buf[n] = '\0';
    check_str("Cursor ausblenden", buf, "\x1b[?25l");

    n = q9_ansi_show_cursor(buf, sizeof(buf));
    buf[n] = '\0';
    check_str("Cursor einblenden", buf, "\x1b[?25h");

    printf("=== q9_ansi: Ruecklese-Parse -- 'Zeile X angefordert' == 'Zeile X in den Bytes' ===\n");
    {
        int row, col, i;
        /* Mehrere Werte, inkl. der konkreten "3 Zeilen tiefer, 4 Zeichen nach links"-Verschiebung
           aus der urspruenglichen Planungsrunde: Basis (10,20), verschoben -> (13,16). */
        static const int cases[][2] = { {5, 10}, {1, 1}, {99, 1}, {10, 20}, {13, 16} };
        for (i = 0; i < (int)(sizeof(cases) / sizeof(cases[0])); i++) {
            char label[64];
            n = q9_ansi_move(buf, sizeof(buf), cases[i][0], cases[i][1]);
            buf[n] = '\0';
            row = col = -1;
            sscanf(buf, "\x1b[%d;%dH", &row, &col);
            snprintf(label, sizeof(label), "angefordert (%d,%d) -> Zeile in den Bytes",
                     cases[i][0], cases[i][1]);
            check_int(label, row, cases[i][0]);
            snprintf(label, sizeof(label), "angefordert (%d,%d) -> Spalte in den Bytes",
                     cases[i][0], cases[i][1]);
            check_int(label, col, cases[i][1]);
        }
    }
    {
        int r, g, b;
        n = q9_ansi_fg_rgb(buf, sizeof(buf), 12, 34, 56);
        buf[n] = '\0';
        r = g = b = -1;
        sscanf(buf, "\x1b[38;2;%d;%d;%dm", &r, &g, &b);
        check_int("RGB(12,34,56) -> R in den Bytes", r, 12);
        check_int("RGB(12,34,56) -> G in den Bytes", g, 34);
        check_int("RGB(12,34,56) -> B in den Bytes", b, 56);
    }

    printf("=== q9_ansi: Farbwerte werden auf 0..255 geklemmt (keine UB bei Fehleingabe) ===\n");
    {
        int r = -1, g = -1, b = -1;
        n = q9_ansi_fg_rgb(buf, sizeof(buf), 300, -5, 128);
        buf[n] = '\0';
        sscanf(buf, "\x1b[38;2;%d;%d;%dm", &r, &g, &b);
        check_int("300 wird auf 255 geklemmt", r, 255);
        check_int("-5 wird auf 0 geklemmt", g, 0);
        check_int("128 bleibt unveraendert", b, 128);
    }

    printf("=== q9_ansi: kleiner Puffer -- kein Ueberlauf, kein Absturz ===\n");
    {
        /* Sentinel-Muster VOR und NACH einem absichtlich zu kleinen Puffer -- ein Ueberlauf
           (Schreiben ausserhalb des uebergebenen Bereichs) wuerde die Nachbarn veraendern. */
        struct { char guard_lo[8]; char small[6]; char guard_hi[8]; } probe;
        unsigned wrote;
        memset(&probe, 0x5A, sizeof(probe));
        wrote = q9_ansi_move(probe.small, sizeof(probe.small), 12345, 67890);
        check_int("Rueckgabe bleibt innerhalb des Puffers (< out_max)", wrote < sizeof(probe.small), 1);
        {
            int guard_ok = 1, i;
            for (i = 0; i < 8; i++) {
                if ((unsigned char)probe.guard_lo[i] != 0x5A || (unsigned char)probe.guard_hi[i] != 0x5A) {
                    guard_ok = 0;
                }
            }
            check_int("Sentinel-Bereiche vor/nach dem Puffer unveraendert", guard_ok, 1);
        }
    }

    printf("\n=== Zusammenfassung ===\n");
    printf("  Gesamt: %d Checks fehlgeschlagen\n", g_fails);
    if (g_fails == 0) {
        printf("  (--demo fuer eine Sichtpruefung in einem echten Terminal)\n");
    }
    return g_fails ? 1 : 0;
}

//────────────────────────────────────────────────────────────────────────────────────────────────
// EOF ansi_selftest.c                                                                     Ver. 1.00
//────────────────────────────────────────────────────────────────────────────────────────────────
