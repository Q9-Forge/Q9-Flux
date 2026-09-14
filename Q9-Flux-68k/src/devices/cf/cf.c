//════════════════════════════════════════════════════════════════════════════════════════════════
// File:   cf.c                                                                            Ver. 1.00
// Owner:  CF
// Desc.:  Implementierung, siehe cf.h. Reine Verschiebung aus src/kernel/q9board.c (2026-08-20,
//         Hardware-Vereinheitlichung Pilot "cf") -- Registerlogik/ATA-PIO-Protokoll/RBF-Heuristik
//         UNVERAENDERT, s. cf.h-Kopfkommentar. Neu ist nur q9_devdesc_cf am Dateiende (bisher
//         devschema.c's "cf"-Schema, Feldinhalte 1:1 uebernommen).
//
// Edition History
//─────────┬──────┬────────────────────────────────────────────────────────────────────────┬──────
// Date    │ Ver. │ Description                                                            │ By
//─────────┼──────┼────────────────────────────────────────────────────────────────────────┬──────
// 26-07-04│ 1.xx │ 5.2c/5.5a/5.17/5.19a: urspruenglich Teil von q9board.c, s. dortige        │ CF
//         │      │ Historie bis Ver. 2.00 fuer die volle Entwicklungsgeschichte             │
// 26-08-20│ 1.00 │ Hardware-Vereinheitlichung, Pilot "cf": aus q9board.c hierher verschoben,│ Cld
//         │      │ neu q9_devdesc_cf (Feldbeschreibung, bisher devschema.c "cf", die 10      │
//         │      │ typspezifischen Felder 1:1 uebernommen -- descriptor/descriptorName       │
//         │      │ kommen jetzt aus dem gemeinsamen q9_devschema_common_fields, devdesc.h)    │
//═════════╧══════╧════════════════════════════════════════════════════════════════════════╧══════
#include "cf.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* 5.2c: interne ATA-Registeroffsets relativ zu Q9_BOARD_CF_BASE. Nur das Minimum, das das
   READ/WRITE-SECTOR(S)-Protokoll braucht — kein Feature-/Error-Register, keine Mehrfach-Laufwerke. */
#define CF_REG_DATA     0x00u                        /* Datenregister, 1 Byte pro Zugriff        */
#define CF_REG_SECCNT   0x02u                        /* Sektor-Anzahl                             */
#define CF_REG_LBA0     0x03u                        /* LBA Bits  7.. 0                           */
#define CF_REG_LBA1     0x04u                        /* LBA Bits 15.. 8                           */
#define CF_REG_LBA2     0x05u                        /* LBA Bits 23..16                           */
#define CF_REG_LBA3     0x06u                        /* LBA/DEV: 111x + LBA Bits 27..24           */
#define CF_REG_CMD      0x07u                        /* Kommando (schreiben) / Status (lesen)     */
#define CF_REG_FEAT     0x01u                        /* Feature (schreiben) / Error (lesen)       */
#define CF_CMD_IDENTIFY 0xECu
#define RBF_DD_DIR      0x08u
#define RBF_DD_LSNSIZE  0x68u

static int board_cf_trace_enabled(void)
{
    static int cached = -1;
    if (cached < 0) {
        cached = getenv("Q9_BOARD_CF_TRACE") != 0;
    }
    return cached;
}

#define CF_TRACE(...) do { if (board_cf_trace_enabled()) fprintf(stderr, __VA_ARGS__); } while (0)

/* 5.19a: Die aktuell adressierte Einheit des Interfaces — DEV-Bit (Bit 4) in LBA3 waehlt Master
   ($E0) oder Slave ($F0), exakt wie die e0/f0-Descriptoren im REF-Q9-Port es programmieren.
   Die c0..c3-Descriptoren der Onboard-CF schreiben alle $E0 (DrvNum 0) — Verhalten wie vor 5.19a. */
static q9_cf_unit_t *cf_cur_unit(q9_cf_t *c)
{
    return &c->unit[(c->lba3 >> 4) & 1u];
}

static int board_cf_ensure_open(q9_cf_t *c)
{
    q9_cf_unit_t *u = cf_cur_unit(c);

    if (u->file) {
        return 1;
    }
    if (!u->path) {
        return 0;
    }
    u->file = fopen(u->path, "r+b");
    if (!u->file) {
        u->file = fopen(u->path, "w+b");
    }
    if (u->file && u->image_sector_size == 0) {
        u->image_sector_size = Q9_BOARD_CF_SECTOR_SIZE;

        /* 5.19a: PCF-Images (FAT12/16) sind immer 512er-Sektoren — die RBF-Heuristik unten wuerde
           die FAT-Bootsektor-Bytes als LSN0 fehlgedeutet abtasten, deshalb hier ueberspringen. */
        if (u->format != Q9_CF_FMT_PCF) {
            uint8_t hdr[Q9_BOARD_CF_SECTOR_SIZE];
            size_t  n;
            long    cur;

            cur = ftell(u->file);
            fseek(u->file, (long)u->start_sector * (long)Q9_BOARD_CF_SECTOR_SIZE, SEEK_SET);
            n = fread(hdr, 1, sizeof(hdr), u->file);
            fseek(u->file, cur, SEEK_SET);

            if (n >= 128) {
                uint32_t root_lsn = ((uint32_t)hdr[RBF_DD_DIR] << 16) |
                                    ((uint32_t)hdr[RBF_DD_DIR + 1] << 8) |
                                     (uint32_t)hdr[RBF_DD_DIR + 2];
                uint16_t lsn_size = (uint16_t)(((uint16_t)hdr[RBF_DD_LSNSIZE] << 8) |
                                                (uint16_t)hdr[RBF_DD_LSNSIZE + 1]);
                if (lsn_size == 256u) {
                    u->image_sector_size = 256u;
                } else if (lsn_size == 512u) {
                    u->image_sector_size = 512u;          /* explizite 512er-LSN: vertrauen,
                                                             KEINE 256-Heuristik (die kann sonst
                                                             ein modernes 512er-Image faelschlich
                                                             als 256er erkennen -- s.u.) */
                } else if (root_lsn > 0) {
                    /* DD.LSNSize weder 256 noch 512 (altes Image ohne verlaesslichen Wert):
                       zusaetzliche Heuristik -- die FD des Root-Verzeichnisses steht bei 256er-
                       Images an root_lsn*256 und traegt das Directory-Attribut (Bit 7). ACHTUNG:
                       nur als LETZTER Ausweg, weil derselbe Byte-Offset in einem 512er-Image ein
                       beliebiges Datenbyte trifft und dann falsch anschlaegt. */
                    uint8_t fd0 = 0;
                    fseek(u->file, (long)root_lsn * 256L, SEEK_SET);
                    if (fread(&fd0, 1, 1, u->file) == 1 && (fd0 & 0x80u)) {
                        u->image_sector_size = 256u;
                    }
                    fseek(u->file, cur, SEEK_SET);
                }
            }
        }
        CF_TRACE("[cf image-sector-size=%u]\n", u->image_sector_size);
    }
    return u->file != NULL;
}

//────────────────────────────────────────────────────────────────────────────────────────────────
// Function: board_cf_load_sector / board_cf_store_sector
// Desc.:    5.5a: Ein einzelner Sektor-Transfer zwischen Backing-Datei und sector-Puffer, an der
//           aktuellen c->lba. Ausgelagert aus board_cf_write, weil READ/WRITE SECTOR(S) jetzt
//           mehrere Sektoren hintereinander bedienen (s. board_cf_read/write unten) und pro
//           Sektor genau dieselben zwei Operationen brauchen.
// Call:     board_cf_load_sector(c); ... board_cf_store_sector(c);
//────────────────────────────────────────────────────────────────────────────────────────────────
static void board_cf_load_sector(q9_cf_t *c)
{
    uint32_t img_sec;

    memset(c->sector, 0, Q9_BOARD_CF_SECTOR_SIZE);
    if (board_cf_ensure_open(c)) {
        q9_cf_unit_t *u = cf_cur_unit(c);
        img_sec = u->image_sector_size ? u->image_sector_size : Q9_BOARD_CF_SECTOR_SIZE;
        fseek(u->file, (long)(u->start_sector + c->lba) * (long)img_sec, SEEK_SET);
        fread(c->sector, 1, img_sec, u->file);
    }
    if (c->lba == 0 || board_cf_trace_enabled()) {
        CF_TRACE("[cf read lba=%u first=%02x %02x %02x %02x %02x %02x %02x %02x]\n",
                 c->lba, c->sector[0], c->sector[1], c->sector[2], c->sector[3],
                 c->sector[4], c->sector[5], c->sector[6], c->sector[7]);
    }
    c->pos = 0;
}

static void board_cf_load_write_buffer(q9_cf_t *c)
{
    uint32_t img_sec;

    memset(c->sector, 0, Q9_BOARD_CF_SECTOR_SIZE);
    if (board_cf_ensure_open(c)) {
        q9_cf_unit_t *u = cf_cur_unit(c);
        img_sec = u->image_sector_size ? u->image_sector_size : Q9_BOARD_CF_SECTOR_SIZE;
        fseek(u->file, (long)(u->start_sector + c->lba) * (long)img_sec, SEEK_SET);
        fread(c->sector, 1, img_sec, u->file);
        if (img_sec < Q9_BOARD_CF_SECTOR_SIZE) {
            fseek(u->file, (long)(u->start_sector + c->lba + 1u) * (long)img_sec, SEEK_SET);
            fread(c->sector + img_sec, 1, Q9_BOARD_CF_SECTOR_SIZE - img_sec, u->file);
        }
    }
    c->pos = 0;
}

static void board_cf_store_sector(q9_cf_t *c)
{
    if (board_cf_ensure_open(c)) {
        q9_cf_unit_t *u = cf_cur_unit(c);
        uint32_t img_sec = u->image_sector_size ? u->image_sector_size : Q9_BOARD_CF_SECTOR_SIZE;
        uint32_t written = c->pos;
        if (written == 0 || written > Q9_BOARD_CF_SECTOR_SIZE) {
            written = Q9_BOARD_CF_SECTOR_SIZE;
        }
        fseek(u->file, (long)(u->start_sector + c->lba) * (long)img_sec, SEEK_SET);

        /* For 256-byte images: write TWO sectors (matching the read behavior in load_write_buffer) */
        if (img_sec < Q9_BOARD_CF_SECTOR_SIZE) {
            fwrite(c->sector, 1, img_sec, u->file);
            fseek(u->file, (long)(u->start_sector + c->lba + 1u) * (long)img_sec, SEEK_SET);
            fwrite(c->sector + img_sec, 1, Q9_BOARD_CF_SECTOR_SIZE - img_sec, u->file);
        } else {
            fwrite(c->sector, 1, written < img_sec ? written : img_sec, u->file);
        }
        fflush(u->file);
    }
    CF_TRACE("[cf write lba=%u first=%02x %02x %02x %02x %02x %02x %02x %02x]\n",
             c->lba, c->sector[0], c->sector[1], c->sector[2], c->sector[3],
             c->sector[4], c->sector[5], c->sector[6], c->sector[7]);
}

static void board_cf_identify(q9_cf_t *c)
{
    uint32_t sectors = 0;

    memset(c->sector, 0, Q9_BOARD_CF_SECTOR_SIZE);
    if (board_cf_ensure_open(c)) {
        q9_cf_unit_t *u = cf_cur_unit(c);
        long cur;

        /* 5.19a: DD_TOT aus LSN0 gilt nur fuer RBF-Images — bei PCF (FAT12/16) staenden an
           denselben Bytes Sprungbefehl+OEM-Name des Bootsektors und ergaeben eine Phantasie-
           Sektorzahl; dort zaehlt allein die Dateigroesse. */
        if (u->format != Q9_CF_FMT_PCF) {
            uint8_t hdr[16];
            cur = ftell(u->file);
            fseek(u->file, 0, SEEK_SET);
            if (fread(hdr, 1, sizeof(hdr), u->file) == sizeof(hdr)) {
                uint32_t dd_tot = ((uint32_t)hdr[0] << 16) |
                                  ((uint32_t)hdr[1] << 8) |
                                   (uint32_t)hdr[2];
                if (dd_tot > 0) {
                    sectors = dd_tot;
                }
            }
            fseek(u->file, cur, SEEK_SET);
        }

        fseek(u->file, 0, SEEK_END);
        cur = ftell(u->file);
        if (sectors == 0 && cur > 0) {
            uint32_t img_sec = u->image_sector_size ? u->image_sector_size : Q9_BOARD_CF_SECTOR_SIZE;
            unsigned long skip = (unsigned long)u->start_sector * (unsigned long)img_sec;
            sectors = cur > (long)skip ? (uint32_t)(((unsigned long)cur - skip) / img_sec) : 0;
        }
    }

    /* ATA identify words are little-endian on the data port. Word 0 = 0 means a regular ATA
       device; words 60/61 report total LBA28 sectors. */
    c->sector[120] = (uint8_t)(sectors & 0xFFu);
    c->sector[121] = (uint8_t)((sectors >> 8) & 0xFFu);
    c->sector[122] = (uint8_t)((sectors >> 16) & 0xFFu);
    c->sector[123] = (uint8_t)((sectors >> 24) & 0xFFu);
    c->pos = 0;
    c->transfer_size = Q9_BOARD_CF_SECTOR_SIZE;
    CF_TRACE("[cf identify sectors=%u]\n", sectors);
}

static uint32_t board_cf_transfer_size(q9_cf_t *c)
{
    uint32_t size;

    if (!board_cf_ensure_open(c)) {
        return Q9_BOARD_CF_SECTOR_SIZE;
    }
    size = cf_cur_unit(c)->image_sector_size;
    if (size == 0) {
        size = Q9_BOARD_CF_SECTOR_SIZE;
    }
    return (size < Q9_BOARD_CF_SECTOR_SIZE) ? size : Q9_BOARD_CF_SECTOR_SIZE;
}

//────────────────────────────────────────────────────────────────────────────────────────────────
// Function: board_cf_read / board_cf_write
// Desc.:    5.2c/5.5a: ATA-PIO-Minimalprotokoll — READ SECTOR(S) (0x20) und WRITE SECTOR(S)
//           (0x30) zaehlen den Sector-Count jetzt echt durch (cf_remaining, 0 in CF_REG_SECCNT
//           bedeutet 256 Sektoren, ATA-Konvention): nach jedem vollen Sektor wird die LBA
//           weitergezaehlt und — solange noch Sektoren ausstehen — der naechste Sektor
//           nachgeladen (Read) bzw. angenommen (Write), DRQ bleibt dabei gesetzt; erst beim
//           letzten Sektor wird DRQ geloescht (Read) bzw. cf_write_pending beendet (Write).
//           Datenregister ist 8-Bit-weise adressiert (ein Byte pro Zugriff, cf_pos zaehlt
//           0..511 pro Sektor hoch).
//────────────────────────────────────────────────────────────────────────────────────────────────
static uint8_t board_cf_read(q9_cf_t *c, uint32_t off)
{
    if (off == CF_REG_CMD) {
        return c->status;
    }
    if (off == CF_REG_DATA) {
        uint32_t xfer_size = c->transfer_size ? c->transfer_size : Q9_BOARD_CF_SECTOR_SIZE;
        if (c->pos < xfer_size) {
            uint8_t v = c->sector[c->pos++];
            if (c->pos >= xfer_size) {
                c->remaining--;
                c->lba++;
                if (c->remaining > 0) {
                    board_cf_load_sector(c);              /* naechster Sektor, DRQ bleibt gesetzt */
                    c->status = (uint8_t)(Q9_BOARD_CF_STAT_RDY | Q9_BOARD_CF_STAT_DRQ);
                } else {
                    c->status = (uint8_t)(Q9_BOARD_CF_STAT_RDY);      /* DRQ fertig geloescht */
                }
            }
            return v;
        }
        return 0;
    }
    return 0;                                           /* LBA/Seccnt: hier nicht rueckgelesen */
}

static void board_cf_write(q9_cf_t *c, uint32_t off, uint8_t val)
{
    switch (off) {
    case CF_REG_FEAT:
        return;                                           /* 8-bit feature wird beim Kommando angenommen */
    case CF_REG_SECCNT:
        c->sectcnt = val;
        return;
    case CF_REG_LBA0:
        c->lba = (c->lba & 0xFFFFFF00u) | val;
        return;
    case CF_REG_LBA1:
        c->lba = (c->lba & 0xFFFF00FFu) | ((uint32_t)val << 8);
        return;
    case CF_REG_LBA2:
        c->lba = (c->lba & 0xFF00FFFFu) | ((uint32_t)val << 16);
        return;
    case CF_REG_LBA3:
        c->lba3 = val;
        c->lba = (c->lba & 0x00FFFFFFu) | ((uint32_t)(val & 0x0Fu) << 24);
        return;
    case CF_REG_DATA:
        if (c->write_pending && c->pos < c->transfer_size) {
            c->sector[c->pos++] = val;
            if (c->pos >= c->transfer_size) {
                board_cf_store_sector(c);
                c->remaining--;
                c->lba++;
                if (c->remaining > 0) {
                    c->pos = 0;                           /* naechster Sektor, DRQ bleibt gesetzt */
                    c->status = (uint8_t)(Q9_BOARD_CF_STAT_RDY | Q9_BOARD_CF_STAT_DRQ);
                } else {
                    c->write_pending = 0;
                    c->status = Q9_BOARD_CF_STAT_RDY;
                }
            }
        }
        return;
    case CF_REG_CMD:
        CF_TRACE("[cf cmd=%02x count=%u lba=%u lba3=%02x]\n",
                 val, c->sectcnt ? c->sectcnt : 256u, c->lba, c->lba3);
        /* 5.19a: Kommando an eine unbestueckt Einheit (kein Image angehaengt, z.B. Slave ohne
           Config-Eintrag) — wie fehlendes Geraet beantworten: ERR statt DRQ, damit ein iniz
           des e0/f0-Descriptors sauber scheitert statt Null-Sektoren zu liefern. */
        if (!cf_cur_unit(c)->path) {
            c->write_pending = 0;
            c->status = (uint8_t)(Q9_BOARD_CF_STAT_RDY | Q9_BOARD_CF_STAT_ERR);
            return;
        }
        if (val == Q9_BOARD_CF_CMD_READ) {
            c->remaining = c->sectcnt ? c->sectcnt : 256u;
            c->transfer_size = board_cf_transfer_size(c);
            board_cf_load_sector(c);
            c->write_pending = 0;
            c->status = (uint8_t)(Q9_BOARD_CF_STAT_RDY | Q9_BOARD_CF_STAT_DRQ);
        } else if (val == Q9_BOARD_CF_CMD_WRITE) {
            c->remaining = c->sectcnt ? c->sectcnt : 256u;
            c->transfer_size = board_cf_transfer_size(c);
            board_cf_load_write_buffer(c);
            c->write_pending = 1;
            c->status = (uint8_t)(Q9_BOARD_CF_STAT_RDY | Q9_BOARD_CF_STAT_DRQ);
        } else if (val == Q9_BOARD_CF_CMD_SETFEAT) {
            /* SET FEATURES (z.B. 8-Bit-Mode, den der Board-Boot-Treiber setzt): kommentarlos
               annehmen — unser Datenregister ist ohnehin byteweise (s. board_cf_read). */
            c->status = Q9_BOARD_CF_STAT_RDY;
        } else if (val == CF_CMD_IDENTIFY) {
            c->remaining = 1;
            board_cf_identify(c);
            c->write_pending = 0;
            c->status = (uint8_t)(Q9_BOARD_CF_STAT_RDY | Q9_BOARD_CF_STAT_DRQ);
        } else {
            c->status = (uint8_t)(Q9_BOARD_CF_STAT_RDY | Q9_BOARD_CF_STAT_ERR);
        }
        return;
    default:
        return;                                          /* uebrige Register: sauber angenommen */
    }
}

void q9_cf_attach(q9_cf_t *c, int unit, const char *path, int format)
{
    q9_cf_unit_t *u = &c->unit[unit & 1];

    if (u->file) {
        fclose(u->file);
    }
    u->path = path;
    u->file = NULL;
    u->image_sector_size = 0;
    u->start_sector = 0;
    u->format = format;

    /* Registersatz zuruecksetzen wie bei einem Kartenwechsel (Verhalten wie das alte
       q9_board_cf_attach, nur dass der Zustand jetzt am Interface statt am Board haengt). */
    c->lba = 0;
    c->lba3 = 0;
    c->sectcnt = 0;
    c->pos = 0;
    c->transfer_size = 0;
    c->write_pending = 0;
    c->remaining = 0;
    c->status = Q9_BOARD_CF_STAT_RDY;
}

void q9_cf_set_start_sector(q9_cf_t *c, int unit, uint32_t start_sector)
{
    c->unit[unit & 1].start_sector = start_sector;
}

//────────────────────────────────────────────────────────────────────────────────────────────────
// Function: cf_dev_* / q9_devtype_cf
// Desc.:    5.17: Vtable-Adapter fuer die Geraete-Registry (devreg.h), zweites umgezogenes Geraet.
//           5.19a: dev->state zeigt jetzt auf das q9_cf_t-INTERFACE (nicht mehr aufs Board) und
//           die Basisadresse kommt aus dev->base — damit dieselbe Vtable auch die zweite Instanz
//           (RC2014-SC145 bei Q9_BOARD_CF2_BASE) bedienen kann. CF ist die im ARBEITSPLAN 5.17
//           genannte Ausnahme "behaelt eigene 16/32-Bit-Pfade": am Datenregister (CF_REG_DATA,
//           addr==dev->base) liest/schreibt ein 16/32-Bit-Zugriff MEHRERE aufeinanderfolgende
//           Byte-Transfers desselben ATA-PIO-Datenregisters (kein Adress-Fortschreiten wie bei
//           generischer Byte-Synthese!) -- genau das musste schon vor 5.17 in
//           q9_board_read16/32/write16/32 speziell behandelt werden. Kein IRQ (poll/irq_pending
//           bleiben NULL, wie im alten Board-Fallback: CF wurde nie vom Hauptschleifen-Poll
//           abgefragt).
//────────────────────────────────────────────────────────────────────────────────────────────────
static uint8_t cf_dev_read8(q9_device_t *dev, uint32_t addr)
{
    return board_cf_read((q9_cf_t *)dev->state, addr - dev->base);
}

static void cf_dev_write8(q9_device_t *dev, uint32_t addr, uint8_t val)
{
    board_cf_write((q9_cf_t *)dev->state, addr - dev->base, val);
}

static uint16_t cf_dev_read16(q9_device_t *dev, uint32_t addr)
{
    q9_cf_t *c = (q9_cf_t *)dev->state;
    if (addr == dev->base) {
        uint16_t hi = board_cf_read(c, CF_REG_DATA);
        uint16_t lo = board_cf_read(c, CF_REG_DATA);
        return (uint16_t)((hi << 8) | lo);
    }
    {
        uint16_t hi = cf_dev_read8(dev, addr);
        uint16_t lo = cf_dev_read8(dev, addr + 1);
        return (uint16_t)((hi << 8) | lo);
    }
}

static void cf_dev_write16(q9_device_t *dev, uint32_t addr, uint16_t val)
{
    q9_cf_t *c = (q9_cf_t *)dev->state;
    if (addr == dev->base) {
        board_cf_write(c, CF_REG_DATA, (uint8_t)(val >> 8));
        board_cf_write(c, CF_REG_DATA, (uint8_t)val);
        return;
    }
    cf_dev_write8(dev, addr,      (uint8_t)(val >> 8));
    cf_dev_write8(dev, addr + 1u, (uint8_t)val);
}

static uint32_t cf_dev_read32(q9_device_t *dev, uint32_t addr)
{
    q9_cf_t *c = (q9_cf_t *)dev->state;
    if (addr == dev->base) {
        uint32_t b0 = board_cf_read(c, CF_REG_DATA);
        uint32_t b1 = board_cf_read(c, CF_REG_DATA);
        uint32_t b2 = board_cf_read(c, CF_REG_DATA);
        uint32_t b3 = board_cf_read(c, CF_REG_DATA);
        return (b0 << 24) | (b1 << 16) | (b2 << 8) | b3;
    }
    {
        uint32_t b0 = cf_dev_read8(dev, addr);
        uint32_t b1 = cf_dev_read8(dev, addr + 1);
        uint32_t b2 = cf_dev_read8(dev, addr + 2);
        uint32_t b3 = cf_dev_read8(dev, addr + 3);
        return (b0 << 24) | (b1 << 16) | (b2 << 8) | b3;
    }
}

static void cf_dev_write32(q9_device_t *dev, uint32_t addr, uint32_t val)
{
    q9_cf_t *c = (q9_cf_t *)dev->state;
    if (addr == dev->base) {
        board_cf_write(c, CF_REG_DATA, (uint8_t)(val >> 24));
        board_cf_write(c, CF_REG_DATA, (uint8_t)(val >> 16));
        board_cf_write(c, CF_REG_DATA, (uint8_t)(val >> 8));
        board_cf_write(c, CF_REG_DATA, (uint8_t)val);
        return;
    }
    cf_dev_write8(dev, addr,      (uint8_t)(val >> 24));
    cf_dev_write8(dev, addr + 1u, (uint8_t)(val >> 16));
    cf_dev_write8(dev, addr + 2u, (uint8_t)(val >> 8));
    cf_dev_write8(dev, addr + 3u, (uint8_t)val);
}

const q9_device_vtable_t q9_devtype_cf = {
    .read8         = cf_dev_read8,
    .write8        = cf_dev_write8,
    .read16        = cf_dev_read16,                   /* eigener Pfad (s.o.), NICHT synthetisiert */
    .write16       = cf_dev_write16,
    .read32        = cf_dev_read32,
    .write32       = cf_dev_write32,
    .poll          = NULL,
    .irq_pending   = NULL,                            /* CF hat keinen IRQ (wie vor 5.17)         */
    .reset         = NULL,
    .irq_vector_fn = NULL,
};

//────────────────────────────────────────────────────────────────────────────────────────────────
// q9_devdesc_cf (2026-08-20): Feldbeschreibung, bisher devschema.c's "cf"-Schema -- Inhalt der 10
// typspezifischen Felder 1:1 uebernommen (Namen sind die .q9-DATEI-Schluesselwoerter, NICHT die
// internen C-Struct-Feldnamen, s. devschema.h-Historie 2026-08-14). descriptor/descriptorName
// sind NICHT mehr hier -- die kommen jetzt aus dem gemeinsamen q9_devschema_common_fields
// (devdesc.h), ein Aufrufer haengt extra_fields[] gedanklich dahinter an.
//────────────────────────────────────────────────────────────────────────────────────────────────
static const char *const g_cf_bus_values[]    = { "onboard", "cf", "rc2014", "sc145", "secondary", NULL };
static const char *const g_cf_unit_values[]   = { "master", "0", "slave", "1", NULL };
static const char *const g_cf_format_values[] = { "auto", "rbf", "pcf", "fat", NULL };

static const q9_field_schema_t g_cf_extra_fields[] = {
    {
        .name = "image", .kind = Q9_FIELD_STR, .required = 1,
        .desc = "Pfad zum Image (relativ zur Config-Datei aufgeloest)",
    },
    {
        .name = "bus", .kind = Q9_FIELD_ENUM, .enum_values = g_cf_bus_values,
        .desc = "welches emulierte CF-Interface (Default onboard)",
    },
    {
        .name = "unit", .kind = Q9_FIELD_ENUM, .enum_values = g_cf_unit_values,
        .desc = "Master/Slave am ATA-Bus (Default master)",
    },
    {
        .name = "type", .kind = Q9_FIELD_ENUM, .enum_values = g_cf_format_values,
        .desc = "Image-Format, auto erkennt RBF/PCF an der Groesse (Default auto)",
    },
    /* 2026-08-14 (Andreas' Vorschlag, Fortsetzung von ARBEITSPLAN 5.18 "Config-seitig bewusst
       BEIDES anbieten"): Wahl zwischen einem automatisch zugeteilten I/O-Tabellenplatz (schneller
       Dispatch, s. m68krt.c g_io_table) und einer frei gewaehlten Adresse. Nur additiv beschrieben,
       NOCH KEIN boardcfg.c-Anschluss (die eigentliche Config-gesteuerte Instanziierung ueber die
       Tabelle ist selbst noch nicht gebaut, s. 5.18 "Weiterhin offen"). */
    {
        .name = "useSlot", .kind = Q9_FIELD_BOOL,
        .desc = "vordefinierten I/O-Tabellenplatz nutzen (schneller Dispatch, 256-Byte-Raster ab "
                "$FFFF0000) statt einer frei gewaehlten Adresse -- Default no",
    },
    {
        .name = "slot", .kind = Q9_FIELD_INT, .has_range = 1, .min = 0, .max = 255,
        .depends_on = "useSlot", .depends_on_value = "yes",
        .desc = "I/O-Tabellenplatz-Nummer (0-255); Adresse = $FFFF0000 + slot*256 -- nur relevant "
                "wenn useSlot=yes",
    },
    {
        .name = "base", .kind = Q9_FIELD_INT, .has_range = 1, .min = 0, .max = 0xFFFFFFFFL,
        .desc = "ATA-Basisadresse; 0 = Standard-Base anhand von bus. Wenn useSlot=yes wird dieser "
                "Wert automatisch aus slot berechnet (Editor: nur anzeigen, nicht eingeben lassen)",
    },
    {
        .name = "start_sector", .kind = Q9_FIELD_INT, .has_range = 1, .min = 0, .max = 0xFFFFFFFFL,
        .desc = "Host-Startsektor, der als Gast-LBA 0 erscheint (Default 0)",
    },
    {
        .name = "length_sectors", .kind = Q9_FIELD_INT, .has_range = 1, .min = 0, .max = 0xFFFFFFFFL,
        .desc = "logische Partitionslaenge fuer Descriptor/Pruefung",
    },
    {
        .name = "descriptor_lsn", .kind = Q9_FIELD_INT, .has_range = 1, .min = 0, .max = 0xFFFFFFFFL,
        .desc = "PD_LSNOffs im OS-9-Descriptor -- fuer spaeteren Descriptor-Abgleich",
    },
};
#define Q9_CF_EXTRA_FIELD_COUNT (int)(sizeof(g_cf_extra_fields) / sizeof(g_cf_extra_fields[0]))

const q9_devdesc_t q9_devdesc_cf = {
    .type              = "cf",
    .desc              = "Compact-Flash-Interface (ATA-PIO, RBF/PCF-Images)",
    .vt                = &q9_devtype_cf,
    .use_table_default = 1,                            /* liegt im Fast-Table-Cluster, s. devreg.h */
    .extra_fields      = g_cf_extra_fields,
    .extra_field_count = Q9_CF_EXTRA_FIELD_COUNT,
};

//────────────────────────────────────────────────────────────────────────────────────────────────
// EOF cf.c                                                                                Ver. 1.00
//────────────────────────────────────────────────────────────────────────────────────────────────
