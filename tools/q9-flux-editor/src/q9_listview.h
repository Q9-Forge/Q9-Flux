//════════════════════════════════════════════════════════════════════════════════════════════════
// File:   q9_listview.h                                                                   Ver. 1.20
// Owner:  Claudia
// Desc.:  Scrollbare Listenansicht auf q9_screenbuf.h aufgesetzt -- Andreas' Frage (2026-08-16):
//         "Könnte man einen Bereich Scrollbar machen?" fuer den Config-Startbildschirm (mehr Felder/
//         Hardware-Eintraege als in den sichtbaren Bereich zwischen Titel und den START/SAVE/EXIT-
//         Buttons passen).
//
//         Aufgeteilt in eine REINE Kernfunktion (q9_listview_scroll -- Index rein, neuer Scroll-
//         Offset raus, kein Bildschirmpuffer noetig, testbar wie q9_input_decode) und die eigent-
//         liche Zeichenroutine (q9_listview_render, schreibt in einen q9_screenbuf_t wie
//         q9_widgets.h). q9_listview_t selbst ist bewusst NUR Buchhaltung (Position/Groesse/
//         Auswahl/Scroll-Offset) -- die Listeninhalte selbst (Zeilen-Strings) bleiben beim Aufrufer,
//         genau wie q9_screenbuf keine eigenen Farben "kennt", sondern nur speichert, was man ihm
//         gibt.
//
// Call:   q9_listview_t lv;
//         q9_listview_init(&lv, 5, 10, 8, 40, item_count);   // Viewport: Zeile 5, Spalte 10, 8x40
//         q9_listview_move(&lv, +1);                          // Auswahl eine Zeile runter (Pfeil ab)
//         q9_listview_render(&lv, &sb, items, 255,255,255, 0,0,0, 255,255,0);
//
// Edition History
//─────────┬──────┬────────────────────────────────────────────────────────────────────────┬──────
// Date    │ Ver. │ Description                                                            │ By
//─────────┼──────┼────────────────────────────────────────────────────────────────────────┬──────
// 26-08-16│ 1.00 │ Erster Wurf -- q9_listview_scroll (Kern), render (inkl. Scrollbalken)   │ Cld
// 26-08-17│ 1.10 │ Scrollbalken-Griff jetzt proportional zum sichtbaren Anteil (Andreas'    │ Cld
//         │      │ Wunsch), echte Unicode-Zeichen (Q9_GLYPH_VLINE/BLOCK) statt ASCII '|'/'#' │
// 26-08-17│ 1.20 │ Rechte Spalte zeigt jetzt IMMER die Linie, auch ohne Scrollbedarf (Andreas'│ Cld
//         │      │ Wunsch) -- gilt fuer Hauptfenster UND Datei-Dialog gleichermassen          │
//═════════╧══════╧════════════════════════════════════════════════════════════════════════╧══════
#ifndef Q9_LISTVIEW_H
#define Q9_LISTVIEW_H

#include "q9_screenbuf.h"

typedef struct {
    int row, col;                                       /* Position des Viewports im Bildschirmpuffer */
    int height, width;                                   /* Groesse des Viewports (Zeilen/Spalten)     */
    int item_count;                                       /* Gesamtzahl der Listeneintraege             */
    int selected;                                          /* Index des ausgewaehlten Eintrags, -1 wenn
                                                             item_count==0 (nichts auswaehlbar)          */
    int scroll_offset;                                     /* Index des ERSTEN sichtbaren Eintrags       */
} q9_listview_t;

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_listview_init
// Desc.:    Setzt Viewport-Geometrie und Eintragsanzahl, selected/scroll_offset auf 0 (bzw. selected
//           = -1 bei item_count==0). height/width < 1 werden auf 1 angehoben (ein Viewport der
//           Groesse 0 ergibt keinen Sinn und wuerde die Divisionen in q9_listview_scroll gefaehrden).
// Call:     q9_listview_init(&lv, 5, 10, 8, 40, 12)
//════════════════════════════════════════════════════════════════════════════════════════════════
void q9_listview_init(q9_listview_t *lv, int row, int col, int height, int width, int item_count);

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_listview_scroll
// Desc.:    REINE Funktion (kein Bildschirmpuffer): berechnet den neuen Scroll-Offset, damit
//           `selected` (auf [0, item_count-1] geklemmt) innerhalb des Sichtfensters
//           [offset, offset+height-1] bleibt -- "Auswahl bleibt immer sichtbar". Scrollt NIE weiter,
//           als noetig (Offset bleibt in [0, max(0, item_count-height)]). item_count<=0 oder
//           height<=0 liefert immer 0.
// Call:     new_offset = q9_listview_scroll(selected, offset, height, item_count)
//════════════════════════════════════════════════════════════════════════════════════════════════
int q9_listview_scroll(int selected, int offset, int height, int item_count);

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_listview_move
// Desc.:    Bewegt die Auswahl um delta (z.B. +1 fuer Pfeil runter, -1 fuer Pfeil hoch), klemmt auf
//           [0, item_count-1], und zieht scroll_offset ueber q9_listview_scroll automatisch nach.
//           Tut nichts bei item_count==0.
// Call:     q9_listview_move(&lv, +1)
//════════════════════════════════════════════════════════════════════════════════════════════════
void q9_listview_move(q9_listview_t *lv, int delta);

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_listview_render
// Desc.:    Zeichnet die sichtbaren Eintraege (items[scroll_offset..scroll_offset+height-1], soweit
//           vorhanden -- ueberschuessige Viewport-Zeilen bleiben leer) in sb. Die Zeile des
//           ausgewaehlten Eintrags bekommt sel_fg/sel_bg (Hervorhebung), alle anderen fg auf
//           schwarzem/Standard-Hintergrund (kein eigener Hintergrund -- Aufrufer kann vorher selbst
//           fill_rect fuer eine Flaeche aufrufen). Zeilen werden an der Viewport-Breite abgeschnitten
//           (kein Umbruch). items muss mindestens item_count Eintraege haben (Aufrufer-Verantwortung,
//           wie bei allen anderen q9_*-Funktionen hier kein Bounds-Tracking ueber die Puffergrenze
//           von sb selbst hinaus noetig -- q9_screenbuf_puts klemmt ohnehin am Bildschirmpuffer).
//           Die Spalte col+width-1 ist IMMER fuer die Bildlaufleiste reserviert (Andreas' Wunsch,
//           2026-08-17: "wird keine Laufleiste benoetigt ist es einfach der normale Strich") --
//           zeigt bei item_count<=height eine durchgehende Linie (Q9_GLYPH_VLINE), sonst zusaetzlich
//           den Griff (Q9_GLYPH_BLOCK) darauf. Der Griff ist PROPORTIONAL zum sichtbaren Anteil
//           (height/item_count, z.B. 50% sichtbar -> Griff nimmt 50% der Balkenhoehe ein), mindestens
//           1 Zeile, hoechstens height-1 (damit immer sichtbar bleibt, DASS es ueberhaupt etwas zu
//           scrollen gibt). content_width ist entsprechend IMMER width-1 (nicht mehr bedingt).
// Call:     q9_listview_render(&lv, &sb, items, 255,255,255, 0,0,0, 255,255,0)
//════════════════════════════════════════════════════════════════════════════════════════════════
void q9_listview_render(const q9_listview_t *lv, q9_screenbuf_t *sb, const char *const *items,
                         int fg_r, int fg_g, int fg_b,
                         int sel_fg_r, int sel_fg_g, int sel_fg_b,
                         int sel_bg_r, int sel_bg_g, int sel_bg_b);

#endif /* Q9_LISTVIEW_H */
//────────────────────────────────────────────────────────────────────────────────────────────────
// EOF q9_listview.h                                                                       Ver. 1.20
//────────────────────────────────────────────────────────────────────────────────────────────────
