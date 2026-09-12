//════════════════════════════════════════════════════════════════════════════════════════════════
// File:   q9_widgets.h                                                                    Ver. 1.10
// Owner:  Claudia
// Desc.:  UI-Zeichenroutinen auf q9_screenbuf.h aufgesetzt -- Q9FLUX_EDITOR_de.md verlangt an
//         mehreren Stellen einen Rahmen um den modalen Config-Dialog (Abschnitt 2). Nutzt echte
//         Unicode-Box-Drawing-Zeichen (Q9_GLYPH_*, q9_screenbuf.h) -- Andreas' Wunsch (2026-08-17)
//         nach "vollen Linien" statt des urspruenglich bewusst gewaehlten reinen ASCII ('+','-','|').
//         BEKANNTER KOMPROMISS: aeltere Windows-Konsolen (vor UTF-8-Codepage/Windows Terminal)
//         stellen diese Zeichen ggf. falsch dar -- s. Kopfkommentar in q9_screenbuf.h.
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
// 26-08-17│ 1.10 │ Q9_GLYPH_* (echte Unicode-Box-Drawing) statt ASCII (Andreas: "volle       │ Cld
//         │      │ Linien")                                                                  │
//═════════╧══════╧════════════════════════════════════════════════════════════════════════╧══════
#ifndef Q9_WIDGETS_H
#define Q9_WIDGETS_H

#include "q9_screenbuf.h"

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_screenbuf_draw_frame
// Desc.:    Zeichnet einen Rahmen (Q9_GLYPH_TL/TR/BL/BR Ecken, Q9_GLYPH_HLINE/VLINE Kanten, s.
//           q9_screenbuf.h)
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
// EOF q9_widgets.h                                                                        Ver. 1.10
//────────────────────────────────────────────────────────────────────────────────────────────────
