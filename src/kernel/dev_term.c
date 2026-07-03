//════════════════════════════════════════════════════════════════════════════════════════════════
// File:   dev_term.c                                                                      Ver. 1.00
// Owner:  AF
// Desc.:  Konsolen-Treiber /term als internes Modul (OS-9-Vorbild: SCF + scf-Treiber).
//         Zeilen-Editierung (Echo, Backspace), CR -> CR+LF beim Schreiben, Roh-Lesen ohne Echo.
//         Zustand liegt im Static Storage des Geräts — Logik stammt aus syscall.c (Phase 1.2).
//
// Call:   über die q9_drv_term-Ops, registriert von q9_dev_init() (device.c)
//
// Edition History
//─────────┬──────┬────────────────────────────────────────────────────────────────────────┬──────
// Date    │ Ver. │ Description                                                            │ By
//─────────┼──────┼────────────────────────────────────────────────────────────────────────┼──────
// 26-07-03│ 1.00 │ Initiale Version: read/write/readln/writln, Zeilenpuffer im Storage    │ CF
//═════════╧══════╧════════════════════════════════════════════════════════════════════════╧══════

#include "../hal/q9_hal.h"
#include "device.h"
#include "syscall.h"

#define TERM_LINEBUF 256

//╔══════════════════════════════════════════════════════════════════════════════════════════════╗
//║ STATIC STORAGE                                                                               ║
//╚══════════════════════════════════════════════════════════════════════════════════════════════╝
//  Der Eingabe-Zeilenpuffer gehört zum Gerät (wie SCF-Static-Storage), nicht zum Pfad —
//  alle Pfade auf /term teilen sich die eine physische Konsole.

typedef struct term_state {
    uint8_t  linebuf[TERM_LINEBUF];                    /* line assembly buffer                   */
    uint32_t linelen;
    int      line_done;                                /* complete line waiting for pickup       */
} term_state_t;

static term_state_t term_state;

//╔══════════════════════════════════════════════════════════════════════════════════════════════╗
//║ INTERNAL HELPERS                                                                             ║
//╚══════════════════════════════════════════════════════════════════════════════════════════════╝

static void con_put_cooked(uint8_t c)
{
    if (c == '\r' || c == '\n') {                      /* line end -> CR+LF on the console       */
        q9_hal_con_put('\r');
        q9_hal_con_put('\n');
    } else {
        q9_hal_con_put((char)c);
    }
}

//────────────────────────────────────────────────────────────────────────────────────────────────
// Function: poll_line
// Desc.:    Sammelt Konsolen-Zeichen in den Zeilenpuffer (Echo + Backspace, SCF-Verhalten).
//           Setzt line_done, sobald CR/LF eintrifft. Nicht blockierend.
// Call:     poll_line(st)
//────────────────────────────────────────────────────────────────────────────────────────────────
static void poll_line(term_state_t *st)
{
    int c;

    while (!st->line_done && (c = q9_hal_con_get()) >= 0) {
        if (c == '\r' || c == '\n') {
            st->linebuf[st->linelen++] = '\r';         /* OS-9 line terminator is CR             */
            st->line_done = 1;
            con_put_cooked('\r');
        } else if (c == 0x08 || c == 0x7f) {           /* backspace / delete                     */
            if (st->linelen > 0) {
                st->linelen--;
                q9_hal_con_put('\b');
                q9_hal_con_put(' ');
                q9_hal_con_put('\b');
            }
        } else if (st->linelen < TERM_LINEBUF - 1) {
            st->linebuf[st->linelen++] = (uint8_t)c;
            q9_hal_con_put((char)c);                   /* echo                                   */
        }
    }
}

//╔══════════════════════════════════════════════════════════════════════════════════════════════╗
//║ DRIVER OPERATIONS                                                                            ║
//╚══════════════════════════════════════════════════════════════════════════════════════════════╝

//────────────────────────────────────────────────────────────────────────────────────────────────
// Function: term_init
// Desc.:    Hängt das Static Storage ans Gerät und setzt den Zeilenzustand zurück.
// Call:     device.c: dev_add()
//────────────────────────────────────────────────────────────────────────────────────────────────
static int term_init(q9_dev_t *dev)
{
    term_state.linelen   = 0;
    term_state.line_done = 0;
    dev->storage = &term_state;
    return 0;
}

//────────────────────────────────────────────────────────────────────────────────────────────────
// Function: term_read
// Desc.:    Rohe Zeichen ohne Echo, soviel wie da ist (max. *n). E$NotRdy, wenn nichts ansteht.
// Call:     Treiber-Op read
//────────────────────────────────────────────────────────────────────────────────────────────────
static int term_read(q9_dev_t *dev, uint8_t *buf, uint32_t *n)
{
    uint32_t max = *n;
    uint32_t got = 0;
    int      c;

    (void)dev;
    while (got < max && (c = q9_hal_con_get()) >= 0) {
        buf[got++] = (uint8_t)c;
    }
    if (got == 0) {
        return E_NOTRDY;
    }
    *n = got;
    return 0;
}

//────────────────────────────────────────────────────────────────────────────────────────────────
// Function: term_readln
// Desc.:    Zeilenweise mit Echo/Editierung, liefert Zeile inkl. CR.
//           E$NotRdy solange die Zeile nicht komplett ist (Phase 1: kein Blockieren).
// Call:     Treiber-Op readln
//────────────────────────────────────────────────────────────────────────────────────────────────
static int term_readln(q9_dev_t *dev, uint8_t *buf, uint32_t *n)
{
    term_state_t *st  = (term_state_t *)dev->storage;
    uint32_t      max = *n;
    uint32_t      len;

    poll_line(st);
    if (!st->line_done) {
        return E_NOTRDY;
    }
    len = (st->linelen < max) ? st->linelen : max;
    for (uint32_t i = 0; i < len; i++) {
        buf[i] = st->linebuf[i];
    }
    st->linelen   = 0;
    st->line_done = 0;
    *n = len;
    return 0;
}

//────────────────────────────────────────────────────────────────────────────────────────────────
// Function: term_write
// Desc.:    Schreibt *n Bytes gekocht (CR/LF -> CR+LF) auf die Konsole.
// Call:     Treiber-Op write
//────────────────────────────────────────────────────────────────────────────────────────────────
static int term_write(q9_dev_t *dev, const uint8_t *buf, uint32_t *n)
{
    (void)dev;
    for (uint32_t i = 0; i < *n; i++) {
        con_put_cooked(buf[i]);
    }
    return 0;
}

//────────────────────────────────────────────────────────────────────────────────────────────────
// Function: term_writln
// Desc.:    Wie write, stoppt aber nach dem ersten CR/LF (inklusive). *n = geschrieben.
// Call:     Treiber-Op writln
//────────────────────────────────────────────────────────────────────────────────────────────────
static int term_writln(q9_dev_t *dev, const uint8_t *buf, uint32_t *n)
{
    uint32_t max = *n;
    uint32_t i;

    (void)dev;
    for (i = 0; i < max; i++) {
        uint8_t c = buf[i];
        con_put_cooked(c);
        if (c == '\r' || c == '\n') {
            i++;
            break;
        }
    }
    *n = i;
    return 0;
}

//╔══════════════════════════════════════════════════════════════════════════════════════════════╗
//║ MODULE EXPORT                                                                                ║
//╚══════════════════════════════════════════════════════════════════════════════════════════════╝

const q9_drv_t q9_drv_term = {
    "term",
    term_init,
    term_read,
    term_write,
    term_readln,
    term_writln,
};

//────────────────────────────────────────────────────────────────────────────────────────────────
// EOF dev_term.c                                                                          Ver. 1.00
//────────────────────────────────────────────────────────────────────────────────────────────────
