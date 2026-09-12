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

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: write_part
// Desc.:    Schreibt genau len Bytes aus s nach Pfad 1 und meldet E$NotRdy bei Kurzschreibung.
// Call:     err = write_part("rm: ", 4)
//════════════════════════════════════════════════════════════════════════════════════════════════
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

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: write_confirm
// Desc.:    Schreibt die q9rm-Bestaetigungszeile fuer path nach Pfad 1.
// Call:     err = write_confirm(path)
//════════════════════════════════════════════════════════════════════════════════════════════════
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

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9rm_run
// Desc.:    Siehe q9rm.h. Fuehrt I$Delete ueber libq9 aus und bestaetigt nur bei Erfolg.
// Call:     err = q9rm_run("/d0/DATEI.TXT")
//════════════════════════════════════════════════════════════════════════════════════════════════
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
