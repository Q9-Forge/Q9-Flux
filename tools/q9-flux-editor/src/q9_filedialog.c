//════════════════════════════════════════════════════════════════════════════════════════════════
// File:   q9_filedialog.c                                                                 Ver. 1.40
// Owner:  Claudia
// Desc.:  Implementierung, siehe q9_filedialog.h. Layout in Zeilen relativ zu dlg->row (rows==Hoehe
//         des Dialogs, s. layout_rows() -- EINZIGE Stelle, die diese Aufteilung kennt, init() und
//         render() rufen beide dieselbe Funktion auf):
//             0            Kopfzeile (Titel + "X")
//             1            Spaltentitel ("Name  Datum  Groesse")
//             2..rows-5    Dateiliste (Hoehe = rows-6)
//             rows-4       Auswahl-/Filterzeile               ┐
//             rows-3       Halbblock-Kappe UEBER den Buttons   │ FUSSBEREICH, eigene footer_bg-
//             rows-2       Buttons OK / Abbrechen              │ Hintergrundfarbe (s. render())
//             rows-1       Halbblock-Kappe UNTER den Buttons   ┘
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

/* Zentrale Geometrie fuer den Fussbereich -- s. Kopfkommentar. Alle Rueckgabe-Zeiger duerfen NULL
   sein (Aufrufer holt sich nur, was er braucht: init() nur list_height, render() den Rest). Werte
   sind Zeilen-OFFSETS relativ zu dlg->row, noch NICHT damit addiert. */
static void layout_rows(int rows, int *list_height, int *status_row, int *cap_above_row,
                         int *buttons_row, int *cap_below_row)
{
    int lh = rows - 6;
    if (lh < 1) { lh = 1; }
    if (list_height)  { *list_height  = lh; }
    if (status_row)    { *status_row    = rows - 4; }
    if (cap_above_row) { *cap_above_row = rows - 3; }
    if (buttons_row)   { *buttons_row   = rows - 2; }
    if (cap_below_row) { *cap_below_row = rows - 1; }
}

/* Formatiert alle aktuell in dlg->files stehenden Eintraege zu spaltenausgerichteten Zeilen fuer
   q9_listview_render (s. Kopfkommentar q9_filedialog.h -- flaches String-Array, kein eigenes
   Mehrspalten-Feature in q9_listview.c). Name-Spalte in dlg->name_col_width (dynamisch, EINMAL in
   init() berechnet, s. dort) -- Groesse ganz rechts, Datum davor, Name fuellt den Rest
   (Andreas' Wunsch, 2026-08-17). */
static void format_rows(q9_filedialog_t *dlg)
{
    int i;
    for (i = 0; i < dlg->files.count; i++) {
        const q9_fileentry_t *e = &dlg->files.entry[i];
        snprintf(dlg->row_text[i], sizeof(dlg->row_text[i]), "%-*.*s %*s %*s",
                 dlg->name_col_width, dlg->name_col_width, e->name,
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
       nie row/col/height/width. Spaltenaufteilung (Andreas' Wunsch, 2026-08-17, zweite Runde: "bei
       Rahmenkanten kommen nach ganz aussen, und rechts zusaetzlich die Bildlaufleiste auf den
       gleichen Strich"): links EINE Spalte GENAU am Dialogrand fuer die vom Dialog selbst
       gezeichnete Linie (render()), rechts KEIN eigener Rand mehr -- q9_listview's eigene rechte
       Spalte (immer Linie, Griff nur bei Bedarf, s. q9_listview.c) liegt jetzt direkt AUF der
       rechten Dialogkante, keine separate "Fensterkante + danebenliegende Bildlaufleiste" mehr. */
    layout_rows(dlg->rows, &list_height, NULL, NULL, NULL, NULL);
    dlg->list.row    = dlg->row + 2;
    dlg->list.col    = dlg->col + 1;
    dlg->list.height = list_height;
    dlg->list.width  = dlg->cols - 1;
    if (dlg->list.width < 1) { dlg->list.width = 1; }

    /* Name-Spaltenbreite: der Rest der Listenbreite, nachdem q9_listview's eigene rechte Spalte (1
       Zeichen, s.o.) sowie Trennzeichen/Datum/Groesse abgezogen sind -- "Dateiname so lang wie der
       Rest". */
    {
        int content_width = dlg->list.width - 1;             /* -1 == q9_listview's eigene Linie/Leiste */
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

/* Zentriert label in ein Feld der Breite width (leerzeichenaufgefuellt), z.B. "OK" in einem 13
   Zeichen breiten Button -> "     OK      ". Schneidet label ab, falls es nicht passt (sollte bei
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
   die beiden Zeilen fuer die Kappen, footer_bg die Hintergrundfarbe DORT (die "aeussere" Haelfte
   jeder Kappe zeigt diese Farbe, die "innere" -- an den Button angrenzende -- Haelfte fg/bg). */
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

/* Zeichnet das Filter-Aufklapp-Menue (dlg->filter_popup_open) ueber der Statuszeile -- waechst nach
   OBEN in den Bereich, den sonst die Dateiliste einnimmt (unterhalb der Buttons ist kein Platz
   mehr, s. Layout-Uebersicht .h), rechtsbuendig unter dem Filterfeld. Wird ganz am Ende von
   q9_filedialog_render() aufgerufen, ueberzeichnet also bewusst alles darunter. */
static void render_filter_popup(const q9_filedialog_t *dlg, q9_screenbuf_t *sb, int status_row_abs)
{
    const q9_filedialog_palette_t *p = &dlg->pal;
    int avail = status_row_abs - (dlg->row + 2);            /* Platz unter der Spaltentitel-Zeile   */
    int popup_h = dlg->filter_count;
    int popup_w = Q9_FILEDIALOG_FILTER_MAX + 2;
    int popup_row, popup_col, i;

    if (popup_h > avail) { popup_h = avail; }
    if (popup_h < 1)     { popup_h = 1; }
    if (popup_w > dlg->cols - 2) { popup_w = dlg->cols - 2; }
    if (popup_w < 1)     { popup_w = 1; }

    popup_row = status_row_abs - popup_h;
    /* Rechtsbuendig mit der rechten Linie der Dateiliste (nicht mehr am absoluten Dialogrand,
       s. q9_filedialog_render() -- "Buttons und Filter wandern etwas nach links"). */
    popup_col = (dlg->list.col + dlg->list.width - 1) - popup_w + 1;
    if (popup_col < dlg->col + 1) { popup_col = dlg->col + 1; }

    q9_screenbuf_fill_rect(sb, popup_row, popup_col, popup_h, popup_w, ' ',
                            p->sub_fg_r, p->sub_fg_g, p->sub_fg_b,
                            1, p->sub_bg_r, p->sub_bg_g, p->sub_bg_b);

    for (i = 0; i < dlg->filter_count && i < popup_h; i++) {
        int is_sel = (i == dlg->filter_popup_index);
        char line[Q9_FILEDIALOG_FILTER_MAX + 4];
        snprintf(line, sizeof(line), " %-*.*s", popup_w - 1, popup_w - 1, dlg->filters[i]);
        if (is_sel) {
            q9_screenbuf_fill_rect(sb, popup_row + i, popup_col, 1, popup_w, ' ',
                                    p->focus_fg_r, p->focus_fg_g, p->focus_fg_b,
                                    1, p->focus_bg_r, p->focus_bg_g, p->focus_bg_b);
        }
        q9_screenbuf_puts(sb, popup_row + i, popup_col, line,
                           is_sel ? p->focus_fg_r : p->sub_fg_r,
                           is_sel ? p->focus_fg_g : p->sub_fg_g,
                           is_sel ? p->focus_fg_b : p->sub_fg_b);
    }
}

void q9_filedialog_render(const q9_filedialog_t *dlg, q9_screenbuf_t *sb)
{
    const q9_filedialog_palette_t *p;
    char line[96];
    int status_row, cap_above_row, buttons_row, cap_below_row;
    int ok_focus, cancel_focus, filter_focus;

    if (!dlg || !sb) { return; }
    p = &dlg->pal;
    layout_rows(dlg->rows, NULL, &status_row, &cap_above_row, &buttons_row, &cap_below_row);
    status_row    += dlg->row;
    cap_above_row += dlg->row;
    buttons_row   += dlg->row;
    cap_below_row += dlg->row;
    filter_focus = (dlg->focus == Q9_FILEDIALOG_FOCUS_FILTER);
    ok_focus     = (dlg->focus == Q9_FILEDIALOG_FOCUS_OK);
    cancel_focus = (dlg->focus == Q9_FILEDIALOG_FOCUS_CANCEL);

    /* Hauptflaeche -- einzige Abgrenzung vom Hintergrund dahinter (rahmenlos, s. .h). */
    q9_screenbuf_fill_rect(sb, dlg->row, dlg->col, dlg->rows, dlg->cols, ' ',
                            p->body_fg_r, p->body_fg_g, p->body_fg_b,
                            1, p->body_bg_r, p->body_bg_g, p->body_bg_b);

    /* Kopfzeile: Titel links, "X" rechts (rein dekorativ -- kein Maus-Support, s. .h). */
    q9_screenbuf_fill_rect(sb, dlg->row, dlg->col, 1, dlg->cols, ' ',
                            p->header_fg_r, p->header_fg_g, p->header_fg_b,
                            1, p->header_bg_r, p->header_bg_g, p->header_bg_b);
    q9_screenbuf_puts(sb, dlg->row, dlg->col + 2, dlg->title,
                       p->header_fg_r, p->header_fg_g, p->header_fg_b);
    if (dlg->cols >= 4) {
        q9_screenbuf_puts(sb, dlg->row, dlg->col + dlg->cols - 3, "X",
                           p->header_fg_r, p->header_fg_g, p->header_fg_b);
    }

    /* Spaltentitel-Zeile -- GENAU so breit wie die Tabelle darunter (linke bis zur rechten Kante,
       s.u.), NICHT die volle Dialogbreite (Andreas' Wunsch). Groesse ganz rechts, Datum davor,
       Name (dynamisch, dlg->name_col_width) fuellt den Rest. Der TEXT beginnt bei dlg->list.col
       (nicht bei table_left/der Randspalte selbst!) -- sonst stuende "Name" eine Spalte weiter
       links als die tatsaechlichen Dateinamen darunter. table_left/table_width bestimmen NUR die
       Hintergrundflaeche (schliesst beide Randspalten farblich mit ein -- die Rahmenlinien selbst
       kommen gleich danach, s.u., und ueberschreiben die aeussersten beiden Zellen dieser Zeile). */
    {
        int table_left  = dlg->list.col - 1;
        int table_width = (dlg->list.col + dlg->list.width - 1) - table_left + 1;
        q9_screenbuf_fill_rect(sb, dlg->row + 1, table_left, 1, table_width, ' ',
                                p->sub_fg_r, p->sub_fg_g, p->sub_fg_b,
                                1, p->sub_bg_r, p->sub_bg_g, p->sub_bg_b);
        snprintf(line, sizeof(line), "%-*.*s %*s %*s",
                 dlg->name_col_width, dlg->name_col_width, "Name",
                 Q9_FILEDIALOG_DATE_COL, "Datum", Q9_FILEDIALOG_SIZE_COL, "Groesse");
        q9_screenbuf_puts(sb, dlg->row + 1, dlg->list.col, line, p->sub_fg_r, p->sub_fg_g, p->sub_fg_b);
    }

    /* Tabellen-Rahmen (Andreas' Wunsch, 2026-08-17, zweite Runde: "der Strich geht dann allerdings
       ab dem Header") -- links eine feste Linie (dieses Modul, q9_listview kennt nur seine EIGENE
       rechte Spalte), rechts fuer die Listenzeilen selbst die von q9_listview.c IMMER gezeichnete
       Linie/Bildlaufleiste (s.u., liegt jetzt direkt auf der rechten Dialogkante, s. init()) --
       beide spannen ab der Spaltentitel-Zeile bis zum Ende der Liste (NICHT den Fussbereich, der
       hat eine eigene Abgrenzung ueber footer_bg statt Linien, s.u.). Fuer die Spaltentitel-Zeile
       selbst zeichnet dieses Modul die rechte Randzelle mit -- q9_listview kennt diese Zeile gar
       nicht (sie ist nicht Teil seines Viewports). */
    {
        int left_border_col  = dlg->list.col - 1;
        int right_border_col = dlg->list.col + dlg->list.width - 1;
        int r;
        char vline[2];
        vline[0] = (char)Q9_GLYPH_VLINE; vline[1] = '\0';
        for (r = 0; r < 1 + dlg->list.height; r++) {
            q9_screenbuf_puts(sb, dlg->row + 1 + r, left_border_col, vline,
                               p->list_fg_r, p->list_fg_g, p->list_fg_b);
        }
        q9_screenbuf_puts(sb, dlg->row + 1, right_border_col, vline,
                           p->list_fg_r, p->list_fg_g, p->list_fg_b);
    }

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

    /* Fussbereich -- eigene Hintergrundfarbe ab der Statuszeile bis zum Dialogende, damit sich
       dieser Bereich sichtbar von der Dateiliste absetzt (Andreas' Wunsch, 2026-08-17). Deckt
       Auswahl-/Filterzeile, beide Halbblock-Kappen UND die Buttons-Zeile ab -- die Buttons selbst
       (s.u.) uebermalen ihren eigenen Ausschnitt gleich wieder mit sub_ bzw. focus_ Farben. */
    q9_screenbuf_fill_rect(sb, status_row, dlg->col, cap_below_row - status_row + 1, dlg->cols, ' ',
                            p->footer_fg_r, p->footer_fg_g, p->footer_fg_b,
                            1, p->footer_bg_r, p->footer_bg_g, p->footer_bg_b);

    /* Rechte Bezugsspalte fuer Filter UND Buttons (Andreas' Wunsch: "wandern etwas nach links damit
       Platz fuer den Rahmen/Bildlaufleiste ist") -- buendig mit der rechten Linie der Dateiliste,
       nicht mehr am absoluten Dialogrand. */
    {
        int right_border_col = dlg->list.col + dlg->list.width - 1;

        /* Auswahl-/Filterzeile -- links "Datei: " + der Name in einem eigenen, abgesetzten Kaestchen
           (sub_fg/bg, wie die Spaltentitel-Zeile -- Andreas' Wunsch: "Hintergrundfarbe noch mal
           abgesetzt... als Kaestchen fuer den Namen"), rechts der Extensions-Umschalter mit Dropdown-
           Pfeil (▾, s. q9_screenbuf.h) -- Enter/Pfeil-runter darauf oeffnet das Aufklapp-Menue mit
           allen Filtern (render_filter_popup(), ganz am Ende dieser Funktion), Pfeil links/rechts
           bleibt als Schnellzugriff (einzeln durchschalten) erhalten. */
        {
            const char *sel_name = (dlg->list.selected >= 0 && dlg->list.selected < dlg->files.count)
                                    ? dlg->files.entry[dlg->list.selected].name : "-";
            static const char label[] = "Datei: ";
            int label_len = (int)sizeof(label) - 1;
            int box_col = dlg->col + 1 + label_len;
            char namebuf[Q9_FILELIST_NAME_MAX];

            q9_screenbuf_puts(sb, status_row, dlg->col + 1, label,
                               p->footer_fg_r, p->footer_fg_g, p->footer_fg_b);
            snprintf(namebuf, sizeof(namebuf), "%-*.*s",
                     dlg->name_col_width, dlg->name_col_width, sel_name);
            q9_screenbuf_fill_rect(sb, status_row, box_col, 1, dlg->name_col_width, ' ',
                                    p->sub_fg_r, p->sub_fg_g, p->sub_fg_b,
                                    1, p->sub_bg_r, p->sub_bg_g, p->sub_bg_b);
            q9_screenbuf_puts(sb, status_row, box_col, namebuf, p->sub_fg_r, p->sub_fg_g, p->sub_fg_b);

            {
                char filt[Q9_FILEDIALOG_FILTER_MAX + 4];
                int flen, fcol;
                snprintf(filt, sizeof(filt), "%s ", dlg->filters[dlg->filter_index]);
                flen = (int)strlen(filt);
                filt[flen]     = (char)Q9_GLYPH_DOWN_ARROW;
                filt[flen + 1] = '\0';
                flen += 1;
                fcol = right_border_col - flen + 1;          /* letztes Zeichen endet AN der Linie */
                if (fcol < dlg->col + 1) { fcol = dlg->col + 1; }
                if (filter_focus) {
                    q9_screenbuf_fill_rect(sb, status_row, fcol, 1, flen, ' ',
                                            p->focus_fg_r, p->focus_fg_g, p->focus_fg_b,
                                            1, p->focus_bg_r, p->focus_bg_g, p->focus_bg_b);
                }
                q9_screenbuf_puts(sb, status_row, fcol, filt,
                                   filter_focus ? p->focus_fg_r : p->footer_fg_r,
                                   filter_focus ? p->focus_fg_g : p->footer_fg_g,
                                   filter_focus ? p->focus_fg_b : p->footer_fg_b);
            }
        }

        /* Buttons -- "richtige" Buttons (Andreas' Wunsch, 2026-08-17): sub_fg/bg (Spaltentitel-Farbe)
           wenn unfokussiert, focus_fg/bg wenn fokussiert, gleich breit, rechtsbuendig (buendig mit
           der Linie der Dateiliste, s.o.), per Halbblock-Kappen "aufgeblasen" (draw_button(), s.
           Kopfkommentar .h). */
        {
            int button_w = (int)strlen("Abbrechen") + 4;
            int cancel_col = right_border_col - button_w + 1;   /* letztes Zeichen endet AN der Linie */
            int ok_col = cancel_col - 1 - button_w;
            if (ok_col < dlg->col + 1) { ok_col = dlg->col + 1; }

            draw_button(sb, buttons_row, ok_col, button_w, "OK",
                        ok_focus ? p->focus_fg_r : p->sub_fg_r,
                        ok_focus ? p->focus_fg_g : p->sub_fg_g,
                        ok_focus ? p->focus_fg_b : p->sub_fg_b,
                        ok_focus ? p->focus_bg_r : p->sub_bg_r,
                        ok_focus ? p->focus_bg_g : p->sub_bg_g,
                        ok_focus ? p->focus_bg_b : p->sub_bg_b,
                        cap_above_row, cap_below_row,
                        p->footer_bg_r, p->footer_bg_g, p->footer_bg_b);

            draw_button(sb, buttons_row, cancel_col, button_w, "Abbrechen",
                        cancel_focus ? p->focus_fg_r : p->sub_fg_r,
                        cancel_focus ? p->focus_fg_g : p->sub_fg_g,
                        cancel_focus ? p->focus_fg_b : p->sub_fg_b,
                        cancel_focus ? p->focus_bg_r : p->sub_bg_r,
                        cancel_focus ? p->focus_bg_g : p->sub_bg_g,
                        cancel_focus ? p->focus_bg_b : p->sub_bg_b,
                        cap_above_row, cap_below_row,
                        p->footer_bg_r, p->footer_bg_g, p->footer_bg_b);
        }
    }

    if (dlg->filter_popup_open) {
        render_filter_popup(dlg, sb, status_row);
    }
}

//────────────────────────────────────────────────────────────────────────────────────────────────
// EOF q9_filedialog.c                                                                     Ver. 1.40
//────────────────────────────────────────────────────────────────────────────────────────────────
