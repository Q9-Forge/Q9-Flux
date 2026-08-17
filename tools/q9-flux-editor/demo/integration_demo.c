//════════════════════════════════════════════════════════════════════════════════════════════════
// File:   integration_demo.c                                                             Ver. 1.00
// Owner:  Claudia
// Desc.:  Reine SICHTPRUEFUNG (kein automatisierter Test, wie ansi_selftest --demo) -- zeigt alle
//         sechs Bausteine zusammen in einem einzigen, echten Bildschirm: Rahmen (q9_widgets),
//         scrollbare Liste (q9_listview) in einem Bildschirmpuffer (q9_screenbuf), Tastatur-
//         Navigation + dynamische Groessenanpassung (q9_input). NICHT der eigentliche Q9-Flux-
//         Editor (keine echte Config-Anbindung, keine Hardware-Typen, keine Buttons) -- nur der
//         Nachweis, dass die Bausteine zusammenpassen. Fenstergroesse-Reagieren per SIGWINCH (echtes
//         Terminal wird groesser/kleiner gezogen -> Neuzeichnung ohne Tastendruck noetig).
//
// Call:   make -C tools/q9-flux-editor demo-integration
//         Pfeiltasten hoch/runter: Auswahl bewegen. Strg-C: beenden.
//
// Edition History
//─────────┬──────┬────────────────────────────────────────────────────────────────────────┬──────
// Date    │ Ver. │ Description                                                            │ By
//─────────┼──────┼────────────────────────────────────────────────────────────────────────┬──────
// 26-08-17│ 1.00 │ Erster Wurf -- Andreas wollte sich das Ganze mal ansehen                │ Cld
//═════════╧══════╧════════════════════════════════════════════════════════════════════════╧══════
#include <stdio.h>
#include <string.h>

#include "../src/q9_ansi.h"
#include "../src/q9_screenbuf.h"
#include "../src/q9_widgets.h"
#include "../src/q9_listview.h"
#include "../src/q9_input.h"

/* Rein zur Demonstration -- kein echtes Hardware-Modell, s. Kopfkommentar. */
static const char *const g_items[] = {
    "CF-Interface (onboard, c0)", "Netz-Terminal x1", "Netz-Terminal x2",
    "Netz-Terminal x3", "Netz-Terminal x4", "Netz-Terminal x5",
    "Netz-Terminal x6", "Netz-Terminal x7", "Netz-Terminal x8",
    "RTC72421 (Echtzeituhr)", "DUART 68681 (Konsole)", "QUICC-Ethernet",
    "MC6845 (GDP/CRTC)", "CLUT (Farbtabelle)", "RC2014-CF (sekundaer)",
    "Framebuffer (VRAM)", "Systemspeicher (RAM)", "ROM-Spiegel",
    "NVRAM (Akku-gepuffert)", "Timer/IRQ3-Trigger",
};
#define ITEM_COUNT (int)(sizeof(g_items) / sizeof(g_items[0]))

static void write_ansi(unsigned (*fn)(char *, unsigned))
{
    char buf[32];
    unsigned n = fn(buf, sizeof(buf));
    fwrite(buf, 1, n, stdout);
}

static unsigned wrap_hide(char *b, unsigned n)  { return q9_ansi_hide_cursor(b, n); }
static unsigned wrap_show(char *b, unsigned n)  { return q9_ansi_show_cursor(b, n); }

static void clamp_dims(int *rows, int *cols)
{
    if (*rows < 6)                       { *rows = 6; }
    if (*rows > Q9_SCREENBUF_MAX_ROWS)   { *rows = Q9_SCREENBUF_MAX_ROWS; }
    if (*cols < 20)                      { *cols = 20; }
    if (*cols > Q9_SCREENBUF_MAX_COLS)   { *cols = Q9_SCREENBUF_MAX_COLS; }
}

int main(void)
{
    int rows, cols;
    q9_screenbuf_t sb;
    q9_listview_t lv;
    char out[1 << 16];
    int running = 1;

    if (q9_term_size(&rows, &cols) != 0) {
        rows = 24;
        cols = 80;
    }
    clamp_dims(&rows, &cols);

    if (q9_input_init() != 0) {
        fprintf(stderr, "integration_demo: kein echtes Terminal (TTY) -- Abbruch.\n");
        return 1;
    }
    write_ansi(wrap_hide);

    q9_listview_init(&lv, 2, 3, rows - 5, cols - 6, ITEM_COUNT);

    while (running) {
        char status[256];

        q9_screenbuf_init(&sb, rows, cols);
        q9_screenbuf_draw_frame(&sb, 0, 0, rows, cols,
                                 "Q9-Flux Editor -- Integrations-Demo", 200, 200, 255);
        q9_screenbuf_puts(&sb, rows - 1 < 2 ? 1 : rows - 2, 3,
                           "Pfeiltasten: navigieren   Strg-C: beenden", 150, 150, 150);

        lv.row    = 2;
        lv.col    = 3;
        lv.height = rows - 5;
        lv.width  = cols - 6;
        if (lv.height < 1) { lv.height = 1; }
        if (lv.width  < 1) { lv.width  = 1; }
        lv.scroll_offset = q9_listview_scroll(lv.selected, lv.scroll_offset, lv.height, lv.item_count);
        q9_listview_render(&lv, &sb, g_items, 220, 220, 220, 0, 0, 0, 255, 255, 0);

        snprintf(status, sizeof(status), "Ausgewaehlt: %s  |  Terminal: %dx%d",
                 (lv.selected >= 0 && lv.selected < ITEM_COUNT) ? g_items[lv.selected] : "-",
                 rows, cols);
        q9_screenbuf_puts(&sb, rows - 1, 3, status, 100, 200, 255);

        {
            unsigned n = q9_screenbuf_render(&sb, 0, 0, out, sizeof(out));
            fwrite(out, 1, n, stdout);
            fflush(stdout);
        }

        {
            q9_key_t k = q9_input_read_key();
            switch (k.kind) {
                case Q9_KEY_CTRL_C:
                case Q9_KEY_EOF:
                    running = 0;
                    break;
                case Q9_KEY_UP:
                    q9_listview_move(&lv, -1);
                    break;
                case Q9_KEY_DOWN:
                    q9_listview_move(&lv, 1);
                    break;
                case Q9_KEY_RESIZE:
                    if (q9_term_size(&rows, &cols) == 0) {
                        clamp_dims(&rows, &cols);
                    }
                    break;
                default:
                    break;                                 /* alle anderen Tasten: ignorieren */
            }
        }
    }

    {
        char buf[64];
        unsigned n = 0;
        n += q9_ansi_reset(buf + n, sizeof(buf) - n);
        n += q9_ansi_clear(buf + n, sizeof(buf) - n);
        n += q9_ansi_move(buf + n, sizeof(buf) - n, 1, 1);
        fwrite(buf, 1, n, stdout);
    }
    write_ansi(wrap_show);
    fflush(stdout);
    q9_input_shutdown();
    printf("integration_demo beendet.\n");
    return 0;
}

//────────────────────────────────────────────────────────────────────────────────────────────────
// EOF integration_demo.c                                                                  Ver. 1.00
//────────────────────────────────────────────────────────────────────────────────────────────────
