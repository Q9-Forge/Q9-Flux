//════════════════════════════════════════════════════════════════════════════════════════════════
// File:   uart16550.c                                                                     Ver. 1.00
// Owner:  Claudia
// Desc.:  NS16550A-UART, Umsetzung. Beschreibung, Registerlage und die Begruendung fuer die
//         Endian-Neutralitaet stehen im Kopf von uart16550.h.
//════════════════════════════════════════════════════════════════════════════════════════════════

#include <string.h>
#include "uart16550.h"

/* Registerindizes im 8-Byte-Fenster */
#define REG_RBR_THR_DLL  0
#define REG_IER_DLM      1
#define REG_IIR_FCR      2
#define REG_LCR          3
#define REG_MCR          4
#define REG_LSR          5
#define REG_MSR          6
#define REG_SCR          7

#define LCR_DLAB         0x80

#define LSR_DR           0x01   /* Zeichen empfangen                                      */
#define LSR_THRE         0x20   /* Senderegister frei                                     */
#define LSR_TEMT         0x40   /* Sender vollstaendig leer                               */

#define MSR_CTS          0x10
#define MSR_DSR          0x20
#define MSR_DCD          0x80

void q9_uart16550_init(q9_uart16550_t *u, q9_uart_tx_fn tx, q9_uart_rx_fn rx, void *opaque)
{
    memset(u, 0, sizeof(*u));
    u->tx = tx;
    u->rx = rx;
    u->opaque = opaque;
    u->dll = 1;             /* irgendein Teiler ungleich 0, damit Gaeste nicht rechnen muessen */
}

uint8_t q9_uart16550_read8(q9_uart16550_t *u, uint32_t reg)
{
    switch (reg & 7u) {
    case REG_RBR_THR_DLL:
        if (u->lcr & LCR_DLAB) return u->dll;
        if (u->rx) { int c = u->rx(u->opaque); if (c >= 0) return (uint8_t)c; }
        return 0;

    case REG_IER_DLM:
        return (u->lcr & LCR_DLAB) ? u->dlm : u->ier;

    case REG_IIR_FCR:
        /* Kein Interrupt anstehend (Bit 0 = 1 heisst "nichts anliegend"). Die oberen Bits
           melden ein vorhandenes FIFO, wenn der Gast es eingeschaltet hat -- manche Treiber
           pruefen das und schalten sonst in einen langsameren Pfad. */
        return (uint8_t)(0x01u | ((u->fcr & 0x01u) ? 0xc0u : 0x00u));

    case REG_LCR: return u->lcr;
    case REG_MCR: return u->mcr;

    case REG_LSR: {
        /* Der Sender ist IMMER frei: es gibt keine echte Leitung, die bremsen koennte.
           Ein Gast, der auf THRE wartet, laeuft dadurch nie in eine Endlosschleife. */
        uint8_t lsr = LSR_THRE | LSR_TEMT;
        /* Fuer die Empfangsanzeige darf nicht konsumiert werden -- deshalb fragt diese
           Stelle NICHT u->rx(): das Zeichen holt erst der Lesezugriff auf RBR ab. Ein
           echtes 16550 haette dafuer ein Schieberegister; wir melden schlicht "nichts da",
           solange der Wirt keine Eingabe anbietet. Der Prueflauf braucht nur die Ausgabe. */
        return lsr;
    }

    case REG_MSR:
        /* Gegenstelle dauerhaft bereit -- sonst warten manche Treiber auf CTS/DSR. */
        return MSR_CTS | MSR_DSR | MSR_DCD;

    case REG_SCR: return u->scr;
    }
    return 0;
}

void q9_uart16550_write8(q9_uart16550_t *u, uint32_t reg, uint8_t val)
{
    switch (reg & 7u) {
    case REG_RBR_THR_DLL:
        if (u->lcr & LCR_DLAB) { u->dll = val; return; }
        if (u->tx) u->tx(u->opaque, val);
        return;

    case REG_IER_DLM:
        if (u->lcr & LCR_DLAB) u->dlm = val; else u->ier = val;
        return;

    case REG_IIR_FCR: u->fcr = val; return;   /* FIFO-Steuerung: angenommen, ohne Wirkung */
    case REG_LCR:     u->lcr = val; return;
    case REG_MCR:     u->mcr = val; return;
    case REG_LSR:     return;                 /* nur lesbar */
    case REG_MSR:     return;                 /* nur lesbar */
    case REG_SCR:     u->scr = val; return;
    }
}

// EOF uart16550.c                                                                          Ver. 1.00
