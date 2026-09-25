//════════════════════════════════════════════════════════════════════════════════════════════════
// File:   13_test_dhf.c                                                                   Ver. 2.00
// Owner:  Claude
// Desc.:  Rauchtest fuer das DHF-Geraet (src/devices/dhf/q9_dhf.c + dhf_emu_device.c), analog
//         test-cf-sector: faehrt das MMIO-Registerprotokoll (dhf_shared, big-endian, A0/A1 als
//         Gast-RAM-Zeiger fuer Pfad/Daten) direkt gegen q9_devtype_dhf.read8/write8, OHNE 68k-CPU,
//         OHNE OS-9-Treiber -- simuliert Gast-RAM als schlichtes Array, genau wie es der spaetere
//         echte 68k-Treiber (Q9-OS/Q9-DHFDRV-68k/driver/dhfdrv.c) tun wuerde (A0/A1 zeigen auf vom
//         Treiber bereitgestellte Puffer im Gast-RAM, s. dessen README "Zero-Copy Pointer-Modell").
//
// Call:   make test-dhf   (baut+startet)
//
// Edition History
//─────────┬──────┬────────────────────────────────────────────────────────────────────────┬──────
// Date    │ Ver. │ Description                                                            │ By
//─────────┼──────┼────────────────────────────────────────────────────────────────────────┬──────
// 26-09-25│ 1.00 │ Initiale Version (eigenes Byte-Kopier-Protokoll)                        │ Cld
// 26-09-25│ 2.00 │ Umgestellt auf das Q9-DHFDRV-68k-Protokoll (dhf_shared/A0-A1/D0-D2)      │ Cld
//═════════╧══════╧════════════════════════════════════════════════════════════════════════╧══════
#include "../src/devices/dhf/q9_dhf.h"
#include "../src/devices/dhf/dhf_proto.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <unistd.h>
#include <arpa/inet.h>

static int g_total_fail = 0;
static q9_device_t g_dev;
static uint8_t g_ram[65536];
#define GUEST_PATH_ADDR  0x1000u
#define GUEST_PATH2_ADDR 0x1200u
#define GUEST_DATA_ADDR  0x2000u

static void dev_w8(uint32_t off, uint8_t v)  { g_dev.vt->write8(&g_dev, g_dev.base + off, v); }
static uint8_t dev_r8(uint32_t off)          { return g_dev.vt->read8(&g_dev, g_dev.base + off); }

static void put_str(uint32_t guest_addr, const char *s)
{
    strcpy((char *)&g_ram[guest_addr], s);
}

static void set_a0(uint32_t v) { uint32_t be = htonl(v); size_t o = offsetof(struct dhf_shared, a0); for (int i = 0; i < 4; i++) dev_w8((uint32_t)o + i, ((uint8_t*)&be)[i]); }
static void set_a1(uint32_t v) { uint32_t be = htonl(v); size_t o = offsetof(struct dhf_shared, a1); for (int i = 0; i < 4; i++) dev_w8((uint32_t)o + i, ((uint8_t*)&be)[i]); }
static void set_d0(uint32_t v) { uint32_t be = htonl(v); size_t o = offsetof(struct dhf_shared, d0); for (int i = 0; i < 4; i++) dev_w8((uint32_t)o + i, ((uint8_t*)&be)[i]); }
static void set_d1(uint32_t v) { uint32_t be = htonl(v); size_t o = offsetof(struct dhf_shared, d1); for (int i = 0; i < 4; i++) dev_w8((uint32_t)o + i, ((uint8_t*)&be)[i]); }
static void set_d2(uint32_t v) { uint32_t be = htonl(v); size_t o = offsetof(struct dhf_shared, d2); for (int i = 0; i < 4; i++) dev_w8((uint32_t)o + i, ((uint8_t*)&be)[i]); }

static uint32_t get_u32(size_t off)
{
    uint8_t b[4];
    int i;
    for (i = 0; i < 4; i++) b[i] = dev_r8((uint32_t)off + (uint32_t)i);
    uint32_t be; memcpy(&be, b, 4);
    return ntohl(be);
}
static uint32_t get_d0(void) { return get_u32(offsetof(struct dhf_shared, d0)); }
static uint32_t get_d1(void) { return get_u32(offsetof(struct dhf_shared, d1)); }
static uint8_t  get_status(void) { return dev_r8((uint32_t)offsetof(struct dhf_shared, status)); }

static void run_cmd(uint8_t cmd)
{
    dev_w8((uint32_t)offsetof(struct dhf_shared, command), cmd);
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
    q9_dhf_init(&dhf, tmpdir, g_ram, sizeof(g_ram));
    memset(&g_dev, 0, sizeof(g_dev));
    g_dev.type = "dhf"; g_dev.name = "dhf0";
    g_dev.base = Q9_BOARD_DHF_BASE; g_dev.size = (uint32_t)sizeof(struct dhf_shared);
    g_dev.vt = &q9_devtype_dhf; g_dev.state = &dhf;

    /* --- CREATE hello.txt, WRITE "Hello DHF!", CLOSE --- */
    put_str(GUEST_PATH_ADDR, "hello.txt");
    set_a0(GUEST_PATH_ADDR);
    set_d1(0644); set_d2(DHF_MODE_WRITE);
    run_cmd(DHF_CMD_CREATE);
    check("CREATE hello.txt", get_status() == DHF_ERR_OK);
    uint32_t h1 = get_d0();

    const char *payload = "Hello DHF!";
    size_t plen = strlen(payload);
    memcpy(&g_ram[GUEST_DATA_ADDR], payload, plen);
    set_d0(h1); set_a1(GUEST_DATA_ADDR); set_d1((uint32_t)plen);
    run_cmd(DHF_CMD_WRITE);
    check("WRITE 10 Byte", get_status() == DHF_ERR_OK && get_d1() == plen);

    set_d0(h1);
    run_cmd(DHF_CMD_CLOSE);
    check("CLOSE", get_status() == DHF_ERR_OK);

    /* --- OPEN (read), READ zurueck, vergleichen --- */
    put_str(GUEST_PATH_ADDR, "hello.txt");
    set_a0(GUEST_PATH_ADDR); set_d2(DHF_MODE_READ);
    run_cmd(DHF_CMD_OPEN);
    check("OPEN hello.txt", get_status() == DHF_ERR_OK);
    uint32_t h2 = get_d0();

    set_d0(h2); set_a1(GUEST_DATA_ADDR + 4096); set_d1(64);
    run_cmd(DHF_CMD_READ);
    uint32_t nread = get_d1();
    check("READ liefert genau 10 Byte", get_status() == DHF_ERR_OK && nread == plen);
    check("READ-Inhalt == geschriebener Inhalt",
          memcmp(&g_ram[GUEST_DATA_ADDR + 4096], payload, plen) == 0);

    set_d0(h2);
    run_cmd(DHF_CMD_CLOSE);

    /* --- SEEK-Roundtrip --- */
    put_str(GUEST_PATH_ADDR, "hello.txt");
    set_a0(GUEST_PATH_ADDR); set_d2(DHF_MODE_READ);
    run_cmd(DHF_CMD_OPEN);
    uint32_t h3 = get_d0();
    set_d0(h3); set_d1(6); set_d2(DHF_SEEK_SET);
    run_cmd(DHF_CMD_SEEK);
    check("SEEK auf Offset 6", get_status() == DHF_ERR_OK && get_d1() == 6);
    set_d0(h3); set_a1(GUEST_DATA_ADDR + 4096); set_d1(4);
    run_cmd(DHF_CMD_READ);
    check("READ nach SEEK == \"DHF!\"",
          get_d1() == 4 && memcmp(&g_ram[GUEST_DATA_ADDR + 4096], "DHF!", 4) == 0);
    set_d0(h3);
    run_cmd(DHF_CMD_CLOSE);

    /* --- MKDIR + OPENDIR/READDIR --- */
    put_str(GUEST_PATH_ADDR, "subdir");
    set_a0(GUEST_PATH_ADDR); set_d1(0755);
    run_cmd(DHF_CMD_MKDIR);
    check("MKDIR subdir", get_status() == DHF_ERR_OK);

    put_str(GUEST_PATH_ADDR, ".");
    set_a0(GUEST_PATH_ADDR);
    run_cmd(DHF_CMD_OPENDIR);
    check("OPENDIR .", get_status() == DHF_ERR_OK);
    uint32_t hd = get_d0();

    int saw_hello = 0, saw_subdir = 0, n = 0;
    for (;;) {
        set_d0(hd); set_a1(GUEST_DATA_ADDR + 8192);
        run_cmd(DHF_CMD_READDIR);
        if (get_d1() == 0) break;
        const char *name = (const char *)&g_ram[GUEST_DATA_ADDR + 8192];
        if (strcmp(name, "hello.txt") == 0) saw_hello = 1;
        if (strcmp(name, "subdir") == 0) saw_subdir = 1;
        n++;
        if (n > 100) { fprintf(stderr, "READDIR-Endlosschleife?\n"); break; }
    }
    check("READDIR listet hello.txt", saw_hello);
    check("READDIR listet subdir", saw_subdir);

    set_d0(hd);
    run_cmd(DHF_CMD_CLOSE);

    /* --- RENAME + DELETE + RMDIR --- */
    put_str(GUEST_PATH_ADDR, "hello.txt");
    put_str(GUEST_PATH2_ADDR, "hello2.txt");
    set_a0(GUEST_PATH_ADDR); set_a1(GUEST_PATH2_ADDR);
    run_cmd(DHF_CMD_RENAME);
    check("RENAME hello.txt -> hello2.txt", get_status() == DHF_ERR_OK);

    put_str(GUEST_PATH_ADDR, "hello2.txt");
    set_a0(GUEST_PATH_ADDR);
    run_cmd(DHF_CMD_DELETE);
    check("DELETE hello2.txt", get_status() == DHF_ERR_OK);

    put_str(GUEST_PATH_ADDR, "subdir");
    set_a0(GUEST_PATH_ADDR);
    run_cmd(DHF_CMD_RMDIR);
    check("RMDIR subdir", get_status() == DHF_ERR_OK);

    /* --- Confinement: ".." muss abgelehnt werden --- */
    put_str(GUEST_PATH_ADDR, "../escape.txt");
    set_a0(GUEST_PATH_ADDR); set_d1(0644); set_d2(DHF_MODE_WRITE);
    run_cmd(DHF_CMD_CREATE);
    check("CREATE mit \"..\" wird abgelehnt", get_status() != DHF_ERR_OK);

    printf("\n%s\n", g_total_fail == 0 ? "ALLE TESTS BESTANDEN" : "TESTS FEHLGESCHLAGEN");

    char cmd[600];
    snprintf(cmd, sizeof(cmd), "rm -rf '%s'", tmpdir);
    if (system(cmd) != 0) { /* best effort */ }

    return g_total_fail == 0 ? 0 : 1;
}
//────────────────────────────────────────────────────────────────────────────────────────────────
// EOF 13_test_dhf.c                                                                       Ver. 2.00
//────────────────────────────────────────────────────────────────────────────────────────────────
