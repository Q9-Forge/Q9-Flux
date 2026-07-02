//════════════════════════════════════════════════════════════════════════════════════════════════
// File:   syscall.c                                                                       Ver. 1.00
// Owner:  AF
// Desc.:  Q9 Syscall-Dispatcher + Phase-1-Implementierungen (Konsolen-I/O auf Pfaden 0/1/2,
//         F$Exit, F$ID, F$Time provisorisch). Semantik-Details: docs/SYSCALLS.md
//
// Call:   über q9_syscall(func, &regs), siehe syscall.h
//
// Edition History
//─────────┬──────┬────────────────────────────────────────────────────────────────────────┬──────
// Date    │ Ver. │ Description                                                            │ By
//─────────┼──────┼────────────────────────────────────────────────────────────────────────┬──────
// 26-07-03│ 1.00 │ Initiale Version: Dispatcher, I$Read/Write/ReadLn/WritLn, F$Exit/ID    │ CF
//═════════╧══════╧════════════════════════════════════════════════════════════════════════╧══════

#include "../hal/q9_hal.h"
#include "syscall.h"

#define LINEBUF_SIZE 256

//╔══════════════════════════════════════════════════════════════════════════════════════════════╗
//║ KERNEL STATE (Phase 1: one proto process, console paths 0/1/2)                               ║
//╚══════════════════════════════════════════════════════════════════════════════════════════════╝

static uint8_t  linebuf[LINEBUF_SIZE];                 /* I$ReadLn line assembly buffer          */
static uint32_t linelen   = 0;
static int      line_done = 0;                         /* complete line waiting for pickup       */
static int      proc_halted = 0;                       /* set by F$Exit until real processes     */
static uint32_t boot_ticks;

//╔══════════════════════════════════════════════════════════════════════════════════════════════╗
//║ INTERNAL HELPERS                                                                             ║
//╚══════════════════════════════════════════════════════════════════════════════════════════════╝

static int is_console_path(uint32_t path)
{
    return path <= 2;                                  /* 0/1/2 hardwired until phase 3          */
}

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
// Call:     poll_line()
//────────────────────────────────────────────────────────────────────────────────────────────────
static void poll_line(void)
{
    int c;

    while (!line_done && (c = q9_hal_con_get()) >= 0) {
        if (c == '\r' || c == '\n') {
            linebuf[linelen++] = '\r';                 /* OS-9 line terminator is CR             */
            line_done = 1;
            con_put_cooked('\r');
        } else if (c == 0x08 || c == 0x7f) {           /* backspace / delete                     */
            if (linelen > 0) {
                linelen--;
                q9_hal_con_put('\b');
                q9_hal_con_put(' ');
                q9_hal_con_put('\b');
            }
        } else if (linelen < LINEBUF_SIZE - 1) {
            linebuf[linelen++] = (uint8_t)c;
            q9_hal_con_put((char)c);                   /* echo                                   */
        }
    }
}

//╔══════════════════════════════════════════════════════════════════════════════════════════════╗
//║ SYSCALL IMPLEMENTATIONS                                                                      ║
//╚══════════════════════════════════════════════════════════════════════════════════════════════╝

//────────────────────────────────────────────────────────────────────────────────────────────────
// Function: sc_write
// Desc.:    I$Write/I$WritLn — d0.w Pfad, a0 Puffer, d1.l Anzahl; out: d1.l geschrieben.
//           WritLn stoppt nach CR/LF (inklusive).
// Call:     Dispatcher
//────────────────────────────────────────────────────────────────────────────────────────────────
static int sc_write(q9_regs_t *r, int line_mode)
{
    const uint8_t *buf = (const uint8_t *)r->a[0];
    uint32_t       max = r->d[1];
    uint32_t       n;

    if (!is_console_path(r->d[0] & 0xffffu)) {
        return E_BPNUM;
    }
    if (!buf) {
        return E_BPADDR;
    }
    for (n = 0; n < max; n++) {
        uint8_t c = buf[n];
        con_put_cooked(c);
        if (line_mode && (c == '\r' || c == '\n')) {
            n++;
            break;
        }
    }
    r->d[1] = n;
    return 0;
}

//────────────────────────────────────────────────────────────────────────────────────────────────
// Function: sc_read
// Desc.:    I$Read — rohe Zeichen ohne Echo, soviel wie da ist (max d1.l).
//           Liefert E$NotRdy, wenn nichts ansteht (Phase 1: kein Blockieren, siehe SYSCALLS.md).
// Call:     Dispatcher
//────────────────────────────────────────────────────────────────────────────────────────────────
static int sc_read(q9_regs_t *r)
{
    uint8_t *buf = (uint8_t *)r->a[0];
    uint32_t max = r->d[1];
    uint32_t n   = 0;
    int      c;

    if (!is_console_path(r->d[0] & 0xffffu)) {
        return E_BPNUM;
    }
    if (!buf) {
        return E_BPADDR;
    }
    while (n < max && (c = q9_hal_con_get()) >= 0) {
        buf[n++] = (uint8_t)c;
    }
    if (n == 0) {
        return E_NOTRDY;
    }
    r->d[1] = n;
    return 0;
}

//────────────────────────────────────────────────────────────────────────────────────────────────
// Function: sc_readln
// Desc.:    I$ReadLn — zeilenweise mit Echo/Editierung, liefert Zeile inkl. CR.
//           E$NotRdy solange die Zeile nicht komplett ist (Phase 1: kein Blockieren).
// Call:     Dispatcher
//────────────────────────────────────────────────────────────────────────────────────────────────
static int sc_readln(q9_regs_t *r)
{
    uint8_t *buf = (uint8_t *)r->a[0];
    uint32_t max = r->d[1];
    uint32_t n;

    if (!is_console_path(r->d[0] & 0xffffu)) {
        return E_BPNUM;
    }
    if (!buf || max == 0) {
        return E_BPADDR;
    }
    poll_line();
    if (!line_done) {
        return E_NOTRDY;
    }
    n = (linelen < max) ? linelen : max;
    for (uint32_t i = 0; i < n; i++) {
        buf[i] = linebuf[i];
    }
    r->d[1]   = n;
    linelen   = 0;
    line_done = 0;
    return 0;
}

//╔══════════════════════════════════════════════════════════════════════════════════════════════╗
//║ DISPATCHER                                                                                   ║
//╚══════════════════════════════════════════════════════════════════════════════════════════════╝

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_syscall
// Desc.:    Zentraler Dispatcher — verteilt auf die F$/I$-Implementierungen.
// Call:     err = q9_syscall(I_WRITLN, &regs)
//════════════════════════════════════════════════════════════════════════════════════════════════
int q9_syscall(uint16_t func, q9_regs_t *r)
{
    static int ticks_init = 0;

    if (!r) {
        return E_BPADDR;
    }
    if (!ticks_init) {
        boot_ticks = q9_hal_ticks_ms();
        ticks_init = 1;
    }

    switch (func) {
    case I_WRITE:   return sc_write(r, 0);
    case I_WRITLN:  return sc_write(r, 1);
    case I_READ:    return sc_read(r);
    case I_READLN:  return sc_readln(r);

    case F_EXIT:                                       /* real semantics arrive with phase 4     */
        proc_halted = 1;
        return 0;

    case F_ID:
        r->d[0] = 1;                                   /* proto process                          */
        r->d[1] = 0;                                   /* super user                             */
        return 0;

    case F_TIME: {                                     /* provisional: uptime, no RTC yet        */
        uint32_t ms = q9_hal_ticks_ms() - boot_ticks;
        r->d[0] = ms / 1000u;
        r->d[3] = ms;
        return 0;
    }

    default:
        return E_UNKSVC;
    }
}

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_proc_halted
// Desc.:    Liefert 1, wenn der Proto-Prozess per F$Exit beendet wurde (Übergangslösung bis
//           echte Prozesse in Phase 4 existieren).
// Call:     if (q9_proc_halted()) ...
//════════════════════════════════════════════════════════════════════════════════════════════════
int q9_proc_halted(void)
{
    return proc_halted;
}

//────────────────────────────────────────────────────────────────────────────────────────────────
// EOF syscall.c                                                                           Ver. 1.00
//────────────────────────────────────────────────────────────────────────────────────────────────
