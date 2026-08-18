//════════════════════════════════════════════════════════════════════════════════════════════════
// File:   q9_listview.h                                                                   Ver. 1.50
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
//         q9_listview_render(&lv, &sb, items, 255,255,255, 0,0,0, 255,255,0, 200,200,200);
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
// 26-08-17│ 1.30 │ Luftspalte zwischen Inhalt und der Linie dazu (content_width jetzt width-2│ Cld
//         │      │ statt width-1) -- Andreas: "wirkt jetzt doch gequetscht"                   │
// 26-08-17│ 1.40 │ Neuer Parameter line_fg -- Linie/Bildlaufleiste bekommt eine EIGENE Farbe, │ Cld
//         │      │ unabhaengig von der Text-fg (Andreas: "Striche links/rechts unterschiedlich")│
// 26-08-18│ 1.50 │ Erweiterbare Eintraege (Andreas: "groessere Eintraege... minimiert ein/zwei │ Cld
//         │      │ Zeilen, aufgeklappt so viele wie sie brauchen") -- q9_listview_item_t NEU    │
//         │      │ (Name + Detailzeilen), q9_listview_item_rows()/_scroll_ex()/_move_ex()/       │
//         │      │ _render_ex() NEU dazu, bestehende Funktionen UNVERAENDERT (Datei-Dialog nutzt │
//         │      │ weiter die einfachen 1-Zeile-pro-Eintrag-Varianten)                           │
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

/* Fuer die "_ex"-Funktionen (erweiterbare Eintraege, s.u.): ein Eintrag ist jetzt mehr als ein
   blosser String -- er hat eine Kopfzeile (name, IMMER sichtbar) und optional Detailzeilen (nur
   sichtbar, wenn der Eintrag aufgeklappt ist, s. das expanded-Array bei den einzelnen Funktionen).
   detail_lines/detail_count bleiben beim Aufrufer (wie items bei den einfachen Funktionen oben) --
   q9_listview_item_t selbst kopiert nichts. detail_count<=0 bedeutet "nicht erweiterbar" (kein
   Pfeil-Symbol, q9_listview_item_rows() liefert dafuer immer 1, egal was im expanded-Array steht). */
typedef struct {
    const char *name;
    const char *const *detail_lines;
    int detail_count;
} q9_listview_item_t;

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_listview_item_rows
// Desc.:    Wie viele Bildschirmzeilen Eintrag index braucht: 1 (nur die Kopfzeile), wenn er
//           zugeklappt ist ODER detail_count<=0 (nicht erweiterbar) -- sonst 1 (Kopf) +
//           detail_count (Detailzeilen) + 1 (Trennlinie danach). items/expanded duerfen NULL sein
//           (liefert dann immer 1, wie ein ganz normaler Ein-Zeile-Eintrag) -- damit verhalten sich
//           die "_ex"-Funktionen bei NULL/NULL exakt wie ihre einfachen Gegenstuecke oben.
// Call:     int rows = q9_listview_item_rows(items, expanded, 3)
//════════════════════════════════════════════════════════════════════════════════════════════════
int q9_listview_item_rows(const q9_listview_item_t *items, const int *expanded, int index);

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_listview_scroll_ex
// Desc.:    Wie q9_listview_scroll(), aber ROW-bewusst (Eintraege koennen mehr als eine Zeile
//           brauchen, s. q9_listview_item_rows()). Garantiert, dass die KOPFZEILE von `selected`
//           sichtbar bleibt -- bei einem sehr grossen aufgeklappten Eintrag (mehr Detailzeilen als
//           height) wird NICHT versucht, den kompletten Eintrag ins Fenster zu quetschen (das
//           wuerde bei stark unterschiedlichen Eintragsgroessen zu ueberraschenden Spruengen
//           fuehren) -- es reicht, wenn der Kopf oben im Fenster steht, der Rest wird unten
//           abgeschnitten (genau wie ein zu grosser einzelner Eintrag im normalen Textfluss vieler
//           anderer TUI-Listenansichten). BEKANNTE VEREINFACHUNG: anders als q9_listview_scroll()
//           wird NICHT versucht, unnoetigen Leerraum am Fensterende zu vermeiden (kein "so weit wie
//           moeglich zurueckziehen, wenn ohnehin nichts mehr folgt") -- fuer die erste Fassung
//           bewusst weggelassen, bei Bedarf spaeter nachruestbar.
// Call:     new_offset = q9_listview_scroll_ex(selected, offset, height, items, expanded, item_count)
//════════════════════════════════════════════════════════════════════════════════════════════════
int q9_listview_scroll_ex(int selected, int offset, int height,
                           const q9_listview_item_t *items, const int *expanded, int item_count);

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_listview_move_ex
// Desc.:    Wie q9_listview_move(), nutzt aber q9_listview_scroll_ex() zum Nachziehen. delta==0 ist
//           ein gueltiger, gewollter Aufruf: bewegt die Auswahl NICHT, richtet aber scroll_offset neu
//           aus -- genau das braucht der Aufrufer direkt NACH dem Auf-/Zuklappen eines Eintrags
//           (dessen Zeilenzahl sich dadurch aendert, die Auswahl selbst aber gleich bleibt).
// Call:     q9_listview_move_ex(&lv, 0, items, expanded)     // nur neu ausrichten, nicht bewegen
//════════════════════════════════════════════════════════════════════════════════════════════════
void q9_listview_move_ex(q9_listview_t *lv, int delta,
                          const q9_listview_item_t *items, const int *expanded);

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_listview_render_ex
// Desc.:    Wie q9_listview_render(), aber fuer erweiterbare Eintraege (s. q9_listview_item_t).
//           Jede Kopfzeile bekommt ein Pfeil-Symbol davor (Q9_GLYPH_DOWN_ARROW aufgeklappt,
//           Q9_GLYPH_RIGHT_ARROW zugeklappt, ein Leerzeichen bei detail_count<=0 -- nichts zum
//           Auf-/Zuklappen). Aufgeklappte Detailzeilen werden um zwei Spalten eingerueckt (Pfeil +
//           Luftspalte) in EINER EIGENEN Farbe (detail_fg, gedaempft/anders als der normale
//           Eintragstext -- optische Unterscheidung Kopf/Detail), danach eine volle Trennlinie
//           (Q9_GLYPH_HLINE ueber content_width, in line_fg). Bewusst LEICHTGEWICHTIG (Andreas'
//           Wahl, 2026-08-18, aus drei vorgeschlagenen Stilen): KEIN Rahmen um den aufgeklappten
//           Bereich -- nur Einrueckung + die eine Trennlinie danach, spart am meisten Platz. Zeilen,
//           die nicht mehr in den Viewport passen (Eintrag laeuft ueber das Fensterende hinaus),
//           werden abgeschnitten, wie bei render() ueberzaehlige Eintraege. Rechte Spalte (Linie +
//           Bildlaufleiste) jetzt ROW-basiert statt item-basiert -- Griffgroesse/-position richten
//           sich nach der GESAMTZEILENZAHL aller Eintraege (inkl. aufgeklappter), nicht mehr nach
//           der reinen Eintragsanzahl.
// Call:     q9_listview_render_ex(&lv, &sb, items, expanded, 255,255,255, 0,0,0, 255,255,0,
//                                  200,200,200, 150,120,80)
//════════════════════════════════════════════════════════════════════════════════════════════════
void q9_listview_render_ex(const q9_listview_t *lv, q9_screenbuf_t *sb,
                            const q9_listview_item_t *items, const int *expanded,
                            int fg_r, int fg_g, int fg_b,
                            int sel_fg_r, int sel_fg_g, int sel_fg_b,
                            int sel_bg_r, int sel_bg_g, int sel_bg_b,
                            int line_fg_r, int line_fg_g, int line_fg_b,
                            int detail_fg_r, int detail_fg_g, int detail_fg_b);

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
//           scrollen gibt). content_width ist width-2 (Linie + eine Luftspalte davor, Andreas'
//           Wunsch, 2026-08-17, fuenfte Runde: "zum Strich jeweils ein Leerzeichen") -- die Linie
//           selbst bleibt dabei unveraendert bei col+width-1, nur der Inhalt bekommt mehr Abstand.
//           line_fg ist die Farbe der Linie/Bildlaufleiste, UNABHAENGIG von fg (Andreas' Wunsch,
//           sechste Runde: "die Striche links und rechts am Hauptfenster sind unterschiedlich") --
//           vorher erbte die Linie einfach fg (die normale Text-Vordergrundfarbe), was im
//           Hauptfenster nicht zur Rahmenfarbe von q9_widgets.c's draw_frame() passte, obwohl beide
//           Linien auf derselben Bildschirmspalte aufeinandertreffen. Aufrufer, denen das egal ist
//           (z.B. weil fg und die gewuenschte Rahmenfarbe ohnehin gleich sein sollen), koennen
//           einfach denselben Farbwert fuer fg und line_fg uebergeben.
// Call:     q9_listview_render(&lv, &sb, items, 255,255,255, 0,0,0, 255,255,0, 200,200,200)
//════════════════════════════════════════════════════════════════════════════════════════════════
void q9_listview_render(const q9_listview_t *lv, q9_screenbuf_t *sb, const char *const *items,
                         int fg_r, int fg_g, int fg_b,
                         int sel_fg_r, int sel_fg_g, int sel_fg_b,
                         int sel_bg_r, int sel_bg_g, int sel_bg_b,
                         int line_fg_r, int line_fg_g, int line_fg_b);

#endif /* Q9_LISTVIEW_H */
//────────────────────────────────────────────────────────────────────────────────────────────────
// EOF q9_listview.h                                                                       Ver. 1.50
//────────────────────────────────────────────────────────────────────────────────────────────────
