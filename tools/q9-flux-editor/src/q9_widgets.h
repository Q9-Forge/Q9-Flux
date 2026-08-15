//════════════════════════════════════════════════════════════════════════════════════════════════
// File:   q9_widgets.h                                                                    Ver. 1.00
// Owner:  Claudia
// Desc.:  UI-Zeichenroutinen auf q9_screenbuf.h aufgesetzt -- Q9FLUX_EDITOR_de.md verlangt an
//         mehreren Stellen "ASCII-gerahmt" (Abschnitt 2, modaler Config-Dialog). Bewusst reine
//         ASCII-Zeichen ('+','-','|'), keine Unicode-Box-Drawing-Glyphen -- deckt sich mit dem
//         Wortlaut des Plans UND vermeidet Breiten-/Encoding-Fallstricke auf Terminals, die breite
//         Unicode-Zeichen nicht wie ein Terminalzeichen behandeln (v.a. aeltere Windows-Konsolen).
//
//         Erster Baustein: nur der Rahmen selbst (draw_frame). Buttons/Textfelder folgen erst, wenn
//         ihr genaues Aussehen mit Andreas geklaert ist (im Plan noch offen, s. dortiger
//         "Start/Speichern/Beenden-Buttons -- genauer Zuschnitt/Wortlaut nicht final").
//
// Call:   q9_screenbuf_draw_frame(&sb, 5, 10, 8, 40, "Config auswaehlen", 255, 255, 255);
//
// Edition History
//─────────┬──────┬────────────────────────────────────────────────────────────────────────┬──────
// Date    │ Ver. │ Description                                                            │ By
//─────────┼──────┼────────────────────────────────────────────────────────────────────────┬──────
// 26-08-16│ 1.00 │ Erster Wurf -- draw_frame (Rahmen + optionaler mittiger Titel)          │ Cld
//═════════╧══════╧════════════════════════════════════════════════════════════════════════╧══════
#ifndef Q9_WIDGETS_H
#define Q9_WIDGETS_H

#include "q9_screenbuf.h"

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_screenbuf_draw_frame
// Desc.:    Zeichnet einen ASCII-Rahmen ('+' Ecken, '-' obere/untere Kante, '|' linke/rechte Kante)
//           von (row,col) bis (row+rows-1,col+cols-1). Bei rows<2 oder cols<2 (kein gueltiger
//           Rahmen moeglich) passiert nichts. title (optional, NULL/leer = kein Titel) wird mittig
//           in die OBERE Kante geschrieben, mit je einem Leerzeichen davor/danach, und bei Bedarf
//           abgeschnitten, wenn er nicht in die Breite passt (kein Umbruch, kein Ueberlauf ueber
//           die Ecken hinaus). Zeichnet NUR die Kante selbst -- das Innere des Rahmens bleibt
//           unveraendert (fuer eine eigene Innenflaeche vorher/danach q9_screenbuf_fill_rect
//           verwenden). Bounds-sicher wie alle q9_screenbuf-Funktionen (Rechteck teilweise/ganz
//           ausserhalb des Puffers wird stillschweigend abgeschnitten, kein UB).
// Call:     q9_screenbuf_draw_frame(&sb, 5, 10, 8, 40, "Config auswaehlen", 255, 255, 255)
//════════════════════════════════════════════════════════════════════════════════════════════════
void q9_screenbuf_draw_frame(q9_screenbuf_t *sb, int row, int col, int rows, int cols,
                              const char *title, int fg_r, int fg_g, int fg_b);

#endif /* Q9_WIDGETS_H */
//────────────────────────────────────────────────────────────────────────────────────────────────
// EOF q9_widgets.h                                                                        Ver. 1.00
//────────────────────────────────────────────────────────────────────────────────────────────────
