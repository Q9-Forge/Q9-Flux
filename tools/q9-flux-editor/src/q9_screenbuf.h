//════════════════════════════════════════════════════════════════════════════════════════════════
// File:   q9_screenbuf.h                                                                  Ver. 1.00
// Owner:  Claudia
// Desc.:  Bildschirmpuffer auf q9_ansi.h aufgesetzt -- der in Q9FLUX_EDITOR_de.md Abschnitt 2
//         angekuendigte Baustein fuer den modalen Config-Auswahl-Dialog ("liegt UEBER dem Rest,
//         Bildschirmbereich vor dem Zeichnen sichern, nach dem Schliessen wiederherstellen").
//
//         Der Puffer haelt den LOGISCHEN Bildschirminhalt (Zeichen + Vorder-/Hintergrundfarbe je
//         Zelle) als festes 2D-Array (kein malloc, Q9-Grundsatz, s. q9_ansi.h). Zeilen/Spalten sind
//         INTERN 0-indiziert (normale C-Array-Konvention) -- erst q9_screenbuf_render() rechnet auf
//         die 1-indizierte ANSI-Welt (q9_ansi_move) um. "snapshot" kopiert ein Rechteck in einen
//         ZWEITEN, kleineren q9_screenbuf_t (Aufrufer legt den an, z.B. auf dem Stack); "restore"
//         schreibt es zurueck. Ein modaler Dialog macht also: snapshot(Bereich) -> Dialog
//         zeichnen+bedienen -> restore(Bereich) -> render(restore-Ergebnis) an den echten
//         Terminal-Bereich schicken (nur DAS Rechteck neu zeichnen, nicht den ganzen Schirm).
//
//         Bewusst KEIN eingebautes stdout/fwrite -- wie q9_ansi.h schreibt jede Funktion nur in
//         einen vom Aufrufer bereitgestellten Puffer (testbar ohne echtes Terminal).
//
// Call:   q9_screenbuf_t sb; char out[8192]; unsigned n;
//         q9_screenbuf_init(&sb, 24, 80);
//         q9_screenbuf_fill_rect(&sb, 0,0, 24,80, ' ', 255,255,255, 0, 0,0,0);
//         q9_screenbuf_puts(&sb, 2, 4, "Hallo", 255,140,0);
//         n = q9_screenbuf_render(&sb, 0, 0, out, sizeof(out));
//         fwrite(out, 1, n, stdout);
//
//         q9_screenbuf_t snap; q9_screenbuf_snapshot(&sb, 5,10, 8,40, &snap);  // vor dem Dialog
//         ... Dialog in sb zeichnen (fill_rect/puts) ...
//         q9_screenbuf_restore(&sb, 5,10, &snap);                              // nach dem Dialog
//         n = q9_screenbuf_render(&snap, 5, 10, out, sizeof(out));             // nur das Rechteck
//         fwrite(out, 1, n, stdout);
//
// Edition History
//─────────┬──────┬────────────────────────────────────────────────────────────────────────┬──────
// Date    │ Ver. │ Description                                                            │ By
//─────────┼──────┼────────────────────────────────────────────────────────────────────────┬──────
// 26-08-16│ 1.00 │ Erster Wurf -- Q9FLUX_EDITOR_de.md Abschnitt 2 (modaler Dialog braucht   │ Cld
//         │      │ Bildschirmbereich-Sichern/Wiederherstellen)                              │
//═════════╧══════╧════════════════════════════════════════════════════════════════════════╧══════
#ifndef Q9_SCREENBUF_H
#define Q9_SCREENBUF_H

/* Obergrenzen fuer das feste Array (kein malloc). 60x200 deckt jedes realistische Terminal-Fenster
   ab (Standard ist 24x80); q9_screenbuf_init() beschraenkt rows/cols zusaetzlich zur Laufzeit auf
   diese Maximalwerte, falls ein Aufrufer versehentlich mehr anfordert. */
#define Q9_SCREENBUF_MAX_ROWS 60
#define Q9_SCREENBUF_MAX_COLS 200

typedef struct {
    char          ch;                                 /* 0/'\0' wird beim Rendern wie ' ' behandelt */
    unsigned char has_fg;                              /* 0 = keine Vordergrundfarbe (Terminal-Default) */
    unsigned char fg_r, fg_g, fg_b;
    unsigned char has_bg;                              /* 0 = kein Hintergrund (Terminal-Default)     */
    unsigned char bg_r, bg_g, bg_b;
} q9_screencell_t;

typedef struct {
    int rows;                                          /* tatsaechlich genutzt, 0..Q9_SCREENBUF_MAX_ROWS */
    int cols;                                          /* tatsaechlich genutzt, 0..Q9_SCREENBUF_MAX_COLS */
    q9_screencell_t cell[Q9_SCREENBUF_MAX_ROWS][Q9_SCREENBUF_MAX_COLS];
} q9_screenbuf_t;

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_screenbuf_init
// Desc.:    Setzt rows/cols (auf Q9_SCREENBUF_MAX_ROWS/_COLS geklemmt, negative Werte -> 0) und
//           leert alle genutzten Zellen (Leerzeichen, keine Farbe). Erster Aufruf vor allem anderen.
// Call:     q9_screenbuf_init(&sb, 24, 80)
//════════════════════════════════════════════════════════════════════════════════════════════════
void q9_screenbuf_init(q9_screenbuf_t *sb, int rows, int cols);

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_screenbuf_puts
// Desc.:    Schreibt s ab (row,col) in EINE Zeile (kein Zeilenumbruch -- s wird an der rechten
//           Pufferkante abgeschnitten, laeuft NICHT in die naechste Zeile weiter). col < 0
//           ueberspringt die entsprechend vielen Zeichen am STRING-Anfang (linksseitiges Abschneiden,
//           z.B. wenn ein Label absichtlich teilweise vor der linken Puffergrenze beginnen wuerde).
//           row ausserhalb [0,rows) tut nichts. Setzt nur Zeichen+Vordergrund, laesst den
//           Hintergrund jeder betroffenen Zelle UNVERAENDERT (fuer einen eigenen Hintergrund vorher
//           fill_rect verwenden).
// Call:     q9_screenbuf_puts(&sb, 2, 4, "Hallo", 255, 140, 0)
//════════════════════════════════════════════════════════════════════════════════════════════════
void q9_screenbuf_puts(q9_screenbuf_t *sb, int row, int col, const char *s,
                        int fg_r, int fg_g, int fg_b);

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_screenbuf_fill_rect
// Desc.:    Fuellt ein Rechteck (row,col) bis (row+rows-1,col+cols-1) mit ch in der gegebenen
//           Vordergrundfarbe. use_bg=0: Hintergrund der betroffenen Zellen wird geloescht (has_bg=0,
//           Terminal-Default); use_bg!=0: bg_r/g/b wird gesetzt. Fuer Rahmen (ch='#' o.ae.) und
//           gefuellte Flaechen (ch=' ' mit use_bg) gleichermassen gedacht. Teile ausserhalb des
//           Puffers werden stillschweigend uebersprungen (kein Fehler, kein UB).
// Call:     q9_screenbuf_fill_rect(&sb, 0,0, 24,80, ' ', 255,255,255, 1, 0,0,80)   // blauer Hintergrund
//════════════════════════════════════════════════════════════════════════════════════════════════
void q9_screenbuf_fill_rect(q9_screenbuf_t *sb, int row, int col, int rows, int cols, char ch,
                             int fg_r, int fg_g, int fg_b,
                             int use_bg, int bg_r, int bg_g, int bg_b);

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_screenbuf_render
// Desc.:    Erzeugt die komplette ANSI-Byte-Folge, um sb auf einem echten Terminal darzustellen.
//           origin_row/origin_col verschieben sb's Zelle (0,0) auf die ECHTE Terminalposition
//           (origin_row, origin_col) -- 0,0 fuer einen Voll-Render; die tatsaechliche Position eines
//           frueheren snapshot()-Aufrufs, wenn NUR ein wiederhergestelltes Rechteck neu gezeichnet
//           werden soll (spart die Vollbild-Ausgabe). Emittiert SGR-Codes nur, wenn sich Vorder-/
//           Hintergrund gegenueber der vorherigen Zelle aendern (nicht pro Zelle einzeln) --
//           trotzdem bewusst simpel gehalten (Reset+Neuansage bei JEDER Aenderung, keine
//           Delta-Optimierung zwischen Vorder- und Hintergrund einzeln) fuer leichte Testbarkeit.
//           Endet mit einem abschliessenden SGR-Reset (Terminal bleibt in sauberem Zustand).
//           Truncation-sicher wie q9_ansi.h: bei zu kleinem out kein UB, aber eine unvollstaendige
//           (abgeschnittene) Byte-Folge -- Aufrufer traegt Verantwortung fuer ausreichend grossen
//           Puffer (Faustregel: rows*cols*~24 Byte fuer den ungünstigsten Fall, jede Zelle wechselt
//           Farbe).
// Call:     n = q9_screenbuf_render(&sb, 0, 0, out, sizeof(out))
//════════════════════════════════════════════════════════════════════════════════════════════════
unsigned q9_screenbuf_render(const q9_screenbuf_t *sb, int origin_row, int origin_col,
                              char *out, unsigned out_max);

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_screenbuf_snapshot
// Desc.:    Kopiert das Rechteck (row,col)..(row+rows-1,col+cols-1) aus sb in snap (eigenstaendiger,
//           kleinerer Puffer -- snap->rows/cols werden hier gesetzt). Das Rechteck wird an ALLEN
//           vier Seiten auf die tatsaechlichen sb-Grenzen UND auf Q9_SCREENBUF_MAX_ROWS/_COLS
//           geklemmt (kein Fehler bei zu grosser/negativer Anfrage, einfach ein kleineres
//           Ergebnis). Fuer den modalen Dialog: VOR dem Zeichnen des Dialogs aufrufen.
// Call:     q9_screenbuf_t snap; q9_screenbuf_snapshot(&sb, 5,10, 8,40, &snap)
//════════════════════════════════════════════════════════════════════════════════════════════════
void q9_screenbuf_snapshot(const q9_screenbuf_t *sb, int row, int col, int rows, int cols,
                            q9_screenbuf_t *snap);

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_screenbuf_restore
// Desc.:    Schreibt einen frueheren snapshot() zurueck nach sb, an derselben Position (row,col),
//           an der er entnommen wurde (Aufrufer traegt row/col selbst weiter -- snap kennt seine
//           urspruengliche Position nicht). Zellen ausserhalb von sb werden uebersprungen. Aendert
//           NUR den logischen Puffer -- fuer die sichtbare Wiederherstellung anschliessend
//           q9_screenbuf_render(snap, row, col, ...) aufrufen und an den Terminal schicken (nur das
//           betroffene Rechteck, kein Vollbild-Redraw noetig).
// Call:     q9_screenbuf_restore(&sb, 5, 10, &snap)
//════════════════════════════════════════════════════════════════════════════════════════════════
void q9_screenbuf_restore(q9_screenbuf_t *sb, int row, int col, const q9_screenbuf_t *snap);

#endif /* Q9_SCREENBUF_H */
//────────────────────────────────────────────────────────────────────────────────────────────────
// EOF q9_screenbuf.h                                                                      Ver. 1.00
//────────────────────────────────────────────────────────────────────────────────────────────────
