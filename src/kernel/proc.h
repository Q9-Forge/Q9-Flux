//════════════════════════════════════════════════════════════════════════════════════════════════
// File:   proc.h                                                                          Ver. 1.10
// Owner:  AF
// Desc.:  Q9 Prozess-Descriptor-Tabelle + Round-Robin-Scheduler (Phase 4). Grundsatzentscheidung
//         E8 (PROJECT.md): Step-Modell statt Stack-Umschaltung — WASM kennt keinen Stack-Wechsel,
//         also verwaltet der Scheduler Prozess-ZUSTAENDE (Active/Waiting/Sleeping) und ruft pro
//         q9_kernel_step()-Tick den naechsten aktiven Prozess als Step-Funktion auf. Blockieren
//         (4.3) wird ein Zustandswechsel + Weckgrund, nie ein eingefrorener Stack.
//
//         4.1 legte das Fundament: die statische Tabelle (wie devtab, kein malloc), den
//         Round-Robin-Scheduler und EINEN Prozess (PID 1 = die bisherige REPL). 4.2 bringt echte
//         Mehrprozess-Semantik: F$Fork (neuer Prozess aus einem Modul-Directory-Eintrag, siehe
//         Entscheidung E9 zu Q9_MOD_NATIVE), F$Exit (echtes Beenden — Zombie, falls ein Parent
//         existiert, der reapen kann; sofortiges Freigeben bei Parent 0 = niemand reapt), F$Wait
//         (Parent sammelt einen beendeten Kind-Prozess ein) und F$Chain (Prozess ersetzt sein
//         eigenes Modul, PID/Parent/Std-Pfade bleiben). Q9_PS_ZOMBIE ist bewusst KEIN Zustand aus
//         Entscheidung E8 (die beschreibt nur Scheduler-Zustaende) — er haelt lediglich Exit-Code +
//         PID bis zum F$Wait vor, der Scheduler ignoriert ihn wie FREE.
//
//         Bewusst NICHT Teil von 4.2 (Ideenspeicher): Reparenting verwaister Kind-Prozesse auf
//         PID 1, falls deren Parent selbst beendet wird, bevor er sie reapen konnte — Q9 hat noch
//         keine tiefen Prozessbaeume, das waere vorgezogene Komplexitaet ohne aktuellen Bedarf.
//
// Call:   q9_proc_init(); danach q9_proc_schedule() einmal pro q9_kernel_step()-Tick.
//
// Edition History
//─────────┬──────┬────────────────────────────────────────────────────────────────────────┬──────
// Date    │ Ver. │ Description                                                            │ By
//─────────┼──────┼────────────────────────────────────────────────────────────────────────┼──────
// 26-07-04│ 1.00 │ 4.1: Initiale Version (Tabelle + Round-Robin-Scheduler, PID 1 = REPL)   │ CF
// 26-07-04│ 1.10 │ 4.2: Q9_PS_ZOMBIE + q9_proc_fork/exit/wait, q9_proc_native_entry        │ CF
//         │      │ (Q9_MOD_NATIVE-Funktionszeiger aus einem Modul lesen, Entscheidung E9)  │ CF
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
    Q9_PS_SLEEPING,                                     /* schlaeft bis Tick-Zaehler ablaeuft (4.3) */
    Q9_PS_ZOMBIE                                        /* beendet (F$Exit), wartet auf F$Wait     */
                                                        /*   seines Parents (4.2, s. Funktionskopf) */
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

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_proc_native_entry
// Desc.:    F$Fork/F$Chain-Unterbau (4.2, Entscheidung E9): liest aus einem Q9_MOD_NATIVE-Modul
//           den rohen Funktionszeiger direkt hinter dem Header (Offset hdr->execoff) aus. 0 = ok
//           (*out gesetzt), E$NEMod wenn hdr NULL, die Sprache nicht Q9_MOD_NATIVE ist, oder das
//           Modul zu klein fuer einen Funktionszeiger ist.
// Call:     err = q9_proc_native_entry(hdr, &step)
//════════════════════════════════════════════════════════════════════════════════════════════════
int q9_proc_native_entry(const struct q9_modhdr *hdr, q9_proc_step_fn *out);

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_proc_fork
// Desc.:    F$Fork-Unterbau (4.2): alloziert einen neuen Prozess (Parent = parent_pid, Std-Pfade
//           vom Parent geerbt — bzw. 0/1/2, falls parent_pid keinen Eintrag hat), Zustand ACTIVE.
//           0 = ok (*out_pid gesetzt), E$PrcFul wenn die Prozesstabelle voll ist.
// Call:     err = q9_proc_fork(parent_pid, module, step, &newpid)
//════════════════════════════════════════════════════════════════════════════════════════════════
int q9_proc_fork(uint32_t parent_pid, const struct q9_modhdr *module, q9_proc_step_fn step,
                  uint32_t *out_pid);

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_proc_exit
// Desc.:    F$Exit-Unterbau (4.2): setzt den Exit-Code, gibt ein evtl. verlinktes Modul frei
//           (q9_mod_unlink). Hat der Prozess einen Parent (parent != 0), wird er Zombie (wartet
//           auf F$Wait); sonst wird der Tabellenslot sofort freigegeben (niemand kann reapen).
//           Unbekannte PID: no-op.
// Call:     q9_proc_exit(pid, exitcode)
//════════════════════════════════════════════════════════════════════════════════════════════════
void q9_proc_exit(uint32_t pid, int32_t exitcode);

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_proc_wait
// Desc.:    F$Wait-Unterbau (4.2): sucht ein Zombie-Kind von waiter_pid, reapt es (*out_pid/
//           *out_exitcode gesetzt, Tabellenslot frei, Modul war schon in q9_proc_exit entlinkt).
//           0 = ok. Kein Zombie, aber mindestens ein noch laufendes Kind -> E$NotRdy (Provisorium,
//           wie E$NotRdy bei I$Read vor 4.3 — echtes Blockieren kommt mit 4.3). Gar kein Kind
//           (auch nie eins gehabt) -> E$NoChld.
// Call:     err = q9_proc_wait(waiter_pid, &pid, &exitcode)
//════════════════════════════════════════════════════════════════════════════════════════════════
int q9_proc_wait(uint32_t waiter_pid, uint32_t *out_pid, int32_t *out_exitcode);

#endif // Q9_PROC_H

//────────────────────────────────────────────────────────────────────────────────────────────────
// EOF proc.h                                                                              Ver. 1.10
//────────────────────────────────────────────────────────────────────────────────────────────────
