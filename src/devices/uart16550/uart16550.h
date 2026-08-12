//════════════════════════════════════════════════════════════════════════════════════════════════
// File:   uart16550.h                                                                     Ver. 1.00
// Owner:  Claudia
// Desc.:  NS16550A-UART, so weit wie ein Gast sie zum Reden braucht. Registerlage und Verhalten
//         nach der QEMU-"virt"-Maschine modelliert -- das ist die De-facto-Standardmaschine der
//         RISC-V-Welt, und wer sie nachbildet, kann deren gesamte Bildersammlung benutzen
//         (xv6, Linux, Zephyr, U-Boot) UND gegen qemu-system-riscv als Referenz gegenpruefen.
//
//         BEWUSST ENDIAN-NEUTRAL und ohne Bindung an src/kernel/devreg.h: dessen Vtable
//         synthetisiert 16/32-Bit-Zugriffe BIG-ENDIAN aus read8/write8 (dort so dokumentiert,
//         68k-gepraegt). Fuer RISC-V waere das falsch herum. Die Verallgemeinerung des
//         devreg-Schemas ist Schritt 6.7 des Arbeitsplans; bis dahin bietet dieses Geraet eine
//         schmale, byteweise Schnittstelle an, die BEIDE Welten sauber bedienen koennen.
//
//         Registerlage (Abstand 1 Byte, wie bei QEMU virt -- NICHT der 4-Byte-Abstand mancher
//         SoCs; das steht im Device-Tree als "reg-shift" und ist dort 0):
//           +0  RBR (lesen) / THR (schreiben)      Zeichen empfangen / senden
//           +1  IER                                 Interruptfreigabe
//           +2  IIR (lesen) / FCR (schreiben)       Interruptkennung / FIFO-Steuerung
//           +3  LCR                                 Leitungssteuerung (Bit 7 = DLAB)
//           +4  MCR                                 Modemsteuerung
//           +5  LSR                                 Leitungsstatus (Bit 0 = Zeichen da, Bit 5/6 = Sender frei)
//           +6  MSR                                 Modemstatus
//           +7  SCR                                 Notizregister
//         Bei gesetztem DLAB liegen auf +0/+1 stattdessen die Teilerwerte (DLL/DLM). Die werden
//         angenommen und aufbewahrt, aber nicht ausgewertet: die Baudrate hat in einer Emulation
//         ohne echte Leitung keine Bedeutung -- der Gast konfiguriert sie trotzdem, und ein
//         UART, der dabei stolpert, ist nutzlos.
//════════════════════════════════════════════════════════════════════════════════════════════════
#ifndef Q9_UART16550_H
#define Q9_UART16550_H

#include <stdint.h>

#define Q9_UART16550_SIZE   8u        /* belegtes Adressfenster in Byte */

/* Ausgabe: der Wirt entscheidet, wohin ein gesendetes Zeichen geht (stdout, Puffer, Netz).
   Eingabe: liefert -1, wenn gerade kein Zeichen anliegt. Beide duerfen NULL sein. */
typedef void (*q9_uart_tx_fn)(void *opaque, uint8_t ch);
typedef int  (*q9_uart_rx_fn)(void *opaque);

typedef struct {
    uint8_t ier, lcr, mcr, scr, fcr;
    uint8_t dll, dlm;                 /* aufbewahrt, nicht ausgewertet -- s. Kopf */
    q9_uart_tx_fn tx;
    q9_uart_rx_fn rx;
    void *opaque;
} q9_uart16550_t;

void    q9_uart16550_init(q9_uart16550_t *u, q9_uart_tx_fn tx, q9_uart_rx_fn rx, void *opaque);
uint8_t q9_uart16550_read8 (q9_uart16550_t *u, uint32_t reg);
void    q9_uart16550_write8(q9_uart16550_t *u, uint32_t reg, uint8_t val);

#endif /* Q9_UART16550_H */
// EOF uart16550.h                                                                          Ver. 1.00
