//════════════════════════════════════════════════════════════════════════════════════════════════
// File:   integration_demo.c                                                             Ver. 1.60
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
//         Pfeiltasten hoch/runter: Auswahl bewegen. O: Datei-Auswahl-Dialog oeffnen (scannt ".",
//         das Arbeitsverzeichnis der Demo, s. run_file_dialog()). Strg-C: beenden.
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
//═════════╧══════╧════════════════════════════════════════════════════════════════════════╧══════
#include <stdio.h>
#include <string.h>

#include "../src/q9_ansi.h"
#include "../src/q9_screenbuf.h"
#include "../src/q9_widgets.h"
#include "../src/q9_listview.h"
#include "../src/q9_input.h"
#include "../src/q9_filedialog.h"

/* Rein zur Demonstration -- kein echtes Hardware-Modell, s. Kopfkommentar. */
static const char *const g_items[] = {
    "CF-Interface (onboard, c0)", "Netz-Terminal x1", "Netz-Terminal x2",
    "Netz-Terminal x3", "Netz-Terminal x4", "Netz-Terminal x5",
    "Netz-Terminal x6", "Netz-Terminal x7", "Netz-Terminal x8",
    "RTC72421 (Echtzeituhr)", "DUART 68681 (Konsole)", "QUICC-Ethernet",
    "MC6845 (GDP/CRTC)", "CLUT (Farbtabelle)", "RC2014-CF (sekundaer)",
    "Framebuffer (VRAM)", "Systemspeicher (RAM)", "ROM-Spiegel",
    "NVRAM (Akku-gepuffert)", "Timer/IRQ3-Trigger",
};
#define ITEM_COUNT (int)(sizeof(g_items) / sizeof(g_items[0]))

/* Warme Amber-/Beige-/Braun-Palette, s. Kopfkommentar. Kopf-/Statuszeile bewusst zwei
   UNTERSCHIEDLICHE (aber verwandte) Hintergrundtoene -- Kopf etwas heller als Status, damit man sie
   auf den ersten Blick auseinanderhalten kann, ohne aus der Farbfamilie auszubrechen. */
#define PAL_FRAME_R      190
#define PAL_FRAME_G      150
#define PAL_FRAME_B       70
#define PAL_LIST_FG_R    215
#define PAL_LIST_FG_G    195
#define PAL_LIST_FG_B    155
#define PAL_SEL_FG_R      35
#define PAL_SEL_FG_G      25
#define PAL_SEL_FG_B      10
#define PAL_SEL_BG_R     215
#define PAL_SEL_BG_G     165
#define PAL_SEL_BG_B      35
#define PAL_STATUS_FG_R  230
#define PAL_STATUS_FG_G  212
#define PAL_STATUS_FG_B  178
#define PAL_STATUS_BG_R   95
#define PAL_STATUS_BG_G   68
#define PAL_STATUS_BG_B   25
#define PAL_HEADER_FG_R  255                                /* Andreas' Wunsch (2026-08-17):      */
#define PAL_HEADER_FG_G  248                                 /* "das weiss etwas heller" -- naeher */
#define PAL_HEADER_FG_B  225                                 /* an Weiss, noch leicht warm getoent */
#define PAL_HEADER_BG_R  140                                /* etwas heller als PAL_STATUS_BG,    */
#define PAL_HEADER_BG_G  100                                /* gleiche Farbfamilie                */
#define PAL_HEADER_BG_B   40

#define MIN_ROWS 20                                        /* Andreas' Wunsch (2026-08-17):     */
#define MIN_COLS 60                                         /* darunter sieht es "sehr komisch"
                                                                aus -- Hinweis statt Versuch      */
#define RESIZE_SETTLE_MS 1000                               /* Andreas' Wunsch: waehrend eines
                                                                Resizes nur die Groesse zeigen,
                                                                nach 1s Stille zurueck zum Inhalt  */

/* Feste Feldbreiten fuer die Statuszeile (Andreas' Wunsch: "sonst huepfen die Texte hin und her").
   NAME_FIELD_WIDTH >= der laengste Eintrag in g_items ("CF-Interface (onboard, c0)" = 27 Zeichen). */
#define NAME_FIELD_WIDTH 30

/* Zusaetzliche Toene NUR fuer den Datei-Auswahl-Dialog (task #22) -- bewusst in derselben warmen
   Amber-/Braun-Familie wie der Rest (Andreas' Wunsch nach "aehnlichen" Farben), aber dunkler als
   PAL_STATUS_BG/PAL_HEADER_BG, damit der Dialog optisch klar "ueber" dem Hauptbildschirm liegt. */
#define PAL_DIALOG_BODY_BG_R  55
#define PAL_DIALOG_BODY_BG_G  40
#define PAL_DIALOG_BODY_BG_B  15
#define PAL_DIALOG_SUB_BG_R  110
#define PAL_DIALOG_SUB_BG_G   78
#define PAL_DIALOG_SUB_BG_B   30

#define DIALOG_ROWS 14                                      /* Wunschgroesse -- wird in            */
#define DIALOG_COLS 50                                       /* run_file_dialog() an rows/cols geklemmt */

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

/* Ersetzt die fruehere separate "Fenster zu klein"-Meldung: EIN einheitliches, zentriertes
   Overlay fuer zwei Faelle -- (a) waehrend/kurz nach einer Groessenaenderung (too_small=0, wird nach
   RESIZE_SETTLE_MS Stille wieder durch den vollen Inhalt ersetzt) und (b) das Fenster ist
   (weiterhin) kleiner als die Mindestgroesse (too_small=1, bleibt dauerhaft sichtbar, zeigt
   zusaetzlich die Mindestgroesse). Beide Faelle sind bewusst DASSELBE einfache "nur die Groesse"-
   Layout -- Andreas: "sieht doof aus, wenn man immer versucht den kompletten Inhalt darzustellen". */
static void render_size_overlay(int rows, int cols, int too_small)
{
    q9_screenbuf_t sb;
    char out[4096];
    char line1[64];
    int mid_row, mid_col;
    int len1;

    q9_screenbuf_init(&sb, rows, cols);

    /* Columns vor Rows (Andreas' Wunsch, 2026-08-17: "Columns und Rows solltest du bitte
       tauschen") -- entspricht auch der ueblichen "80x24"-Schreibweise (Spalten x Zeilen). */
    snprintf(line1, sizeof(line1), "%d Columns - %d Rows", cols, rows);
    len1 = (int)strlen(line1);
    mid_row = rows / 2;
    mid_col = (cols - len1) / 2;
    if (mid_col < 0) { mid_col = 0; }
    q9_screenbuf_puts(&sb, too_small ? mid_row - 1 : mid_row, mid_col, line1,
                       PAL_HEADER_FG_R, PAL_HEADER_FG_G, PAL_HEADER_FG_B);

    if (too_small) {
        char line2[64];
        int len2, mid_col2;
        snprintf(line2, sizeof(line2), "Fenster zu klein (mind. %dx%d)", MIN_COLS, MIN_ROWS);
        len2 = (int)strlen(line2);
        mid_col2 = (cols - len2) / 2;
        if (mid_col2 < 0) { mid_col2 = 0; }
        q9_screenbuf_puts(&sb, mid_row + 1, mid_col2, line2,
                           PAL_HEADER_FG_R, PAL_HEADER_FG_G, PAL_HEADER_FG_B);
    }

    {
        unsigned n = q9_screenbuf_render(&sb, 0, 0, out, sizeof(out));
        fwrite(out, 1, n, stdout);
        fflush(stdout);
    }
}

/* hint: NULL/leer -> Standardtext ("Pfeiltasten: ..."); sonst wird STATTDESSEN hint angezeigt --
   dient der Demo dazu, das Ergebnis des Datei-Auswahl-Dialogs (task #22) sichtbar zu machen, ohne
   die Statuszeile selbst (feste Feldbreiten, s.o.) umbauen zu muessen. */
static void render_full_content(q9_listview_t *lv, int rows, int cols, const char *hint)
{
    q9_screenbuf_t sb;
    char out[1 << 16];
    char status[256];

    q9_screenbuf_init(&sb, rows, cols);
    q9_screenbuf_draw_frame(&sb, 0, 0, rows, cols,
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
    q9_screenbuf_fill_rect(&sb, 0, 0, 1, cols, ' ',
                            PAL_HEADER_FG_R, PAL_HEADER_FG_G, PAL_HEADER_FG_B,
                            1, PAL_HEADER_BG_R, PAL_HEADER_BG_G, PAL_HEADER_BG_B);
    q9_screenbuf_puts(&sb, 0, 3, "Q9-Flux Editor -- Integrations-Demo",
                       PAL_HEADER_FG_R, PAL_HEADER_FG_G, PAL_HEADER_FG_B);

    q9_screenbuf_puts(&sb, rows - 2, 3,
                       (hint && hint[0]) ? hint
                                         : "Pfeiltasten: navigieren   O: Datei oeffnen   Strg-C: beenden",
                       PAL_FRAME_R, PAL_FRAME_G, PAL_FRAME_B);

    lv->row    = 2;
    lv->col    = 3;
    lv->height = rows - 5;                                   /* Rand+Hinweis+Statuszeile/-kante s.u. */
    lv->width  = cols - 6;
    if (lv->height < 1) { lv->height = 1; }
    if (lv->width  < 1) { lv->width  = 1; }
    lv->scroll_offset = q9_listview_scroll(lv->selected, lv->scroll_offset, lv->height, lv->item_count);
    q9_listview_render(lv, &sb, g_items,
                        PAL_LIST_FG_R, PAL_LIST_FG_G, PAL_LIST_FG_B,
                        PAL_SEL_FG_R, PAL_SEL_FG_G, PAL_SEL_FG_B,
                        PAL_SEL_BG_R, PAL_SEL_BG_G, PAL_SEL_BG_B);

    /* Statuszeile ALS untere Rahmenkante, ueber die volle Breite (vorherige Feedback-Runden) --
       jetzt zusaetzlich mit FESTEN Feldbreiten (Andreas: "sonst huepfen die Texte hin und her"):
       der Eintragsname wird auf NAME_FIELD_WIDTH Zeichen aufgefuellt (linksbuendig), die
       Terminal-Groesse mit fester Breite je Zahl (rows dreistellig rechtsbuendig, cols dreistellig
       linksbuendig) -- "Terminal:" steht dadurch bei jeder Auswahl/Groesse an derselben Spalte. */
    q9_screenbuf_fill_rect(&sb, rows - 1, 0, 1, cols, ' ',
                            PAL_STATUS_FG_R, PAL_STATUS_FG_G, PAL_STATUS_FG_B,
                            1, PAL_STATUS_BG_R, PAL_STATUS_BG_G, PAL_STATUS_BG_B);
    snprintf(status, sizeof(status), " Ausgewaehlt: %-*.*s | Terminal: %3dx%-3d",
             NAME_FIELD_WIDTH, NAME_FIELD_WIDTH,
             (lv->selected >= 0 && lv->selected < ITEM_COUNT) ? g_items[lv->selected] : "-",
             rows, cols);
    q9_screenbuf_puts(&sb, rows - 1, 1, status, PAL_STATUS_FG_R, PAL_STATUS_FG_G, PAL_STATUS_FG_B);

    {
        unsigned n = q9_screenbuf_render(&sb, 0, 0, out, sizeof(out));
        fwrite(out, 1, n, stdout);
        fflush(stdout);
    }
}

/* Oeffnet den modalen Datei-Auswahl-Dialog (q9_filedialog.h/.c, task #20) zentriert ueber dem
   aktuellen Bildschirm, scannt bewusst "." (das Arbeitsverzeichnis der Demo selbst -- reine
   Vorfuehrung, keine echte Config-Anbindung, s. Kopfkommentar) mit ein paar Beispiel-Filtern.
   DEMO-GRENZE: ein Terminal-Resize WAEHREND der Dialog offen ist, wird hier bewusst IGNORIERT
   (Dialog bleibt in seiner urspruenglichen Groesse/Position stehen, kein Nachziehen) -- ein
   echter Editor muesste hier neu snapshot/restore + den Dialog re-initialisieren, das würde die
   Demo aber unnoetig verkomplizieren. Strg-C/EOF waehrend des Dialogs wird dagegen NICHT
   ignoriert (sonst liesse sich das Programm aus dem Dialog heraus nicht mehr beenden) --
   signalisiert per Rueckgabe 0 an den Aufrufer, der dann seinerseits sauber beendet.
   Rueckgabe: 1 = Datei ausgewaehlt (result_msg beschreibt sie), -1 = abgebrochen (result_msg
   entsprechend gesetzt), 0 = Strg-C/EOF (result_msg unveraendert -- Aufrufer beendet ohnehin). */
static int run_file_dialog(int rows, int cols, char *result_msg, unsigned result_msg_size)
{
    static const char *const filters[] = { "*.*", ".c", ".h" };
    q9_filedialog_palette_t pal;
    q9_filedialog_t dlg;
    int dlg_rows = DIALOG_ROWS, dlg_cols = DIALOG_COLS;
    int dlg_row, dlg_col;
    int done = 0;
    int want_quit = 0;

    if (dlg_rows > rows - 2) { dlg_rows = rows - 2; }
    if (dlg_cols > cols - 2) { dlg_cols = cols - 2; }
    dlg_row = (rows - dlg_rows) / 2;
    dlg_col = (cols - dlg_cols) / 2;
    if (dlg_row < 1) { dlg_row = 1; }
    if (dlg_col < 1) { dlg_col = 1; }

    memset(&pal, 0, sizeof(pal));
    pal.header_fg_r = PAL_HEADER_FG_R; pal.header_fg_g = PAL_HEADER_FG_G; pal.header_fg_b = PAL_HEADER_FG_B;
    pal.header_bg_r = PAL_HEADER_BG_R; pal.header_bg_g = PAL_HEADER_BG_G; pal.header_bg_b = PAL_HEADER_BG_B;
    pal.sub_fg_r    = PAL_FRAME_R;     pal.sub_fg_g    = PAL_FRAME_G;     pal.sub_fg_b    = PAL_FRAME_B;
    pal.sub_bg_r    = PAL_DIALOG_SUB_BG_R;  pal.sub_bg_g = PAL_DIALOG_SUB_BG_G;  pal.sub_bg_b = PAL_DIALOG_SUB_BG_B;
    pal.body_fg_r   = PAL_LIST_FG_R;   pal.body_fg_g   = PAL_LIST_FG_G;   pal.body_fg_b   = PAL_LIST_FG_B;
    pal.body_bg_r   = PAL_DIALOG_BODY_BG_R; pal.body_bg_g = PAL_DIALOG_BODY_BG_G; pal.body_bg_b = PAL_DIALOG_BODY_BG_B;
    pal.list_fg_r   = PAL_LIST_FG_R;   pal.list_fg_g   = PAL_LIST_FG_G;   pal.list_fg_b   = PAL_LIST_FG_B;
    pal.sel_fg_r    = PAL_SEL_FG_R;    pal.sel_fg_g    = PAL_SEL_FG_G;    pal.sel_fg_b    = PAL_SEL_FG_B;
    pal.sel_bg_r    = PAL_SEL_BG_R;    pal.sel_bg_g    = PAL_SEL_BG_G;    pal.sel_bg_b    = PAL_SEL_BG_B;
    pal.focus_fg_r  = PAL_SEL_FG_R;    pal.focus_fg_g  = PAL_SEL_FG_G;    pal.focus_fg_b  = PAL_SEL_FG_B;
    pal.focus_bg_r  = PAL_SEL_BG_R;    pal.focus_bg_g  = PAL_SEL_BG_G;    pal.focus_bg_b  = PAL_SEL_BG_B;

    if (q9_filedialog_init(&dlg, dlg_row, dlg_col, dlg_rows, dlg_cols,
                            "Konfigurationsauswahl", ".", filters, 3, &pal) != 0) {
        snprintf(result_msg, result_msg_size, "Dateidialog: Fehler beim Start (O: erneut versuchen)");
        return -1;
    }

    for (;;) {
        q9_screenbuf_t sb;
        char out[1 << 16];

        q9_screenbuf_init(&sb, rows, cols);
        /* Hintergrund hinter dem Dialog -- fuer die Demo reicht eine einfache gefuellte Flaeche
           (kein echtes snapshot()/restore() des vorherigen Hauptinhalts, s. Funktionskommentar). */
        q9_screenbuf_fill_rect(&sb, 0, 0, rows, cols, ' ', PAL_LIST_FG_R, PAL_LIST_FG_G, PAL_LIST_FG_B,
                                1, PAL_DIALOG_BODY_BG_R, PAL_DIALOG_BODY_BG_G, PAL_DIALOG_BODY_BG_B);
        q9_filedialog_render(&dlg, &sb);
        {
            unsigned n = q9_screenbuf_render(&sb, 0, 0, out, sizeof(out));
            fwrite(out, 1, n, stdout);
            fflush(stdout);
        }

        {
            q9_key_t k = q9_input_read_key();
            if (k.kind == Q9_KEY_CTRL_C || k.kind == Q9_KEY_EOF) { want_quit = 1; break; }
            if (k.kind == Q9_KEY_RESIZE) { continue; }        /* Demo-Grenze, s. Funktionskommentar */
            done = q9_filedialog_handle_key(&dlg, k);
            if (done != 0) { break; }
        }
    }

    if (want_quit) { return 0; }

    if (done == 1) {
        char name[Q9_FILELIST_NAME_MAX];
        q9_filedialog_selected_name(&dlg, name, sizeof(name));
        snprintf(result_msg, result_msg_size, "Datei gewaehlt: %s   (O: erneut oeffnen)", name);
        return 1;
    }
    snprintf(result_msg, result_msg_size, "Dateiauswahl abgebrochen   (O: erneut oeffnen)");
    return -1;
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

    q9_listview_init(&lv, 2, 3, 1, 1, ITEM_COUNT);          /* echte Geometrie folgt in render_full_content */

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
                    if (!showing_overlay) { q9_listview_move(&lv, -1); }
                    break;
                case Q9_KEY_DOWN:
                    if (!showing_overlay) { q9_listview_move(&lv, 1); }
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
                    if (!showing_overlay && (k.ch == 'o' || k.ch == 'O')) {
                        /* task #22: modaler Datei-Auswahl-Dialog (q9_filedialog.h/.c, task #20). */
                        int r = run_file_dialog(rows, cols, last_dialog_msg, sizeof(last_dialog_msg));
                        if (r == 0) { running = 0; }         /* Strg-C/EOF waehrend des Dialogs */
                        /* naechste Schleifenrunde zeichnet automatisch alles neu (inkl. last_dialog_msg) */
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
// EOF integration_demo.c                                                                  Ver. 1.60
//────────────────────────────────────────────────────────────────────────────────────────────────
