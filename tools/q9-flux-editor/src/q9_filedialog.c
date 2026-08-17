//════════════════════════════════════════════════════════════════════════════════════════════════
// File:   q9_filedialog.c                                                                 Ver. 1.10
// Owner:  Claudia
// Desc.:  Implementierung, siehe q9_filedialog.h. Layout in Zeilen relativ zu dlg->row (rows==Hoehe
//         des Dialogs):
//             0            Kopfzeile (Titel + "X")
//             1            Spaltentitel ("Name  Datum  Groesse")
//             2..rows-4    Dateiliste (Hoehe = rows-5, s. LIST_HEIGHT unten)
//             rows-3       Auswahl-/Filterzeile
//             rows-2       (frei, body_bg -- kleiner Abstand vor den Buttons)
//             rows-1       Buttons OK / Abbrechen
//         Macht rows kleiner als das Minimum (7) keinen Sinn mehr -- init klemmt die Listenhoehe
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

/* Formatiert alle aktuell in dlg->files stehenden Eintraege zu spaltenausgerichteten Zeilen fuer
   q9_listview_render (s. Kopfkommentar q9_filedialog.h -- flaches String-Array, kein eigenes
   Mehrspalten-Feature in q9_listview.c). */
static void format_rows(q9_filedialog_t *dlg)
{
    int i;
    for (i = 0; i < dlg->files.count; i++) {
        const q9_fileentry_t *e = &dlg->files.entry[i];
        snprintf(dlg->row_text[i], sizeof(dlg->row_text[i]), "%-*.*s %8s %7s",
                 Q9_FILEDIALOG_NAME_COL, Q9_FILEDIALOG_NAME_COL, e->name, e->date, e->size);
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

    dlg->pal = *pal;
    dlg->focus = Q9_FILEDIALOG_FOCUS_LIST;
    dlg->done = 0;

    /* Listen-Geometrie EINMAL berechnen (s. Layout-Uebersicht im Kopfkommentar) -- rescan()
       aendert nur noch item_count, nie row/col/height/width. */
    list_height = dlg->rows - 5;
    if (list_height < 1) { list_height = 1; }
    dlg->list.row    = dlg->row + 2;
    dlg->list.col    = dlg->col + 1;
    dlg->list.height = list_height;
    dlg->list.width  = dlg->cols - 2;
    if (dlg->list.width < 1) { dlg->list.width = 1; }

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

static void cycle_filter(q9_filedialog_t *dlg, int dir)
{
    int f = dlg->filter_index + dir;
    if (f < 0) { f = dlg->filter_count - 1; }
    if (f >= dlg->filter_count) { f = 0; }
    dlg->filter_index = f;
    rescan(dlg);
}

int q9_filedialog_handle_key(q9_filedialog_t *dlg, q9_key_t key)
{
    if (!dlg) { return 0; }
    if (dlg->done != 0) { return dlg->done; }               /* schon entschieden -- eingefroren, s. .h */

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
            if (dlg->focus == Q9_FILEDIALOG_FOCUS_LIST) { q9_listview_move(&dlg->list, 1); }
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
                    cycle_filter(dlg, 1);                    /* Enter auf dem Filter == Pfeil rechts  */
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

void q9_filedialog_render(const q9_filedialog_t *dlg, q9_screenbuf_t *sb)
{
    const q9_filedialog_palette_t *p;
    char line[96];
    int status_row, buttons_row;
    int ok_focus, cancel_focus, filter_focus;

    if (!dlg || !sb) { return; }
    p = &dlg->pal;
    status_row  = dlg->row + dlg->rows - 3;
    buttons_row = dlg->row + dlg->rows - 1;
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

    /* Spaltentitel-Zeile. */
    q9_screenbuf_fill_rect(sb, dlg->row + 1, dlg->col, 1, dlg->cols, ' ',
                            p->sub_fg_r, p->sub_fg_g, p->sub_fg_b,
                            1, p->sub_bg_r, p->sub_bg_g, p->sub_bg_b);
    snprintf(line, sizeof(line), "%-*.*s %8s %7s",
             Q9_FILEDIALOG_NAME_COL, Q9_FILEDIALOG_NAME_COL, "Name", "Datum", "Groesse");
    q9_screenbuf_puts(sb, dlg->row + 1, dlg->col + 1, line, p->sub_fg_r, p->sub_fg_g, p->sub_fg_b);

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

    /* Auswahl-/Filterzeile -- links die ausgewaehlte Datei, rechts der Extensions-Umschalter mit
       Pfeil (nur der Filter-Teil wird bei Fokus hervorgehoben, der Dateiname links ist nicht
       bedienbar). */
    {
        const char *sel_name = (dlg->list.selected >= 0 && dlg->list.selected < dlg->files.count)
                                ? dlg->files.entry[dlg->list.selected].name : "-";
        snprintf(line, sizeof(line), "Datei: %-*.*s", Q9_FILEDIALOG_NAME_COL, Q9_FILEDIALOG_NAME_COL,
                 sel_name);
        q9_screenbuf_puts(sb, status_row, dlg->col + 1, line, p->body_fg_r, p->body_fg_g, p->body_fg_b);

        {
            char filt[Q9_FILEDIALOG_FILTER_MAX + 4];
            int flen, fcol;
            snprintf(filt, sizeof(filt), "%s >", dlg->filters[dlg->filter_index]);
            flen = (int)strlen(filt);
            fcol = dlg->col + dlg->cols - 1 - flen;
            if (fcol < dlg->col + 1) { fcol = dlg->col + 1; }
            if (filter_focus) {
                q9_screenbuf_fill_rect(sb, status_row, fcol, 1, flen, ' ',
                                        p->focus_fg_r, p->focus_fg_g, p->focus_fg_b,
                                        1, p->focus_bg_r, p->focus_bg_g, p->focus_bg_b);
            }
            q9_screenbuf_puts(sb, status_row, fcol, filt,
                               filter_focus ? p->focus_fg_r : p->body_fg_r,
                               filter_focus ? p->focus_fg_g : p->body_fg_g,
                               filter_focus ? p->focus_fg_b : p->body_fg_b);
        }
    }

    /* Buttons -- fokussiertes Element bekommt focus_fg/focus_bg (analog zur markierten Zeile in
       der Liste), das andere bleibt auf der normalen Hauptflaeche stehen. */
    {
        const char *ok_label = "  OK  ";
        const char *cancel_label = "  Abbrechen  ";
        int ok_col = dlg->col + 2;
        int cancel_col = ok_col + (int)strlen(ok_label) + 3;

        if (ok_focus) {
            q9_screenbuf_fill_rect(sb, buttons_row, ok_col, 1, (int)strlen(ok_label), ' ',
                                    p->focus_fg_r, p->focus_fg_g, p->focus_fg_b,
                                    1, p->focus_bg_r, p->focus_bg_g, p->focus_bg_b);
        }
        q9_screenbuf_puts(sb, buttons_row, ok_col, ok_label,
                           ok_focus ? p->focus_fg_r : p->body_fg_r,
                           ok_focus ? p->focus_fg_g : p->body_fg_g,
                           ok_focus ? p->focus_fg_b : p->body_fg_b);

        if (cancel_focus) {
            q9_screenbuf_fill_rect(sb, buttons_row, cancel_col, 1, (int)strlen(cancel_label), ' ',
                                    p->focus_fg_r, p->focus_fg_g, p->focus_fg_b,
                                    1, p->focus_bg_r, p->focus_bg_g, p->focus_bg_b);
        }
        q9_screenbuf_puts(sb, buttons_row, cancel_col, cancel_label,
                           cancel_focus ? p->focus_fg_r : p->body_fg_r,
                           cancel_focus ? p->focus_fg_g : p->body_fg_g,
                           cancel_focus ? p->focus_fg_b : p->body_fg_b);
    }
}

//────────────────────────────────────────────────────────────────────────────────────────────────
// EOF q9_filedialog.c                                                                     Ver. 1.10
//────────────────────────────────────────────────────────────────────────────────────────────────
