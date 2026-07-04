//════════════════════════════════════════════════════════════════════════════════════════════════
// File:   cb030.c                                                                         Ver. 1.00
// Owner:  AF
// Desc.:  Implementierung der CB030-Board-Speicherlogik, siehe cb030.h.
//
// Edition History
//─────────┬──────┬────────────────────────────────────────────────────────────────────────┬──────
// Date    │ Ver. │ Description                                                            │ By
//─────────┼──────┼────────────────────────────────────────────────────────────────────────┼──────
// 26-07-04│ 1.00 │ 5.2a: Erster Grundbaustein                                              │ CF
//═════════╧══════╧════════════════════════════════════════════════════════════════════════╧══════
#include "cb030.h"
#include <string.h>

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
// Function: cb030_read_byte
// Desc.:    Adress-Dispatch fuer einen einzelnen Lesezugriff (Kern der if/else-Kette aus
//           docs/CB030.md, RAM zuerst geprueft). Ein Treffer im REMAP-Registerbereich schaltet
//           immer um, unabhaengig vom bisherigen Zustand oder vom gelesenen Wert (0).
// Call:     v = cb030_read_byte(b, addr)
//────────────────────────────────────────────────────────────────────────────────────────────────
static uint8_t cb030_read_byte(q9_cb030_t *b, uint32_t addr)
{
    if (cb030_is_remap_reg(addr)) {
        b->remapped = 1;
        return 0;
    }

    if (!b->remapped) {
        /* Reset-Zustand: noch kein RAM sichtbar, ROM gespiegelt bis zur Mirror-Grenze. */
        if (addr < Q9_CB030_ROM_MIRROR_LIMIT && b->rom_len > 0) {
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

    /* Peripherie (UART/CF/Timer) ist erst 5.2b-d dran; bis dahin liest der I/O-Bereich 0. */
    return 0;
}

//────────────────────────────────────────────────────────────────────────────────────────────────
// Function: cb030_write_byte
// Desc.:    Adress-Dispatch fuer einen einzelnen Schreibzugriff. ROM ist nie beschreibbar; ein
//           Treffer im REMAP-Registerbereich schaltet um, der Wert selbst wird verworfen.
// Call:     cb030_write_byte(b, addr, val)
//────────────────────────────────────────────────────────────────────────────────────────────────
static void cb030_write_byte(q9_cb030_t *b, uint32_t addr, uint8_t val)
{
    if (cb030_is_remap_reg(addr)) {
        b->remapped = 1;
        return;
    }

    if (!b->remapped) {
        return;                                       /* Reset-Zustand: nur ROM sichtbar, read-only */
    }

    if (addr < b->ram_len) {
        b->ram[addr] = val;
        return;
    }

    /* ROM-Bereich (read-only) und Peripherie (5.2b-d): Schreibzugriff wird kommentarlos verworfen. */
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
    return Q9_CB030_OK;
}

void q9_cb030_reset(q9_cb030_t *b)
{
    b->remapped = 0;
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
// EOF cb030.c                                                                             Ver. 1.00
//────────────────────────────────────────────────────────────────────────────────────────────────
