//════════════════════════════════════════════════════════════════════════════════════════════════
// File:   q9rm.h                                                                          Ver. 1.00
// Owner:  AF
// Desc.:  Deklaration fuer das kleine q9rm-Userland-Tool.
//
// Call:   err = q9rm_run("/d0/DATEI.TXT")
//
// Edition History
//─────────┬──────┬────────────────────────────────────────────────────────────────────────┬──────
// Date    │ Ver. │ Description                                                            │ By
//─────────┼──────┼────────────────────────────────────────────────────────────────────────┼──────
// 26-07-04│ 1.00 │ Initiale Version                                                        │ CX
// 26-07-04│ 1.01 │ argc/argv-Einsprungpunkt deklariert                                     │ CX
//═════════╧══════╧════════════════════════════════════════════════════════════════════════╧══════
#ifndef Q9_USERLAND_Q9RM_H
#define Q9_USERLAND_Q9RM_H

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9rm_run
// Desc.:    Loescht die Datei "path" und schreibt bei Erfolg eine Bestaetigung nach Pfad 1.
//           Rueckgabe: 0 = ok, sonst Q9-Fehlercode.
// Call:     err = q9rm_run("/d0/DATEI.TXT")
//════════════════════════════════════════════════════════════════════════════════════════════════
int q9rm_run(const char *path);

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9rm_main
// Desc.:    Kommandozeilen-Einsprungpunkt fuer spaetere 68k-Module; parst genau einen Pfad
//           oder -h/--help und ruft q9rm_run(). Rueckgabe: Tool- oder Parser-Status.
// Call:     err = q9rm_main(argc, argv)
//════════════════════════════════════════════════════════════════════════════════════════════════
int q9rm_main(int argc, char **argv);

#endif // Q9_USERLAND_Q9RM_H

//────────────────────────────────────────────────────────────────────────────────────────────────
// EOF q9rm.h                                                                            Ver. 1.00
//────────────────────────────────────────────────────────────────────────────────────────────────
