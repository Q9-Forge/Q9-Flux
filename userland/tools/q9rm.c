//════════════════════════════════════════════════════════════════════════════════════════════════
// File:   q9rm.c                                                                          Ver. 1.00
// Owner:  AF
// Desc.:  Beispiel-Tool: loescht eine Datei und bestaetigt den Erfolg auf Pfad 1. Arbeitet
//         ausschliesslich ueber libq9, nicht direkt ueber q9_syscall.
//
// Call:   err = q9rm_run("/d0/DATEI.TXT")
//
// Edition History
//─────────┬──────┬────────────────────────────────────────────────────────────────────────┬──────
// Date    │ Ver. │ Description                                                            │ By
//─────────┼──────┼────────────────────────────────────────────────────────────────────────┼──────
// 26-07-04│ 1.00 │ Initiale Version                                                        │ CX
//═════════╧══════╧════════════════════════════════════════════════════════════════════════╧══════

#include <string.h>

#include "../lib/libq9.h"
#include "q9rm.h"

static int write_part(const char *s, uint32_t len)
{
    uint32_t put = 0;
    int      err;

    err = q9_writln(1, s, len, &put);
    if (err != 0) {
        return err;
    }
    return put == len ? 0 : E_NOTRDY;
}

static int write_confirm(const char *path)
{
    int err;

    err = write_part("rm: ", 4u);
    if (err != 0) {
        return err;
    }
    err = write_part(path, (uint32_t)strlen(path));
    if (err != 0) {
        return err;
    }
    return write_part("\n", 1u);
}

int q9rm_run(const char *path)
{
    int err;

    err = q9_delete(path);
    if (err != 0) {
        return err;
    }
    return write_confirm(path);
}

//────────────────────────────────────────────────────────────────────────────────────────────────
// EOF q9rm.c                                                                            Ver. 1.00
//────────────────────────────────────────────────────────────────────────────────────────────────
