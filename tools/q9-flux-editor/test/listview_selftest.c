//════════════════════════════════════════════════════════════════════════════════════════════════
// File:   listview_selftest.c                                                             Ver. 1.10
// Owner:  Claudia
// Desc.:  Automatischer Nachweis fuer q9_listview.h/.c: die reine Scroll-Logik (q9_listview_scroll)
//         haelt die Auswahl immer im Sichtfenster, ohne unnoetig zu scrollen; render() zeichnet die
//         richtigen Eintraege an die richtige Stelle, hebt die Auswahl hervor, und der Scrollbalken
//         erscheint nur, wenn tatsaechlich etwas zu scrollen ist.
//
// Call:   build/listview_selftest
//════════════════════════════════════════════════════════════════════════════════════════════════
#include <stdio.h>
#include <string.h>

#include "../src/q9_listview.h"

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
    q9_listview_t lv;
    const char *items[] = {
        "Eintrag 0", "Eintrag 1", "Eintrag 2", "Eintrag 3", "Eintrag 4",
        "Eintrag 5", "Eintrag 6", "Eintrag 7", "Eintrag 8", "Eintrag 9",
    };

    printf("=== q9_listview_scroll: Auswahl bleibt im Fenster, kein unnoetiges Scrollen ===\n");
    /* 10 Eintraege, Fenster 4 hoch. */
    check_int("Auswahl 0, Offset 0 -> bleibt 0", q9_listview_scroll(0, 0, 4, 10), 0);
    check_int("Auswahl 3 (letzte im Fenster [0..3]), Offset 0 -> bleibt 0",
              q9_listview_scroll(3, 0, 4, 10), 0);
    check_int("Auswahl 4 (faellt aus [0..3] raus), Offset 0 -> Offset auf 1 (Fenster [1..4])",
              q9_listview_scroll(4, 0, 4, 10), 1);
    check_int("Auswahl 9 (letzter Eintrag), Offset 0 -> Offset 6 (Fenster [6..9])",
              q9_listview_scroll(9, 0, 4, 10), 6);
    check_int("Auswahl 0, Offset 6 (weit gescrollt) -> Offset zurueck auf 0",
              q9_listview_scroll(0, 6, 4, 10), 0);
    check_int("Auswahl 5, Offset 6 (Auswahl VOR dem Fenster) -> Offset auf 5",
              q9_listview_scroll(5, 6, 4, 10), 5);

    printf("=== q9_listview_scroll: Randfaelle ===\n");
    check_int("item_count == 0 -> immer 0", q9_listview_scroll(0, 3, 4, 0), 0);
    check_int("height == 0 -> immer 0", q9_listview_scroll(0, 3, 0, 10), 0);
    check_int("Auswahl aus dem Bereich (negativ) wird geklemmt -> wie Auswahl 0",
              q9_listview_scroll(-5, 6, 4, 10), 0);
    check_int("Auswahl aus dem Bereich (zu gross) wird geklemmt -> wie Auswahl 9",
              q9_listview_scroll(999, 0, 4, 10), 6);
    check_int("Fenster >= item_count -> Offset bleibt 0 (nichts zu scrollen)",
              q9_listview_scroll(3, 0, 20, 10), 0);

    printf("=== q9_listview_init: Grunddaten + Klemmung ===\n");
    q9_listview_init(&lv, 5, 10, 4, 20, 10);
    check_int("row", lv.row, 5);
    check_int("col", lv.col, 10);
    check_int("height", lv.height, 4);
    check_int("width", lv.width, 20);
    check_int("item_count", lv.item_count, 10);
    check_int("selected startet bei 0", lv.selected, 0);
    check_int("scroll_offset startet bei 0", lv.scroll_offset, 0);

    q9_listview_init(&lv, 0, 0, 4, 20, 0);
    check_int("item_count==0 -> selected == -1 (nichts auswaehlbar)", lv.selected, -1);

    q9_listview_init(&lv, 0, 0, 0, 0, 5);
    check_int("height<1 wird auf 1 angehoben", lv.height, 1);
    check_int("width<1 wird auf 1 angehoben", lv.width, 1);

    printf("=== q9_listview_move: Auswahl bewegen + automatisches Nachscrollen ===\n");
    q9_listview_init(&lv, 0, 0, 4, 20, 10);
    q9_listview_move(&lv, 1);
    check_int("nach +1: selected == 1", lv.selected, 1);
    check_int("nach +1: scroll_offset bleibt 0 (noch im Fenster)", lv.scroll_offset, 0);
    q9_listview_move(&lv, 3);                              /* selected 1 -> 4, faellt aus [0..3] raus */
    check_int("nach +3 weiter (jetzt 4): selected == 4", lv.selected, 4);
    check_int("nach +3 weiter: scroll_offset auf 1 nachgezogen", lv.scroll_offset, 1);
    q9_listview_move(&lv, -100);                           /* weit ueber den Anfang hinaus */
    check_int("nach -100 (weit ueber den Anfang): selected auf 0 geklemmt", lv.selected, 0);
    check_int("nach -100: scroll_offset zurueck auf 0", lv.scroll_offset, 0);
    q9_listview_move(&lv, 100);                            /* weit ueber das Ende hinaus */
    check_int("nach +100 (weit ueber das Ende): selected auf 9 geklemmt", lv.selected, 9);
    check_int("nach +100: scroll_offset auf 6 (Fenster [6..9])", lv.scroll_offset, 6);

    q9_listview_init(&lv, 0, 0, 4, 20, 0);
    q9_listview_move(&lv, 1);
    check_int("move() bei item_count==0 tut nichts -- selected bleibt -1", lv.selected, -1);

    printf("=== q9_listview_render: sichtbare Eintraege landen an der richtigen Stelle ===\n");
    q9_screenbuf_init(&sb, 24, 80);
    q9_listview_init(&lv, 5, 10, 4, 20, 10);                /* Viewport 4 hoch -- zeigt Eintraege 0..3 */
    q9_listview_render(&lv, &sb, items, 200, 200, 200, 0, 0, 0, 255, 255, 0);
    check_int("'E' von 'Eintrag 0' an (5,10)", sb.cell[5][10].ch, 'E');
    check_true("'0' von 'Eintrag 0' irgendwo in der Zeile", sb.cell[5][18].ch == '0');
    check_int("'E' von 'Eintrag 3' an (8,10) (letzte sichtbare Zeile)", sb.cell[8][10].ch, 'E');
    check_int("Eintrag 4 ist NICHT sichtbar -- Zeile 9 bleibt leer", sb.cell[9][10].ch, ' ');

    printf("=== q9_listview_render: Auswahl wird farblich hervorgehoben ===\n");
    check_int("ausgewaehlte Zeile (0) hat den Auswahl-Hintergrund", sb.cell[5][10].has_bg, 1);
    check_int("ausgewaehlte Zeile: bg_g == 255 (Gelb, wie im Test uebergeben)", sb.cell[5][10].bg_g, 255);
    check_int("nicht ausgewaehlte Zeile (1) hat KEINEN eigenen Hintergrund", sb.cell[6][10].has_bg, 0);
    check_int("ausgewaehlte Zeile: fg stimmt (sel_fg, hier 0,0,0)", sb.cell[5][10].fg_r, 0);
    check_int("nicht ausgewaehlte Zeile: fg stimmt (normales fg, hier 200)", sb.cell[6][10].fg_r, 200);

    printf("=== q9_listview_render: Scrollbalken NUR wenn tatsaechlich etwas zu scrollen ist ===\n");
    check_int("Scrollbalken-Spalte (col+width-1 = 29) zeigt '|' oder '#' -- nicht leer",
              sb.cell[5][29].ch != ' ', 1);

    q9_screenbuf_init(&sb, 24, 80);
    q9_listview_init(&lv, 5, 10, 20, 20, 10);               /* Viewport GROESSER als item_count */
    q9_listview_render(&lv, &sb, items, 1, 1, 1, 0, 0, 0, 1, 1, 1);
    check_int("kein Scrollbalken, wenn alles reinpasst -- Spalte 29 bleibt leer", sb.cell[5][29].ch, ' ');

    printf("=== q9_listview_render: Scrollbalken-Griff ist PROPORTIONAL zum sichtbaren Anteil ===\n");
    {
        /* 20 Eintraege, Viewport-Hoehe 10 -- genau 50% sichtbar, damit muss der Griff auch genau
           50% der Balkenhoehe (5 von 10 Zeilen) einnehmen -- exakt Andreas' eigenes Beispiel
           (2026-08-17). */
        const char *big_items[20];
        int i, filled = 0, sb_col;
        for (i = 0; i < 20; i++) { big_items[i] = "Eintrag"; }

        q9_screenbuf_init(&sb, 24, 80);
        q9_listview_init(&lv, 0, 0, 10, 20, 20);
        q9_listview_render(&lv, &sb, big_items, 1, 1, 1, 0, 0, 0, 1, 1, 1);
        sb_col = lv.col + lv.width - 1;
        for (i = 0; i < 10; i++) {
            if (sb.cell[lv.row + i][sb_col].ch != Q9_GLYPH_VLINE) { filled++; }
        }
        check_int("bei 50% sichtbar (10 von 20): Griff nimmt 5 von 10 Zeilen ein", filled, 5);
        check_int("Griff beginnt oben (Auswahl/Offset == 0)", sb.cell[lv.row][sb_col].ch, Q9_GLYPH_BLOCK);

        /* Ans Ende scrollen -- Griff muss ans untere Ende der Spur wandern. */
        q9_listview_move(&lv, 19);
        q9_listview_render(&lv, &sb, big_items, 1, 1, 1, 0, 0, 0, 1, 1, 1);
        check_int("am Ende: Griff-UNTERKANTE erreicht das Ende der Spur (letzte Zeile ist Griff)",
                  sb.cell[lv.row + 9][sb_col].ch, Q9_GLYPH_BLOCK);
        check_int("am Ende: erste Spur-Zeile ist NICHT mehr Teil des Griffs",
                  sb.cell[lv.row][sb_col].ch, Q9_GLYPH_VLINE);
    }

    printf("=== q9_listview_render: Randfaelle (NULL-Zeiger), kein Absturz ===\n");
    q9_listview_render(NULL, &sb, items, 1, 1, 1, 0, 0, 0, 1, 1, 1);
    q9_listview_render(&lv, NULL, items, 1, 1, 1, 0, 0, 0, 1, 1, 1);
    q9_listview_render(&lv, &sb, NULL, 1, 1, 1, 0, 0, 0, 1, 1, 1);
    check_true("kein Absturz bis hierher", 1);

    printf("\n=== Zusammenfassung ===\n");
    printf("  Gesamt: %d Checks fehlgeschlagen\n", g_fails);
    return g_fails ? 1 : 0;
}

//────────────────────────────────────────────────────────────────────────────────────────────────
// EOF listview_selftest.c                                                                 Ver. 1.10
//────────────────────────────────────────────────────────────────────────────────────────────────
