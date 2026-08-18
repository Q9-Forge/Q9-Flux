//════════════════════════════════════════════════════════════════════════════════════════════════
// File:   integration_demo.c                                                             Ver. 3.70
// Owner:  Claudia
// Desc.:  Reine SICHTPRUEFUNG (kein automatisierter Test, wie ansi_selftest --demo) -- zeigt alle
//         sechs Bausteine zusammen in einem einzigen, echten Bildschirm: Rahmen (q9_widgets),
//         scrollbare Liste (q9_listview) in einem Bildschirmpuffer (q9_screenbuf), Tastatur-
//         Navigation + dynamische Groessenanpassung (q9_input). NICHT der eigentliche Q9-Flux-
//         Editor (keine echte Config-Anbindung, keine Hardware-Typen, keine Buttons) -- nur der
//         Nachweis, dass die Bausteine zusammenpassen.
//
//         Farbpalette (Andreas' Wunsch, 2026-08-17): "aehnliche" Toene statt bunt gemischt --
//         warme Amber-/Beige-/Braun-Palette (Vorbild: Hermes-Farbschema). Eine "kuehle" Variante
//         (Blau/Gruen/Cyan) waere nach demselben Muster (nur die Zahlenwerte unten aendern) ebenso
//         moeglich -- eine echte UMSCHALTBARE Palette (mehrere Saetze + Auswahl zur Laufzeit) ist
//         als "Endausbau"-Idee vorgemerkt, hier erstmal nur EIN fest verdrahteter Satz.
//
// Call:   make -C tools/q9-flux-editor demo-integration
//         Pfeiltasten hoch/runter: Auswahl bewegen. O: Datei-Auswahl-Dialog oeffnen (scannt
//         ~/.q9-flux mit *.q9-Filter, s. run_file_dialog()). Strg-C: beenden.
//
// Edition History
//─────────┬──────┬────────────────────────────────────────────────────────────────────────┬──────
// Date    │ Ver. │ Description                                                            │ By
//─────────┼──────┼────────────────────────────────────────────────────────────────────────┬──────
// 26-08-17│ 1.00 │ Erster Wurf -- Andreas wollte sich das Ganze mal ansehen                │ Cld
// 26-08-17│ 1.10 │ Andreas' Feedback nach dem ersten Ansehen: Mindestgroesse (60x20, sonst  │ Cld
//         │      │ Hinweis statt verzerrtem Layout), Statuszeile bekommt eine EIGENE Zeile   │
//         │      │ mit eigenem Hintergrund statt in die untere Rahmenkante gemischt zu       │
//         │      │ werden (das war die "kleine Linie ganz rechts unten"), warme Amber-/      │
//         │      │ Beige-Palette statt der bisherigen Zufallsfarben                          │
// 26-08-17│ 1.20 │ Zweite Feedback-Runde: Statuszeile jetzt ALS untere Rahmenkante (nicht mehr │ Cld
//         │      │ eigene separate Zeile darueber) -- Resize-Flackern behoben (150ms-Entprel-  │
//         │      │ lung in q9_input.c), Scrollbalken-Abstand zur markierten Zeile (q9_listview.c)│
// 26-08-17│ 1.30 │ Statuszeile jetzt UEBER DIE VOLLE BREITE (Andreas: "aufgeraeumter als dieser │ Cld
//         │      │ doppelte Strich") -- ueberschreibt auch die beiden unteren Eckzeichen, keine │
//         │      │ Ecken mehr unten                                                             │
// 26-08-17│ 1.40 │ Dritte Feedback-Runde: Kopfzeile jetzt ebenfalls volle Breite (etwas heller  │ Cld
//         │      │ als die Statuszeile), feste Feldbreiten in der Statuszeile (kein Hin- und    │
//         │      │ Herspringen mehr bei unterschiedlich langen Eintragsnamen), neuer Resize-    │
//         │      │ Overlay-Modus: waehrend/nach einer Groessenaenderung wird HOECHSTENS 1s lang │
//         │      │ nur "R Rows - C Columns" zentriert angezeigt (LIVE aktualisiert, kein voller  │
//         │      │ Neuaufbau bei jedem Zwischenschritt) -- erst nach 1s Stille kommt der volle   │
//         │      │ Inhalt zurueck. Ist das Fenster dabei (immer noch) zu klein, bleibt das       │
//         │      │ Overlay dauerhaft sichtbar (plus Zusatzzeile), statt den vollen Inhalt zu     │
//         │      │ versuchen -- ersetzt die vorherige separate "Fenster zu klein"-Anzeige         │
// 26-08-17│ 1.50 │ Vierte Feedback-Runde: Overlay zeigt "Columns - Rows" (statt "Rows - Columns"),│ Cld
//         │      │ Kopfzeile-Titel linksbuendig ab Spalte 3 (statt zentriert), helleres Weiss;    │
//         │      │ bei anhaltend zu kleinem Fenster EINMALIGER Versuch, per XTWINOPS-Escape-       │
//         │      │ Sequenz (q9_ansi_resize_window) automatisch auf die Mindestgroesse zu           │
//         │      │ vergroessern -- nicht universell unterstuetzt, wirkt nur auf Terminals mit      │
//         │      │ aktivierten "Window Ops" (z.B. xterm)                                           │
// 26-08-17│ 1.60 │ Taste 'O' oeffnet den modalen Datei-Auswahl-Dialog (q9_filedialog.h/.c,          │ Cld
//         │      │ task #20/#22) zentriert ueber dem Bildschirm, scannt "." mit Beispielfiltern     │
//         │      │ ("*.*"/".c"/".h"); Ergebnis (Datei gewaehlt/Abbruch) ersetzt bis zur naechsten   │
//         │      │ Dialog-Oeffnung den unteren Hinweistext. DEMO-GRENZE: ein Resize waehrend der    │
//         │      │ Dialog offen ist, wird ignoriert (kein Nachziehen der Dialog-Geometrie)          │
// 26-08-17│ 1.70 │ Andreas' Feedback nach dem ersten Test: Dialog-Hintergrund ist jetzt der ECHTE   │ Cld
//         │      │ Hauptbildschirm (build_full_content() ausgelagert) statt einer reinen Fuellfarbe │
//         │      │ ueber den ganzen Schirm -- vorher sah der (schon immer kleine) Dialog dadurch    │
//         │      │ wie Vollbild aus                                                                 │
// 26-08-17│ 1.80 │ Zweite Feedback-Runde: eigene footer_bg-Palette fuer q9_filedialog.c (Fuss-       │ Cld
//         │      │ bereich mit den "richtigen" Buttons), Scan-Verzeichnis auf getenv("HOME") gestellt │
//         │      │ (Andreas: "stell den Pfad mal auf das ~ Verzeichnis, dann sieht man das besser"), │
//         │      │ DIALOG_ROWS/_COLS vergroessert (neuer Fussbereich braucht mehr Platz)             │
// 26-08-17│ 1.90 │ Dritte Feedback-Runde: Hauptfenster-Liste reicht jetzt bis zur rechten Rahmen-    │ Cld
//         │      │ kante von draw_frame() -- verschmilzt mit q9_listview's Bildlaufleiste zu EINER   │
//         │      │ Linie statt "Fensterkante + separate Bildlaufleiste" (Andreas' Wunsch)            │
// 26-08-17│ 2.00 │ Vierte Feedback-Runde ("wirkt jetzt doch gequetscht"): DIALOG_ROWS/_COLS         │ Cld
//         │      │ vergroessert (neue Filterzeile + Statuszeile im Dialog, plus Luftspalten)         │
// 26-08-17│ 2.10 │ Fuenfte Feedback-Runde ("zwei Zeilen sparen"): DIALOG_ROWS wieder verkleinert    │ Cld
//         │      │ (Buttons teilen sich jetzt Namens-/Filterzeile statt eigene Zeilen zu belegen)    │
// 26-08-17│ 2.20 │ Sechste Feedback-Runde ("Farben Richtung Braun abgerutscht"): komplette Palette   │ Cld
//         │      │ auf durchgehende Gelb-/Orange-Leiter umgerechnet (gleicher Farbton/Saettigung,    │
//         │      │ nur die Helligkeit unterscheidet die Ebenen), Tabellenhintergrund dunkler,         │
//         │      │ groesserer Helligkeitssprung zwischen den Ebenen fuer mehr Kontrast                │
// 26-08-17│ 2.30 │ Siebte Feedback-Runde: Kopfzeilen-Text jetzt dunkel statt fast-weiss (schlecht     │ Cld
//         │      │ lesbar auf hellem Gelb), neue PAL_DIALOG_SUB_FG fuer Tabellenkopf/Namens-Kaestchen/│
//         │      │ OK-Abbrechen (mehr Kontrast), PAL_DIALOG_FOOTER_BG referenziert jetzt PAL_STATUS_BG│
//         │      │ direkt, Hauptfenster-Liste bekommt line_fg=PAL_FRAME (Linien-Farbinkonsistenz-Fix) │
// 26-08-17│ 2.40 │ Achte Feedback-Runde: render_size_overlay() nutzt PAL_STATUS_FG statt PAL_HEADER_FG│ Cld
//         │      │ (war unlesbar dunkel geworden, kein farbiger Hintergrund dort), neue eigene       │
//         │      │ status_fg/bg-Felder fuer die Dialog-Statuszeile (jetzt = Hauptfenster-Statuszeile),│
//         │      │ run_file_dialog() behandelt Resize waehrend der Dialog offen ist (compute_dialog_  │
//         │      │ geometry() zentriert dabei automatisch neu), rows/cols jetzt Zeiger                │
// 26-08-18│ 2.60 │ Sechzehnte Feedback-Runde: g_items durch g_list_items ersetzt (Name + Detailzeilen,│ Cld
//         │      │ q9_listview_item_t), g_expanded-Array, Enter klappt den ausgewaehlten Eintrag      │
//         │      │ auf/zu, Haupt-Listenansicht nutzt jetzt q9_listview_render_ex()/_scroll_ex()/       │
//         │      │ _move_ex() (erweiterbare Eintraege, s. q9_listview.h)                               │
// 26-08-18│ 2.70 │ Siebzehnte Feedback-Runde: neue PAL_LIST_EXP_BG (V=0.44) fuer die Kopfzeile eines  │ Cld
//         │      │ aufgeklappten, nicht ausgewaehlten Eintrags (Andreas: "die Headerzeile geht ein    │
//         │      │ wenig unter")                                                                       │
// 26-08-18│ 2.80 │ Achtzehnte Feedback-Runde: Feld-Navigation -- g_items-Detailzeilen durch echte,     │ Cld
//         │      │ editierbare Felder (label+value) ersetzt, Pfeil rechts betritt den Eintrag (erstes │
//         │      │ Feld fokussiert), Pfeil links/Esc verlassen es (Esc klappt zusaetzlich zu), Zeichen/│
//         │      │ Backspace aendern den fokussierten Feldwert direkt                                  │
// 26-08-17│ 2.50 │ Neunte Feedback-Runde ("da wird immer alles neu gezeichnet"): run_file_dialog()    │ Cld
//         │      │ nutzt jetzt dasselbe Overlay-Settle-Muster wie main() -- waehrend des Ziehens nur  │
//         │      │ billiges render_size_overlay(), teurer Dialog-Neuaufbau erst nach RESIZE_SETTLE_MS │
//         │      │ Stille statt bei jedem Zwischenschritt; render_size_overlay() zeigt den Groessen-  │
//         │      │ Text jetzt an fester Position (OVERLAY_ROW/_COL = 3,3) statt zentriert (huepfte     │
//         │      │ sonst waehrend des Ziehens staendig an eine andere Stelle)                          │
// 26-08-18│ 2.90 │ Neunzehnte Feedback-Runde: erster fester Eintrag "Emulator-Konfiguration" (Name +   │ Cld
//         │      │ Button-Feld), run_file_dialog() liefert optional den rohen Dateinamen zurueck,      │
//         │      │ Enter auf einem BUTTON-Feld oeffnet den Dialog, Ergebnis geht ins Feld davor         │
// 26-08-18│ 3.00 │ Zwanzigste Feedback-Runde: Datei-Dialog scannt jetzt ~/.q9-flux (wird bei Bedarf    │ Cld
//         │      │ angelegt) statt HOME, Standardfilter auf *.q9 umgestellt (*.* bleibt als Ausweich-  │
//         │      │ option im Dropdown) -- kein reines Demo-Verzeichnis mehr, echtes Zielverzeichnis     │
// 26-08-18│ 3.10 │ Einundzwanzigste Feedback-Runde: Button-Feldwert auf "Datei" gekuerzt, Platzhalter  │ Cld
//         │      │ "(keine ausgewaehlt)" durch "<leer>" ersetzt, render_ex()-Aufruf um box_fg/bg       │
//         │      │ (PAL_DIALOG_SUB_FG/BG, "wie im Dialog") ergaenzt                                    │
// 26-08-18│ 3.20 │ Dreiundzwanzigste Feedback-Runde: Basis:/IRQ:/Port:-Felder auf NUMERIC_HEX/_DEC      │ Cld
//         │      │ umgestellt ("$" faellt aus dem Wert, wird jetzt automatisch gezeichnet), Slot:-Feld │
//         │      │ bei RC2014-CF mit Bereichs-Beispiel (0-255, clamp_field_range())                    │
// 26-08-18│ 3.30 │ Vierundzwanzigste Feedback-Runde: alle "ja"/"nein"-Felder (Aktiv:/Link:/Parity:/     │ Cld
//         │      │ Getestet:/Schreibschutz:) auf Q9_LISTVIEW_FIELD_BOOLEAN umgestellt, Leertaste        │
//         │      │ schaltet um (q9_listview_field_toggle()), Hinweistext ergaenzt (Andreas: "Boolean    │
//         │      │ Eingabe")                                                                            │
// 26-08-18│ 3.40 │ Fuenfundzwanzigste Feedback-Runde: echtes Laden der .q9-Datei -- neue Felder Name:/  │ Cld
//         │      │ ROM:/Netz:/CPU: bei Emulator-Konfiguration, load_q9_config_fields() ruft den ECHTEN  │
//         │      │ Board-Config-Parser des Emulators (src/kernel/boardcfg.c) auf, kein zweiter eigener  │
//         │      │ INI-Parser (Andreas: "echtes Laden/Auswerten der .q9-Datei")                         │
// 26-08-18│ 3.50 │ Sechsundzwanzigste Feedback-Runde: Speichern-Funktion -- Taste S ruft save_q9_config()│ Cld
//         │      │ auf (q9_board_cfg_save(), gleiches Prinzip wie beim Laden), g_loaded_cfg haelt eine   │
//         │      │ geladene Konfiguration vollstaendig (inkl. [cfN]) am Leben, damit Speichern sie nicht │
//         │      │ stillschweigend loescht (Andreas: "Speichern-Funktion")                              │
// 26-08-18│ 3.60 │ NACHTRAG (Versionsbump beim Original-Commit vergessen): EOF-Fusszeile war noch auf    │ Cld
//         │      │ 3.20 stehen geblieben, jetzt nachgezogen. Siebenundzwanzigste Feedback-Runde: [cfN]-  │
//         │      │ Abschnitte einer geladenen Datei erscheinen jetzt als eigene CF-Image-#N-Eintraege    │
//         │      │ (g_cfimg_fields, sync_cfimg_items()), editierbar UND ins Speichern eingebunden         │
// 26-08-18│ 3.70 │ Achtundzwanzigste Feedback-Runde: mechanische NUMERIC-Uebernahme -- einziger noch     │ Cld
//         │      │ verbliebener reiner Zahlenwert (CLUT "Eintraege:") auf NUMERIC_DEC umgestellt; die    │
//         │      │ "Groesse:"-Felder (512K/4 MB/256K/2K) bleiben bewusst TEXT (Einheit im Wert, keine    │
//         │      │ reine Zahl -- NUMERIC_DEC wuerde K/MB/Leerzeichen beim Tippen verwerfen)               │
//═════════╧══════╧════════════════════════════════════════════════════════════════════════╧══════
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>                                        /* mkdir() fuer ~/.q9-flux, s.
                                                                  run_file_dialog()             */

#include "../src/q9_ansi.h"
#include "../src/q9_screenbuf.h"
#include "../src/q9_widgets.h"
#include "../src/q9_listview.h"
#include "../src/q9_input.h"
#include "../src/q9_filedialog.h"
/* Andreas' Wunsch (2026-08-18, fuenfundzwanzigste Runde): "echtes Laden/Auswerten der .q9-Datei"
   -- der ECHTE Board-Config-Parser des Emulators (derselbe, den q9.exe beim Start liest), KEIN
   zweiter, eigener INI-Parser hier. boardcfg.c ist bewusst frei von weiteren Kernel-Funktions-
   Aufrufen (nur q9board.h/devreg.h fuer ein paar Konstanten/Typen, s. dortiger Kopfkommentar) --
   laesst sich daher unveraendert in dieses eigenstaendige Tool hineinlinken (s. Makefile),
   garantiert dabei echte Formatkompatibilitaet statt eines zweiten, driftenden Parsers. */
#include "../../../src/kernel/boardcfg.h"
/* Siebenundzwanzigste Runde: Q9_CF_FMT_RBF/_PCF/_AUTO fuer die CF-Image-Felder (Typ:, s. unten)
   -- diese Konstanten leben in q9board.h, NICHT in boardcfg.h (das Struct-Feld q9_cfg_cf_t.format
   dokumentiert das explizit, s. dort). q9board.h selbst bindet nur devreg.h (Konstanten/Typen,
   kein Funktionsaufruf) -- bleibt daher ebenso gefahrlos linkbar wie boardcfg.c. */
#include "../../../src/kernel/q9board.h"

/* Rein zur Demonstration -- kein echtes Hardware-Modell, s. Kopfkommentar. Erweiterbare, editierbare
   Eintraege (Andreas' Wunsch, 2026-08-18: "groessere Eintraege... minimiert ein oder zwei Zeilen,
   aufgeklappt so viele wie sie brauchen", dann siebzehnte Runde: "wie komme ich in das item rein um
   dort Werte zu aendern?") -- jeder Eintrag hat eine Kopfzeile (name) plus ein paar FELDER (Label +
   editierbarer Wert), die nur sichtbar werden, wenn der Eintrag aufgeklappt ist (Pfeil rechts auf
   der Auswahl, s. main()). Vier Felder je Eintrag reichen fuer die Vorfuehrung -- das Datenmodell
   (q9_listview_item_t.field_count, s. q9_listview.h) erlaubt aber pro Eintrag eine BELIEBIGE Anzahl,
   das ist keine feste Grenze der Bibliothek. Die Feld-Arrays sind BEWUSST NICHT const (im Gegensatz
   zu g_list_items[] selbst) -- q9_listview_field_t.value wird durch Tippen direkt veraendert (s.
   q9_listview_field_putc()/_backspace()), reine Vorfuehrdaten, KEINE echte Config-Anbindung (s.
   Kopfkommentar) -- das (noch offene) naechste Stueck waere, ein kleines Datenfile pro Hardware-Typ
   auszuwerten (aehnlich devschema.h/.c) statt dieser fest verdrahteten Felder. */
/* Erster fester Eintrag -- KEIN Hardware-Ding wie die Eintraege darunter, sondern die eigentliche
   Konfigurationsdatei-Auswahl (Andreas' Wunsch, 2026-08-18, neunzehnte Runde). Zwei Felder: das
   Namensfeld (normaler TEXT, zeigt den gewaehlten Dateinamen -- von Hand tippbar UND per Button
   befuellbar) und ein BUTTON-Feld dahinter, das den bestehenden Datei-Dialog oeffnet (s.
   Q9_KEY_ENTER-Behandlung in main()).
   Name:/ROM:/Netz:/CPU: (fuenfundzwanzigste Runde, "echtes Laden/Auswerten der .q9-Datei") --
   FESTE Positionen g_cfg_fields[2..5] (s. load_q9_config_fields()), werden erst nach erfolgreicher
   Dateiauswahl befuellt (vorher "<leer>", bei Ladefehler "<Fehler>"). Von Hand editierbar UND
   (sechsundzwanzigste Runde, "Speichern-Funktion") per Taste S in die Datei zurueckgeschrieben --
   s. save_q9_config(). */
static q9_listview_field_t g_cfg_fields[] = {
    {"Datei:", "<leer>", Q9_LISTVIEW_FIELD_TEXT},
    { "",       "Datei", Q9_LISTVIEW_FIELD_BUTTON },
    {"Name:",  "<leer>", Q9_LISTVIEW_FIELD_TEXT},
    {"ROM:",   "<leer>", Q9_LISTVIEW_FIELD_TEXT},
    {"Netz:",  "<leer>", Q9_LISTVIEW_FIELD_TEXT},
    {"CPU:",   "<leer>", Q9_LISTVIEW_FIELD_TEXT},
};

static q9_listview_field_t g_cf_fields[]     = { {"Bus:", "onboard", Q9_LISTVIEW_FIELD_TEXT},   {"Basis:", "FFFFE000", Q9_LISTVIEW_FIELD_NUMERIC_HEX},
                                                  {"Slot:", "-", Q9_LISTVIEW_FIELD_TEXT},        {"Aktiv:", "ja", Q9_LISTVIEW_FIELD_BOOLEAN} };
static q9_listview_field_t g_net1_fields[]   = { {"Port:", "2001", Q9_LISTVIEW_FIELD_NUMERIC_DEC},     {"Protokoll:", "Telnet", Q9_LISTVIEW_FIELD_TEXT},
                                                  {"Status:", "bereit", Q9_LISTVIEW_FIELD_TEXT}, {"Baudrate:", "-", Q9_LISTVIEW_FIELD_TEXT} };
static q9_listview_field_t g_net2_fields[]   = { {"Port:", "2002", Q9_LISTVIEW_FIELD_NUMERIC_DEC},     {"Protokoll:", "Telnet", Q9_LISTVIEW_FIELD_TEXT},
                                                  {"Status:", "bereit", Q9_LISTVIEW_FIELD_TEXT}, {"Baudrate:", "-", Q9_LISTVIEW_FIELD_TEXT} };
static q9_listview_field_t g_net3_fields[]   = { {"Port:", "2003", Q9_LISTVIEW_FIELD_NUMERIC_DEC},     {"Protokoll:", "Telnet", Q9_LISTVIEW_FIELD_TEXT},
                                                  {"Status:", "bereit", Q9_LISTVIEW_FIELD_TEXT}, {"Baudrate:", "-", Q9_LISTVIEW_FIELD_TEXT} };
static q9_listview_field_t g_net4_fields[]   = { {"Port:", "2004", Q9_LISTVIEW_FIELD_NUMERIC_DEC},     {"Protokoll:", "Telnet", Q9_LISTVIEW_FIELD_TEXT},
                                                  {"Status:", "bereit", Q9_LISTVIEW_FIELD_TEXT}, {"Baudrate:", "-", Q9_LISTVIEW_FIELD_TEXT} };
static q9_listview_field_t g_net5_fields[]   = { {"Port:", "2005", Q9_LISTVIEW_FIELD_NUMERIC_DEC},     {"Protokoll:", "Telnet", Q9_LISTVIEW_FIELD_TEXT},
                                                  {"Status:", "bereit", Q9_LISTVIEW_FIELD_TEXT}, {"Baudrate:", "-", Q9_LISTVIEW_FIELD_TEXT} };
static q9_listview_field_t g_net6_fields[]   = { {"Port:", "2006", Q9_LISTVIEW_FIELD_NUMERIC_DEC},     {"Protokoll:", "Telnet", Q9_LISTVIEW_FIELD_TEXT},
                                                  {"Status:", "bereit", Q9_LISTVIEW_FIELD_TEXT}, {"Baudrate:", "-", Q9_LISTVIEW_FIELD_TEXT} };
static q9_listview_field_t g_net7_fields[]   = { {"Port:", "2007", Q9_LISTVIEW_FIELD_NUMERIC_DEC},     {"Protokoll:", "Telnet", Q9_LISTVIEW_FIELD_TEXT},
                                                  {"Status:", "bereit", Q9_LISTVIEW_FIELD_TEXT}, {"Baudrate:", "-", Q9_LISTVIEW_FIELD_TEXT} };
static q9_listview_field_t g_net8_fields[]   = { {"Port:", "2008", Q9_LISTVIEW_FIELD_NUMERIC_DEC},     {"Protokoll:", "Telnet", Q9_LISTVIEW_FIELD_TEXT},
                                                  {"Status:", "bereit", Q9_LISTVIEW_FIELD_TEXT}, {"Baudrate:", "-", Q9_LISTVIEW_FIELD_TEXT} };
static q9_listview_field_t g_rtc_fields[]    = { {"Basis:", "FFFFA000", Q9_LISTVIEW_FIELD_NUMERIC_HEX}, {"IRQ:", "-", Q9_LISTVIEW_FIELD_TEXT},
                                                  {"Batterie:", "ok", Q9_LISTVIEW_FIELD_TEXT},    {"Aktiv:", "ja", Q9_LISTVIEW_FIELD_BOOLEAN} };
static q9_listview_field_t g_duart_fields[]  = { {"Basis:", "FFFFA000", Q9_LISTVIEW_FIELD_NUMERIC_HEX}, {"IRQ:", "2", Q9_LISTVIEW_FIELD_NUMERIC_DEC},
                                                  {"Kanal A:", "Konsole", Q9_LISTVIEW_FIELD_TEXT}, {"Kanal B:", "frei", Q9_LISTVIEW_FIELD_TEXT} };
static q9_listview_field_t g_quicc_fields[]  = { {"MAC:", "00:1A:2B:03:04:05", Q9_LISTVIEW_FIELD_TEXT},
                                                  {"Link:", "nein", Q9_LISTVIEW_FIELD_BOOLEAN}, {"Aktiv:", "nein", Q9_LISTVIEW_FIELD_BOOLEAN} };
static q9_listview_field_t g_mc6845_fields[] = { {"Basis:", "FFFF9000", Q9_LISTVIEW_FIELD_NUMERIC_HEX}, {"IRQ:", "3", Q9_LISTVIEW_FIELD_NUMERIC_DEC},
                                                  {"Modus:", "Text 80x25", Q9_LISTVIEW_FIELD_TEXT}, {"Aktiv:", "ja", Q9_LISTVIEW_FIELD_BOOLEAN} };
static q9_listview_field_t g_clut_fields[]   = { {"Basis:", "FFFF9800", Q9_LISTVIEW_FIELD_NUMERIC_HEX}, {"Eintraege:", "256", Q9_LISTVIEW_FIELD_NUMERIC_DEC},
                                                  {"Tiefe:", "8 Bit", Q9_LISTVIEW_FIELD_TEXT},   {"Aktiv:", "ja", Q9_LISTVIEW_FIELD_BOOLEAN} };
static q9_listview_field_t g_rc2014_fields[] = { {"Bus:", "rc2014", Q9_LISTVIEW_FIELD_TEXT},  {"Basis:", "FFFFC010", Q9_LISTVIEW_FIELD_NUMERIC_HEX},
                                                  {"Slot:", "0", Q9_LISTVIEW_FIELD_NUMERIC_DEC}, {"Aktiv:", "nein", Q9_LISTVIEW_FIELD_BOOLEAN} };
static q9_listview_field_t g_fb_fields[]     = { {"Basis:", "00300000", Q9_LISTVIEW_FIELD_NUMERIC_HEX}, {"Groesse:", "512K", Q9_LISTVIEW_FIELD_TEXT},
                                                  {"Aufloesung:", "640x480", Q9_LISTVIEW_FIELD_TEXT}, {"Aktiv:", "ja", Q9_LISTVIEW_FIELD_BOOLEAN} };
static q9_listview_field_t g_ram_fields[]    = { {"Basis:", "00000000", Q9_LISTVIEW_FIELD_NUMERIC_HEX}, {"Groesse:", "4 MB", Q9_LISTVIEW_FIELD_TEXT},
                                                  {"Parity:", "nein", Q9_LISTVIEW_FIELD_BOOLEAN}, {"Getestet:", "ja", Q9_LISTVIEW_FIELD_BOOLEAN} };
static q9_listview_field_t g_rom_fields[]    = { {"Basis:", "00F00000", Q9_LISTVIEW_FIELD_NUMERIC_HEX}, {"Groesse:", "256K", Q9_LISTVIEW_FIELD_TEXT},
                                                  {"Schreibschutz:", "ja", Q9_LISTVIEW_FIELD_BOOLEAN}, {"Aktiv:", "ja", Q9_LISTVIEW_FIELD_BOOLEAN} };
static q9_listview_field_t g_nvram_fields[]  = { {"Basis:", "FFFFB000", Q9_LISTVIEW_FIELD_NUMERIC_HEX}, {"Groesse:", "2K", Q9_LISTVIEW_FIELD_TEXT},
                                                  {"Batterie:", "ok", Q9_LISTVIEW_FIELD_TEXT},   {"Aktiv:", "ja", Q9_LISTVIEW_FIELD_BOOLEAN} };
static q9_listview_field_t g_timer_fields[]  = { {"Basis:", "FFFFA800", Q9_LISTVIEW_FIELD_NUMERIC_HEX}, {"IRQ:", "3", Q9_LISTVIEW_FIELD_NUMERIC_DEC},
                                                  {"Intervall:", "10ms", Q9_LISTVIEW_FIELD_TEXT}, {"Aktiv:", "ja", Q9_LISTVIEW_FIELD_BOOLEAN} };

/* Siebenundzwanzigste Runde ("CF-Image-Abschnitte im Editor selbst anzeigen/bearbeiten") -- die
   [cfN]-Abschnitte einer GELADENEN Datei (g_loaded_cfg.cf[], s. load_q9_config_fields()), NICHT
   zu verwechseln mit den obigen Hardware-DEMO-Eintraegen (die bleiben reine Vorfuehrdaten, s.
   Kopfkommentar). Q9_CFG_MAX_CF (boardcfg.h) feste Slots, IMMER am ENDE von g_list_items[]
   (s. dort) -- dadurch braucht ein wechselnder cf_count keine Verschiebung anderer Eintraege,
   nur g_item_count (s. dort) waechst/schrumpft. Vier Felder je Slot: Typ:/Bus:/Unit: (die drei
   kurzen Enum-Werte aus q9_cfg_cf_t, s. boardcfg.h) und Datei: (der Image-Pfad, wie ROM: beim
   Laden bereits aufgeloest angezeigt). BEKANNTE VEREINFACHUNG: Basis/Slot/Descriptor sind (noch)
   NICHT editierbar hier -- bleiben beim Speichern unangetastet wie bisher (s. save_q9_config()),
   das waere ein eigener, noch groesserer Schritt (Bereichspruefung, useSlot/base-Wechselspiel). */
#define Q9_LISTVIEW_CFIMG_SLOTS Q9_CFG_MAX_CF
static q9_listview_field_t g_cfimg_fields[Q9_LISTVIEW_CFIMG_SLOTS][4] = {
    { {"Typ:", "<leer>", Q9_LISTVIEW_FIELD_TEXT}, {"Bus:", "<leer>", Q9_LISTVIEW_FIELD_TEXT},
      {"Unit:", "<leer>", Q9_LISTVIEW_FIELD_TEXT}, {"Datei:", "<leer>", Q9_LISTVIEW_FIELD_TEXT} },
    { {"Typ:", "<leer>", Q9_LISTVIEW_FIELD_TEXT}, {"Bus:", "<leer>", Q9_LISTVIEW_FIELD_TEXT},
      {"Unit:", "<leer>", Q9_LISTVIEW_FIELD_TEXT}, {"Datei:", "<leer>", Q9_LISTVIEW_FIELD_TEXT} },
    { {"Typ:", "<leer>", Q9_LISTVIEW_FIELD_TEXT}, {"Bus:", "<leer>", Q9_LISTVIEW_FIELD_TEXT},
      {"Unit:", "<leer>", Q9_LISTVIEW_FIELD_TEXT}, {"Datei:", "<leer>", Q9_LISTVIEW_FIELD_TEXT} },
    { {"Typ:", "<leer>", Q9_LISTVIEW_FIELD_TEXT}, {"Bus:", "<leer>", Q9_LISTVIEW_FIELD_TEXT},
      {"Unit:", "<leer>", Q9_LISTVIEW_FIELD_TEXT}, {"Datei:", "<leer>", Q9_LISTVIEW_FIELD_TEXT} },
};

static const q9_listview_item_t g_list_items[] = {
    { "Emulator-Konfiguration",     g_cfg_fields,    6 },
    { "CF-Interface (onboard, c0)", g_cf_fields,     4 },
    { "Netz-Terminal x1",           g_net1_fields,   4 },
    { "Netz-Terminal x2",           g_net2_fields,   4 },
    { "Netz-Terminal x3",           g_net3_fields,   4 },
    { "Netz-Terminal x4",           g_net4_fields,   4 },
    { "Netz-Terminal x5",           g_net5_fields,   4 },
    { "Netz-Terminal x6",           g_net6_fields,   4 },
    { "Netz-Terminal x7",           g_net7_fields,   4 },
    { "Netz-Terminal x8",           g_net8_fields,   4 },
    { "RTC72421 (Echtzeituhr)",     g_rtc_fields,    4 },
    { "DUART 68681 (Konsole)",      g_duart_fields,  4 },
    { "QUICC-Ethernet",             g_quicc_fields,  3 },
    { "MC6845 (GDP/CRTC)",          g_mc6845_fields, 4 },
    { "CLUT (Farbtabelle)",         g_clut_fields,   4 },
    { "RC2014-CF (sekundaer)",      g_rc2014_fields, 4 },
    { "Framebuffer (VRAM)",         g_fb_fields,     4 },
    { "Systemspeicher (RAM)",       g_ram_fields,    4 },
    { "ROM-Spiegel",                g_rom_fields,    4 },
    { "NVRAM (Akku-gepuffert)",     g_nvram_fields,  4 },
    { "Timer/IRQ3-Trigger",         g_timer_fields,  4 },
    /* CF-Image #0..#3 (s. Kommentar oben) -- IMMER am Ende, nur die ersten g_item_count-
       BASE_ITEM_COUNT davon sind tatsaechlich sichtbar/erreichbar, s. dort. */
    { "CF-Image #0", g_cfimg_fields[0], 4 },
    { "CF-Image #1", g_cfimg_fields[1], 4 },
    { "CF-Image #2", g_cfimg_fields[2], 4 },
    { "CF-Image #3", g_cfimg_fields[3], 4 },
};
/* MAX_ITEM_COUNT: volle Array-Kapazitaet (Compile-Zeit-Konstante fuer g_expanded[]s Groesse, s.
   unten). BASE_ITEM_COUNT: Emulator-Konfiguration + alle hardcodierten Hardware-Demo-Eintraege,
   OHNE die vier CF-Image-Slots am Ende. g_item_count (Laufzeit-Variable, s. unten) ist die
   tatsaechlich AKTIVE Anzahl -- startet bei BASE_ITEM_COUNT (kein Datei geladen, keine CF-Image-
   Slots sichtbar) und waechst nach einem erfolgreichen Laden um g_loaded_cfg.cf_count (0-4),
   s. load_q9_config_fields(). */
#define MAX_ITEM_COUNT  (int)(sizeof(g_list_items) / sizeof(g_list_items[0]))
#define BASE_ITEM_COUNT (MAX_ITEM_COUNT - Q9_LISTVIEW_CFIMG_SLOTS)

/* 0 = zugeklappt (Default), 1 = aufgeklappt -- Pfeil rechts auf der Hauptliste klappt den
   AUSGEWAEHLTEN Eintrag auf und setzt den Feld-Fokus aufs erste Feld (q9_listview_field_enter()),
   mehrere gleichzeitig aufgeklappte Eintraege sind ausdruecklich erlaubt (kein "nur einer offen"-
   Akkordeon -- einfacher zu verstehen, kein ueberraschendes Zuklappen anderer Eintraege). Feste
   Groesse MAX_ITEM_COUNT (nicht g_item_count) -- unsichtbare CF-Image-Slots brauchen trotzdem
   einen gueltigen Speicherplatz (q9_listview_field_enter() darf sie theoretisch adressieren,
   auch wenn main() sie nie erreicht, solange g_item_count kleiner ist). */
static int g_expanded[MAX_ITEM_COUNT];

/* Laufzeit-Anzahl der AKTIVEN Eintraege (s. Kommentar bei BASE_ITEM_COUNT oben) -- startet ohne
   geladene Datei bei BASE_ITEM_COUNT, load_q9_config_fields() passt sie nach jedem Laden an. */
static int g_item_count = BASE_ITEM_COUNT;

/* Warme Gelb-/Orange-Palette. Neunte Feedback-Runde (Andreas, 2026-08-17): "die ganzen Farben
   sind jetzt alle so in Richtung Braun abgerutscht... mehr in Richtung gelb orange" -- alle Toene
   HIER auf denselben Farbton (~42 Grad, Orange-Gelb) und dieselbe hohe Saettigung (~82%)
   umgerechnet, nur die HELLIGKEIT (V) unterscheidet die Ebenen -- eine durchgehende Leiter von
   ganz dunkel (Tabellenhintergrund) bis ganz hell (Kopfzeile/Auswahl), damit der Sprung zwischen
   den Ebenen klarer wirkt ("etwas mehr Kontrast"), statt wie zuvor nur unterschiedlich HELLE
   BRAUNTOENE zu sein:
     V=0.15  PAL_DIALOG_BODY_BG (Tabellenhintergrund -- "noch etwas dunkler")
     V=0.32  PAL_DIALOG_FOOTER_BG / PAL_STATUS_BG ("der untere Teil")
     V=0.44  PAL_DIALOG_SUB_BG (Tabellenkopf/Spaltentitel-Zeile, Button-Grundfarbe)
     V=0.72  PAL_FRAME (Rahmenlinien im Hauptfenster + Text auf PAL_STATUS_BG)
     V=0.80  PAL_HEADER_BG
     V=0.90  PAL_SEL_BG (hellster, kraeftigster Farbton -- die Auswahl-Hervorhebung)

   Zehnte Feedback-Runde, selber Tag: "in den beiden Kopfzeile das weiss auf dem hellen Gelb...
   im Tabellenkopf und unter der Tabelle zu wenig Kontrast" -- zwei zusaetzliche Text-Sonderfaelle,
   die NICHT einfach denselben V-Schritt wie ihr Hintergrund bekommen koennen (das waere ja gerade
   der Kontrast-Verlust), sondern bewusst ans jeweils ANDERE Ende der Leiter geholt werden:
   PAL_HEADER_FG jetzt so dunkel wie PAL_DIALOG_BODY_BG (dunkler Text auf hellem PAL_HEADER_BG,
   statt fast-weiss), PAL_DIALOG_SUB_FG (NEU) so hell wie PAL_HEADER_FG vorher war (heller Text auf
   dem mittelhellen PAL_DIALOG_SUB_BG). */
#define PAL_FRAME_R      184
#define PAL_FRAME_G      138
#define PAL_FRAME_B       33
#define PAL_LIST_FG_R    217
#define PAL_LIST_FG_G    191
#define PAL_LIST_FG_B    130
#define PAL_SEL_FG_R      35
#define PAL_SEL_FG_G      25
#define PAL_SEL_FG_B      10
#define PAL_SEL_BG_R     230
#define PAL_SEL_BG_G     173
#define PAL_SEL_BG_B      41
#define PAL_STATUS_FG_R  230
#define PAL_STATUS_FG_G  212
#define PAL_STATUS_FG_B  178
#define PAL_STATUS_BG_R   82
#define PAL_STATUS_BG_G   62
#define PAL_STATUS_BG_B   15
#define PAL_HEADER_FG_R   38                                /* Andreas' Wunsch (2026-08-17, zehnte */
#define PAL_HEADER_FG_G   29                                 /* Runde): "weiss auf hellem Gelb ist  */
#define PAL_HEADER_FG_B    7                                 /* schlecht lesbar" -- jetzt dunkel    */
#define PAL_HEADER_BG_R  204                                /* etwas heller als PAL_STATUS_BG,    */
#define PAL_HEADER_BG_G  154                                 /* gleiche Farbfamilie                */
#define PAL_HEADER_BG_B   37
/* V=0.44 (derselbe Ton/Saettigung wie der Rest der Leiter, s.o.) -- bisher nur im Datei-Dialog
   verwendet (PAL_DIALOG_SUB_BG, Tabellenkopf/Buttons), hier fuer denselben Zweck im Hauptfenster:
   Kopfzeile eines AUFGEKLAPPTEN, aber nicht ausgewaehlten Listeneintrags (Andreas' Feedback,
   2026-08-18, siebzehnte Runde: "die Headerzeile geht ein wenig unter" -- ohne Auswahl sah eine
   aufgeklappte Kopfzeile bisher aus wie jede andere, ging neben den gedaempften Detailzeilen
   darunter optisch unter). Bewusst DIESELBEN Zahlen wie PAL_DIALOG_SUB_BG (keine neue Sprosse auf
   der Leiter noetig), aber als eigene Konstante benannt -- der Datei-Dialog und diese Liste sind
   unabhaengige Aufrufer, sollen aber nicht zufaellig aneinander gekoppelt sein. */
#define PAL_LIST_EXP_BG_R  112
#define PAL_LIST_EXP_BG_G   85
#define PAL_LIST_EXP_BG_B   20

#define MIN_ROWS 20                                        /* Andreas' Wunsch (2026-08-17):     */
#define MIN_COLS 60                                         /* darunter sieht es "sehr komisch"
                                                                aus -- Hinweis statt Versuch      */
#define RESIZE_SETTLE_MS 1000                               /* Andreas' Wunsch: waehrend eines
                                                                Resizes nur die Groesse zeigen,
                                                                nach 1s Stille zurueck zum Inhalt  */

/* Feste Feldbreiten fuer die Statuszeile (Andreas' Wunsch: "sonst huepfen die Texte hin und her").
   NAME_FIELD_WIDTH >= der laengste Name in g_list_items ("CF-Interface (onboard, c0)" = 27 Zeichen). */
#define NAME_FIELD_WIDTH 30

/* Zusaetzliche Toene NUR fuer den Datei-Auswahl-Dialog -- Teil derselben Gelb-/Orange-Leiter wie
   oben (gleicher Farbton/Saettigung, s. dortiger Kopfkommentar), V=0.15 (dunkelster Schritt, "der
   Tabellenhintergrund bitte noch etwas dunkler"). */
#define PAL_DIALOG_BODY_BG_R  38
#define PAL_DIALOG_BODY_BG_G  29
#define PAL_DIALOG_BODY_BG_B   7
/* Tabellenkopf/Spaltentitel-Zeile + Button-Grundfarbe, V=0.44 -- deutlicher Sprung gegenueber
   PAL_DIALOG_BODY_BG (V=0.15) UND gegenueber PAL_DIALOG_FOOTER_BG (V=0.32) fuer "etwas mehr
   Kontrast" (Andreas' Wunsch, 2026-08-17). */
#define PAL_DIALOG_SUB_BG_R  112
#define PAL_DIALOG_SUB_BG_G   85
#define PAL_DIALOG_SUB_BG_B   20
/* TEXT auf PAL_DIALOG_SUB_BG ("Name Datum Groesse", das Namens-Kaestchen, OK/Abbrechen
   unfokussiert) -- Andreas' Feedback, zehnte Runde: "im Tabellenkopf... und OK/Abbrechen zu wenig
   Kontrast". Vorher wurde dafuer PAL_FRAME (V=0.72) verwendet -- zu nah an PAL_DIALOG_SUB_BG
   (V=0.44) fuer guten Kontrast, UND PAL_FRAME wird an anderer Stelle (Rahmenlinien, Hinweistext)
   bewusst NICHT geaendert (haette dort unerwuenschte Nebenwirkungen). Stattdessen ein eigener,
   heller Farbton NUR fuer diese Rolle -- derselbe Wert, den PAL_HEADER_FG bis zur letzten Runde
   hatte (fast-weiss, jetzt frei geworden, s.o.). */
#define PAL_DIALOG_SUB_FG_R  255
#define PAL_DIALOG_SUB_FG_G  248
#define PAL_DIALOG_SUB_FG_B  225
/* Fussbereich ("der untere Teil"), V=0.32 -- zwischen PAL_DIALOG_BODY_BG und PAL_DIALOG_SUB_BG,
   deutlich sichtbar anders als beide. Bewusst IDENTISCH mit PAL_STATUS_BG (Andreas' Wunsch:
   "den Footer des Dialogs in der Farbe des Hauptfensters machen") -- ueber die Konstante selbst
   referenziert statt nur zufaellig gleiche Zahlen zu haben, damit das auch so BLEIBT. */
#define PAL_DIALOG_FOOTER_BG_R  PAL_STATUS_BG_R
#define PAL_DIALOG_FOOTER_BG_G  PAL_STATUS_BG_G
#define PAL_DIALOG_FOOTER_BG_B  PAL_STATUS_BG_B

#define DIALOG_ROWS 18                                      /* Wunschgroesse -- wird in            */
#define DIALOG_COLS 56                                       /* run_file_dialog() an rows/cols geklemmt.
                                                                  18 statt 20 (2026-08-17, sechste Runde:
                                                                  "ich wollte noch zwei Zeilen sparen") --
                                                                  Buttons teilen sich jetzt Namens-/
                                                                  Filterzeile statt eigene Zeilen zu
                                                                  belegen, 2 Zeilen weniger noetig         */

static void write_ansi(unsigned (*fn)(char *, unsigned))
{
    char buf[32];
    unsigned n = fn(buf, sizeof(buf));
    fwrite(buf, 1, n, stdout);
}

static unsigned wrap_hide(char *b, unsigned n)  { return q9_ansi_hide_cursor(b, n); }
static unsigned wrap_show(char *b, unsigned n)  { return q9_ansi_show_cursor(b, n); }

/* q9_screenbuf_init() klemmt intern zwar schon auf Q9_SCREENBUF_MAX_ROWS/_COLS (s. dortiger
   Kopfkommentar) -- ohne diese Klemmung HIER wuerden rows/cols aber trotzdem noch die groesseren,
   unklemmten Werte fuer Layout-Berechnungen (draw_frame/listview-Geometrie) verwenden, was auf sehr
   grossen Terminals (>200 Spalten) zu einem Rahmen fuehren wuerde, der nicht bis zum tatsaechlichen
   rechten/unteren Rand des Puffers reicht -- kein Crash (q9_screenbuf_* klemmt selbst pro Zelle),
   aber ein unschoenes Layout. Deshalb hier vorsorglich mit klemmen. */
static void clamp_dims(int *rows, int *cols)
{
    if (*rows > Q9_SCREENBUF_MAX_ROWS) { *rows = Q9_SCREENBUF_MAX_ROWS; }
    if (*cols > Q9_SCREENBUF_MAX_COLS) { *cols = Q9_SCREENBUF_MAX_COLS; }
}

/* Ersetzt die fruehere separate "Fenster zu klein"-Meldung: EIN einheitliches Overlay fuer zwei
   Faelle -- (a) waehrend/kurz nach einer Groessenaenderung (too_small=0, wird nach
   RESIZE_SETTLE_MS Stille wieder durch den vollen Inhalt ersetzt) und (b) das Fenster ist
   (weiterhin) kleiner als die Mindestgroesse (too_small=1, bleibt dauerhaft sichtbar, zeigt
   zusaetzlich die Mindestgroesse). Beide Faelle sind bewusst DASSELBE einfache "nur die Groesse"-
   Layout -- Andreas: "sieht doof aus, wenn man immer versucht den kompletten Inhalt darzustellen".

   Feste Position OVERLAY_ROW/OVERLAY_COL statt zentriert (Andreas' Feedback, 2026-08-17,
   fuenfzehnte Runde: "das duerfte auch nicht mehr so durch die Gegend huepfen") -- bei zentrierter
   Position haengt die Zeichenposition selbst von rows/cols ab, der Text "sprang" beim Ziehen also
   bei JEDEM Zwischenschritt an eine andere Bildschirmstelle. Feste Position bleibt stattdessen
   immer an derselben Stelle stehen (dieselbe Spalte wie Kopfzeilen-Titel/Hinweistext/Listenansicht,
   s. build_full_content()). */
#define OVERLAY_ROW 3
#define OVERLAY_COL 3
static void render_size_overlay(int rows, int cols, int too_small)
{
    q9_screenbuf_t sb;
    char out[4096];
    char line1[64];

    q9_screenbuf_init(&sb, rows, cols);

    /* Columns vor Rows (Andreas' Wunsch, 2026-08-17: "Columns und Rows solltest du bitte
       tauschen") -- entspricht auch der ueblichen "80x24"-Schreibweise (Spalten x Zeilen). */
    snprintf(line1, sizeof(line1), "%d Columns - %d Rows", cols, rows);
    /* PAL_STATUS_FG statt PAL_HEADER_FG (Andreas' Feedback, 2026-08-17): dieser Text steht auf
       KEINEM eigenen farbigen Hintergrund (nur q9_screenbuf_init(), also Terminal-Default,
       typischerweise dunkel) -- PAL_HEADER_FG wurde in der letzten Runde bewusst DUNKEL gemacht
       (Kontrast auf dem jetzt kraeftigen PAL_HEADER_BG), hier fehlte dieser Hintergrund also
       komplett und der Text war praktisch unsichtbar. PAL_STATUS_FG ist nach wie vor hell und
       genau fuer "Text ohne eigenen Hintergrund" gedacht. */
    q9_screenbuf_puts(&sb, OVERLAY_ROW, OVERLAY_COL, line1,
                       PAL_STATUS_FG_R, PAL_STATUS_FG_G, PAL_STATUS_FG_B);

    if (too_small) {
        char line2[64];
        snprintf(line2, sizeof(line2), "Fenster zu klein (mind. %dx%d)", MIN_COLS, MIN_ROWS);
        q9_screenbuf_puts(&sb, OVERLAY_ROW + 1, OVERLAY_COL, line2,
                           PAL_STATUS_FG_R, PAL_STATUS_FG_G, PAL_STATUS_FG_B);
    }

    {
        unsigned n = q9_screenbuf_render(&sb, 0, 0, out, sizeof(out));
        fwrite(out, 1, n, stdout);
        fflush(stdout);
    }
}

/* Baut den vollen Hauptbildschirm-Inhalt in sb auf (KEIN stdout-Schreiben) -- ausgelagert aus
   render_full_content(), damit run_file_dialog() denselben echten Hintergrund hinter dem Dialog
   zeigen kann (Andreas' Feedback, 2026-08-17: "auf volle Groesse hatte ich mir den nicht
   vorgestellt" -- der Dialog WAR schon immer nur DIALOG_ROWS x DIALOG_COLS gross, sah aber wie
   Vollbild aus, weil vorher der GESAMTE Bildschirm mit der Dialog-Hintergrundfarbe gefuellt wurde,
   statt den echten Hauptbildschirm dahinter stehen zu lassen -- jetzt behoben).
   hint: NULL/leer -> Standardtext ("Pfeiltasten: ..."); sonst wird STATTDESSEN hint angezeigt --
   dient der Demo dazu, das Ergebnis des Datei-Auswahl-Dialogs (task #22) sichtbar zu machen, ohne
   die Statuszeile selbst (feste Feldbreiten, s.o.) umbauen zu muessen. */
static void build_full_content(q9_screenbuf_t *sb, q9_listview_t *lv, int rows, int cols,
                                const char *hint)
{
    char status[256];

    q9_screenbuf_init(sb, rows, cols);
    q9_screenbuf_draw_frame(sb, 0, 0, rows, cols,
                             "Q9-Flux Editor -- Integrations-Demo",
                             PAL_FRAME_R, PAL_FRAME_G, PAL_FRAME_B);

    /* Kopfzeile UEBER DIE VOLLE BREITE (Andreas' Wunsch, 2026-08-17, analog zur Statuszeile) --
       ueberschreibt auch die beiden oberen Eckzeichen von draw_frame(), etwas heller als die
       Statuszeile (PAL_HEADER_* statt PAL_STATUS_*), damit man Kopf/Fuss auf einen Blick
       unterscheiden kann, aber in derselben Farbfamilie bleibt. Der Titeltext von draw_frame()
       wird hier mit demselben Text erneut geschrieben (draw_frame's eigene Titel-Platzierung wird
       durch fill_rect vollstaendig ueberschrieben) -- LINKSBUENDIG ab Spalte 3 statt zentriert
       (Andreas' Wunsch, 2026-08-17: "lass uns mal links versuchen, ab dem dritten Zeichen" --
       Spalte 3 passt auch zur Linksbuendigkeit von Listenansicht/Hinweistext weiter unten). */
    q9_screenbuf_fill_rect(sb, 0, 0, 1, cols, ' ',
                            PAL_HEADER_FG_R, PAL_HEADER_FG_G, PAL_HEADER_FG_B,
                            1, PAL_HEADER_BG_R, PAL_HEADER_BG_G, PAL_HEADER_BG_B);
    q9_screenbuf_puts(sb, 0, 3, "Q9-Flux Editor -- Integrations-Demo",
                       PAL_HEADER_FG_R, PAL_HEADER_FG_G, PAL_HEADER_FG_B);

    q9_screenbuf_puts(sb, rows - 2, 3,
                       (hint && hint[0]) ? hint
                                         : "Pfeiltasten: navigieren   Rechts: oeffnen+bearbeiten   "
                                           "Leertaste: ja/nein   Esc: schliessen   O: Datei oeffnen   "
                                           "S: speichern   Strg-C: beenden",
                       PAL_FRAME_R, PAL_FRAME_G, PAL_FRAME_B);

    lv->row    = 2;
    lv->col    = 3;
    lv->height = rows - 5;                                   /* Rand+Hinweis+Statuszeile/-kante s.u. */
    /* Bis zur rechten Rahmenkante des Fensters (Andreas' Wunsch, 2026-08-17, zweite Runde: "im
       Hauptfenster haben wir immer noch rechts die Fensterkante UND zusaetzlich die Bildlauf-
       leiste, das soll jetzt in einem sein" -- q9_listview's eigene rechte Spalte, s. q9_listview.c,
       liegt jetzt direkt AUF draw_frame()'s eigener rechter Kante statt 3 Spalten davor). */
    lv->width  = cols - lv->col;
    if (lv->height < 1) { lv->height = 1; }
    if (lv->width  < 1) { lv->width  = 1; }
    /* _ex statt der einfachen q9_listview_scroll() -- Eintraege koennen jetzt mehr als eine
       Bildschirmzeile brauchen (aufgeklappt), s. q9_listview.h. */
    lv->scroll_offset = q9_listview_scroll_ex(lv->selected, lv->scroll_offset, lv->height,
                                               g_list_items, g_expanded, lv->item_count);
    /* line_fg = PAL_FRAME (NICHT PAL_LIST_FG) -- die Linie liegt seit der dritten Feedback-Runde
       direkt AUF draw_frame()'s eigener Kante (s.o.), muss also auch DIESELBE Farbe zeigen, sonst
       wirken links/rechts UND verschiedene Hoehen der rechten Kante unterschiedlich eingefaerbt
       (Andreas' Feedback, 2026-08-17, siebte Runde: "die Striche links und rechts... sind
       unterschiedlich... das oberste rechts ist noch mal anders" -- die Eckzeichen/Kanten VOR und
       NACH dem Listenbereich blieben in PAL_FRAME, waehrend die Liste selbst bisher PAL_LIST_FG
       zeichnete, obwohl beide auf derselben Spalte liegen). detail_fg = PAL_FRAME ebenfalls -- die
       eingerueckten Detailzeilen bekommen dieselbe gedaempfte Farbe wie der Hinweistext unten
       (strukturell/sekundaer, nicht der "wichtige" Text wie der Eintragsname selbst). exp_bg =
       PAL_LIST_EXP_BG -- Kopfzeile eines aufgeklappten, nicht ausgewaehlten Eintrags (Andreas'
       Feedback, siebzehnte Runde: "die Headerzeile geht ein wenig unter"). box_fg/bg =
       PAL_DIALOG_SUB_FG/PAL_DIALOG_SUB_BG -- fuer den TEXT+BUTTON-Sonderfall (Wert-Box +
       dreizeiliger Button, s. q9_listview.h) BEWUSST dieselben Konstanten wie das Namens-Kaestchen
       und die OK/Abbrechen-Buttons im Datei-Dialog (Andreas' Wunsch, 2026-08-18, einundzwanzigste
       Runde: "mit der Darstellung wie im Dialog"). */
    q9_listview_render_ex(lv, sb, g_list_items, g_expanded,
                           PAL_LIST_FG_R, PAL_LIST_FG_G, PAL_LIST_FG_B,
                           PAL_SEL_FG_R, PAL_SEL_FG_G, PAL_SEL_FG_B,
                           PAL_SEL_BG_R, PAL_SEL_BG_G, PAL_SEL_BG_B,
                           PAL_FRAME_R, PAL_FRAME_G, PAL_FRAME_B,
                           PAL_FRAME_R, PAL_FRAME_G, PAL_FRAME_B,
                           PAL_LIST_EXP_BG_R, PAL_LIST_EXP_BG_G, PAL_LIST_EXP_BG_B,
                           PAL_DIALOG_SUB_FG_R, PAL_DIALOG_SUB_FG_G, PAL_DIALOG_SUB_FG_B,
                           PAL_DIALOG_SUB_BG_R, PAL_DIALOG_SUB_BG_G, PAL_DIALOG_SUB_BG_B);

    /* Statuszeile ALS untere Rahmenkante, ueber die volle Breite (vorherige Feedback-Runden) --
       jetzt zusaetzlich mit FESTEN Feldbreiten (Andreas: "sonst huepfen die Texte hin und her"):
       der Eintragsname wird auf NAME_FIELD_WIDTH Zeichen aufgefuellt (linksbuendig), die
       Terminal-Groesse mit fester Breite je Zahl (rows dreistellig rechtsbuendig, cols dreistellig
       linksbuendig) -- "Terminal:" steht dadurch bei jeder Auswahl/Groesse an derselben Spalte. */
    q9_screenbuf_fill_rect(sb, rows - 1, 0, 1, cols, ' ',
                            PAL_STATUS_FG_R, PAL_STATUS_FG_G, PAL_STATUS_FG_B,
                            1, PAL_STATUS_BG_R, PAL_STATUS_BG_G, PAL_STATUS_BG_B);
    snprintf(status, sizeof(status), " Ausgewaehlt: %-*.*s | Terminal: %3dx%-3d",
             NAME_FIELD_WIDTH, NAME_FIELD_WIDTH,
             (lv->selected >= 0 && lv->selected < g_item_count) ? g_list_items[lv->selected].name : "-",
             rows, cols);
    q9_screenbuf_puts(sb, rows - 1, 1, status, PAL_STATUS_FG_R, PAL_STATUS_FG_G, PAL_STATUS_FG_B);
}

static void render_full_content(q9_listview_t *lv, int rows, int cols, const char *hint)
{
    q9_screenbuf_t sb;
    char out[1 << 16];
    unsigned n;

    build_full_content(&sb, lv, rows, cols, hint);
    n = q9_screenbuf_render(&sb, 0, 0, out, sizeof(out));
    fwrite(out, 1, n, stdout);
    fflush(stdout);
}

/* Berechnet dlg_row/dlg_col/dlg_rows/dlg_cols aus der aktuellen Terminal-Groesse -- IMMER zentriert
   (Andreas' Wunsch, 2026-08-17, elfte Runde: "waere auch gut wenn der Dialog nach dem
   Positionieren immer wieder mittig positioniert wird"). Ausgelagert aus run_file_dialog(), damit
   dieselbe Formel sowohl beim ersten Oeffnen ALS AUCH bei einem Resize waehrend der Dialog offen
   ist verwendet wird (s. dort) -- ein Resize zentriert den Dialog dadurch automatisch neu. */
static void compute_dialog_geometry(int rows, int cols, int *dlg_row, int *dlg_col,
                                     int *dlg_rows, int *dlg_cols)
{
    *dlg_rows = DIALOG_ROWS;
    *dlg_cols = DIALOG_COLS;
    if (*dlg_rows > rows - 2) { *dlg_rows = rows - 2; }
    if (*dlg_cols > cols - 2) { *dlg_cols = cols - 2; }
    *dlg_row = (rows - *dlg_rows) / 2;
    *dlg_col = (cols - *dlg_cols) / 2;
    if (*dlg_row < 1) { *dlg_row = 1; }
    if (*dlg_col < 1) { *dlg_col = 1; }
}

/* ~/.q9-flux -- das ECHTE Zielverzeichnis fuer Config-Dateien (Andreas' Wunsch, 2026-08-18,
   zwanzigste Runde), wird bei Bedarf angelegt (mkdir, Fehler bewusst ignoriert -- existiert es
   schon, ist das kein Problem). Kein HOME gesetzt -> Rueckfall auf ".". Ausgelagert aus
   run_file_dialog() (fuenfundzwanzigste Runde), damit load_q9_config_fields() unten DIESELBE
   Verzeichnisermittlung verwendet -- der Dateiname, den der Dialog liefert, ist relativ zu genau
   diesem Verzeichnis, nicht zum CWD. */
static void q9flux_dir(char *out, unsigned out_size)
{
    const char *home = getenv("HOME");
    if (home) {
        snprintf(out, out_size, "%s/.q9-flux", home);
        mkdir(out, 0755);
    } else {
        snprintf(out, out_size, ".");
    }
}

/* Sechsundzwanzigste Runde ("Speichern-Funktion"): die zuletzt erfolgreich GELADENE Konfiguration
   bleibt hier vollstaendig erhalten (inkl. [cfN]-Abschnitte, die der Editor selbst noch nicht
   anzeigt/bearbeitet) -- save_q9_config() startet beim Speichern davon (statt bei leeren Defaults)
   und ueberschreibt nur die vier Felder, die der Editor tatsaechlich zeigt. Ohne das wuerden beim
   Speichern einer geladenen Datei ihre [cfN]-Abschnitte stillschweigend verschwinden.
   g_cfg_loaded==0 bedeutet "noch nie erfolgreich geladen (oder letzter Ladeversuch ist
   fehlgeschlagen)" -- Speichern startet dann bei q9_board_cfg_default() (frische Config, kein
   [cfN]), deckt den "neue Config von Grund auf"-Fall ab. */
static q9_board_cfg_t g_loaded_cfg;
static int             g_cfg_loaded = 0;

/* Wandeln q9_cfg_cf_t.format/bus/unit (Enum-Werte) in genau die Kurztexte um, die auch die
   .q9-Datei selbst verwendet (rbf/pcf/auto, onboard/rc2014, master/slave) -- fuer die Anzeige in
   den CF-Image-Feldern (Typ:/Bus:/Unit:). Die Umkehrung (String -> Enum, fuers Speichern) s.
   parse_cf_format()/_bus()/_unit() bei save_q9_config(). BEWUSST hier dupliziert statt
   boardcfg.c's private cfg_parse_*()-Helfer freizulegen (die sind absichtlich `static`, kein Teil
   der oeffentlichen boardcfg.h-API) -- die Duplikation ist klein und risikoarm (feste, seit
   langem stabile Wertelisten), im Unterschied zur vollen INI-Syntax (Kommentare/Abschnitte/
   Escaping), die deshalb bewusst NICHT dupliziert wird (s. Kommentar bei den #include-Zeilen
   oben). Ein ungueltiger String wuerde beim naechsten Laden ohnehin vom echten Parser abgelehnt
   (klare Fehlermeldung) -- hier daher keine eigene Validierung noetig. */
static const char *cf_format_str(int format)
{
    if (format == Q9_CF_FMT_RBF) { return "rbf"; }
    if (format == Q9_CF_FMT_PCF) { return "pcf"; }
    return "auto";
}
static const char *cf_bus_str(int bus)  { return bus == Q9_CFG_BUS_RC2014 ? "rc2014" : "onboard"; }
static const char *cf_unit_str(int unit) { return unit ? "slave" : "master"; }

/* Traegt die CF-Image-Felder (g_cfimg_fields[0..g_loaded_cfg.cf_count-1]) aus g_loaded_cfg.cf[]
   ein und richtet lv NEU aus (item_count/g_item_count wachsen um cf_count, s. Kommentar bei
   BASE_ITEM_COUNT). lv darf NULL sein (z.B. wenn kein Listview-Kontext verfuegbar ist) -- dann
   werden nur die Felder befuellt, ohne Sichtbarkeits-/Fokus-Anpassung. */
static void sync_cfimg_items(q9_listview_t *lv)
{
    int i;
    for (i = 0; i < g_loaded_cfg.cf_count && i < Q9_LISTVIEW_CFIMG_SLOTS; i++) {
        const q9_cfg_cf_t *cf = &g_loaded_cfg.cf[i];
        snprintf(g_cfimg_fields[i][0].value, sizeof(g_cfimg_fields[i][0].value), "%s",
                 cf_format_str(cf->format));
        snprintf(g_cfimg_fields[i][1].value, sizeof(g_cfimg_fields[i][1].value), "%s",
                 cf_bus_str(cf->bus));
        snprintf(g_cfimg_fields[i][2].value, sizeof(g_cfimg_fields[i][2].value), "%s",
                 cf_unit_str(cf->unit));
        snprintf(g_cfimg_fields[i][3].value, sizeof(g_cfimg_fields[i][3].value), "%s",
                 cf->path[0] ? cf->path : "<leer>");
    }
    g_item_count = BASE_ITEM_COUNT + (g_loaded_cfg.cf_count < Q9_LISTVIEW_CFIMG_SLOTS
                                       ? g_loaded_cfg.cf_count : Q9_LISTVIEW_CFIMG_SLOTS);
    if (lv) {
        lv->item_count = g_item_count;
        if (lv->selected >= g_item_count) { lv->selected = g_item_count - 1; }
        q9_listview_move_ex(lv, 0, g_list_items, g_expanded);   /* Scroll-Offset neu ausrichten */
    }
}

/* Andreas' Wunsch (2026-08-18, fuenfundzwanzigste Runde): "echtes Laden/Auswerten der .q9-Datei"
   -- ruft den ECHTEN Board-Config-Parser des Emulators auf (src/kernel/boardcfg.c/.h, s. Include
   oben), KEIN zweiter, eigener INI-Parser hier. Fuellt Name:/ROM:/Netz:/CPU: (g_cfg_fields[2..5],
   FESTE Positionen -- s. Kommentar dort) aus der geladenen Datei; leere Config-Werte werden als
   "<leer>" angezeigt (unterscheidet "im Feld steht nichts" von "wurde noch nie geladen" nicht
   extra -- beides sieht fuer den Nutzer gleich aus, das ist in dieser Runde bewusst so einfach
   gehalten). Siebenundzwanzigste Runde: zusaetzlich sync_cfimg_items() -- die [cfN]-Abschnitte
   der Datei werden jetzt als eigene CF-Image-#N-Eintraege sichtbar (s. Kommentar bei
   g_cfimg_fields). Schlaegt das Laden fehl (kaputte/unlesbare Datei), werden alle vier Felder auf
   "<Fehler>" gesetzt und msg traegt die genaue Fehlermeldung (inkl. Zeilennummer, s.
   q9_board_cfg_load()); g_cfg_loaded bleibt/wird 0 (kein Speichern auf Basis einer kaputten
   Ladung, s. save_q9_config()) UND vorhandene CF-Image-Eintraege verschwinden wieder (cf_count
   wird von q9_board_cfg_default() auf 0 zurueckgesetzt). */
static void load_q9_config_fields(const char *filename, char *msg, unsigned msg_size,
                                   q9_listview_t *lv)
{
    char dir[512];
    char path[Q9_CFG_PATH_MAX];
    char err[160];

    q9flux_dir(dir, sizeof(dir));
    snprintf(path, sizeof(path), "%s/%s", dir, filename);

    q9_board_cfg_default(&g_loaded_cfg);
    if (q9_board_cfg_load(&g_loaded_cfg, path, err, sizeof(err)) != 0) {
        g_cfg_loaded = 0;
        snprintf(g_cfg_fields[2].value, sizeof(g_cfg_fields[2].value), "<Fehler>");
        snprintf(g_cfg_fields[3].value, sizeof(g_cfg_fields[3].value), "<Fehler>");
        snprintf(g_cfg_fields[4].value, sizeof(g_cfg_fields[4].value), "<Fehler>");
        snprintf(g_cfg_fields[5].value, sizeof(g_cfg_fields[5].value), "<Fehler>");
        sync_cfimg_items(lv);                                   /* cf_count==0 -- Slots verschwinden */
        snprintf(msg, msg_size, "Laden fehlgeschlagen: %s", err);
        return;
    }
    g_cfg_loaded = 1;
    snprintf(g_cfg_fields[2].value, sizeof(g_cfg_fields[2].value), "%s",
             g_loaded_cfg.name[0]     ? g_loaded_cfg.name     : "<leer>");
    snprintf(g_cfg_fields[3].value, sizeof(g_cfg_fields[3].value), "%s",
             g_loaded_cfg.rom_path[0] ? g_loaded_cfg.rom_path : "<leer>");
    snprintf(g_cfg_fields[4].value, sizeof(g_cfg_fields[4].value), "%s",
             g_loaded_cfg.net_mode[0] ? g_loaded_cfg.net_mode : "<leer>");
    snprintf(g_cfg_fields[5].value, sizeof(g_cfg_fields[5].value), "%s",
             g_loaded_cfg.cpu[0]      ? g_loaded_cfg.cpu      : "<leer>");
    sync_cfimg_items(lv);
    snprintf(msg, msg_size, "Konfiguration geladen: %s", filename);
}

/* Uebernimmt field_value in out -- die Platzhalter "<leer>"/"<Fehler>" (reine ANZEIGE-Werte, s.
   load_q9_config_fields()) werden dabei als "nichts eingetragen" (leerer String) behandelt, alles
   andere woertlich uebernommen. Kleine Hilfsfunktion fuer save_q9_config() unten. */
static void field_to_cfg_str(const char *field_value, char *out, unsigned out_max)
{
    if (strcmp(field_value, "<leer>") == 0 || strcmp(field_value, "<Fehler>") == 0) {
        out[0] = '\0';
    } else {
        snprintf(out, out_max, "%s", field_value);
    }
}

/* q9_cfg_cf_t.format/bus/unit sind Enum-Werte -- kehrt cf_format_str()/_bus_str()/_unit_str() um
   (fuers Speichern der CF-Image-Felder Typ:/Bus:/Unit:, s. save_q9_config()). fallback greift bei
   unbekanntem Text (z.B. Tippfehler) -- BEHAELT den vorherigen (aus g_loaded_cfg uebernommenen)
   Wert bei, statt auf einen willkuerlichen Default zu springen; ein wirklich falscher Wert faellt
   spaetestens beim naechsten Laden auf (s. Kommentar bei cf_format_str() oben). */
static int parse_cf_format(const char *v, int fallback)
{
    if (strcmp(v, "rbf") == 0)  { return Q9_CF_FMT_RBF; }
    if (strcmp(v, "pcf") == 0)  { return Q9_CF_FMT_PCF; }
    if (strcmp(v, "auto") == 0) { return Q9_CF_FMT_AUTO; }
    return fallback;
}
static int parse_cf_bus(const char *v, int fallback)
{
    if (strcmp(v, "onboard") == 0) { return Q9_CFG_BUS_ONBOARD; }
    if (strcmp(v, "rc2014") == 0)  { return Q9_CFG_BUS_RC2014; }
    return fallback;
}
static int parse_cf_unit(const char *v, int fallback)
{
    if (strcmp(v, "master") == 0) { return 0; }
    if (strcmp(v, "slave") == 0)  { return 1; }
    return fallback;
}

/* Andreas' Wunsch (2026-08-18, sechsundzwanzigste Runde): "Speichern-Funktion" -- schreibt die
   AKTUELLEN Feldwerte (Name:/ROM:/Netz:/CPU:, egal ob von Hand editiert oder aus einem vorigen
   Laden uebernommen) in die Datei zurueck, die im Datei:-Feld steht. Startet dabei bei
   g_loaded_cfg (falls vorhanden, s. Kommentar dort) statt bei leeren Defaults, damit unbekannte
   [cfN]-Felder (Basis/Slot/Descriptor -- der Editor zeigt/bearbeitet die noch nicht, s. "Noch
   offen" in Q9FLUX_EDITOR_de.md) NICHT stillschweigend verschwinden. Siebenundzwanzigste Runde:
   Typ:/Bus:/Unit:/Datei: DER SICHTBAREN CF-Image-Eintraege (g_item_count-BASE_ITEM_COUNT Stueck)
   werden zusaetzlich zurueckgeschrieben (cfg.cf_count entsprechend gesetzt) -- ALLES darueber
   hinaus (weitere, nie geladene/sichtbare Slots) bleibt unberuehrt und wird NICHT mitgespeichert.
   Ruft (wie load_q9_config_fields()) den ECHTEN Board-Config-Parser des Emulators auf
   (q9_board_cfg_save(), src/kernel/boardcfg.c/.h) -- KEIN zweiter, eigener Serialisierer hier. */
static void save_q9_config(char *msg, unsigned msg_size)
{
    q9_board_cfg_t cfg;
    char dir[512];
    char path[Q9_CFG_PATH_MAX];
    char err[160];
    const char *filename = g_cfg_fields[0].value;
    int i, cf_visible;

    if (filename[0] == '\0' || strcmp(filename, "<leer>") == 0) {
        snprintf(msg, msg_size,
                 "Speichern: keine Datei ausgewaehlt (erst 'Datei' waehlen oder Namen eintippen)");
        return;
    }

    if (g_cfg_loaded) {
        cfg = g_loaded_cfg;
    } else {
        q9_board_cfg_default(&cfg);
    }
    field_to_cfg_str(g_cfg_fields[2].value, cfg.name,     sizeof(cfg.name));
    field_to_cfg_str(g_cfg_fields[3].value, cfg.rom_path, sizeof(cfg.rom_path));
    field_to_cfg_str(g_cfg_fields[4].value, cfg.net_mode, sizeof(cfg.net_mode));
    field_to_cfg_str(g_cfg_fields[5].value, cfg.cpu,      sizeof(cfg.cpu));

    /* cf_visible == cfg.cf_count per Konstruktion (sync_cfimg_items() haelt g_item_count IMMER
       exakt in dieser Beziehung zu g_loaded_cfg.cf_count, s. dort) -- trotzdem explizit gesetzt,
       nicht stillschweigend vorausgesetzt (robust, falls sich das je aendert, z.B. ein kuenftiges
       "neuen CF-Image-Slot hinzufuegen"). */
    cf_visible = g_item_count - BASE_ITEM_COUNT;
    cfg.cf_count = cf_visible;
    for (i = 0; i < cf_visible; i++) {
        q9_cfg_cf_t *cf = &cfg.cf[i];
        cf->format = parse_cf_format(g_cfimg_fields[i][0].value, cf->format);
        cf->bus    = parse_cf_bus(g_cfimg_fields[i][1].value, cf->bus);
        cf->unit   = parse_cf_unit(g_cfimg_fields[i][2].value, cf->unit);
        field_to_cfg_str(g_cfimg_fields[i][3].value, cf->path, sizeof(cf->path));
    }

    q9flux_dir(dir, sizeof(dir));
    snprintf(path, sizeof(path), "%s/%s", dir, filename);

    if (q9_board_cfg_save(&cfg, path, err, sizeof(err)) != 0) {
        snprintf(msg, msg_size, "Speichern fehlgeschlagen: %s", err);
        return;
    }
    snprintf(msg, msg_size, "Konfiguration gespeichert: %s", filename);
}

/* Oeffnet den modalen Datei-Auswahl-Dialog (q9_filedialog.h/.c, task #20) zentriert ueber dem
   aktuellen Bildschirm, scannt ~/.q9-flux (wird bei Bedarf angelegt) mit *.q9 als Standardfilter
   (Andreas' Wunsch, 2026-08-18, zwanzigste Runde) -- das ECHTE Zielverzeichnis fuer
   Config-Dateien, kein reines Demo-Verzeichnis mehr. Die Config-DATEN selbst (die einzelnen
   Hardware-Eintraege/-Felder) bleiben weiterhin reine Vorfuehrdaten, s. Kopfkommentar.
   Der Dialog ist bewusst NUR DIALOG_ROWS x DIALOG_COLS gross (Andreas' Feedback, 2026-08-17:
   "auf volle Groesse hatte ich mir den jetzt nicht vorgestellt") -- als Hintergrund steht der
   ECHTE Hauptbildschirm (ueber build_full_content(), s.o.), nicht mehr eine reine Fuellfarbe ueber
   den ganzen Schirm (das war der eigentliche Grund, warum es vorher wie Vollbild aussah, obwohl
   der Dialog selbst schon immer klein war).
   rows/cols sind jetzt Zeiger (nicht mehr nur Eingabe) -- ein Terminal-Resize WAEHREND der Dialog
   offen ist wird jetzt behandelt (Andreas' Feedback, elfte Runde: "Fenster Groesse aendern
   waehrend ein Dialog auf ist funktioniert nicht richtig"). ZWEITER Anlauf, fuenfzehnte Runde
   (Andreas: "geht immer noch nicht, da wird immer alles neu gezeichnet") -- der ERSTE Anlauf
   (vierzehnte Runde) hat bei JEDEM einzelnen Q9_KEY_RESIZE (waehrend des Ziehens an der
   Terminal-Ecke kommen davon viele kurz hintereinander, s. main()) sofort den KOMPLETTEN Dialog
   per q9_filedialog_init() + vollem Redraw neu aufgebaut -- teuer und sichtbar ruckelig/
   flackernd bei jedem Zwischenschritt. Jetzt dasselbe Overlay-Settle-Muster wie main() (s.
   RESIZE_SETTLE_MS oben): waehrend gezogen wird, nur das billige render_size_overlay() (fixe
   Position, kein Dialog-Redraw); q9_term_size() wird pro Q9_KEY_RESIZE-Ereignis neu abgefragt,
   aber der teure Dialog-Neuaufbau (compute_dialog_geometry() + q9_filedialog_init(), zentriert
   dabei automatisch neu -- erledigt den Wunsch "immer wieder mittig") passiert erst EINMAL, nach
   RESIZE_SETTLE_MS Stille. DEMO-GRENZE: das Neu-Initialisieren setzt Fokus/Auswahl/Filter auf ihre
   Startwerte zurueck (kein Nachziehen des BISHERIGEN Zustands) -- ein echter Editor wuerde hier
   gezielter nur die Geometrie aktualisieren; fuer die Demo ist das ein akzeptabler Kompromiss
   (Resize mitten in der Dateiauswahl ist ein Randfall). Die Zeiger werden aktualisiert, damit der
   Aufrufer nach Rueckkehr die dann aktuelle Terminal-Groesse kennt. Strg-C/EOF waehrend des Dialogs
   wird NICHT ignoriert (sonst liesse sich das Programm aus dem Dialog heraus nicht mehr beenden) --
   signalisiert per Rueckgabe 0 an den Aufrufer, der dann seinerseits sauber beendet.
   Rueckgabe: 1 = Datei ausgewaehlt (result_msg beschreibt sie), -1 = abgebrochen (result_msg
   entsprechend gesetzt), 0 = Strg-C/EOF (result_msg unveraendert -- Aufrufer beendet ohnehin).
   result_msg dient WAEHREND der Dialoglaufzeit zusaetzlich als "aktueller Hinweistext" fuer den
   Hintergrund (unveraendert bis zum Ende der Funktion) -- spart einen eigenen Parameter dafuer. */
static int run_file_dialog(int *rows, int *cols, q9_listview_t *lv,
                            char *result_msg, unsigned result_msg_size,
                            char *out_name, unsigned out_name_size)
{
    /* Andreas' Wunsch (2026-08-18, zwanzigste Runde): "als Verzeichnis sollte ~/.q9-flux gesetzt
       sein und dort sollen alle *.q9 Dateien angezeigt werden" -- kein reines Demo-Verzeichnis
       mehr, sondern das ECHTE Zielverzeichnis fuer Config-Dateien dieses Editors. ".q9" (OHNE
       fuehrenden Stern) als Standardfilter -- q9_filelist.c's ext_matches() erwartet eine reine
       Endung (mit oder ohne fuehrenden Punkt, s. dort), KEIN Glob-Muster wie "*.q9"; "*.q9" haette
       NIE getroffen (beim ersten Testlauf per pyte tatsaechlich aufgefallen: leere Liste trotz
       vorhandener .q9-Dateien). "*.*" bleibt als Ausweichoption im Dropdown (z.B. um zu sehen, ob
       ueberhaupt etwas im Verzeichnis liegt). Verzeichnis wird bei Bedarf angelegt (mkdir, Fehler
       bewusst ignoriert -- existiert es schon, ist das kein Problem; schlaegt es aus anderem Grund
       fehl, zeigt der Dialog einfach eine leere Liste, s. q9_filedialog.c rescan()). */
    static const char *const filters[] = { ".q9", "*.*" };
    char dir_buf[512];
    const char *dir = dir_buf;
    q9flux_dir(dir_buf, sizeof(dir_buf));
    q9_filedialog_palette_t pal;
    q9_filedialog_t dlg;
    int dlg_row, dlg_col, dlg_rows, dlg_cols;
    int done = 0;
    int want_quit = 0;
    int showing_overlay = 0;                                 /* wie main(): billiges Groessen-
                                                                 Overlay waehrend des Ziehens statt
                                                                 teurem Dialog-Redraw bei jedem
                                                                 Zwischenschritt, s.o. */

    compute_dialog_geometry(*rows, *cols, &dlg_row, &dlg_col, &dlg_rows, &dlg_cols);

    memset(&pal, 0, sizeof(pal));
    pal.header_fg_r = PAL_HEADER_FG_R; pal.header_fg_g = PAL_HEADER_FG_G; pal.header_fg_b = PAL_HEADER_FG_B;
    pal.header_bg_r = PAL_HEADER_BG_R; pal.header_bg_g = PAL_HEADER_BG_G; pal.header_bg_b = PAL_HEADER_BG_B;
    pal.sub_fg_r    = PAL_DIALOG_SUB_FG_R; pal.sub_fg_g = PAL_DIALOG_SUB_FG_G; pal.sub_fg_b = PAL_DIALOG_SUB_FG_B;
    pal.sub_bg_r    = PAL_DIALOG_SUB_BG_R;  pal.sub_bg_g = PAL_DIALOG_SUB_BG_G;  pal.sub_bg_b = PAL_DIALOG_SUB_BG_B;
    pal.body_fg_r   = PAL_LIST_FG_R;   pal.body_fg_g   = PAL_LIST_FG_G;   pal.body_fg_b   = PAL_LIST_FG_B;
    pal.body_bg_r   = PAL_DIALOG_BODY_BG_R; pal.body_bg_g = PAL_DIALOG_BODY_BG_G; pal.body_bg_b = PAL_DIALOG_BODY_BG_B;
    pal.list_fg_r   = PAL_LIST_FG_R;   pal.list_fg_g   = PAL_LIST_FG_G;   pal.list_fg_b   = PAL_LIST_FG_B;
    pal.sel_fg_r    = PAL_SEL_FG_R;    pal.sel_fg_g    = PAL_SEL_FG_G;    pal.sel_fg_b    = PAL_SEL_FG_B;
    pal.sel_bg_r    = PAL_SEL_BG_R;    pal.sel_bg_g    = PAL_SEL_BG_G;    pal.sel_bg_b    = PAL_SEL_BG_B;
    pal.focus_fg_r  = PAL_SEL_FG_R;    pal.focus_fg_g  = PAL_SEL_FG_G;    pal.focus_fg_b  = PAL_SEL_FG_B;
    pal.focus_bg_r  = PAL_SEL_BG_R;    pal.focus_bg_g  = PAL_SEL_BG_G;    pal.focus_bg_b  = PAL_SEL_BG_B;
    /* Andreas' Feedback (2026-08-17): "wenn der Selektor auf die Dateiauswahl steht sehe ich
       nichts" -- die markierte Zeile sah IMMER gleich aus, egal ob die Liste den Fokus hatte oder
       nicht (der Fokus-Wechsel war dadurch unsichtbar). Jetzt gedaempfter Ton, wenn die Liste
       NICHT fokussiert ist -- gleiche Grundfarbe wie die Spaltentitel-Zeile, aber lesbarer Text
       (statt komplett unsichtbarer Markierung), s. q9_filedialog.c list_focus-Unterscheidung. */
    pal.unfocus_sel_fg_r = PAL_LIST_FG_R; pal.unfocus_sel_fg_g = PAL_LIST_FG_G; pal.unfocus_sel_fg_b = PAL_LIST_FG_B;
    pal.unfocus_sel_bg_r = PAL_DIALOG_SUB_BG_R; pal.unfocus_sel_bg_g = PAL_DIALOG_SUB_BG_G; pal.unfocus_sel_bg_b = PAL_DIALOG_SUB_BG_B;
    /* Fussbereich (zweite Feedback-Runde, 2026-08-17) -- eigener Hintergrund ab der Statuszeile,
       s. PAL_DIALOG_FOOTER_BG_* oben. Text darauf in derselben Farbe wie der Dialog-Fliesstext. */
    pal.footer_fg_r = PAL_LIST_FG_R; pal.footer_fg_g = PAL_LIST_FG_G; pal.footer_fg_b = PAL_LIST_FG_B;
    pal.footer_bg_r = PAL_DIALOG_FOOTER_BG_R; pal.footer_bg_g = PAL_DIALOG_FOOTER_BG_G; pal.footer_bg_b = PAL_DIALOG_FOOTER_BG_B;
    /* Untere Statuszeile -- EXAKT dieselbe Farbe wie die Hauptfenster-Statuszeile (Andreas'
       Feedback, elfte Runde: "die Statuszeilen sind noch unterschiedlich"), ueber eigene
       status_fg/bg-Felder statt header_fg/bg (das bliebe sonst an die Dialog-Kopfzeile gekoppelt,
       s. q9_filedialog.h). */
    pal.status_fg_r = PAL_STATUS_FG_R; pal.status_fg_g = PAL_STATUS_FG_G; pal.status_fg_b = PAL_STATUS_FG_B;
    pal.status_bg_r = PAL_STATUS_BG_R; pal.status_bg_g = PAL_STATUS_BG_G; pal.status_bg_b = PAL_STATUS_BG_B;

    /* Urspruenglich (2026-08-17) nur HOME statt "." fuer den Test (mehr/andere Dateien als im
       leeren Demo-Arbeitsverzeichnis) -- seit der zwanzigsten Runde (2026-08-18) das ECHTE
       Zielverzeichnis ~/.q9-flux mit *.q9-Filter, s. dir/filters oben. */
    if (q9_filedialog_init(&dlg, dlg_row, dlg_col, dlg_rows, dlg_cols,
                            "Konfigurationsauswahl", dir, filters, 2, &pal) != 0) {
        snprintf(result_msg, result_msg_size, "Dateidialog: Fehler beim Start (O: erneut versuchen)");
        return -1;
    }

    for (;;) {
        int too_small = (*rows < MIN_ROWS || *cols < MIN_COLS);

        if (showing_overlay) {
            /* Billig: nur die Groesse anzeigen, KEIN Dialog-/Hintergrund-Redraw (s.o.). */
            render_size_overlay(*rows, *cols, too_small);
        } else {
            q9_screenbuf_t sb;
            char out[1 << 16];

            /* Echter Hauptbildschirm als Hintergrund (s. Funktionskommentar) -- der Dialog selbst
               ueberschreibt danach nur sein EIGENES Rechteck (q9_filedialog_render() faengt mit
               seinem eigenen fill_rect ueber dlg->row/col/rows/cols an), der Rest bleibt sichtbar. */
            build_full_content(&sb, lv, *rows, *cols, result_msg);
            q9_filedialog_render(&dlg, &sb);
            {
                unsigned n = q9_screenbuf_render(&sb, 0, 0, out, sizeof(out));
                fwrite(out, 1, n, stdout);
                fflush(stdout);
            }
        }

        {
            q9_key_t k = showing_overlay ? q9_input_read_key_timeout(RESIZE_SETTLE_MS)
                                          : q9_input_read_key();

            if (showing_overlay && k.kind == Q9_KEY_NONE) {
                /* RESIZE_SETTLE_MS ohne weiteres Resize-Ereignis abgelaufen -- JETZT (und nur
                   jetzt) den teuren Dialog-Neuaufbau machen: Geometrie neu berechnen (zentriert
                   automatisch neu, s. compute_dialog_geometry()) und den Dialog MIT DENSELBEN
                   Filtern/Verzeichnis/Palette neu aufsetzen (s. Funktionskommentar zur
                   DEMO-GRENZE). Bleibt das Fenster zu klein, einfach im Overlay bleiben (naechste
                   Runde zeichnet es idempotent neu, wie in main()). */
                if (!too_small) {
                    compute_dialog_geometry(*rows, *cols, &dlg_row, &dlg_col, &dlg_rows, &dlg_cols);
                    if (q9_filedialog_init(&dlg, dlg_row, dlg_col, dlg_rows, dlg_cols,
                                            "Konfigurationsauswahl", dir, filters, 2, &pal) != 0) {
                        snprintf(result_msg, result_msg_size,
                                 "Dateidialog: Fehler nach Groessenaenderung (Esc: abbrechen)");
                        return -1;
                    }
                    showing_overlay = 0;
                }
                continue;
            }

            if (k.kind == Q9_KEY_CTRL_C || k.kind == Q9_KEY_EOF) { want_quit = 1; break; }

            if (k.kind == Q9_KEY_RESIZE) {
                /* Nur die neue Groesse merken und ins (billige) Overlay wechseln -- der teure
                   Dialog-Neuaufbau passiert oben erst nach RESIZE_SETTLE_MS Stille, NICHT bei
                   jedem einzelnen Zwischenschritt waehrend des Ziehens (das war der eigentliche
                   Bug: "da wird immer alles neu gezeichnet"). */
                if (q9_term_size(rows, cols) == 0) {
                    clamp_dims(rows, cols);
                }
                showing_overlay = 1;
                continue;
            }

            if (showing_overlay) { continue; }              /* andere Tasten waehrend des
                                                                 Overlays: ignorieren (wie main()) */

            done = q9_filedialog_handle_key(&dlg, k);
            if (done != 0) { break; }
        }
    }

    if (want_quit) { return 0; }

    if (done == 1) {
        char name[Q9_FILELIST_NAME_MAX];
        q9_filedialog_selected_name(&dlg, name, sizeof(name));
        snprintf(result_msg, result_msg_size, "Datei gewaehlt: %s   (O: erneut oeffnen)", name);
        /* out_name ist optional (NULL = Aufrufer interessiert sich nur fuer die Hinweis-Nachricht,
           s. der 'o'/'O'-Aufruf in main()) -- der NEUE Button-Feld-Aufruf (Emulator-Konfiguration)
           will dagegen den rohen Dateinamen, um ihn ins Namensfeld zu uebernehmen. */
        if (out_name && out_name_size > 0) {
            snprintf(out_name, out_name_size, "%s", name);
        }
        return 1;
    }
    snprintf(result_msg, result_msg_size, "Dateiauswahl abgebrochen   (O: erneut oeffnen)");
    return -1;
}

/* Andreas' Wunsch (2026-08-18, dreiundzwanzigste Runde): "Numerische Eingabe Dezimal/Hex opt. mit
   Bereich" -- die Bibliothek selbst kennt KEINE Bereiche (Anwendungswissen, s.
   q9_listview_field_kind_t in q9_listview.h), hier ein konkretes Beispiel dafuer, wie ein Aufrufer
   das nachruesten kann: klemmt f->value auf [lo,hi], falls es (als Dezimal- oder Hex-Zahl, je nach
   f->kind) ausserhalb liegt. Ein LEERER oder noch UNVOLLSTAENDIGER Wert (z.B. waehrend des Tippens)
   wird NICHT angefasst -- nur ein bereits vollstaendiger, aber zu kleiner/grosser Wert wird beim
   Verlassen des Feldes auf die naechstliegende Grenze gezogen. Bewusst nur EIN Beispiel (das
   "Slot:"-Feld bei RC2014-CF, spiegelt die bestehende useSlot/slot-Pruefung 0-255 in
   src/kernel/boardcfg.c) statt eines generischen Bereichs-Systems -- welche Felder ueberhaupt einen
   Bereich brauchen, ist Teil des (noch offenen) echten Datenfiles pro Hardware-Typ. */
static void clamp_field_range(q9_listview_field_t *f, long lo, long hi)
{
    long v;
    char *end;
    if (!f || f->value[0] == '\0') { return; }
    v = strtol(f->value, &end, f->kind == Q9_LISTVIEW_FIELD_NUMERIC_HEX ? 16 : 10);
    if (*end != '\0') { return; }                            /* unvollstaendig/ungueltig -- in Ruhe lassen */
    if (v < lo) { v = lo; }
    if (v > hi) { v = hi; }
    snprintf(f->value, sizeof(f->value), f->kind == Q9_LISTVIEW_FIELD_NUMERIC_HEX ? "%lX" : "%ld", v);
}

/* Ruft clamp_field_range() fuer das gerade fokussierte Feld auf, WENN es das "Slot:"-Beispiel ist
   (per Label erkannt -- reine Vorfuehrung, s. clamp_field_range()-Kommentar). Wird beim Verlassen
   des Feldes aufgerufen (Pfeil links UND Esc, s. main()). */
static void maybe_clamp_focused_field(q9_listview_t *lv)
{
    q9_listview_field_t *f;
    if (lv->selected < 0 || lv->selected >= g_item_count || lv->field_focus < 0) { return; }
    if (lv->field_focus >= g_list_items[lv->selected].field_count) { return; }
    f = &g_list_items[lv->selected].fields[lv->field_focus];
    if (f->label && strcmp(f->label, "Slot:") == 0) {
        clamp_field_range(f, 0, 255);
    }
}

int main(void)
{
    int rows, cols;
    q9_listview_t lv;
    int running = 1;
    int showing_overlay = 0;                                /* 1 = Resize-Overlay statt Vollinhalt */
    int resize_attempted = 0;                                /* s.u.: XTWINOPS-Versuch nur EINMAL
                                                                  pro zu-klein-Phase, nicht bei jeder
                                                                  einzelnen 1s-Wiederholung erneut   */
    char last_dialog_msg[128] = "";                          /* Ergebnis des letzten Datei-Dialogs
                                                                  (task #22) -- ersetzt den Standard-
                                                                  Hinweistext, bis 'O' erneut gedrueckt
                                                                  wird, s. run_file_dialog()          */

    if (q9_term_size(&rows, &cols) != 0) {
        rows = 24;
        cols = 80;
    }
    clamp_dims(&rows, &cols);

    if (q9_input_init() != 0) {
        fprintf(stderr, "integration_demo: kein echtes Terminal (TTY) -- Abbruch.\n");
        return 1;
    }
    write_ansi(wrap_hide);

    q9_listview_init(&lv, 2, 3, 1, 1, g_item_count);          /* echte Geometrie folgt in render_full_content */

    while (running) {
        int too_small = (rows < MIN_ROWS || cols < MIN_COLS);
        if (too_small) { showing_overlay = 1; }              /* zu klein -> immer Overlay, s.u. */

        if (showing_overlay) {
            render_size_overlay(rows, cols, too_small);
        } else {
            render_full_content(&lv, rows, cols, last_dialog_msg);
        }

        {
            q9_key_t k = showing_overlay ? q9_input_read_key_timeout(RESIZE_SETTLE_MS)
                                          : q9_input_read_key();

            if (showing_overlay && k.kind == Q9_KEY_NONE) {
                /* RESIZE_SETTLE_MS ohne weitere Aenderung abgelaufen. Nur zurueck zum vollen
                   Inhalt, wenn die Groesse tatsaechlich ausreicht -- sonst bleibt das Overlay
                   (mit "zu klein"-Zusatzzeile) einfach stehen und wird in der naechsten Runde
                   identisch neu gezeichnet (idempotent, kein Problem). */
                if (!too_small) {
                    showing_overlay = 0;
                } else if (!resize_attempted) {
                    /* Andreas' Wunsch (2026-08-17): bei anhaltend zu kleinem Fenster EINMAL
                       versuchen, das Terminal per XTWINOPS auf die Mindestgroesse zu bringen
                       (q9_ansi_resize_window, NICHT universell unterstuetzt, s. dortiger
                       Kopfkommentar -- wirkt es, kommt ganz normal ein neues Q9_KEY_RESIZE mit
                       der dann tatsaechlichen Groesse; wirkt es nicht, passiert einfach nichts,
                       das Overlay bleibt unveraendert stehen). Nur EINMAL pro zu-klein-Phase, nicht
                       bei jeder 1s-Wiederholung erneut (sonst wuerde ein Terminal, das die
                       Sequenz konsequent ignoriert, sie trotzdem staendig neu bekommen). */
                    char rbuf[32];
                    unsigned rn = q9_ansi_resize_window(rbuf, sizeof(rbuf), MIN_ROWS, MIN_COLS);
                    fwrite(rbuf, 1, rn, stdout);
                    fflush(stdout);
                    resize_attempted = 1;
                }
                continue;
            }

            switch (k.kind) {
                case Q9_KEY_CTRL_C:
                case Q9_KEY_EOF:
                    running = 0;
                    break;
                case Q9_KEY_UP:
                    /* Waehrend ein Feld fokussiert ist: NUR zwischen den Feldern DIESES Eintrags
                       bewegen (field_move), nicht zum vorigen/naechsten LISTENEINTRAG springen --
                       sonst wuerde man beim Tippen versehentlich aus dem Eintrag herausnavigieren. */
                    if (!showing_overlay) {
                        if (lv.field_focus >= 0) { q9_listview_field_move(&lv, -1, g_list_items); }
                        else                      { q9_listview_move_ex(&lv, -1, g_list_items, g_expanded); }
                    }
                    break;
                case Q9_KEY_DOWN:
                    if (!showing_overlay) {
                        if (lv.field_focus >= 0) { q9_listview_field_move(&lv, 1, g_list_items); }
                        else                      { q9_listview_move_ex(&lv, 1, g_list_items, g_expanded); }
                    }
                    break;
                case Q9_KEY_RIGHT:
                    /* Andreas' Wunsch (2026-08-18, achtzehnte Runde): "anstatt ENTER kann ich Pfeil
                       rechts druecken, item geht auf, und ich komme auf den ersten Eintrag" -- klappt
                       den ausgewaehlten Eintrag auf (falls noch zu) UND setzt den Feld-Fokus auf das
                       erste Feld, in einem Schritt (q9_listview_field_enter()). Nur auf Item-Ebene
                       sinnvoll (field_focus<0) -- waehrend ein Feld schon fokussiert ist, tut Pfeil
                       rechts nichts (kein Rechts-Cursor INNERHALB eines Feldwerts fuer diese erste
                       Fassung, s. Doku). */
                    if (!showing_overlay && lv.field_focus < 0) {
                        q9_listview_field_enter(&lv, g_expanded, g_list_items);
                    }
                    break;
                case Q9_KEY_LEFT:
                    /* "mit ESC oder Pfeil links komme ich wieder raus" -- Pfeil links verlaesst NUR
                       das Feld (zurueck auf die Kopfzeile), der Eintrag bleibt aufgeklappt. Nur
                       wirksam, wenn tatsaechlich ein Feld fokussiert ist. maybe_clamp_focused_field()
                       VOR dem Verlassen (braucht field_focus noch) -- Bereichs-Beispiel, s. dort. */
                    if (!showing_overlay && lv.field_focus >= 0) {
                        maybe_clamp_focused_field(&lv);
                        q9_listview_field_leave(&lv);
                    }
                    break;
                case Q9_KEY_ESCAPE:
                    /* "bei ESC wird das item auch geschlossen" -- wie Pfeil links, klappt den
                       Eintrag danach ZUSAETZLICH zu. Wirkt auch OHNE aktiven Feld-Fokus (klappt
                       einen bereits aufgeklappten Eintrag einfach zu). */
                    if (!showing_overlay) {
                        maybe_clamp_focused_field(&lv);
                        q9_listview_field_escape(&lv, g_expanded);
                    }
                    break;
                case Q9_KEY_ENTER:
                    if (showing_overlay) { break; }
                    if (lv.field_focus < 0) {
                        /* Kurzform auf Item-Ebene (frueheres Verhalten, sechzehnte Runde) -- klappt
                           den AUSGEWAEHLTEN Eintrag auf/zu, OHNE in die Felder zu springen (das
                           macht seit der achtzehnten Runde gezielt Pfeil rechts). */
                        if (lv.selected >= 0 && lv.selected < g_item_count) {
                            g_expanded[lv.selected] = !g_expanded[lv.selected];
                            q9_listview_move_ex(&lv, 0, g_list_items, g_expanded);
                        }
                    } else if (lv.selected >= 0 && lv.selected < g_item_count
                               && lv.field_focus < g_list_items[lv.selected].field_count
                               && g_list_items[lv.selected].fields[lv.field_focus].kind
                                  == Q9_LISTVIEW_FIELD_BUTTON) {
                        /* Andreas' Wunsch (2026-08-18, neunzehnte Runde): "dahinter ein Button um
                           den Dialog zu oeffnen" -- Enter auf einem BUTTON-Feld loest die Aktion aus
                           (bei TEXT-Feldern bleibt Enter weiterhin wirkungslos, s. Kommentar bei
                           field_putc() zur direkten Manipulation -- kein Bestaetigen noetig). Der
                           gewaehlte Dateiname geht ins Feld DAVOR (per Konvention: "Datei:" liegt
                           immer direkt vor seinem Button, s. g_cfg_fields). */
                        char chosen[Q9_FILELIST_NAME_MAX];
                        int r;
                        chosen[0] = '\0';
                        r = run_file_dialog(&rows, &cols, &lv, last_dialog_msg,
                                             sizeof(last_dialog_msg), chosen, sizeof(chosen));
                        if (r == 1 && lv.field_focus > 0) {
                            q9_listview_field_t *namefield =
                                &g_list_items[lv.selected].fields[lv.field_focus - 1];
                            snprintf(namefield->value, sizeof(namefield->value), "%s", chosen);
                            /* Fuenfundzwanzigste Runde: nicht nur den Namen uebernehmen, sondern
                               die Datei auch WIRKLICH laden (ueberschreibt last_dialog_msg mit dem
                               genaueren Lade-Ergebnis statt der reinen Auswahl-Bestaetigung). */
                            load_q9_config_fields(chosen, last_dialog_msg, sizeof(last_dialog_msg), &lv);
                        } else if (r == 0) {
                            running = 0;                     /* Strg-C/EOF waehrend des Dialogs */
                        }
                    }
                    break;
                case Q9_KEY_BACKSPACE:
                    if (!showing_overlay && lv.field_focus >= 0) {
                        q9_listview_field_backspace(&lv, g_list_items);
                    }
                    break;
                case Q9_KEY_RESIZE:
                    if (q9_term_size(&rows, &cols) == 0) {
                        clamp_dims(&rows, &cols);
                    }
                    showing_overlay = 1;                     /* sofort ins Overlay, LIVE aktualisiert
                                                                 bei weiteren RESIZE-Ereignissen     */
                    resize_attempted = 0;                    /* neue zu-klein-Phase (falls es dazu
                                                                 kommt) darf wieder EINEN Versuch
                                                                 machen */
                    break;
                case Q9_KEY_CHAR:
                    /* Waehrend ein Feld fokussiert ist: das Zeichen geht DIREKT in den Feldwert
                       (Andreas' Wunsch: "kann dort alles aendern") -- 'o'/'O' oeffnet dann bewusst
                       NICHT den Datei-Dialog (sonst liesse sich kein "o" in einen Wert tippen).
                       SONDERFALL Leertaste bei einem BOOLEAN-Feld (Andreas' Wunsch, 2026-08-18,
                       vierundzwanzigste Runde: "Boolean Eingabe") -- schaltet ja/nein um statt (wie
                       bei field_putc() ohnehin wirkungslos, s. dort) einfach zu verpuffen. */
                    if (!showing_overlay && lv.field_focus >= 0
                        && k.ch == ' '
                        && lv.selected >= 0 && lv.selected < g_item_count
                        && lv.field_focus < g_list_items[lv.selected].field_count
                        && g_list_items[lv.selected].fields[lv.field_focus].kind
                           == Q9_LISTVIEW_FIELD_BOOLEAN) {
                        q9_listview_field_toggle(&lv, g_list_items);
                    } else if (!showing_overlay && lv.field_focus >= 0) {
                        q9_listview_field_putc(&lv, g_list_items, k.ch);
                    } else if (!showing_overlay && (k.ch == 'o' || k.ch == 'O')) {
                        /* task #22: modaler Datei-Auswahl-Dialog (q9_filedialog.h/.c, task #20).
                           out_name NULL -- hier interessiert nur die Hinweis-Nachricht, s.
                           run_file_dialog()-Kommentar. */
                        int r = run_file_dialog(&rows, &cols, &lv, last_dialog_msg,
                                                 sizeof(last_dialog_msg), NULL, 0);
                        if (r == 0) { running = 0; }         /* Strg-C/EOF waehrend des Dialogs */
                        /* naechste Schleifenrunde zeichnet automatisch alles neu (inkl. last_dialog_msg) */
                    } else if (!showing_overlay && (k.ch == 's' || k.ch == 'S')) {
                        /* Sechsundzwanzigste Runde (Andreas: "Speichern-Funktion") -- global wie
                           'o'/'O', kein Feld muss fokussiert sein. */
                        save_q9_config(last_dialog_msg, sizeof(last_dialog_msg));
                    }
                    break;
                default:
                    break;                                   /* alle anderen Tasten: ignorieren */
            }
        }
    }

    {
        char buf[64];
        unsigned n = 0;
        n += q9_ansi_reset(buf + n, sizeof(buf) - n);
        n += q9_ansi_clear(buf + n, sizeof(buf) - n);
        n += q9_ansi_move(buf + n, sizeof(buf) - n, 1, 1);
        fwrite(buf, 1, n, stdout);
    }
    write_ansi(wrap_show);
    fflush(stdout);
    q9_input_shutdown();
    printf("integration_demo beendet.\n");
    return 0;
}

//────────────────────────────────────────────────────────────────────────────────────────────────
// EOF integration_demo.c                                                                  Ver. 3.60
//────────────────────────────────────────────────────────────────────────────────────────────────
