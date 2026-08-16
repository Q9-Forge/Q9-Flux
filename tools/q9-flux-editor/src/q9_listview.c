//════════════════════════════════════════════════════════════════════════════════════════════════
// File:   q9_listview.c                                                                   Ver. 1.00
// Owner:  Claudia
// Desc.:  Implementierung, siehe q9_listview.h.
//
// Edition History
//─────────┬──────┬────────────────────────────────────────────────────────────────────────┬──────
// Date    │ Ver. │ Description                                                            │ By
//─────────┼──────┼────────────────────────────────────────────────────────────────────────┬──────
// 26-08-16│ 1.00 │ Erster Wurf                                                              │ Cld
//═════════╧══════╧════════════════════════════════════════════════════════════════════════╧══════
#include "q9_listview.h"

int q9_listview_scroll(int selected, int offset, int height, int item_count)
{
    int max_offset;

    if (item_count <= 0 || height <= 0) {
        return 0;
    }
    if (selected < 0)              { selected = 0; }
    if (selected > item_count - 1) { selected = item_count - 1; }

    if (selected < offset) {
        offset = selected;                                /* nach oben rausgelaufen -- nachziehen  */
    } else if (selected >= offset + height) {
        offset = selected - height + 1;                    /* nach unten rausgelaufen -- nachziehen */
    }

    max_offset = item_count - height;
    if (max_offset < 0) { max_offset = 0; }
    if (offset > max_offset) { offset = max_offset; }
    if (offset < 0)          { offset = 0; }

    return offset;
}

void q9_listview_init(q9_listview_t *lv, int row, int col, int height, int width, int item_count)
{
    if (!lv) { return; }
    if (height < 1) { height = 1; }
    if (width  < 1) { width  = 1; }
    if (item_count < 0) { item_count = 0; }

    lv->row          = row;
    lv->col          = col;
    lv->height       = height;
    lv->width        = width;
    lv->item_count   = item_count;
    lv->selected     = (item_count > 0) ? 0 : -1;
    lv->scroll_offset = 0;
}

void q9_listview_move(q9_listview_t *lv, int delta)
{
    if (!lv || lv->item_count <= 0) { return; }
    lv->selected += delta;
    if (lv->selected < 0)                  { lv->selected = 0; }
    if (lv->selected > lv->item_count - 1) { lv->selected = lv->item_count - 1; }
    lv->scroll_offset = q9_listview_scroll(lv->selected, lv->scroll_offset, lv->height, lv->item_count);
}

void q9_listview_render(const q9_listview_t *lv, q9_screenbuf_t *sb, const char *const *items,
                         int fg_r, int fg_g, int fg_b,
                         int sel_fg_r, int sel_fg_g, int sel_fg_b,
                         int sel_bg_r, int sel_bg_g, int sel_bg_b)
{
    int i;
    int content_width;
    int has_scrollbar;

    if (!lv || !sb || !items) { return; }

    has_scrollbar = (lv->item_count > lv->height) ? 1 : 0;
    content_width = has_scrollbar ? lv->width - 1 : lv->width;
    if (content_width < 1) { content_width = 1; }

    for (i = 0; i < lv->height; i++) {
        int idx = lv->scroll_offset + i;
        int row = lv->row + i;
        if (idx < 0 || idx >= lv->item_count) {
            continue;                                     /* Viewport-Zeile ohne Eintrag -- leer lassen */
        }
        if (idx == lv->selected) {
            /* Erst die Zeile mit dem Auswahl-Hintergrund fuellen, dann den Text drueberschreiben --
               fill_rect setzt has_bg, puts() laesst den Hintergrund einer Zelle unangetastet (s.
               dortiger Kopfkommentar), deshalb diese Reihenfolge. */
            q9_screenbuf_fill_rect(sb, row, lv->col, 1, content_width, ' ',
                                    sel_fg_r, sel_fg_g, sel_fg_b, 1, sel_bg_r, sel_bg_g, sel_bg_b);
            q9_screenbuf_puts(sb, row, lv->col, items[idx], sel_fg_r, sel_fg_g, sel_fg_b);
        } else {
            q9_screenbuf_puts(sb, row, lv->col, items[idx], fg_r, fg_g, fg_b);
        }
    }

    if (has_scrollbar) {
        int sb_col = lv->col + lv->width - 1;
        int max_offset = lv->item_count - lv->height;      /* > 0 garantiert, s. has_scrollbar oben  */
        int thumb_row;
        if (max_offset < 1) { max_offset = 1; }             /* defensiv, Divisionsschutz              */

        for (i = 0; i < lv->height; i++) {
            q9_screenbuf_puts(sb, lv->row + i, sb_col, "|", fg_r, fg_g, fg_b);
        }
        thumb_row = lv->row + (lv->scroll_offset * (lv->height - 1)) / max_offset;
        q9_screenbuf_puts(sb, thumb_row, sb_col, "#", sel_bg_r, sel_bg_g, sel_bg_b);
    }
}

//────────────────────────────────────────────────────────────────────────────────────────────────
// EOF q9_listview.c                                                                       Ver. 1.00
//────────────────────────────────────────────────────────────────────────────────────────────────
