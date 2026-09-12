//════════════════════════════════════════════════════════════════════════════════════════════════
// File:   q9rm_main.c                                                                     Ver. 1.00
// Owner:  AF
// Desc.:  argc/argv-Einsprungpunkt fuer q9rm als Vorbereitung auf spaetere 68k-Module.
//         Kein Host-main(); der native Testharness ruft q9rm_main() direkt auf.
//
// Call:   err = q9rm_main(argc, argv)
//
// Edition History
//─────────┬──────┬────────────────────────────────────────────────────────────────────────┬──────
// Date    │ Ver. │ Description                                                            │ By
//─────────┼──────┼────────────────────────────────────────────────────────────────────────┼──────
// 26-07-04│ 1.00 │ Initiale argc/argv-Huelle um q9rm_run                                   │ CX
//═════════╧══════╧════════════════════════════════════════════════════════════════════════╧══════

#include <string.h>

#include "../lib/libq9.h"
#include "q9rm.h"

#define Q9RM_ARGERR 1

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
    int err = write_line("usage: q9rm <path>\n");

    return err == 0 ? Q9RM_ARGERR : err;
}

static int help(void)
{
    int err = write_line("q9rm: Datei loeschen\n");

    return err != 0 ? err : write_line("usage: q9rm <path>\n");
}

int q9rm_main(int argc, char **argv)
{
    if (argc == 2 && is_help(argv[1])) {
        return help();
    }
    if (argc != 2) {
        return usage();
    }
    return q9rm_run(argv[1]);
}

//────────────────────────────────────────────────────────────────────────────────────────────────
// EOF q9rm_main.c                                                                        Ver. 1.00
//────────────────────────────────────────────────────────────────────────────────────────────────
