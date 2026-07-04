//════════════════════════════════════════════════════════════════════════════════════════════════
// File:   proc.h                                                                          Ver. 1.00
// Owner:  AF
// Desc.:  Q9 Prozess-Descriptor-Tabelle + Round-Robin-Scheduler (Phase 4.1). Grundsatzentscheidung
//         E8 (PROJECT.md): Step-Modell statt Stack-Umschaltung — WASM kennt keinen Stack-Wechsel,
//         also verwaltet der Scheduler Prozess-ZUSTAENDE (Active/Waiting/Sleeping) und ruft pro
//         q9_kernel_step()-Tick den naechsten aktiven Prozess als Step-Funktion auf. Blockieren
//         (4.3) wird ein Zustandswechsel + Weckgrund, nie ein eingefrorener Stack.
//
//         4.1 legt nur das Fundament: die statische Tabelle (wie devtab, kein malloc), den
//         Round-Robin-Scheduler und EINEN Prozess (PID 1 = die bisherige REPL, jetzt als "erster
//         echter Prozess" registriert). F$Fork/F$Exit/F$Wait/F$Chain (echte Mehrprozess-Semantik)
//         kommen erst mit 4.2 — Waiting/Sleeping-Zustaende werden hier schon als Enum-Werte
//         vorgesehen, aber von 4.1 noch nicht erzeugt (kein Blockieren ohne 4.3).
//
// Call:   q9_proc_init(); danach q9_proc_schedule() einmal pro q9_kernel_step()-Tick.
//
// Edition History
//─────────┬──────┬────────────────────────────────────────────────────────────────────────┬──────
// Date    │ Ver. │ Description                                                            │ By
//─────────┼──────┼────────────────────────────────────────────────────────────────────────┼──────
// 26-07-04│ 1.00 │ 4.1: Initiale Version (Tabelle + Round-Robin-Scheduler, PID 1 = REPL)   │ CF
//═════════╧══════╧════════════════════════════════════════════════════════════════════════╧══════
#ifndef Q9_PROC_H
#define Q9_PROC_H

#include <stdint.h>

#define Q9_NPROCS 8                                     /* Prozesstabellengroesse (statisch)      */

struct q9_modhdr;                                       /* fwd (module.h)                         */

typedef enum q9_proc_state {
    Q9_PS_FREE = 0,                                     /* Tabellenslot unbenutzt                  */
    Q9_PS_ACTIVE,                                       /* laeuft, wird vom Scheduler gestept       */
    Q9_PS_WAITING,                                      /* blockiert auf ein Ereignis (ab 4.3)     */
    Q9_PS_SLEEPING                                      /* schlaeft bis Tick-Zaehler ablaeuft (4.3) */
} q9_proc_state_t;

typedef void (*q9_proc_step_fn)(void);

typedef struct q9_pd {
    uint32_t                pid;                        /* 0 = frei, sonst 1..N (nie recycled     */
                                                        /*   innerhalb eines Laufs wie OS-9)       */
    uint32_t                parent;                      /* Parent-PID, 0 = Kernel selbst           */
    const struct q9_modhdr *module;                     /* Modul, aus dem der Prozess kam;         */
                                                        /*   NULL = interner/nativer Prozess (REPL) */
    q9_proc_state_t         state;
    int32_t                 exitcode;
    int32_t                 stdpath[3];                 /* eigene Std-Pfade 0/1/2 (Indizes in die  */
                                                        /*   globale Pfadtabelle, device.h)        */
    q9_proc_step_fn         step;                        /* Step-Funktion (intern/C); 68k/WASM      */
                                                        /*   bekommen spaeter eigene step-Varianten */
} q9_pd_t;

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_proc_init
// Desc.:    Leert die Prozesstabelle und legt PID 1 an (Parent 0, kein Modul, Std-Pfade 0/1/2,
//           Zustand ACTIVE, Step-Funktion "step"). Einmalig beim Boot, nach q9_dev_init().
// Call:     q9_proc_init(repl_step)
//════════════════════════════════════════════════════════════════════════════════════════════════
void q9_proc_init(q9_proc_step_fn step);

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_proc_schedule
// Desc.:    Ein Scheduler-Tick: ruft reihum (Round-Robin, beginnend hinter dem zuletzt gestepten
//           Prozess) die Step-Funktion JEDES Prozesses im Zustand ACTIVE genau einmal auf.
//           Waehrend des Aufrufs liefert q9_proc_current() diesen Prozess. Keine Prozesse aktiv
//           -> no-op (z.B. bevor q9_proc_init lief, oder wenn alle beendet sind).
// Call:     q9_proc_schedule()
//════════════════════════════════════════════════════════════════════════════════════════════════
void q9_proc_schedule(void);

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_proc_current
// Desc.:    Liefert den Prozess-Deskriptor, dessen Step-Funktion der Scheduler gerade ausfuehrt
//           (fuer Syscalls wie F$ID, die "den aufrufenden Prozess" kennen muessen). NULL nur
//           ausserhalb eines Scheduler-Aufrufs (z.B. vor q9_proc_init).
// Call:     pd = q9_proc_current()
//════════════════════════════════════════════════════════════════════════════════════════════════
q9_pd_t *q9_proc_current(void);

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_proc_find
// Desc.:    Sucht einen Prozess per PID. NULL = keine solche PID (auch nicht mehr belegt).
// Call:     pd = q9_proc_find(pid)
//════════════════════════════════════════════════════════════════════════════════════════════════
q9_pd_t *q9_proc_find(uint32_t pid);

#endif // Q9_PROC_H

//────────────────────────────────────────────────────────────────────────────────────────────────
// EOF proc.h                                                                              Ver. 1.00
//────────────────────────────────────────────────────────────────────────────────────────────────
