//════════════════════════════════════════════════════════════════════════════════════════════════
// File:   q9stat_main.c                                                                   Ver. 1.00
// Owner:  AF
// Desc.:  argc/argv-Einsprungpunkt fuer q9stat als Vorbereitung auf spaetere 68k-Module.
//         Kein Host-main(); der native Testharness ruft q9stat_main() direkt auf.
//
// Call:   err = q9stat_main(argc, argv)
//
// Edition History
//─────────┬──────┬────────────────────────────────────────────────────────────────────────┬──────
// Date    │ Ver. │ Description                                                            │ By
//─────────┼──────┼────────────────────────────────────────────────────────────────────────┼──────
// 26-07-04│ 1.00 │ Initiale argc/argv-Huelle um q9stat_run                                 │ CX
//═════════╧══════╧════════════════════════════════════════════════════════════════════════╧══════

#include <string.h>

#include "../lib/libq9.h"
#include "q9stat.h"

#define Q9STAT_ARGERR 1

static int write_line(const char *s)
{
    uint32_t len = (uint32_t)strlen(s);
    uint32_t put = 0;
    int      err;

    err = q9_writln(1, s, len, &put);
    if (err != 0) {
        return err;
    }
    return put == len ? 0 : E_NOTRDY;
}

static int is_help(const char *s)
{
    return strcmp(s, "-h") == 0 || strcmp(s, "--help") == 0;
}

static int usage(void)
{
    int err = write_line("usage: q9stat <dir> <name>\n");

    return err == 0 ? Q9STAT_ARGERR : err;
}

static int help(void)
{
    int err = write_line("q9stat: 8.3-Namen in einem Directory suchen\n");

    return err != 0 ? err : write_line("usage: q9stat <dir> <name>\n");
}

int q9stat_main(int argc, char **argv)
{
    if (argc == 2 && is_help(argv[1])) {
        return help();
    }
    if (argc != 3) {
        return usage();
    }
    return q9stat_run(argv[1], argv[2]);
}

//────────────────────────────────────────────────────────────────────────────────────────────────
// EOF q9stat_main.c                                                                      Ver. 1.00
//────────────────────────────────────────────────────────────────────────────────────────────────
