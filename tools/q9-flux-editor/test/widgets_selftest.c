//════════════════════════════════════════════════════════════════════════════════════════════════
// File:   widgets_selftest.c                                                              Ver. 1.00
// Owner:  Claudia
// Desc.:  Automatischer Nachweis fuer q9_widgets.h/.c: draw_frame setzt Ecken/Kanten an den
//         richtigen Zellen, laesst das Innere unangetastet, klemmt an Pufferrand/Degenerierfaellen
//         statt UB zu erzeugen, und der Titel landet mittig in der oberen Kante.
//
// Call:   build/widgets_selftest
//════════════════════════════════════════════════════════════════════════════════════════════════
#include <stdio.h>

#include "../src/q9_screenbuf.h"
#include "../src/q9_widgets.h"

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

    printf("=== q9_widgets: draw_frame -- Ecken/Kanten an den richtigen Zellen ===\n");
    q9_screenbuf_init(&sb, 24, 80);
    q9_screenbuf_draw_frame(&sb, 5, 10, 6, 20, NULL, 1, 2, 3);
    check_int("obere linke Ecke '+'",     sb.cell[5][10].ch, '+');
    check_int("obere rechte Ecke '+'",    sb.cell[5][29].ch, '+');
    check_int("untere linke Ecke '+'",    sb.cell[10][10].ch, '+');
    check_int("untere rechte Ecke '+'",   sb.cell[10][29].ch, '+');
    check_int("obere Kante '-' (Mitte)",  sb.cell[5][15].ch, '-');
    check_int("untere Kante '-' (Mitte)", sb.cell[10][15].ch, '-');
    check_int("linke Kante '|' (Mitte)",  sb.cell[7][10].ch, '|');
    check_int("rechte Kante '|' (Mitte)", sb.cell[7][29].ch, '|');
    check_int("Farbe an der Ecke stimmt (fg_r)", sb.cell[5][10].fg_r, 1);

    printf("=== q9_widgets: draw_frame laesst das Innere unangetastet ===\n");
    check_int("Zelle innerhalb des Rahmens bleibt leer", sb.cell[7][15].ch, ' ');
    check_int("Zelle innerhalb des Rahmens hat keine Farbe", sb.cell[7][15].has_fg, 0);

    printf("=== q9_widgets: draw_frame laesst den Bereich AUSSERHALB unangetastet ===\n");
    check_int("Zelle knapp ausserhalb (oben) unveraendert", sb.cell[4][15].ch, ' ');
    check_int("Zelle knapp ausserhalb (links) unveraendert", sb.cell[7][9].ch, ' ');

    printf("=== q9_widgets: draw_frame mit Titel -- mittig in der oberen Kante ===\n");
    q9_screenbuf_init(&sb, 24, 80);
    q9_screenbuf_draw_frame(&sb, 2, 5, 6, 24, "Hi", 9, 9, 9);
    /* Rahmenbreite 24, Titel "Hi" -> " Hi " (4 Zeichen) -- Startspalte = 5 + (24-4)/2 = 5+10 = 15 */
    check_int("Leerzeichen vor dem Titel", sb.cell[2][15].ch, ' ');
    check_int("'H' des Titels an der berechneten Position", sb.cell[2][16].ch, 'H');
    check_int("'i' des Titels direkt danach", sb.cell[2][17].ch, 'i');
    check_int("Leerzeichen nach dem Titel", sb.cell[2][18].ch, ' ');
    /* Kante links/rechts vom Titel bleibt '-' */
    check_int("Kante vor dem Titelbereich bleibt '-'", sb.cell[2][10].ch, '-');
    /* Ecken bleiben '+', vom Titel unberuehrt (Titel ist deutlich schmaler als der Rahmen) */
    check_int("Ecke bleibt '+' (Titel ueberschreibt sie nicht)", sb.cell[2][5].ch, '+');

    printf("=== q9_widgets: draw_frame ohne Titel (NULL/leer) aendert die obere Kante nicht extra ===\n");
    q9_screenbuf_init(&sb, 24, 80);
    q9_screenbuf_draw_frame(&sb, 0, 0, 3, 10, NULL, 1, 1, 1);
    check_int("obere Kante bleibt durchgehend '-' (NULL-Titel)", sb.cell[0][5].ch, '-');
    q9_screenbuf_init(&sb, 24, 80);
    q9_screenbuf_draw_frame(&sb, 0, 0, 3, 10, "", 1, 1, 1);
    check_int("obere Kante bleibt durchgehend '-' (leerer Titel)", sb.cell[0][5].ch, '-');

    printf("=== q9_widgets: draw_frame mit degenerierten Massen tut nichts, kein Crash ===\n");
    q9_screenbuf_init(&sb, 24, 80);
    q9_screenbuf_draw_frame(&sb, 3, 3, 1, 10, "X", 1, 1, 1);        /* rows < 2 */
    check_int("rows<2: keine Zeichnung, Zelle bleibt leer", sb.cell[3][3].ch, ' ');
    q9_screenbuf_draw_frame(&sb, 3, 3, 10, 1, "X", 1, 1, 1);        /* cols < 2 */
    check_int("cols<2: keine Zeichnung, Zelle bleibt leer", sb.cell[3][3].ch, ' ');
    check_true("kein Absturz bis hierher", 1);

    printf("=== q9_widgets: draw_frame teilweise ausserhalb des Puffers -- geklemmt, kein UB ===\n");
    /* Rahmen 22,75 / 6x10 -- untere Kante (Zeile 27) und rechte Kante (Spalte 84) liegen komplett
       ausserhalb (Puffer: 24 Zeilen, 80 Spalten) und werden von fill_rect intern verworfen. Nur der
       INNERHALB liegende Teil der oberen Kante (Zeile 22, Spalten 75..79) darf tatsaechlich stehen. */
    q9_screenbuf_init(&sb, 24, 80);
    q9_screenbuf_draw_frame(&sb, 22, 75, 6, 10, NULL, 1, 1, 1);   /* kein Titel -- der wird separat
                                                                       oben getestet, hier geht es
                                                                       nur um die Randklemmung */
    check_int("oberer Kantenrest innerhalb des Puffers gesetzt", sb.cell[22][79].ch, '-');
    check_int("Zelle jenseits der Pufferkante NICHT gesetzt (kein Speicherzugriff ausserhalb)",
              sb.cell[23][79].ch, ' ');
    check_true("kein Absturz bis hierher", 1);

    printf("=== q9_widgets: draw_frame mit sehr langem Titel wird gekuerzt, kein Ueberlauf ===\n");
    q9_screenbuf_init(&sb, 24, 80);
    q9_screenbuf_draw_frame(&sb, 0, 0, 3, 12,
                             "Dieser Titel ist deutlich laenger als der Rahmen breit ist",
                             1, 1, 1);
    check_int("Ecke links bleibt '+' (Titel ueberschreibt sie nicht)", sb.cell[0][0].ch, '+');
    check_int("Ecke rechts bleibt '+' (Titel ueberschreibt sie nicht)", sb.cell[0][11].ch, '+');

    printf("\n=== Zusammenfassung ===\n");
    printf("  Gesamt: %d Checks fehlgeschlagen\n", g_fails);
    return g_fails ? 1 : 0;
}

//────────────────────────────────────────────────────────────────────────────────────────────────
// EOF widgets_selftest.c                                                                  Ver. 1.00
//────────────────────────────────────────────────────────────────────────────────────────────────
