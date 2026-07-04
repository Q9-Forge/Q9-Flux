//════════════════════════════════════════════════════════════════════════════════════════════════
// File:   q9stat.h                                                                        Ver. 1.00
// Owner:  AF
// Desc.:  Deklaration fuer das kleine q9stat-Userland-Tool.
//
// Call:   err = q9stat_run("/d0", "DATEI.TXT")
//
// Edition History
//─────────┬──────┬────────────────────────────────────────────────────────────────────────┬──────
// Date    │ Ver. │ Description                                                            │ By
//─────────┼──────┼────────────────────────────────────────────────────────────────────────┼──────
// 26-07-04│ 1.00 │ Initiale Version                                                        │ CX
//═════════╧══════╧════════════════════════════════════════════════════════════════════════╧══════
#ifndef Q9_USERLAND_Q9STAT_H
#define Q9_USERLAND_Q9STAT_H

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9stat_run
// Desc.:    Sucht den 8.3-Namen "name" im Directory dir_path und schreibt den Treffer nach
//           Pfad 1. Rueckgabe: 0 = gefunden, E$PNNF = nicht gefunden, sonst Q9-Fehlercode.
// Call:     err = q9stat_run("/d0", "DATEI.TXT")
//════════════════════════════════════════════════════════════════════════════════════════════════
int q9stat_run(const char *dir_path, const char *name);

#endif // Q9_USERLAND_Q9STAT_H

//────────────────────────────────────────────────────────────────────────────────────────────────
// EOF q9stat.h                                                                          Ver. 1.00
//────────────────────────────────────────────────────────────────────────────────────────────────
