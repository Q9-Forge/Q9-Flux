//════════════════════════════════════════════════════════════════════════════════════════════════
// File:   07_test_cf_sector512.c                                                         Ver. 1.00
// Owner:  AF
// Desc.:  Dateisystem-unabhaengiger Sektor-Roundtrip-Test fuer die CF-Emulation (cb030.c). Faehrt
//         das rohe ATA-PIO-READ/WRITE-SECTOR(S)-Protokoll direkt gegen q9_cf_attach/q9_devtype_cf,
//         OHNE 68k-CPU, OHNE OS-9, OHNE RBF/PCF-Treiber -- die Frage ist ausschliesslich: schreibt/
//         liest die Emulation bei image_sector_size=512 dieselben Bytes zurueck wie bei 256?
//         cb030.h dokumentiert die CF-Emulation ausdruecklich als "eigenstaendig testbar"
//         (q9_cb030_read8/write8, q9_cf_attach) -- dieser Test nutzt genau diesen Seiteneingang.
//
//         Vergleicht drei Szenarien im selben Lauf: RBF/256 (Kontrolle, laut Andreas bekannt gut),
//         RBF/512 (explizite DD.LSNSize=512 im Header, KEINE Heuristik), PCF/512 (Format-Flag
//         schaltet die RBF-Heuristik komplett ab, s. 5.19a-Kommentar in cb030.c). Jedes Szenario
//         durchlaeuft: Einzel-Sektor an low/high LBA, Mehrfach-Sektor (count=4), volle 256er-Kette
//         (SECCNT=0 = ATA-Konvention fuer 256 Sektoren), Master/Slave-Isolation (DEV-Bit LBA3),
//         sowie 16-/32-Bit-Datenregisterzugriffe (cf_dev_read16/32 haben einen eigenen Pfad, s.
//         cb030.c-Kommentar bei q9_devtype_cf).
//
// Call:   make test-cf-sector   (baut+startet), oder manuell s. Makefile-Target
//
// Edition History
//─────────┬──────┬────────────────────────────────────────────────────────────────────────┬──────
// Date    │ Ver. │ Description                                                            │ By
//─────────┼──────┼────────────────────────────────────────────────────────────────────────┼──────
// 26-07-21│ 1.00 │ Initiale Version (Andreas' Wunsch: genereller 512-Byte-Sektortest,       │ CF
//         │      │ unabhaengig vom Dateisystem, vor der PCF-Format-Fehlersuche)             │
//═════════╧══════╧════════════════════════════════════════════════════════════════════════╧══════
#include "../src/kernel/cb030.h"
#include "../src/kernel/devreg.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ATA-Registeroffsets -- 1:1 aus cb030.c gespiegelt (dort als CF_REG_* static, hier fuer den
   Test-Treiber dupliziert, weil cb030.c bewusst keine internen Offsets exportiert). */
#define REG_DATA    0x00u
#define REG_SECCNT  0x02u
#define REG_LBA0    0x03u
#define REG_LBA1    0x04u
#define REG_LBA2    0x05u
#define REG_LBA3    0x06u
#define REG_CMD     0x07u
#define CMD_READ    0x20u
#define CMD_WRITE   0x30u
#define DEV_MASTER  0xE0u
#define DEV_SLAVE   0xF0u

#define TOTAL_SECTORS 300u                                /* reicht fuer SECCNT=0 (=256) + Marge */

static int g_total_fail = 0;
static int g_total_checks = 0;

static void check(int cond, const char *what)
{
    g_total_checks++;
    if (cond) {
        printf("    OK   %s\n", what);
    } else {
        printf("    FAIL %s\n", what);
        g_total_fail++;
    }
}

static uint8_t pattern(uint32_t lba, uint32_t idx, uint8_t salt)
{
    return (uint8_t)((lba * 131u + idx * 17u + salt) & 0xFFu);
}

static void set_lba(q9_device_t *d, uint32_t lba, uint8_t dev)
{
    q9_device_write8(d, REG_LBA0, (uint8_t)(lba & 0xFFu));
    q9_device_write8(d, REG_LBA1, (uint8_t)((lba >> 8) & 0xFFu));
    q9_device_write8(d, REG_LBA2, (uint8_t)((lba >> 16) & 0xFFu));
    q9_device_write8(d, REG_LBA3, (uint8_t)(dev | ((lba >> 24) & 0x0Fu)));
}

/* 8-Bit-Pfad: ein Byte pro Zugriff (Standardfall, wie der reale cfide-Treiber im 8-Bit-Modus).
   sector_bytes = tatsaechliche Transfergroesse je ATA-"Sektor" (256 bei RBF/256, sonst 512, s.
   cb030_cf_transfer_size) -- bei 256-Byte-LSNs pulst DRQ schon nach 256 statt 512 Byte, ein Treiber
   (real oder Test) MUSS das wissen, sonst laeuft die zweite Haelfte des Puffers ins Leere. */
static void ata_write8(q9_device_t *d, uint32_t lba, uint32_t count, uint8_t dev, uint8_t salt, uint32_t sector_bytes)
{
    uint32_t total = (count == 0) ? 256u : count;
    uint32_t s, i;
    set_lba(d, lba, dev);
    q9_device_write8(d, REG_SECCNT, (uint8_t)count);
    q9_device_write8(d, REG_CMD, CMD_WRITE);
    for (s = 0; s < total; s++) {
        for (i = 0; i < sector_bytes; i++) {
            q9_device_write8(d, REG_DATA, pattern(lba + s, i, salt));
        }
    }
}

static int ata_read8_verify(q9_device_t *d, uint32_t lba, uint32_t count, uint8_t dev, uint8_t salt, uint32_t sector_bytes)
{
    uint32_t total = (count == 0) ? 256u : count;
    uint32_t s, i;
    int mism = 0;
    set_lba(d, lba, dev);
    q9_device_write8(d, REG_SECCNT, (uint8_t)count);
    q9_device_write8(d, REG_CMD, CMD_READ);
    for (s = 0; s < total; s++) {
        for (i = 0; i < sector_bytes; i++) {
            uint8_t got  = q9_device_read8(d, REG_DATA);
            uint8_t want = pattern(lba + s, i, salt);
            if (got != want) {
                if (mism < 5) {
                    printf("      Mismatch lba=%u byte=%u: got=%02x want=%02x\n", lba + s, i, got, want);
                }
                mism++;
            }
        }
    }
    return mism;
}

/* 16-/32-Bit-Pfad: cf_dev_read16/32 haben laut cb030.c-Kommentar einen EIGENEN Pfad am
   Datenregister (mehrere aufeinanderfolgende Byte-Transfers desselben Registers, kein Adress-
   Fortschreiten) -- getrennt gegengeprueft, weil das eine bekannte Ausnahme von der generischen
   Byte-Synthese ist. */
static void ata_write16(q9_device_t *d, uint32_t lba, uint32_t count, uint8_t dev, uint8_t salt, uint32_t sector_bytes)
{
    uint32_t total = (count == 0) ? 256u : count;
    uint32_t s, i;
    set_lba(d, lba, dev);
    q9_device_write8(d, REG_SECCNT, (uint8_t)count);
    q9_device_write8(d, REG_CMD, CMD_WRITE);
    for (s = 0; s < total; s++) {
        for (i = 0; i < sector_bytes; i += 2u) {
            uint16_t hi = pattern(lba + s, i, salt);
            uint16_t lo = pattern(lba + s, i + 1u, salt);
            q9_device_write16(d, REG_DATA, (uint16_t)((hi << 8) | lo));
        }
    }
}

static int ata_read16_verify(q9_device_t *d, uint32_t lba, uint32_t count, uint8_t dev, uint8_t salt, uint32_t sector_bytes)
{
    uint32_t total = (count == 0) ? 256u : count;
    uint32_t s, i;
    int mism = 0;
    set_lba(d, lba, dev);
    q9_device_write8(d, REG_SECCNT, (uint8_t)count);
    q9_device_write8(d, REG_CMD, CMD_READ);
    for (s = 0; s < total; s++) {
        for (i = 0; i < sector_bytes; i += 2u) {
            uint16_t got  = q9_device_read16(d, REG_DATA);
            uint16_t want = (uint16_t)(((uint16_t)pattern(lba + s, i, salt) << 8) | pattern(lba + s, i + 1u, salt));
            if (got != want) {
                if (mism < 5) {
                    printf("      Mismatch(16) lba=%u byte=%u: got=%04x want=%04x\n", lba + s, i, got, want);
                }
                mism++;
            }
        }
    }
    return mism;
}

static void ata_write32(q9_device_t *d, uint32_t lba, uint32_t count, uint8_t dev, uint8_t salt, uint32_t sector_bytes)
{
    uint32_t total = (count == 0) ? 256u : count;
    uint32_t s, i;
    set_lba(d, lba, dev);
    q9_device_write8(d, REG_SECCNT, (uint8_t)count);
    q9_device_write8(d, REG_CMD, CMD_WRITE);
    for (s = 0; s < total; s++) {
        for (i = 0; i < sector_bytes; i += 4u) {
            uint32_t v = ((uint32_t)pattern(lba + s, i,      salt) << 24) |
                         ((uint32_t)pattern(lba + s, i + 1u, salt) << 16) |
                         ((uint32_t)pattern(lba + s, i + 2u, salt) << 8)  |
                          (uint32_t)pattern(lba + s, i + 3u, salt);
            q9_device_write32(d, REG_DATA, v);
        }
    }
}

static int ata_read32_verify(q9_device_t *d, uint32_t lba, uint32_t count, uint8_t dev, uint8_t salt, uint32_t sector_bytes)
{
    uint32_t total = (count == 0) ? 256u : count;
    uint32_t s, i;
    int mism = 0;
    set_lba(d, lba, dev);
    q9_device_write8(d, REG_SECCNT, (uint8_t)count);
    q9_device_write8(d, REG_CMD, CMD_READ);
    for (s = 0; s < total; s++) {
        for (i = 0; i < sector_bytes; i += 4u) {
            uint32_t got  = q9_device_read32(d, REG_DATA);
            uint32_t want = ((uint32_t)pattern(lba + s, i,      salt) << 24) |
                             ((uint32_t)pattern(lba + s, i + 1u, salt) << 16) |
                             ((uint32_t)pattern(lba + s, i + 2u, salt) << 8)  |
                              (uint32_t)pattern(lba + s, i + 3u, salt);
            if (got != want) {
                if (mism < 5) {
                    printf("      Mismatch(32) lba=%u byte=%u: got=%08x want=%08x\n", lba + s, i, got, want);
                }
                mism++;
            }
        }
    }
    return mism;
}

/* Legt eine leere Backing-Datei mit 'sectors' Sektoren a 'sector_bytes' an. force_lsn_size!=0
   schreibt zusaetzlich DD.LSNSize (Offset 0x68/0x69, s. RBF_DD_LSNSIZE in cb030.c) an den
   Dateianfang, damit die RBF-Heuristik in cb030_cf_ensure_open genau diese Groesse erkennt. */
static void make_image(const char *path, uint32_t sectors, uint32_t sector_bytes, uint16_t force_lsn_size)
{
    FILE *f = fopen(path, "w+b");
    uint8_t *buf;
    uint32_t total = sectors * sector_bytes;

    if (!f) {
        fprintf(stderr, "make_image: kann %s nicht anlegen\n", path);
        exit(2);
    }
    buf = (uint8_t *)calloc(1, total);
    if (force_lsn_size) {
        buf[0x68] = (uint8_t)(force_lsn_size >> 8);
        buf[0x69] = (uint8_t)(force_lsn_size & 0xFFu);
    }
    fwrite(buf, 1, total, f);
    fclose(f);
    free(buf);
}

/* Ein komplettes Szenario (RBF/256, RBF/512-explizit oder PCF/512) -- gibt die Zahl der Fails
   zurueck. sector_bytes steuert NUR die Testerwartung (wie viele Bytes pro ATA-"Sektor" wirklich
   uebertragen werden); die Emulation entscheidet selbst per Header/Format, s.o. */
static int run_suite(const char *label, int format, uint16_t force_lsn_size, uint32_t sector_bytes)
{
    char path_m[256], path_s[256];
    q9_cf_t   cf;
    q9_device_t dev;
    int fails_before = g_total_fail;
    int mism;

    printf("\n=== Szenario: %s (Transfergroesse %u Byte/Sektor) ===\n", label, sector_bytes);

    snprintf(path_m, sizeof(path_m), "/tmp/q9_cf_test_master_%s.img", label);
    snprintf(path_s, sizeof(path_s), "/tmp/q9_cf_test_slave_%s.img",  label);
    make_image(path_m, TOTAL_SECTORS, 512u, force_lsn_size);
    make_image(path_s, TOTAL_SECTORS, 512u, force_lsn_size);

    memset(&cf, 0, sizeof(cf));
    q9_cf_attach(&cf, 0, path_m, format);
    q9_cf_attach(&cf, 1, path_s, format);

    memset(&dev, 0, sizeof(dev));
    dev.base  = 0;
    dev.size  = 8;
    dev.vt    = &q9_devtype_cf;
    dev.state = &cf;

    /* 1) Einzel-Sektor an LBA 0 (haeufigster/erster Zugriff, z.B. Boot-Sektor) */
    ata_write8(&dev, 0, 1, DEV_MASTER, 1, sector_bytes);
    mism = ata_read8_verify(&dev, 0, 1, DEV_MASTER, 1, sector_bytes);
    check(mism == 0, "Einzel-Sektor LBA 0, Roundtrip 8-Bit");

    /* 2) Einzel-Sektor an einer "krummen" mittleren LBA */
    ata_write8(&dev, 137, 1, DEV_MASTER, 2, sector_bytes);
    mism = ata_read8_verify(&dev, 137, 1, DEV_MASTER, 2, sector_bytes);
    check(mism == 0, "Einzel-Sektor LBA 137, Roundtrip 8-Bit");

    /* 3) Einzel-Sektor an der letzten LBA des Images */
    ata_write8(&dev, TOTAL_SECTORS - 1u, 1, DEV_MASTER, 3, sector_bytes);
    mism = ata_read8_verify(&dev, TOTAL_SECTORS - 1u, 1, DEV_MASTER, 3, sector_bytes);
    check(mism == 0, "Einzel-Sektor letzte LBA, Roundtrip 8-Bit");

    /* 4) Mehrfach-Sektor (count=4), typischer RBF-Directory-/FAT-Cluster-Lesevorgang */
    ata_write8(&dev, 10, 4, DEV_MASTER, 4, sector_bytes);
    mism = ata_read8_verify(&dev, 10, 4, DEV_MASTER, 4, sector_bytes);
    check(mism == 0, "Mehrfach-Sektor count=4 @ LBA 10, Roundtrip 8-Bit");

    /* 5) SECCNT=0 = ATA-Konvention fuer 256 Sektoren am Stueck */
    ata_write8(&dev, 0, 0, DEV_MASTER, 5, sector_bytes);
    mism = ata_read8_verify(&dev, 0, 0, DEV_MASTER, 5, sector_bytes);
    check(mism == 0, "SECCNT=0 (=256 Sektoren) @ LBA 0, Roundtrip 8-Bit");

    /* 6) Master/Slave-Isolation ueber das DEV-Bit in LBA3 */
    ata_write8(&dev, 20, 1, DEV_MASTER, 6, sector_bytes);
    ata_write8(&dev, 20, 1, DEV_SLAVE, 60, sector_bytes);
    mism  = ata_read8_verify(&dev, 20, 1, DEV_MASTER, 6, sector_bytes);
    mism += ata_read8_verify(&dev, 20, 1, DEV_SLAVE, 60, sector_bytes);
    check(mism == 0, "Master/Slave-Isolation @ LBA 20 (DEV-Bit)");

    /* 7) 16-Bit-Datenregisterpfad (cf_dev_read16/write16, eigener Code) */
    ata_write16(&dev, 30, 1, DEV_MASTER, 7, sector_bytes);
    mism = ata_read16_verify(&dev, 30, 1, DEV_MASTER, 7, sector_bytes);
    check(mism == 0, "Einzel-Sektor LBA 30, Roundtrip 16-Bit-Pfad");

    /* 8) 32-Bit-Datenregisterpfad (cf_dev_read32/write32, eigener Code) */
    ata_write32(&dev, 40, 1, DEV_MASTER, 8, sector_bytes);
    mism = ata_read32_verify(&dev, 40, 1, DEV_MASTER, 8, sector_bytes);
    check(mism == 0, "Einzel-Sektor LBA 40, Roundtrip 32-Bit-Pfad");

    /* 9) Gemischt: mit 8-Bit geschrieben, mit 16-Bit gelesen (und umgekehrt) -- deckt auf, ob
       die Puffer-Interpretation zwischen den Pfaden auseinanderlaeuft. */
    ata_write8(&dev, 50, 1, DEV_MASTER, 9, sector_bytes);
    mism = ata_read16_verify(&dev, 50, 1, DEV_MASTER, 9, sector_bytes);
    check(mism == 0, "LBA 50: 8-Bit geschrieben, 16-Bit gelesen");
    ata_write16(&dev, 51, 1, DEV_MASTER, 10, sector_bytes);
    mism = ata_read8_verify(&dev, 51, 1, DEV_MASTER, 10, sector_bytes);
    check(mism == 0, "LBA 51: 16-Bit geschrieben, 8-Bit gelesen");

    remove(path_m);
    remove(path_s);

    return g_total_fail - fails_before;
}

int main(void)
{
    int f256, f512_rbf, f512_pcf;

    printf("Q9 CF-Sektor-Emulationstest (dateisystem-unabhaengig, s. cb030.c/cb030.h)\n");
    printf("Testet ausschliesslich das rohe ATA-PIO-Protokoll (q9_cf_attach + q9_devtype_cf),\n");
    printf("keine 68k-CPU, kein OS-9, kein RBF-/PCF-Treiber beteiligt.\n");

    f256     = run_suite("RBF-256-Kontrolle", Q9_CF_FMT_RBF, 256u, 256u);
    f512_rbf = run_suite("RBF-512-explizit",  Q9_CF_FMT_RBF, 512u, 512u);
    f512_pcf = run_suite("PCF-512-Standard",  Q9_CF_FMT_PCF, 0u,   512u);

    printf("\n=== Zusammenfassung ===\n");
    printf("  RBF/256 (Kontrolle):  %s (%d Fails)\n", f256     == 0 ? "PASS" : "FAIL", f256);
    printf("  RBF/512 (explizit):   %s (%d Fails)\n", f512_rbf == 0 ? "PASS" : "FAIL", f512_rbf);
    printf("  PCF/512 (Standard):   %s (%d Fails)\n", f512_pcf == 0 ? "PASS" : "FAIL", f512_pcf);
    printf("  Gesamt: %d/%d Checks fehlgeschlagen\n", g_total_fail, g_total_checks);

    return g_total_fail == 0 ? 0 : 1;
}

//────────────────────────────────────────────────────────────────────────────────────────────────
// EOF 07_test_cf_sector512.c                                                             Ver. 1.00
//────────────────────────────────────────────────────────────────────────────────────────────────
