//════════════════════════════════════════════════════════════════════════════════════════════════
// File:   q9_ansi.c                                                                       Ver. 1.10
// Owner:  Claudia
// Desc.:  Implementierung, siehe q9_ansi.h.
//
// Edition History
//─────────┬──────┬────────────────────────────────────────────────────────────────────────┬──────
// Date    │ Ver. │ Description                                                            │ By
//─────────┼──────┼────────────────────────────────────────────────────────────────────────┼──────
// 26-08-14│ 1.00 │ Erster Wurf                                                              │ Cld
// 26-08-17│ 1.10 │ q9_ansi_resize_window (XTWINOPS, "ESC[8;rows;colst")                     │ Cld
//═════════╧══════╧════════════════════════════════════════════════════════════════════════╧══════
#include "q9_ansi.h"
#include <stdio.h>

static int clamp_byte(int v)
{
    if (v < 0)   return 0;
    if (v > 255) return 255;
    return v;
}

/* snprintf() liefert bei Erfolg die Anzahl Zeichen, die OHNE Kuerzung geschrieben WORDEN WAEREN --
   kann also groesser als out_max sein, wenn der Puffer zu klein war. Fuer den Rueckgabewert dieser
   Funktionen (Aufrufer haengt mehrere Sequenzen per Offset aneinander) wird deshalb auf out_max-1
   geklemmt, damit ein zu kleiner Puffer nicht zu einem Offset FUEHRT, der ausserhalb des Puffers
   liegt -- ein abgeschnittenes Ergebnis ist erkennbar (kuerzer als erwartet), aber kein UB. */
static unsigned clamp_len(int n, unsigned out_max)
{
    if (n < 0) {
        return 0;                                          /* snprintf-Fehler, sollte nie eintreten */
    }
    if ((unsigned)n >= out_max) {
        return out_max ? out_max - 1u : 0u;
    }
    return (unsigned)n;
}

unsigned q9_ansi_move(char *out, unsigned out_max, int row, int col)
{
    return clamp_len(snprintf(out, out_max, "\x1b[%d;%dH", row, col), out_max);
}

unsigned q9_ansi_fg_rgb(char *out, unsigned out_max, int r, int g, int b)
{
    return clamp_len(snprintf(out, out_max, "\x1b[38;2;%d;%d;%dm",
                               clamp_byte(r), clamp_byte(g), clamp_byte(b)), out_max);
}

unsigned q9_ansi_bg_rgb(char *out, unsigned out_max, int r, int g, int b)
{
    return clamp_len(snprintf(out, out_max, "\x1b[48;2;%d;%d;%dm",
                               clamp_byte(r), clamp_byte(g), clamp_byte(b)), out_max);
}

unsigned q9_ansi_reset(char *out, unsigned out_max)
{
    return clamp_len(snprintf(out, out_max, "\x1b[0m"), out_max);
}

unsigned q9_ansi_clear(char *out, unsigned out_max)
{
    return clamp_len(snprintf(out, out_max, "\x1b[2J"), out_max);
}

unsigned q9_ansi_hide_cursor(char *out, unsigned out_max)
{
    return clamp_len(snprintf(out, out_max, "\x1b[?25l"), out_max);
}

unsigned q9_ansi_show_cursor(char *out, unsigned out_max)
{
    return clamp_len(snprintf(out, out_max, "\x1b[?25h"), out_max);
}

unsigned q9_ansi_resize_window(char *out, unsigned out_max, int rows, int cols)
{
    return clamp_len(snprintf(out, out_max, "\x1b[8;%d;%dt", rows, cols), out_max);
}

//────────────────────────────────────────────────────────────────────────────────────────────────
// EOF q9_ansi.c                                                                           Ver. 1.10
//────────────────────────────────────────────────────────────────────────────────────────────────
