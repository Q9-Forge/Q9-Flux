//════════════════════════════════════════════════════════════════════════════════════════════════
// File:   q9_filedialog.c                                                                 Ver. 1.60
// Owner:  Claudia
// Desc.:  Implementierung, siehe q9_filedialog.h. Layout in Zeilen relativ zu dlg->row (rows==Hoehe
//         des Dialogs, s. layout_rows() -- EINZIGE Stelle, die diese Aufteilung kennt, init() und
//         render() rufen beide dieselbe Funktion auf):
//             0            Kopfzeile (Titel + "X")
//             1            Spaltentitel ("Name  Datum  Groesse")                    ┐
//             2..rows-6    Dateiliste (Hoehe = rows-6)                              │ Rahmenlinien
//             rows-4       Namenszeile ("Datei: ...") + obere Halbblock-Kappen      │ links+rechts
//                           der Buttons (rechts, s.u. -- teilen sich diese Zeile!)   │ spannen GENAU
//             rows-3       Filterzeile ("Filter: ...") + Buttons OK/Abbrechen       │ diesen Bereich,
//                           (rechts, s.u. -- teilen sich ebenfalls die Zeile!)       │ s. render() --
//             rows-2       untere Halbblock-Kappe der Buttons                       ┘ "der Strich
//             rows-1       NEUE Statuszeile (voller Breite, wie Hauptfenster)          geht ab dem
//         Zeilen rows-4..rows-2 = Fussbereich (eigene footer_bg-Hintergrundfarbe).      Header bis
//         Sechste Feedback-Runde (Andreas, 2026-08-17): "die beiden Buttons kommen      zur Status-
//         direkt unter die Dateitabelle" -- die Buttons teilen sich jetzt die Namens-/  zeile"
//         und Filterzeile (rechtsbuendig, waehrend Datei:/Filter: links stehen) statt
//         eigene Zeilen zu belegen -- spart 2 Zeilen gegenueber der fuenften Runde.
//         Der Name in der Namenszeile wird dafuer entsprechend schmaler UND -- wie auch
//         in der Tabelle selbst -- mit "..." gekuerzt, wenn er nicht passt.
//         Macht rows kleiner als das Minimum (8) keinen Sinn mehr -- init klemmt die Listenhoehe
//         auf mindestens 1, ein winziger Dialog sieht dann einfach gedraengt aus (kein Crash, wie
//         der Rest der q9_screenbuf-Familie).
//
// Edition History
//─────────┬──────┬────────────────────────────────────────────────────────────────────────┬──────
// Date    │ Ver. │ Description                                                            │ By
//─────────┼──────┼────────────────────────────────────────────────────────────────────────┬──────
// 26-08-17│ 1.00 │ Erster Wurf                                                              │ Cld
// 26-08-17│ 1.10 │ Dateiliste zeigt jetzt gedaempfte unfocus_sel_*-Farben, wenn sie NICHT   │ Cld
//         │      │ den Fokus hat (s. q9_filedialog.h)                                       │
// 26-08-17│ 1.20 │ Zweite Feedback-Runde: layout_rows() als gemeinsame Geometrie-Quelle,    │ Cld
//         │      │ eigener Fussbereich (footer_bg), "richtige" Buttons per Halbblock-Kappen,│
//         │      │ Filter-Aufklapp-Menue (filter_popup_open/-index) statt reinem Durchschalten│
// 26-08-17│ 1.30 │ Dritte Feedback-Runde: linke Linie fuer die Dateiliste dazu (q9_listview  │ Cld
//         │      │ zeichnet nur die rechte), Spaltentitel-Zeile jetzt exakt so breit wie die │
//         │      │ Tabelle, Name-Spalte dynamisch (name_col_width statt fester Konstante),   │
//         │      │ Filter/Buttons/Popup buendig mit der rechten Linie statt Dialogrand,       │
//         │      │ "Datei:"-Wert in eigenem sub_fg/bg-Kaestchen                               │
// 26-08-17│ 1.40 │ Vierte Feedback-Runde: keine Randspalten mehr -- Linien liegen GENAU auf   │ Cld
//         │      │ der Dialogkante, rechte Linie verschmilzt mit q9_listview's Bildlaufleiste,│
//         │      │ Rahmen beginnt schon bei der Spaltentitel-Zeile (nicht erst bei der Liste) │
// 26-08-17│ 1.50 │ Fuenfte Feedback-Runde ("wirkt jetzt doch gequetscht"): 1 Zeichen Luft     │ Cld
//         │      │ zwischen Inhalt und Linie (q9_listview.c content_width jetzt width-2),     │
//         │      │ Buttons 1 Zeichen schmaler je Seite, Filter jetzt eigene Zeile ("Filter: ")│
//         │      │ statt rechtsbuendig neben "Datei:", neue Statuszeile ganz unten (wie       │
//         │      │ Hauptfenster), Rahmenlinien gehen jetzt bis zu dieser Statuszeile           │
// 26-08-17│ 1.60 │ Sechste Feedback-Runde: Buttons teilen sich jetzt Namens-/Filterzeile      │ Cld
//         │      │ statt eigene Zeilen zu belegen (2 Zeilen gespart), Name wird dafuer         │
//         │      │ schmaler + mit "..." gekuerzt (auch in der Tabelle selbst), Rahmenlinien   │
//         │      │ bekommen als Hintergrund IMMER body_bg (dunkler als die jeweilige Zeile),  │
//         │      │ "X" 1 Zeichen weiter rechts, Statuszeile gekuerzt + mit Trennstrichen       │
//═════════╧══════╧════════════════════════════════════════════════════════════════════════╧══════
#include "q9_filedialog.h"
#include <string.h>
#include <stdio.h>

static void copy_bounded(char *dst, unsigned dst_size, const char *src)
{
    size_t n;
    if (!src) { if (dst_size) { dst[0] = '\0'; } return; }
    n = strlen(src);
    if (n >= dst_size) { n = dst_size ? dst_size - 1 : 0; }
    memcpy(dst, src, n);
    if (dst_size) { dst[n] = '\0'; }
}

/* Kopiert src in out, links ausgerichtet auf genau width Zeichen aufgefuellt (wie "%-*.*s") --
   PASST src NICHT, ersetzt aber die letzten 3 Zeichen durch "..." wenn src laenger als width ist
   (Andreas' Wunsch, 2026-08-17, sechste Runde: "der Dateiname wird dann gekuerzt und ggf. mit den
   ... dargestellt"). Bei width<3 wird hart abgeschnitten (keine sinnvolle Ellipse mehr moeglich). */
static void truncate_ellipsis(char *out, unsigned out_size, const char *src, int width)
{
    int len;
    if (width < 0) { width = 0; }
    if ((unsigned)width >= out_size) { width = (int)out_size - 1; }
    len = (int)strlen(src);
    if (len <= width) {
        snprintf(out, out_size, "%-*.*s", width, width, src);
        return;
    }
    if (width >= 3) {
        memcpy(out, src, (size_t)(width - 3));
        memcpy(out + (width - 3), "...", 3);
    } else {
        memcpy(out, src, (size_t)width);
    }
    out[width] = '\0';
}

/* Zentrale Geometrie fuer den Fussbereich -- s. Kopfkommentar. Alle Rueckgabe-Zeiger duerfen NULL
   sein (Aufrufer holt sich nur, was er braucht: init() nur list_height, render() den Rest). Werte
   sind Zeilen-OFFSETS relativ zu dlg->row, noch NICHT damit addiert. */
static void layout_rows(int rows, int *list_height, int *datei_row, int *filter_buttons_row,
                         int *cap_below_row, int *bottom_status_row)
{
    int lh = rows - 6;
    if (lh < 1) { lh = 1; }
    if (list_height)       { *list_height       = lh; }
    /* Liste belegt Zeilen 2..(2+lh-1) = 2..(rows-5) -- die folgenden Zeilen schliessen NAHTLOS
       daran an. datei_row traegt zusaetzlich die OBERE Halbblock-Kappe der Buttons (rechts),
       filter_buttons_row zusaetzlich die Buttons selbst (rechts) -- s. Kopfkommentar. */
    if (datei_row)           { *datei_row           = rows - 4; }
    if (filter_buttons_row)  { *filter_buttons_row  = rows - 3; }
    if (cap_below_row)       { *cap_below_row       = rows - 2; }
    if (bottom_status_row)   { *bottom_status_row   = rows - 1; }
}

/* Formatiert alle aktuell in dlg->files stehenden Eintraege zu spaltenausgerichteten Zeilen fuer
   q9_listview_render (s. Kopfkommentar q9_filedialog.h -- flaches String-Array, kein eigenes
   Mehrspalten-Feature in q9_listview.c). Name-Spalte in dlg->name_col_width (dynamisch, EINMAL in
   init() berechnet, s. dort) -- Groesse ganz rechts, Datum davor, Name fuellt den Rest, mit "..."
   gekuerzt wenn er nicht passt (Andreas' Wunsch, 2026-08-17, sechste Runde). */
static void format_rows(q9_filedialog_t *dlg)
{
    int i;
    for (i = 0; i < dlg->files.count; i++) {
        const q9_fileentry_t *e = &dlg->files.entry[i];
        char namebuf[Q9_FILELIST_NAME_MAX];
        truncate_ellipsis(namebuf, sizeof(namebuf), e->name, dlg->name_col_width);
        snprintf(dlg->row_text[i], sizeof(dlg->row_text[i]), "%-*.*s %*s %*s",
                 dlg->name_col_width, dlg->name_col_width, namebuf,
                 Q9_FILEDIALOG_DATE_COL, e->date, Q9_FILEDIALOG_SIZE_COL, e->size);
        dlg->row_ptr[i] = dlg->row_text[i];
    }
}

/* Scannt dlg->dir neu mit dem aktuell gewaehlten Filter, formatiert die Zeilen, und setzt die
   Listview auf die (moeglicherweise neue) Eintragsanzahl zurueck (Auswahl/Scroll auf 0 -- ein
   Filterwechsel macht die alte Auswahlposition ohnehin meist sinnlos). */
static void rescan(q9_filedialog_t *dlg)
{
    int n = q9_filelist_scan(dlg->dir, dlg->filters[dlg->filter_index], &dlg->files);
    if (n < 0) { dlg->files.count = 0; }
    format_rows(dlg);
    q9_listview_init(&dlg->list, dlg->list.row, dlg->list.col,
                      dlg->list.height, dlg->list.width, dlg->files.count);
}

int q9_filedialog_init(q9_filedialog_t *dlg, int row, int col, int rows, int cols,
                        const char *title, const char *dir,
                        const char *const *filters, int filter_count,
                        const q9_filedialog_palette_t *pal)
{
    int i;
    int list_height;

    if (!dlg || !title || !dir || !filters || !pal || filter_count < 1) { return -1; }

    dlg->row = row; dlg->col = col; dlg->rows = rows; dlg->cols = cols;
    copy_bounded(dlg->title, sizeof(dlg->title), title);
    copy_bounded(dlg->dir, sizeof(dlg->dir), dir);

    if (filter_count > Q9_FILEDIALOG_MAX_FILTERS) { filter_count = Q9_FILEDIALOG_MAX_FILTERS; }
    dlg->filter_count = filter_count;
    for (i = 0; i < filter_count; i++) {
        copy_bounded(dlg->filters[i], sizeof(dlg->filters[i]), filters[i]);
    }
    dlg->filter_index = 0;
    dlg->filter_popup_open = 0;
    dlg->filter_popup_index = 0;

    dlg->pal = *pal;
    dlg->focus = Q9_FILEDIALOG_FOCUS_LIST;
    dlg->done = 0;

    /* Listen-Geometrie EINMAL berechnen (s. layout_rows()) -- rescan() aendert nur noch item_count,
       nie row/col/height/width. Spaltenaufteilung: EINE Spalte Randlinie plus EINE Spalte Luft,
       dann erst der Inhalt -- q9_listview.c reserviert seine rechte Spalte fuer die Linie UND eine
       Luftspalte davor (content_width = width-2, s. dort), auf der LINKEN Seite macht dieses
       Modul dasselbe von Hand (Randlinie bei dlg->col, Inhalt ab dlg->col+2). Die Linie selbst
       bleibt GENAU auf der Dialogkante. */
    layout_rows(dlg->rows, &list_height, NULL, NULL, NULL, NULL);
    dlg->list.row    = dlg->row + 2;
    dlg->list.col    = dlg->col + 2;
    dlg->list.height = list_height;
    dlg->list.width  = dlg->cols - 2;
    if (dlg->list.width < 1) { dlg->list.width = 1; }

    /* Name-Spaltenbreite: der Rest der Listenbreite, nachdem q9_listview's eigene rechte Spalte
       PLUS die Luftspalte davor (2 Zeichen, s.o.) sowie Trennzeichen/Datum/Groesse abgezogen
       sind -- "Dateiname so lang wie der Rest". */
    {
        int content_width = dlg->list.width - 2;             /* -2 == q9_listview's Linie + Luft   */
        int name_w = content_width - 1 - Q9_FILEDIALOG_DATE_COL - 1 - Q9_FILEDIALOG_SIZE_COL;
        if (name_w < 1) { name_w = 1; }
        dlg->name_col_width = name_w;
    }

    rescan(dlg);
    return 0;
}

/* Zyklische Fokus-Bewegung: LIST -> FILTER -> OK -> CANCEL -> (wieder LIST), bzw. rueckwaerts bei
   dir<0 (Shift-Tab). Reine kleine Modulo-Rechnung, extra Funktion nur der Lesbarkeit halber. */
static void advance_focus(q9_filedialog_t *dlg, int dir)
{
    int f = (int)dlg->focus + dir;
    if (f < 0) { f = 3; }
    if (f > 3) { f = 0; }
    dlg->focus = (q9_filedialog_focus_t)f;
}

/* Filter EINEN weiter (Schnellzugriff per Pfeil links/rechts, s. .h) -- rescanned sofort. */
static void cycle_filter(q9_filedialog_t *dlg, int dir)
{
    int f = dlg->filter_index + dir;
    if (f < 0) { f = dlg->filter_count - 1; }
    if (f >= dlg->filter_count) { f = 0; }
    dlg->filter_index = f;
    rescan(dlg);
}

/* Oeffnet das Aufklapp-Menue mit ALLEN Filtern (Andreas' Wunsch, 2026-08-17) -- die Auswahl DARIN
   startet beim aktuell aktiven Filter, wird aber erst bei Enter uebernommen (Escape verwirft sie
   wieder, s. .h). */
static void open_filter_popup(q9_filedialog_t *dlg)
{
    dlg->filter_popup_open = 1;
    dlg->filter_popup_index = dlg->filter_index;
}

int q9_filedialog_handle_key(q9_filedialog_t *dlg, q9_key_t key)
{
    if (!dlg) { return 0; }
    if (dlg->done != 0) { return dlg->done; }               /* schon entschieden -- eingefroren, s. .h */

    /* Solange das Filter-Aufklapp-Menue offen ist, gehoert ihm die volle Tastatur -- s. .h
       Kopfkommentar zu q9_filedialog_handle_key(): Escape schliesst hier NUR das Menue, nicht den
       ganzen Dialog (klassisches "oberste Ueberlagerung zuerst"-Popup-Verhalten). */
    if (dlg->filter_popup_open) {
        switch (key.kind) {
            case Q9_KEY_UP:
                if (dlg->filter_popup_index > 0) { dlg->filter_popup_index--; }
                break;
            case Q9_KEY_DOWN:
                if (dlg->filter_popup_index < dlg->filter_count - 1) { dlg->filter_popup_index++; }
                break;
            case Q9_KEY_ENTER:
                dlg->filter_index = dlg->filter_popup_index;
                rescan(dlg);
                dlg->filter_popup_open = 0;
                break;
            case Q9_KEY_ESCAPE:
                dlg->filter_popup_open = 0;                  /* verwirft die Auswahl, s. Funktionskomm. */
                break;
            default:
                break;                                        /* alles andere: ignorieren, auch TAB     */
        }
        return dlg->done;                                     /* bleibt 0 -- Popup kann Dialog nicht beenden */
    }

    switch (key.kind) {
        case Q9_KEY_ESCAPE:
            dlg->done = -1;                                  /* Escape = IMMER Abbruch, unabh. vom Fokus */
            break;
        case Q9_KEY_TAB:
            advance_focus(dlg, 1);
            break;
        case Q9_KEY_SHIFT_TAB:
            advance_focus(dlg, -1);
            break;
        case Q9_KEY_UP:
            if (dlg->focus == Q9_FILEDIALOG_FOCUS_LIST) { q9_listview_move(&dlg->list, -1); }
            break;
        case Q9_KEY_DOWN:
            if (dlg->focus == Q9_FILEDIALOG_FOCUS_LIST) {
                q9_listview_move(&dlg->list, 1);
            } else if (dlg->focus == Q9_FILEDIALOG_FOCUS_FILTER) {
                open_filter_popup(dlg);                       /* Pfeil runter = "aufklappen" */
            }
            break;
        case Q9_KEY_LEFT:
            if (dlg->focus == Q9_FILEDIALOG_FOCUS_FILTER) { cycle_filter(dlg, -1); }
            break;
        case Q9_KEY_RIGHT:
            if (dlg->focus == Q9_FILEDIALOG_FOCUS_FILTER) { cycle_filter(dlg, 1); }
            break;
        case Q9_KEY_ENTER:
            switch (dlg->focus) {
                case Q9_FILEDIALOG_FOCUS_LIST:
                case Q9_FILEDIALOG_FOCUS_OK:
                    /* Bestaetigen nur, wenn tatsaechlich etwas ausgewaehlt ist (leeres Verzeichnis
                       -> list.selected bleibt -1, s. q9_listview_init) -- sonst passiert nichts. */
                    if (dlg->list.selected >= 0) { dlg->done = 1; }
                    break;
                case Q9_FILEDIALOG_FOCUS_FILTER:
                    open_filter_popup(dlg);                  /* Enter oeffnet jetzt das Aufklapp-Menue */
                    break;
                case Q9_FILEDIALOG_FOCUS_CANCEL:
                    dlg->done = -1;
                    break;
            }
            break;
        default:
            break;                                            /* alle anderen Tasten: ignorieren       */
    }
    return dlg->done;
}

int q9_filedialog_selected_name(const q9_filedialog_t *dlg, char *out, unsigned out_max)
{
    if (!dlg || !out || out_max == 0) { return -1; }
    if (dlg->list.selected < 0 || dlg->list.selected >= dlg->files.count) {
        out[0] = '\0';
        return -1;
    }
    copy_bounded(out, out_max, dlg->files.entry[dlg->list.selected].name);
    return 0;
}

/* Zentriert label in ein Feld der Breite width (leerzeichenaufgefuellt), z.B. "OK" in einem 11
   Zeichen breiten Button -> "    OK     ". Schneidet label ab, falls es nicht passt (sollte bei
   den beiden festen Beschriftungen hier nie vorkommen, s. Aufrufer). */
static void center_label(char *out, unsigned out_size, const char *label, int width)
{
    int len = (int)strlen(label);
    int left, i;
    if (width < 0) { width = 0; }
    if ((unsigned)width >= out_size) { width = (int)out_size - 1; }
    if (len > width) { len = width; }
    left = (width - len) / 2;
    for (i = 0; i < width; i++) { out[i] = ' '; }
    memcpy(out + left, label, (size_t)len);
    out[width] = '\0';
}

/* Zeichnet EINEN Button (Text + zwei Halbblock-Kappen ueber/unter der Textzeile, s. Kopfkommentar
   q9_filedialog.h fuer den Trick) an (row,col), Breite width, in der Farbe fg/bg (Aufrufer waehlt
   schon die richtige -- sub_* unfokussiert, focus_* fokussiert). cap_row_above/cap_row_below sind
   die beiden Zeilen fuer die Kappen -- seit der sechsten Runde TEILEN sich diese Zeilen mit der
   Namens- bzw. der unteren Fusszeile (s. Kopfkommentar oben), betreffen dort aber NUR die eigenen
   Spalten (col..col+width-1), der Rest der jeweiligen Zeile bleibt unberuehrt. footer_bg ist die
   Hintergrundfarbe DORT (die "aeussere" Haelfte jeder Kappe zeigt diese Farbe, die "innere" -- an
   den Button angrenzende -- Haelfte fg/bg). */
static void draw_button(q9_screenbuf_t *sb, int row, int col, int width, const char *label,
                         int fg_r, int fg_g, int fg_b, int bg_r, int bg_g, int bg_b,
                         int cap_row_above, int cap_row_below,
                         int footer_bg_r, int footer_bg_g, int footer_bg_b)
{
    char text[32];
    center_label(text, sizeof(text), label, width);
    q9_screenbuf_fill_rect(sb, row, col, 1, width, ' ', fg_r, fg_g, fg_b, 1, bg_r, bg_g, bg_b);
    q9_screenbuf_puts(sb, row, col, text, fg_r, fg_g, fg_b);
    q9_screenbuf_fill_rect(sb, cap_row_above, col, 1, width, (char)Q9_GLYPH_LOWER_HALF,
                            bg_r, bg_g, bg_b, 1, footer_bg_r, footer_bg_g, footer_bg_b);
    q9_screenbuf_fill_rect(sb, cap_row_below, col, 1, width, (char)Q9_GLYPH_UPPER_HALF,
                            bg_r, bg_g, bg_b, 1, footer_bg_r, footer_bg_g, footer_bg_b);
}

/* Zeichnet das Filter-Aufklapp-Menue (dlg->filter_popup_open) UEBER der Filterzeile -- waechst nach
   OBEN in den Bereich, den sonst die Dateiliste einnimmt, linksbuendig unter dem Filterfeld. Wird
   ganz am Ende von q9_filedialog_render() aufgerufen, ueberzeichnet also bewusst alles darunter. */
static void render_filter_popup(const q9_filedialog_t *dlg, q9_screenbuf_t *sb, int filter_row_abs)
{
    const q9_filedialog_palette_t *p = &dlg->pal;
    int avail = filter_row_abs - (dlg->row + 2);            /* Platz unter der Spaltentitel-Zeile   */
    int popup_h = dlg->filter_count;
    int popup_w = Q9_FILEDIALOG_FILTER_MAX + 2;
    int popup_row, popup_col, i;

    if (popup_h > avail) { popup_h = avail; }
    if (popup_h < 1)     { popup_h = 1; }
    if (popup_w > dlg->cols - 2) { popup_w = dlg->cols - 2; }
    if (popup_w < 1)     { popup_w = 1; }

    popup_row = filter_row_abs - popup_h;
    popup_col = dlg->list.col;                                /* linksbuendig unter "Filter: ", s.o. */
    if (popup_col < dlg->col + 1) { popup_col = dlg->col + 1; }

    q9_screenbuf_fill_rect(sb, popup_row, popup_col, popup_h, popup_w, ' ',
                            p->sub_fg_r, p->sub_fg_g, p->sub_fg_b,
                            1, p->sub_bg_r, p->sub_bg_g, p->sub_bg_b);

    for (i = 0; i < dlg->filter_count && i < popup_h; i++) {
        int is_sel = (i == dlg->filter_popup_index);
        char popup_line[Q9_FILEDIALOG_FILTER_MAX + 4];
        snprintf(popup_line, sizeof(popup_line), " %-*.*s", popup_w - 1, popup_w - 1, dlg->filters[i]);
        if (is_sel) {
            q9_screenbuf_fill_rect(sb, popup_row + i, popup_col, 1, popup_w, ' ',
                                    p->focus_fg_r, p->focus_fg_g, p->focus_fg_b,
                                    1, p->focus_bg_r, p->focus_bg_g, p->focus_bg_b);
        }
        q9_screenbuf_puts(sb, popup_row + i, popup_col, popup_line,
                           is_sel ? p->focus_fg_r : p->sub_fg_r,
                           is_sel ? p->focus_fg_g : p->sub_fg_g,
                           is_sel ? p->focus_fg_b : p->sub_fg_b);
    }
}

void q9_filedialog_render(const q9_filedialog_t *dlg, q9_screenbuf_t *sb)
{
    const q9_filedialog_palette_t *p;
    char line[96];
    int datei_row, filter_buttons_row, cap_below_row, bottom_status_row;
    int ok_focus, cancel_focus, filter_focus;
    int right_border_col, content_right, content_left;
    int button_w, ok_col, cancel_col;

    if (!dlg || !sb) { return; }
    p = &dlg->pal;
    layout_rows(dlg->rows, NULL, &datei_row, &filter_buttons_row, &cap_below_row, &bottom_status_row);
    datei_row          += dlg->row;
    filter_buttons_row += dlg->row;
    cap_below_row       += dlg->row;
    bottom_status_row   += dlg->row;
    filter_focus = (dlg->focus == Q9_FILEDIALOG_FOCUS_FILTER);
    ok_focus     = (dlg->focus == Q9_FILEDIALOG_FOCUS_OK);
    cancel_focus = (dlg->focus == Q9_FILEDIALOG_FOCUS_CANCEL);

    /* right_border_col ist GENAU die Dialogkante (dlg->list.col+dlg->list.width-1, s. init()) --
       content_right laesst davor die Luftspalte frei ("zum Strich jeweils ein Leerzeichen"),
       content_left ist die entsprechende linke Bezugsspalte (== dlg->list.col, hat die Luft zur
       linken Linie schon eingebaut, s. init()). */
    right_border_col = dlg->list.col + dlg->list.width - 1;
    content_right     = right_border_col - 1;
    content_left       = dlg->list.col;

    /* Button-Geometrie VORAB berechnen (nicht erst beim Zeichnen der Buttons selbst) -- die
       Namenszeile braucht ok_col schon, um zu wissen, wie breit das Namens-Kaestchen noch sein
       darf, bevor es mit der oberen Halbblock-Kappe von OK kollidieren wuerde (Andreas' Wunsch,
       2026-08-17, sechste Runde: "die beiden Buttons kommen direkt unter die Dateitabelle...
       dadurch kollidiert das mit den Dateinamen, der muss dann kuerzer"). */
    button_w = (int)strlen("Abbrechen") + 2;
    cancel_col = content_right - button_w + 1;
    ok_col = cancel_col - 1 - button_w;
    if (ok_col < content_left) { ok_col = content_left; }

    /* Hauptflaeche -- einzige Abgrenzung vom Hintergrund dahinter (rahmenlos, s. .h). */
    q9_screenbuf_fill_rect(sb, dlg->row, dlg->col, dlg->rows, dlg->cols, ' ',
                            p->body_fg_r, p->body_fg_g, p->body_fg_b,
                            1, p->body_bg_r, p->body_bg_g, p->body_bg_b);

    /* Kopfzeile: Titel links, "X" rechts (rein dekorativ -- kein Maus-Support, s. .h). Eine Spalte
       weiter rechts als zuvor (Andreas' Wunsch, sechste Runde). */
    q9_screenbuf_fill_rect(sb, dlg->row, dlg->col, 1, dlg->cols, ' ',
                            p->header_fg_r, p->header_fg_g, p->header_fg_b,
                            1, p->header_bg_r, p->header_bg_g, p->header_bg_b);
    q9_screenbuf_puts(sb, dlg->row, dlg->col + 2, dlg->title,
                       p->header_fg_r, p->header_fg_g, p->header_fg_b);
    if (dlg->cols >= 4) {
        q9_screenbuf_puts(sb, dlg->row, dlg->col + dlg->cols - 2, "X",
                           p->header_fg_r, p->header_fg_g, p->header_fg_b);
    }

    /* Spaltentitel-Zeile -- die Hintergrundflaeche (sub_bg) spannt GENAU die Dialogbreite (beruehrt
       also beide Randlinien farblich), der TEXT selbst faengt bei content_left an (die Luftspalte
       zur linken Linie ist damit automatisch mit drin, s. init()). Groesse ganz rechts, Datum
       davor, Name (dynamisch, dlg->name_col_width) fuellt den Rest bis zur rechten Luftspalte. */
    q9_screenbuf_fill_rect(sb, dlg->row + 1, dlg->col, 1, dlg->cols, ' ',
                            p->sub_fg_r, p->sub_fg_g, p->sub_fg_b,
                            1, p->sub_bg_r, p->sub_bg_g, p->sub_bg_b);
    snprintf(line, sizeof(line), "%-*.*s %*s %*s",
             dlg->name_col_width, dlg->name_col_width, "Name",
             Q9_FILEDIALOG_DATE_COL, "Datum", Q9_FILEDIALOG_SIZE_COL, "Groesse");
    q9_screenbuf_puts(sb, dlg->row + 1, content_left, line, p->sub_fg_r, p->sub_fg_g, p->sub_fg_b);

    /* Dateiliste -- die markierte Zeile bekommt NUR dann die kraeftige sel_fg/sel_bg-Hervorhebung,
       wenn die Liste tatsaechlich den Fokus hat; sonst die gedaempfte unfocus_sel_*-Variante (s.
       .h Kopfkommentar zu diesem Feld -- sonst ist ein Fokuswechsel auf/von der Liste unsichtbar,
       weil die Markierung vorher immer gleich aussah). */
    {
        int list_focus = (dlg->focus == Q9_FILEDIALOG_FOCUS_LIST);
        q9_listview_render(&dlg->list, sb, dlg->row_ptr,
                            p->list_fg_r, p->list_fg_g, p->list_fg_b,
                            list_focus ? p->sel_fg_r : p->unfocus_sel_fg_r,
                            list_focus ? p->sel_fg_g : p->unfocus_sel_fg_g,
                            list_focus ? p->sel_fg_b : p->unfocus_sel_fg_b,
                            list_focus ? p->sel_bg_r : p->unfocus_sel_bg_r,
                            list_focus ? p->sel_bg_g : p->unfocus_sel_bg_g,
                            list_focus ? p->sel_bg_b : p->unfocus_sel_bg_b);
    }

    /* Fussbereich -- eigene Hintergrundfarbe von der Namenszeile bis zur unteren Halbblock-Kappe,
       damit sich dieser Bereich sichtbar von der Dateiliste absetzt. Die NEUE Statuszeile ganz
       unten (s.u.) hat bewusst eine EIGENE, andere Farbe (wie die Kopfzeile) -- gehoert farblich
       NICHT zu diesem Fussbereich. */
    q9_screenbuf_fill_rect(sb, datei_row, dlg->col, cap_below_row - datei_row + 1, dlg->cols, ' ',
                            p->footer_fg_r, p->footer_fg_g, p->footer_fg_b,
                            1, p->footer_bg_r, p->footer_bg_g, p->footer_bg_b);

    /* Namenszeile -- "Datei: " + der Name in einem eigenen, abgesetzten Kaestchen (sub_fg/bg, wie
       die Spaltentitel-Zeile). Das Kaestchen ist jetzt SCHMALER als dlg->name_col_width (endet 1
       Zeichen VOR ok_col, sonst wuerde es mit der oberen Halbblock-Kappe von OK kollidieren, s.o.)
       -- der Name wird deshalb mit "..." gekuerzt, wenn er nicht mehr passt (Andreas' Wunsch). */
    {
        const char *sel_name = (dlg->list.selected >= 0 && dlg->list.selected < dlg->files.count)
                                ? dlg->files.entry[dlg->list.selected].name : "-";
        static const char label[] = "Datei:  ";                /* auf Q9_FILEDIALOG_LABEL_WIDTH   */
        int box_col = content_left + Q9_FILEDIALOG_LABEL_WIDTH;
        int box_width = ok_col - 1 - box_col;                  /* 1 Zeichen Luft vor der Kappe     */
        char namebuf[Q9_FILELIST_NAME_MAX];
        if (box_width < 1) { box_width = 1; }

        q9_screenbuf_puts(sb, datei_row, content_left, label,
                           p->footer_fg_r, p->footer_fg_g, p->footer_fg_b);
        truncate_ellipsis(namebuf, sizeof(namebuf), sel_name, box_width);
        q9_screenbuf_fill_rect(sb, datei_row, box_col, 1, box_width, ' ',
                                p->sub_fg_r, p->sub_fg_g, p->sub_fg_b,
                                1, p->sub_bg_r, p->sub_bg_g, p->sub_bg_b);
        q9_screenbuf_puts(sb, datei_row, box_col, namebuf, p->sub_fg_r, p->sub_fg_g, p->sub_fg_b);
    }

    /* Filterzeile -- "Filter: " + Extensions-Umschalter mit Dropdown-Pfeil (▾, s. q9_screenbuf.h)
       -- Enter/Pfeil-runter darauf oeffnet das Aufklapp-Menue mit allen Filtern
       (render_filter_popup(), ganz am Ende dieser Funktion), Pfeil links/rechts bleibt als
       Schnellzugriff erhalten. Kurz genug, um nicht mit den Buttons zu kollidieren (Andreas:
       "Filter bleibt so ist kurz genug") -- KEINE Kuerzung noetig, anders als bei "Datei:". */
    {
        static const char label[] = "Filter: ";                /* auf Q9_FILEDIALOG_LABEL_WIDTH   */
        char filt[Q9_FILEDIALOG_FILTER_MAX + 4];
        int flen, fcol;

        q9_screenbuf_puts(sb, filter_buttons_row, content_left, label,
                           p->footer_fg_r, p->footer_fg_g, p->footer_fg_b);

        snprintf(filt, sizeof(filt), "%s ", dlg->filters[dlg->filter_index]);
        flen = (int)strlen(filt);
        filt[flen]     = (char)Q9_GLYPH_DOWN_ARROW;
        filt[flen + 1] = '\0';
        flen += 1;
        fcol = content_left + Q9_FILEDIALOG_LABEL_WIDTH;
        if (filter_focus) {
            q9_screenbuf_fill_rect(sb, filter_buttons_row, fcol, 1, flen, ' ',
                                    p->focus_fg_r, p->focus_fg_g, p->focus_fg_b,
                                    1, p->focus_bg_r, p->focus_bg_g, p->focus_bg_b);
        }
        q9_screenbuf_puts(sb, filter_buttons_row, fcol, filt,
                           filter_focus ? p->focus_fg_r : p->footer_fg_r,
                           filter_focus ? p->focus_fg_g : p->footer_fg_g,
                           filter_focus ? p->focus_fg_b : p->footer_fg_b);
    }

    /* Buttons -- "richtige" Buttons: sub_fg/bg (Spaltentitel-Farbe) wenn unfokussiert, focus_fg/bg
       wenn fokussiert, rechtsbuendig mit 1 Zeichen Luft vor der rechten Linie (content_right, s.o.),
       per Halbblock-Kappen "aufgeblasen" (draw_button()). Seit der sechsten Runde liegt der
       Haupttext auf filter_buttons_row (teilt sich die Zeile mit "Filter: ..."), die obere Kappe
       auf datei_row (teilt sich die Zeile mit "Datei: ..."), die untere Kappe auf cap_below_row
       (eigene, unveraenderte Zeile) -- s. Kopfkommentar. */
    draw_button(sb, filter_buttons_row, ok_col, button_w, "OK",
                ok_focus ? p->focus_fg_r : p->sub_fg_r,
                ok_focus ? p->focus_fg_g : p->sub_fg_g,
                ok_focus ? p->focus_fg_b : p->sub_fg_b,
                ok_focus ? p->focus_bg_r : p->sub_bg_r,
                ok_focus ? p->focus_bg_g : p->sub_bg_g,
                ok_focus ? p->focus_bg_b : p->sub_bg_b,
                datei_row, cap_below_row,
                p->footer_bg_r, p->footer_bg_g, p->footer_bg_b);

    draw_button(sb, filter_buttons_row, cancel_col, button_w, "Abbrechen",
                cancel_focus ? p->focus_fg_r : p->sub_fg_r,
                cancel_focus ? p->focus_fg_g : p->sub_fg_g,
                cancel_focus ? p->focus_fg_b : p->sub_fg_b,
                cancel_focus ? p->focus_bg_r : p->sub_bg_r,
                cancel_focus ? p->focus_bg_g : p->sub_bg_g,
                cancel_focus ? p->focus_bg_b : p->sub_bg_b,
                datei_row, cap_below_row,
                p->footer_bg_r, p->footer_bg_g, p->footer_bg_b);

    /* NEUE Statuszeile ganz unten ("auch eine Statuszeile wie im Hauptfenster") -- volle Breite,
       eigene Farbe (dieselbe wie die Kopfzeile, fuer ein symmetrisches Erscheinungsbild oben/unten).
       Gekuerzter Text mit Trennstrichen (Andreas' Wunsch, sechste Runde: "der Text ist zu lang,
       steht zwei Zeichen ueber... bitte auch dort mit senkrechten Strichen teilen") -- 1 Zeichen
       Luft an beiden Seiten, wie beim Rest des Dialogs. */
    q9_screenbuf_fill_rect(sb, bottom_status_row, dlg->col, 1, dlg->cols, ' ',
                            p->header_fg_r, p->header_fg_g, p->header_fg_b,
                            1, p->header_bg_r, p->header_bg_g, p->header_bg_b);
    {
        char status_line[64];
        char sep[2];
        sep[0] = (char)Q9_GLYPH_VLINE; sep[1] = '\0';
        snprintf(status_line, sizeof(status_line), "TAB: weiter %s Enter: OK %s Esc: Abbruch",
                 sep, sep);
        q9_screenbuf_puts(sb, bottom_status_row, content_left, status_line,
                           p->header_fg_r, p->header_fg_g, p->header_fg_b);
    }

    /* Rahmenlinien -- links (von diesem Modul komplett selbst gezeichnet) UND rechts (fuer die
       Spaltentitel-/Fusszeilen von diesem Modul, fuer die Listenzeilen selbst von q9_listview.c
       IMMER als Linie mitgezeichnet, s. dort) -- spannen von der Spaltentitel-Zeile bis zur
       unteren Halbblock-Kappe, GENAU bis zur neuen Statuszeile. Bewusst GANZ AM ENDE gezeichnet
       (nach allen Hintergrund-Fuellungen UND nach q9_listview_render()). Die Randzellen bekommen
       IMMER body_bg als Hintergrund (Andreas' Wunsch, sechste Runde: "die dunklere [Farbe] aus der
       Zeile darunter") -- dafuer wird hier fill_rect (setzt Zeichen+Vorder-+Hintergrund) statt nur
       puts() (laesst den Hintergrund unangetastet) verwendet, damit die Linie sich als eigene,
       durchgehend dunkle "Rinne" von der jeweiligen Zeilenfarbe abhebt, statt sie zu uebernehmen.
       Die rechte Linie ueberspringt bewusst den Listenbereich selbst (dlg->list.row..+height-1) --
       dort hat q9_listview_render() bereits die richtige Linie (inkl. Scroll-Griff, falls noetig)
       gezeichnet; deren Hintergrund ist ohnehin schon body_bg (s. dort, kein eigener Zeilen-
       Hintergrund fuer unselektierte Listenzeilen), ein zweiter Durchgang ist dort unnoetig. */
    {
        int left_border_col = dlg->col;
        int span_top    = dlg->row + 1;
        int span_bottom = cap_below_row;
        int list_top    = dlg->list.row;
        int list_bottom = dlg->list.row + dlg->list.height - 1;
        int r;

        for (r = span_top; r <= span_bottom; r++) {
            q9_screenbuf_fill_rect(sb, r, left_border_col, 1, 1, (char)Q9_GLYPH_VLINE,
                                    p->list_fg_r, p->list_fg_g, p->list_fg_b,
                                    1, p->body_bg_r, p->body_bg_g, p->body_bg_b);
            if (r < list_top || r > list_bottom) {
                q9_screenbuf_fill_rect(sb, r, right_border_col, 1, 1, (char)Q9_GLYPH_VLINE,
                                        p->list_fg_r, p->list_fg_g, p->list_fg_b,
                                        1, p->body_bg_r, p->body_bg_g, p->body_bg_b);
            }
        }
    }

    if (dlg->filter_popup_open) {
        render_filter_popup(dlg, sb, filter_buttons_row);
    }
}

//────────────────────────────────────────────────────────────────────────────────────────────────
// EOF q9_filedialog.c                                                                     Ver. 1.60
//────────────────────────────────────────────────────────────────────────────────────────────────
