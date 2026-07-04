//════════════════════════════════════════════════════════════════════════════════════════════════
// File:   q9cat.c                                                                         Ver. 1.00
// Owner:  AF
// Desc.:  Beispiel-Tool: Datei komplett lesen und auf Pfad 1 (stdout) schreiben. Arbeitet
//         ausschliesslich ueber libq9, nicht direkt ueber q9_syscall.
//
// Call:   err = q9cat_run("/d0/DATEI.TXT")
//
// Edition History
//─────────┬──────┬────────────────────────────────────────────────────────────────────────┬──────
// Date    │ Ver. │ Description                                                            │ By
//─────────┼──────┼────────────────────────────────────────────────────────────────────────┼──────
// 26-07-04│ 1.00 │ Initiale Version                                                        │ CX
//═════════╧══════╧════════════════════════════════════════════════════════════════════════╧══════

#include "../lib/libq9.h"
#include "q9cat.h"

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9cat_run
// Desc.:    Siehe q9cat.h. Liest in 128-Byte-Bloecken und prueft, dass Pfad 1 jeden Block
//           vollstaendig angenommen hat.
// Call:     err = q9cat_run("/d0/DATEI.TXT")
//════════════════════════════════════════════════════════════════════════════════════════════════
int q9cat_run(const char *path)
{
    uint8_t  buf[128];
    uint16_t in;
    int      err;

    err = q9_open(path, Q9_MODE_READ, &in);
    if (err != 0) {
        return err;
    }
    for (;;) {
        uint32_t got = 0;

        err = q9_read(in, buf, sizeof(buf), &got);
        if (err == E_EOF) {
            err = 0;
            break;
        }
        if (err != 0) {
            break;
        }
        if (got > 0) {
            uint32_t done = 0;
            err = q9_write(1, buf, got, &done);
            if (err != 0) {
                break;
            }
            if (done != got) {
                err = E_NOTRDY;
                break;
            }
        }
    }
    {
        int cerr = q9_close(in);
        return err != 0 ? err : cerr;
    }
}

//────────────────────────────────────────────────────────────────────────────────────────────────
// EOF q9cat.c                                                                            Ver. 1.00
//────────────────────────────────────────────────────────────────────────────────────────────────
