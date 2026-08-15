//════════════════════════════════════════════════════════════════════════════════════════════════
// File:   q9_widgets.c                                                                    Ver. 1.00
// Owner:  Claudia
// Desc.:  Implementierung, siehe q9_widgets.h.
//
// Edition History
//─────────┬──────┬────────────────────────────────────────────────────────────────────────┬──────
// Date    │ Ver. │ Description                                                            │ By
//─────────┼──────┼────────────────────────────────────────────────────────────────────────┬──────
// 26-08-16│ 1.00 │ Erster Wurf                                                              │ Cld
//═════════╧══════╧════════════════════════════════════════════════════════════════════════╧══════
#include "q9_widgets.h"
#include <stdio.h>

void q9_screenbuf_draw_frame(q9_screenbuf_t *sb, int row, int col, int rows, int cols,
                              const char *title, int fg_r, int fg_g, int fg_b)
{
    if (!sb || rows < 2 || cols < 2) { return; }

    /* Kanten -- Ecken werden gleich danach ueberschrieben, deshalb ist die Reihenfolge hier egal.
       fill_rect uebernimmt die Bounds-Pruefung (Rahmen teilweise ausserhalb des Puffers -> die
       betroffenen Zellen werden dort einfach uebersprungen, kein Sonderfall noetig). */
    q9_screenbuf_fill_rect(sb, row,        col, 1,    cols, '-', fg_r, fg_g, fg_b, 0, 0, 0, 0);
    q9_screenbuf_fill_rect(sb, row+rows-1, col, 1,    cols, '-', fg_r, fg_g, fg_b, 0, 0, 0, 0);
    q9_screenbuf_fill_rect(sb, row, col,        rows, 1,    '|', fg_r, fg_g, fg_b, 0, 0, 0, 0);
    q9_screenbuf_fill_rect(sb, row, col+cols-1, rows, 1,    '|', fg_r, fg_g, fg_b, 0, 0, 0, 0);

    q9_screenbuf_fill_rect(sb, row,        col,        1, 1, '+', fg_r, fg_g, fg_b, 0, 0, 0, 0);
    q9_screenbuf_fill_rect(sb, row,        col+cols-1, 1, 1, '+', fg_r, fg_g, fg_b, 0, 0, 0, 0);
    q9_screenbuf_fill_rect(sb, row+rows-1, col,        1, 1, '+', fg_r, fg_g, fg_b, 0, 0, 0, 0);
    q9_screenbuf_fill_rect(sb, row+rows-1, col+cols-1, 1, 1, '+', fg_r, fg_g, fg_b, 0, 0, 0, 0);

    /* Titel mittig in der oberen Kante, mit je einem Leerzeichen als Abstand -- ueberschreibt dort
       die '-'-Zeichen. Passt der Titel (inkl. der zwei Leerzeichen) nicht zwischen die beiden Ecken,
       wird er gekuerzt (kein Umbruch/Ueberlauf ueber die Ecken hinaus). */
    if (title && title[0] && cols > 4) {
        char buf[128];
        int avail = cols - 4;                            /* Platz zwischen den Ecken, minus Rand */
        int len = 0;
        int start_col;
        while (title[len] && len < avail && len < (int)sizeof(buf) - 3) { len++; }
        if (len > 0) {
            snprintf(buf, sizeof(buf), " %.*s ", len, title);
            start_col = col + (cols - (len + 2)) / 2;
            q9_screenbuf_puts(sb, row, start_col, buf, fg_r, fg_g, fg_b);
        }
    }
}

//────────────────────────────────────────────────────────────────────────────────────────────────
// EOF q9_widgets.c                                                                        Ver. 1.00
//────────────────────────────────────────────────────────────────────────────────────────────────
