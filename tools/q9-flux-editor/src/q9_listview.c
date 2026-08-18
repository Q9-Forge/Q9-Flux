//════════════════════════════════════════════════════════════════════════════════════════════════
// File:   q9_listview.c                                                                   Ver. 2.30
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
// 26-08-18│ 1.70 │ Neuer Parameter exp_bg an render_ex() -- Kopfzeile eines aufgeklappten, nicht  │ Cld
//         │      │ ausgewaehlten Eintrags bekommt einen eigenen Hintergrund (Andreas: "die        │
//         │      │ Headerzeile geht ein wenig unter")                                             │
// 26-08-18│ 1.80 │ NACHTRAG (Versionsbump beim Original-Commit vergessen): Feld-Navigation --      │ Cld
//         │      │ field_enter()/_leave()/_escape()/_move()/_putc()/_backspace() dazu (Andreas:    │
//         │      │ "wie komme ich in das item rein um dort Werte zu aendern?")                     │
// 26-08-18│ 1.90 │ Neuer q9_listview_field_kind_t (TEXT/BUTTON) -- field_putc()/_backspace()       │ Cld
//         │      │ ignorieren BUTTON-Felder (Andreas: "dahinter ein Button um den Dialog zu         │
//         │      │ oeffnen")                                                                        │
// 26-08-18│ 2.00 │ TEXT+BUTTON-Paar jetzt Sonderfall: 3 Zeilen statt 2, echter dreizeiliger Button  │ Cld
//         │      │ wie im Datei-Dialog (Halbblock-Kappen), Wert-Box (box_fg/bg NEU) statt Fliesstext │
//         │      │ (Andreas: "wie im Dialog... zentrisch hinter Datei ausgerichtet")                │
// 26-08-18│ 2.10 │ Q9_LISTVIEW_VALUE_BOX_WIDTH 20 -> 35 Zeichen -- dasselbe Mass wie das Namens-      │ Cld
//         │      │ Kaestchen im Datei-Dialog (Andreas: "Im Dialog sind es ca. 35 Zeichen")           │
// 26-08-18│ 2.20 │ Numerische Feldtypen NUMERIC_DEC/_HEX -- field_putc() filtert die Zeichenklasse,  │ Cld
//         │      │ render_ex() zeigt automatisch "$" vor Hex-Werten (Andreas: "Numerische Eingabe    │
//         │      │ Dezimal/Hex opt. mit Bereich")                                                    │
// 26-08-18│ 2.30 │ Boolean-Feldtyp -- field_toggle() NEU (schaltet "ja"/"nein" um, value_equals()/    │ Cld
//         │      │ value_assign() als Hilfsfunktionen ohne <string.h>), putc/backspace ignorieren     │
//         │      │ BOOLEAN-Felder jetzt zusaetzlich zu BUTTON (Andreas: "Boolean Eingabe")            │
//═════════╧══════╧════════════════════════════════════════════════════════════════════════╧══════
#include "q9_listview.h"

/* Spalte, ab der der WERT eines Feldes beginnt (relativ zur Feld-Startspalte col+2, s.
   q9_listview_render_ex()) -- feste Ausrichtung, damit alle Werte untereinander in einer Spalte
   stehen, unabhaengig von der Laenge des jeweiligen Labels (formularaehnlicher Look). */
#define Q9_LISTVIEW_FIELD_VALUE_COL 16

/* Nur fuer den TEXT+BUTTON-Sonderfall (s. q9_listview.h): feste Breite der Wert-Box, Luftspalte
   zum Button, und Breite des Buttons selbst (dieselbe Groessenordnung wie OK/Abbrechen im
   Datei-Dialog -- "Datei" zentriert darin). 35 Zeichen -- Andreas' Nachfrage, 2026-08-18,
   zweiundzwanzigste Runde: "Im Dialog sind es ca. 35 Zeichen, sollen wir das hier auch nehmen?" --
   dasselbe Mass wie das Namens-Kaestchen im Datei-Dialog (dort dynamisch aus der Dialogbreite
   berechnet, hier als fester Wert uebernommen, da die Listenansicht keine eigene Breitenrechnung
   dafuer hat). */
#define Q9_LISTVIEW_VALUE_BOX_WIDTH 35
#define Q9_LISTVIEW_BUTTON_GAP       3
#define Q9_LISTVIEW_BUTTON_WIDTH     8

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
    lv->field_focus   = -1;                                 /* Fokus auf der Kopfzeile, kein Feld,
                                                                  s. q9_listview_field_enter() */
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

/* TEXT-Feld field[j], DIREKT gefolgt von einem BUTTON-Feld -- der Sonderfall aus q9_listview.h
   (dreizeiliger Button, vertikal zentriert neben dem Textfeld statt darunter). Von
   q9_listview_item_rows() UND q9_listview_render_ex() genutzt, damit beide exakt dieselbe
   Definition verwenden (sonst koennten Zeilenzahl und tatsaechliches Rendering auseinanderlaufen). */
static int is_text_button_pair(const q9_listview_item_t *items, int index, int j)
{
    int fc = items[index].field_count;
    if (j + 1 >= fc) { return 0; }
    return items[index].fields[j].kind == Q9_LISTVIEW_FIELD_TEXT
        && items[index].fields[j + 1].kind == Q9_LISTVIEW_FIELD_BUTTON;
}

int q9_listview_item_rows(const q9_listview_item_t *items, const int *expanded, int index)
{
    int fc, j, rows;

    if (!items)                       { return 1; }
    if (!expanded || !expanded[index]) { return 1; }
    fc = items[index].field_count;
    if (fc <= 0) { return 1; }

    rows = 1;                                               /* Kopfzeile */
    for (j = 0; j < fc; j++) {
        if (is_text_button_pair(items, index, j)) {
            rows += 3;                                      /* Kappe oben + gemeinsame Zeile + Kappe unten */
            j++;                                             /* das BUTTON-Feld ist schon mitgezaehlt */
        } else {
            rows += 1;
        }
    }
    return rows + 1;                                        /* Trennlinie danach */
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
    lv->field_focus = -1;                                   /* defensiv: Feld-Fokus ergibt bei einem
                                                                  ANDEREN Eintrag keinen Sinn, s.
                                                                  q9_listview.h */
}

void q9_listview_field_enter(q9_listview_t *lv, int *expanded, const q9_listview_item_t *items)
{
    int sel;
    if (!lv || !items || lv->selected < 0) { return; }
    sel = lv->selected;
    if (items[sel].field_count <= 0) { return; }            /* nichts zum Betreten */
    if (expanded) { expanded[sel] = 1; }
    lv->field_focus = 0;
}

void q9_listview_field_leave(q9_listview_t *lv)
{
    if (!lv) { return; }
    lv->field_focus = -1;
}

void q9_listview_field_escape(q9_listview_t *lv, int *expanded)
{
    if (!lv) { return; }
    lv->field_focus = -1;
    if (expanded && lv->selected >= 0) { expanded[lv->selected] = 0; }
}

void q9_listview_field_move(q9_listview_t *lv, int delta, const q9_listview_item_t *items)
{
    int count;
    if (!lv || !items || lv->selected < 0 || lv->field_focus < 0) { return; }
    count = items[lv->selected].field_count;
    if (count <= 0) { return; }
    lv->field_focus += delta;
    if (lv->field_focus < 0)         { lv->field_focus = 0; }
    if (lv->field_focus > count - 1) { lv->field_focus = count - 1; }
}

/* Zeichenklasse fuer NUMERIC_DEC/_HEX (s. q9_listview_field_putc()) -- reine Filterfunktion, kein
   Bereich (das bleibt Sache des Aufrufers, s. q9_listview_field_kind_t-Kommentar). */
static int is_allowed_numeric_char(q9_listview_field_kind_t kind, char ch)
{
    if (kind == Q9_LISTVIEW_FIELD_NUMERIC_DEC) {
        return ch >= '0' && ch <= '9';
    }
    if (kind == Q9_LISTVIEW_FIELD_NUMERIC_HEX) {
        return (ch >= '0' && ch <= '9') || (ch >= 'a' && ch <= 'f') || (ch >= 'A' && ch <= 'F');
    }
    return 1;                                               /* TEXT -- alles erlaubt */
}

void q9_listview_field_putc(q9_listview_t *lv, const q9_listview_item_t *items, char ch)
{
    q9_listview_field_t *f;
    int len;
    if (!lv || !items || lv->selected < 0 || lv->field_focus < 0) { return; }
    if (lv->field_focus >= items[lv->selected].field_count) { return; }
    f = &items[lv->selected].fields[lv->field_focus];
    if (f->kind == Q9_LISTVIEW_FIELD_BUTTON || f->kind == Q9_LISTVIEW_FIELD_BOOLEAN) {
        return;                                               /* nicht antippbar -- BOOLEAN wird
                                                                   ueber field_toggle() umgeschaltet,
                                                                   s. q9_listview_item_t */
    }
    if (!is_allowed_numeric_char(f->kind, ch)) { return; }   /* falsche Zeichenklasse -- verwerfen */
    len = 0;
    while (len < Q9_LISTVIEW_FIELD_VALUE_MAX - 1 && f->value[len] != '\0') { len++; }
    if (len >= Q9_LISTVIEW_FIELD_VALUE_MAX - 1) { return; }  /* voll -- kein Ueberlauf */
    f->value[len]     = ch;
    f->value[len + 1] = '\0';
}

void q9_listview_field_backspace(q9_listview_t *lv, const q9_listview_item_t *items)
{
    q9_listview_field_t *f;
    int len;
    if (!lv || !items || lv->selected < 0 || lv->field_focus < 0) { return; }
    if (lv->field_focus >= items[lv->selected].field_count) { return; }
    f = &items[lv->selected].fields[lv->field_focus];
    if (f->kind == Q9_LISTVIEW_FIELD_BUTTON || f->kind == Q9_LISTVIEW_FIELD_BOOLEAN) { return; }
    len = 0;
    while (len < Q9_LISTVIEW_FIELD_VALUE_MAX - 1 && f->value[len] != '\0') { len++; }
    if (len > 0) { f->value[len - 1] = '\0'; }
}

/* Vergleicht value mit dem Literal s (bis zum NUL bei s) -- reine Hilfsfunktion, ersetzt strcmp()
   ohne <string.h> dazuzunehmen (Datei kommt bisher bewusst ganz ohne aus). */
static int value_equals(const char *value, const char *s)
{
    int i;
    for (i = 0; s[i] != '\0'; i++) {
        if (value[i] != s[i]) { return 0; }
    }
    return value[i] == '\0';
}

/* Kopiert das Literal s (bis zum NUL bei s) in value -- s ist IMMER "ja" oder "nein", passt also
   sicher in Q9_LISTVIEW_FIELD_VALUE_MAX, kein Laengencheck noetig. */
static void value_assign(char *value, const char *s)
{
    int i;
    for (i = 0; s[i] != '\0'; i++) { value[i] = s[i]; }
    value[i] = '\0';
}

void q9_listview_field_toggle(q9_listview_t *lv, const q9_listview_item_t *items)
{
    q9_listview_field_t *f;
    if (!lv || !items || lv->selected < 0 || lv->field_focus < 0) { return; }
    if (lv->field_focus >= items[lv->selected].field_count) { return; }
    f = &items[lv->selected].fields[lv->field_focus];
    if (f->kind != Q9_LISTVIEW_FIELD_BOOLEAN) { return; }
    if (value_equals(f->value, "ja")) {
        value_assign(f->value, "nein");
    } else {
        value_assign(f->value, "ja");                         /* "nein" ODER unerwarteter Wert
                                                                   -- beides wird "ja" */
    }
}

void q9_listview_render_ex(const q9_listview_t *lv, q9_screenbuf_t *sb,
                            const q9_listview_item_t *items, const int *expanded,
                            int fg_r, int fg_g, int fg_b,
                            int sel_fg_r, int sel_fg_g, int sel_fg_b,
                            int sel_bg_r, int sel_bg_g, int sel_bg_b,
                            int line_fg_r, int line_fg_g, int line_fg_b,
                            int detail_fg_r, int detail_fg_g, int detail_fg_b,
                            int exp_bg_r, int exp_bg_g, int exp_bg_b,
                            int box_fg_r, int box_fg_g, int box_fg_b,
                            int box_bg_r, int box_bg_g, int box_bg_b)
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
        int has_details = items[idx].field_count > 0;
        int is_expanded = has_details && expanded && expanded[idx];
        int is_selected = (idx == lv->selected);
        /* header_is_focused statt rohem is_selected: die Kopfzeile zeigt die STARKE sel_bg-
           Hervorhebung nur, wenn der Fokus tatsaechlich AUF ihr liegt (field_focus==-1). Ist der
           Fokus in ein Feld gewandert (field_focus>=0, s. q9_listview_field_enter()), faellt die
           Kopfzeile auf die schwaechere exp_bg zurueck -- die Feldzeile weiter unten uebernimmt die
           sel_bg-Rolle (s.u.). Zu jedem Zeitpunkt genau EINE Zeile in sel_bg. */
        int header_is_focused = is_selected && lv->field_focus < 0;
        int use_fg_r = header_is_focused ? sel_fg_r : fg_r;
        int use_fg_g = header_is_focused ? sel_fg_g : fg_g;
        int use_fg_b = header_is_focused ? sel_fg_b : fg_b;
        char marker[2];

        /* Kopfzeile: Pfeil-Symbol (auf-/zugeklappt, s. q9_listview.h) + Luftspalte + Name --
           zwei separate puts() statt einem zusammengesetzten String (spart eine feste Puffergroesse
           fuer beliebig lange Namen, s. q9_screenbuf_puts()-Klemmung an der Puffergrenze). Eintraege
           ohne Felder bekommen KEIN Pfeil-Symbol (nichts zum Auf-/Zuklappen) -- ein Leerzeichen
           an derselben Stelle, damit der Name trotzdem an derselben Spalte wie bei erweiterbaren
           Eintraegen beginnt. */
        marker[0] = has_details ? (is_expanded ? (char)Q9_GLYPH_DOWN_ARROW : (char)Q9_GLYPH_RIGHT_ARROW)
                                 : ' ';
        marker[1] = '\0';
        if (header_is_focused) {
            q9_screenbuf_fill_rect(sb, screen_row, lv->col, 1, content_width, ' ',
                                    sel_fg_r, sel_fg_g, sel_fg_b, 1, sel_bg_r, sel_bg_g, sel_bg_b);
        } else if (is_expanded) {
            /* Aufgeklappt, aber der Fokus liegt NICHT auf der Kopfzeile (entweder ein anderer
               Eintrag ist ausgewaehlt, oder der Fokus ist in ein Feld dieses Eintrags gewandert) --
               eigener, gedaempfter Hintergrund (exp_bg), damit sie sich von den (noch gedaempfteren)
               Feldzeilen darunter UND von normalen, zugeklappten Eintraegen abhebt (Andreas'
               Feedback, 2026-08-18: "die Headerzeile geht ein wenig unter"). */
            q9_screenbuf_fill_rect(sb, screen_row, lv->col, 1, content_width, ' ',
                                    use_fg_r, use_fg_g, use_fg_b, 1, exp_bg_r, exp_bg_g, exp_bg_b);
        }
        q9_screenbuf_puts(sb, screen_row, lv->col, marker, use_fg_r, use_fg_g, use_fg_b);
        q9_screenbuf_puts(sb, screen_row, lv->col + 2, items[idx].name ? items[idx].name : "",
                           use_fg_r, use_fg_g, use_fg_b);
        row_cursor++;

        if (is_expanded) {
            int j;
            /* Felder -- um zwei Spalten eingerueckt (Pfeil + Luftspalte, dieselbe Einrueckung wie
               der Name in der Kopfzeile), Label + Wert in einer festen Spalte nebeneinander. Normal
               in EIGENER, gedaempfter Farbe (detail_fg) statt der normalen Text-fg -- optische
               Unterscheidung Kopf/Feld (Andreas' gewaehlter Stil, s. q9_listview.h). Das FOKUSSIERTE
               Feld (field_focus==j, NUR beim ausgewaehlten Eintrag moeglich) bekommt stattdessen
               sel_fg/sel_bg -- deutlich als "hier tippst du gerade" erkennbar (Andreas' Wunsch,
               2026-08-18: Feld-Navigation/-Bearbeitung). TEXT+BUTTON-Paar: s. q9_listview.h fuer
               den Sonderfall (dreizeiliger Button wie im Datei-Dialog). */
            for (j = 0; j < items[idx].field_count && row_cursor < lv->height; j++) {
                if (is_text_button_pair(items, idx, j)) {
                    int cap_above_row = lv->row + row_cursor;
                    int mid_row       = lv->row + row_cursor + 1;
                    int cap_below_row = lv->row + row_cursor + 2;
                    int is_text_focused   = is_selected && lv->field_focus == j;
                    int is_button_focused = is_selected && lv->field_focus == j + 1;
                    int btn_fg_r = is_button_focused ? sel_fg_r : box_fg_r;
                    int btn_fg_g = is_button_focused ? sel_fg_g : box_fg_g;
                    int btn_fg_b = is_button_focused ? sel_fg_b : box_fg_b;
                    int btn_bg_r = is_button_focused ? sel_bg_r : box_bg_r;
                    int btn_bg_g = is_button_focused ? sel_bg_g : box_bg_g;
                    int btn_bg_b = is_button_focused ? sel_bg_b : box_bg_b;
                    int button_col = lv->col + 2 + Q9_LISTVIEW_FIELD_VALUE_COL
                                      + Q9_LISTVIEW_VALUE_BOX_WIDTH + Q9_LISTVIEW_BUTTON_GAP;

                    if (row_cursor + 2 >= lv->height) { break; }  /* die 3 Zeilen passen nicht mehr
                                                                       komplett -- lieber ganz weglassen
                                                                       als abgeschnitten darstellen */

                    /* Label + Wert-Box (TEXT-Feld) -- fokussiert: normale sel_fg/bg-Zeile wie jedes
                       andere Feld. Unfokussiert: feste, sichtbare Box (box_fg/bg) statt reinem
                       Fliesstext, damit die Laenge/Ausdehnung des Werts erkennbar bleibt (Andreas'
                       Wunsch: "einen anderen Farbton, so dass man erkennen kann wie lang es ist"). */
                    if (is_text_focused) {
                        q9_screenbuf_fill_rect(sb, mid_row, lv->col, 1, content_width, ' ',
                                                sel_fg_r, sel_fg_g, sel_fg_b, 1,
                                                sel_bg_r, sel_bg_g, sel_bg_b);
                        q9_screenbuf_puts(sb, mid_row, lv->col + 2, items[idx].fields[j].label,
                                           sel_fg_r, sel_fg_g, sel_fg_b);
                        q9_screenbuf_puts(sb, mid_row, lv->col + 2 + Q9_LISTVIEW_FIELD_VALUE_COL,
                                           items[idx].fields[j].value, sel_fg_r, sel_fg_g, sel_fg_b);
                    } else {
                        q9_screenbuf_puts(sb, mid_row, lv->col + 2, items[idx].fields[j].label,
                                           detail_fg_r, detail_fg_g, detail_fg_b);
                        q9_screenbuf_fill_rect(sb, mid_row, lv->col + 2 + Q9_LISTVIEW_FIELD_VALUE_COL,
                                                1, Q9_LISTVIEW_VALUE_BOX_WIDTH, ' ',
                                                box_fg_r, box_fg_g, box_fg_b, 1,
                                                box_bg_r, box_bg_g, box_bg_b);
                        q9_screenbuf_puts(sb, mid_row, lv->col + 2 + Q9_LISTVIEW_FIELD_VALUE_COL,
                                           items[idx].fields[j].value, box_fg_r, box_fg_g, box_fg_b);
                    }

                    /* Button -- Text in eigener Farbflaeche, Kappen darueber/darunter, genau wie
                       draw_button() in q9_filedialog.c (Q9_GLYPH_LOWER_HALF/UPPER_HALF). Kappen
                       OHNE eigenen Hintergrund (use_bg=0) -- die "gefuellte" Haelfte des Zeichens
                       zeigt dadurch die Buttonfarbe auf dem normalen Zeilenhintergrund. */
                    {
                        char text[Q9_LISTVIEW_BUTTON_WIDTH + 1];
                        int len = 0, left, k;
                        while (items[idx].fields[j + 1].value[len] != '\0'
                               && len < Q9_LISTVIEW_BUTTON_WIDTH) { len++; }
                        left = (Q9_LISTVIEW_BUTTON_WIDTH - len) / 2;
                        if (left < 0) { left = 0; }
                        for (k = 0; k < Q9_LISTVIEW_BUTTON_WIDTH; k++) { text[k] = ' '; }
                        for (k = 0; k < len && left + k < Q9_LISTVIEW_BUTTON_WIDTH; k++) {
                            text[left + k] = items[idx].fields[j + 1].value[k];
                        }
                        text[Q9_LISTVIEW_BUTTON_WIDTH] = '\0';

                        q9_screenbuf_fill_rect(sb, mid_row, button_col, 1, Q9_LISTVIEW_BUTTON_WIDTH,
                                                ' ', btn_fg_r, btn_fg_g, btn_fg_b, 1,
                                                btn_bg_r, btn_bg_g, btn_bg_b);
                        q9_screenbuf_puts(sb, mid_row, button_col, text, btn_fg_r, btn_fg_g, btn_fg_b);
                    }
                    q9_screenbuf_fill_rect(sb, cap_above_row, button_col, 1, Q9_LISTVIEW_BUTTON_WIDTH,
                                            (char)Q9_GLYPH_LOWER_HALF, btn_bg_r, btn_bg_g, btn_bg_b,
                                            0, 0, 0, 0);
                    q9_screenbuf_fill_rect(sb, cap_below_row, button_col, 1, Q9_LISTVIEW_BUTTON_WIDTH,
                                            (char)Q9_GLYPH_UPPER_HALF, btn_bg_r, btn_bg_g, btn_bg_b,
                                            0, 0, 0, 0);

                    row_cursor += 3;
                    j++;                                     /* BUTTON-Feld ist mit erledigt */
                    continue;
                }

                {
                    int field_row = lv->row + row_cursor;
                    int is_field_focused = is_selected && lv->field_focus == j;
                    int fld_fg_r = is_field_focused ? sel_fg_r : detail_fg_r;
                    int fld_fg_g = is_field_focused ? sel_fg_g : detail_fg_g;
                    int fld_fg_b = is_field_focused ? sel_fg_b : detail_fg_b;

                    if (is_field_focused) {
                        q9_screenbuf_fill_rect(sb, field_row, lv->col, 1, content_width, ' ',
                                                sel_fg_r, sel_fg_g, sel_fg_b, 1,
                                                sel_bg_r, sel_bg_g, sel_bg_b);
                    }
                    q9_screenbuf_puts(sb, field_row, lv->col + 2, items[idx].fields[j].label,
                                       fld_fg_r, fld_fg_g, fld_fg_b);
                    /* NUMERIC_HEX: "$" automatisch vor den Wert (nicht Teil von value selbst,
                       s. q9_listview_field_kind_t) -- Wert dadurch um eine Spalte verschoben. */
                    if (items[idx].fields[j].kind == Q9_LISTVIEW_FIELD_NUMERIC_HEX) {
                        q9_screenbuf_puts(sb, field_row, lv->col + 2 + Q9_LISTVIEW_FIELD_VALUE_COL,
                                           "$", fld_fg_r, fld_fg_g, fld_fg_b);
                        q9_screenbuf_puts(sb, field_row, lv->col + 3 + Q9_LISTVIEW_FIELD_VALUE_COL,
                                           items[idx].fields[j].value, fld_fg_r, fld_fg_g, fld_fg_b);
                    } else {
                        q9_screenbuf_puts(sb, field_row, lv->col + 2 + Q9_LISTVIEW_FIELD_VALUE_COL,
                                           items[idx].fields[j].value, fld_fg_r, fld_fg_g, fld_fg_b);
                    }
                    row_cursor++;
                }
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
// EOF q9_listview.c                                                                       Ver. 2.30
//────────────────────────────────────────────────────────────────────────────────────────────────
