//════════════════════════════════════════════════════════════════════════════════════════════════
// File:   q9_screenbuf.c                                                                  Ver. 1.00
// Owner:  Claudia
// Desc.:  Implementierung, siehe q9_screenbuf.h.
//
// Edition History
//─────────┬──────┬────────────────────────────────────────────────────────────────────────┬──────
// Date    │ Ver. │ Description                                                            │ By
//─────────┼──────┼────────────────────────────────────────────────────────────────────────┬──────
// 26-08-16│ 1.00 │ Erster Wurf                                                              │ Cld
//═════════╧══════╧════════════════════════════════════════════════════════════════════════╧══════
#include "q9_screenbuf.h"
#include "q9_ansi.h"

static int clamp_byte(int v)
{
    if (v < 0)   { return 0;   }
    if (v > 255) { return 255; }
    return v;
}

static int clamp_int(int v, int lo, int hi)
{
    if (v < lo) { return lo; }
    if (v > hi) { return hi; }
    return v;
}

/* Verbleibender Platz in out -- unsigned-Unterlauf vermeiden, falls n bereits >= out_max ist
   (kann durch die saettigende clamp_len-Konvention in q9_ansi.c passieren, s. dortiger
   Kopfkommentar: n waechst nach einem zu kleinen Rest nicht mehr weiter). */
static unsigned rem(unsigned out_max, unsigned n)
{
    return out_max > n ? out_max - n : 0u;
}

void q9_screenbuf_init(q9_screenbuf_t *sb, int rows, int cols)
{
    int r, c;
    if (!sb) { return; }
    rows = clamp_int(rows, 0, Q9_SCREENBUF_MAX_ROWS);
    cols = clamp_int(cols, 0, Q9_SCREENBUF_MAX_COLS);
    sb->rows = rows;
    sb->cols = cols;
    for (r = 0; r < rows; r++) {
        for (c = 0; c < cols; c++) {
            sb->cell[r][c].ch     = ' ';
            sb->cell[r][c].has_fg = 0;
            sb->cell[r][c].has_bg = 0;
        }
    }
}

void q9_screenbuf_puts(q9_screenbuf_t *sb, int row, int col, const char *s,
                        int fg_r, int fg_g, int fg_b)
{
    int c;
    unsigned char r8, g8, b8;
    if (!sb || !s || row < 0 || row >= sb->rows) { return; }
    r8 = (unsigned char)clamp_byte(fg_r);
    g8 = (unsigned char)clamp_byte(fg_g);
    b8 = (unsigned char)clamp_byte(fg_b);
    if (col < 0) {
        int skip = -col;
        while (skip > 0 && *s) { s++; skip--; }
        col = 0;
    }
    for (c = col; *s && c < sb->cols; c++, s++) {
        sb->cell[row][c].ch     = *s;
        sb->cell[row][c].has_fg = 1;
        sb->cell[row][c].fg_r   = r8;
        sb->cell[row][c].fg_g   = g8;
        sb->cell[row][c].fg_b   = b8;
    }
}

void q9_screenbuf_fill_rect(q9_screenbuf_t *sb, int row, int col, int rows, int cols, char ch,
                             int fg_r, int fg_g, int fg_b,
                             int use_bg, int bg_r, int bg_g, int bg_b)
{
    int r, c;
    unsigned char fr, fg8, fb, br, bg8, bb;
    if (!sb) { return; }
    fr  = (unsigned char)clamp_byte(fg_r);
    fg8 = (unsigned char)clamp_byte(fg_g);
    fb  = (unsigned char)clamp_byte(fg_b);
    br  = (unsigned char)clamp_byte(bg_r);
    bg8 = (unsigned char)clamp_byte(bg_g);
    bb  = (unsigned char)clamp_byte(bg_b);
    for (r = row; r < row + rows; r++) {
        if (r < 0 || r >= sb->rows) { continue; }
        for (c = col; c < col + cols; c++) {
            if (c < 0 || c >= sb->cols) { continue; }
            sb->cell[r][c].ch     = ch;
            sb->cell[r][c].has_fg = 1;
            sb->cell[r][c].fg_r   = fr;
            sb->cell[r][c].fg_g   = fg8;
            sb->cell[r][c].fg_b   = fb;
            sb->cell[r][c].has_bg = use_bg ? 1 : 0;
            if (use_bg) {
                sb->cell[r][c].bg_r = br;
                sb->cell[r][c].bg_g = bg8;
                sb->cell[r][c].bg_b = bb;
            }
        }
    }
}

unsigned q9_screenbuf_render(const q9_screenbuf_t *sb, int origin_row, int origin_col,
                              char *out, unsigned out_max)
{
    unsigned n = 0;
    int r, c;
    /* "unbekannt" vor der ersten Zelle -- erzwingt bei der allerersten Zelle mit Farbe eine
       Ansage, unabhaengig davon, was auf dem echten Terminal vorher stand. */
    int have_fg = 0, have_bg = 0;
    unsigned char cur_fr = 0, cur_fg = 0, cur_fb = 0;
    unsigned char cur_br = 0, cur_bg = 0, cur_bb = 0;

    if (!sb || !out) { return 0; }

    for (r = 0; r < sb->rows; r++) {
        n += q9_ansi_move(out + n, rem(out_max, n), origin_row + r + 1, origin_col + 1);
        for (c = 0; c < sb->cols; c++) {
            const q9_screencell_t *cell = &sb->cell[r][c];
            int want_fg = cell->has_fg ? 1 : 0;
            int want_bg = cell->has_bg ? 1 : 0;
            int fg_changed = want_fg != have_fg ||
                              (want_fg && (cell->fg_r != cur_fr || cell->fg_g != cur_fg || cell->fg_b != cur_fb));
            int bg_changed = want_bg != have_bg ||
                              (want_bg && (cell->bg_r != cur_br || cell->bg_g != cur_bg || cell->bg_b != cur_bb));

            if (fg_changed || bg_changed) {
                n += q9_ansi_reset(out + n, rem(out_max, n));
                have_fg = have_bg = 0;
                if (want_fg) {
                    n += q9_ansi_fg_rgb(out + n, rem(out_max, n), cell->fg_r, cell->fg_g, cell->fg_b);
                    have_fg = 1;
                    cur_fr = cell->fg_r; cur_fg = cell->fg_g; cur_fb = cell->fg_b;
                }
                if (want_bg) {
                    n += q9_ansi_bg_rgb(out + n, rem(out_max, n), cell->bg_r, cell->bg_g, cell->bg_b);
                    have_bg = 1;
                    cur_br = cell->bg_r; cur_bg = cell->bg_g; cur_bb = cell->bg_b;
                }
            }
            if (n < out_max) {
                out[n++] = cell->ch ? cell->ch : ' ';
            }
        }
    }
    n += q9_ansi_reset(out + n, rem(out_max, n));
    return n;
}

void q9_screenbuf_snapshot(const q9_screenbuf_t *sb, int row, int col, int rows, int cols,
                            q9_screenbuf_t *snap)
{
    int r, c;
    if (!sb || !snap) { return; }
    /* Negativer Start: die entsprechend vielen Zeilen/Spalten links/oberhalb des Puffers gehoeren
       nicht dazu -- rows/cols schrumpfen um denselben Betrag, row/col ruecken auf 0. */
    if (row < 0) { rows += row; row = 0; }
    if (col < 0) { cols += col; col = 0; }
    if (row + rows > sb->rows) { rows = sb->rows - row; }
    if (col + cols > sb->cols) { cols = sb->cols - col; }
    rows = clamp_int(rows, 0, Q9_SCREENBUF_MAX_ROWS);
    cols = clamp_int(cols, 0, Q9_SCREENBUF_MAX_COLS);

    snap->rows = rows;
    snap->cols = cols;
    for (r = 0; r < rows; r++) {
        for (c = 0; c < cols; c++) {
            snap->cell[r][c] = sb->cell[row + r][col + c];
        }
    }
}

void q9_screenbuf_restore(q9_screenbuf_t *sb, int row, int col, const q9_screenbuf_t *snap)
{
    int r, c;
    if (!sb || !snap) { return; }
    for (r = 0; r < snap->rows; r++) {
        int tr = row + r;
        if (tr < 0 || tr >= sb->rows) { continue; }
        for (c = 0; c < snap->cols; c++) {
            int tc = col + c;
            if (tc < 0 || tc >= sb->cols) { continue; }
            sb->cell[tr][tc] = snap->cell[r][c];
        }
    }
}

//────────────────────────────────────────────────────────────────────────────────────────────────
// EOF q9_screenbuf.c                                                                      Ver. 1.00
//────────────────────────────────────────────────────────────────────────────────────────────────
