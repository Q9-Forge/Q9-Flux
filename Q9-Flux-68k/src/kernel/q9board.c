//════════════════════════════════════════════════════════════════════════════════════════════════
// File:   q9board.c                                                                         Ver. 3.10
// Owner:  AF
// Desc.:  Implementierung der Board-Emulation, siehe q9board.h.
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
//         │      │ Remap erreichbar (Dispatch umgestellt), q9_board_rom_load neu             │
// 26-07-05│ 1.50 │ 5.5a: CF-Multi-Sektor — READ/WRITE SECTOR(S) zaehlen cf_sectcnt jetzt      │ CF
//         │      │ echt durch (0 = 256 Sektoren), Puffer wird pro Sektor nachgeladen/          │
//         │      │ geschrieben, DRQ bleibt bis zum letzten Sektor gesetzt                     │
// 26-07-10│ 1.60 │ 5.7: SRA-TxRDY/TxEMT sind kein Immer-Bereit-Fake mehr, sondern spiegeln     │ CF
//         │      │ den Fuellstand des HAL-TX-Ringpuffers (q9_hal_con_tx_ready/tx_empty)        │
// 26-07-14│ 1.70 │ 5.6: RTC72421 ($FFFFD000): board_rtc_refresh/_read — Host-Uhr als BCD-   │ CF
//         │      │ Nibbles mit S1-Latch, Schreibzugriffe im Dispatch ignoriert               │
// 26-07-14│ 1.80 │ 5.17: 68681-DUART aus dem hartkodierten Dispatch in board_read_byte/       │ CF
//         │      │ board_write_byte in die Geraete-Registry verlegt (q9_devtype_duart68681,   │
//         │      │ Instanz in m68krt.c) — Registerlogik selbst unveraendert                  │
// 26-07-14│ 1.90 │ 5.17: Compact-Flash umgezogen (q9_devtype_cf, eigene 16/32-Bit-Pfade)      │ CF
// 26-07-14│ 1.91 │ 5.17: Timer/IRQ3-Adress-Trigger umgezogen (q9_devtype_timer_irq, neues     │ CF
//         │      │ Feld timer_irq_pending fuer den transienten Poll-Merker)                   │
// 26-07-14│ 1.92 │ 5.17: RTC72421 umgezogen (q9_devtype_rtc72421) -- letztes board-internes   │ CF
//         │      │ Geraet; board_read_byte/write_byte kennen jetzt nur noch REMAP+RAM/ROM      │
// 26-07-16│ 2.00 │ 5.19a: CF-Emulation auf q9_cf_t umgestellt (mehrfach instanziierbar,        │ CF
//         │      │ Master/Slave via DEV-Bit, Format rbf/pcf steuert Sektor-Heuristik/IDENTIFY) │
// 26-08-20│ 2.10 │ Hardware-Vereinheitlichung, Pilot "cf": komplette CF-Emulation (Register-    │ Cld
//         │      │ offsets, ATA-PIO-Protokoll, RBF-Heuristik, cf_dev_*/q9_devtype_cf) nach       │
//         │      │ src/devices/cf/cf.c verschoben -- reine Verschiebung, q9_board_cf_attach      │
//         │      │ bleibt duenner Wrapper hier                                                   │
// 26-08-21│ 3.00 │ Hardware-Vereinheitlichung, Folgeschritt: DUART/RTC/Timer komplett nach        │ Cld
//         │      │ src/devices/duart68681/, src/devices/rtc72421/, src/devices/timer_irq/         │
//         │      │ verschoben (reine Verschiebung wie bei "cf") -- diese Datei ist jetzt auf die  │
//         │      │ eigentliche RAM/ROM/REMAP-Speicherlogik (5.2a) reduziert                        │
// 26-08-21│ 3.10 │ Hardware-Vereinheitlichung, Andreas' Idee: der REMAP-Trigger selbst (reiner      │ Cld
//         │      │ Adress-Trigger $FFFF8000-$FFFF8FFF) ist jetzt ein eigenes devreg-Geraet           │
//         │      │ (src/devices/remap/remap.c) -- board_read_byte/write_byte kennen nur noch         │
//         │      │ RAM/ROM, der REMAP-Check entfaellt hier (devreg dispatcht ihn vorher). Bewusst    │
//         │      │ NICHT verschoben: die RAM/ROM-Interpretation selbst (Performance-Fast-Path)        │
//═════════╧══════╧════════════════════════════════════════════════════════════════════════╧══════
#include "q9board.h"
#include <stdlib.h>
#include <string.h>

/* 2026-08-20/21: DUART/RTC/CF-Registerlogik sind nach src/devices/duart68681/duart68681.c,
   src/devices/rtc72421/rtc72421.c bzw. src/devices/cf/cf.c umgezogen, der REMAP-Trigger selbst
   (board_is_remap_reg) nach src/devices/remap/remap.c (Hardware-Vereinheitlichung) -- s. dort.
   devreg dispatcht das REMAP-Fenster jetzt VOR diesem Board-Fallback, board_read_byte/write_byte
   sehen deshalb NIE mehr eine Adresse im REMAP-Registerbereich -- der Check entfaellt hier. */
//────────────────────────────────────────────────────────────────────────────────────────────────
// Function: board_read_byte
// Desc.:    Adress-Dispatch fuer einen einzelnen Lesezugriff, NUR NOCH RAM/ROM (docs/BOARD.md) --
//           REMAP/Timer/CF/UART/RTC/nettty/... laufen inzwischen alle ueber devreg VOR diesem
//           Board-Fallback (s.o.).
// Call:     v = board_read_byte(b, addr)
//────────────────────────────────────────────────────────────────────────────────────────────────
static uint8_t board_read_byte(q9_board_t *b, uint32_t addr)
{
    if (!b->remapped) {
        /* Reset-Zustand: noch kein RAM sichtbar, ROM gespiegelt bis zum oberen Byte des
           Adressraums (0xFEFF_FFFF einschl., docs/BOARD.md Speicherkarte) — das Boot-ROM
           springt darum vor dem REMAP-Trigger hoch nach 0xFE00_xxxx. */
        if (addr <= Q9_BOARD_ROM_MIRROR_TOP && b->rom_len > 0) {
            return b->rom[addr % b->rom_len];
        }
        return 0;
    }

    /* Remap-Zustand: RAM zuerst (haeufigster Fall, s. docs/BOARD.md), danach ROM (einmalig). */
    if (addr < b->ram_len) {
        return b->ram[addr];
    }
    if (addr >= Q9_BOARD_ROM_REMAP_BASE && addr <= Q9_BOARD_ROM_REMAP_TOP && b->rom_len > 0) {
        uint32_t off = addr - Q9_BOARD_ROM_REMAP_BASE;
        return (off < b->rom_len) ? b->rom[off] : 0;
    }

    return 0;
}

//────────────────────────────────────────────────────────────────────────────────────────────────
// Function: board_write_byte
// Desc.:    Adress-Dispatch fuer einen einzelnen Schreibzugriff, NUR NOCH RAM/ROM -- REMAP laeuft
//           inzwischen ueber devreg VOR diesem Board-Fallback (s.o.). ROM ist nie beschreibbar.
// Call:     board_write_byte(b, addr, val)
//────────────────────────────────────────────────────────────────────────────────────────────────
static void board_write_byte(q9_board_t *b, uint32_t addr, uint8_t val)
{
    if (!b->remapped) {
        return;                                       /* Reset-Zustand: nur ROM sichtbar, read-only */
    }

    if (addr < b->ram_len) {
        b->ram[addr] = val;
        return;
    }

    /* ROM-Bereich (read-only) und undefinierte Adressen: kommentarlos verwerfen. */
}

int q9_board_init(q9_board_t *b, const uint8_t *rom, uint32_t rom_len, uint8_t *ram, uint32_t ram_len)
{
    if (!ram || ram_len == 0) {
        return Q9_BOARD_ERR_RAM;
    }

    memset(b, 0, sizeof(*b));
    b->rom     = rom;
    b->rom_len = rom_len;
    b->ram     = ram;
    b->ram_len = ram_len;
    b->remapped = 0;
    b->uart_rx_fifo = (uint8_t *)malloc(Q9_BOARD_UART_RX_FIFO_SIZE);
    if (!b->uart_rx_fifo) {
        return Q9_BOARD_ERR_RAM;
    }
    b->uart_rx_fifo_size = Q9_BOARD_UART_RX_FIFO_SIZE;
    b->cf.status = Q9_BOARD_CF_STAT_RDY;
    b->uart_ivr  = 0x0F;                              /* 68681-Reset-Wert "uninitialisierter
                                                         Vektor" — der OS-9-Treiber sc68681
                                                         prueft GENAU darauf (sonst E$BMode) */
    return Q9_BOARD_OK;
}

void q9_board_reset(q9_board_t *b)
{
    b->remapped = 0;
}

/* 2026-08-20: q9_cf_attach/q9_cf_set_start_sector sind nach src/devices/cf/cf.c umgezogen
   (Hardware-Vereinheitlichung, Pilot "cf") -- dieser duenne Wrapper bleibt hier, ruft q9_cf_attach
   jetzt ueber cf.h auf (transitiv per q9board.h eingebunden). */
void q9_board_cf_attach(q9_board_t *b, const char *path)
{
    q9_cf_attach(&b->cf, 0, path, Q9_CF_FMT_AUTO);
}

int q9_board_rom_load(const char *path, uint8_t *buf, uint32_t buf_max, uint32_t *out_len)
{
    FILE  *f = fopen(path, "rb");
    size_t n;

    if (!f) {
        return Q9_BOARD_ERR_ROM;
    }
    n = fread(buf, 1, buf_max, f);
    if (n == 0 || fgetc(f) != EOF) {                  /* leer oder groesser als der Puffer */
        fclose(f);
        return Q9_BOARD_ERR_ROM;
    }
    fclose(f);
    *out_len = (uint32_t)n;
    return Q9_BOARD_OK;
}

/* 2026-08-21: q9_board_poll_timer ist nach src/devices/timer_irq/timer_irq.c umgezogen (dort jetzt
   `static`, s. dortiger Kommentar). */

uint8_t q9_board_read8(q9_board_t *b, uint32_t addr)
{
    return board_read_byte(b, addr);
}

uint16_t q9_board_read16(q9_board_t *b, uint32_t addr)
{
    /* 5.17: CF hat einen eigenen Word/Long-Pfad (q9_devtype_cf, s.u.) — wird ueber die
       Geraete-Registry in m68krt.c VOR diesem Board-Fallback abgefangen, erreicht diese
       Funktion also nicht mehr. */
    uint16_t hi = board_read_byte(b, addr);
    uint16_t lo = board_read_byte(b, addr + 1);
    return (uint16_t)((hi << 8) | lo);
}

uint32_t q9_board_read32(q9_board_t *b, uint32_t addr)
{
    uint32_t b0 = board_read_byte(b, addr);
    uint32_t b1 = board_read_byte(b, addr + 1);
    uint32_t b2 = board_read_byte(b, addr + 2);
    uint32_t b3 = board_read_byte(b, addr + 3);
    return (b0 << 24) | (b1 << 16) | (b2 << 8) | b3;
}

void q9_board_write8(q9_board_t *b, uint32_t addr, uint8_t val)
{
    board_write_byte(b, addr, val);
}

void q9_board_write16(q9_board_t *b, uint32_t addr, uint16_t val)
{
    board_write_byte(b, addr, (uint8_t)(val >> 8));
    board_write_byte(b, addr + 1, (uint8_t)val);
}

void q9_board_write32(q9_board_t *b, uint32_t addr, uint32_t val)
{
    board_write_byte(b, addr, (uint8_t)(val >> 24));
    board_write_byte(b, addr + 1, (uint8_t)(val >> 16));
    board_write_byte(b, addr + 2, (uint8_t)(val >> 8));
    board_write_byte(b, addr + 3, (uint8_t)val);
}


/* 2026-08-20/21: cf_dev_* / q9_devtype_cf, duart_dev_* / q9_devtype_duart68681, timer_dev_* /
   q9_devtype_timer_irq und rtc_dev_* / q9_devtype_rtc72421 sind nach src/devices/cf/cf.c,
   src/devices/duart68681/duart68681.c, src/devices/timer_irq/timer_irq.c bzw.
   src/devices/rtc72421/rtc72421.c umgezogen (Hardware-Vereinheitlichung) -- s. dort. Diese Datei
   enthaelt jetzt nur noch die reine RAM/ROM/REMAP-Speicherlogik (5.2a) plus q9_board_init/_reset/
   _rom_load und die duenne q9_board_cf_attach-Bruecke. */

//────────────────────────────────────────────────────────────────────────────────────────────────
// EOF q9board.c                                                                             Ver. 3.00
//────────────────────────────────────────────────────────────────────────────────────────────────
