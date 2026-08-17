//════════════════════════════════════════════════════════════════════════════════════════════════
// File:   q9_listview.c                                                                   Ver. 1.30
// Owner:  Claudia
// Desc.:  Implementierung, siehe q9_listview.h.
//
// Edition History
//─────────┬──────┬────────────────────────────────────────────────────────────────────────┬──────
// Date    │ Ver. │ Description                                                            │ By
//─────────┼──────┼────────────────────────────────────────────────────────────────────────┬──────
// 26-08-16│ 1.00 │ Erster Wurf                                                              │ Cld
// 26-08-17│ 1.10 │ Proportionaler Scrollbalken-Griff (Andreas), Q9_GLYPH_VLINE/BLOCK statt   │ Cld
//         │      │ ASCII                                                                      │
// 26-08-17│ 1.20 │ Ein Zeichen Abstand zwischen markierter Zeile und Scrollbalken (Andreas:   │ Cld
//         │      │ "verschmilzt sonst")                                                       │
// 26-08-17│ 1.30 │ Rechte Spalte zeigt jetzt IMMER die Linie (Andreas: "wird keine Laufleiste │ Cld
//         │      │ benoetigt ist es einfach der normale Strich") -- ersetzt die -2-Sonderregel │
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
    int line_col;

    if (!lv || !sb || !items) { return; }

    has_scrollbar = (lv->item_count > lv->height) ? 1 : 0;
    /* Rechte Spalte IMMER fuer die Bildlaufleiste reserviert (Andreas' Wunsch, 2026-08-17: "wird
       keine Laufleiste benoetigt ist es einfach der normale Strich" -- die Spalte zeigt IMMER
       mindestens die Linie, unabhaengig davon, ob tatsaechlich etwas zu scrollen ist, sowohl im
       Hauptfenster als auch im Datei-Dialog). Ersetzt die fruehere "-2 nur wenn Scrollbalken
       noetig"-Sonderregel (Luecken-Spalte vor dem Balken) -- die durchgehende Linie ist jetzt
       selbst die Abgrenzung zur markierten Zeile, keine separate Luecke mehr noetig. */
    content_width = lv->width - 1;
    if (content_width < 1) { content_width = 1; }
    line_col = lv->col + lv->width - 1;

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

    /* Die rechte Spalte -- IMMER die Linie (auch ohne Scrollbedarf), der Griff (falls noetig) wird
       zusaetzlich darauf gelegt. */
    {
        char track_str[2];
        track_str[0] = Q9_GLYPH_VLINE; track_str[1] = '\0';
        for (i = 0; i < lv->height; i++) {
            q9_screenbuf_puts(sb, lv->row + i, line_col, track_str, fg_r, fg_g, fg_b);
        }
    }

    if (has_scrollbar) {
        int max_offset = lv->item_count - lv->height;      /* > 0 garantiert, s. has_scrollbar oben  */
        int thumb_height, thumb_start, max_thumb_start;
        char thumb_str[2];
        thumb_str[0] = Q9_GLYPH_BLOCK; thumb_str[1] = '\0';
        if (max_offset < 1) { max_offset = 1; }             /* defensiv, Divisionsschutz              */

        /* Proportionale Griffgroesse (Andreas' Wunsch, 2026-08-17): der Griff nimmt denselben Anteil
           der Balkenhoehe ein wie der sichtbare Anteil der Liste (height/item_count) -- bei 50%
           sichtbar also auch 50% Griffhoehe. Mindestens 1 Zeile; hoechstens height-1, damit IMMER
           erkennbar bleibt, dass es ueberhaupt etwas zu scrollen gibt (ein Griff so gross wie die
           ganze Spur saehe wie "nichts zu scrollen" aus -- kann bei has_scrollbar aber ohnehin nicht
           passieren, da item_count>height hier garantiert ist). */
        thumb_height = (lv->height * lv->height) / lv->item_count;
        if (thumb_height < 1)              { thumb_height = 1; }
        if (thumb_height > lv->height - 1) { thumb_height = lv->height - 1; }
        if (thumb_height < 1)              { thumb_height = 1; }   /* height==1: height-1==0-Randfall */

        max_thumb_start = lv->height - thumb_height;
        if (max_thumb_start < 1) { max_thumb_start = 1; }
        thumb_start = (lv->scroll_offset * max_thumb_start) / max_offset;
        if (thumb_start > lv->height - thumb_height) { thumb_start = lv->height - thumb_height; }
        if (thumb_start < 0)                          { thumb_start = 0; }

        for (i = 0; i < thumb_height; i++) {
            q9_screenbuf_puts(sb, lv->row + thumb_start + i, line_col, thumb_str,
                               sel_bg_r, sel_bg_g, sel_bg_b);
        }
    }
}

//────────────────────────────────────────────────────────────────────────────────────────────────
// EOF q9_listview.c                                                                       Ver. 1.30
//────────────────────────────────────────────────────────────────────────────────────────────────
