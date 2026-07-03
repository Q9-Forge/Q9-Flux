//════════════════════════════════════════════════════════════════════════════════════════════════
// File:   syscall.c                                                                       Ver. 1.30
// Owner:  AF
// Desc.:  Q9 Syscall-Dispatcher + Phase-1-Implementierungen. I/O läuft über das Device-Modell
//         (device.c, Pfadtabelle) statt fest verdrahteter Pfade. Semantik: docs/SYSCALLS.md
//
// Call:   über q9_syscall(func, &regs), siehe syscall.h
//
// Edition History
//─────────┬──────┬────────────────────────────────────────────────────────────────────────┬──────
// Date    │ Ver. │ Description                                                            │ By
//─────────┼──────┼────────────────────────────────────────────────────────────────────────┼──────
// 26-07-03│ 1.00 │ Initiale Version: Dispatcher, I$Read/Write/ReadLn/WritLn, F$Exit/ID    │ CF
// 26-07-03│ 1.10 │ 1.3: I/O über Device-Modell/Pfadtabelle, Mode-Check (E$BMode)          │ CF
// 26-07-03│ 1.20 │ 1.4: I$Dup + I$Close                                                   │ CF
// 26-07-03│ 1.30 │ 1.5: F$PrsNam + F$CmpNam                                               │ CF
//═════════╧══════╧════════════════════════════════════════════════════════════════════════╧══════

#include "../hal/q9_hal.h"
#include "device.h"
#include "name.h"
#include "syscall.h"

//╔══════════════════════════════════════════════════════════════════════════════════════════════╗
//║ KERNEL STATE (Phase 1: one proto process)                                                    ║
//╚══════════════════════════════════════════════════════════════════════════════════════════════╝

static int      proc_halted = 0;                       /* set by F$Exit until real processes     */
static uint32_t boot_ticks;

//╔══════════════════════════════════════════════════════════════════════════════════════════════╗
//║ INTERNAL HELPERS                                                                             ║
//╚══════════════════════════════════════════════════════════════════════════════════════════════╝

//────────────────────────────────────────────────────────────────────────────────────────────────
// Function: path_check
// Desc.:    Pfadnummer (d0.w) + Puffer (a0) prüfen: Pfad offen, Modus erlaubt, Adresse gültig.
//           Liefert den Deskriptor über *out, Rückgabe 0 oder Fehlercode.
// Call:     err = path_check(r, Q9_MODE_WRITE, &p)
//────────────────────────────────────────────────────────────────────────────────────────────────
static int path_check(q9_regs_t *r, uint8_t need_mode, q9_path_t **out)
{
    q9_path_t *p = q9_path_get(r->d[0] & 0xffffu);

    if (!p) {
        return E_BPNUM;
    }
    if (!(p->mode & need_mode)) {
        return E_BMODE;
    }
    if (!r->a[0]) {
        return E_BPADDR;
    }
    *out = p;
    return 0;
}

//╔══════════════════════════════════════════════════════════════════════════════════════════════╗
//║ SYSCALL IMPLEMENTATIONS                                                                      ║
//╚══════════════════════════════════════════════════════════════════════════════════════════════╝

//────────────────────────────────────────────────────────────────────────────────────────────────
// Function: sc_write
// Desc.:    I$Write/I$WritLn — d0.w Pfad, a0 Puffer, d1.l Anzahl; out: d1.l geschrieben.
//           Delegiert an die write/writln-Op des Treibers hinter dem Pfad.
// Call:     Dispatcher
//────────────────────────────────────────────────────────────────────────────────────────────────
static int sc_write(q9_regs_t *r, int line_mode)
{
    q9_path_t *p;
    uint32_t   n = r->d[1];
    int        err;

    err = path_check(r, Q9_MODE_WRITE, &p);
    if (err != 0) {
        return err;
    }
    err = line_mode ? p->dev->drv->writln(p->dev, (const uint8_t *)r->a[0], &n)
                    : p->dev->drv->write(p->dev, (const uint8_t *)r->a[0], &n);
    if (err != 0) {
        return err;
    }
    r->d[1] = n;
    return 0;
}

//────────────────────────────────────────────────────────────────────────────────────────────────
// Function: sc_read
// Desc.:    I$Read/I$ReadLn — d0.w Pfad, a0 Puffer, d1.l max; out: d1.l gelesen.
//           Delegiert an die read/readln-Op des Treibers (E$NotRdy-Semantik siehe SYSCALLS.md).
// Call:     Dispatcher
//────────────────────────────────────────────────────────────────────────────────────────────────
static int sc_read(q9_regs_t *r, int line_mode)
{
    q9_path_t *p;
    uint32_t   n = r->d[1];
    int        err;

    err = path_check(r, Q9_MODE_READ, &p);
    if (err != 0) {
        return err;
    }
    if (line_mode && n == 0) {
        return E_BPADDR;
    }
    err = line_mode ? p->dev->drv->readln(p->dev, (uint8_t *)r->a[0], &n)
                    : p->dev->drv->read(p->dev, (uint8_t *)r->a[0], &n);
    if (err != 0) {
        return err;
    }
    r->d[1] = n;
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
    case I_READ:    return sc_read(r, 0);
    case I_READLN:  return sc_read(r, 1);

    case I_DUP: {                                      /* d0.w path -> d0.w new path             */
        int np = q9_path_dup(r->d[0] & 0xffffu);
        if (np < 0) {
            return -np;
        }
        r->d[0] = (uint32_t)np;
        return 0;
    }

    case I_CLOSE:                                      /* d0.w path                              */
        return q9_path_close(r->d[0] & 0xffffu);

    case F_PRSNAM: {                                   /* a0 pathlist -> a0 name, a1 past-end,   */
        const char *start;                             /*   d1.w len, d0.b delimiter char        */
        uint32_t    len;
        int         err;
        if (!r->a[0]) {
            return E_BPADDR;
        }
        err = q9_name_parse((const char *)r->a[0], &start, &len);
        if (err != 0) {
            return err;
        }
        r->a[0] = (void *)start;
        r->a[1] = (void *)(start + len);
        r->d[0] = (uint32_t)(uint8_t)start[len];
        r->d[1] = len;
        return 0;
    }

    case F_CMPNAM:                                     /* a0 name1, d1.w len, a1 name2           */
        if (!r->a[0] || !r->a[1]) {
            return E_BPADDR;
        }
        return q9_name_cmp((const char *)r->a[0], r->d[1] & 0xffffu, (const char *)r->a[1]);

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
// EOF syscall.c                                                                           Ver. 1.30
//────────────────────────────────────────────────────────────────────────────────────────────────
