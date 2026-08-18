//════════════════════════════════════════════════════════════════════════════════════════════════
// File:   q9_listview.h                                                                   Ver. 2.20
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
// 26-08-18│ 1.60 │ Neuer Parameter exp_bg an render_ex() -- Kopfzeile eines aufgeklappten, nicht  │ Cld
//         │      │ ausgewaehlten Eintrags bekommt einen eigenen Hintergrund (Andreas: "die        │
//         │      │ Headerzeile geht ein wenig unter")                                             │
// 26-08-18│ 1.70 │ Feld-Navigation (Andreas: "wie komme ich in das item rein um dort Werte zu     │ Cld
//         │      │ aendern?") -- detail_lines/detail_count durch echte, EDITIERBARE Felder ersetzt │
//         │      │ (q9_listview_field_t: label + value), lv->field_focus NEU (-1 = Item-Ebene),    │
//         │      │ q9_listview_field_enter()/_leave()/_escape()/_move()/_putc()/_backspace() NEU    │
// 26-08-18│ 1.90 │ Neuer q9_listview_field_kind_t (TEXT/BUTTON) -- ein BUTTON-Feld ignoriert Tippen │ Cld
//         │      │ (putc/backspace), der Aufrufer erkennt am Typ, dass Enter eine eigene Aktion     │
//         │      │ ausloesen soll (Andreas: "dahinter ein Button um den Dialog zu oeffnen")          │
// 26-08-18│ 2.00 │ TEXT-Feld direkt gefolgt von einem BUTTON-Feld ist jetzt ein SONDERFALL: 3 Zeilen │ Cld
//         │      │ statt 2 (q9_listview_item_rows()), render_ex() zeichnet den Wert in einer         │
//         │      │ eigenen Box (neu: box_fg/bg) UND den Button ECHT wie im Datei-Dialog (Halbblock-  │
//         │      │ Kappen, vertikal zentriert neben dem Textfeld) -- Andreas: "der dreizeilige       │
//         │      │ Button wie im Dialog... zentrisch hinter Datei ausgerichtet"                       │
// 26-08-18│ 2.10 │ Numerische Feldtypen (Andreas: "Numerische Eingabe Dezimal/Hex opt. mit Bereich") │ Cld
//         │      │ -- NUMERIC_DEC/_HEX als NEUE kind-Werte (kein neues Struct-Feld, s. dortiger      │
//         │      │ Kommentar), field_putc() filtert die Zeichenklasse, render_ex() zeigt "$" vor Hex │
// 26-08-18│ 2.20 │ Boolean-Feldtyp (Andreas: "Boolean Eingabe") -- Q9_LISTVIEW_FIELD_BOOLEAN als     │ Cld
//         │      │ weiterer NEUER kind-Wert, neue Funktion field_toggle() (Leertaste schaltet ja/    │
//         │      │ nein um, s. integration_demo.c) -- putc/backspace ignorieren BOOLEAN wie BUTTON   │
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
    /* NUR fuer die "_ex"-Funktionen relevant (s.u.) -- Index des fokussierten FELDES innerhalb des
       AUSGEWAEHLTEN Eintrags, -1 = kein Feld fokussiert (Fokus liegt auf der Kopfzeile/Item-Ebene,
       das bisherige Verhalten). q9_listview_init() setzt dies auf -1; jede item-EBENE-Bewegung
       (q9_listview_move_ex()) setzt es defensiv ebenfalls auf -1 zurueck (Feld-Fokus ergibt bei
       einem ANDEREN Eintrag keinen Sinn), s. q9_listview_field_enter()/_leave()/_escape() fuer die
       eigentliche Navigation hinein/hinaus. */
    int field_focus;
} q9_listview_t;

/* Fuer die "_ex"-Funktionen (erweiterbare, editierbare Eintraege, s.u.): ein Eintrag hat eine
   Kopfzeile (name, IMMER sichtbar) und optional FELDER (nur sichtbar, wenn der Eintrag aufgeklappt
   ist, s. das expanded-Array bei den einzelnen Funktionen). Jedes Feld hat ein Label (konstant) und
   einen Wert (value, MUTABLE -- q9_listview_field_putc()/_backspace() aendern ihn direkt in place,
   "direkte Manipulation" statt separatem Bearbeiten-Bestaetigen-Zyklus, s. dortiger Kommentar).
   fields/field_count bleiben beim Aufrufer (wie items bei den einfachen Funktionen oben) --
   q9_listview_item_t selbst kopiert nichts. field_count<=0 bedeutet "nicht erweiterbar" (kein
   Pfeil-Symbol, q9_listview_item_rows() liefert dafuer immer 1, egal was im expanded-Array steht,
   q9_listview_field_enter() tut dann nichts).
   kind unterscheidet TEXT (Standard, Tippen aendert value direkt) von BUTTON (Andreas' Wunsch,
   2026-08-18, neunzehnte Runde: "dahinter ein button um den Dialog zu oeffnen") -- ein BUTTON-Feld
   ignoriert Tippen (q9_listview_field_putc()/_backspace() tun bei kind!=TEXT nichts), der AUFRUFER
   erkennt am Feld-Typ, dass Enter WAEHREND dieses Feld fokussiert ist stattdessen eine eigene Aktion
   ausloesen soll (die Bibliothek selbst kennt keine Aktionen/Callbacks, bleibt bewusst "nur
   Buchhaltung" -- der Aufrufer prueft `items[selected].fields[field_focus].kind` direkt, s.
   integration_demo.c). Bestehende Initialisierer mit nur zwei Feldern ({label, value}) bleiben
   gueltig -- kind wird dabei automatisch auf 0 = Q9_LISTVIEW_FIELD_TEXT genullt (C99-Aggregat-
   Initialisierung). */
/* NUMERIC_DEC/_HEX (Andreas' Wunsch von Anfang an, 2026-08-18: "Numerische Eingabe Dezimal/Hex
   opt. mit Bereich") -- BEWUSST als zwei neue kind-WERTE statt neuer Struct-Felder (die letzte
   Runde mit dem BUTTON-Feldtyp hat gezeigt, wie muehsam ein neues Struct-Feld ist: ~108
   bestehende {label,value}-Initialisierer mussten nachtraeglich um kind ergaenzt werden, sonst
   -Wextra-Warnungen). q9_listview_field_putc() filtert bei diesen kinds die Zeichenklasse (nur
   Ziffern bzw. nur Hex-Ziffern, s. dort) -- das ist die Kernfunktion "Dezimal/Hex". Die
   BEREICHSPRUEFUNG ("opt. mit Bereich") ist bewusst NICHT Teil der Bibliothek -- welcher Bereich
   fuer welches Feld gilt, ist Anwendungswissen, keine Listenansicht-Zustaendigkeit; der Aufrufer
   prueft/klemmt selbst (z.B. beim Verlassen des Feldes), s. integration_demo.c fuer ein Beispiel
   (Slot: 0-255). NUMERIC_HEX-Werte werden OHNE fuehrendes "$" gespeichert (reine Hex-Ziffern) --
   q9_listview_render_ex() zeichnet das "$" automatisch davor, s. dort. */
/* BOOLEAN (Andreas' Wunsch von Anfang an, 2026-08-18: "Boolean Eingabe") -- wieder ein neuer
   kind-WERT statt neuer Struct-Felder, wie schon bei NUMERIC_DEC/_HEX. Der Wert ist IMMER genau
   "ja" oder "nein" (die Konvention, die schon alle "Aktiv:"-Felder in integration_demo.c
   verwenden). Tippen/Loeschen wirkt bei BOOLEAN-Feldern NICHT (wie bei BUTTON) -- stattdessen
   schaltet q9_listview_field_toggle() zwischen "ja"/"nein" um (Aufrufer bindet das ueblicherweise
   an die Leertaste, s. integration_demo.c). Rendering unveraendert wie TEXT (kein eigenes Symbol
   fuer diese erste Fassung -- bei Bedarf spaeter nachruestbar, z.B. ein Kaestchen-Symbol). */
typedef enum {
    Q9_LISTVIEW_FIELD_TEXT = 0,
    Q9_LISTVIEW_FIELD_BUTTON,
    Q9_LISTVIEW_FIELD_NUMERIC_DEC,
    Q9_LISTVIEW_FIELD_NUMERIC_HEX,
    Q9_LISTVIEW_FIELD_BOOLEAN
} q9_listview_field_kind_t;

#define Q9_LISTVIEW_FIELD_VALUE_MAX 40
typedef struct {
    const char *label;
    char value[Q9_LISTVIEW_FIELD_VALUE_MAX];
    q9_listview_field_kind_t kind;
} q9_listview_field_t;

typedef struct {
    const char *name;
    q9_listview_field_t *fields;                        /* NICHT const -- value ist editierbar */
    int field_count;
} q9_listview_item_t;

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_listview_item_rows
// Desc.:    Wie viele Bildschirmzeilen Eintrag index braucht: 1 (nur die Kopfzeile), wenn er
//           zugeklappt ist ODER field_count<=0 (nicht erweiterbar) -- sonst 1 (Kopf) + eine Zeile
//           pro Feld + 1 (Trennlinie danach). SONDERFALL (Andreas' Wunsch, 2026-08-18, einund-
//           zwanzigste Runde): ein TEXT-Feld, DIREKT gefolgt von einem BUTTON-Feld, zaehlt als 3
//           Zeilen statt 2 -- der Button wird als echter, dreizeiliger Button wie im Datei-Dialog
//           gezeichnet (Halbblock-Kappen ueber/unter der Textzeile, s. q9_listview_render_ex()),
//           vertikal zentriert NEBEN dem Textfeld statt darunter. items/expanded duerfen NULL sein
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
// Function: q9_listview_field_enter
// Desc.:    Pfeil RECHTS (Andreas' Wunsch, 2026-08-18): den ausgewaehlten Eintrag betreten -- klappt
//           ihn auf (falls noch zu, expanded[selected]=1) und setzt field_focus auf 0 (erstes Feld).
//           Tut NICHTS, wenn der Eintrag keine Felder hat (field_count<=0, s. q9_listview_item_t) --
//           es gibt dann nichts zu betreten.
// Call:     q9_listview_field_enter(&lv, expanded, items)
//════════════════════════════════════════════════════════════════════════════════════════════════
void q9_listview_field_enter(q9_listview_t *lv, int *expanded, const q9_listview_item_t *items);

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_listview_field_leave
// Desc.:    Pfeil LINKS WAEHREND ein Feld fokussiert ist: field_focus zurueck auf -1 (Fokus auf die
//           Kopfzeile) -- der Eintrag bleibt aufgeklappt (im Gegensatz zu q9_listview_field_escape()
//           unten). Kein Effekt, wenn bereits field_focus==-1.
// Call:     q9_listview_field_leave(&lv)
//════════════════════════════════════════════════════════════════════════════════════════════════
void q9_listview_field_leave(q9_listview_t *lv);

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_listview_field_escape
// Desc.:    Esc: wie q9_listview_field_leave() (field_focus zurueck auf -1), klappt den Eintrag
//           danach ZUSAETZLICH zu (Andreas' Wunsch: "bei ESC wird das item auch geschlossen" --
//           "ganz zurueck" statt nur "ein Level zurueck", s. q9_listview_field_leave() fuer die
//           reine Pfeil-links-Variante). Wirkt auch OHNE aktiven Feld-Fokus (klappt einen bereits
//           aufgeklappten Eintrag einfach zu, wie Enter das auch koennte) -- ein allgemeines
//           "Esc = zurueck"-Verhalten, unabhaengig davon, wo genau man gerade steht.
// Call:     q9_listview_field_escape(&lv, expanded)
//════════════════════════════════════════════════════════════════════════════════════════════════
void q9_listview_field_escape(q9_listview_t *lv, int *expanded);

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_listview_field_move
// Desc.:    Pfeil hoch/runter WAEHREND ein Feld fokussiert ist (field_focus>=0): bewegt field_focus
//           um delta, geklemmt auf [0, field_count-1] DES AUSGEWAEHLTEN Eintrags -- verlaesst den
//           Eintrag NICHT ueber die Feldgrenzen hinaus (kein automatisches Umschalten auf den
//           naechsten/vorigen LISTENEINTRAG, das bleibt q9_listview_move_ex() auf Item-Ebene
//           vorbehalten). Tut nichts, wenn field_focus==-1 (Aufrufer-Verantwortung, dann
//           stattdessen move_ex() aufzurufen, s. Kopfkommentar zu field_focus).
// Call:     q9_listview_field_move(&lv, +1, items)
//════════════════════════════════════════════════════════════════════════════════════════════════
void q9_listview_field_move(q9_listview_t *lv, int delta, const q9_listview_item_t *items);

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_listview_field_putc
// Desc.:    Waehrend ein Feld fokussiert ist: haengt ch an den WERT des fokussierten Feldes an
//           (DIREKTE Manipulation -- kein separater Bearbeiten-Modus mit eigenem Bestaetigen/
//           Abbrechen, Andreas' Wunsch: "kann dort alles aendern", tippen wirkt sofort). Ignoriert
//           den Aufruf, wenn field_focus==-1, das fokussierte Feld kind==Q9_LISTVIEW_FIELD_BUTTON
//           ist (ein BUTTON-Feld laesst sich nicht antippen, s. q9_listview_item_t) ODER der Wert
//           bereits Q9_LISTVIEW_FIELD_VALUE_MAX-1 Zeichen erreicht hat (Puffer bleibt IMMER
//           NUL-terminiert, kein Ueberlauf). Bei kind==Q9_LISTVIEW_FIELD_NUMERIC_DEC/_HEX wird ch
//           zusaetzlich auf die passende Zeichenklasse geprueft (nur '0'-'9' bzw. zusaetzlich
//           'a'-'f'/'A'-'F') -- ch wird bei falscher Klasse einfach verworfen (kein Fehler, kein
//           Absturz), genau wie bei zu langem Wert. KEINE Bereichspruefung hier (s.
//           q9_listview_field_kind_t-Kommentar) -- das bleibt Sache des Aufrufers.
// Call:     q9_listview_field_putc(&lv, items, 'x')
//════════════════════════════════════════════════════════════════════════════════════════════════
void q9_listview_field_putc(q9_listview_t *lv, const q9_listview_item_t *items, char ch);

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_listview_field_backspace
// Desc.:    Waehrend ein Feld fokussiert ist: entfernt das LETZTE Zeichen aus dem Wert des
//           fokussierten Feldes (kein Effekt bei bereits leerem Wert). Ignoriert den Aufruf, wenn
//           field_focus==-1 oder kind==Q9_LISTVIEW_FIELD_BUTTON (s. q9_listview_field_putc()) --
//           bei NUMERIC_DEC/_HEX ganz normal wirksam (keine Bereichspruefung beim Loeschen, ein
//           kuerzerer Wert kann voruebergehend ausserhalb eines vom Aufrufer gedachten Bereichs
//           liegen, das ist waehrend des Tippens ein normaler Zwischenzustand).
// Call:     q9_listview_field_backspace(&lv, items)
//════════════════════════════════════════════════════════════════════════════════════════════════
void q9_listview_field_backspace(q9_listview_t *lv, const q9_listview_item_t *items);

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_listview_field_toggle
// Desc.:    Waehrend ein Feld mit kind==Q9_LISTVIEW_FIELD_BOOLEAN fokussiert ist: schaltet dessen
//           Wert um -- "ja" wird zu "nein" und umgekehrt. Der Aufrufer bindet dies ueblicherweise an
//           die Leertaste (s. integration_demo.c). Ignoriert den Aufruf, wenn field_focus==-1 oder
//           das fokussierte Feld NICHT kind==Q9_LISTVIEW_FIELD_BOOLEAN ist (dann tut sich nichts,
//           kein Fehler, kein Absturz). Ist der aktuelle Wert weder "ja" noch "nein" (sollte bei
//           korrekt initialisierten BOOLEAN-Feldern nicht vorkommen), wird er auf "ja" gesetzt.
// Call:     q9_listview_field_toggle(&lv, items)
//════════════════════════════════════════════════════════════════════════════════════════════════
void q9_listview_field_toggle(q9_listview_t *lv, const q9_listview_item_t *items);

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_listview_render_ex
// Desc.:    Wie q9_listview_render(), aber fuer erweiterbare, editierbare Eintraege (s.
//           q9_listview_item_t). Jede Kopfzeile bekommt ein Pfeil-Symbol davor (Q9_GLYPH_DOWN_ARROW
//           aufgeklappt, Q9_GLYPH_RIGHT_ARROW zugeklappt, ein Leerzeichen bei field_count<=0 --
//           nichts zum Auf-/Zuklappen). Aufgeklappte Felder werden um zwei Spalten eingerueckt
//           (Pfeil + Luftspalte), Label und Wert nebeneinander in EINER EIGENEN Farbe (detail_fg,
//           gedaempft/anders als der normale Eintragstext), danach eine volle Trennlinie
//           (Q9_GLYPH_HLINE ueber content_width, in line_fg). Ein Feld mit kind==
//           Q9_LISTVIEW_FIELD_NUMERIC_HEX bekommt vor dem Wert automatisch ein "$" gezeichnet
//           (nicht Teil von value selbst, s. q9_listview_field_kind_t) -- NUMERIC_DEC und TEXT
//           sehen sich sonst gleich (der Unterschied ist nur die Zeichenklassen-Filterung beim
//           Tippen, s. q9_listview_field_putc()). Bewusst LEICHTGEWICHTIG (Andreas'
//           Wahl, 2026-08-18, aus drei vorgeschlagenen Stilen): KEIN Rahmen um den aufgeklappten
//           Bereich -- nur Einrueckung + die eine Trennlinie danach, spart am meisten Platz. Zeilen,
//           die nicht mehr in den Viewport passen (Eintrag laeuft ueber das Fensterende hinaus),
//           werden abgeschnitten, wie bei render() ueberzaehlige Eintraege. Rechte Spalte (Linie +
//           Bildlaufleiste) jetzt ROW-basiert statt item-basiert -- Griffgroesse/-position richten
//           sich nach der GESAMTZEILENZAHL aller Eintraege (inkl. aufgeklappter), nicht mehr nach
//           der reinen Eintragsanzahl.
//           Kopfzeile eines AUFGEKLAPPTEN Eintrags bekommt exp_bg als eigenen Hintergrund (Andreas'
//           Feedback, 2026-08-18: "die Headerzeile geht ein wenig unter" -- ohne Hervorhebung sah
//           eine aufgeklappte Kopfzeile bisher genauso aus wie jede andere, gedaempfte Detailzeilen
//           direkt darunter liessen sie optisch "verschwimmen"). Ist der Eintrag ZUSAETZLICH
//           ausgewaehlt UND field_focus==-1 (Fokus auf der Kopfzeile selbst, kein Feld), gewinnt
//           stattdessen sel_bg (staerker/wichtiger als die reine "ist aufgeklappt"-Markierung). Ist
//           dagegen field_focus>=0 (Fokus ist in ein Feld gewandert, s. q9_listview_field_enter()),
//           faellt die Kopfzeile auf exp_bg zurueck -- sel_bg wandert stattdessen auf die FOKUSSIERTE
//           Feldzeile (Wert-Spalte in sel_fg/sel_bg statt detail_fg, deutlich als "hier tippst du
//           gerade" erkennbar), es ist zu jedem Zeitpunkt genau EINE Zeile in sel_bg.
//           TEXT+BUTTON-SONDERFALL (Andreas' Wunsch, 2026-08-18, einundzwanzigste Runde -- "das
//           Feld mit dem Dateinamen einen anderen Farbton, so dass man erkennen kann wie lang es
//           ist, wie im Dialog" + "der dreizeilige Button wie im Dialog... zentrisch hinter Datei
//           ausgerichtet"): folgt auf ein TEXT-Feld DIREKT ein BUTTON-Feld, werden beide als EINE
//           Einheit gezeichnet (s. q9_listview_item_rows() fuer die Zeilenzahl):
//             - der WERT des TEXT-Feldes bekommt eine feste, sichtbare Box (box_fg/box_bg NEU --
//               dieselben Farben wie das Namens-Kaestchen im Datei-Dialog, wenn der Aufrufer das
//               so uebergibt) statt reinem Fliesstext in detail_fg -- die feste Breite macht sofort
//               sichtbar, wie lang der Wert (noch) werden kann/ist.
//             - der BUTTON wird wie im Datei-Dialog gezeichnet: Text in einer eigenen Farbflaeche,
//               darueber/darunter je eine Halbblock-Kappenzeile (Q9_GLYPH_LOWER_HALF/UPPER_HALF,
//               genau wie draw_button() in q9_filedialog.c) -- dadurch braucht das Feld-Paar
//               INSGESAMT 3 Zeilen: Kappe oben, die gemeinsame Zeile (Textfeld-Label+Box LINKS, der
//               eigentliche Button-Text RECHTS daneben, mit Abstand), Kappe unten -- der Button
//               steht dadurch vertikal zentriert AUF HOEHE des Textfelds, nicht darunter. box_fg/bg
//               gilt fuer den Button UNFOKUSSIERT (inkl. seiner Kappen); ist der Button fokussiert
//               (field_focus zeigt auf ihn), springt er wie jedes andere Feld auf sel_fg/sel_bg um.
// Call:     q9_listview_render_ex(&lv, &sb, items, expanded, 255,255,255, 0,0,0, 255,255,0,
//                                  200,200,200, 150,120,80, 112,85,20, 255,248,225, 112,85,20)
//════════════════════════════════════════════════════════════════════════════════════════════════
void q9_listview_render_ex(const q9_listview_t *lv, q9_screenbuf_t *sb,
                            const q9_listview_item_t *items, const int *expanded,
                            int fg_r, int fg_g, int fg_b,
                            int sel_fg_r, int sel_fg_g, int sel_fg_b,
                            int sel_bg_r, int sel_bg_g, int sel_bg_b,
                            int line_fg_r, int line_fg_g, int line_fg_b,
                            int detail_fg_r, int detail_fg_g, int detail_fg_b,
                            int exp_bg_r, int exp_bg_g, int exp_bg_b,
                            int box_fg_r, int box_fg_g, int box_fg_b,
                            int box_bg_r, int box_bg_g, int box_bg_b);

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
// EOF q9_listview.h                                                                       Ver. 2.20
//────────────────────────────────────────────────────────────────────────────────────────────────
