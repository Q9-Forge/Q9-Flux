//════════════════════════════════════════════════════════════════════════════════════════════════
// File:   harness.c                                                                       Ver. 1.00
// Owner:  AF
// Desc.:  Nativer Userland-Testharness fuer libq9 und die Beispiel-Tools. Baut ein minimales
//         FAT16-Testimage, startet HAL+Kernel und testet danach Userland-Code gegen echte
//         Kernel-Syscalls.
//
// Call:   userland/build.sh
//
// Edition History
//─────────┬──────┬────────────────────────────────────────────────────────────────────────┬──────
// Date    │ Ver. │ Description                                                            │ By
//─────────┼──────┼────────────────────────────────────────────────────────────────────────┼──────
// 26-07-04│ 1.00 │ Initiale Version                                                        │ CX
// 26-07-04│ 1.01 │ q9dir-Test mit stdout-Capture fuer Pfad 1 ergaenzt                      │ CX
// 26-07-04│ 1.02 │ q9mkdir/q9rm-Tests ergaenzt                                             │ CX
// 26-07-04│ 1.03 │ q9touch/q9stat-Tests ergaenzt und Capture-Helfer konsolidiert           │ CX
// 26-07-04│ 1.04 │ Tests fuer synthetische argc/argv-Einsprungpunkte ergaenzt              │ CX
//═════════╧══════╧════════════════════════════════════════════════════════════════════════╧══════

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>

#include "../../src/hal/q9_hal.h"
#include "../../src/kernel/kernel.h"
#include "../lib/libq9.h"
#include "../tools/q9cat.h"
#include "../tools/q9copy.h"
#include "../tools/q9dir.h"
#include "../tools/q9mkdir.h"
#include "../tools/q9rm.h"
#include "../tools/q9touch.h"
#include "../tools/q9stat.h"

#define IMG_NAME       "local_images/q9disk.img"
#define SECTOR         512u
#define SEC_PER_CLUS   1u
#define RESERVED_SECS  1u
#define NUM_FATS       2u
#define ROOT_ENT_CNT   32u
#define FAT_SECTORS    1u
#define TOTAL_CLUSTERS 16u
#define ROOT_SECTORS   ((ROOT_ENT_CNT * 32u) / SECTOR)
#define TOTAL_SECTORS  (RESERVED_SECS + NUM_FATS * FAT_SECTORS + ROOT_SECTORS + TOTAL_CLUSTERS)

static void put16(uint8_t *p, uint16_t v)
{
    p[0] = (uint8_t)v;
    p[1] = (uint8_t)(v >> 8);
}

static void put32(uint8_t *p, uint32_t v)
{
    p[0] = (uint8_t)v;
    p[1] = (uint8_t)(v >> 8);
    p[2] = (uint8_t)(v >> 16);
    p[3] = (uint8_t)(v >> 24);
}

static void make_boot(uint8_t *b)
{
    memset(b, 0, SECTOR);
    b[0] = 0xEB;
    b[1] = 0x3C;
    b[2] = 0x90;
    memcpy(&b[3], "Q9ULAND1", 8);
    put16(&b[0x0B], SECTOR);
    b[0x0D] = SEC_PER_CLUS;
    put16(&b[0x0E], RESERVED_SECS);
    b[0x10] = NUM_FATS;
    put16(&b[0x11], ROOT_ENT_CNT);
    put16(&b[0x13], TOTAL_SECTORS);
    b[0x15] = 0xF8;
    put16(&b[0x16], FAT_SECTORS);
    b[0x26] = 0x29;
    put32(&b[0x27], 0x51554C39u);
    memcpy(&b[0x2B], "Q9USERLAND ", 11);
    memcpy(&b[0x36], "FAT16   ", 8);
    put16(&b[0x1FE], 0xAA55u);
}

static int make_image(void)
{
    FILE    *f;
    uint8_t  sec[SECTOR];
    uint8_t  fat[SECTOR];
    uint32_t i;

    f = fopen(IMG_NAME, "wb");
    if (!f) {
        return 0;
    }

    /* Minimal-FAT16: genau gross genug fuer Root-Tests, aber mit gueltigem Bootsektor. */
    make_boot(sec);
    if (fwrite(sec, SECTOR, 1, f) != 1) {
        fclose(f);
        return 0;
    }

    memset(fat, 0, sizeof(fat));
    put16(&fat[0], 0xFFF8u);
    put16(&fat[2], 0xFFFFu);
    if (fwrite(fat, SECTOR, 1, f) != 1 || fwrite(fat, SECTOR, 1, f) != 1) {
        fclose(f);
        return 0;
    }

    memset(sec, 0, sizeof(sec));
    /* Root-Directory und Datenbereich bleiben leer; die Tests erzeugen alle Nutzdaten via Q9. */
    for (i = 0; i < ROOT_SECTORS; i++) {
        if (fwrite(sec, SECTOR, 1, f) != 1) {
            fclose(f);
            return 0;
        }
    }
    for (i = 0; i < TOTAL_CLUSTERS; i++) {
        if (fwrite(sec, SECTOR, 1, f) != 1) {
            fclose(f);
            return 0;
        }
    }
    fclose(f);
    return 1;
}

static int expect_ok(const char *name, int err)
{
    if (err == 0) {
        printf("  [ok] %s\n", name);
        return 1;
    }
    printf("  [FEHLER] %s -> E$%02X\n", name, err);
    return 0;
}

static int write_file(const char *path, const uint8_t *data, uint32_t len)
{
    uint16_t p;
    uint32_t done;
    int      err;

    err = q9_create(path, Q9_MODE_WRITE, &p);
    if (err != 0) {
        return err;
    }
    err = q9_write(p, data, len, &done);
    if (err == 0 && done != len) {
        err = E_NOTRDY;
    }
    {
        int cerr = q9_close(p);
        return err != 0 ? err : cerr;
    }
}

static int read_file(const char *path, uint8_t *buf, uint32_t maxlen, uint32_t *out_len)
{
    uint16_t p;
    uint32_t total = 0;
    int      err;

    err = q9_open(path, Q9_MODE_READ, &p);
    if (err != 0) {
        return err;
    }
    while (total < maxlen) {
        uint32_t got = 0;

        err = q9_read(p, &buf[total], maxlen - total, &got);
        if (err == E_EOF) {
            err = 0;
            break;
        }
        if (err != 0) {
            break;
        }
        total += got;
        if (got == 0) {
            break;
        }
    }
    if (out_len) {
        *out_len = total;
    }
    {
        int cerr = q9_close(p);
        return err != 0 ? err : cerr;
    }
}

typedef int (*capture_fn_t)(void *ctx);

static int capture_output(capture_fn_t fn, void *ctx, const char *out_file)
{
    int   saved_stdout;
    FILE *f;
    int   err;

    fflush(stdout);
    saved_stdout = dup(STDOUT_FILENO);
    if (saved_stdout < 0) {
        return E_NOTRDY;
    }
    /* Die Tools schreiben absichtlich auf Pfad 1; fuer Tests wird stdout temporaer umgebogen. */
    f = fopen(out_file, "wb");
    if (!f) {
        close(saved_stdout);
        return E_NOTRDY;
    }
    if (dup2(fileno(f), STDOUT_FILENO) < 0) {
        fclose(f);
        close(saved_stdout);
        return E_NOTRDY;
    }

    err = fn(ctx);

    fflush(stdout);
    if (dup2(saved_stdout, STDOUT_FILENO) < 0 && err == 0) {
        err = E_NOTRDY;
    }
    close(saved_stdout);
    if (fclose(f) != 0 && err == 0) {
        err = E_NOTRDY;
    }
    return err;
}

static int run_q9dir_ctx(void *ctx)
{
    return q9dir_run((const char *)ctx);
}

typedef struct stat_ctx {
    const char *dir_path;
    const char *name;
} stat_ctx_t;

typedef int (*tool_main_fn_t)(int argc, char **argv);

typedef struct tool_main_ctx {
    tool_main_fn_t fn;
    int            argc;
    char         **argv;
} tool_main_ctx_t;

static int run_q9stat_ctx(void *ctx)
{
    const stat_ctx_t *st = (const stat_ctx_t *)ctx;

    return q9stat_run(st->dir_path, st->name);
}

static int run_tool_main_ctx(void *ctx)
{
    tool_main_ctx_t *tm = (tool_main_ctx_t *)ctx;

    return tm->fn(tm->argc, tm->argv);
}

static int capture_q9dir(const char *path, const char *out_file)
{
    return capture_output(run_q9dir_ctx, (void *)path, out_file);
}

static int capture_q9stat(const char *dir_path, const char *name, const char *out_file)
{
    stat_ctx_t st;

    st.dir_path = dir_path;
    st.name = name;
    return capture_output(run_q9stat_ctx, &st, out_file);
}

static int capture_tool_main(tool_main_fn_t fn, int argc, char **argv, const char *out_file)
{
    tool_main_ctx_t tm;

    tm.fn = fn;
    tm.argc = argc;
    tm.argv = argv;
    return capture_output(run_tool_main_ctx, &tm, out_file);
}

static int file_contains(const char *file, const char *needle)
{
    FILE  *f;
    char   buf[1024];
    size_t n;

    f = fopen(file, "rb");
    if (!f) {
        return 0;
    }
    n = fread(buf, 1, sizeof(buf) - 1u, f);
    fclose(f);
    buf[n] = 0;
    return strstr(buf, needle) != 0;
}

static int expect_contains(const char *name, const char *file, const char *needle)
{
    if (file_contains(file, needle)) {
        printf("  [ok] %s\n", name);
        return 1;
    }
    printf("  [FEHLER] %s -> '%s' nicht gefunden\n", name, needle);
    return 0;
}

static int expect_not_contains(const char *name, const char *file, const char *needle)
{
    if (!file_contains(file, needle)) {
        printf("  [ok] %s\n", name);
        return 1;
    }
    printf("  [FEHLER] %s -> '%s' unerwartet gefunden\n", name, needle);
    return 0;
}

static int file_is_empty(const char *file)
{
    FILE *f;
    int   c;

    f = fopen(file, "rb");
    if (!f) {
        return 0;
    }
    c = fgetc(f);
    fclose(f);
    return c == EOF;
}

static int expect_empty_output(const char *name, const char *file)
{
    if (file_is_empty(file)) {
        printf("  [ok] %s\n", name);
        return 1;
    }
    printf("  [FEHLER] %s -> Ausgabe war nicht leer\n", name);
    return 0;
}

static int expect_error(const char *name, int err)
{
    if (err != 0) {
        printf("  [ok] %s -> E$%02X\n", name, err);
        return 1;
    }
    printf("  [FEHLER] %s -> unerwartet erfolgreich\n", name);
    return 0;
}

static int expect_errno(const char *name, int err, int want)
{
    if (err == want) {
        printf("  [ok] %s -> E$%02X\n", name, err);
        return 1;
    }
    printf("  [FEHLER] %s -> E$%02X statt E$%02X\n", name, err, want);
    return 0;
}

int main(void)
{
    static const uint8_t src_data[] =
        "libq9 schreibt diese Datei.\n"
        "q9copy liest sie wieder.\n";
    static const uint8_t one_data[] = "eins\n";
    static const uint8_t two_data[] = "zwei\n";
    static const uint8_t bin_data[] = {0x51u, 0x39u, 0x00u};
    uint8_t   buf[128];
    uint32_t  got = 0;
    q9_time_t tm;
    uint16_t  pid = 0;
    uint32_t  uid = 0;
    q9_name_parse_t pn;
    int       ok = 1;
    int       err;
    int       q9cat_direct_err;
    int       q9copy_direct_err;
    int       q9mkdir_direct_err;

    remove(IMG_NAME);
    if (!make_image()) {
        printf("userland_test: FAIL (FAT16-Image konnte nicht gebaut werden)\n");
        return 1;
    }

    /* Das Image muss vor q9_kernel_init() existieren, weil q9_dev_init() dort /d0 mountet. */
    q9_hal_init();
    q9_kernel_init();

    /* Zuerst die nackten libq9-F$-Wrapper pruefen, bevor Tools auf deren Verhalten aufbauen. */
    err = q9_time(&tm);
    ok = expect_ok("libq9: F$Time", err) && ok;
    err = q9_id(&pid, &uid);
    ok = expect_ok("libq9: F$ID", err) && ok;
    err = q9_prsnam("/d0/SRC.TXT", &pn);
    ok = expect_ok("libq9: F$PrsNam", err) && ok;
    err = q9_cmpnam(pn.name, "D0", pn.len);
    ok = expect_ok("libq9: F$CmpNam", err) && ok;

    err = write_file("/d0/SRC.TXT", src_data, (uint32_t)(sizeof(src_data) - 1u));
    ok = expect_ok("libq9: I$Create/I$Write /d0/SRC.TXT", err) && ok;

    /* q9cat schreibt direkt auf stdout; hier ist die sichtbare Ausgabe Teil des Smoke-Tests. */
    printf("  [info] q9cat-Ausgabe beginnt:\n");
    err = q9cat_run("/d0/SRC.TXT");
    q9cat_direct_err = err;
    printf("\n  [info] q9cat-Ausgabe endet\n");
    ok = expect_ok("q9cat_run: Datei nach Pfad 1 schreiben", err) && ok;
    {
        char *argv[] = {"q9cat", "/d0/SRC.TXT", 0};

        err = capture_tool_main(q9cat_main, 2, argv, "q9cat-main.out");
        ok = expect_ok("q9cat_main: argc/argv ruft q9cat_run", err) && ok;
        if (err == q9cat_direct_err) {
            printf("  [ok] q9cat_main: Status deckt sich mit q9cat_run\n");
        } else {
            printf("  [FEHLER] q9cat_main: Status %d statt %d\n", err, q9cat_direct_err);
            ok = 0;
        }
        ok = expect_contains("q9cat_main: Ausgabe enthaelt Dateitext",
                             "q9cat-main.out", "libq9 schreibt diese Datei.") && ok;
    }
    {
        char *argv[] = {"q9cat", 0};

        err = capture_tool_main(q9cat_main, 1, argv, "q9cat-usage.out");
        ok = expect_error("q9cat_main: fehlendes Argument liefert Parser-Fehler", err) && ok;
        ok = expect_contains("q9cat_main: Usage bei fehlendem Argument",
                             "q9cat-usage.out", "usage: q9cat <path>") && ok;
    }

    err = q9copy_run("/d0/SRC.TXT", "/d0/DST.TXT");
    q9copy_direct_err = err;
    ok = expect_ok("q9copy_run: /d0/SRC.TXT -> /d0/DST.TXT", err) && ok;

    err = read_file("/d0/DST.TXT", buf, sizeof(buf), &got);
    ok = expect_ok("libq9: Kopie zuruecklesen", err) && ok;
    if (err == 0 && got == sizeof(src_data) - 1u &&
        memcmp(buf, src_data, sizeof(src_data) - 1u) == 0) {
        printf("  [ok] q9copy: Inhalt identisch\n");
    } else {
        printf("  [FEHLER] q9copy: Inhalt abweichend (got=%u)\n", got);
        ok = 0;
    }
    {
        char *argv[] = {"q9copy", "/d0/SRC.TXT", "/d0/MAINCPY.TXT", 0};

        err = capture_tool_main(q9copy_main, 3, argv, "q9copy-main.out");
        ok = expect_ok("q9copy_main: argc/argv ruft q9copy_run", err) && ok;
        if (err == q9copy_direct_err) {
            printf("  [ok] q9copy_main: Status deckt sich mit q9copy_run\n");
        } else {
            printf("  [FEHLER] q9copy_main: Status %d statt %d\n", err, q9copy_direct_err);
            ok = 0;
        }
        err = read_file("/d0/MAINCPY.TXT", buf, sizeof(buf), &got);
        ok = expect_ok("q9copy_main: Kopie zuruecklesen", err) && ok;
        if (err == 0 && got == sizeof(src_data) - 1u &&
            memcmp(buf, src_data, sizeof(src_data) - 1u) == 0) {
            printf("  [ok] q9copy_main: Inhalt identisch\n");
        } else {
            printf("  [FEHLER] q9copy_main: Inhalt abweichend (got=%u)\n", got);
            ok = 0;
        }
    }
    {
        char *argv[] = {"q9copy", "--help", 0};

        err = capture_tool_main(q9copy_main, 2, argv, "q9copy-help.out");
        ok = expect_ok("q9copy_main: --help liefert Erfolg", err) && ok;
        ok = expect_contains("q9copy_main: Help enthaelt Usage",
                             "q9copy-help.out", "usage: q9copy [-v] <src> <dst>") && ok;
    }
    {
        char *argv[] = {"q9copy", "-v", "/d0/SRC.TXT", "/d0/VERBOSE.TXT", 0};

        err = capture_tool_main(q9copy_main, 4, argv, "q9copy-verbose.out");
        ok = expect_ok("q9copy_main: -v kopiert Datei", err) && ok;
        ok = expect_contains("q9copy_main: -v schreibt Kopierzeile",
                             "q9copy-verbose.out",
                             "copy: /d0/SRC.TXT -> /d0/VERBOSE.TXT") && ok;
    }

    err = q9_makdir("/d0/TESTDIR");
    ok = expect_ok("libq9: I$MakDir /d0/TESTDIR", err) && ok;
    err = write_file("/d0/TESTDIR/ONE.TXT", one_data, (uint32_t)(sizeof(one_data) - 1u));
    ok = expect_ok("libq9: Datei /d0/TESTDIR/ONE.TXT anlegen", err) && ok;
    err = write_file("/d0/TESTDIR/TWO.TXT", two_data, (uint32_t)(sizeof(two_data) - 1u));
    ok = expect_ok("libq9: Datei /d0/TESTDIR/TWO.TXT anlegen", err) && ok;
    err = write_file("/d0/TESTDIR/RAW.BIN", bin_data, (uint32_t)sizeof(bin_data));
    ok = expect_ok("libq9: Datei /d0/TESTDIR/RAW.BIN anlegen", err) && ok;

    /* Reihenfolge: erst erzeugen, dann Directory-Capture, damit q9dir den neuen Eintrag beweist. */
    err = q9mkdir_run("/d0/MKDIR");
    q9mkdir_direct_err = err;
    ok = expect_ok("q9mkdir_run: /d0/MKDIR anlegen", err) && ok;
    {
        char *argv[] = {"q9mkdir", "/d0/MKMAIN", 0};

        err = capture_tool_main(q9mkdir_main, 2, argv, "q9mkdir-main.out");
        ok = expect_ok("q9mkdir_main: argc/argv ruft q9mkdir_run", err) && ok;
        if (err == q9mkdir_direct_err) {
            printf("  [ok] q9mkdir_main: Status deckt sich mit q9mkdir_run\n");
        } else {
            printf("  [FEHLER] q9mkdir_main: Status %d statt %d\n", err, q9mkdir_direct_err);
            ok = 0;
        }
        ok = expect_contains("q9mkdir_main: Ausgabe bestaetigt Pfad",
                             "q9mkdir-main.out", "mkdir: /d0/MKMAIN") && ok;
    }
    err = capture_q9dir("/d0", "q9dir-after-mkdir.out");
    ok = expect_ok("q9dir_run: Root nach q9mkdir_run schreiben", err) && ok;
    ok = expect_contains("q9mkdir: Root zeigt MKDIR als DIR", "q9dir-after-mkdir.out",
                         "DIR 0 MKDIR") && ok;

    err = write_file("/d0/REMOVE.TXT", two_data, (uint32_t)(sizeof(two_data) - 1u));
    ok = expect_ok("libq9: Datei /d0/REMOVE.TXT anlegen", err) && ok;
    /* Vorher/Nachher-Capture prueft, dass q9rm wirklich den Directory-Eintrag entfernt. */
    err = capture_q9dir("/d0", "q9dir-before-rm.out");
    ok = expect_ok("q9dir_run: Root vor q9rm_run schreiben", err) && ok;
    ok = expect_contains("q9rm: Root zeigt REMOVE.TXT vor Delete", "q9dir-before-rm.out",
                         "FILE 5 REMOVE.TXT") && ok;
    err = q9rm_run("/d0/REMOVE.TXT");
    ok = expect_ok("q9rm_run: /d0/REMOVE.TXT loeschen", err) && ok;
    err = capture_q9dir("/d0", "q9dir-after-rm.out");
    ok = expect_ok("q9dir_run: Root nach q9rm_run schreiben", err) && ok;
    ok = expect_not_contains("q9rm: Root zeigt REMOVE.TXT nicht mehr", "q9dir-after-rm.out",
                             "REMOVE.TXT") && ok;
    err = q9rm_run("/d0/FEHLT.TXT");
    ok = expect_error("q9rm_run: fehlender Pfad liefert Fehler", err) && ok;

    err = q9touch_run("/d0/EMPTY.TXT");
    ok = expect_ok("q9touch_run: /d0/EMPTY.TXT anlegen", err) && ok;
    /* q9touch soll eine Null-Byte-Datei erzeugen und spaeter existierende Inhalte nicht kuerzen. */
    err = read_file("/d0/EMPTY.TXT", buf, sizeof(buf), &got);
    ok = expect_ok("libq9: /d0/EMPTY.TXT zuruecklesen", err) && ok;
    if (err == 0 && got == 0) {
        printf("  [ok] q9touch: Datei hat Groesse 0\n");
    } else {
        printf("  [FEHLER] q9touch: Datei nicht leer (got=%u)\n", got);
        ok = 0;
    }
    err = q9touch_run("/d0/EMPTY.TXT");
    ok = expect_ok("q9touch_run: existierende Datei bleibt ok", err) && ok;
    err = capture_q9dir("/d0", "q9dir-after-touch.out");
    ok = expect_ok("q9dir_run: Root nach q9touch_run schreiben", err) && ok;
    ok = expect_contains("q9touch: Root zeigt EMPTY.TXT mit Groesse 0",
                         "q9dir-after-touch.out", "FILE 0 EMPTY.TXT") && ok;

    /* q9stat nutzt q9dir_next intern; Treffer- und Fehlfall pruefen Suche und leere Ausgabe. */
    err = capture_q9stat("/d0", "EMPTY.TXT", "q9stat-empty.out");
    ok = expect_ok("q9stat_run: EMPTY.TXT finden", err) && ok;
    ok = expect_contains("q9stat: Treffer zeigt gleiche Zeile wie q9dir",
                         "q9stat-empty.out", "FILE 0 EMPTY.TXT") && ok;
    err = capture_q9stat("/d0", "FEHLT.TXT", "q9stat-missing.out");
    ok = expect_errno("q9stat_run: fehlender Name liefert E$PNNF", err, E_PNNF) && ok;
    ok = expect_empty_output("q9stat: Fehlfall schreibt nichts", "q9stat-missing.out") && ok;

    /* Zum Schluss das gemeinsame Directory-Parsing auf Root und Unterverzeichnis absichern. */
    err = capture_q9dir("/d0", "q9dir-root.out");
    ok = expect_ok("q9dir_run: Root-Directory nach Pfad 1 schreiben", err) && ok;
    err = capture_q9dir("/d0/TESTDIR", "q9dir-testdir.out");
    ok = expect_ok("q9dir_run: Unterverzeichnis nach Pfad 1 schreiben", err) && ok;
    ok = expect_contains("q9dir: Root zeigt TESTDIR als DIR", "q9dir-root.out",
                         "DIR 0 TESTDIR") && ok;
    ok = expect_contains("q9dir: Unterverzeichnis zeigt ONE.TXT", "q9dir-testdir.out",
                         "FILE 5 ONE.TXT") && ok;
    ok = expect_contains("q9dir: Unterverzeichnis zeigt TWO.TXT", "q9dir-testdir.out",
                         "FILE 5 TWO.TXT") && ok;
    ok = expect_contains("q9dir: Unterverzeichnis zeigt RAW.BIN", "q9dir-testdir.out",
                         "FILE 3 RAW.BIN") && ok;

    err = q9_delete("/d0/SRC.TXT");
    ok = expect_ok("libq9: I$Delete /d0/SRC.TXT", err) && ok;
    err = q9_delete("/d0/DST.TXT");
    ok = expect_ok("libq9: I$Delete /d0/DST.TXT", err) && ok;

    if (ok) {
        printf("userland_test: PASS\n");
        return 0;
    }
    printf("userland_test: FAIL\n");
    return 1;
}

//────────────────────────────────────────────────────────────────────────────────────────────────
// EOF harness.c                                                                          Ver. 1.00
//────────────────────────────────────────────────────────────────────────────────────────────────
