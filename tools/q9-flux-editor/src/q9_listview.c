//════════════════════════════════════════════════════════════════════════════════════════════════
// File:   q9_listview.c                                                                   Ver. 1.60
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
// 26-08-17│ 1.40 │ Fuenfte Feedback-Runde: eine Luftspalte zwischen Inhalt und der Linie dazu  │ Cld
//         │      │ (Andreas: "wirkt jetzt doch gequetscht") -- content_width jetzt width-2      │
// 26-08-17│ 1.50 │ Neuer Parameter line_fg -- die rechte Linie/Bildlaufleiste bekommt jetzt    │ Cld
//         │      │ eine EIGENE Farbe statt die normale Text-fg zu erben (Andreas: "die Striche  │
//         │      │ links und rechts am Hauptfenster sind unterschiedlich")                     │
// 26-08-18│ 1.60 │ Erweiterbare Eintraege: q9_listview_item_rows()/_scroll_ex()/_move_ex()/     │ Cld
//         │      │ _render_ex() dazu (s. q9_listview.h) -- bestehende Funktionen unveraendert    │
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
                         int sel_bg_r, int sel_bg_g, int sel_bg_b,
                         int line_fg_r, int line_fg_g, int line_fg_b)
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
       Hauptfenster als auch im Datei-Dialog). PLUS eine Luftspalte davor (fuenfte Runde, selber
       Tag: "wirkt jetzt doch gequetscht... bitte zum Strich jeweils ein Leerzeichen") -- content
       reicht also bis width-2, nicht bis width-1. Die Linie selbst bleibt an derselben Stelle
       (line_col haengt NICHT von content_width ab) -- nur der Inhalt bekommt mehr Luft davor. */
    content_width = lv->width - 2;
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
       zusaetzlich darauf gelegt. EIGENE Farbe (line_fg statt fg) -- Andreas' Feedback, 2026-08-17:
       "die Striche links und rechts am Hauptfenster sind unterschiedlich... das oberste rechts ist
       noch mal anders". Ursache: diese Linie wurde bisher in der normalen Text-Vordergrundfarbe
       (fg) gezeichnet, waehrend q9_widgets.c's draw_frame() den Rahmen (inkl. der Eckzeichen UND
       der Randspalten AUSSERHALB des Listenbereichs) in einer eigenen Rahmenfarbe zeichnet -- im
       Hauptfenster sind das zwei VERSCHIEDENE Farben (fg=Listentext, fg des Rahmens=PAL_FRAME), die
       zufaellig auf derselben Spalte aufeinandertreffen. Mit einem eigenen line_fg-Parameter kann
       der Aufrufer beide Linien in DERSELBEN Farbe zeichnen (s. integration_demo.c). */
    {
        char track_str[2];
        track_str[0] = Q9_GLYPH_VLINE; track_str[1] = '\0';
        for (i = 0; i < lv->height; i++) {
            q9_screenbuf_puts(sb, lv->row + i, line_col, track_str, line_fg_r, line_fg_g, line_fg_b);
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

int q9_listview_item_rows(const q9_listview_item_t *items, const int *expanded, int index)
{
    int dc;

    if (!items)                       { return 1; }
    if (!expanded || !expanded[index]) { return 1; }
    dc = items[index].detail_count;
    if (dc <= 0) { return 1; }
    return 1 /* Kopfzeile */ + dc /* Detailzeilen */ + 1 /* Trennlinie */;
}

int q9_listview_scroll_ex(int selected, int offset, int height,
                           const q9_listview_item_t *items, const int *expanded, int item_count)
{
    int sum;
    int i;

    if (item_count <= 0 || height <= 0) { return 0; }
    if (selected < 0)              { selected = 0; }
    if (selected > item_count - 1) { selected = item_count - 1; }
    if (offset < 0)                { offset = 0; }
    if (offset > item_count - 1)   { offset = item_count - 1; }

    if (selected < offset) {
        return selected;                                  /* nach oben rausgelaufen -- nachziehen,
                                                                wie beim einfachen q9_listview_scroll() */
    }

    /* Zeilen von offset bis einschliesslich selected aufsummieren, dann offset so lange nach vorn
       schieben (Eintraege von oben "abschneiden"), bis die Summe wieder ins Fenster passt ODER
       offset==selected erreicht ist (dann bleibt zumindest die Kopfzeile von selected sichtbar,
       s. Kopfkommentar zur bekannten Vereinfachung bei ueberlangen Eintraegen). */
    sum = 0;
    for (i = offset; i <= selected; i++) {
        sum += q9_listview_item_rows(items, expanded, i);
    }
    while (sum > height && offset < selected) {
        sum -= q9_listview_item_rows(items, expanded, offset);
        offset++;
    }
    return offset;
}

void q9_listview_move_ex(q9_listview_t *lv, int delta,
                          const q9_listview_item_t *items, const int *expanded)
{
    if (!lv || lv->item_count <= 0) { return; }
    lv->selected += delta;
    if (lv->selected < 0)                  { lv->selected = 0; }
    if (lv->selected > lv->item_count - 1) { lv->selected = lv->item_count - 1; }
    lv->scroll_offset = q9_listview_scroll_ex(lv->selected, lv->scroll_offset, lv->height,
                                               items, expanded, lv->item_count);
}

void q9_listview_render_ex(const q9_listview_t *lv, q9_screenbuf_t *sb,
                            const q9_listview_item_t *items, const int *expanded,
                            int fg_r, int fg_g, int fg_b,
                            int sel_fg_r, int sel_fg_g, int sel_fg_b,
                            int sel_bg_r, int sel_bg_g, int sel_bg_b,
                            int line_fg_r, int line_fg_g, int line_fg_b,
                            int detail_fg_r, int detail_fg_g, int detail_fg_b)
{
    int content_width;
    int line_col;
    int idx;
    int row_cursor;
    int total_rows;
    int i;

    if (!lv || !sb || !items) { return; }

    content_width = lv->width - 2;
    if (content_width < 1) { content_width = 1; }
    line_col = lv->col + lv->width - 1;

    row_cursor = 0;
    for (idx = lv->scroll_offset; idx < lv->item_count && row_cursor < lv->height; idx++) {
        int screen_row = lv->row + row_cursor;
        int has_details = items[idx].detail_count > 0;
        int is_expanded = has_details && expanded && expanded[idx];
        int is_selected = (idx == lv->selected);
        int use_fg_r = is_selected ? sel_fg_r : fg_r;
        int use_fg_g = is_selected ? sel_fg_g : fg_g;
        int use_fg_b = is_selected ? sel_fg_b : fg_b;
        char marker[2];

        /* Kopfzeile: Pfeil-Symbol (auf-/zugeklappt, s. q9_listview.h) + Luftspalte + Name --
           zwei separate puts() statt einem zusammengesetzten String (spart eine feste Puffergroesse
           fuer beliebig lange Namen, s. q9_screenbuf_puts()-Klemmung an der Puffergrenze). Eintraege
           ohne Detailzeilen bekommen KEIN Pfeil-Symbol (nichts zum Auf-/Zuklappen) -- ein Leerzeichen
           an derselben Stelle, damit der Name trotzdem an derselben Spalte wie bei erweiterbaren
           Eintraegen beginnt. */
        marker[0] = has_details ? (is_expanded ? (char)Q9_GLYPH_DOWN_ARROW : (char)Q9_GLYPH_RIGHT_ARROW)
                                 : ' ';
        marker[1] = '\0';
        if (is_selected) {
            q9_screenbuf_fill_rect(sb, screen_row, lv->col, 1, content_width, ' ',
                                    sel_fg_r, sel_fg_g, sel_fg_b, 1, sel_bg_r, sel_bg_g, sel_bg_b);
        }
        q9_screenbuf_puts(sb, screen_row, lv->col, marker, use_fg_r, use_fg_g, use_fg_b);
        q9_screenbuf_puts(sb, screen_row, lv->col + 2, items[idx].name ? items[idx].name : "",
                           use_fg_r, use_fg_g, use_fg_b);
        row_cursor++;

        if (is_expanded) {
            int j;
            /* Detailzeilen -- um zwei Spalten eingerueckt (Pfeil + Luftspalte, dieselbe Einrueckung
               wie der Name in der Kopfzeile), EIGENE Farbe (detail_fg) statt der normalen Text-fg --
               optische Unterscheidung Kopf/Detail (Andreas' gewaehlter Stil, s. q9_listview.h). */
            for (j = 0; j < items[idx].detail_count && row_cursor < lv->height; j++) {
                q9_screenbuf_puts(sb, lv->row + row_cursor, lv->col + 2, items[idx].detail_lines[j],
                                   detail_fg_r, detail_fg_g, detail_fg_b);
                row_cursor++;
            }
            /* Trennlinie danach -- volle content_width, in line_fg (dieselbe Rolle wie die rechte
               Rahmenlinie: strukturell, nicht Text). */
            if (row_cursor < lv->height) {
                char hl[2];
                int k;
                hl[0] = (char)Q9_GLYPH_HLINE; hl[1] = '\0';
                for (k = 0; k < content_width; k++) {
                    q9_screenbuf_puts(sb, lv->row + row_cursor, lv->col + k, hl,
                                       line_fg_r, line_fg_g, line_fg_b);
                }
                row_cursor++;
            }
        }
    }

    /* Rechte Spalte -- wie q9_listview_render(), aber ROW-basiert statt item-basiert (s.
       q9_listview.h): total_rows zaehlt alle Bildschirmzeilen ueber ALLE Eintraege (inkl.
       aufgeklappter), nicht mehr nur item_count. */
    {
        char track_str[2];
        track_str[0] = (char)Q9_GLYPH_VLINE; track_str[1] = '\0';
        for (i = 0; i < lv->height; i++) {
            q9_screenbuf_puts(sb, lv->row + i, line_col, track_str, line_fg_r, line_fg_g, line_fg_b);
        }
    }

    total_rows = 0;
    for (i = 0; i < lv->item_count; i++) {
        total_rows += q9_listview_item_rows(items, expanded, i);
    }

    if (total_rows > lv->height) {
        int offset_rows;
        int max_offset_rows;
        int thumb_height, thumb_start, max_thumb_start;
        char thumb_str[2];
        thumb_str[0] = (char)Q9_GLYPH_BLOCK; thumb_str[1] = '\0';

        offset_rows = 0;
        for (i = 0; i < lv->scroll_offset && i < lv->item_count; i++) {
            offset_rows += q9_listview_item_rows(items, expanded, i);
        }
        max_offset_rows = total_rows - lv->height;
        if (max_offset_rows < 1) { max_offset_rows = 1; }

        thumb_height = (lv->height * lv->height) / total_rows;
        if (thumb_height < 1)              { thumb_height = 1; }
        if (thumb_height > lv->height - 1) { thumb_height = lv->height - 1; }
        if (thumb_height < 1)              { thumb_height = 1; }   /* height==1: height-1==0-Randfall */

        max_thumb_start = lv->height - thumb_height;
        if (max_thumb_start < 1) { max_thumb_start = 1; }
        thumb_start = (offset_rows * max_thumb_start) / max_offset_rows;
        if (thumb_start > lv->height - thumb_height) { thumb_start = lv->height - thumb_height; }
        if (thumb_start < 0)                          { thumb_start = 0; }

        for (i = 0; i < thumb_height; i++) {
            q9_screenbuf_puts(sb, lv->row + thumb_start + i, line_col, thumb_str,
                               sel_bg_r, sel_bg_g, sel_bg_b);
        }
    }
}

//────────────────────────────────────────────────────────────────────────────────────────────────
// EOF q9_listview.c                                                                       Ver. 1.60
//────────────────────────────────────────────────────────────────────────────────────────────────
