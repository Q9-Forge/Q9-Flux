//════════════════════════════════════════════════════════════════════════════════════════════════
// File:   q9stat.c                                                                        Ver. 1.00
// Owner:  AF
// Desc.:  Beispiel-Tool: sucht einen 8.3-Eintrag in einem Directory und schreibt bei Fund
//         dieselbe eine Zeile wie q9dir. Arbeitet ausschliesslich ueber libq9.
//
// Call:   err = q9stat_run("/d0", "DATEI.TXT")
//
// Edition History
//─────────┬──────┬────────────────────────────────────────────────────────────────────────┬──────
// Date    │ Ver. │ Description                                                            │ By
//─────────┼──────┼────────────────────────────────────────────────────────────────────────┼──────
// 26-07-04│ 1.00 │ Initiale Version                                                        │ CX
//═════════╧══════╧════════════════════════════════════════════════════════════════════════╧══════

#include <stdint.h>
#include <string.h>

#include "../lib/libq9.h"
#include "q9dir.h"
#include "q9stat.h"

static uint32_t cstr_len(const char *s)
{
    return (uint32_t)strlen(s);
}

static int entry_matches(const q9dir_entry_t *entry, const q9_name_parse_t *pn)
{
    uint32_t nlen;

    nlen = cstr_len(entry->name);
    if (nlen != pn->len) {
        return 0;
    }
    return q9_cmpnam(pn->name, entry->name, pn->len) == 0;
}

int q9stat_run(const char *dir_path, const char *name)
{
    q9_name_parse_t pn;
    uint16_t        in;
    q9dir_entry_t   entry;
    int             err;

    err = q9_prsnam(name, &pn);
    if (err != 0) {
        return err;
    }
    if (pn.next && *pn.next != 0) {
        return E_BPNAM;
    }

    err = q9_open(dir_path, Q9_MODE_READ, &in);
    if (err != 0) {
        return err;
    }
    for (;;) {
        err = q9dir_next(in, &entry);
        if (err == E_EOF) {
            err = E_PNNF;
            break;
        }
        if (err != 0) {
            break;
        }
        if (entry_matches(&entry, &pn)) {
            err = q9dir_write_entry(&entry);
            break;
        }
    }
    {
        int cerr = q9_close(in);
        return err != 0 ? err : cerr;
    }
}

//────────────────────────────────────────────────────────────────────────────────────────────────
// EOF q9stat.c                                                                          Ver. 1.00
//────────────────────────────────────────────────────────────────────────────────────────────────
