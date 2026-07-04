//════════════════════════════════════════════════════════════════════════════════════════════════
// File:   q9dir.h                                                                         Ver. 1.00
// Owner:  AF
// Desc.:  Deklaration fuer das kleine q9dir-Userland-Tool und gemeinsame Directory-Helfer.
//
// Call:   err = q9dir_run("/d0")
//
// Edition History
//─────────┬──────┬────────────────────────────────────────────────────────────────────────┬──────
// Date    │ Ver. │ Description                                                            │ By
//─────────┼──────┼────────────────────────────────────────────────────────────────────────┼──────
// 26-07-04│ 1.00 │ Initiale Version                                                        │ CX
// 26-07-04│ 1.01 │ Gemeinsame Directory-Helfer fuer q9stat freigegeben                     │ CX
//═════════╧══════╧════════════════════════════════════════════════════════════════════════╧══════
#ifndef Q9_USERLAND_Q9DIR_H
#define Q9_USERLAND_Q9DIR_H

#include <stdint.h>

typedef struct q9dir_entry {
    int      is_dir;
    uint32_t size;
    char     name[13];
} q9dir_entry_t;

int q9dir_next(uint16_t path, q9dir_entry_t *out_entry);
int q9dir_write_entry(const q9dir_entry_t *entry);
int q9dir_run(const char *path);

#endif // Q9_USERLAND_Q9DIR_H

//────────────────────────────────────────────────────────────────────────────────────────────────
// EOF q9dir.h                                                                            Ver. 1.00
//────────────────────────────────────────────────────────────────────────────────────────────────
