//════════════════════════════════════════════════════════════════════════════════════════════════
// File:   integration_demo.c                                                             Ver. 1.10
// Owner:  Claudia
// Desc.:  Reine SICHTPRUEFUNG (kein automatisierter Test, wie ansi_selftest --demo) -- zeigt alle
//         sechs Bausteine zusammen in einem einzigen, echten Bildschirm: Rahmen (q9_widgets),
//         scrollbare Liste (q9_listview) in einem Bildschirmpuffer (q9_screenbuf), Tastatur-
//         Navigation + dynamische Groessenanpassung (q9_input). NICHT der eigentliche Q9-Flux-
//         Editor (keine echte Config-Anbindung, keine Hardware-Typen, keine Buttons) -- nur der
//         Nachweis, dass die Bausteine zusammenpassen.
//
//         Farbpalette (Andreas' Wunsch, 2026-08-17): "aehnliche" Toene statt bunt gemischt --
//         warme Amber-/Beige-/Braun-Palette (Vorbild: Hermes-Farbschema). Eine "kuehle" Variante
//         (Blau/Gruen/Cyan) waere nach demselben Muster (nur die Zahlenwerte unten aendern) ebenso
//         moeglich -- eine echte UMSCHALTBARE Palette (mehrere Saetze + Auswahl zur Laufzeit) ist
//         als "Endausbau"-Idee vorgemerkt, hier erstmal nur EIN fest verdrahteter Satz.
//
// Call:   make -C tools/q9-flux-editor demo-integration
//         Pfeiltasten hoch/runter: Auswahl bewegen. Strg-C: beenden.
//
// Edition History
//─────────┬──────┬────────────────────────────────────────────────────────────────────────┬──────
// Date    │ Ver. │ Description                                                            │ By
//─────────┼──────┼────────────────────────────────────────────────────────────────────────┬──────
// 26-08-17│ 1.00 │ Erster Wurf -- Andreas wollte sich das Ganze mal ansehen                │ Cld
// 26-08-17│ 1.10 │ Andreas' Feedback nach dem ersten Ansehen: Mindestgroesse (60x20, sonst  │ Cld
//         │      │ Hinweis statt verzerrtem Layout), Statuszeile bekommt eine EIGENE Zeile   │
//         │      │ mit eigenem Hintergrund statt in die untere Rahmenkante gemischt zu       │
//         │      │ werden (das war die "kleine Linie ganz rechts unten"), warme Amber-/      │
//         │      │ Beige-Palette statt der bisherigen Zufallsfarben                          │
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

/* Warme Amber-/Beige-/Braun-Palette, s. Kopfkommentar. */
#define PAL_FRAME_R      190
#define PAL_FRAME_G      150
#define PAL_FRAME_B       70
#define PAL_LIST_FG_R    215
#define PAL_LIST_FG_G    195
#define PAL_LIST_FG_B    155
#define PAL_SEL_FG_R      35
#define PAL_SEL_FG_G      25
#define PAL_SEL_FG_B      10
#define PAL_SEL_BG_R     215
#define PAL_SEL_BG_G     165
#define PAL_SEL_BG_B      35
#define PAL_STATUS_FG_R  230
#define PAL_STATUS_FG_G  212
#define PAL_STATUS_FG_B  178
#define PAL_STATUS_BG_R   95
#define PAL_STATUS_BG_G   68
#define PAL_STATUS_BG_B   25

#define MIN_ROWS 20                                        /* Andreas' Wunsch (2026-08-17):     */
#define MIN_COLS 60                                         /* darunter sieht es "sehr komisch"
                                                                aus -- Hinweis statt Versuch      */

static void write_ansi(unsigned (*fn)(char *, unsigned))
{
    char buf[32];
    unsigned n = fn(buf, sizeof(buf));
    fwrite(buf, 1, n, stdout);
}

static unsigned wrap_hide(char *b, unsigned n)  { return q9_ansi_hide_cursor(b, n); }
static unsigned wrap_show(char *b, unsigned n)  { return q9_ansi_show_cursor(b, n); }

/* q9_screenbuf_init() klemmt intern zwar schon auf Q9_SCREENBUF_MAX_ROWS/_COLS (s. dortiger
   Kopfkommentar) -- ohne diese Klemmung HIER wuerden rows/cols aber trotzdem noch die groesseren,
   unklemmten Werte fuer Layout-Berechnungen (draw_frame/listview-Geometrie) verwenden, was auf sehr
   grossen Terminals (>200 Spalten) zu einem Rahmen fuehren wuerde, der nicht bis zum tatsaechlichen
   rechten/unteren Rand des Puffers reicht -- kein Crash (q9_screenbuf_* klemmt selbst pro Zelle),
   aber ein unschoenes Layout. Deshalb hier vorsorglich mit klemmen. */
static void clamp_dims(int *rows, int *cols)
{
    if (*rows > Q9_SCREENBUF_MAX_ROWS) { *rows = Q9_SCREENBUF_MAX_ROWS; }
    if (*cols > Q9_SCREENBUF_MAX_COLS) { *cols = Q9_SCREENBUF_MAX_COLS; }
}

static void render_too_small(int rows, int cols)
{
    q9_screenbuf_t sb;
    char out[4096];
    char msg[64];

    q9_screenbuf_init(&sb, rows, cols);
    snprintf(msg, sizeof(msg), "Fenster zu klein (%dx%d) -- mind. %dx%d noetig",
             rows, cols, MIN_ROWS, MIN_COLS);
    q9_screenbuf_puts(&sb, rows / 2, 1, msg, PAL_FRAME_R, PAL_FRAME_G, PAL_FRAME_B);

    {
        unsigned n = q9_screenbuf_render(&sb, 0, 0, out, sizeof(out));
        fwrite(out, 1, n, stdout);
        fflush(stdout);
    }
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

    q9_listview_init(&lv, 2, 3, 1, 1, ITEM_COUNT);          /* echte Geometrie folgt in der Schleife */

    while (running) {
        char status[256];

        if (rows < MIN_ROWS || cols < MIN_COLS) {
            render_too_small(rows, cols);
        } else {
            q9_screenbuf_init(&sb, rows, cols);
            q9_screenbuf_draw_frame(&sb, 0, 0, rows, cols,
                                     "Q9-Flux Editor -- Integrations-Demo",
                                     PAL_FRAME_R, PAL_FRAME_G, PAL_FRAME_B);
            q9_screenbuf_puts(&sb, rows - 3, 3, "Pfeiltasten: navigieren   Strg-C: beenden",
                               PAL_FRAME_R, PAL_FRAME_G, PAL_FRAME_B);

            lv.row    = 2;
            lv.col    = 3;
            lv.height = rows - 6;                            /* Rand+Border+Hinweis+Statuszeile s.o. */
            lv.width  = cols - 6;
            if (lv.height < 1) { lv.height = 1; }
            if (lv.width  < 1) { lv.width  = 1; }
            lv.scroll_offset = q9_listview_scroll(lv.selected, lv.scroll_offset, lv.height, lv.item_count);
            q9_listview_render(&lv, &sb, g_items,
                                PAL_LIST_FG_R, PAL_LIST_FG_G, PAL_LIST_FG_B,
                                PAL_SEL_FG_R, PAL_SEL_FG_G, PAL_SEL_FG_B,
                                PAL_SEL_BG_R, PAL_SEL_BG_G, PAL_SEL_BG_B);

            /* Statuszeile: EIGENE Zeile mit eigenem (invertiertem, gedecktem) Hintergrund -- nicht
               mehr in die untere Rahmenkante gemischt (das war vorher die stoerende Kante-plus-Text-
               Mischung, s. Edition-History). Innerhalb der Seitenraender (Spalte 1..cols-2). */
            q9_screenbuf_fill_rect(&sb, rows - 2, 1, 1, cols - 2, ' ',
                                    PAL_STATUS_FG_R, PAL_STATUS_FG_G, PAL_STATUS_FG_B,
                                    1, PAL_STATUS_BG_R, PAL_STATUS_BG_G, PAL_STATUS_BG_B);
            snprintf(status, sizeof(status), " Ausgewaehlt: %s  |  Terminal: %dx%d",
                     (lv.selected >= 0 && lv.selected < ITEM_COUNT) ? g_items[lv.selected] : "-",
                     rows, cols);
            q9_screenbuf_puts(&sb, rows - 2, 1, status, PAL_STATUS_FG_R, PAL_STATUS_FG_G, PAL_STATUS_FG_B);

            {
                unsigned n = q9_screenbuf_render(&sb, 0, 0, out, sizeof(out));
                fwrite(out, 1, n, stdout);
                fflush(stdout);
            }
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
                    break;                                   /* alle anderen Tasten: ignorieren */
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
// EOF integration_demo.c                                                                  Ver. 1.10
//────────────────────────────────────────────────────────────────────────────────────────────────
