//════════════════════════════════════════════════════════════════════════════════════════════════
// File:   13_test_dhf.c                                                                   Ver. 1.00
// Owner:  Claude
// Desc.:  Rauchtest fuer das DHF-Geraet (src/devices/dhf/q9_dhf.c), analog test-cf-sector: faehrt
//         das MMIO-Registerprotokoll direkt gegen q9_devtype_dhf.read8/write8, OHNE 68k-CPU, OHNE
//         OS-9-Treiber -- die Frage ist ausschliesslich: kommen bei OPEN/WRITE/CLOSE/OPEN/READ/
//         MKDIR/OPENDIR/READDIR/RENAME/UNLINK/RMDIR dieselben Bytes/Ergebnisse zurueck, die ein
//         spaeter angeschlossener 68k-Treiber ueber genau dieses Registerfenster erwarten wuerde?
//
// Call:   make test-dhf   (baut+startet)
//
// Edition History
//─────────┬──────┬────────────────────────────────────────────────────────────────────────┬──────
// Date    │ Ver. │ Description                                                            │ By
//─────────┼──────┼────────────────────────────────────────────────────────────────────────┬──────
// 26-09-25│ 1.00 │ Initiale Version                                                        │ Cld
//═════════╧══════╧════════════════════════════════════════════════════════════════════════╧══════
#include "../src/devices/dhf/q9_dhf.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static int g_total_fail = 0;
static q9_device_t g_dev;

static void dhf_w8(uint32_t off, uint8_t v)  { g_dev.vt->write8(&g_dev, g_dev.base + off, v); }
static uint8_t dhf_r8(uint32_t off)          { return g_dev.vt->read8(&g_dev, g_dev.base + off); }

static void dhf_wstr(uint32_t off, const char *s)
{
    size_t i, n = strlen(s);
    for (i = 0; i <= n; i++) dhf_w8(off + (uint32_t)i, (uint8_t)s[i]); /* inkl. NUL */
}

static void dhf_w32(uint32_t off, uint32_t v)
{
    dhf_w8(off + 0, (uint8_t)(v >> 24));
    dhf_w8(off + 1, (uint8_t)(v >> 16));
    dhf_w8(off + 2, (uint8_t)(v >> 8));
    dhf_w8(off + 3, (uint8_t)(v));
}

static uint32_t dhf_r32(uint32_t off)
{
    return ((uint32_t)dhf_r8(off + 0) << 24) | ((uint32_t)dhf_r8(off + 1) << 16)
         | ((uint32_t)dhf_r8(off + 2) << 8)  | (uint32_t)dhf_r8(off + 3);
}

static int32_t dhf_cmd(uint8_t cmd)
{
    dhf_w8(Q9_DHF_OFF_CMD, cmd);
    return (int32_t)dhf_r32(Q9_DHF_OFF_RESULT);
}

static void check(const char *label, int ok)
{
    printf("%-55s %s\n", label, ok ? "OK" : "FAIL");
    if (!ok) g_total_fail++;
}

int main(void)
{
    char tmpl[] = "/tmp/q9dhf_test_XXXXXX";
    char *tmpdir = mkdtemp(tmpl);
    if (!tmpdir) { fprintf(stderr, "mkdtemp fehlgeschlagen\n"); return 2; }
    printf("Basepath: %s\n\n", tmpdir);

    static q9_dhf_t dhf;
    q9_dhf_init(&dhf, tmpdir);
    memset(&g_dev, 0, sizeof(g_dev));
    g_dev.type = "dhf"; g_dev.name = "dhf0";
    g_dev.base = 0xFFFF4000u; g_dev.size = Q9_DHF_WINDOW_SIZE;
    g_dev.vt = &q9_devtype_dhf; g_dev.state = &dhf;

    /* --- OPEN (create+trunc) "hello.txt", WRITE "Hello DHF!", CLOSE --- */
    dhf_wstr(Q9_DHF_OFF_PATH, "hello.txt");
    dhf_w32(Q9_DHF_OFF_ARG1, 0x4 | 0x8);           /* O_CREAT|O_TRUNC, s. dhf_open_mode() */
    dhf_w32(Q9_DHF_OFF_ARG2, 0644);
    int32_t r = dhf_cmd(Q9_DHF_CMD_OPEN);
    uint8_t h1 = dhf_r8(Q9_DHF_OFF_HANDLE);
    check("OPEN hello.txt (create+trunc)", r == 0);

    const char *payload = "Hello DHF!";
    size_t plen = strlen(payload);
    { size_t i; for (i = 0; i < plen; i++) dhf_w8(Q9_DHF_OFF_DATA + (uint32_t)i, (uint8_t)payload[i]); }
    dhf_w8(Q9_DHF_OFF_HANDLE, h1);
    dhf_w32(Q9_DHF_OFF_ARG1, (uint32_t)plen);
    r = dhf_cmd(Q9_DHF_CMD_WRITE);
    check("WRITE 10 Byte", r == (int32_t)plen);

    dhf_w8(Q9_DHF_OFF_HANDLE, h1);
    r = dhf_cmd(Q9_DHF_CMD_CLOSE);
    check("CLOSE", r == 0);

    /* --- OPEN (read-only), READ zurueck, vergleichen --- */
    dhf_wstr(Q9_DHF_OFF_PATH, "hello.txt");
    dhf_w32(Q9_DHF_OFF_ARG1, 0);                   /* O_RDONLY */
    dhf_w32(Q9_DHF_OFF_ARG2, 0);
    r = dhf_cmd(Q9_DHF_CMD_OPEN);
    uint8_t h2 = dhf_r8(Q9_DHF_OFF_HANDLE);
    check("OPEN hello.txt (rdonly)", r == 0);

    dhf_w8(Q9_DHF_OFF_HANDLE, h2);
    dhf_w32(Q9_DHF_OFF_ARG1, 64);
    r = dhf_cmd(Q9_DHF_CMD_READ);
    char back[64] = {0};
    { int32_t i; for (i = 0; i < r; i++) back[i] = (char)dhf_r8(Q9_DHF_OFF_DATA + (uint32_t)i); }
    check("READ liefert genau 10 Byte", r == (int32_t)plen);
    check("READ-Inhalt == geschriebener Inhalt", strncmp(back, payload, plen) == 0);

    dhf_w8(Q9_DHF_OFF_HANDLE, h2);
    r = dhf_cmd(Q9_DHF_CMD_CLOSE);
    check("CLOSE (2. Handle)", r == 0);

    /* --- SEEK-Roundtrip --- */
    dhf_wstr(Q9_DHF_OFF_PATH, "hello.txt");
    dhf_w32(Q9_DHF_OFF_ARG1, 0); dhf_w32(Q9_DHF_OFF_ARG2, 0);
    r = dhf_cmd(Q9_DHF_CMD_OPEN);
    uint8_t h3 = dhf_r8(Q9_DHF_OFF_HANDLE);
    dhf_w8(Q9_DHF_OFF_HANDLE, h3);
    dhf_w32(Q9_DHF_OFF_ARG1, 6); dhf_w32(Q9_DHF_OFF_ARG2, 0); /* SEEK_SET, offset 6 -> "DHF!" */
    r = dhf_cmd(Q9_DHF_CMD_SEEK);
    check("SEEK auf Offset 6", r == 6);
    dhf_w8(Q9_DHF_OFF_HANDLE, h3);
    dhf_w32(Q9_DHF_OFF_ARG1, 4);
    r = dhf_cmd(Q9_DHF_CMD_READ);
    memset(back, 0, sizeof(back));
    { int32_t i; for (i = 0; i < r; i++) back[i] = (char)dhf_r8(Q9_DHF_OFF_DATA + (uint32_t)i); }
    check("READ nach SEEK == \"DHF!\"", r == 4 && strncmp(back, "DHF!", 4) == 0);
    dhf_w8(Q9_DHF_OFF_HANDLE, h3);
    dhf_cmd(Q9_DHF_CMD_CLOSE);

    /* --- MKDIR + OPENDIR/READDIR --- */
    dhf_wstr(Q9_DHF_OFF_PATH, "subdir");
    dhf_w32(Q9_DHF_OFF_ARG1, 0755);
    r = dhf_cmd(Q9_DHF_CMD_MKDIR);
    check("MKDIR subdir", r == 0);

    dhf_wstr(Q9_DHF_OFF_PATH, ".");
    r = dhf_cmd(Q9_DHF_CMD_OPENDIR);
    uint8_t hd = dhf_r8(Q9_DHF_OFF_HANDLE);
    check("OPENDIR .", r == 0);

    int saw_hello = 0, saw_subdir = 0, n = 0;
    for (;;) {
        dhf_w8(Q9_DHF_OFF_HANDLE, hd);
        r = dhf_cmd(Q9_DHF_CMD_READDIR);
        if (r != 1) break;
        char name[Q9_DHF_DATA_SIZE];
        strncpy(name, (char *)&dhf.regs[Q9_DHF_OFF_DATA], sizeof(name) - 1);
        name[sizeof(name) - 1] = 0;
        if (strcmp(name, "hello.txt") == 0) saw_hello = 1;
        if (strcmp(name, "subdir") == 0) saw_subdir = 1;
        n++;
        if (n > 100) { fprintf(stderr, "READDIR-Endlosschleife?\n"); break; }
    }
    check("READDIR beendet mit RESULT=0", r == 0);
    check("READDIR listet hello.txt", saw_hello);
    check("READDIR listet subdir", saw_subdir);

    dhf_w8(Q9_DHF_OFF_HANDLE, hd);
    r = dhf_cmd(Q9_DHF_CMD_CLOSEDIR);
    check("CLOSEDIR", r == 0);

    /* --- RENAME + UNLINK + RMDIR --- */
    dhf_wstr(Q9_DHF_OFF_PATH, "hello.txt");
    dhf_wstr(Q9_DHF_OFF_PATH2, "hello2.txt");
    r = dhf_cmd(Q9_DHF_CMD_RENAME);
    check("RENAME hello.txt -> hello2.txt", r == 0);

    dhf_wstr(Q9_DHF_OFF_PATH, "hello2.txt");
    r = dhf_cmd(Q9_DHF_CMD_UNLINK);
    check("UNLINK hello2.txt", r == 0);

    dhf_wstr(Q9_DHF_OFF_PATH, "subdir");
    r = dhf_cmd(Q9_DHF_CMD_RMDIR);
    check("RMDIR subdir", r == 0);

    /* --- Confinement: ".." muss abgelehnt werden --- */
    dhf_wstr(Q9_DHF_OFF_PATH, "../escape.txt");
    dhf_w32(Q9_DHF_OFF_ARG1, 0x4 | 0x8); dhf_w32(Q9_DHF_OFF_ARG2, 0644);
    r = dhf_cmd(Q9_DHF_CMD_OPEN);
    check("OPEN mit \"..\" wird abgelehnt", r < 0);

    printf("\n%s\n", g_total_fail == 0 ? "ALLE TESTS BESTANDEN" : "TESTS FEHLGESCHLAGEN");

    /* Aufraeumen */
    char cmd[600];
    snprintf(cmd, sizeof(cmd), "rm -rf '%s'", tmpdir);
    if (system(cmd) != 0) { /* best effort */ }

    return g_total_fail == 0 ? 0 : 1;
}
//────────────────────────────────────────────────────────────────────────────────────────────────
// EOF 13_test_dhf.c                                                                       Ver. 1.00
//────────────────────────────────────────────────────────────────────────────────────────────────
