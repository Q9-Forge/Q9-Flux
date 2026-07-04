//════════════════════════════════════════════════════════════════════════════════════════════════
// File:   syscall.c                                                                       Ver. 2.70
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
// 26-07-03│ 1.40 │ 1.6: I$Attach + I$Detach                                               │ CF
// 26-07-03│ 1.50 │ 1.8: I$GetStt + I$SetStt                                               │ CF
// 26-07-03│ 1.60 │ 1.9: F$Time echte Uhrzeit + F$STime, Kalenderlogik                     │ CF
// 26-07-03│ 1.70 │ Bugfix: F$Time/F$STime d0/d1 vertauscht (MWOS: d0=Zeit,d1=Datum)        │ CF
// 26-07-03│ 1.80 │ 2.3d: F$Link/F$UnLink ueber q9_mod_link/unlink (Modul-Directory)       │ CF
// 26-07-04│ 1.90 │ 3.2: I$Open/I$ChgDir ueber die VFS-Schicht (vfs.c); I$Create/I$MakDir/ │ CF
//         │      │ I$Delete als Geruest (E$UnkSvc, echte Semantik erst 3.4)               │ CF
// 26-07-04│ 2.00 │ 3.3: I$Read routet auf fm->read() (FAT16 ueber File-Manager statt      │ CF
//         │      │ Treiber, wenn vorhanden); I$Seek neu ueber fm->seek()                  │ CF
// 26-07-04│ 2.10 │ 3.4: I$Write routet analog zu I$Read auf fm->write(); I$Create/        │ CF
//         │      │ I$MakDir/I$Delete jetzt echt ueber q9_vfs_open + fm->create/makdir/    │ CF
//         │      │ remove (FAT16 schreibend)                                             │ CF
// 26-07-04│ 2.20 │ 3.5: F$Load ueber q9_mod_load (Modul aus Datei statt nur ROM-Image)    │ CF
// 26-07-04│ 2.30 │ 4.1: F$ID liest PID jetzt aus der Prozesstabelle (proc.c) statt fest    │ CF
//         │      │ verdrahtet (Fallback PID 1 ausserhalb eines Scheduler-Aufrufs)           │
// 26-07-04│ 2.40 │ 4.2: F$Fork/F$Wait/F$Chain echt ueber proc.c implementiert; F$Exit hat   │ CF
//         │      │ jetzt echte Semantik (q9_proc_exit statt proc_halted-Stub, der samt      │
//         │      │ q9_proc_halted() entfernt wurde)                                        │
// 26-07-04│ 2.50 │ 4.3: sc_read versetzt den aufrufenden Prozess bei E$NotRdy (Treiberpfad, │ CF
//         │      │ kein File-Manager) per q9_proc_wait_device in WAITING; F$Wait ebenso     │
//         │      │ per q9_proc_wait_child bei E$NotRdy; neu F$Sleep ueber q9_proc_sleep     │
// 26-07-04│ 2.60 │ 4.4: F$SSpd + F$SPrior neu ueber q9_proc_suspend/set_priority            │ CF
// 26-07-04│ 2.70 │ 4.5: F$Send/F$Icpt/F$RTE neu ueber q9_proc_send/icpt/rte                  │ CF
//═════════╧══════╧════════════════════════════════════════════════════════════════════════╧══════

#include "../hal/q9_hal.h"
#include "device.h"
#include "module.h"
#include "name.h"
#include "proc.h"
#include "syscall.h"
#include "vfs.h"

//╔══════════════════════════════════════════════════════════════════════════════════════════════╗
//║ KERNEL STATE                                                                                 ║
//╚══════════════════════════════════════════════════════════════════════════════════════════════╝

static uint32_t boot_ticks;
static uint32_t time_base_s;                           /* seconds since 2000-01-01 at boot       */
static int      time_have = 0;                         /* 0 = not yet initialized from HAL/STime */

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
//║ CALENDAR (Epoche 2000-01-01, ein Sonnabend; reicht per uint32 bis ins Jahr 2136)             ║
//╚══════════════════════════════════════════════════════════════════════════════════════════════╝

static const uint8_t mdays[12] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

//────────────────────────────────────────────────────────────────────────────────────────────────
// Function: is_leap
// Desc.:    Gregorianisches Schaltjahr? (durch 4 teilbar, außer durch 100, außer durch 400)
// Call:     leap = is_leap(2000)  // 1
//────────────────────────────────────────────────────────────────────────────────────────────────
static int is_leap(uint32_t y)
{
    return (y % 4 == 0 && y % 100 != 0) || y % 400 == 0;
}

//────────────────────────────────────────────────────────────────────────────────────────────────
// Function: secs_from_dt / dt_from_secs
// Desc.:    Kalenderdatum <-> Sekunden seit 2000-01-01 00:00:00 (lokale Zeit, keine Zeitzonen).
// Call:     s = secs_from_dt(&dt);   dt_from_secs(s, &dt);
//────────────────────────────────────────────────────────────────────────────────────────────────
static uint32_t secs_from_dt(const q9_datetime_t *dt)
{
    uint32_t days = 0;

    for (uint32_t y = 2000; y < dt->year; y++) {
        days += is_leap(y) ? 366u : 365u;
    }
    for (uint32_t m = 1; m < dt->month; m++) {
        days += mdays[m - 1];
        if (m == 2 && is_leap(dt->year)) {
            days++;
        }
    }
    days += dt->day - 1u;
    return ((days * 24u + dt->hour) * 60u + dt->min) * 60u + dt->sec;
}

static void dt_from_secs(uint32_t s, q9_datetime_t *dt)
{
    uint32_t days = s / 86400u;
    uint32_t rest = s % 86400u;
    uint32_t y    = 2000;
    uint32_t m    = 1;

    for (;;) {
        uint32_t ylen = is_leap(y) ? 366u : 365u;
        if (days < ylen) {
            break;
        }
        days -= ylen;
        y++;
    }
    for (;;) {
        uint32_t mlen = mdays[m - 1] + ((m == 2 && is_leap(y)) ? 1u : 0u);
        if (days < mlen) {
            break;
        }
        days -= mlen;
        m++;
    }
    dt->year  = (uint16_t)y;
    dt->month = (uint8_t)m;
    dt->day   = (uint8_t)(days + 1);
    dt->hour  = (uint8_t)(rest / 3600u);
    dt->min   = (uint8_t)((rest / 60u) % 60u);
    dt->sec   = (uint8_t)(rest % 60u);
}

//────────────────────────────────────────────────────────────────────────────────────────────────
// Function: time_init_lazy
// Desc.:    Holt die Uhrzeit einmalig aus der HAL und rechnet sie auf den Boot-Zeitpunkt
//           zurück. Ohne Zeitquelle startet die Uhr bei 2000-01-01 00:00:00.
// Call:     time_init_lazy()
//────────────────────────────────────────────────────────────────────────────────────────────────
static void time_init_lazy(void)
{
    q9_datetime_t dt;

    if (time_have) {
        return;
    }
    if (q9_hal_time(&dt) == 0 && dt.year >= 2000) {
        uint32_t uptime_s = (q9_hal_ticks_ms() - boot_ticks) / 1000u;
        time_base_s = secs_from_dt(&dt) - uptime_s;
    } else {
        time_base_s = 0;
    }
    time_have = 1;
}

//╔══════════════════════════════════════════════════════════════════════════════════════════════╗
//║ SYSCALL IMPLEMENTATIONS                                                                      ║
//╚══════════════════════════════════════════════════════════════════════════════════════════════╝

//────────────────────────────────────────────────────────────────────────────────────────────────
// Function: sc_write
// Desc.:    I$Write/I$WritLn — d0.w Pfad, a0 Puffer, d1.l Anzahl; out: d1.l geschrieben.
//           Hat das Geraet hinter dem Pfad einen File-Manager MIT write-Op (ab 3.4, FAT16), geht
//           I$Write (nicht I$WritLn — Zeilenmodus ergibt fuer Dateien keinen Sinn, analog zu
//           sc_read) an dessen fm->write(); sonst wie bisher an die write/writln-Op des Treibers.
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
    if (!line_mode && p->dev->fm && p->dev->fm->write) {
        err = p->dev->fm->write(p->dev, p, (const uint8_t *)r->a[0], &n);
    } else {
        err = line_mode ? p->dev->drv->writln(p->dev, (const uint8_t *)r->a[0], &n)
                        : p->dev->drv->write(p->dev, (const uint8_t *)r->a[0], &n);
    }
    if (err != 0) {
        return err;
    }
    r->d[1] = n;
    return 0;
}

//────────────────────────────────────────────────────────────────────────────────────────────────
// Function: sc_read
// Desc.:    I$Read/I$ReadLn — d0.w Pfad, a0 Puffer, d1.l max; out: d1.l gelesen.
//           Hat das Geraet hinter dem Pfad einen File-Manager MIT read-Op (ab 3.3, FAT16), geht
//           I$Read (nicht I$ReadLn — Zeilenmodus mit Echo/Editierung ergibt fuer Dateien keinen
//           Sinn) an dessen fm->read(); sonst wie bisher an die read/readln-Op des Treibers
//           (E$NotRdy-Semantik siehe SYSCALLS.md). 4.3: liefert der Treiberpfad E$NotRdy, wird der
//           AUFRUFENDE Prozess per q9_proc_wait_device blockiert (WAITING, Weckgrund SS.Ready des
//           Geraets) — der Rueckgabewert an den Aufrufer bleibt E$NotRdy (kein eingefrorener
//           Stack, s. proc.h), aber der Scheduler steppt ihn nicht mehr sinnlos weiter, bis
//           Eingabe ansteht. Der File-Manager-Pfad bleibt unveraendert (E$NotRdy dort ist ein
//           echter I/O-Fehler, kein "bald wieder versuchen").
// Call:     Dispatcher
//────────────────────────────────────────────────────────────────────────────────────────────────
static int sc_read(q9_regs_t *r, int line_mode)
{
    q9_path_t *p;
    uint32_t   n = r->d[1];
    int        err;
    int        via_fm;

    err = path_check(r, Q9_MODE_READ, &p);
    if (err != 0) {
        return err;
    }
    if (line_mode && n == 0) {
        return E_BPADDR;
    }
    via_fm = !line_mode && p->dev->fm && p->dev->fm->read;
    if (via_fm) {
        err = p->dev->fm->read(p->dev, p, (uint8_t *)r->a[0], &n);
    } else {
        err = line_mode ? p->dev->drv->readln(p->dev, (uint8_t *)r->a[0], &n)
                        : p->dev->drv->read(p->dev, (uint8_t *)r->a[0], &n);
    }
    if (err == E_NOTRDY && !via_fm) {
        q9_pd_t *me = q9_proc_current();
        if (me) {
            q9_proc_wait_device(me->pid, p->dev);
        }
    }
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

    case I_GETSTT:                                     /* d0.w path, d1.w SS code, Rest je Code  */
    case I_SETSTT: {
        q9_path_t *p = q9_path_get(r->d[0] & 0xffffu);
        int (*op)(q9_dev_t *, uint32_t, q9_regs_t *);
        if (!p) {
            return E_BPNUM;
        }
        op = (func == I_GETSTT) ? p->dev->drv->getstat : p->dev->drv->setstat;
        if (!op) {
            return E_UNKSVC;
        }
        return op(p->dev, r->d[1] & 0xffffu, r);
    }

    case I_ATTACH: {                                   /* d0.b mode, a0 devname -> a2 device     */
        q9_dev_t *dev;
        int       err;
        if (!r->a[0]) {
            return E_BPADDR;
        }
        err = q9_dev_attach((const char *)r->a[0], &dev);
        if (err != 0) {
            return err;
        }
        r->a[2] = dev;
        return 0;
    }

    case I_DETACH:                                     /* a2 device                              */
        return q9_dev_detach((q9_dev_t *)r->a[2]);

    case I_OPEN: {                                      /* d0.b mode, a0 pathlist -> d0.w path    */
        int p;
        if (!r->a[0]) {
            return E_BPADDR;
        }
        p = q9_vfs_open((const char *)r->a[0], (uint8_t)r->d[0]);
        if (p < 0) {
            return -p;
        }
        r->d[0] = (uint32_t)p;
        return 0;
    }

    case I_CHGDIR:                                      /* a0 pathlist                            */
        if (!r->a[0]) {
            return E_BPADDR;
        }
        return q9_vfs_chdir((const char *)r->a[0]);

    case I_SEEK: {                                       /* d0.w path, d1.l Position (absolut)     */
        q9_path_t *p = q9_path_get(r->d[0] & 0xffffu);
        if (!p) {
            return E_BPNUM;
        }
        if (!p->dev->fm || !p->dev->fm->seek) {           /* kein File-Manager -> kein Sinn (3.3)   */
            return E_UNKSVC;
        }
        return p->dev->fm->seek(p->dev, p, r->d[1]);
    }

    case I_CREATE: {                                     /* d0.b mode, a0 pathlist -> d0.w path    */
        int p;
        if (!r->a[0]) {
            return E_BPADDR;
        }
        p = q9_vfs_create((const char *)r->a[0], (uint8_t)r->d[0]);
        if (p < 0) {
            return -p;
        }
        r->d[0] = (uint32_t)p;
        return 0;
    }

    case I_MAKDIR:                                       /* a0 pathlist                            */
        if (!r->a[0]) {
            return E_BPADDR;
        }
        return q9_vfs_makdir((const char *)r->a[0]);

    case I_DELETE:                                       /* a0 pathlist                            */
        if (!r->a[0]) {
            return E_BPADDR;
        }
        return q9_vfs_remove((const char *)r->a[0]);

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

    case F_LINK: {                                      /* a0 name, d1.b type, d2.b lang ->       */
        const q9_modhdr_t *hdr;                         /*   a1 header, a2 Einsprung, d0.b rev    */
        int                 err;
        if (!r->a[0]) {
            return E_BPADDR;
        }
        err = q9_mod_link((const char *)r->a[0], (uint8_t)r->d[1], (uint8_t)r->d[2], &hdr);
        if (err != 0) {
            return err;
        }
        r->a[1] = (void *)hdr;
        r->a[2] = (uint8_t *)hdr + hdr->execoff;
        r->d[0] = hdr->rev;
        return 0;
    }

    case F_UNLINK:                                      /* a1 header (von F$Link)                 */
        if (!r->a[1]) {
            return E_BPADDR;
        }
        return q9_mod_unlink((const q9_modhdr_t *)r->a[1]);

    case F_LOAD: {                                       /* a0 pathlist -> a1 header, a2 Einsprung,*/
        const q9_modhdr_t *hdr;                          /*   d0.b rev (wie F$Link)                */
        int                 err;
        if (!r->a[0]) {
            return E_BPADDR;
        }
        err = q9_mod_load((const char *)r->a[0], &hdr);
        if (err != 0) {
            return err;
        }
        r->a[1] = (void *)hdr;
        r->a[2] = (uint8_t *)hdr + hdr->execoff;
        r->d[0] = hdr->rev;
        return 0;
    }

    case F_FORK: {                                      /* a0 name, d1.b type, d2.b lang (wie      */
        const q9_modhdr_t *hdr;                          /*   F$Link) -> d0.w Kind-PID              */
        q9_proc_step_fn    step;
        q9_pd_t           *me = q9_proc_current();
        uint32_t           newpid;
        int                err;

        if (!r->a[0]) {
            return E_BPADDR;
        }
        err = q9_mod_link((const char *)r->a[0], (uint8_t)r->d[1], (uint8_t)r->d[2], &hdr);
        if (err != 0) {
            return err;
        }
        err = q9_proc_native_entry(hdr, &step);          /* E9: nur Q9_MOD_NATIVE ausfuehrbar       */
        if (err != 0) {
            q9_mod_unlink(hdr);
            return err;
        }
        err = q9_proc_fork(me ? me->pid : 1, hdr, step, &newpid);
        if (err != 0) {
            q9_mod_unlink(hdr);
            return err;
        }
        r->d[0] = newpid;
        return 0;
    }

    case F_WAIT: {                                      /* -> d0.w Kind-PID, d1.w Exit-Code        */
        q9_pd_t *me = q9_proc_current();
        uint32_t pid;
        int32_t  code;
        int      err = q9_proc_wait(me ? me->pid : 1, &pid, &code);
        if (err != 0) {
            if (err == E_NOTRDY && me) {                /* 4.3: echt blockieren (Kind laeuft noch) */
                q9_proc_wait_child(me->pid);
            }
            return err;                                  /* E$NotRdy oder E$NoChld                   */
        }
        r->d[0] = pid;
        r->d[1] = (uint32_t)code;
        return 0;
    }

    case F_CHAIN: {                                      /* a0 name, d1.b type, d2.b lang — ersetzt */
        const q9_modhdr_t *hdr;                          /*   das eigene Modul (PID/Parent/Std-     */
        q9_proc_step_fn    step;                          /*   Pfade bleiben)                        */
        q9_pd_t           *me = q9_proc_current();
        int                err;

        if (!r->a[0]) {
            return E_BPADDR;
        }
        if (!me) {
            return E_IPRCID;                              /* F$Chain ausserhalb eines Prozesses      */
        }
        err = q9_mod_link((const char *)r->a[0], (uint8_t)r->d[1], (uint8_t)r->d[2], &hdr);
        if (err != 0) {
            return err;
        }
        err = q9_proc_native_entry(hdr, &step);
        if (err != 0) {
            q9_mod_unlink(hdr);
            return err;
        }
        if (me->module) {
            q9_mod_unlink(me->module);
        }
        me->module   = hdr;
        me->step     = step;
        me->exitcode = 0;
        return 0;
    }

    case F_EXIT: {                                       /* d1.w Status-Code                        */
        q9_pd_t *me = q9_proc_current();
        q9_proc_exit(me ? me->pid : 1, (int32_t)(int16_t)r->d[1]);
        return 0;
    }

    case F_SLEEP: {                                      /* d1.l Ticks (0 = einmal yielden), 4.3    */
        q9_pd_t *me = q9_proc_current();
        if (!me) {
            return E_IPRCID;                              /* F$Sleep ausserhalb eines Prozesses      */
        }
        q9_proc_sleep(me->pid, r->d[1]);
        return 0;
    }

    case F_ID: {                                       /* 4.1: aus der Prozesstabelle statt fest  */
        q9_pd_t *pd = q9_proc_current();               /*   verdrahtet — ausserhalb eines         */
        r->d[0] = pd ? pd->pid : 1;                    /*   Scheduler-Aufrufs (z.B. Selbsttest    */
        r->d[1] = 0;                                   /*   vor dem ersten Tick) Fallback PID 1   */
        return 0;
    }

    case F_SSPD: {                                      /* d0.w PID (0 = aufrufender Prozess), 4.4 */
        q9_pd_t *me  = q9_proc_current();
        uint32_t pid = r->d[0] & 0xffffu;
        if (pid == 0) {
            if (!me) {
                return E_IPRCID;                          /* F$SSpd(0) ausserhalb eines Prozesses    */
            }
            pid = me->pid;
        }
        return q9_proc_suspend(pid);
    }

    case F_SPRIOR: {                                     /* d0.w PID (0=Aufrufer), d1.b neue Prio,  */
        q9_pd_t *me  = q9_proc_current();                /*   -> d1.b alte Prioritaet, 4.4          */
        uint32_t pid = r->d[0] & 0xffffu;
        uint8_t  old;
        int      err;
        if (pid == 0) {
            if (!me) {
                return E_IPRCID;
            }
            pid = me->pid;
        }
        err = q9_proc_set_priority(pid, (uint8_t)r->d[1], &old);
        if (err != 0) {
            return err;
        }
        r->d[1] = old;
        return 0;
    }

    case F_SEND: {                                       /* d0.w Ziel-PID, d1.l Signal, 4.5          */
        return q9_proc_send(r->d[0] & 0xffffu, r->d[1]);
    }

    case F_ICPT: {                                       /* a0 Handler (q9_proc_step_fn), NULL =    */
        q9_pd_t *me = q9_proc_current();                  /*   deinstalliert; nur fuer sich selbst,  */
        if (!me) {                                        /*   4.5                                    */
            return E_IPRCID;
        }
        return q9_proc_icpt(me->pid, (q9_proc_step_fn)r->a[0]);
    }

    case F_RTE: {                                        /* Rueckkehr aus Intercept, nur fuer sich   */
        q9_pd_t *me = q9_proc_current();                  /*   selbst, 4.5                            */
        if (!me) {
            return E_IPRCID;
        }
        return q9_proc_rte(me->pid);
    }

    case F_TIME: {                                     /* d0 = Zeit, d1 = Datum (OS-9-Packung,   */
        q9_datetime_t dt;                              /* MWOS-verifiziert), d2.w = Wochentag    */
        uint32_t      ms = q9_hal_ticks_ms() - boot_ticks; /* (0=So), d3 = ms-Ticks               */
        uint32_t      now;
        time_init_lazy();
        now = time_base_s + ms / 1000u;
        dt_from_secs(now, &dt);
        r->d[0] = ((uint32_t)dt.hour << 16) | ((uint32_t)dt.min << 8) | dt.sec;
        r->d[1] = ((uint32_t)dt.year << 16) | ((uint32_t)dt.month << 8) | dt.day;
        r->d[2] = (6u + now / 86400u) % 7u;            /* 2000-01-01 war ein Sonnabend           */
        r->d[3] = ms;
        return 0;
    }

    case F_STIME: {                                    /* d0 = Zeit, d1 = Datum (wie F$Time)     */
        q9_datetime_t dt;
        dt.hour  = (uint8_t)(r->d[0] >> 16);
        dt.min   = (uint8_t)(r->d[0] >> 8);
        dt.sec   = (uint8_t)r->d[0];
        dt.year  = (uint16_t)(r->d[1] >> 16);
        dt.month = (uint8_t)(r->d[1] >> 8);
        dt.day   = (uint8_t)r->d[1];
        if (dt.year < 2000 || dt.month < 1 || dt.month > 12 || dt.day < 1 ||
            dt.day > mdays[dt.month - 1] + ((dt.month == 2 && is_leap(dt.year)) ? 1 : 0) ||
            dt.hour > 23 || dt.min > 59 || dt.sec > 59) {
            return E_PARAM;
        }
        time_base_s = secs_from_dt(&dt) - (q9_hal_ticks_ms() - boot_ticks) / 1000u;
        time_have   = 1;
        return 0;
    }

    default:
        return E_UNKSVC;
    }
}

//────────────────────────────────────────────────────────────────────────────────────────────────
// EOF syscall.c                                                                           Ver. 2.70
//────────────────────────────────────────────────────────────────────────────────────────────────
