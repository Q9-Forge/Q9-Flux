//════════════════════════════════════════════════════════════════════════════════════════════════
// File:   q9copy.h                                                                        Ver. 1.00
// Owner:  AF
// Desc.:  Deklaration fuer das kleine q9copy-Userland-Tool.
//
// Call:   err = q9copy_run("/d0/A.TXT", "/d0/B.TXT")
//
// Edition History
//─────────┬──────┬────────────────────────────────────────────────────────────────────────┬──────
// Date    │ Ver. │ Description                                                            │ By
//─────────┼──────┼────────────────────────────────────────────────────────────────────────┼──────
// 26-07-04│ 1.00 │ Initiale Version                                                        │ CX
// 26-07-04│ 1.01 │ argc/argv-Einsprungpunkt deklariert                                     │ CX
//═════════╧══════╧════════════════════════════════════════════════════════════════════════╧══════
#ifndef Q9_USERLAND_Q9COPY_H
#define Q9_USERLAND_Q9COPY_H

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9copy_run
// Desc.:    Kopiert die Datei src nach dst ueber Q9-Dateioperationen. Rueckgabe: 0 = ok,
//           sonst Q9-Fehlercode.
// Call:     err = q9copy_run("/d0/A.TXT", "/d0/B.TXT")
//════════════════════════════════════════════════════════════════════════════════════════════════
int q9copy_run(const char *src, const char *dst);

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9copy_main
// Desc.:    Kommandozeilen-Einsprungpunkt fuer spaetere 68k-Module; parst Quelle/Ziel,
//           -v/--verbose und -h/--help und ruft q9copy_run(). Rueckgabe: Status.
// Call:     err = q9copy_main(argc, argv)
//════════════════════════════════════════════════════════════════════════════════════════════════
int q9copy_main(int argc, char **argv);

#endif // Q9_USERLAND_Q9COPY_H

//────────────────────────────────────────────────────────────────────────────────────────────────
// EOF q9copy.h                                                                           Ver. 1.00
//────────────────────────────────────────────────────────────────────────────────────────────────
