//════════════════════════════════════════════════════════════════════════════════════════════════
// File:   listview_selftest.c                                                             Ver. 2.10
// Owner:  Claudia
// Desc.:  Automatischer Nachweis fuer q9_listview.h/.c: die reine Scroll-Logik (q9_listview_scroll)
//         haelt die Auswahl immer im Sichtfenster, ohne unnoetig zu scrollen; render() zeichnet die
//         richtigen Eintraege an die richtige Stelle, hebt die Auswahl hervor, und die rechte Spalte
//         zeigt IMMER die Linie (Griff nur zusaetzlich, wenn tatsaechlich etwas zu scrollen ist).
//
// Call:   build/listview_selftest
//
// Edition History
//─────────┬──────┬────────────────────────────────────────────────────────────────────────┬──────
// Date    │ Ver. │ Description                                                            │ By
//─────────┼──────┼────────────────────────────────────────────────────────────────────────┬──────
// 26-08-16│ 1.10 │ Erster Wurf (vorherige Historie s. q9_listview.h/.c)                    │ Cld
// 26-08-17│ 1.20 │ Test fuer "kein Scrollbedarf" angepasst -- Spalte zeigt jetzt IMMER die  │ Cld
//         │      │ Linie statt leer zu bleiben (Andreas' Wunsch, s. q9_listview.h/.c)       │
// 26-08-17│ 1.30 │ Neuer line_fg-Parameter an allen Aufrufen dazu, neuer Check bestaetigt,   │ Cld
//         │      │ dass die Linie tatsaechlich line_fg statt fg zeigt                        │
// 26-08-18│ 1.40 │ Tests fuer q9_listview_item_rows()/_scroll_ex()/_move_ex()/_render_ex()   │ Cld
//         │      │ dazu (erweiterbare Eintraege, s. q9_listview.h)                            │
// 26-08-18│ 1.50 │ render_ex()-Aufrufe um neuen exp_bg-Parameter ergaenzt, neue Checks fuer   │ Cld
//         │      │ Kopfzeilen-Hintergrund (aufgeklappt+nicht ausgewaehlt vs. ausgewaehlt)      │
// 26-08-18│ 1.60 │ Feld-Navigation: detail_lines/detail_count-Testdaten auf echte Felder      │ Cld
//         │      │ (label+value) umgestellt, neue Tests fuer field_enter/_leave/_escape/_move/│
//         │      │ _putc/_backspace + fokussierte Feldzeile in render_ex()                    │
// 26-08-18│ 1.70 │ Bestehende Feld-Initialisierer um explizites Q9_LISTVIEW_FIELD_TEXT ergaenzt│ Cld
//         │      │ (neues drittes Struct-Feld kind, s. q9_listview.h), neuer Test: BUTTON-Feld │
//         │      │ ignoriert putc()/_backspace()                                               │
// 26-08-18│ 1.80 │ render_ex()-Aufrufe um box_fg/bg ergaenzt, neue Tests fuer TEXT+BUTTON-Paar  │ Cld
//         │      │ (item_rows()==3, Box-Farbe, dreizeiliger Button mit Kappen, Fokus-Wechsel)   │
// 26-08-18│ 1.90 │ Spaltenerwartungen auf VALUE_BOX_WIDTH=35 angepasst (Andreas: "im Dialog ca. │ Cld
//         │      │ 35 Zeichen")                                                                 │
// 26-08-18│ 2.00 │ Neue Tests fuer NUMERIC_DEC/_HEX -- Zeichenklassen-Filterung (field_putc()), │ Cld
//         │      │ automatisches "$"-Praefix bei NUMERIC_HEX (render_ex())                       │
// 26-08-18│ 2.10 │ Neue Tests fuer field_toggle() -- ja/nein-Umschaltung, No-op bei TEXT-Feldern, │ Cld
//         │      │ unerwarteter Ausgangswert wird zu "ja", NULL-Zeiger-Sicherheit                │
//═════════╧══════╧════════════════════════════════════════════════════════════════════════╧══════
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
    q9_listview_render(&lv, &sb, items, 200, 200, 200, 0, 0, 0, 255, 255, 0, 77, 88, 99);
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

    printf("=== q9_listview_render: rechte Spalte zeigt IMMER eine Linie (Andreas' Wunsch, 2026-08-17) ===\n");
    check_int("Scrollbalken-Spalte (col+width-1 = 29) zeigt '|' oder '#' -- nicht leer",
              sb.cell[5][29].ch != ' ', 1);

    printf("=== q9_listview_render: Linie hat eine EIGENE Farbe (line_fg), unabhaengig von fg ===\n");
    /* Zeile 6 (nicht 5!) -- bei item_count=10, height=4 ist der Scroll-Griff genau 1 Zeile hoch
       und beginnt bei Zeile 5 (scroll_offset==0, s. Test oben); Zeile 6 zeigt garantiert die reine
       Spur (Q9_GLYPH_VLINE in line_fg), nicht den Griff (der in sel_bg gezeichnet wird). */
    check_int("Linien-Spalte (reine Spur, Zeile 6): fg_r stimmt mit line_fg (77) ueberein, NICHT mit fg (200)",
              sb.cell[6][29].fg_r, 77);
    check_int("Linien-Spalte: fg_g stimmt mit line_fg (88) ueberein", sb.cell[6][29].fg_g, 88);
    check_int("Linien-Spalte: fg_b stimmt mit line_fg (99) ueberein", sb.cell[6][29].fg_b, 99);

    q9_screenbuf_init(&sb, 24, 80);
    q9_listview_init(&lv, 5, 10, 20, 20, 10);               /* Viewport GROESSER als item_count */
    q9_listview_render(&lv, &sb, items, 1, 1, 1, 0, 0, 0, 1, 1, 1, 1, 1, 1);
    check_int("kein Scrollbedarf -- Spalte 29 zeigt trotzdem die durchgehende Linie (kein Griff)",
              sb.cell[5][29].ch, Q9_GLYPH_VLINE);

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
        q9_listview_render(&lv, &sb, big_items, 1, 1, 1, 0, 0, 0, 1, 1, 1, 1, 1, 1);
        sb_col = lv.col + lv.width - 1;
        for (i = 0; i < 10; i++) {
            if (sb.cell[lv.row + i][sb_col].ch != Q9_GLYPH_VLINE) { filled++; }
        }
        check_int("bei 50% sichtbar (10 von 20): Griff nimmt 5 von 10 Zeilen ein", filled, 5);
        check_int("Griff beginnt oben (Auswahl/Offset == 0)", sb.cell[lv.row][sb_col].ch, Q9_GLYPH_BLOCK);

        /* Ans Ende scrollen -- Griff muss ans untere Ende der Spur wandern. */
        q9_listview_move(&lv, 19);
        q9_listview_render(&lv, &sb, big_items, 1, 1, 1, 0, 0, 0, 1, 1, 1, 1, 1, 1);
        check_int("am Ende: Griff-UNTERKANTE erreicht das Ende der Spur (letzte Zeile ist Griff)",
                  sb.cell[lv.row + 9][sb_col].ch, Q9_GLYPH_BLOCK);
        check_int("am Ende: erste Spur-Zeile ist NICHT mehr Teil des Griffs",
                  sb.cell[lv.row][sb_col].ch, Q9_GLYPH_VLINE);
    }

    printf("=== q9_listview_render: Randfaelle (NULL-Zeiger), kein Absturz ===\n");
    q9_listview_render(NULL, &sb, items, 1, 1, 1, 0, 0, 0, 1, 1, 1, 1, 1, 1);
    q9_listview_render(&lv, NULL, items, 1, 1, 1, 0, 0, 0, 1, 1, 1, 1, 1, 1);
    q9_listview_render(&lv, &sb, NULL, 1, 1, 1, 0, 0, 0, 1, 1, 1, 1, 1, 1);
    check_true("kein Absturz bis hierher", 1);

    printf("=== q9_listview_item_rows: erweiterbare Eintraege (Andreas' Wunsch, 2026-08-18) ===\n");
    {
        static q9_listview_field_t fields_a[] = { {"L1", "V1", Q9_LISTVIEW_FIELD_TEXT}, {"L2", "V2", Q9_LISTVIEW_FIELD_TEXT} };
        static const q9_listview_item_t rows_items[] = {
            { "Item0", NULL,     0 },                       /* nicht erweiterbar (keine fields)  */
            { "Item1", fields_a, 2 },
        };
        /* JE EIN eigenes 2-Element-Array pro Fall -- q9_listview_item_rows() liest expanded[index],
           ein Zeiger auf einen einzelnen int waere bei index==1 ein Zugriff ausserhalb des Arrays
           (undefiniertes Verhalten). */
        int exp_none[2]  = { 0, 0 };                        /* nichts aufgeklappt                */
        int exp_item0[2] = { 1, 0 };                        /* Item0 "aufgeklappt", hat aber keine
                                                                 Felder (field_count==0)           */
        int exp_item1[2] = { 0, 1 };                        /* Item1 aufgeklappt                 */

        check_int("items==NULL -> immer 1", q9_listview_item_rows(NULL, exp_item1, 0), 1);
        check_int("expanded==NULL -> immer 1 (auch bei field_count>0)",
                  q9_listview_item_rows(rows_items, NULL, 1), 1);
        check_int("zugeklappt (expanded[i]==0) -> 1, egal wie viele Felder",
                  q9_listview_item_rows(rows_items, exp_none, 1), 1);
        check_int("field_count<=0 -> 1, auch wenn expanded[i]==1 (nichts zum Aufklappen)",
                  q9_listview_item_rows(rows_items, exp_item0, 0), 1);
        check_int("aufgeklappt, field_count==2 -> 1 (Kopf) + 2 (Felder) + 1 (Trennlinie) == 4",
                  q9_listview_item_rows(rows_items, exp_item1, 1), 4);
    }

    printf("=== q9_listview_item_rows: TEXT+BUTTON-Paar zaehlt als 3 Zeilen (Andreas' Wunsch, "
           "2026-08-18: \"der dreizeilige Button wie im Dialog\") ===\n");
    {
        static q9_listview_field_t pair_fields[] = {
            {"Datei:", "x", Q9_LISTVIEW_FIELD_TEXT}, {"", "Datei", Q9_LISTVIEW_FIELD_BUTTON},
        };
        static q9_listview_field_t mixed_fields[] = {
            {"L0:", "v0", Q9_LISTVIEW_FIELD_TEXT},                    /* normales Feld, 1 Zeile     */
            {"Datei:", "x", Q9_LISTVIEW_FIELD_TEXT},                  /* TEXT+BUTTON-Paar, 3 Zeilen */
            {"", "Datei", Q9_LISTVIEW_FIELD_BUTTON},
            {"L3:", "v3", Q9_LISTVIEW_FIELD_TEXT},                    /* wieder normal, 1 Zeile     */
        };
        static const q9_listview_item_t pair_items[] = {
            { "NurPaar", pair_fields, 2 },
            { "Gemischt", mixed_fields, 4 },
        };
        int exp_all[2] = { 1, 1 };

        check_int("nur ein TEXT+BUTTON-Paar: 1 (Kopf) + 3 (Paar) + 1 (Trennlinie) == 5",
                  q9_listview_item_rows(pair_items, exp_all, 0), 5);
        check_int("gemischt (1 normal + Paar + 1 normal): 1 + 1 + 3 + 1 + 1 == 7",
                  q9_listview_item_rows(pair_items, exp_all, 1), 7);
    }

    printf("=== q9_listview_scroll_ex: identisch zu q9_listview_scroll() bei items==NULL ===\n");
    /* Ohne Items/Zustand muss sich _ex() exakt wie das einfache q9_listview_scroll() verhalten
       (alle Zeilenzahlen == 1) -- direkter Regressionsnachweis anhand derselben Faelle wie oben. */
    check_int("Auswahl 4 (faellt aus [0..3] raus), Offset 0 -> Offset auf 1",
              q9_listview_scroll_ex(4, 0, 4, NULL, NULL, 10), 1);
    check_int("Auswahl 9 (letzter Eintrag), Offset 0 -> Offset 6",
              q9_listview_scroll_ex(9, 0, 4, NULL, NULL, 10), 6);
    check_int("Auswahl 5, Offset 6 (Auswahl VOR dem Fenster) -> Offset auf 5",
              q9_listview_scroll_ex(5, 6, 4, NULL, NULL, 10), 5);
    check_int("item_count<=0 -> immer 0", q9_listview_scroll_ex(0, 3, 4, NULL, NULL, 0), 0);
    check_int("height<=0 -> immer 0", q9_listview_scroll_ex(0, 3, 0, NULL, NULL, 10), 0);

    printf("=== q9_listview_scroll_ex: gemischte Zeilenhoehen (aufgeklappte Eintraege) ===\n");
    {
        /* 5 Eintraege: Item0 (1 Zeile), Item1 aufgeklappt (1+2+1=4 Zeilen), Item2-4 (je 1 Zeile).
           Viewport-Hoehe 4. */
        static q9_listview_field_t fields_a[] = { {"L1", "V1", Q9_LISTVIEW_FIELD_TEXT}, {"L2", "V2", Q9_LISTVIEW_FIELD_TEXT} };
        static const q9_listview_item_t mix_items[] = {
            { "Item0", NULL,     0 },
            { "Item1", fields_a, 2 },
            { "Item2", NULL,     0 },
            { "Item3", NULL,     0 },
            { "Item4", NULL,     0 },
        };
        int mix_expanded[5] = { 0, 1, 0, 0, 0 };             /* nur Item1 aufgeklappt */

        check_int("Auswahl 0 (1 Zeile), Offset 0 -> passt, bleibt 0",
                  q9_listview_scroll_ex(0, 0, 4, mix_items, mix_expanded, 5), 0);
        check_int("Auswahl 1 (4 Zeilen aufgeklappt), Offset 0 -> Item0 wird oben abgeschnitten, "
                  "Offset auf 1 (genau die 4 Zeilen von Item1 passen)",
                  q9_listview_scroll_ex(1, 0, 4, mix_items, mix_expanded, 5), 1);
        check_int("Auswahl 2, Offset 1 (Item1 nimmt allein schon die volle Hoehe) -> Item1 muss "
                  "weichen, Offset auf 2 (nur noch Item2's Kopfzeile muss sichtbar bleiben)",
                  q9_listview_scroll_ex(2, 1, 4, mix_items, mix_expanded, 5), 2);
        check_int("Auswahl 0, Offset 2 (nach oben rausgelaufen) -> Offset direkt auf 0",
                  q9_listview_scroll_ex(0, 2, 4, mix_items, mix_expanded, 5), 0);
    }

    printf("=== q9_listview_move_ex: delta==0 richtet nur den Scroll neu aus (nach Auf-/Zuklappen) ===\n");
    {
        static q9_listview_field_t fields_a[] = { {"L1", "V1", Q9_LISTVIEW_FIELD_TEXT}, {"L2", "V2", Q9_LISTVIEW_FIELD_TEXT} };
        static const q9_listview_item_t mix_items[] = {
            { "Item0", NULL,     0 },
            { "Item1", fields_a, 2 },
            { "Item2", NULL,     0 },
            { "Item3", NULL,     0 },
            { "Item4", NULL,     0 },
        };
        int mix_expanded[5] = { 0, 0, 0, 0, 0 };

        q9_listview_init(&lv, 0, 0, 4, 20, 5);
        q9_listview_move_ex(&lv, 1, mix_items, mix_expanded);     /* Auswahl auf Item1, noch zugeklappt */
        check_int("Item1 ausgewaehlt, noch zugeklappt: scroll_offset bleibt 0 (1 Zeile passt)",
                  lv.scroll_offset, 0);

        mix_expanded[1] = 1;                                       /* jetzt aufklappen (4 Zeilen) */
        q9_listview_move_ex(&lv, 0, mix_items, mix_expanded);      /* delta==0 -- nur neu ausrichten */
        check_int("selected bleibt unveraendert (delta==0)", lv.selected, 1);
        check_int("nach dem Aufklappen: scroll_offset auf 1 nachgezogen (Item0 faellt raus)",
                  lv.scroll_offset, 1);

        q9_listview_init(&lv, 0, 0, 4, 20, 0);
        q9_listview_move_ex(&lv, 1, mix_items, mix_expanded);
        check_int("move_ex() bei item_count==0 tut nichts -- selected bleibt -1", lv.selected, -1);
    }

    printf("=== q9_listview_render_ex: Pfeil-Symbol, Einrueckung, Trennlinie ===\n");
    {
        static q9_listview_field_t fields_a[] = { {"L1:", "Wert1", Q9_LISTVIEW_FIELD_TEXT}, {"L2:", "Wert2", Q9_LISTVIEW_FIELD_TEXT} };
        static const q9_listview_item_t rx_items[] = {
            { "Fest",     NULL,     0 },                    /* nicht erweiterbar             */
            { "Klappbar", fields_a, 2 },                    /* erweiterbar, hier AUFGEKLAPPT  */
        };
        int rx_expanded[2] = { 0, 1 };

        q9_screenbuf_init(&sb, 24, 80);
        q9_listview_init(&lv, 5, 10, 6, 30, 2);
        q9_listview_render_ex(&lv, &sb, rx_items, rx_expanded,
                               200, 200, 200, 0, 0, 0, 255, 255, 0, 77, 88, 99, 44, 55, 66,
                               111, 122, 133,
                               200, 210, 220, 230, 240, 250);

        check_int("Item0 (nicht erweiterbar): Leerzeichen statt Pfeil-Symbol an (5,10)",
                  sb.cell[5][10].ch, ' ');
        check_int("Item0: Name beginnt an Spalte 12 (col+2)", sb.cell[5][12].ch, 'F');
        /* Item0 ist nach q9_listview_init() automatisch ausgewaehlt (selected==0) UND field_focus
           startet bei -1 -- bekommt daher ganz normal sel_bg (header_is_focused), NICHT exp_bg. */
        check_true("Item0 (ausgewaehlt, nicht erweiterbar): normaler sel_bg-Hintergrund",
                   sb.cell[5][10].has_bg == 1 && sb.cell[5][10].bg_g == 255);
        check_int("Item1 (aufgeklappt): Q9_GLYPH_DOWN_ARROW an (6,10)",
                  sb.cell[6][10].ch, Q9_GLYPH_DOWN_ARROW);
        check_int("Item1: Name beginnt an Spalte 12", sb.cell[6][12].ch, 'K');
        check_true("Item1 (aufgeklappt, NICHT ausgewaehlt): Kopfzeile hat exp_bg als Hintergrund "
                   "(Andreas' Feedback, 2026-08-18: \"die Headerzeile geht ein wenig unter\")",
                   sb.cell[6][10].has_bg == 1 && sb.cell[6][10].bg_r == 111);
        check_int("Feld 1 Label eingerueckt (Spalte 12), Zeile 7", sb.cell[7][12].ch, 'L');
        check_int("Feld 1 Wert an fester Spalte (12+16=28), Zeile 7", sb.cell[7][28].ch, 'W');
        check_true("Feldzeile (nicht fokussiert) hat detail_fg (44), nicht die normale fg (200)",
                   sb.cell[7][12].fg_r == 44);
        check_true("Feldzeile (nicht fokussiert) hat KEINEN eigenen Hintergrund",
                   sb.cell[7][12].has_bg == 0);
        check_int("Feld 2 Label, Zeile 8", sb.cell[8][12].ch, 'L');
        check_int("Trennlinie danach (Zeile 9): Q9_GLYPH_HLINE", sb.cell[9][10].ch, Q9_GLYPH_HLINE);
        check_true("Trennlinie hat line_fg (77), nicht detail_fg", sb.cell[9][10].fg_r == 77);
    }
    {
        /* Ausgewaehlt UND aufgeklappt, field_focus==-1 (Kopfzeile) -- sel_bg gewinnt, NICHT exp_bg. */
        static q9_listview_field_t fields_a[] = { {"L1:", "Wert1", Q9_LISTVIEW_FIELD_TEXT}, {"L2:", "Wert2", Q9_LISTVIEW_FIELD_TEXT} };
        static const q9_listview_item_t rx_items[] = { { "Klappbar", fields_a, 2 } };
        int rx_expanded[1] = { 1 };

        q9_screenbuf_init(&sb, 24, 80);
        q9_listview_init(&lv, 5, 10, 6, 30, 1);                 /* selected startet bei 0 -- ausgewaehlt */
        q9_listview_render_ex(&lv, &sb, rx_items, rx_expanded,
                               200, 200, 200, 0, 0, 0, 255, 255, 0, 77, 88, 99, 44, 55, 66,
                               111, 122, 133,
                               200, 210, 220, 230, 240, 250);
        check_true("ausgewaehlt UND aufgeklappt (field_focus==-1): sel_bg (255) gewinnt, nicht exp_bg",
                   sb.cell[5][10].has_bg == 1 && sb.cell[5][10].bg_g == 255);
    }
    {
        /* field_focus>=0 -- die fokussierte Feldzeile bekommt sel_fg/sel_bg, die Kopfzeile faellt
           TROTZ Auswahl auf exp_bg zurueck (Andreas' Wunsch, achtzehnte Runde: Feld-Navigation). */
        static q9_listview_field_t fields_a[] = { {"L1:", "Wert1", Q9_LISTVIEW_FIELD_TEXT}, {"L2:", "Wert2", Q9_LISTVIEW_FIELD_TEXT} };
        static const q9_listview_item_t rx_items[] = { { "Klappbar", fields_a, 2 } };
        int rx_expanded[1] = { 1 };

        q9_screenbuf_init(&sb, 24, 80);
        q9_listview_init(&lv, 5, 10, 6, 30, 1);
        lv.field_focus = 1;                                     /* zweites Feld fokussiert */
        q9_listview_render_ex(&lv, &sb, rx_items, rx_expanded,
                               200, 200, 200, 0, 0, 0, 255, 255, 0, 77, 88, 99, 44, 55, 66,
                               111, 122, 133,
                               200, 210, 220, 230, 240, 250);
        check_true("Kopfzeile faellt bei field_focus>=0 auf exp_bg zurueck (nicht sel_bg)",
                   sb.cell[5][10].has_bg == 1 && sb.cell[5][10].bg_r == 111);
        check_true("Feld 0 (nicht fokussiert): detail_fg, kein eigener Hintergrund",
                   sb.cell[6][12].fg_r == 44 && sb.cell[6][12].has_bg == 0);
        check_true("Feld 1 (fokussiert): sel_bg als Hintergrund", sb.cell[7][10].has_bg == 1
                   && sb.cell[7][10].bg_g == 255);
        check_true("Feld 1 (fokussiert): sel_fg als Textfarbe", sb.cell[7][12].fg_r == 0);
    }

    printf("=== q9_listview_render_ex: TEXT+BUTTON-Paar (Box + dreizeiliger Button, \"wie im "
           "Dialog\", Andreas' Wunsch 2026-08-18) ===\n");
    {
        static q9_listview_field_t pair_fields[] = {
            {"Datei:", "64K", Q9_LISTVIEW_FIELD_TEXT},
            {"",       "Datei", Q9_LISTVIEW_FIELD_BUTTON},
        };
        static const q9_listview_item_t pair_items[] = { { "Konfiguration", pair_fields, 2 } };
        int pair_expanded[1] = { 1 };

        q9_screenbuf_init(&sb, 24, 80);
        q9_listview_init(&lv, 5, 10, 6, 30, 1);
        q9_listview_render_ex(&lv, &sb, pair_items, pair_expanded,
                               200, 200, 200, 0, 0, 0, 255, 255, 0, 77, 88, 99, 44, 55, 66,
                               111, 122, 133, 200, 210, 220, 230, 240, 250);

        /* Zeile 5 = Kopf, 6 = Kappe oben, 7 = gemeinsame Zeile, 8 = Kappe unten, 9 = Trennlinie
           (Button-Spalte = col+2+VALUE_COL(16)+VALUE_BOX_WIDTH(35)+GAP(3) = 10+2+16+35+3 = 66 --
           VALUE_BOX_WIDTH seit der zweiundzwanzigsten Runde 35 statt 20, Andreas: "im Dialog sind
           es ca. 35 Zeichen"). */
        check_int("Kappe oben (Zeile 6): Q9_GLYPH_LOWER_HALF im Button-Bereich (Spalte 66)",
                  sb.cell[6][66].ch, Q9_GLYPH_LOWER_HALF);
        check_true("Kappe oben: nichts ausserhalb des Button-Bereichs (Spalte 40)",
                   sb.cell[6][40].ch == ' ');
        check_int("Label 'Datei:' an Spalte 12 (Zeile 7)", sb.cell[7][12].ch, 'D');
        check_int("Box-Wert '64K' an fester Spalte (12+16=28)", sb.cell[7][28].ch, '6');
        check_true("Box hat box_fg/bg (200/230), NICHT detail_fg (44)",
                   sb.cell[7][28].fg_r == 200 && sb.cell[7][28].has_bg == 1 && sb.cell[7][28].bg_r == 230);
        check_true("Box-Flaeche hat feste Breite -- Spalte 40 (hinter dem Wert) noch im Kasten",
                   sb.cell[7][40].has_bg == 1 && sb.cell[7][40].bg_r == 230);
        check_int("Button-Text 'Datei' zentriert im Button-Bereich (Spalte 67 = 66+1)",
                  sb.cell[7][67].ch, 'D');
        check_true("Button (nicht fokussiert) hat box_fg/bg, wie im Dialog",
                   sb.cell[7][66].fg_r == 200 && sb.cell[7][66].bg_r == 230);
        check_int("Kappe unten (Zeile 8): Q9_GLYPH_UPPER_HALF im Button-Bereich",
                  sb.cell[8][66].ch, Q9_GLYPH_UPPER_HALF);
        check_int("Trennlinie danach auf Zeile 9 (5+1+3)", sb.cell[9][10].ch, Q9_GLYPH_HLINE);
    }
    {
        /* Fokussierter Button innerhalb des Paars -- sel_fg/bg statt box_fg/bg. */
        static q9_listview_field_t pair_fields[] = {
            {"Datei:", "64K", Q9_LISTVIEW_FIELD_TEXT},
            {"",       "Datei", Q9_LISTVIEW_FIELD_BUTTON},
        };
        static const q9_listview_item_t pair_items[] = { { "Konfiguration", pair_fields, 2 } };
        int pair_expanded[1] = { 1 };

        q9_screenbuf_init(&sb, 24, 80);
        q9_listview_init(&lv, 5, 10, 6, 30, 1);
        lv.field_focus = 1;                                     /* Button fokussiert */
        q9_listview_render_ex(&lv, &sb, pair_items, pair_expanded,
                               200, 200, 200, 0, 0, 0, 255, 255, 0, 77, 88, 99, 44, 55, 66,
                               111, 122, 133, 200, 210, 220, 230, 240, 250);
        check_true("fokussierter Button: sel_fg (0)/sel_bg (255) statt box_fg/bg",
                   sb.cell[7][66].fg_r == 0 && sb.cell[7][66].bg_g == 255);
    }
    {
        /* Zugeklappt: RIGHT_ARROW statt DOWN_ARROW, keine Felder/Trennlinie. */
        static q9_listview_field_t fields_a[] = { {"L1:", "Wert1", Q9_LISTVIEW_FIELD_TEXT}, {"L2:", "Wert2", Q9_LISTVIEW_FIELD_TEXT} };
        static const q9_listview_item_t rx_items[] = {
            { "Klappbar", fields_a, 2 },
        };
        int rx_expanded[1] = { 0 };

        q9_screenbuf_init(&sb, 24, 80);
        q9_listview_init(&lv, 5, 10, 6, 30, 1);
        q9_listview_render_ex(&lv, &sb, rx_items, rx_expanded,
                               200, 200, 200, 0, 0, 0, 255, 255, 0, 77, 88, 99, 44, 55, 66,
                               111, 122, 133,
                               200, 210, 220, 230, 240, 250);
        check_int("zugeklappt: Q9_GLYPH_RIGHT_ARROW an (5,10)", sb.cell[5][10].ch, Q9_GLYPH_RIGHT_ARROW);
        check_int("zugeklappt: kein Feld sichtbar -- Zeile 6 bleibt leer", sb.cell[6][10].ch, ' ');
    }
    {
        /* Eintrag laeuft ueber das Fensterende hinaus -- wird abgeschnitten, kein Absturz. */
        static q9_listview_field_t fields_c[] = { {"L1:", "1", Q9_LISTVIEW_FIELD_TEXT}, {"L2:", "2", Q9_LISTVIEW_FIELD_TEXT}, {"L3:", "3", Q9_LISTVIEW_FIELD_TEXT},
                                                   {"L4:", "4", Q9_LISTVIEW_FIELD_TEXT}, {"L5:", "5", Q9_LISTVIEW_FIELD_TEXT} };
        static const q9_listview_item_t rx_items[] = {
            { "Gross", fields_c, 5 },
        };
        int rx_expanded[1] = { 1 };

        q9_screenbuf_init(&sb, 24, 80);
        q9_listview_init(&lv, 0, 0, 3, 30, 1);                  /* Hoehe 3 -- Eintrag braucht 7 */
        q9_listview_render_ex(&lv, &sb, rx_items, rx_expanded,
                               1, 1, 1, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1);
        check_true("kein Absturz bei ueberlangem aufgeklapptem Eintrag", 1);
    }

    printf("=== q9_listview_render_ex: Randfaelle (NULL-Zeiger), kein Absturz ===\n");
    {
        static const q9_listview_item_t rx_items[] = { { "X", NULL, 0 } };
        int rx_expanded[1] = { 0 };
        q9_listview_render_ex(NULL, &sb, rx_items, rx_expanded, 1,1,1, 0,0,0, 1,1,1, 1,1,1, 1,1,1, 1,1,1, 1,1,1, 1,1,1);
        q9_listview_render_ex(&lv, NULL, rx_items, rx_expanded, 1,1,1, 0,0,0, 1,1,1, 1,1,1, 1,1,1, 1,1,1, 1,1,1, 1,1,1);
        q9_listview_render_ex(&lv, &sb, NULL, rx_expanded, 1,1,1, 0,0,0, 1,1,1, 1,1,1, 1,1,1, 1,1,1, 1,1,1, 1,1,1);
        check_true("kein Absturz bis hierher", 1);
    }

    printf("=== q9_listview_field_enter/_leave/_escape: Navigation hinein/heraus ===\n");
    {
        static q9_listview_field_t fields_a[] = { {"L1:", "V1", Q9_LISTVIEW_FIELD_TEXT}, {"L2:", "V2", Q9_LISTVIEW_FIELD_TEXT} };
        static const q9_listview_item_t nav_items[] = {
            { "Item0", NULL,     0 },                       /* nicht erweiterbar */
            { "Item1", fields_a, 2 },
        };
        int nav_expanded[2] = { 0, 0 };

        q9_listview_init(&lv, 0, 0, 4, 20, 2);
        q9_listview_move_ex(&lv, 1, nav_items, nav_expanded);       /* Item1 auswaehlen */
        check_int("field_focus startet bei -1 (Item-Ebene)", lv.field_focus, -1);

        q9_listview_field_enter(&lv, nav_expanded, nav_items);
        check_int("field_enter: field_focus auf 0 (erstes Feld)", lv.field_focus, 0);
        check_int("field_enter: Eintrag automatisch aufgeklappt", nav_expanded[1], 1);

        q9_listview_field_leave(&lv);
        check_int("field_leave: field_focus zurueck auf -1", lv.field_focus, -1);
        check_int("field_leave: Eintrag BLEIBT aufgeklappt", nav_expanded[1], 1);

        q9_listview_field_enter(&lv, nav_expanded, nav_items);
        q9_listview_field_escape(&lv, nav_expanded);
        check_int("field_escape: field_focus zurueck auf -1", lv.field_focus, -1);
        check_int("field_escape: Eintrag WIRD zusaetzlich zugeklappt (Andreas: \"ESC schliesst\")",
                  nav_expanded[1], 0);

        /* field_enter auf einem NICHT erweiterbaren Eintrag tut nichts. */
        q9_listview_move_ex(&lv, -1, nav_items, nav_expanded);      /* zurueck auf Item0 */
        q9_listview_field_enter(&lv, nav_expanded, nav_items);
        check_int("field_enter auf Eintrag ohne Felder: field_focus bleibt -1", lv.field_focus, -1);
    }

    printf("=== q9_listview_field_move: nur INNERHALB der Felder des Eintrags ===\n");
    {
        static q9_listview_field_t fields_a[] = { {"L1:", "V1", Q9_LISTVIEW_FIELD_TEXT}, {"L2:", "V2", Q9_LISTVIEW_FIELD_TEXT}, {"L3:", "V3", Q9_LISTVIEW_FIELD_TEXT} };
        static const q9_listview_item_t nav_items[] = { { "Item0", fields_a, 3 } };
        int nav_expanded[1] = { 0 };

        q9_listview_init(&lv, 0, 0, 4, 20, 1);
        q9_listview_field_enter(&lv, nav_expanded, nav_items);
        check_int("field_focus startet bei 0", lv.field_focus, 0);
        q9_listview_field_move(&lv, 1, nav_items);
        check_int("field_move(+1): field_focus auf 1", lv.field_focus, 1);
        q9_listview_field_move(&lv, 1, nav_items);
        check_int("field_move(+1): field_focus auf 2 (letztes Feld)", lv.field_focus, 2);
        q9_listview_field_move(&lv, 1, nav_items);
        check_int("field_move(+1) am Ende: field_focus geklemmt auf 2", lv.field_focus, 2);
        q9_listview_field_move(&lv, -100, nav_items);
        check_int("field_move(-100) weit ueber den Anfang: field_focus geklemmt auf 0",
                  lv.field_focus, 0);

        q9_listview_field_leave(&lv);
        q9_listview_field_move(&lv, 1, nav_items);
        check_int("field_move() ohne aktiven Feld-Fokus: tut nichts, bleibt -1", lv.field_focus, -1);
    }

    printf("=== q9_listview_field_putc/_backspace: direkte Wert-Bearbeitung ===\n");
    {
        static q9_listview_field_t fields_a[] = { {"L1:", "", Q9_LISTVIEW_FIELD_TEXT}, {"L2:", "xy", Q9_LISTVIEW_FIELD_TEXT} };
        static const q9_listview_item_t nav_items[] = { { "Item0", fields_a, 2 } };
        int nav_expanded[1] = { 0 };

        q9_listview_init(&lv, 0, 0, 4, 20, 1);
        q9_listview_field_enter(&lv, nav_expanded, nav_items);       /* field_focus == 0 */

        q9_listview_field_putc(&lv, nav_items, 'a');
        q9_listview_field_putc(&lv, nav_items, 'b');
        check_true("putc haengt Zeichen an", strcmp(fields_a[0].value, "ab") == 0);

        q9_listview_field_backspace(&lv, nav_items);
        check_true("backspace entfernt letztes Zeichen", strcmp(fields_a[0].value, "a") == 0);

        q9_listview_field_backspace(&lv, nav_items);
        q9_listview_field_backspace(&lv, nav_items);                 /* bei leerem Wert: kein Effekt */
        check_true("backspace bei leerem Wert: kein Absturz, bleibt leer",
                   strcmp(fields_a[0].value, "") == 0);

        q9_listview_field_move(&lv, 1, nav_items);                   /* Feld 1 fokussieren */
        q9_listview_field_putc(&lv, nav_items, 'Z');
        check_true("putc wirkt auf das RICHTIGE (fokussierte) Feld -- Feld 0 bleibt unveraendert",
                   strcmp(fields_a[0].value, "") == 0 && strcmp(fields_a[1].value, "xyZ") == 0);

        q9_listview_field_leave(&lv);
        q9_listview_field_putc(&lv, nav_items, 'Q');
        check_true("putc ohne aktiven Feld-Fokus: kein Effekt", strcmp(fields_a[1].value, "xyZ") == 0);
    }

    printf("=== q9_listview_field_putc: Puffer laeuft nicht ueber ===\n");
    {
        static q9_listview_field_t fields_a[] = { {"L1:", "", Q9_LISTVIEW_FIELD_TEXT} };
        static const q9_listview_item_t nav_items[] = { { "Item0", fields_a, 1 } };
        int nav_expanded[1] = { 0 };
        int i;

        q9_listview_init(&lv, 0, 0, 4, 20, 1);
        q9_listview_field_enter(&lv, nav_expanded, nav_items);
        for (i = 0; i < Q9_LISTVIEW_FIELD_VALUE_MAX + 5; i++) {
            q9_listview_field_putc(&lv, nav_items, 'x');
        }
        check_int("Wert bleibt auf Q9_LISTVIEW_FIELD_VALUE_MAX-1 Zeichen begrenzt",
                  (int)strlen(fields_a[0].value), Q9_LISTVIEW_FIELD_VALUE_MAX - 1);
    }

    printf("=== q9_listview_field_putc/_backspace: BUTTON-Feld ignoriert Tippen (Andreas' Wunsch, "
           "2026-08-18: \"dahinter ein Button um den Dialog zu oeffnen\") ===\n");
    {
        static q9_listview_field_t fields_a[] = {
            {"Datei:", "urspruenglich", Q9_LISTVIEW_FIELD_TEXT},
            {"",       "[ Oeffnen... ]", Q9_LISTVIEW_FIELD_BUTTON},
        };
        static const q9_listview_item_t nav_items[] = { { "Item0", fields_a, 2 } };
        int nav_expanded[1] = { 0 };

        q9_listview_init(&lv, 0, 0, 4, 20, 1);
        q9_listview_field_enter(&lv, nav_expanded, nav_items);       /* field_focus == 0 (TEXT) */
        check_int("TEXT-Feld: normales Verhalten (Kontrolle)", lv.field_focus, 0);
        q9_listview_field_putc(&lv, nav_items, 'X');
        check_true("TEXT-Feld: putc wirkt normal", strcmp(fields_a[0].value, "urspruenglichX") == 0);

        q9_listview_field_move(&lv, 1, nav_items);                   /* field_focus == 1 (BUTTON) */
        q9_listview_field_putc(&lv, nav_items, 'Y');
        check_true("BUTTON-Feld: putc tut nichts -- value bleibt unveraendert",
                   strcmp(fields_a[1].value, "[ Oeffnen... ]") == 0);
        q9_listview_field_backspace(&lv, nav_items);
        check_true("BUTTON-Feld: backspace tut nichts -- value bleibt unveraendert",
                   strcmp(fields_a[1].value, "[ Oeffnen... ]") == 0);
    }

    printf("=== q9_listview_field_putc: NUMERIC_DEC/_HEX filtern die Zeichenklasse (Andreas' "
           "Wunsch: \"Numerische Eingabe Dezimal/Hex opt. mit Bereich\") ===\n");
    {
        static q9_listview_field_t fields_a[] = {
            {"Dez:", "1", Q9_LISTVIEW_FIELD_NUMERIC_DEC},
            {"Hex:", "1", Q9_LISTVIEW_FIELD_NUMERIC_HEX},
        };
        static const q9_listview_item_t nav_items[] = { { "Item0", fields_a, 2 } };
        int nav_expanded[1] = { 0 };

        q9_listview_init(&lv, 0, 0, 4, 20, 1);
        q9_listview_field_enter(&lv, nav_expanded, nav_items);        /* field_focus == 0 (DEZ) */
        q9_listview_field_putc(&lv, nav_items, '2');
        check_true("NUMERIC_DEC: Ziffer wird akzeptiert", strcmp(fields_a[0].value, "12") == 0);
        q9_listview_field_putc(&lv, nav_items, 'a');
        check_true("NUMERIC_DEC: Buchstabe wird verworfen (auch Hex-Ziffern wie 'a')",
                   strcmp(fields_a[0].value, "12") == 0);
        q9_listview_field_putc(&lv, nav_items, ' ');
        check_true("NUMERIC_DEC: Leerzeichen wird verworfen", strcmp(fields_a[0].value, "12") == 0);

        q9_listview_field_move(&lv, 1, nav_items);                    /* field_focus == 1 (HEX) */
        q9_listview_field_putc(&lv, nav_items, 'F');
        check_true("NUMERIC_HEX: Grossbuchstabe A-F wird akzeptiert", strcmp(fields_a[1].value, "1F") == 0);
        q9_listview_field_putc(&lv, nav_items, 'c');
        check_true("NUMERIC_HEX: Kleinbuchstabe a-f wird akzeptiert", strcmp(fields_a[1].value, "1Fc") == 0);
        q9_listview_field_putc(&lv, nav_items, 'g');
        check_true("NUMERIC_HEX: 'g' (kein Hex) wird verworfen", strcmp(fields_a[1].value, "1Fc") == 0);
        q9_listview_field_putc(&lv, nav_items, '9');
        check_true("NUMERIC_HEX: Ziffer wird ebenfalls akzeptiert", strcmp(fields_a[1].value, "1Fc9") == 0);
    }

    printf("=== q9_listview_render_ex: NUMERIC_HEX zeigt automatisch \"$\" vor dem Wert ===\n");
    {
        static q9_listview_field_t fields_a[] = {
            {"Basis:", "FFFFE000", Q9_LISTVIEW_FIELD_NUMERIC_HEX},
            {"Slot:",  "5",        Q9_LISTVIEW_FIELD_NUMERIC_DEC},
        };
        static const q9_listview_item_t rx_items[] = { { "Item0", fields_a, 2 } };
        int rx_expanded[1] = { 1 };

        q9_screenbuf_init(&sb, 24, 80);
        q9_listview_init(&lv, 5, 10, 6, 30, 1);
        q9_listview_render_ex(&lv, &sb, rx_items, rx_expanded,
                               200, 200, 200, 0, 0, 0, 255, 255, 0, 77, 88, 99, 44, 55, 66,
                               111, 122, 133, 200, 210, 220, 230, 240, 250);
        /* Zeile 6 = erstes Feld (Zeile 5 ist die Kopfzeile). Wert-Spalte = col+2+VALUE_COL = 28. */
        check_int("NUMERIC_HEX: \"$\" an der Wert-Spalte (28)", sb.cell[6][28].ch, '$');
        check_int("NUMERIC_HEX: Wert beginnt eine Spalte weiter (29)", sb.cell[6][29].ch, 'F');
        check_int("NUMERIC_DEC (Zeile 7): KEIN \"$\" -- Wert direkt an Spalte 28", sb.cell[7][28].ch, '5');
    }

    printf("=== q9_listview_field_toggle: schaltet BOOLEAN-Felder um (Andreas' Wunsch: \"Boolean "
           "Eingabe\") ===\n");
    {
        static q9_listview_field_t fields_a[] = {
            {"Aktiv:", "ja",      Q9_LISTVIEW_FIELD_BOOLEAN},
            {"Bus:",   "onboard", Q9_LISTVIEW_FIELD_TEXT},
            {"Link:",  "seltsam", Q9_LISTVIEW_FIELD_BOOLEAN},
        };
        static const q9_listview_item_t nav_items[] = { { "Item0", fields_a, 3 } };
        int nav_expanded[1] = { 0 };

        q9_listview_init(&lv, 0, 0, 4, 20, 1);
        q9_listview_field_enter(&lv, nav_expanded, nav_items);        /* field_focus == 0 (Aktiv:) */
        q9_listview_field_toggle(&lv, nav_items);
        check_true("BOOLEAN: \"ja\" wird zu \"nein\"", strcmp(fields_a[0].value, "nein") == 0);
        q9_listview_field_toggle(&lv, nav_items);
        check_true("BOOLEAN: \"nein\" wird wieder zu \"ja\"", strcmp(fields_a[0].value, "ja") == 0);

        q9_listview_field_putc(&lv, nav_items, 'x');
        check_true("BOOLEAN-Feld: putc tut nichts -- value bleibt unveraendert",
                   strcmp(fields_a[0].value, "ja") == 0);
        q9_listview_field_backspace(&lv, nav_items);
        check_true("BOOLEAN-Feld: backspace tut nichts -- value bleibt unveraendert",
                   strcmp(fields_a[0].value, "ja") == 0);

        q9_listview_field_move(&lv, 1, nav_items);                    /* field_focus == 1 (TEXT) */
        q9_listview_field_toggle(&lv, nav_items);
        check_true("TEXT-Feld: toggle tut nichts -- value bleibt unveraendert",
                   strcmp(fields_a[1].value, "onboard") == 0);

        q9_listview_field_move(&lv, 1, nav_items);                    /* field_focus == 2 (Link:) */
        check_true("Ausgangswert weder \"ja\" noch \"nein\" (Kontrolle)",
                   strcmp(fields_a[2].value, "seltsam") == 0);
        q9_listview_field_toggle(&lv, nav_items);
        check_true("BOOLEAN: unerwarteter Ausgangswert wird zu \"ja\"",
                   strcmp(fields_a[2].value, "ja") == 0);
    }

    printf("=== q9_listview_field_*: Randfaelle (NULL-Zeiger), kein Absturz ===\n");
    {
        static q9_listview_field_t fields_a[] = { {"L1:", "V1", Q9_LISTVIEW_FIELD_TEXT} };
        static const q9_listview_item_t nav_items[] = { { "Item0", fields_a, 1 } };
        int nav_expanded[1] = { 0 };

        q9_listview_field_enter(NULL, nav_expanded, nav_items);
        q9_listview_field_enter(&lv, nav_expanded, NULL);
        q9_listview_field_leave(NULL);
        q9_listview_field_escape(NULL, nav_expanded);
        q9_listview_field_move(NULL, 1, nav_items);
        q9_listview_field_move(&lv, 1, NULL);
        q9_listview_field_putc(NULL, nav_items, 'x');
        q9_listview_field_putc(&lv, NULL, 'x');
        q9_listview_field_backspace(NULL, nav_items);
        q9_listview_field_backspace(&lv, NULL);
        q9_listview_field_toggle(NULL, nav_items);
        q9_listview_field_toggle(&lv, NULL);
        check_true("kein Absturz bis hierher", 1);
    }

    printf("\n=== Zusammenfassung ===\n");
    printf("  Gesamt: %d Checks fehlgeschlagen\n", g_fails);
    return g_fails ? 1 : 0;
}

//────────────────────────────────────────────────────────────────────────────────────────────────
// EOF listview_selftest.c                                                                 Ver. 2.00
//────────────────────────────────────────────────────────────────────────────────────────────────
