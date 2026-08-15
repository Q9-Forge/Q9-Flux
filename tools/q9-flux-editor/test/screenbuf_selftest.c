//════════════════════════════════════════════════════════════════════════════════════════════════
// File:   screenbuf_selftest.c                                                            Ver. 1.00
// Owner:  Claudia
// Desc.:  Automatischer Nachweis fuer q9_screenbuf.h/.c: Zellen landen an der richtigen Position mit
//         der richtigen Farbe im gerenderten ANSI-Byte-Strom (Ruecklese-Parse wie ansi_selftest.c),
//         Grenzen (row/col ausserhalb, zu grosse Rechtecke) werden abgeschnitten statt UB zu
//         erzeugen, und snapshot()+restore() liefern den Ausgangszustand exakt zurueck -- das ist
//         die eigentliche Zusicherung, die der modale Dialog aus Q9FLUX_EDITOR_de.md Abschnitt 2
//         braucht ("Bildschirmbereich sichern, nach dem Schliessen wiederherstellen").
//
// Call:   build/screenbuf_selftest
//════════════════════════════════════════════════════════════════════════════════════════════════
#include <stdio.h>
#include <string.h>

#include "../src/q9_screenbuf.h"

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

int main(void)
{
    q9_screenbuf_t sb;
    char out[16384];
    unsigned n;

    printf("=== q9_screenbuf: init (Groesse, Klemmung auf MAX, negative Werte -> 0) ===\n");
    q9_screenbuf_init(&sb, 24, 80);
    check_int("rows == 24", sb.rows, 24);
    check_int("cols == 80", sb.cols, 80);
    q9_screenbuf_init(&sb, 999, 999);
    check_int("rows auf MAX geklemmt", sb.rows, Q9_SCREENBUF_MAX_ROWS);
    check_int("cols auf MAX geklemmt", sb.cols, Q9_SCREENBUF_MAX_COLS);
    q9_screenbuf_init(&sb, -5, -5);
    check_int("negative rows -> 0", sb.rows, 0);
    check_int("negative cols -> 0", sb.cols, 0);
    q9_screenbuf_init(&sb, 24, 80);                       /* fuer die restlichen Tests wieder normal */

    printf("=== q9_screenbuf: puts landet Zeichen+Farbe an der richtigen Zelle ===\n");
    q9_screenbuf_puts(&sb, 2, 4, "Hi", 255, 140, 0);
    check_int("Zeichen 'H' an (2,4)", sb.cell[2][4].ch, 'H');
    check_int("Zeichen 'i' an (2,5)", sb.cell[2][5].ch, 'i');
    check_int("has_fg gesetzt", sb.cell[2][4].has_fg, 1);
    check_int("fg_r == 255", sb.cell[2][4].fg_r, 255);
    check_int("fg_g == 140", sb.cell[2][4].fg_g, 140);
    check_int("fg_b == 0", sb.cell[2][4].fg_b, 0);
    check_int("Nachbarzelle (2,6) unveraendert (Leerzeichen)", sb.cell[2][6].ch, ' ');

    printf("=== q9_screenbuf: puts an der rechten Kante wird abgeschnitten, kein UB ===\n");
    q9_screenbuf_puts(&sb, 0, 78, "ABCDEF", 1, 2, 3);
    check_int("Zeichen 'A' an (0,78)", sb.cell[0][78].ch, 'A');
    check_int("Zeichen 'B' an (0,79), letzte gueltige Spalte", sb.cell[0][79].ch, 'B');

    printf("=== q9_screenbuf: puts mit negativer Spalte schneidet links ab ===\n");
    q9_screenbuf_init(&sb, 24, 80);
    q9_screenbuf_puts(&sb, 5, -2, "XYZ!", 9, 9, 9);
    check_int("'X'/'Y' wurden verworfen, 'Z' landet an Spalte 0", sb.cell[5][0].ch, 'Z');
    check_int("'!' an Spalte 1", sb.cell[5][1].ch, '!');

    printf("=== q9_screenbuf: puts an ungueltiger Zeile tut nichts (kein Crash) ===\n");
    q9_screenbuf_puts(&sb, -1, 0, "sollte nichts tun", 1, 1, 1);
    q9_screenbuf_puts(&sb, 999, 0, "sollte nichts tun", 1, 1, 1);
    check_true("kein Absturz bis hierher", 1);

    printf("=== q9_screenbuf: fill_rect setzt Zeichen/Farbe/Hintergrund im Rechteck, nicht daneben ===\n");
    q9_screenbuf_init(&sb, 24, 80);
    q9_screenbuf_fill_rect(&sb, 3, 3, 2, 4, '#', 200, 200, 200, 1, 10, 20, 30);
    check_int("(3,3) gefuellt", sb.cell[3][3].ch, '#');
    check_int("(4,6) gefuellt (Rechteck-Ecke)", sb.cell[4][6].ch, '#');
    check_int("has_bg gesetzt", sb.cell[3][3].has_bg, 1);
    check_int("bg_r == 10", sb.cell[3][3].bg_r, 10);
    check_int("(3,7) AUSSERHALB des Rechtecks unveraendert", sb.cell[3][7].ch, ' ');
    check_int("(3,7) hat keinen Hintergrund", sb.cell[3][7].has_bg, 0);
    check_int("(2,3) AUSSERHALB des Rechtecks unveraendert", sb.cell[2][3].ch, ' ');

    printf("=== q9_screenbuf: fill_rect ueber die Puffergrenze hinaus wird geklemmt, kein UB ===\n");
    q9_screenbuf_fill_rect(&sb, 22, 78, 5, 5, '@', 1, 1, 1, 0, 0, 0, 0);
    check_int("(23,79) (letzte gueltige Zelle) gefuellt", sb.cell[23][79].ch, '@');
    check_true("kein Absturz bis hierher", 1);

    printf("=== q9_screenbuf: render erzeugt CUP an der richtigen Position (Ruecklese-Parse) ===\n");
    {
        int row, col;
        q9_screenbuf_init(&sb, 3, 10);
        q9_screenbuf_puts(&sb, 1, 2, "X", 5, 6, 7);
        n = q9_screenbuf_render(&sb, 0, 0, out, sizeof(out));
        check_true("render liefert etwas (n > 0)", n > 0);
        /* Der Puffer beginnt mit CUP fuer Zeile 1 (0-indiziert Zeile 0 -> ANSI-Zeile 1). */
        row = col = -1;
        sscanf(out, "\x1b[%d;%dH", &row, &col);
        check_int("erste Zeile des Renders -> ANSI-Zeile 1", row, 1);
        check_int("erste Zeile des Renders -> ANSI-Spalte 1", col, 1);
    }

    printf("=== q9_screenbuf: render mit origin_row/origin_col verschiebt die Position ===\n");
    {
        int row, col;
        char *p;
        q9_screenbuf_init(&sb, 2, 2);
        n = q9_screenbuf_render(&sb, 5, 10, out, sizeof(out));
        row = col = -1;
        sscanf(out, "\x1b[%d;%dH", &row, &col);
        check_int("origin_row=5 -> erste ANSI-Zeile 6", row, 6);
        check_int("origin_col=10 -> erste ANSI-Spalte 11", col, 11);
        /* zweite Zeile des Renders: irgendwo im Puffer muss "\x1b[7;11H" auftauchen (Zeile 2). */
        p = strstr(out, "\x1b[7;11H");
        check_true("zweite Zeile des Renders an ANSI-Zeile 7 gefunden", p != NULL);
    }

    printf("=== q9_screenbuf: render enthaelt die gesetzte Farbe (Ruecklese-Parse) ===\n");
    {
        int r, g, b;
        char *p;
        q9_screenbuf_init(&sb, 1, 5);
        q9_screenbuf_puts(&sb, 0, 0, "X", 12, 34, 56);
        n = q9_screenbuf_render(&sb, 0, 0, out, sizeof(out));
        p = strstr(out, "\x1b[38;2;");
        check_true("SGR-Vordergrund-Sequenz gefunden", p != NULL);
        r = g = b = -1;
        if (p) { sscanf(p, "\x1b[38;2;%d;%d;%dm", &r, &g, &b); }
        check_int("R == 12", r, 12);
        check_int("G == 34", g, 34);
        check_int("B == 56", b, 56);
    }

    printf("=== q9_screenbuf: render endet mit SGR-Reset (Terminal bleibt sauber) ===\n");
    {
        unsigned len;
        q9_screenbuf_init(&sb, 1, 1);
        n = q9_screenbuf_render(&sb, 0, 0, out, sizeof(out));
        len = (unsigned)strlen("\x1b[0m");
        check_true("Render endet mit \\x1b[0m",
                   n >= len && memcmp(out + n - len, "\x1b[0m", len) == 0);
    }

    printf("=== q9_screenbuf: kleiner Render-Puffer -- kein Ueberlauf, kein Absturz ===\n");
    {
        struct { char guard_lo[8]; char small[20]; char guard_hi[8]; } probe;
        unsigned wrote, i;
        int guard_ok = 1;
        memset(&probe, 0x5A, sizeof(probe));
        q9_screenbuf_init(&sb, 24, 80);
        q9_screenbuf_fill_rect(&sb, 0, 0, 24, 80, 'X', 1, 2, 3, 1, 4, 5, 6);
        wrote = q9_screenbuf_render(&sb, 0, 0, probe.small, sizeof(probe.small));
        check_true("Rueckgabe bleibt innerhalb des Puffers (<= out_max)", wrote <= sizeof(probe.small));
        for (i = 0; i < 8; i++) {
            if ((unsigned char)probe.guard_lo[i] != 0x5A || (unsigned char)probe.guard_hi[i] != 0x5A) {
                guard_ok = 0;
            }
        }
        check_true("Sentinel-Bereiche vor/nach dem Puffer unveraendert", guard_ok);
    }

    printf("=== q9_screenbuf: snapshot/restore -- der modale-Dialog-Anwendungsfall ===\n");
    {
        q9_screenbuf_t snap;
        q9_screenbuf_init(&sb, 24, 80);
        q9_screenbuf_puts(&sb, 6, 12, "Hintergrund", 1, 2, 3);

        q9_screenbuf_snapshot(&sb, 5, 10, 4, 20, &snap);
        check_int("snap.rows == 4", snap.rows, 4);
        check_int("snap.cols == 20", snap.cols, 20);
        check_int("snap enthaelt 'H' von 'Hintergrund' (Zeile 6-5=1, Spalte 12-10=2)",
                  snap.cell[1][2].ch, 'H');

        /* "Dialog" ueber den Bereich zeichnen -- ueberschreibt den Originalinhalt in sb. */
        q9_screenbuf_fill_rect(&sb, 5, 10, 4, 20, '#', 9, 9, 9, 0, 0, 0, 0);
        check_int("Dialog hat den Originalinhalt ueberschrieben", sb.cell[6][12].ch, '#');

        /* Dialog schliessen: restore bringt den Originalinhalt zurueck. */
        q9_screenbuf_restore(&sb, 5, 10, &snap);
        check_int("nach restore: Originalzeichen 'H' wieder da", sb.cell[6][12].ch, 'H');
        check_int("has_fg des wiederhergestellten Zeichens stimmt", sb.cell[6][12].has_fg, 1);
        check_int("fg_r des wiederhergestellten Zeichens stimmt", sb.cell[6][12].fg_r, 1);
    }

    printf("=== q9_screenbuf: snapshot mit negativem/zu grossem Rechteck wird geklemmt ===\n");
    {
        q9_screenbuf_t snap;
        q9_screenbuf_init(&sb, 10, 10);
        q9_screenbuf_snapshot(&sb, -3, -3, 6, 6, &snap);
        check_int("negativer Start: rows um 3 verkleinert (6-3=3)", snap.rows, 3);
        check_int("negativer Start: cols um 3 verkleinert (6-3=3)", snap.cols, 3);

        q9_screenbuf_snapshot(&sb, 8, 8, 100, 100, &snap);
        check_int("zu grosses Rechteck: rows auf sb-Rest geklemmt (10-8=2)", snap.rows, 2);
        check_int("zu grosses Rechteck: cols auf sb-Rest geklemmt (10-8=2)", snap.cols, 2);
    }

    printf("\n=== Zusammenfassung ===\n");
    printf("  Gesamt: %d Checks fehlgeschlagen\n", g_fails);
    return g_fails ? 1 : 0;
}

//────────────────────────────────────────────────────────────────────────────────────────────────
// EOF screenbuf_selftest.c                                                                Ver. 1.00
//────────────────────────────────────────────────────────────────────────────────────────────────
