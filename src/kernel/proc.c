//════════════════════════════════════════════════════════════════════════════════════════════════
// File:   proc.c                                                                          Ver. 1.00
// Owner:  AF
// Desc.:  Q9 Prozess-Descriptor-Tabelle + Round-Robin-Scheduler (Phase 4.1). Siehe proc.h fuer die
//         Architekturentscheidung (E8, Step-Modell). Statische Tabelle (Q9_NPROCS, kein malloc,
//         analog zur Geraetetabelle in device.c).
//
// Call:   siehe proc.h
//
// Edition History
//─────────┬──────┬────────────────────────────────────────────────────────────────────────┬──────
// Date    │ Ver. │ Description                                                            │ By
//─────────┼──────┼────────────────────────────────────────────────────────────────────────┼──────
// 26-07-04│ 1.00 │ 4.1: Initiale Version (Tabelle + Round-Robin-Scheduler, PID 1 = REPL)   │ CF
//═════════╧══════╧════════════════════════════════════════════════════════════════════════╧══════

#include "proc.h"

static q9_pd_t  proctab[Q9_NPROCS];
static uint32_t next_pid = 1;                          /* PIDs werden nie recycled (OS-9-Vorbild) */
static int      scheduler_pos = -1;                     /* Round-Robin-Zeiger (Tabellenindex)      */
static q9_pd_t *current = 0;                            /* Prozess, der gerade gestept wird        */

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_proc_init
//════════════════════════════════════════════════════════════════════════════════════════════════
void q9_proc_init(q9_proc_step_fn step)
{
    for (int i = 0; i < Q9_NPROCS; i++) {
        proctab[i].pid   = 0;
        proctab[i].state = Q9_PS_FREE;
    }
    scheduler_pos = -1;
    current        = 0;

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
        if (pd->state == Q9_PS_ACTIVE && pd->step) {
            current = pd;
            pd->step();
            current = 0;
        }
    }
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

//────────────────────────────────────────────────────────────────────────────────────────────────
// EOF proc.c                                                                              Ver. 1.00
//────────────────────────────────────────────────────────────────────────────────────────────────
