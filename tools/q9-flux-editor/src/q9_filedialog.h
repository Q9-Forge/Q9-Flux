//════════════════════════════════════════════════════════════════════════════════════════════════
// File:   q9_filedialog.h                                                                 Ver. 1.20
// Owner:  Claudia
// Desc.:  Modaler Datei-Auswahl-Dialog -- komponiert q9_filelist (Verzeichnis-Scan), q9_listview
//         (scrollbare Liste) und q9_screenbuf (Bildschirmpuffer) zu einem echten interaktiven
//         Dialog. Andreas' bestaetigtes Layout (2026-08-17, s. Q9FLUX_EDITOR_de.md Runde 5), von
//         oben nach unten:
//
//             ┌ Kopfzeile: Titel in Kopfzeilenfarbe, rechts ein rein dekoratives "X" (kein
//             │  Maus-Support, s. q9_input.h Kopfkommentar -- Mausklick ist zurueckgestellt)
//             ├ Spaltentitel-Zeile: "Name  Datum  Groesse", leicht andere Farbe
//             ├ Dateiliste (q9_listview, scrollbar, Zeilen vorformatiert -- s.u.)
//             ├──────────────────────────── FUSSBEREICH, eigene Hintergrundfarbe (footer_bg) ──────
//             ├ Auswahl-/Filterzeile: links die aktuell ausgewaehlte Datei, rechts ein kompakter
//             │  Extensions-Umschalter (4-5 Buchstaben + ▾) -- ▾ oeffnet ein kleines Aufklapp-Menue
//             │  mit ALLEN Filtern (s.u.), kein reines Durchschalten mehr
//             ├ (Halbblock-Kappe oberhalb der Buttons, s.u.)
//             ├ Buttons OK / Abbrechen -- "richtige" Buttons in Spaltentitel-Farbe (sub_fg/bg),
//             │  rechtsbuendig, gleich breit, per Q9_GLYPH_UPPER_HALF/LOWER_HALF nach oben+unten
//             │  "aufgeblasen" (klassischer Halbblock-Trick: eine Zeile UEBER dem Button zeigt in
//             │  der unteren Haelfte die Button-Farbe, eine Zeile UNTER dem Button in der oberen
//             │  Haelfte -- der Button wirkt dadurch anderthalb Zeilen hoch, obwohl er nur eine
//             │  einzige Textzeile belegt)
//             └ (Halbblock-Kappe unterhalb der Buttons, s.u.)
//
//         RAHMENLOS -- nur ueber eine eigene Hintergrundfarbe vom Rest des Bildschirms abgegrenzt
//         (Aufgabenbeschreibung, kein q9_widgets-Rahmen). Der AUFRUFER macht snapshot()/restore()
//         um den Dialog herum (s. q9_screenbuf.h Kopfkommentar) -- dieses Modul zeichnet nur IN
//         einen bereits vorhandenen q9_screenbuf_t hinein, kennt kein stdout/Terminal selbst.
//
//         Bedienung (task #20 + Andreas' Feedback 2026-08-17): TAB/Shift-TAB wandert zyklisch
//         Liste -> Filter -> OK -> Abbrechen -> (wieder Liste). Pfeil hoch/runter bewegt die
//         Auswahl NUR wenn die Liste den Fokus hat; Pfeil links/rechts schaltet den Filter EINEN
//         weiter (Schnellzugriff) NUR wenn der Filter den Fokus hat. Enter ODER Pfeil-runter auf
//         dem Filter OEFFNET STATTDESSEN das Aufklapp-Menue mit ALLEN Filtern -- waehrend es offen
//         ist, navigieren Pfeil hoch/runter DARIN, Enter uebernimmt die Auswahl (rescanned das
//         Verzeichnis) und schliesst es wieder, Escape schliesst es OHNE Auswahl (schliesst NUR
//         das Menue, NICHT gleich den ganzen Dialog -- "das oberste Ueberlagerungs-Fenster",
//         klassisches Popup-Verhalten). Alle anderen Tasten werden waehrend das Menue offen ist
//         ignoriert (auch TAB). Enter auf der Liste ODER auf OK = Bestaetigen (nur wenn eine Datei
//         ausgewaehlt ist -- ein leeres Verzeichnis kann nicht bestaetigt werden). Escape bestaetigt
//         (ausserhalb des Filter-Menues) IMMER Abbruch, unabhaengig vom Fokus.
//
//         Multi-Spalten-Problem: q9_listview_render() kennt nur ein flaches `const char *const *`
//         Array (ein String pro Zeile, s. q9_listview.h). Statt die Listview-API zu erweitern,
//         formatiert dieses Modul jede Zeile VORHER zu einem einzigen, spaltenausgerichteten String
//         (Name/Datum/Groesse mit fester Breite) -- genau das Muster, das integration_demo.c fuer
//         seine Statuszeile schon verwendet (dortiges snprintf mit %-*.*s).
//
//         Der aktuelle Verzeichnis-Pfad wird bewusst NICHT angezeigt (Andreas, 2026-08-17: "der
//         aktuelle Pfad fehlt noch... aber machen wir es erst mal ohne" -- als optionale, spaeter
//         nachruestbare Erweiterung vorgemerkt, s. Q9FLUX_EDITOR_de.md).
//
// Call:   static const char *const filters[] = { "*.*", ".q9", ".img" };
//         q9_filedialog_palette_t pal = { ... };
//         q9_filedialog_t dlg;
//         q9_filedialog_init(&dlg, 3, 10, 16, 50, "Konfigurationsauswahl", "/pfad/zu/configs",
//                             filters, 3, &pal);
//         for (;;) {
//             q9_filedialog_render(&dlg, &sb);
//             ... sb rendern, echte Taste lesen (q9_input_read_key) ...
//             int r = q9_filedialog_handle_key(&dlg, key);
//             if (r != 0) break;   // r==1: OK, r==-1: Abbruch
//         }
//         if (r == 1) { char name[Q9_FILELIST_NAME_MAX]; q9_filedialog_selected_name(&dlg, name, sizeof(name)); }
//
// Edition History
//─────────┬──────┬────────────────────────────────────────────────────────────────────────┬──────
// Date    │ Ver. │ Description                                                            │ By
//─────────┼──────┼────────────────────────────────────────────────────────────────────────┬──────
// 26-08-17│ 1.00 │ Erster Wurf -- task #20, Andreas' Layout-Vorgabe aus Runde 5 umgesetzt   │ Cld
// 26-08-17│ 1.10 │ Andreas' Feedback nach dem ersten Test: unfocus_sel_* Farbpaar dazu --   │ Cld
//         │      │ markierte Zeile war bisher unabhaengig vom Fokus immer gleich hell, ein  │
//         │      │ Fokuswechsel auf/von der Liste war dadurch unsichtbar                    │
// 26-08-17│ 1.20 │ Zweite Feedback-Runde: eigener footer_fg/bg-Fussbereich ab der Auswahl-/ │ Cld
//         │      │ Filterzeile, "richtige" Buttons (sub_fg/bg, Halbblock-Kappen, gleich     │
//         │      │ breit, rechtsbuendig), Filter-Pfeil jetzt ▾ + oeffnet ein Aufklapp-Menue │
//         │      │ mit ALLEN Filtern statt nur einzeln durchzuschalten                      │
//═════════╧══════╧════════════════════════════════════════════════════════════════════════╧══════
#ifndef Q9_FILEDIALOG_H
#define Q9_FILEDIALOG_H

#include "q9_screenbuf.h"
#include "q9_listview.h"
#include "q9_filelist.h"
#include "q9_input.h"

#define Q9_FILEDIALOG_MAX_FILTERS 8                         /* vorgegebene Extensionen + "*.*"      */
#define Q9_FILEDIALOG_FILTER_MAX  16                        /* Andreas: "nur 4-5 Buchstaben" + Puffer
                                                                fuer Punkt/Stern/Nullbyte             */
#define Q9_FILEDIALOG_TITLE_MAX   64
#define Q9_FILEDIALOG_DIR_MAX     512
#define Q9_FILEDIALOG_NAME_COL    20                        /* Spaltenbreite "Name" in der vorformat-
                                                                ierten Zeile (Listview-Item-String)   */
#define Q9_FILEDIALOG_ROW_MAX     48                         /* " %-20.20s %8s %7s" -> reichlich Luft */

typedef enum {
    Q9_FILEDIALOG_FOCUS_LIST = 0,
    Q9_FILEDIALOG_FOCUS_FILTER,
    Q9_FILEDIALOG_FOCUS_OK,
    Q9_FILEDIALOG_FOCUS_CANCEL
} q9_filedialog_focus_t;

/* Alle Farben auf einmal beim Aufruf uebergeben (statt einzelner Parameter an jeder Funktion, s.
   q9_listview_render Vorbild) -- der Dialog hat deutlich mehr eigene Farbrollen als die Liste
   alleine. Aufrufer baut sich das typischerweise aus denselben PAL_*-Konstanten zusammen, die er
   schon fuer den Hauptbildschirm hat (s. integration_demo.c). */
typedef struct {
    int header_fg_r, header_fg_g, header_fg_b;
    int header_bg_r, header_bg_g, header_bg_b;
    int sub_fg_r, sub_fg_g, sub_fg_b;                       /* Spaltentitel-Zeile "Name Datum Groesse"*/
    int sub_bg_r, sub_bg_g, sub_bg_b;
    int body_fg_r, body_fg_g, body_fg_b;                    /* Hauptflaeche (Hintergrund des ganzen
                                                                Dialogs -- einzige Abgrenzung, s.o.)  */
    int body_bg_r, body_bg_g, body_bg_b;
    int list_fg_r, list_fg_g, list_fg_b;
    int sel_fg_r, sel_fg_g, sel_fg_b;                       /* markierte Zeile, WENN die Liste den    */
    int sel_bg_r, sel_bg_g, sel_bg_b;                       /* Fokus hat (deutlich hervorgehoben)      */
    int unfocus_sel_fg_r, unfocus_sel_fg_g, unfocus_sel_fg_b;  /* markierte Zeile, wenn die Liste NICHT
                                                                der Fokus hat -- gedaempft, s. .c. Ohne
                                                                diese Unterscheidung war (Andreas'
                                                                Feedback, 2026-08-17) ein Fokuswechsel
                                                                auf/von der Liste unsichtbar, weil die
                                                                Markierung immer gleich aussah */
    int unfocus_sel_bg_r, unfocus_sel_bg_g, unfocus_sel_bg_b;
    int focus_fg_r, focus_fg_g, focus_fg_b;                 /* fokussiertes Bedienelement (Filter/     */
    int focus_bg_r, focus_bg_g, focus_bg_b;                 /* OK/Abbrechen), s. sel_* fuer die Liste  */
    int footer_fg_r, footer_fg_g, footer_fg_b;              /* Fussbereich unterhalb der Dateiliste    */
    int footer_bg_r, footer_bg_g, footer_bg_b;              /* (Andreas' Wunsch, 2026-08-17: "sich
                                                                etwas absetzen") -- eigener Hintergrund,
                                                                Buttons/Filter-Popup nutzen sub_fg/bg
                                                                (Spaltentitel-Farbe) DAVOR, s. .c        */
} q9_filedialog_palette_t;

typedef struct {
    int row, col, rows, cols;                               /* Position/Groesse im Bildschirmpuffer   */
    char title[Q9_FILEDIALOG_TITLE_MAX];
    char dir[Q9_FILEDIALOG_DIR_MAX];

    char filters[Q9_FILEDIALOG_MAX_FILTERS][Q9_FILEDIALOG_FILTER_MAX];  /* eigene Kopie, s. .c        */
    int  filter_count;
    int  filter_index;
    int  filter_popup_open;                                 /* 1 = Aufklapp-Menue mit allen Filtern
                                                                offen (s. Kopfkommentar), 0 = zu       */
    int  filter_popup_index;                                /* Auswahl INNERHALB des offenen Menues --
                                                                erst bei Enter nach filter_index/rescan
                                                                uebernommen, s. .c                      */

    q9_filelist_t files;                                    /* Ergebnis des letzten Scans             */
    char row_text[Q9_FILELIST_MAX_ENTRIES][Q9_FILEDIALOG_ROW_MAX];      /* vorformatierte Listenzeilen */
    const char *row_ptr[Q9_FILELIST_MAX_ENTRIES];           /* Zeiger darauf, fuer q9_listview_render */
    q9_listview_t list;

    q9_filedialog_focus_t focus;
    q9_filedialog_palette_t pal;

    int done;                                                /* 0 = noch offen, 1 = OK, -1 = Abbruch  */
} q9_filedialog_t;

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_filedialog_init
// Desc.:    Setzt Geometrie/Titel/Verzeichnis/Palette, kopiert die filters (Aufrufer-Array darf
//           danach freigegeben/veraendert werden -- eigene Kopie, kein Lebensdauer-Problem wie bei
//           q9_listview's Item-Zeigern), und fuehrt den ERSTEN Verzeichnis-Scan mit filters[0] aus.
//           filter_count wird auf [1, Q9_FILEDIALOG_MAX_FILTERS] geklemmt (mindestens ein Filter --
//           typischerweise "*.*" als erster Eintrag). Fokus startet auf der Liste. Rueckgabe: 0 ok,
//           -1 bei ungueltigen Argumenten (dlg/title/dir/filters NULL, filter_count<1). Ein nicht
//           lesbares Verzeichnis ist KEIN Fehler hier (wie q9_filelist_scan) -- die Liste ist dann
//           einfach leer, OK bleibt bis zur ersten gueltigen Auswahl unbestaetigbar.
// Call:     q9_filedialog_init(&dlg, 3, 10, 16, 50, "Konfigurationsauswahl", "/configs",
//                               filters, 3, &pal)
//════════════════════════════════════════════════════════════════════════════════════════════════
int q9_filedialog_init(q9_filedialog_t *dlg, int row, int col, int rows, int cols,
                        const char *title, const char *dir,
                        const char *const *filters, int filter_count,
                        const q9_filedialog_palette_t *pal);

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_filedialog_handle_key
// Desc.:    Reine Zustandslogik (kein I/O) -- verarbeitet EINE bereits erkannte q9_key_t (s.
//           q9_input.h), aendert Fokus/Auswahl/Filter entsprechend. Ist dlg->done bereits != 0
//           (Dialog schon entschieden), tut die Funktion nichts mehr und liefert einfach den
//           bestehenden Wert zurueck (verhindert z.B., dass ein nachzitterndes Escape nach einem
//           bereits erfolgten OK das Ergebnis noch umbiegt). Ist das Filter-Aufklapp-Menue offen
//           (dlg->filter_popup_open), werden NUR Pfeil hoch/runter (Auswahl darin bewegen), Enter
//           (uebernehmen+schliessen) und Escape (schliessen OHNE Auswahl) verarbeitet -- alle
//           anderen Tasten (auch TAB) werden ignoriert, dlg->done bleibt dabei immer 0 (das Menue
//           schliessen kann den GANZEN Dialog nicht beenden). Rueckgabe == dlg->done danach:
//           0 = weiterhin offen, 1 = OK (q9_filedialog_selected_name abrufen), -1 = Abbruch.
// Call:     int r = q9_filedialog_handle_key(&dlg, key); if (r != 0) { /* Dialog fertig */ }
//════════════════════════════════════════════════════════════════════════════════════════════════
int q9_filedialog_handle_key(q9_filedialog_t *dlg, q9_key_t key);

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_filedialog_render
// Desc.:    Zeichnet den kompletten Dialog (Kopfzeile+X, Spaltentitel, Liste, Auswahl-/Filterzeile,
//           OK/Abbrechen -- fokussiertes Element hervorgehoben) in sb hinein, an (dlg->row,
//           dlg->col). sb muss mindestens dlg->rows x dlg->cols an dieser Position fassen (wie alle
//           q9_screenbuf-Funktionen bounds-sicher, aber ohne sinnvolles Ergebnis bei zu kleinem
//           Puffer). Der Aufrufer ist fuer snapshot()/restore() UM den Dialog herum verantwortlich
//           (s. Kopfkommentar) -- diese Funktion selbst kennt davon nichts.
// Call:     q9_filedialog_render(&dlg, &sb)
//════════════════════════════════════════════════════════════════════════════════════════════════
void q9_filedialog_render(const q9_filedialog_t *dlg, q9_screenbuf_t *sb);

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_filedialog_selected_name
// Desc.:    Kopiert den Dateinamen des aktuell ausgewaehlten Eintrags nach out (aufgerufen
//           typischerweise NACH q9_filedialog_handle_key() == 1). Rueckgabe 0 = ok, -1 wenn nichts
//           ausgewaehlt ist (leere Liste) oder ungueltige Argumente -- out[0] wird dann auf '\0'
//           gesetzt (kein undefinierter Inhalt).
// Call:     char name[Q9_FILELIST_NAME_MAX]; q9_filedialog_selected_name(&dlg, name, sizeof(name))
//════════════════════════════════════════════════════════════════════════════════════════════════
int q9_filedialog_selected_name(const q9_filedialog_t *dlg, char *out, unsigned out_max);

#endif /* Q9_FILEDIALOG_H */
//────────────────────────────────────────────────────────────────────────────────────────────────
// EOF q9_filedialog.h                                                                     Ver. 1.20
//────────────────────────────────────────────────────────────────────────────────────────────────
