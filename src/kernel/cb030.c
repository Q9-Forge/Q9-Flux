//════════════════════════════════════════════════════════════════════════════════════════════════
// File:   cb030.c                                                                         Ver. 1.40
// Owner:  AF
// Desc.:  Implementierung der CB030-Board-Emulation, siehe cb030.h.
//
// Edition History
//─────────┬──────┬────────────────────────────────────────────────────────────────────────┬──────
// Date    │ Ver. │ Description                                                            │ By
//─────────┼──────┼────────────────────────────────────────────────────────────────────────┼──────
// 26-07-04│ 1.00 │ 5.2a: Erster Grundbaustein                                              │ CF
// 26-07-04│ 1.10 │ 5.2b: 68681-DUART (SRA/THRA/RHRA, Rest wird sauber angenommen)           │ CF
// 26-07-04│ 1.20 │ 5.2c: Compact-Flash (ATA-PIO-Minimalprotokoll, Backing-Datei)            │ CF
// 26-07-04│ 1.30 │ 5.2d: Timer/IRQ3 (kooperative Host-Zeitpruefung)                         │ CF
// 26-07-05│ 1.40 │ 5.3: Spiegelgrenze bis 0xFEFF_FFFF (statt 0x0800_0000), I/O vor dem       │ CF
//         │      │ Remap erreichbar (Dispatch umgestellt), q9_cb030_rom_load neu             │
//═════════╧══════╧════════════════════════════════════════════════════════════════════════╧══════
#include "cb030.h"
#include "../hal/q9_hal.h"
#include <string.h>

/* 5.2c: interne ATA-Registeroffsets relativ zu Q9_CB030_CF_BASE. Nur das Minimum, das das
   READ/WRITE-SECTOR(S)-Protokoll braucht — kein Feature-/Error-Register, keine Mehrfach-Laufwerke. */
#define CF_REG_DATA     0x00u                        /* Datenregister, 1 Byte pro Zugriff        */
#define CF_REG_SECCNT   0x02u                        /* Sektor-Anzahl                             */
#define CF_REG_LBA0     0x03u                        /* LBA Bits  7.. 0                           */
#define CF_REG_LBA1     0x04u                        /* LBA Bits 15.. 8                           */
#define CF_REG_LBA2     0x05u                        /* LBA Bits 23..16                           */
#define CF_REG_CMD      0x07u                        /* Kommando (schreiben) / Status (lesen)     */

//────────────────────────────────────────────────────────────────────────────────────────────────
// Function: cb030_is_remap_reg
// Desc.:    Prueft, ob 'addr' im REMAP-Registerbereich liegt (reiner Adress-Trigger, s. cb030.h).
// Call:     if (cb030_is_remap_reg(addr)) ...
//────────────────────────────────────────────────────────────────────────────────────────────────
static int cb030_is_remap_reg(uint32_t addr)
{
    return addr >= Q9_CB030_REMAP_REG_BASE && addr <= Q9_CB030_REMAP_REG_TOP;
}

//────────────────────────────────────────────────────────────────────────────────────────────────
// Function: cb030_uart_read / cb030_uart_write
// Desc.:    5.2b: Minimaler 68681-DUART-Ansatz — nur SRA (Status) und THRA/RHRA (Tx/Rx-Holding,
//           gleiche Adresse) sind wirklich aktiv. Alle anderen Register im UART-Adressbereich
//           werden nur sauber angenommen (Lesewert 0, Schreibzugriff verworfen), s. cb030.h.
//           SRA-Bits: TxRDY (0x04, hier immer gesetzt — q9_hal_con_put ist synchron/blockierend)
//           und RxRDY (0x01, gesetzt wenn ein Zeichen im 1-Byte-Puffer wartet). Der Lesezugriff
//           auf SRA fuellt bei Bedarf den Puffer nach (q9_hal_con_get konsumiert das Zeichen aus
//           der HAL, darum der Zwischenpuffer — sonst ginge ein Byte zwischen Status- und
//           Datenabfrage verloren).
//────────────────────────────────────────────────────────────────────────────────────────────────
static uint8_t cb030_uart_read(q9_cb030_t *b, uint32_t addr)
{
    if (addr == Q9_CB030_UART_SRA) {
        if (!b->uart_rx_pending) {
            int c = q9_hal_con_get();
            if (c >= 0) {
                b->uart_rx_pending = 1;
                b->uart_rx_char = (uint8_t)c;
            }
        }
        return (uint8_t)(0x04u | (b->uart_rx_pending ? 0x01u : 0u));
    }
    if (addr == Q9_CB030_UART_THRA) {                 /* = RHRA-Adresse beim Lesen */
        if (b->uart_rx_pending) {
            b->uart_rx_pending = 0;
            return b->uart_rx_char;
        }
        return 0;
    }
    return 0;                                          /* uebriger Registersatz: sauber angenommen */
}

static void cb030_uart_write(q9_cb030_t *b, uint32_t addr, uint8_t val)
{
    if (addr == Q9_CB030_UART_THRA) {
        q9_hal_con_put((char)val);
        return;
    }
    (void)b;                                            /* uebriger Registersatz: sauber angenommen */
}

//────────────────────────────────────────────────────────────────────────────────────────────────
// Function: cb030_cf_ensure_open
// Desc.:    5.2c: Oeffnet die Backing-Datei lazy (analog zu q9disk.img in der nativen HAL) —
//           "r+b" wenn sie existiert, sonst neu anlegen ("w+b"). Liefert 1 bei Erfolg.
//────────────────────────────────────────────────────────────────────────────────────────────────
static int cb030_cf_ensure_open(q9_cb030_t *b)
{
    if (b->cf_file) {
        return 1;
    }
    if (!b->cf_path) {
        return 0;
    }
    b->cf_file = fopen(b->cf_path, "r+b");
    if (!b->cf_file) {
        b->cf_file = fopen(b->cf_path, "w+b");
    }
    return b->cf_file != NULL;
}

//────────────────────────────────────────────────────────────────────────────────────────────────
// Function: cb030_cf_read / cb030_cf_write
// Desc.:    5.2c: ATA-PIO-Minimalprotokoll — nur READ SECTOR(S) (0x20) und WRITE SECTOR(S) (0x30),
//           je genau ein Sektor pro Kommando (Sector-Count wird angenommen, aber fuer den ersten
//           Ausbaustand nicht mehrfach durchgezaehlt). Datenregister ist 8-Bit-weise adressiert
//           (ein Byte pro Zugriff, cf_pos zaehlt 0..511 hoch).
//────────────────────────────────────────────────────────────────────────────────────────────────
static uint8_t cb030_cf_read(q9_cb030_t *b, uint32_t off)
{
    if (off == CF_REG_CMD) {
        return b->cf_status;
    }
    if (off == CF_REG_DATA) {
        if (b->cf_pos < Q9_CB030_CF_SECTOR_SIZE) {
            uint8_t v = b->cf_sector[b->cf_pos++];
            if (b->cf_pos >= Q9_CB030_CF_SECTOR_SIZE) {
                b->cf_status = (uint8_t)(Q9_CB030_CF_STAT_RDY);   /* DRQ fertig geloescht */
            }
            return v;
        }
        return 0;
    }
    return 0;                                           /* LBA/Seccnt: hier nicht rueckgelesen */
}

static void cb030_cf_write(q9_cb030_t *b, uint32_t off, uint8_t val)
{
    switch (off) {
    case CF_REG_SECCNT:
        b->cf_sectcnt = val;
        return;
    case CF_REG_LBA0:
        b->cf_lba = (b->cf_lba & 0xFFFFFF00u) | val;
        return;
    case CF_REG_LBA1:
        b->cf_lba = (b->cf_lba & 0xFFFF00FFu) | ((uint32_t)val << 8);
        return;
    case CF_REG_LBA2:
        b->cf_lba = (b->cf_lba & 0xFF00FFFFu) | ((uint32_t)val << 16);
        return;
    case CF_REG_DATA:
        if (b->cf_write_pending && b->cf_pos < Q9_CB030_CF_SECTOR_SIZE) {
            b->cf_sector[b->cf_pos++] = val;
            if (b->cf_pos >= Q9_CB030_CF_SECTOR_SIZE) {
                if (cb030_cf_ensure_open(b)) {
                    fseek(b->cf_file, (long)b->cf_lba * Q9_CB030_CF_SECTOR_SIZE, SEEK_SET);
                    fwrite(b->cf_sector, 1, Q9_CB030_CF_SECTOR_SIZE, b->cf_file);
                    fflush(b->cf_file);
                }
                b->cf_write_pending = 0;
                b->cf_status = Q9_CB030_CF_STAT_RDY;
            }
        }
        return;
    case CF_REG_CMD:
        if (val == Q9_CB030_CF_CMD_READ) {
            memset(b->cf_sector, 0, Q9_CB030_CF_SECTOR_SIZE);
            if (cb030_cf_ensure_open(b)) {
                fseek(b->cf_file, (long)b->cf_lba * Q9_CB030_CF_SECTOR_SIZE, SEEK_SET);
                fread(b->cf_sector, 1, Q9_CB030_CF_SECTOR_SIZE, b->cf_file);
            }
            b->cf_pos = 0;
            b->cf_write_pending = 0;
            b->cf_status = (uint8_t)(Q9_CB030_CF_STAT_RDY | Q9_CB030_CF_STAT_DRQ);
        } else if (val == Q9_CB030_CF_CMD_WRITE) {
            b->cf_pos = 0;
            b->cf_write_pending = 1;
            b->cf_status = (uint8_t)(Q9_CB030_CF_STAT_RDY | Q9_CB030_CF_STAT_DRQ);
        } else {
            b->cf_status = (uint8_t)(Q9_CB030_CF_STAT_RDY | Q9_CB030_CF_STAT_ERR);
        }
        return;
    default:
        return;                                          /* uebrige Register: sauber angenommen */
    }
}

//────────────────────────────────────────────────────────────────────────────────────────────────
// Function: cb030_read_byte
// Desc.:    Adress-Dispatch fuer einen einzelnen Lesezugriff (if/else-Kette aus docs/CB030.md).
//           I/O (REMAP/Timer/CF/UART) wird VOR der Zustandsweiche geprueft — die I/O-Region ist
//           in beiden REMAP-Zustaenden erreichbar (das Boot-ROM initialisiert die DUART vor dem
//           Remap). Ein Treffer im REMAP-Registerbereich schaltet immer um, unabhaengig vom
//           bisherigen Zustand oder vom gelesenen Wert (0). TI_IRQ_ON/OFF sind reine Adress-
//           Trigger (5.2d) — auch beim Lesen wirksam.
// Call:     v = cb030_read_byte(b, addr)
//────────────────────────────────────────────────────────────────────────────────────────────────
static uint8_t cb030_read_byte(q9_cb030_t *b, uint32_t addr)
{
    if (cb030_is_remap_reg(addr)) {
        b->remapped = 1;
        return 0;
    }
    if (addr >= Q9_CB030_TIRQ_OFF_BASE && addr <= Q9_CB030_TIRQ_OFF_TOP) {
        b->timer_active = 0;
        return 0;
    }
    if (addr >= Q9_CB030_TIRQ_ON_BASE && addr <= Q9_CB030_TIRQ_ON_TOP) {
        b->timer_active = 1;
        return 0;
    }
    if (addr >= Q9_CB030_CF_BASE && addr <= Q9_CB030_CF_TOP) {
        return cb030_cf_read(b, addr - Q9_CB030_CF_BASE);
    }
    if (addr >= Q9_CB030_UART_BASE && addr <= Q9_CB030_UART_TOP) {
        return cb030_uart_read(b, addr);
    }

    if (!b->remapped) {
        /* Reset-Zustand: noch kein RAM sichtbar, ROM gespiegelt bis zum oberen Byte des
           Adressraums (0xFEFF_FFFF einschl., docs/CB030.md Speicherkarte) — das Boot-ROM
           springt darum vor dem REMAP-Trigger hoch nach 0xFE00_xxxx. */
        if (addr <= Q9_CB030_ROM_MIRROR_TOP && b->rom_len > 0) {
            return b->rom[addr % b->rom_len];
        }
        return 0;
    }

    /* Remap-Zustand: RAM zuerst (haeufigster Fall, s. docs/CB030.md), danach ROM (einmalig). */
    if (addr < b->ram_len) {
        return b->ram[addr];
    }
    if (addr >= Q9_CB030_ROM_REMAP_BASE && addr <= Q9_CB030_ROM_REMAP_TOP && b->rom_len > 0) {
        uint32_t off = addr - Q9_CB030_ROM_REMAP_BASE;
        return (off < b->rom_len) ? b->rom[off] : 0;
    }

    return 0;
}

//────────────────────────────────────────────────────────────────────────────────────────────────
// Function: cb030_write_byte
// Desc.:    Adress-Dispatch fuer einen einzelnen Schreibzugriff. I/O wird — wie beim Lesen — VOR
//           der Zustandsweiche geprueft (in beiden REMAP-Zustaenden erreichbar). ROM ist nie
//           beschreibbar; ein Treffer im REMAP-Registerbereich schaltet um, der Wert selbst wird
//           verworfen. Ebenso TI_IRQ_ON/OFF (5.2d, reine Adress-Trigger).
// Call:     cb030_write_byte(b, addr, val)
//────────────────────────────────────────────────────────────────────────────────────────────────
static void cb030_write_byte(q9_cb030_t *b, uint32_t addr, uint8_t val)
{
    if (cb030_is_remap_reg(addr)) {
        b->remapped = 1;
        return;
    }
    if (addr >= Q9_CB030_TIRQ_OFF_BASE && addr <= Q9_CB030_TIRQ_OFF_TOP) {
        b->timer_active = 0;
        return;
    }
    if (addr >= Q9_CB030_TIRQ_ON_BASE && addr <= Q9_CB030_TIRQ_ON_TOP) {
        b->timer_active = 1;
        return;
    }
    if (addr >= Q9_CB030_CF_BASE && addr <= Q9_CB030_CF_TOP) {
        cb030_cf_write(b, addr - Q9_CB030_CF_BASE, val);
        return;
    }
    if (addr >= Q9_CB030_UART_BASE && addr <= Q9_CB030_UART_TOP) {
        cb030_uart_write(b, addr, val);
        return;
    }

    if (!b->remapped) {
        return;                                       /* Reset-Zustand: nur ROM sichtbar, read-only */
    }

    if (addr < b->ram_len) {
        b->ram[addr] = val;
        return;
    }

    /* ROM-Bereich (read-only) und undefinierte Adressen: kommentarlos verwerfen. */
}

int q9_cb030_init(q9_cb030_t *b, const uint8_t *rom, uint32_t rom_len, uint8_t *ram, uint32_t ram_len)
{
    if (!ram || ram_len == 0) {
        return Q9_CB030_ERR_RAM;
    }

    memset(b, 0, sizeof(*b));
    b->rom     = rom;
    b->rom_len = rom_len;
    b->ram     = ram;
    b->ram_len = ram_len;
    b->remapped = 0;
    b->cf_status = Q9_CB030_CF_STAT_RDY;
    return Q9_CB030_OK;
}

void q9_cb030_reset(q9_cb030_t *b)
{
    b->remapped = 0;
}

void q9_cb030_cf_attach(q9_cb030_t *b, const char *path)
{
    b->cf_path = path;
}

int q9_cb030_rom_load(const char *path, uint8_t *buf, uint32_t buf_max, uint32_t *out_len)
{
    FILE  *f = fopen(path, "rb");
    size_t n;

    if (!f) {
        return Q9_CB030_ERR_ROM;
    }
    n = fread(buf, 1, buf_max, f);
    if (n == 0 || fgetc(f) != EOF) {                  /* leer oder groesser als der Puffer */
        fclose(f);
        return Q9_CB030_ERR_ROM;
    }
    fclose(f);
    *out_len = (uint32_t)n;
    return Q9_CB030_OK;
}

int q9_cb030_poll_timer(q9_cb030_t *b, uint32_t now_ms)
{
    if (!b->timer_active) {
        return 0;
    }
    if (now_ms - b->timer_last_ms >= Q9_CB030_TIMER_PERIOD_MS) {
        b->timer_last_ms = now_ms;
        return 1;
    }
    return 0;
}

uint8_t q9_cb030_read8(q9_cb030_t *b, uint32_t addr)
{
    return cb030_read_byte(b, addr);
}

uint16_t q9_cb030_read16(q9_cb030_t *b, uint32_t addr)
{
    uint16_t hi = cb030_read_byte(b, addr);
    uint16_t lo = cb030_read_byte(b, addr + 1);
    return (uint16_t)((hi << 8) | lo);
}

uint32_t q9_cb030_read32(q9_cb030_t *b, uint32_t addr)
{
    uint32_t b0 = cb030_read_byte(b, addr);
    uint32_t b1 = cb030_read_byte(b, addr + 1);
    uint32_t b2 = cb030_read_byte(b, addr + 2);
    uint32_t b3 = cb030_read_byte(b, addr + 3);
    return (b0 << 24) | (b1 << 16) | (b2 << 8) | b3;
}

void q9_cb030_write8(q9_cb030_t *b, uint32_t addr, uint8_t val)
{
    cb030_write_byte(b, addr, val);
}

void q9_cb030_write16(q9_cb030_t *b, uint32_t addr, uint16_t val)
{
    cb030_write_byte(b, addr, (uint8_t)(val >> 8));
    cb030_write_byte(b, addr + 1, (uint8_t)val);
}

void q9_cb030_write32(q9_cb030_t *b, uint32_t addr, uint32_t val)
{
    cb030_write_byte(b, addr, (uint8_t)(val >> 24));
    cb030_write_byte(b, addr + 1, (uint8_t)(val >> 16));
    cb030_write_byte(b, addr + 2, (uint8_t)(val >> 8));
    cb030_write_byte(b, addr + 3, (uint8_t)val);
}

//────────────────────────────────────────────────────────────────────────────────────────────────
// EOF cb030.c                                                                             Ver. 1.40
//────────────────────────────────────────────────────────────────────────────────────────────────
