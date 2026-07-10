//════════════════════════════════════════════════════════════════════════════════════════════════
// File:   cb030.c                                                                         Ver. 1.50
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
// 26-07-05│ 1.50 │ 5.5a: CF-Multi-Sektor — READ/WRITE SECTOR(S) zaehlen cf_sectcnt jetzt      │ CF
//         │      │ echt durch (0 = 256 Sektoren), Puffer wird pro Sektor nachgeladen/          │
//         │      │ geschrieben, DRQ bleibt bis zum letzten Sektor gesetzt                     │
// 26-07-10│ 1.60 │ 5.7: SRA-TxRDY/TxEMT sind kein Immer-Bereit-Fake mehr, sondern spiegeln     │ CF
//         │      │ den Fuellstand des HAL-TX-Ringpuffers (q9_hal_con_tx_ready/tx_empty)        │
//═════════╧══════╧════════════════════════════════════════════════════════════════════════╧══════
#include "cb030.h"
#include "../hal/q9_hal.h"
#include <stdlib.h>
#include <string.h>

/* 5.2c: interne ATA-Registeroffsets relativ zu Q9_CB030_CF_BASE. Nur das Minimum, das das
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
// Function: cb030_uart_poll_rx
// Desc.:    Leert alle momentan verfuegbaren Host-Terminalzeichen in den emulierten RX-FIFO.
//           Das ist wichtig fuer Copy/Paste: der Host kann viele Bytes auf einmal liefern,
//           OS-9 liest sie aber ueber die 68681 zeichenweise aus RHRA.
//────────────────────────────────────────────────────────────────────────────────────────────────
static int cb030_uart_rx_has_data(const q9_cb030_t *b)
{
    return b->uart_rx_count != 0;
}

static int cb030_uart_rx_push(q9_cb030_t *b, uint8_t c)
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

static uint8_t cb030_uart_rx_pop(q9_cb030_t *b)
{
    uint8_t c;

    if (!cb030_uart_rx_has_data(b)) {
        return 0;
    }
    c = b->uart_rx_fifo[b->uart_rx_tail];
    b->uart_rx_tail = (b->uart_rx_tail + 1u) % b->uart_rx_fifo_size;
    b->uart_rx_count--;
    return c;
}

static void cb030_uart_poll_rx(q9_cb030_t *b)
{
    while (b->uart_rx_fifo && b->uart_rx_count < b->uart_rx_fifo_size) {
        int c = q9_hal_con_get();
        if (c < 0) {
            break;
        }
        cb030_uart_rx_push(b, (uint8_t)c);
    }
}

//────────────────────────────────────────────────────────────────────────────────────────────────
// Function: cb030_uart_read / cb030_uart_write
// Desc.:    5.2b/5.4: 68681-DUART, Registerkarte s. docs/CB030.md. Kanal A ist die Konsole
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
/* Debug-Werkzeug (5.4): mit -DQ9_CB030_UART_TRACE uebersetzt, protokolliert jeder UART-Zugriff
   Offset+Wert auf stderr — damit wurde der sc68681-Treiber-Init beim ersten OS-9-Boot
   durchleuchtet (IVR-Readback, IMR-Sequenz). Im normalen Build komplett wegkompiliert. */
#ifdef Q9_CB030_UART_TRACE
#include <stdio.h>
#define UART_TRACE(...) fprintf(stderr, __VA_ARGS__)
#else
#define UART_TRACE(...)
#endif

static int cb030_cf_trace_enabled(void)
{
    static int cached = -1;
    if (cached < 0) {
        cached = getenv("Q9_CB030_CF_TRACE") != 0;
    }
    return cached;
}

#define CF_TRACE(...) do { if (cb030_cf_trace_enabled()) fprintf(stderr, __VA_ARGS__); } while (0)

static uint8_t cb030_uart_read(q9_cb030_t *b, uint32_t addr)
{
    UART_TRACE("[uart rd %02x]", (unsigned)(addr - Q9_CB030_UART_BASE));
    switch (addr - Q9_CB030_UART_BASE) {
    case 0x00:                                         /* MRA (MR1A/MR2A, interner Zeiger)       */
    {
        uint8_t v = b->uart_mr_a[b->uart_mr_ptr_a];
        b->uart_mr_ptr_a = 1;
        return v;
    }
    case 0x02:                                         /* SRA: 5.7 -- TxRDY/TxEMT ehrlich */
    {
        uint8_t sr = 0;
        cb030_uart_poll_rx(b);
        if (cb030_uart_rx_has_data(b)) {
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
        cb030_uart_poll_rx(b);
        return cb030_uart_rx_pop(b);
    case 0x0A:                                         /* ISR (Polling-Bits, s.o.)               */
        cb030_uart_poll_rx(b);
        return (uint8_t)(0x11u | (cb030_uart_rx_has_data(b) ? 0x02u : 0u));
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

static void cb030_uart_write(q9_cb030_t *b, uint32_t addr, uint8_t val)
{
    UART_TRACE("[uart wr %02x=%02x]", (unsigned)(addr - Q9_CB030_UART_BASE), val);
    switch (addr - Q9_CB030_UART_BASE) {
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

int q9_cb030_uart_irq_pending(q9_cb030_t *b)
{
    cb030_uart_poll_rx(b);

    if (b->uart_imr & 0x01u) {                         /* TxRDYA-Interrupt: 5.7 -- ehrlich pruefen */
        if (q9_hal_con_tx_ready()) {
            return 1;
        }
    }
    if (b->uart_imr & 0x02u) {                         /* RxRDYA-Interrupt: Zeichen da?          */
        if (cb030_uart_rx_has_data(b)) {
            return 1;
        }
    }
    return 0;
}



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
    if (b->cf_file && b->cf_image_sector_size == 0) {
        uint8_t hdr[Q9_CB030_CF_SECTOR_SIZE];
        size_t  n;
        long    cur;

        b->cf_image_sector_size = Q9_CB030_CF_SECTOR_SIZE;
        cur = ftell(b->cf_file);
        fseek(b->cf_file, 0, SEEK_SET);
        n = fread(hdr, 1, sizeof(hdr), b->cf_file);
        fseek(b->cf_file, cur, SEEK_SET);

        if (n >= 128) {
            uint32_t root_lsn = ((uint32_t)hdr[RBF_DD_DIR] << 16) |
                                ((uint32_t)hdr[RBF_DD_DIR + 1] << 8) |
                                 (uint32_t)hdr[RBF_DD_DIR + 2];
            uint16_t lsn_size = (uint16_t)(((uint16_t)hdr[RBF_DD_LSNSIZE] << 8) |
                                            (uint16_t)hdr[RBF_DD_LSNSIZE + 1]);
            if (lsn_size == 256u) {
                b->cf_image_sector_size = 256u;
            } else if (root_lsn > 0) {
                uint8_t fd0 = 0;
                fseek(b->cf_file, (long)root_lsn * 256L, SEEK_SET);
                if (fread(&fd0, 1, 1, b->cf_file) == 1 && (fd0 & 0x80u)) {
                    b->cf_image_sector_size = 256u;
                }
                fseek(b->cf_file, cur, SEEK_SET);
            }
        }
        CF_TRACE("[cf image-sector-size=%u]\n", b->cf_image_sector_size);
    }
    return b->cf_file != NULL;
}

//────────────────────────────────────────────────────────────────────────────────────────────────
// Function: cb030_cf_load_sector / cb030_cf_store_sector
// Desc.:    5.5a: Ein einzelner Sektor-Transfer zwischen Backing-Datei und cf_sector, an der
//           aktuellen b->cf_lba. Ausgelagert aus cb030_cf_write, weil READ/WRITE SECTOR(S) jetzt
//           mehrere Sektoren hintereinander bedienen (s. cb030_cf_read/write unten) und pro
//           Sektor genau dieselben zwei Operationen brauchen.
// Call:     cb030_cf_load_sector(b); ... cb030_cf_store_sector(b);
//────────────────────────────────────────────────────────────────────────────────────────────────
static void cb030_cf_load_sector(q9_cb030_t *b)
{
    uint32_t img_sec;

    memset(b->cf_sector, 0, Q9_CB030_CF_SECTOR_SIZE);
    if (cb030_cf_ensure_open(b)) {
        img_sec = b->cf_image_sector_size ? b->cf_image_sector_size : Q9_CB030_CF_SECTOR_SIZE;
        fseek(b->cf_file, (long)b->cf_lba * (long)img_sec, SEEK_SET);
        fread(b->cf_sector, 1, img_sec, b->cf_file);
    }
    if (b->cf_lba == 0 || cb030_cf_trace_enabled()) {
        CF_TRACE("[cf read lba=%u first=%02x %02x %02x %02x %02x %02x %02x %02x]\n",
                 b->cf_lba, b->cf_sector[0], b->cf_sector[1], b->cf_sector[2], b->cf_sector[3],
                 b->cf_sector[4], b->cf_sector[5], b->cf_sector[6], b->cf_sector[7]);
    }
    b->cf_pos = 0;
}

static void cb030_cf_load_write_buffer(q9_cb030_t *b)
{
    uint32_t img_sec;

    memset(b->cf_sector, 0, Q9_CB030_CF_SECTOR_SIZE);
    if (cb030_cf_ensure_open(b)) {
        img_sec = b->cf_image_sector_size ? b->cf_image_sector_size : Q9_CB030_CF_SECTOR_SIZE;
        fseek(b->cf_file, (long)b->cf_lba * (long)img_sec, SEEK_SET);
        fread(b->cf_sector, 1, img_sec, b->cf_file);
        if (img_sec < Q9_CB030_CF_SECTOR_SIZE) {
            fseek(b->cf_file, (long)(b->cf_lba + 1u) * (long)img_sec, SEEK_SET);
            fread(b->cf_sector + img_sec, 1, Q9_CB030_CF_SECTOR_SIZE - img_sec, b->cf_file);
        }
    }
    b->cf_pos = 0;
}

static void cb030_cf_store_sector(q9_cb030_t *b)
{
    if (cb030_cf_ensure_open(b)) {
        uint32_t img_sec = b->cf_image_sector_size ? b->cf_image_sector_size : Q9_CB030_CF_SECTOR_SIZE;
        uint32_t written = b->cf_pos;
        if (written == 0 || written > Q9_CB030_CF_SECTOR_SIZE) {
            written = Q9_CB030_CF_SECTOR_SIZE;
        }
        fseek(b->cf_file, (long)b->cf_lba * (long)img_sec, SEEK_SET);
        
        /* For 256-byte images: write TWO sectors (matching the read behavior in load_write_buffer) */
        if (img_sec < Q9_CB030_CF_SECTOR_SIZE) {
            fwrite(b->cf_sector, 1, img_sec, b->cf_file);
            fseek(b->cf_file, (long)(b->cf_lba + 1u) * (long)img_sec, SEEK_SET);
            fwrite(b->cf_sector + img_sec, 1, Q9_CB030_CF_SECTOR_SIZE - img_sec, b->cf_file);
        } else {
            fwrite(b->cf_sector, 1, written < img_sec ? written : img_sec, b->cf_file);
        }
        fflush(b->cf_file);
    }
    CF_TRACE("[cf write lba=%u first=%02x %02x %02x %02x %02x %02x %02x %02x]\n",
             b->cf_lba, b->cf_sector[0], b->cf_sector[1], b->cf_sector[2], b->cf_sector[3],
             b->cf_sector[4], b->cf_sector[5], b->cf_sector[6], b->cf_sector[7]);
}

static void cb030_cf_identify(q9_cb030_t *b)
{
    uint32_t sectors = 0;

    memset(b->cf_sector, 0, Q9_CB030_CF_SECTOR_SIZE);
    if (cb030_cf_ensure_open(b)) {
        uint8_t hdr[16];
        long cur;

        cur = ftell(b->cf_file);
        fseek(b->cf_file, 0, SEEK_SET);
        if (fread(hdr, 1, sizeof(hdr), b->cf_file) == sizeof(hdr)) {
            uint32_t dd_tot = ((uint32_t)hdr[0] << 16) |
                              ((uint32_t)hdr[1] << 8) |
                               (uint32_t)hdr[2];
            if (dd_tot > 0) {
                sectors = dd_tot;
            }
        }
        fseek(b->cf_file, cur, SEEK_SET);

        fseek(b->cf_file, 0, SEEK_END);
        cur = ftell(b->cf_file);
        if (sectors == 0 && cur > 0) {
            uint32_t img_sec = b->cf_image_sector_size ? b->cf_image_sector_size : Q9_CB030_CF_SECTOR_SIZE;
            sectors = (uint32_t)((unsigned long)cur / img_sec);
        }
    }

    /* ATA identify words are little-endian on the data port. Word 0 = 0 means a regular ATA
       device; words 60/61 report total LBA28 sectors. */
    b->cf_sector[120] = (uint8_t)(sectors & 0xFFu);
    b->cf_sector[121] = (uint8_t)((sectors >> 8) & 0xFFu);
    b->cf_sector[122] = (uint8_t)((sectors >> 16) & 0xFFu);
    b->cf_sector[123] = (uint8_t)((sectors >> 24) & 0xFFu);
    b->cf_pos = 0;
    b->cf_transfer_size = Q9_CB030_CF_SECTOR_SIZE;
    CF_TRACE("[cf identify sectors=%u]\n", sectors);
}

static uint32_t cb030_cf_read_transfer_size(q9_cb030_t *b)
{
    uint32_t size;

    if (!cb030_cf_ensure_open(b)) {
        return Q9_CB030_CF_SECTOR_SIZE;
    }
    size = b->cf_image_sector_size ? b->cf_image_sector_size : Q9_CB030_CF_SECTOR_SIZE;
    return (size < Q9_CB030_CF_SECTOR_SIZE) ? size : Q9_CB030_CF_SECTOR_SIZE;
}

static uint32_t cb030_cf_write_transfer_size(q9_cb030_t *b)
{
    uint32_t size;

    if (!cb030_cf_ensure_open(b)) {
        return Q9_CB030_CF_SECTOR_SIZE;
    }
    size = b->cf_image_sector_size ? b->cf_image_sector_size : Q9_CB030_CF_SECTOR_SIZE;
    return (size < Q9_CB030_CF_SECTOR_SIZE) ? size : Q9_CB030_CF_SECTOR_SIZE;
}

//────────────────────────────────────────────────────────────────────────────────────────────────
// Function: cb030_cf_read / cb030_cf_write
// Desc.:    5.2c/5.5a: ATA-PIO-Minimalprotokoll — READ SECTOR(S) (0x20) und WRITE SECTOR(S)
//           (0x30) zaehlen den Sector-Count jetzt echt durch (cf_remaining, 0 in CF_REG_SECCNT
//           bedeutet 256 Sektoren, ATA-Konvention): nach jedem vollen Sektor wird die LBA
//           weitergezaehlt und — solange noch Sektoren ausstehen — der naechste Sektor
//           nachgeladen (Read) bzw. angenommen (Write), DRQ bleibt dabei gesetzt; erst beim
//           letzten Sektor wird DRQ geloescht (Read) bzw. cf_write_pending beendet (Write).
//           Datenregister ist 8-Bit-weise adressiert (ein Byte pro Zugriff, cf_pos zaehlt
//           0..511 pro Sektor hoch).
//────────────────────────────────────────────────────────────────────────────────────────────────
static uint8_t cb030_cf_read(q9_cb030_t *b, uint32_t off)
{
    if (off == CF_REG_CMD) {
        return b->cf_status;
    }
    if (off == CF_REG_DATA) {
        uint32_t xfer_size = b->cf_transfer_size ? b->cf_transfer_size : Q9_CB030_CF_SECTOR_SIZE;
        if (b->cf_pos < xfer_size) {
            uint8_t v = b->cf_sector[b->cf_pos++];
            if (b->cf_pos >= xfer_size) {
                b->cf_remaining--;
                b->cf_lba++;
                if (b->cf_remaining > 0) {
                    cb030_cf_load_sector(b);              /* naechster Sektor, DRQ bleibt gesetzt */
                    b->cf_status = (uint8_t)(Q9_CB030_CF_STAT_RDY | Q9_CB030_CF_STAT_DRQ);
                } else {
                    b->cf_status = (uint8_t)(Q9_CB030_CF_STAT_RDY);   /* DRQ fertig geloescht */
                }
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
    case CF_REG_FEAT:
        return;                                           /* 8-bit feature wird beim Kommando angenommen */
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
    case CF_REG_LBA3:
        b->cf_lba3 = val;
        b->cf_lba = (b->cf_lba & 0x00FFFFFFu) | ((uint32_t)(val & 0x0Fu) << 24);
        return;
    case CF_REG_DATA:
        if (b->cf_write_pending && b->cf_pos < b->cf_transfer_size) {
            b->cf_sector[b->cf_pos++] = val;
            if (b->cf_pos >= b->cf_transfer_size) {
                cb030_cf_store_sector(b);
                b->cf_remaining--;
                b->cf_lba++;
                if (b->cf_remaining > 0) {
                    b->cf_pos = 0;                        /* naechster Sektor, DRQ bleibt gesetzt */
                    b->cf_status = (uint8_t)(Q9_CB030_CF_STAT_RDY | Q9_CB030_CF_STAT_DRQ);
                } else {
                    b->cf_write_pending = 0;
                    b->cf_status = Q9_CB030_CF_STAT_RDY;
                }
            }
        }
        return;
    case CF_REG_CMD:
        CF_TRACE("[cf cmd=%02x count=%u lba=%u lba3=%02x]\n",
                 val, b->cf_sectcnt ? b->cf_sectcnt : 256u, b->cf_lba, b->cf_lba3);
        if (val == Q9_CB030_CF_CMD_READ) {
            b->cf_remaining = b->cf_sectcnt ? b->cf_sectcnt : 256u;
            b->cf_transfer_size = cb030_cf_read_transfer_size(b);
            cb030_cf_load_sector(b);
            b->cf_write_pending = 0;
            b->cf_status = (uint8_t)(Q9_CB030_CF_STAT_RDY | Q9_CB030_CF_STAT_DRQ);
        } else if (val == Q9_CB030_CF_CMD_WRITE) {
            b->cf_remaining = b->cf_sectcnt ? b->cf_sectcnt : 256u;
            b->cf_transfer_size = cb030_cf_write_transfer_size(b);
            cb030_cf_load_write_buffer(b);
            b->cf_write_pending = 1;
            b->cf_status = (uint8_t)(Q9_CB030_CF_STAT_RDY | Q9_CB030_CF_STAT_DRQ);
        } else if (val == Q9_CB030_CF_CMD_SETFEAT) {
            /* SET FEATURES (z.B. 8-Bit-Mode, den der CB030-Boot-Treiber setzt): kommentarlos
               annehmen — unser Datenregister ist ohnehin byteweise (s. cb030_cf_read). */
            b->cf_status = Q9_CB030_CF_STAT_RDY;
        } else if (val == CF_CMD_IDENTIFY) {
            b->cf_remaining = 1;
            cb030_cf_identify(b);
            b->cf_write_pending = 0;
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
    b->uart_rx_fifo = (uint8_t *)malloc(Q9_CB030_UART_RX_FIFO_SIZE);
    if (!b->uart_rx_fifo) {
        return Q9_CB030_ERR_RAM;
    }
    b->uart_rx_fifo_size = Q9_CB030_UART_RX_FIFO_SIZE;
    b->cf_status = Q9_CB030_CF_STAT_RDY;
    b->uart_ivr  = 0x0F;                              /* 68681-Reset-Wert "uninitialisierter
                                                         Vektor" — der OS-9-Treiber sc68681
                                                         prueft GENAU darauf (sonst E$BMode) */
    return Q9_CB030_OK;
}

void q9_cb030_reset(q9_cb030_t *b)
{
    b->remapped = 0;
}

void q9_cb030_cf_attach(q9_cb030_t *b, const char *path)
{
    if (b->cf_file) {
        fclose(b->cf_file);
    }
    b->cf_path = path;
    b->cf_file = NULL;
    b->cf_image_sector_size = 0;
    b->cf_lba = 0;
    b->cf_lba3 = 0;
    b->cf_sectcnt = 0;
    b->cf_pos = 0;
    b->cf_transfer_size = 0;
    b->cf_write_pending = 0;
    b->cf_remaining = 0;
    b->cf_status = Q9_CB030_CF_STAT_RDY;
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
    if (addr == Q9_CB030_CF_BASE) {
        uint16_t hi = cb030_cf_read(b, CF_REG_DATA);
        uint16_t lo = cb030_cf_read(b, CF_REG_DATA);
        return (uint16_t)((hi << 8) | lo);
    }
    uint16_t hi = cb030_read_byte(b, addr);
    uint16_t lo = cb030_read_byte(b, addr + 1);
    return (uint16_t)((hi << 8) | lo);
}

uint32_t q9_cb030_read32(q9_cb030_t *b, uint32_t addr)
{
    if (addr == Q9_CB030_CF_BASE) {
        uint32_t b0 = cb030_cf_read(b, CF_REG_DATA);
        uint32_t b1 = cb030_cf_read(b, CF_REG_DATA);
        uint32_t b2 = cb030_cf_read(b, CF_REG_DATA);
        uint32_t b3 = cb030_cf_read(b, CF_REG_DATA);
        return (b0 << 24) | (b1 << 16) | (b2 << 8) | b3;
    }
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
    if (addr == Q9_CB030_CF_BASE) {
        cb030_cf_write(b, CF_REG_DATA, (uint8_t)(val >> 8));
        cb030_cf_write(b, CF_REG_DATA, (uint8_t)val);
        return;
    }
    cb030_write_byte(b, addr, (uint8_t)(val >> 8));
    cb030_write_byte(b, addr + 1, (uint8_t)val);
}

void q9_cb030_write32(q9_cb030_t *b, uint32_t addr, uint32_t val)
{
    if (addr == Q9_CB030_CF_BASE) {
        cb030_cf_write(b, CF_REG_DATA, (uint8_t)(val >> 24));
        cb030_cf_write(b, CF_REG_DATA, (uint8_t)(val >> 16));
        cb030_cf_write(b, CF_REG_DATA, (uint8_t)(val >> 8));
        cb030_cf_write(b, CF_REG_DATA, (uint8_t)val);
        return;
    }
    cb030_write_byte(b, addr, (uint8_t)(val >> 24));
    cb030_write_byte(b, addr + 1, (uint8_t)(val >> 16));
    cb030_write_byte(b, addr + 2, (uint8_t)(val >> 8));
    cb030_write_byte(b, addr + 3, (uint8_t)val);
}

//────────────────────────────────────────────────────────────────────────────────────────────────
// EOF cb030.c                                                                             Ver. 1.40
//────────────────────────────────────────────────────────────────────────────────────────────────
