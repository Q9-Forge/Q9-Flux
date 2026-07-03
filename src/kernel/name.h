//════════════════════════════════════════════════════════════════════════════════════════════════
// File:   name.h                                                                          Ver. 1.00
// Owner:  AF
// Desc.:  Pathlist-Namensparsing nach OS-9-Regeln (F$PrsNam/F$CmpNam-Unterbau).
//         Gültige Namenszeichen: A-Z a-z 0-9 _ . $ — Elemente durch '/' getrennt.
//
// Call:   err = q9_name_parse("/term/xyz", &start, &len)
//
// Edition History
//─────────┬──────┬────────────────────────────────────────────────────────────────────────┬──────
// Date    │ Ver. │ Description                                                            │ By
//─────────┼──────┼────────────────────────────────────────────────────────────────────────┼──────
// 26-07-03│ 1.00 │ Initiale Version: q9_name_parse, q9_name_cmp                           │ CF
//═════════╧══════╧════════════════════════════════════════════════════════════════════════╧══════
#ifndef Q9_NAME_H
#define Q9_NAME_H

#include <stdint.h>

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_name_parse
// Desc.:    Parst das nächste Pathlist-Element: überspringt einen führenden '/', liefert
//           Startadresse und Länge des Namens. E$BPNam, wenn an der Position kein gültiger
//           Name beginnt. Für Ketten: nächster Aufruf mit start+len.
// Call:     err = q9_name_parse(pathlist, &start, &len)
//════════════════════════════════════════════════════════════════════════════════════════════════
int q9_name_parse(const char *pathlist, const char **start, uint32_t *len);

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_name_cmp
// Desc.:    Vergleicht zwei Namen fester Länge, case-insensitiv (OS-9-Namen sind case-
//           unempfindlich). 0 = gleich, sonst E$Diff.
// Call:     err = q9_name_cmp("TERM", 4, "term")
//════════════════════════════════════════════════════════════════════════════════════════════════
int q9_name_cmp(const char *a, uint32_t len, const char *b);

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_name_is_char
// Desc.:    1, wenn c ein gültiges OS-9-Namenszeichen ist (A-Z a-z 0-9 _ . $).
// Call:     if (q9_name_is_char(c)) ...
//════════════════════════════════════════════════════════════════════════════════════════════════
int q9_name_is_char(char c);

#endif // Q9_NAME_H

//────────────────────────────────────────────────────────────────────────────────────────────────
// EOF name.h                                                                              Ver. 1.00
//────────────────────────────────────────────────────────────────────────────────────────────────
