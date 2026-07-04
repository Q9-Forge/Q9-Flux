//════════════════════════════════════════════════════════════════════════════════════════════════
// File:   q9cat.h                                                                         Ver. 1.00
// Owner:  AF
// Desc.:  Deklaration fuer das kleine q9cat-Userland-Tool.
//
// Call:   err = q9cat_run("/d0/DATEI.TXT")
//
// Edition History
//─────────┬──────┬────────────────────────────────────────────────────────────────────────┬──────
// Date    │ Ver. │ Description                                                            │ By
//─────────┼──────┼────────────────────────────────────────────────────────────────────────┼──────
// 26-07-04│ 1.00 │ Initiale Version                                                        │ CX
// 26-07-04│ 1.01 │ argc/argv-Einsprungpunkt deklariert                                     │ CX
//═════════╧══════╧════════════════════════════════════════════════════════════════════════╧══════
#ifndef Q9_USERLAND_Q9CAT_H
#define Q9_USERLAND_Q9CAT_H

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9cat_run
// Desc.:    Oeffnet "path", liest die Datei komplett und schreibt den Inhalt nach Pfad 1
//           (stdout). Rueckgabe: 0 = ok, sonst Q9-Fehlercode.
// Call:     err = q9cat_run("/d0/DATEI.TXT")
//════════════════════════════════════════════════════════════════════════════════════════════════
int q9cat_run(const char *path);

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9cat_main
// Desc.:    Kommandozeilen-Einsprungpunkt fuer spaetere 68k-Module; parst genau einen Pfad
//           oder -h/--help und ruft q9cat_run(). Rueckgabe: Tool- oder Parser-Status.
// Call:     err = q9cat_main(argc, argv)
//════════════════════════════════════════════════════════════════════════════════════════════════
int q9cat_main(int argc, char **argv);

#endif // Q9_USERLAND_Q9CAT_H

//────────────────────────────────────────────────────────────────────────────────────────────────
// EOF q9cat.h                                                                            Ver. 1.00
//────────────────────────────────────────────────────────────────────────────────────────────────
