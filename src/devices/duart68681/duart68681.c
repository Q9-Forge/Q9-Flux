//════════════════════════════════════════════════════════════════════════════════════════════════
// File:   duart68681.c                                                                    Ver. 1.00
// Owner:  CF
// Desc.:  Implementierung, siehe duart68681.h. Reine Verschiebung aus src/kernel/q9board.c
//         (2026-08-21, Hardware-Vereinheitlichung) -- Registerlogik UNVERAENDERT, s.
//         duart68681.h-Kopfkommentar. Neu ist nur q9_devdesc_duart68681 am Dateiende.
//
// Edition History
//─────────┬──────┬────────────────────────────────────────────────────────────────────────┬──────
// Date    │ Ver. │ Description                                                            │ By
//─────────┼──────┼────────────────────────────────────────────────────────────────────────┬──────
// 26-07-04│ 1.xx │ 5.2b/5.4/5.7/5.17: urspruenglich Teil von q9board.c, s. dortige Historie │ CF
// 26-08-21│ 1.00 │ Hardware-Vereinheitlichung: aus q9board.c hierher verschoben, neu         │ Cld
//         │      │ q9_devdesc_duart68681; q9_board_uart_irq_pending wurde `static`           │
//═════════╧══════╧════════════════════════════════════════════════════════════════════════╧══════
#include "duart68681.h"
#include "../../hal/q9_hal.h"

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

/* 2026-08-21: `static` -- kein externer Aufrufer mehr ausser der eigenen Vtable weiter unten
   (verifiziert: weder q9boardrun.c/m68krt.c noch test/tools/ riefen diese Funktion je direkt auf,
   sie war seit 5.17 nur noch ueber duart_dev_irq_pending erreicht). War zuvor q9_board_uart_irq_
   pending (extern, q9board.h) -- Name unveraendert, nur die Sichtbarkeit eingeschraenkt. */
static int q9_board_uart_irq_pending(q9_board_t *b)
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

/* 2026-08-21 (Hardware-Vereinheitlichung, Folgeschritt nach dem "cf"-Piloten): q9_devdesc_
   duart68681 -- noch OHNE extra_fields (kein Config-Schema, immer hartkodiert instanziiert, s.
   m68krt.c q9_m68krt_attach_board -- eigener, spaeterer Schritt). */
const q9_devdesc_t q9_devdesc_duart68681 = {
    .type              = "duart68681",
    .desc              = "68681-DUART (Konsole, Kanal A)",
    .vt                = &q9_devtype_duart68681,
    .use_table_default = 1,                            /* liegt im Fast-Table-Cluster              */
    .extra_fields      = NULL,
    .extra_field_count = 0,
};

//────────────────────────────────────────────────────────────────────────────────────────────────
// EOF duart68681.c                                                                        Ver. 1.00
//────────────────────────────────────────────────────────────────────────────────────────────────
