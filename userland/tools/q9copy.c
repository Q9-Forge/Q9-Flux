//════════════════════════════════════════════════════════════════════════════════════════════════
// File:   q9copy.c                                                                        Ver. 1.00
// Owner:  AF
// Desc.:  Beispiel-Tool: kopiert eine Datei innerhalb von Q9. Arbeitet ausschliesslich ueber
//         libq9, nicht direkt ueber q9_syscall.
//
// Call:   err = q9copy_run("/d0/A.TXT", "/d0/B.TXT")
//
// Edition History
//─────────┬──────┬────────────────────────────────────────────────────────────────────────┬──────
// Date    │ Ver. │ Description                                                            │ By
//─────────┼──────┼────────────────────────────────────────────────────────────────────────┼──────
// 26-07-04│ 1.00 │ Initiale Version                                                        │ CX
//═════════╧══════╧════════════════════════════════════════════════════════════════════════╧══════

#include "../lib/libq9.h"
#include "q9copy.h"

int q9copy_run(const char *src, const char *dst)
{
    uint8_t  buf[128];
    uint16_t in  = 0;
    uint16_t out = 0;
    int      have_in = 0;
    int      have_out = 0;
    int      err;

    err = q9_open(src, Q9_MODE_READ, &in);
    if (err != 0) {
        return err;
    }
    have_in = 1;

    err = q9_create(dst, Q9_MODE_WRITE, &out);
    if (err != 0) {
        q9_close(in);
        return err;
    }
    have_out = 1;

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
            uint32_t put = 0;
            err = q9_write(out, buf, got, &put);
            if (err != 0) {
                break;
            }
            if (put != got) {
                err = E_NOTRDY;
                break;
            }
        }
    }

    if (have_out) {
        int cerr = q9_close(out);
        if (err == 0) {
            err = cerr;
        }
    }
    if (have_in) {
        int cerr = q9_close(in);
        if (err == 0) {
            err = cerr;
        }
    }
    return err;
}

//────────────────────────────────────────────────────────────────────────────────────────────────
// EOF q9copy.c                                                                          Ver. 1.00
//────────────────────────────────────────────────────────────────────────────────────────────────
