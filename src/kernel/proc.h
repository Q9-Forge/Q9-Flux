//════════════════════════════════════════════════════════════════════════════════════════════════
// File:   proc.h                                                                          Ver. 1.20
// Owner:  AF
// Desc.:  Q9 Prozess-Descriptor-Tabelle + Round-Robin-Scheduler (Phase 4). Grundsatzentscheidung
//         E8 (PROJECT.md): Step-Modell statt Stack-Umschaltung — WASM kennt keinen Stack-Wechsel,
//         also verwaltet der Scheduler Prozess-ZUSTAENDE (Active/Waiting/Sleeping) und ruft pro
//         q9_kernel_step()-Tick den naechsten aktiven Prozess als Step-Funktion auf. Blockieren
//         wird ein Zustandswechsel + Weckgrund (q9_wait_reason_t), nie ein eingefrorener Stack.
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
//         4.3 ersetzt das E$NotRdy-Provisorium (I$Read/I$ReadLn seit 1.2, F$Wait seit 4.2) durch
//         echtes Blockieren: der Scheduler steppt WAITING/SLEEPING-Prozesse gar nicht erst, bis
//         ihr Weckgrund (q9_wait_reason_t) erfuellt ist — q9_proc_wait_device (SS.Ready des
//         Geraets, z.B. /term-Eingabe), q9_proc_wait_child (ein Zombie-Kind liegt vor) oder
//         q9_proc_sleep (F$Sleep, Tick-Zaehler-Zielwert; 0 Ticks = einmal yielden). Der Syscall
//         selbst (syscall.c) liefert weiterhin sofort `E$NotRdy` an den Aufrufer zurueck (kein
//         eingefrorener Stack moeglich) — geblockt wird NUR die Scheduler-Sicht: erst wenn der
//         Weckgrund eintritt, steppt der Scheduler den Prozess ueberhaupt wieder, statt ihn wie
//         vor 4.3 bei jedem Tick sinnlos erneut aufzurufen.
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
// 26-07-04│ 1.20 │ 4.3: q9_wait_reason_t + Weckgrund-Felder, q9_proc_wait_device/          │ CF
//         │      │ q9_proc_wait_child/q9_proc_sleep — echtes Blockieren statt E$NotRdy-    │ CF
//         │      │ Poll-Provisorium (I$Read/I$ReadLn, F$Wait); F$Sleep neu ueber die       │ CF
//         │      │ Scheduler-Zustaende WAITING/SLEEPING                                    │ CF
//═════════╧══════╧════════════════════════════════════════════════════════════════════════╧══════
#ifndef Q9_PROC_H
#define Q9_PROC_H

#include <stdint.h>

#define Q9_NPROCS 8                                     /* Prozesstabellengroesse (statisch)      */

struct q9_modhdr;                                       /* fwd (module.h)                         */
struct q9_dev;                                          /* fwd (device.h)                         */

typedef enum q9_proc_state {
    Q9_PS_FREE = 0,                                     /* Tabellenslot unbenutzt                  */
    Q9_PS_ACTIVE,                                       /* laeuft, wird vom Scheduler gestept       */
    Q9_PS_WAITING,                                      /* blockiert auf ein Ereignis, s. wait_reason*/
    Q9_PS_SLEEPING,                                     /* schlaeft bis wake_tick erreicht ist      */
    Q9_PS_ZOMBIE                                        /* beendet (F$Exit), wartet auf F$Wait     */
                                                        /*   seines Parents (4.2, s. Funktionskopf) */
} q9_proc_state_t;

typedef enum q9_wait_reason {
    Q9_WAIT_NONE = 0,                                   /* nur ausserhalb WAITING/SLEEPING gueltig */
    Q9_WAIT_DEVICE,                                     /* WAITING: wait_dev muss SS.Ready melden  */
    Q9_WAIT_CHILD,                                      /* WAITING: ein Zombie-Kind muss vorliegen */
    Q9_WAIT_TIMER                                       /* SLEEPING: wake_tick muss erreicht sein  */
} q9_wait_reason_t;

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
    q9_wait_reason_t         wait_reason;                 /* Weckgrund, gueltig bei WAITING/SLEEPING */
    struct q9_dev           *wait_dev;                    /* Q9_WAIT_DEVICE: Geraet, dessen SS.Ready */
                                                        /*   den Prozess weckt                     */
    uint32_t                 wake_tick;                   /* Q9_WAIT_TIMER: Ziel-Tickwert (4.3)      */
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
// Desc.:    Ein Scheduler-Tick: prueft zuerst bei jedem WAITING/SLEEPING-Prozess, ob sein
//           Weckgrund erfuellt ist (4.3 — Q9_WAIT_DEVICE: SS.Ready; Q9_WAIT_CHILD: Zombie-Kind;
//           Q9_WAIT_TIMER: wake_tick erreicht) und weckt ihn dann nach ACTIVE. Ruft danach reihum
//           (Round-Robin, beginnend hinter dem zuletzt gestepten Prozess) die Step-Funktion JEDES
//           Prozesses im Zustand ACTIVE genau einmal auf. Waehrend des Aufrufs liefert
//           q9_proc_current() diesen Prozess. Keine Prozesse aktiv -> no-op (z.B. bevor
//           q9_proc_init lief, oder wenn alle beendet/blockiert sind).
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
//           0 = ok. Kein Zombie, aber mindestens ein noch laufendes Kind -> E$NotRdy (der Aufrufer
//           blockiert danach echt — 4.3, syscall.c ruft dafuer q9_proc_wait_child auf). Gar kein
//           Kind (auch nie eins gehabt) -> E$NoChld.
// Call:     err = q9_proc_wait(waiter_pid, &pid, &exitcode)
//════════════════════════════════════════════════════════════════════════════════════════════════
int q9_proc_wait(uint32_t waiter_pid, uint32_t *out_pid, int32_t *out_exitcode);

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_proc_wait_device
// Desc.:    4.3: versetzt pid in den Zustand WAITING mit Weckgrund Q9_WAIT_DEVICE — der Scheduler
//           steppt den Prozess erst wieder, wenn dev->drv->getstat(dev, SS_READY, ...) == 0 meldet
//           (kein getstat/keine getstat-Op -> gilt sofort als bereit, um nicht ewig zu blockieren).
//           Unbekannte PID: no-op.
// Call:     q9_proc_wait_device(pid, dev)
//════════════════════════════════════════════════════════════════════════════════════════════════
void q9_proc_wait_device(uint32_t pid, struct q9_dev *dev);

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_proc_wait_child
// Desc.:    4.3: versetzt pid in den Zustand WAITING mit Weckgrund Q9_WAIT_CHILD — der Scheduler
//           steppt den Prozess erst wieder, wenn mindestens ein Kind von pid im Zustand ZOMBIE ist
//           (F$Wait kann es dann reapen). Unbekannte PID: no-op.
// Call:     q9_proc_wait_child(pid)
//════════════════════════════════════════════════════════════════════════════════════════════════
void q9_proc_wait_child(uint32_t pid);

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_proc_sleep
// Desc.:    F$Sleep-Unterbau (4.3): versetzt pid in den Zustand SLEEPING, wake_tick = aktueller
//           Scheduler-Tick + ticks (ticks == 0 -> +1, "einmal yielden" — der Prozess pausiert fuer
//           genau einen Scheduler-Durchlauf). Unbekannte PID: no-op.
// Call:     q9_proc_sleep(pid, ticks)
//════════════════════════════════════════════════════════════════════════════════════════════════
void q9_proc_sleep(uint32_t pid, uint32_t ticks);

#endif // Q9_PROC_H

//────────────────────────────────────────────────────────────────────────────────────────────────
// EOF proc.h                                                                              Ver. 1.20
//────────────────────────────────────────────────────────────────────────────────────────────────
