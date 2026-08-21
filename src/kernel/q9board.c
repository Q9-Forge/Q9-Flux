//════════════════════════════════════════════════════════════════════════════════════════════════
// File:   q9board.c                                                                         Ver. 2.10
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
//═════════╧══════╧════════════════════════════════════════════════════════════════════════╧══════
#include "q9board.h"
#include "../hal/q9_hal.h"
#include <stdlib.h>
#include <string.h>

//────────────────────────────────────────────────────────────────────────────────────────────────
// Function: board_is_remap_reg
// Desc.:    Prueft, ob 'addr' im REMAP-Registerbereich liegt (reiner Adress-Trigger, s. q9board.h).
// Call:     if (board_is_remap_reg(addr)) ...
//────────────────────────────────────────────────────────────────────────────────────────────────
static int board_is_remap_reg(uint32_t addr)
{
    return addr >= Q9_BOARD_REMAP_REG_BASE && addr <= Q9_BOARD_REMAP_REG_TOP;
}

//────────────────────────────────────────────────────────────────────────────────────────────────
// Function: board_uart_poll_rx
// Desc.:    Leert alle momentan verfuegbaren Host-Terminalzeichen in den emulierten RX-FIFO.
//           Das ist wichtig fuer Copy/Paste: der Host kann viele Bytes auf einmal liefern,
//           OS-9 liest sie aber ueber die 68681 zeichenweise aus RHRA.
//────────────────────────────────────────────────────────────────────────────────────────────────
static int board_uart_rx_has_data(const q9_board_t *b)
{
    return b->uart_rx_count != 0;
}

static int board_uart_rx_push(q9_board_t *b, uint8_t c)
{
    if (!b->uart_rx_fifo || b->uart_rx_count >= b->uart_rx_fifo_size) {
        b->uart_rx_overflow++;
        return 0;
    }
    b->uart_rx_fifo[b->uart_rx_head] = c;
    b->uart_rx_head = (b->uart_rx_head + 1u) % b->uart_rx_fifo_size;
    b->uart_rx_count++;
    return 1;
}

static uint8_t board_uart_rx_pop(q9_board_t *b)
{
    uint8_t c;

    if (!board_uart_rx_has_data(b)) {
        return 0;
    }
    c = b->uart_rx_fifo[b->uart_rx_tail];
    b->uart_rx_tail = (b->uart_rx_tail + 1u) % b->uart_rx_fifo_size;
    b->uart_rx_count--;
    return c;
}

static void board_uart_poll_rx(q9_board_t *b)
{
    while (b->uart_rx_fifo && b->uart_rx_count < b->uart_rx_fifo_size) {
        int c = q9_hal_con_get();
        if (c < 0) {
            break;
        }
        board_uart_rx_push(b, (uint8_t)c);
    }
}

//────────────────────────────────────────────────────────────────────────────────────────────────
// Function: board_uart_read / board_uart_write
// Desc.:    5.2b/5.4: 68681-DUART, Registerkarte s. docs/BOARD.md. Kanal A ist die Konsole
//           (THRA -> q9_hal_con_put, RHRA <- Host-Terminal via RX-FIFO); Kanal B ist
//           unverbunden (sendet ins Leere, empfaengt nie). Die Mode-Register MR1/MR2 (einziges
//           echtes R/W-Register der 68681, interner Zeiger: nach jedem Zugriff auf MR2, Reset
//           auf MR1 per CR-Kommando 0x1x) und das IVR werden als Latches gefuehrt — der
//           OS-9-Treiber sc68681 verifiziert den Chip per Readback (sonst E$BMode beim
//           Konsolen-Open). SRA: TxRDY (0x04)/TxEMT (0x08) spiegeln seit 5.7 den HAL-TX-Ringpuffer
//           (q9_hal_con_tx_ready/tx_empty) statt immer gesetzt zu sein — manche Sende-Schleifen
//           warten auf TxEMT statt TxRDY. SRB bleibt "immer bereit" (Kanal B unverbunden, sendet
//           ins Leere, kein Host-Puffer noetig). ISR liefert die entsprechenden Polling-Bits
//           (TxRDYA 0x01, RxRDYA 0x02, TxRDYB 0x10).
//           Der Rest des Registersatzes wird sauber angenommen (liest 0, Schreiben verworfen).
//────────────────────────────────────────────────────────────────────────────────────────────────
/* Debug-Werkzeug (5.4): mit -DQ9_BOARD_UART_TRACE uebersetzt, protokolliert jeder UART-Zugriff
   Offset+Wert auf stderr — damit wurde der sc68681-Treiber-Init beim ersten OS-9-Boot
   durchleuchtet (IVR-Readback, IMR-Sequenz). Im normalen Build komplett wegkompiliert. */
#ifdef Q9_BOARD_UART_TRACE
#include <stdio.h>
#define UART_TRACE(...) fprintf(stderr, __VA_ARGS__)
#else
#define UART_TRACE(...)
#endif

//────────────────────────────────────────────────────────────────────────────────────────────────
// 5.6: RTC72421 — Epson-Echtzeituhr am Bus ($FFFFD000, 16 Nibble-Register).
// Lesen = Host-Uhr (q9_hal_time), Schreiben wird ignoriert (s. q9board.h). Register:
//   0 S1  1 S10  2 MI1  3 MI10  4 H1  5 H10  6 D1  7 D10  8 MO1  9 MO10  A Y1  B Y10  C W
//   D Control D (HOLD/BUSY/IRQ — bei uns immer 0, nie busy)   E Control E (0)
//   F Control F (Bit2 = 24h-Modus, fest gesetzt)
// Ein Lesezugriff auf Register 0 frischt den Latch auf; die uebrigen Register lesen aus dem
// Latch, damit ein Treiber-Lesedurchlauf S1..W einen konsistenten Zeitstempel sieht.
//────────────────────────────────────────────────────────────────────────────────────────────────
static void board_rtc_refresh(q9_board_t *b)
{
    q9_datetime_t dt;
    if (q9_hal_time(&dt) != 0) {
        memset(b->rtc_regs, 0, sizeof(b->rtc_regs));
        b->rtc_latch_valid = 1;
        return;
    }
    /* Wochentag nach Sakamoto, 0 = Sonntag (uebliche 72421-Konvention) */
    static const int wt[] = {0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4};
    int y = dt.year, m = dt.month, d = dt.day;
    if (m < 3) {
        y -= 1;
    }
    int w = (y + y / 4 - y / 100 + y / 400 + wt[m - 1] + d) % 7;

    b->rtc_regs[0]  = (uint8_t)(dt.sec % 10);          /* S1   */
    b->rtc_regs[1]  = (uint8_t)(dt.sec / 10);          /* S10  */
    b->rtc_regs[2]  = (uint8_t)(dt.min % 10);          /* MI1  */
    b->rtc_regs[3]  = (uint8_t)(dt.min / 10);          /* MI10 */
    b->rtc_regs[4]  = (uint8_t)(dt.hour % 10);         /* H1   */
    b->rtc_regs[5]  = (uint8_t)(dt.hour / 10);         /* H10 (24h-Modus: 0..2, kein PM-Bit)     */
    b->rtc_regs[6]  = (uint8_t)(dt.day % 10);          /* D1   */
    b->rtc_regs[7]  = (uint8_t)(dt.day / 10);          /* D10  */
    b->rtc_regs[8]  = (uint8_t)(dt.month % 10);        /* MO1  */
    b->rtc_regs[9]  = (uint8_t)(dt.month / 10);        /* MO10 */
    b->rtc_regs[10] = (uint8_t)((dt.year % 100) % 10); /* Y1 (Basis 2000, wie rtclock-Treiber)   */
    b->rtc_regs[11] = (uint8_t)((dt.year % 100) / 10); /* Y10  */
    b->rtc_regs[12] = (uint8_t)w;                      /* W    */
    b->rtc_latch_valid = 1;
}

static uint8_t board_rtc_read(q9_board_t *b, uint32_t off)
{
    if (off == 0 || !b->rtc_latch_valid) {
        board_rtc_refresh(b);
    }
    if (off <= 12) {
        return b->rtc_regs[off];
    }
    if (off == 15) {
        return 0x04;                                   /* Control F: Bit2 = 24h-Modus            */
    }
    return 0x00;                                       /* Control D/E: nie HOLD/BUSY/IRQ         */
}

static uint8_t board_uart_read(q9_board_t *b, uint32_t addr)
{
    UART_TRACE("[uart rd %02x]", (unsigned)(addr - Q9_BOARD_UART_BASE));
    switch (addr - Q9_BOARD_UART_BASE) {
    case 0x00:                                         /* MRA (MR1A/MR2A, interner Zeiger)       */
    {
        uint8_t v = b->uart_mr_a[b->uart_mr_ptr_a];
        b->uart_mr_ptr_a = 1;
        return v;
    }
    case 0x02:                                         /* SRA: 5.7 -- TxRDY/TxEMT ehrlich */
    {
        uint8_t sr = 0;
        board_uart_poll_rx(b);
        if (board_uart_rx_has_data(b)) {
            sr |= 0x01u;                                /* RxRDY                                  */
        }
        if (q9_hal_con_tx_ready()) {
            sr |= 0x04u;                                /* TxRDY: Platz fuer mind. 1 weiteres Byte */
        }
        if (q9_hal_con_tx_empty()) {
            sr |= 0x08u;                                /* TxEMT: Puffer vollstaendig geleert     */
        }
        return sr;
    }
    case 0x06:                                         /* RHRA */
        board_uart_poll_rx(b);
        return board_uart_rx_pop(b);
    case 0x0A:                                         /* ISR (Polling-Bits, s.o.)               */
        board_uart_poll_rx(b);
        return (uint8_t)(0x11u | (board_uart_rx_has_data(b) ? 0x02u : 0u));
    case 0x10:                                         /* MRB (MR1B/MR2B, interner Zeiger)       */
    {
        uint8_t v = b->uart_mr_b[b->uart_mr_ptr_b];
        b->uart_mr_ptr_b = 1;
        return v;
    }
    case 0x12:                                         /* SRB: sendet sofort, empfaengt nie      */
        return 0x0Cu;
    case 0x18:                                         /* IVR */
        return b->uart_ivr;
    default:
        return 0;                                      /* uebriger Registersatz: sauber angenommen */
    }
}

static void board_uart_write(q9_board_t *b, uint32_t addr, uint8_t val)
{
    UART_TRACE("[uart wr %02x=%02x]", (unsigned)(addr - Q9_BOARD_UART_BASE), val);
    switch (addr - Q9_BOARD_UART_BASE) {
    case 0x00:                                         /* MRA */
        b->uart_mr_a[b->uart_mr_ptr_a] = val;
        b->uart_mr_ptr_a = 1;
        return;
    case 0x04:                                         /* CRA: nur "Reset MR Pointer" (0x1x)     */
        if (((val >> 4) & 0x07u) == 1u) {
            b->uart_mr_ptr_a = 0;
        }
        return;
    case 0x06:                                         /* THRA -> Konsole                        */
        q9_hal_con_put((char)val);
        return;
    case 0x10:                                         /* MRB */
        b->uart_mr_b[b->uart_mr_ptr_b] = val;
        b->uart_mr_ptr_b = 1;
        return;
    case 0x14:                                         /* CRB: nur "Reset MR Pointer" (0x1x)     */
        if (((val >> 4) & 0x07u) == 1u) {
            b->uart_mr_ptr_b = 0;
        }
        return;
    case 0x0A:                                         /* IMR (Interrupt-Maske, write-only)      */
        b->uart_imr = val;
        return;
    case 0x16:                                         /* THRB: Kanal B unverbunden, verwerfen   */
        return;
    case 0x18:                                         /* IVR */
        b->uart_ivr = val;
        return;
    default:
        return;                                        /* uebriger Registersatz: sauber angenommen */
    }
}

int q9_board_uart_irq_pending(q9_board_t *b)
{
    board_uart_poll_rx(b);

    if (b->uart_imr & 0x01u) {                         /* TxRDYA-Interrupt: 5.7 -- ehrlich pruefen */
        if (q9_hal_con_tx_ready()) {
            return 1;
        }
    }
    if (b->uart_imr & 0x02u) {                         /* RxRDYA-Interrupt: Zeichen da?          */
        if (board_uart_rx_has_data(b)) {
            return 1;
        }
    }
    return 0;
}



/* 2026-08-20: die komplette CF-Emulation (cf_cur_unit .. board_cf_write) ist nach
   src/devices/cf/cf.c umgezogen (Hardware-Vereinheitlichung, Pilot "cf") -- s. dort. */
//────────────────────────────────────────────────────────────────────────────────────────────────
// Function: board_read_byte
// Desc.:    Adress-Dispatch fuer einen einzelnen Lesezugriff (if/else-Kette aus docs/BOARD.md).
//           I/O (REMAP/Timer/CF/UART) wird VOR der Zustandsweiche geprueft — die I/O-Region ist
//           in beiden REMAP-Zustaenden erreichbar (das Boot-ROM initialisiert die DUART vor dem
//           Remap). Ein Treffer im REMAP-Registerbereich schaltet immer um, unabhaengig vom
//           bisherigen Zustand oder vom gelesenen Wert (0). TI_IRQ_ON/OFF sind reine Adress-
//           Trigger (5.2d) — auch beim Lesen wirksam.
// Call:     v = board_read_byte(b, addr)
//────────────────────────────────────────────────────────────────────────────────────────────────
static uint8_t board_read_byte(q9_board_t *b, uint32_t addr)
{
    if (board_is_remap_reg(addr)) {
        b->remapped = 1;
        return 0;
    }
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
// Desc.:    Adress-Dispatch fuer einen einzelnen Schreibzugriff. I/O wird — wie beim Lesen — VOR
//           der Zustandsweiche geprueft (in beiden REMAP-Zustaenden erreichbar). ROM ist nie
//           beschreibbar; ein Treffer im REMAP-Registerbereich schaltet um, der Wert selbst wird
//           verworfen. Ebenso TI_IRQ_ON/OFF (5.2d, reine Adress-Trigger).
// Call:     board_write_byte(b, addr, val)
//────────────────────────────────────────────────────────────────────────────────────────────────
static void board_write_byte(q9_board_t *b, uint32_t addr, uint8_t val)
{
    if (board_is_remap_reg(addr)) {
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

int q9_board_poll_timer(q9_board_t *b, uint32_t now_ms)
{
    if (!b->timer_active) {
        return 0;
    }
    if (!b->timer_synced) {
        /* Erster Poll nach TI_IRQ_ON: sofort ausloesen und die Tick-Epoche starten
           (Verhalten wie bisher, s. Selbsttest 5.2d). */
        b->timer_synced  = 1;
        b->timer_last_ms = now_ms;
        return 1;
    }
    if (now_ms - b->timer_last_ms >= Q9_BOARD_TIMER_PERIOD_MS) {
        /* 5.6: Tick-Schulden nachholen statt verwerfen — vorher wurde timer_last_ms auf
           "jetzt" gesetzt, d.h. pro Poll hoechstens EIN Tick, egal wie viel Echtzeit
           vergangen war. Im Idle-Betrieb (STOP + Host-Schlafdrossel, 5.9) verlor die
           OS-9-Uhr dadurch fast alle Ticks und blieb praktisch stehen ('date' fror ein).
           Jetzt rueckt timer_last_ms nur um EINE Periode vor, so dass aufeinanderfolgende
           Polls die aufgelaufenen Ticks einzeln nachliefern (OS-9 zaehlt pro Interrupt
           genau einen Tick). Deckel bei 30 s Rueckstand, damit ein stundenlang
           schlafender Host keinen minutenlangen Tick-Sturm ausloest — den absoluten
           Abgleich liefert dann ohnehin die RTC (setime -s). */
        if (now_ms - b->timer_last_ms > 30000u) {
            b->timer_last_ms = now_ms - 30000u;
        }
        b->timer_last_ms += Q9_BOARD_TIMER_PERIOD_MS;
        return 1;
    }
    return 0;
}

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

//────────────────────────────────────────────────────────────────────────────────────────────────
// Function: duart_dev_* / q9_devtype_duart68681
// Desc.:    5.17: Vtable-Adapter fuer die Geraete-Registry (devreg.h). dev->state zeigt auf das
//           q9_board_t-Board; die eigentliche Registerlogik bleibt unveraendert in board_uart_read/
//           board_uart_write/q9_board_uart_irq_pending (nur der Dispatch-Aufruf wandert hierher, aus
//           dem alten if-Block in board_read_byte/board_write_byte). level_held=1: die IRQ-Leitung
//           bleibt an, bis der RX-Puffer geleert bzw. TxRDY nicht mehr ansteht -- nimmt an der
//           IACK-/Reassert-Pruefschleife in m68krt.c teil (wie vor 5.17).
//────────────────────────────────────────────────────────────────────────────────────────────────
static uint8_t duart_dev_read8(q9_device_t *dev, uint32_t addr)
{
    return board_uart_read((q9_board_t *)dev->state, addr);
}

static void duart_dev_write8(q9_device_t *dev, uint32_t addr, uint8_t val)
{
    board_uart_write((q9_board_t *)dev->state, addr, val);
}

static int duart_dev_irq_pending(q9_device_t *dev)
{
    return q9_board_uart_irq_pending((q9_board_t *)dev->state);
}

/* Der OS-9-Treiber (sc68681) programmiert seinen Vektor laufzeit ins IVR-Register (s. board_uart_
   write case 0x18) -- anders als QUICC/Netz-Terminals ist das KEIN fester, bei der Registrierung
   bekannter Wert, deshalb der dynamische Weg ueber irq_vector_fn statt des statischen
   dev->irq_vector (wie vor 5.17: m68krt_board_int_ack las direkt g_board->uart_ivr). */
static int duart_dev_irq_vector(q9_device_t *dev)
{
    return ((q9_board_t *)dev->state)->uart_ivr;
}

const q9_device_vtable_t q9_devtype_duart68681 = {
    .read8         = duart_dev_read8,
    .write8        = duart_dev_write8,
    .read16        = NULL,                            /* aus read8 synthetisiert (wie bisher)   */
    .write16       = NULL,
    .read32        = NULL,
    .write32       = NULL,
    .poll          = NULL,                            /* RX-Poll steckt im irq_pending-Aufruf   */
    .irq_pending   = duart_dev_irq_pending,
    .reset         = NULL,
    .irq_vector_fn = duart_dev_irq_vector,
};

//────────────────────────────────────────────────────────────────────────────────────────────────
// Function: timer_dev_* / q9_devtype_timer_irq
// Desc.:    5.17: Vtable-Adapter fuer die Geraete-Registry (devreg.h), drittes umgezogenes Geraet.
//           TI_IRQ_ON/OFF sind reine Adress-Trigger (kein Datenwert, Lesen wie Schreiben loesen
//           dieselbe Wirkung aus, s. q9board.h) -- read8/write8 fassen deshalb beide Fenster
//           ($FFFF9000-$FFFF97FF OFF, $FFFF9800-$FFFF9FFF ON) in EINEM Geraet zusammen und
//           unterscheiden per Adresse.
//           poll()/irq_pending() bilden den bisherigen q9boardrun.c-Aufruf ab (q9_board_poll_timer
//           liefert 1 GENAU IN DER RUNDE, in der ein Tick faellig ist): poll() ruft ihn auf und
//           merkt sich das Ergebnis transient in b->timer_irq_pending; irq_pending() liest nur
//           diesen Merker (kein erneuter Seiteneffekt). level_held=0 (s. devreg.h) haelt den
//           Timer bewusst aus der IACK-/Reassert-Pruefschleife in m68krt.c heraus -- exakt wie
//           vor 5.17 (Level 6 faellt beim IACK immer auf den Autovektor, reassert_pending_irq
//           griff nie fuer den Timer).
//────────────────────────────────────────────────────────────────────────────────────────────────
static uint8_t timer_dev_read8(q9_device_t *dev, uint32_t addr)
{
    q9_board_t *b = (q9_board_t *)dev->state;
    if (addr >= Q9_BOARD_TIRQ_ON_BASE && addr <= Q9_BOARD_TIRQ_ON_TOP) {
        b->timer_active = 1;
        b->timer_synced = 0;                          /* 5.6: Tick-Epoche neu starten            */
    } else {
        b->timer_active = 0;
    }
    return 0;
}

static void timer_dev_write8(q9_device_t *dev, uint32_t addr, uint8_t val)
{
    (void)val;
    (void)timer_dev_read8(dev, addr);
}

static void timer_dev_poll(q9_device_t *dev, uint32_t now_ms)
{
    q9_board_t *b = (q9_board_t *)dev->state;
    b->timer_irq_pending = q9_board_poll_timer(b, now_ms);
}

static int timer_dev_irq_pending(q9_device_t *dev)
{
    return ((q9_board_t *)dev->state)->timer_irq_pending;
}

const q9_device_vtable_t q9_devtype_timer_irq = {
    .read8         = timer_dev_read8,
    .write8        = timer_dev_write8,
    .read16        = NULL,
    .write16       = NULL,
    .read32        = NULL,
    .write32       = NULL,
    .poll          = timer_dev_poll,
    .irq_pending   = timer_dev_irq_pending,
    .reset         = NULL,
    .irq_vector_fn = NULL,                            /* Level 6 faellt immer zum Autovektor     */
};

//────────────────────────────────────────────────────────────────────────────────────────────────
// Function: rtc_dev_* / q9_devtype_rtc72421
// Desc.:    5.17: Vtable-Adapter fuer die Geraete-Registry (devreg.h), viertes umgezogenes Geraet
//           (LETZTES der vier board-internen Geraete -- damit kennt board_read_byte/board_write_byte
//           danach nur noch REMAP-Trigger + RAM/ROM, s. ARBEITSPLAN 5.17). board_rtc_read bleibt
//           unveraendert; Schreiben wird weiterhin komplett ignoriert (Host-Uhr ist die Wahrheit,
//           s. q9board.h). Kein IRQ.
//────────────────────────────────────────────────────────────────────────────────────────────────
static uint8_t rtc_dev_read8(q9_device_t *dev, uint32_t addr)
{
    return board_rtc_read((q9_board_t *)dev->state, addr - Q9_BOARD_RTC_BASE);
}

static void rtc_dev_write8(q9_device_t *dev, uint32_t addr, uint8_t val)
{
    (void)dev; (void)addr; (void)val;                 /* 5.6: Schreiben ignoriert (wie bisher)  */
}

const q9_device_vtable_t q9_devtype_rtc72421 = {
    .read8         = rtc_dev_read8,
    .write8        = rtc_dev_write8,
    .read16        = NULL,
    .write16       = NULL,
    .read32        = NULL,
    .write32       = NULL,
    .poll          = NULL,
    .irq_pending   = NULL,
    .reset         = NULL,
    .irq_vector_fn = NULL,
};

/* 2026-08-20: cf_dev_* / q9_devtype_cf sind nach src/devices/cf/cf.c umgezogen (Hardware-
   Vereinheitlichung, Pilot "cf") -- s. dort. */

//────────────────────────────────────────────────────────────────────────────────────────────────
// EOF q9board.c                                                                             Ver. 2.10
//────────────────────────────────────────────────────────────────────────────────────────────────
