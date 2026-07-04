//════════════════════════════════════════════════════════════════════════════════════════════════
// File:   proc.c                                                                          Ver. 1.30
// Owner:  AF
// Desc.:  Q9 Prozess-Descriptor-Tabelle + Round-Robin-Scheduler (Phase 4). Siehe proc.h fuer die
//         Architekturentscheidung (E8, Step-Modell; E9, Q9_MOD_NATIVE-Funktionszeiger-Module).
//         Statische Tabelle (Q9_NPROCS, kein malloc, analog zur Geraetetabelle in device.c).
//
// Call:   siehe proc.h
//
// Edition History
//─────────┬──────┬────────────────────────────────────────────────────────────────────────┬──────
// Date    │ Ver. │ Description                                                            │ By
//─────────┼──────┼────────────────────────────────────────────────────────────────────────┼──────
// 26-07-04│ 1.00 │ 4.1: Initiale Version (Tabelle + Round-Robin-Scheduler, PID 1 = REPL)   │ CF
// 26-07-04│ 1.10 │ 4.2: q9_proc_fork/exit/wait + q9_proc_native_entry (Q9_MOD_NATIVE)      │ CF
// 26-07-04│ 1.20 │ 4.3: Tick-Zaehler + Weckgrund-Pruefung in q9_proc_schedule;             │ CF
//         │      │ q9_proc_wait_device/wait_child/sleep ersetzen das E$NotRdy-Poll-        │ CF
//         │      │ Provisorium durch echtes Blockieren (WAITING/SLEEPING)                  │ CF
// 26-07-04│ 1.30 │ 4.4: q9_proc_suspend (WAITING/Q9_WAIT_SIGNAL, ohne Weckcheck) +          │ CF
//         │      │ q9_proc_set_priority (priority-Feld, reine Bookkeeping-Info)             │ CF
//═════════╧══════╧════════════════════════════════════════════════════════════════════════╧══════

#include "device.h"
#include "module.h"
#include "proc.h"
#include "syscall.h"

static q9_pd_t  proctab[Q9_NPROCS];
static uint32_t next_pid = 1;                          /* PIDs werden nie recycled (OS-9-Vorbild) */
static int      scheduler_pos = -1;                     /* Round-Robin-Zeiger (Tabellenindex)      */
static q9_pd_t *current = 0;                            /* Prozess, der gerade gestept wird        */
static uint32_t q9_tick = 0;                            /* abgeschlossene Scheduler-Durchlaeufe    */
                                                        /*   (4.3, Basis fuer q9_proc_sleep)       */

//────────────────────────────────────────────────────────────────────────────────────────────────
// Function: dev_ready
// Desc.:    Q9_WAIT_DEVICE-Weckcheck: fragt SS.Ready ab. Kein Geraet/keine getstat-Op -> sofort
//           als bereit werten (sonst wuerde ein Prozess auf einem geraetelosen Weckgrund fuer
//           immer haengen bleiben).
//────────────────────────────────────────────────────────────────────────────────────────────────
static int dev_ready(q9_dev_t *dev)
{
    q9_regs_t r = {0};

    if (!dev || !dev->drv || !dev->drv->getstat) {
        return 1;
    }
    return dev->drv->getstat(dev, SS_READY, &r) == 0;
}

//────────────────────────────────────────────────────────────────────────────────────────────────
// Function: zombie_child_exists
// Desc.:    Q9_WAIT_CHILD-Weckcheck: liegt mindestens ein Zombie-Kind von pid vor?
//────────────────────────────────────────────────────────────────────────────────────────────────
static int zombie_child_exists(uint32_t pid)
{
    for (int i = 0; i < Q9_NPROCS; i++) {
        if (proctab[i].pid != 0 && proctab[i].parent == pid &&
            proctab[i].state == Q9_PS_ZOMBIE) {
            return 1;
        }
    }
    return 0;
}

//────────────────────────────────────────────────────────────────────────────────────────────────
// Function: wait_condition_met
// Desc.:    Prueft den Weckgrund eines WAITING-Prozesses (SLEEPING laeuft direkt ueber q9_tick in
//           q9_proc_schedule, s.d.).
//────────────────────────────────────────────────────────────────────────────────────────────────
static int wait_condition_met(const q9_pd_t *pd)
{
    switch (pd->wait_reason) {
    case Q9_WAIT_DEVICE: return dev_ready(pd->wait_dev);
    case Q9_WAIT_CHILD:  return zombie_child_exists(pd->pid);
    default:              return 0;
    }
}

//────────────────────────────────────────────────────────────────────────────────────────────────
// Function: alloc_slot
// Desc.:    Sucht einen freien Tabellenslot und reserviert ihn (pid vergeben, restliche Felder auf
//           Null) — der Aufrufer traegt danach parent/module/step/state selbst ein. NULL = Tabelle
//           voll.
//────────────────────────────────────────────────────────────────────────────────────────────────
static q9_pd_t *alloc_slot(void)
{
    for (int i = 0; i < Q9_NPROCS; i++) {
        if (proctab[i].state == Q9_PS_FREE) {
            q9_pd_t *pd = &proctab[i];
            pd->pid      = next_pid++;
            pd->parent   = 0;
            pd->module   = 0;
            pd->exitcode = 0;
            pd->stdpath[0] = pd->stdpath[1] = pd->stdpath[2] = -1;
            pd->step     = 0;
            pd->wait_reason = Q9_WAIT_NONE;
            pd->wait_dev    = 0;
            pd->wake_tick   = 0;
            pd->priority    = 0;
            return pd;
        }
    }
    return 0;
}

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_proc_init
//════════════════════════════════════════════════════════════════════════════════════════════════
void q9_proc_init(q9_proc_step_fn step)
{
    for (int i = 0; i < Q9_NPROCS; i++) {
        proctab[i].pid         = 0;
        proctab[i].state       = Q9_PS_FREE;
        proctab[i].wait_reason = Q9_WAIT_NONE;
        proctab[i].wait_dev    = 0;
        proctab[i].wake_tick   = 0;
        proctab[i].priority    = 0;
    }
    scheduler_pos = -1;
    current        = 0;
    q9_tick        = 0;

    proctab[0].pid      = next_pid++;                  /* PID 1                                   */
    proctab[0].parent   = 0;                            /* Parent 0 = Kernel selbst                */
    proctab[0].module   = 0;                            /* interner/nativer Prozess, kein Modul    */
    proctab[0].state    = Q9_PS_ACTIVE;
    proctab[0].exitcode = 0;
    proctab[0].stdpath[0] = 0;                          /* Std-Pfade 0/1/2, von q9_dev_init schon  */
    proctab[0].stdpath[1] = 1;                          /*   auf /term geoeffnet                    */
    proctab[0].stdpath[2] = 2;
    proctab[0].step      = step;
}

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_proc_schedule
//════════════════════════════════════════════════════════════════════════════════════════════════
void q9_proc_schedule(void)
{
    for (int n = 0; n < Q9_NPROCS; n++) {
        scheduler_pos = (scheduler_pos + 1) % Q9_NPROCS;
        q9_pd_t *pd = &proctab[scheduler_pos];

        if (pd->state == Q9_PS_SLEEPING && q9_tick >= pd->wake_tick) {
            pd->state       = Q9_PS_ACTIVE;             /* Weckgrund erfuellt (4.3)                */
            pd->wait_reason = Q9_WAIT_NONE;
        } else if (pd->state == Q9_PS_WAITING && wait_condition_met(pd)) {
            pd->state       = Q9_PS_ACTIVE;
            pd->wait_reason = Q9_WAIT_NONE;
        }

        if (pd->state == Q9_PS_ACTIVE && pd->step) {
            current = pd;
            pd->step();
            current = 0;
        }
    }
    q9_tick++;
}

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_proc_current
//════════════════════════════════════════════════════════════════════════════════════════════════
q9_pd_t *q9_proc_current(void)
{
    return current;
}

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_proc_find
//════════════════════════════════════════════════════════════════════════════════════════════════
q9_pd_t *q9_proc_find(uint32_t pid)
{
    if (pid == 0) {
        return 0;
    }
    for (int i = 0; i < Q9_NPROCS; i++) {
        if (proctab[i].pid == pid) {
            return &proctab[i];
        }
    }
    return 0;
}

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_proc_native_entry
//════════════════════════════════════════════════════════════════════════════════════════════════
int q9_proc_native_entry(const q9_modhdr_t *hdr, q9_proc_step_fn *out)
{
    if (!hdr || hdr->lang != Q9_MOD_NATIVE || hdr->datasize < sizeof(q9_proc_step_fn)) {
        return E_NEMOD;
    }
    for (uint32_t i = 0; i < sizeof(q9_proc_step_fn); i++) {
        ((uint8_t *)out)[i] = ((const uint8_t *)hdr)[hdr->execoff + i];
    }
    return 0;
}

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_proc_fork
//════════════════════════════════════════════════════════════════════════════════════════════════
int q9_proc_fork(uint32_t parent_pid, const q9_modhdr_t *module, q9_proc_step_fn step,
                  uint32_t *out_pid)
{
    q9_pd_t *parent = q9_proc_find(parent_pid);
    q9_pd_t *pd     = alloc_slot();

    if (!pd) {
        return E_PRCFUL;
    }
    pd->parent = parent_pid;
    pd->module = module;
    pd->step   = step;
    if (parent) {
        pd->stdpath[0] = parent->stdpath[0];
        pd->stdpath[1] = parent->stdpath[1];
        pd->stdpath[2] = parent->stdpath[2];
    } else {
        pd->stdpath[0] = 0;
        pd->stdpath[1] = 1;
        pd->stdpath[2] = 2;
    }
    pd->state  = Q9_PS_ACTIVE;
    *out_pid   = pd->pid;
    return 0;
}

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_proc_exit
//════════════════════════════════════════════════════════════════════════════════════════════════
void q9_proc_exit(uint32_t pid, int32_t exitcode)
{
    q9_pd_t *pd = q9_proc_find(pid);

    if (!pd) {
        return;
    }
    pd->exitcode = exitcode;
    if (pd->module) {
        q9_mod_unlink(pd->module);
        pd->module = 0;
    }
    if (pd->parent == 0) {
        pd->pid   = 0;                                  /* niemand reapt -> Slot sofort frei       */
        pd->state = Q9_PS_FREE;
    } else {
        pd->state = Q9_PS_ZOMBIE;
    }
}

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_proc_wait
//════════════════════════════════════════════════════════════════════════════════════════════════
int q9_proc_wait(uint32_t waiter_pid, uint32_t *out_pid, int32_t *out_exitcode)
{
    int have_child = 0;

    for (int i = 0; i < Q9_NPROCS; i++) {
        q9_pd_t *pd = &proctab[i];
        if (pd->pid == 0 || pd->parent != waiter_pid) {
            continue;
        }
        have_child = 1;
        if (pd->state == Q9_PS_ZOMBIE) {
            *out_pid       = pd->pid;
            *out_exitcode  = pd->exitcode;
            pd->pid   = 0;
            pd->state = Q9_PS_FREE;
            return 0;
        }
    }
    return have_child ? E_NOTRDY : E_NOCHLD;
}

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_proc_wait_device
//════════════════════════════════════════════════════════════════════════════════════════════════
void q9_proc_wait_device(uint32_t pid, q9_dev_t *dev)
{
    q9_pd_t *pd = q9_proc_find(pid);

    if (!pd) {
        return;
    }
    pd->state       = Q9_PS_WAITING;
    pd->wait_reason = Q9_WAIT_DEVICE;
    pd->wait_dev    = dev;
}

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_proc_wait_child
//════════════════════════════════════════════════════════════════════════════════════════════════
void q9_proc_wait_child(uint32_t pid)
{
    q9_pd_t *pd = q9_proc_find(pid);

    if (!pd) {
        return;
    }
    pd->state       = Q9_PS_WAITING;
    pd->wait_reason = Q9_WAIT_CHILD;
}

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_proc_sleep
//════════════════════════════════════════════════════════════════════════════════════════════════
void q9_proc_sleep(uint32_t pid, uint32_t ticks)
{
    q9_pd_t *pd = q9_proc_find(pid);

    if (!pd) {
        return;
    }
    pd->state       = Q9_PS_SLEEPING;
    pd->wait_reason = Q9_WAIT_TIMER;
    pd->wake_tick   = q9_tick + (ticks == 0 ? 1u : ticks);
}

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_proc_suspend
//════════════════════════════════════════════════════════════════════════════════════════════════
int q9_proc_suspend(uint32_t pid)
{
    q9_pd_t *pd = q9_proc_find(pid);

    if (!pd) {
        return E_IPRCID;
    }
    pd->state       = Q9_PS_WAITING;
    pd->wait_reason = Q9_WAIT_SIGNAL;
    return 0;
}

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_proc_set_priority
//════════════════════════════════════════════════════════════════════════════════════════════════
int q9_proc_set_priority(uint32_t pid, uint8_t new_prio, uint8_t *out_old)
{
    q9_pd_t *pd = q9_proc_find(pid);

    if (!pd) {
        return E_IPRCID;
    }
    *out_old     = pd->priority;
    pd->priority = new_prio;
    return 0;
}

//────────────────────────────────────────────────────────────────────────────────────────────────
// EOF proc.c                                                                              Ver. 1.30
//────────────────────────────────────────────────────────────────────────────────────────────────
