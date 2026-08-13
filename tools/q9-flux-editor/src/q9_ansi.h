//════════════════════════════════════════════════════════════════════════════════════════════════
// File:   q9_ansi.h                                                                       Ver. 1.00
// Owner:  Claudia
// Desc.:  6.7-Nachfolge: rohe ANSI-/VT100-Escape-Sequenzen fuer den kuenftigen Q9-Flux-Launcher/
//         Config-Editor (docs/Q9FLUX_EDITOR_de.md) -- BEWUSST KEIN TUI-Framework (kein Turbo
//         Vision, kein ncurses, kein FTXUI). Grund (Andreas, 2026-08-13, ueber den gescheiterten
//         Erstversuch tools/q9-launcher-prototype/): "keine Kontrolle ueber die Farben, keine
//         Kontrolle ueber die Lage der Objekte, und keine Kontrolle was sie machen sollen ...
//         nichts passte". Turbo Vision positioniert/faerbt ueber mehrere Indirektionsebenen
//         (TRect relativ zum Elternobjekt, TPalette-Indizes) -- eine Anweisung wie "3 Zeilen
//         tiefer" muss durch diese Ebenen hindurch verstanden werden. Hier steht die Zahl direkt
//         im Code: `q9_ansi_move(out, sizeof(out), row+3, col)` -- "3 Zeilen tiefer" ist eine
//         mechanische `+3` im Aufruf, keine Framework-Uebersetzung.
//
//         Alle Funktionen SCHREIBEN NUR TEXT in einen vom Aufrufer bereitgestellten Puffer
//         (kein malloc, Q9-Grundsatz, snprintf-Stil: Rueckgabe = Anzahl geschriebener Zeichen ohne
//         Nullterminator) -- kein direktes stdout hier. Das macht die Erzeugung testbar (ein Test
//         kann die erzeugten Bytes exakt nachpruefen, ohne einen echten Terminal zu brauchen) UND
//         wiederverwendbar (spaeter: in einen Bildschirmpuffer statt direkt auf stdout schreiben,
//         fuer den in der Editor-Planung vorgesehenen modalen Dialog "liegt ueber dem Rest,
//         Wiederherstellung beim Schliessen" -- braucht einen eigenen Bildschirmpuffer, noch nicht
//         Teil dieses ersten Schritts).
//
// Call:   char buf[64]; unsigned n = 0;
//         n += q9_ansi_move(buf+n, sizeof(buf)-n, 5, 10);      // Cursor nach Zeile 5, Spalte 10
//         n += q9_ansi_fg_rgb(buf+n, sizeof(buf)-n, 255,140,0);// Text-Vordergrundfarbe RGB
//         fwrite(buf, 1, n, stdout);                            // ECHTE Anzeige: selbst rausschreiben
//
// Edition History
//─────────┬──────┬────────────────────────────────────────────────────────────────────────┬──────
// Date    │ Ver. │ Description                                                            │ By
//─────────┼──────┼────────────────────────────────────────────────────────────────────────┼──────
// 26-08-14│ 1.00 │ Erster Wurf -- Cursor-Position, RGB-Vorder-/Hintergrund, Clear, Cursor   │ Cld
//         │      │ ein-/ausblenden. Reine Positions-/Farb-Primitive, noch kein Dialog-/     │
//         │      │ Bildschirmpuffer-System (kommt erst, wenn der Editor selbst angegangen   │
//         │      │ wird)                                                                    │
//═════════╧══════╧════════════════════════════════════════════════════════════════════════╧══════
#ifndef Q9_ANSI_H
#define Q9_ANSI_H

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_ansi_move
// Desc.:    Cursor Position (CUP): "ESC[{row};{col}H". ANSI/VT100 zaehlt Zeile/Spalte ab 1 (nicht
//           0) -- row=1,col=1 ist die obere linke Ecke. Keine Bereichspruefung auf row/col (ein
//           Terminal ignoriert Werte ausserhalb seiner Groesse selbst, kein Q9-seitiger Grund,
//           das vorher abzufangen).
// Call:     n = q9_ansi_move(out, out_max, 5, 10);   // -> "\x1b[5;10H"
//════════════════════════════════════════════════════════════════════════════════════════════════
unsigned q9_ansi_move(char *out, unsigned out_max, int row, int col);

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_ansi_fg_rgb / q9_ansi_bg_rgb
// Desc.:    SGR (Select Graphic Rendition) Truecolor: "ESC[38;2;{r};{g};{b}m" (Vordergrund) bzw.
//           "ESC[48;2;{r};{g};{b}m" (Hintergrund) -- direkter 24-Bit-RGB-Wert, KEIN Palette-Index
//           (anders als Turbo Visions TPalette -- genau die Indirektion, die vermieden werden
//           soll). r/g/b werden auf 0..255 geklemmt (kein UB bei falscher Eingabe, aber auch keine
//           Fehlermeldung -- reine Text-Erzeugung, keine Config-Validierung).
// Call:     n = q9_ansi_fg_rgb(out, out_max, 255, 140, 0);   // -> "\x1b[38;2;255;140;0m"
//════════════════════════════════════════════════════════════════════════════════════════════════
unsigned q9_ansi_fg_rgb(char *out, unsigned out_max, int r, int g, int b);
unsigned q9_ansi_bg_rgb(char *out, unsigned out_max, int r, int g, int b);

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_ansi_reset
// Desc.:    SGR Reset: "ESC[0m" -- alle Text-Attribute (Farbe, Fett, ...) auf Terminal-Default.
//════════════════════════════════════════════════════════════════════════════════════════════════
unsigned q9_ansi_reset(char *out, unsigned out_max);

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_ansi_clear
// Desc.:    Erase Display (ED, Modus 2): "ESC[2J" -- loescht den GESAMTEN sichtbaren Bildschirm,
//           bewegt den Cursor NICHT (typischerweise mit q9_ansi_move(1,1) kombiniert).
//════════════════════════════════════════════════════════════════════════════════════════════════
unsigned q9_ansi_clear(char *out, unsigned out_max);

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_ansi_hide_cursor / q9_ansi_show_cursor
// Desc.:    DECTCEM (privater Modus 25): "ESC[?25l" (aus) / "ESC[?25h" (an).
//════════════════════════════════════════════════════════════════════════════════════════════════
unsigned q9_ansi_hide_cursor(char *out, unsigned out_max);
unsigned q9_ansi_show_cursor(char *out, unsigned out_max);

#endif /* Q9_ANSI_H */
//────────────────────────────────────────────────────────────────────────────────────────────────
// EOF q9_ansi.h                                                                           Ver. 1.00
//────────────────────────────────────────────────────────────────────────────────────────────────
