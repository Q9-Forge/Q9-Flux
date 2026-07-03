//════════════════════════════════════════════════════════════════════════════════════════════════
// File:   name.c                                                                          Ver. 1.10
// Owner:  AF
// Desc.:  Pathlist-Namensparsing nach OS-9-Regeln — Unterbau für F$PrsNam/F$CmpNam und
//         alles Namensbasierte (I$Attach 1.6, VFS Phase 3).
//
// Call:   siehe name.h
//
// Edition History
//─────────┬──────┬────────────────────────────────────────────────────────────────────────┬──────
// Date    │ Ver. │ Description                                                            │ By
//─────────┼──────┼────────────────────────────────────────────────────────────────────────┬──────
// 26-07-03│ 1.00 │ Initiale Version: q9_name_parse, q9_name_cmp                           │ CF
// 26-07-03│ 1.10 │ Bugfix: E_DIFF($E2) -> E_DIFFER($A5), MWOS-verifiziert                │ CF
//═════════╧══════╧════════════════════════════════════════════════════════════════════════╧══════

#include "name.h"
#include "syscall.h"

//╔══════════════════════════════════════════════════════════════════════════════════════════════╗
//║ API                                                                                          ║
//╚══════════════════════════════════════════════════════════════════════════════════════════════╝

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_name_is_char
// Desc.:    1, wenn c ein gültiges OS-9-Namenszeichen ist (A-Z a-z 0-9 _ . $).
// Call:     if (q9_name_is_char(c)) ...
//════════════════════════════════════════════════════════════════════════════════════════════════
int q9_name_is_char(char c)
{
    return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
           (c >= '0' && c <= '9') || c == '_' || c == '.' || c == '$';
}

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_name_parse
// Desc.:    Parst das nächste Pathlist-Element: überspringt einen führenden '/', liefert
//           Startadresse + Länge. E$BPNam, wenn an der Position kein gültiger Name beginnt.
// Call:     err = q9_name_parse(pathlist, &start, &len)
//════════════════════════════════════════════════════════════════════════════════════════════════
int q9_name_parse(const char *pathlist, const char **start, uint32_t *len)
{
    const char *p = pathlist;
    uint32_t    n = 0;

    if (*p == '/') {
        p++;
    }
    while (q9_name_is_char(p[n])) {
        n++;
    }
    if (n == 0) {
        return E_BPNAM;                                /* empty element or invalid char          */
    }
    *start = p;
    *len   = n;
    return 0;
}

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_name_cmp
// Desc.:    Vergleicht zwei Namen fester Länge case-insensitiv. 0 = gleich, sonst E$Diff.
// Call:     err = q9_name_cmp("TERM", 4, "term")
//════════════════════════════════════════════════════════════════════════════════════════════════
int q9_name_cmp(const char *a, uint32_t len, const char *b)
{
    if (len == 0) {
        return E_DIFFER;
    }
    for (uint32_t i = 0; i < len; i++) {
        char ca = a[i];
        char cb = b[i];
        if (ca >= 'a' && ca <= 'z') {
            ca -= 0x20;
        }
        if (cb >= 'a' && cb <= 'z') {
            cb -= 0x20;
        }
        if (ca != cb) {
            return E_DIFFER;
        }
    }
    return 0;
}

//────────────────────────────────────────────────────────────────────────────────────────────────
// EOF name.c                                                                              Ver. 1.10
//────────────────────────────────────────────────────────────────────────────────────────────────
