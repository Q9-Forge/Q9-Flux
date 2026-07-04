//════════════════════════════════════════════════════════════════════════════════════════════════
// File:   q9copy_main.c                                                                   Ver. 1.00
// Owner:  AF
// Desc.:  argc/argv-Einsprungpunkt fuer q9copy als Vorbereitung auf spaetere 68k-Module.
//         Kein Host-main(); der native Testharness ruft q9copy_main() direkt auf.
//
// Call:   err = q9copy_main(argc, argv)
//
// Edition History
//─────────┬──────┬────────────────────────────────────────────────────────────────────────┬──────
// Date    │ Ver. │ Description                                                            │ By
//─────────┼──────┼────────────────────────────────────────────────────────────────────────┼──────
// 26-07-04│ 1.00 │ Initiale argc/argv-Huelle um q9copy_run mit -v                          │ CX
//═════════╧══════╧════════════════════════════════════════════════════════════════════════╧══════

#include <string.h>

#include "../lib/libq9.h"
#include "q9copy.h"

#define Q9COPY_ARGERR 1

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

static int write_line(const char *s)
{
    return write_part(s, (uint32_t)strlen(s));
}

static int is_help(const char *s)
{
    return strcmp(s, "-h") == 0 || strcmp(s, "--help") == 0;
}

static int is_verbose(const char *s)
{
    return strcmp(s, "-v") == 0 || strcmp(s, "--verbose") == 0;
}

static int usage(void)
{
    int err = write_line("usage: q9copy [-v] <src> <dst>\n");

    return err == 0 ? Q9COPY_ARGERR : err;
}

static int help(void)
{
    int err = write_line("q9copy: Datei innerhalb von Q9 kopieren\n");

    return err != 0 ? err : write_line("usage: q9copy [-v] <src> <dst>\n");
}

static int write_verbose(const char *src, const char *dst)
{
    int err;

    err = write_part("copy: ", 6u);
    if (err != 0) {
        return err;
    }
    err = write_part(src, (uint32_t)strlen(src));
    if (err != 0) {
        return err;
    }
    err = write_part(" -> ", 4u);
    if (err != 0) {
        return err;
    }
    err = write_part(dst, (uint32_t)strlen(dst));
    if (err != 0) {
        return err;
    }
    return write_part("\n", 1u);
}

int q9copy_main(int argc, char **argv)
{
    const char *src = 0;
    const char *dst = 0;
    int         verbose = 0;

    for (int i = 1; i < argc; i++) {
        if (is_help(argv[i])) {
            return help();
        }
        if (is_verbose(argv[i])) {
            verbose = 1;
            continue;
        }
        if (!src) {
            src = argv[i];
        } else if (!dst) {
            dst = argv[i];
        } else {
            return usage();
        }
    }
    if (!src || !dst) {
        return usage();
    }
    if (verbose) {
        int err = write_verbose(src, dst);
        if (err != 0) {
            return err;
        }
    }
    return q9copy_run(src, dst);
}

//────────────────────────────────────────────────────────────────────────────────────────────────
// EOF q9copy_main.c                                                                      Ver. 1.00
//────────────────────────────────────────────────────────────────────────────────────────────────
