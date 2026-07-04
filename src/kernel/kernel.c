//════════════════════════════════════════════════════════════════════════════════════════════════
// File:   kernel.c                                                                        Ver. 3.60
// Owner:  AF
// Desc.:  Q9-Kernel, Phase 1: Boot + Zeilen-REPL, komplett über die eigene Syscall-Schicht
//         (I$ReadLn/I$WritLn — Dogfooding der OS-9-kompatiblen ABI, siehe docs/SYSCALLS.md).
//
// Call:   q9_kernel_init(); danach zyklisch q9_kernel_step();
//
// Edition History
//─────────┬──────┬────────────────────────────────────────────────────────────────────────┬──────
// Date    │ Ver. │ Description                                                            │ By
//─────────┼──────┼────────────────────────────────────────────────────────────────────────┼──────
// 26-07-02│ 1.00 │ Initiale Version: Banner, Echo-Loop                                    │ CF
// 26-07-03│ 1.10 │ Syscall-Schicht: REPL über I$ReadLn/I$WritLn, F$Exit, Selbsttest       │ CF
// 26-07-03│ 1.20 │ 1.3: q9_dev_init() beim Boot, Selbsttests für Device-Modell            │ CF
// 26-07-03│ 1.30 │ 1.4: Selbsttests I$Dup/I$Close                                         │ CF
// 26-07-03│ 1.40 │ 1.5: Selbsttests F$PrsNam/F$CmpNam                                     │ CF
// 26-07-03│ 1.50 │ 1.6: Selbsttests I$Attach/I$Detach                                     │ CF
// 26-07-03│ 1.60 │ 1.7: /nil im Banner + Selbsttest                                       │ CF
// 26-07-03│ 1.70 │ 1.8: Selbsttests I$GetStt/I$SetStt                                     │ CF
// 26-07-03│ 1.80 │ 1.9: Selbsttests F$STime/F$Time                                        │ CF
// 26-07-03│ 1.90 │ Bugfix: F$CmpNam-Selbsttest nutzt jetzt E_DIFFER($A5)                  │ CF
// 26-07-03│ 2.00 │ Bugfix: F$STime/F$Time-Selbsttest an d0=Zeit/d1=Datum angepasst        │ CF
// 26-07-03│ 2.10 │ 2.1: Selbsttest q9_crc32 (Referenzwert "123456789")                    │ CF
// 26-07-03│ 2.20 │ 2.3a: Selbsttests q9_mod_scan_first/next                              │ CF
// 26-07-03│ 2.30 │ 2.3b-d: Selbsttests Validierung/Directory/F$Link/F$UnLink             │ CF
// 26-07-03│ 2.40 │ 3.1: /d0 im Banner + Selbsttests SS.BlkRd/SS.BlkWr                    │ CF
// 26-07-04│ 2.50 │ 3.2: Selbsttests I$Open/I$ChgDir (VFS-Pfad-Routing, Test-File-Manager)│ CF
// 26-07-04│ 2.60 │ 3.3: Selbsttests FAT16 (8.3/LFN-Datei, Unterverzeichnis, I$Seek);     │ CF
//         │      │ Checks nur aktiv, wenn q9disk.img beim Boot als FAT16 erkannt wurde   │
// 26-07-04│ 2.70 │ 3.4: Selbsttests FAT16 schreibend (I$Create/I$Write/I$MakDir/         │ CF
//         │      │ I$Delete); I$Create/MakDir/Delete-Test ohne fm jetzt gegen /term      │
//         │      │ statt Geruest-Erwartung (echte Semantik ab 3.4)                       │
// 26-07-04│ 2.80 │ 3.5: Selbsttests F$Load (Modul per I$Create/I$Write nach /d0          │ CF
//         │      │ geschrieben, F$Load laedt+validiert+registriert, F$Link findet es     │
//         │      │ danach ueber den Namen, F$UnLink baut beide Referenzen sauber ab,      │
//         │      │ fehlende Datei -> E$PNNF, kaputter Sync -> E$BMHP); Testdateien werden  │
//         │      │ danach per I$Delete wieder entfernt (haelt den FAT16-Cluster-Zustand    │
//         │      │ fuer die 3.4-Nachvalidierung in test/06 unveraendert)                  │
// 26-07-04│ 2.90 │ 4.1: q9_kernel_step() reicht an q9_proc_schedule() weiter (proc.c);     │ CF
//         │      │ bisheriger REPL-Koerper wandert in repl_step() (PID 1, ueber            │
//         │      │ q9_proc_init registriert); Selbsttests fuer die Prozesstabelle          │
// 26-07-04│ 3.00 │ 4.2: build_native_module (E9) + Selbsttests F$Fork/F$Exit/F$Wait/       │ CF
//         │      │ F$Chain (echter zweiter Prozess, Verkettung auf neues Modul); alter      │
//         │      │ q9_proc_halted()-Guard in repl_step entfernt (obsolet: Scheduler ruft    │
//         │      │ nicht-aktive Prozesse ohnehin nie)                                       │
// 26-07-04│ 3.10 │ 4.3: Selbsttests fuer echtes Blockieren — I$ReadLn ohne Eingabe versetzt │ CF
//         │      │ den Prozess in WAITING/Q9_WAIT_DEVICE (Scheduler skippt ihn), F$Wait      │
//         │      │ ohne Zombie ebenso in WAITING/Q9_WAIT_CHILD (weckt automatisch bei Exit   │
//         │      │ des Kindes), F$Sleep legt per Q9_WAIT_TIMER fuer N Ticks schlafen         │
// 26-07-04│ 3.20 │ 4.4: Selbsttests F$SSpd (WAITING/Q9_WAIT_SIGNAL, dauerhaft ohne           │ CF
//         │      │ Weckmechanismus vor 4.5) + F$SPrior (Prioritaetsfeld setzen, alten Wert   │
//         │      │ liefern, E$IPrcID bei unbekannter PID)                                    │
// 26-07-04│ 3.30 │ 4.5: Selbsttests F$Send/F$Icpt/F$RTE — Signal lenkt bei installiertem     │ CF
//         │      │ Handler den naechsten Scheduler-Aufruf auf ihn um (F$RTE schaltet zurueck),│
//         │      │ bricht ohne Handler nur WAITING ab; Fehlerpfade (unbekannte PID,           │
//         │      │ ausserhalb eines Prozesses) -> E$IPrcID                                    │
// 26-07-04│ 3.40 │ 4.6: Selbsttest wasm3-Grundbaustein (nur -DQ9_HAVE_WASM3, native-only) —    │ CF
//         │      │ laedt ein handgebautes add(a,b)-Modul, ruft es auf, prueft 2+3=5           │
// 26-07-04│ 3.50 │ 4.7: build_wasm_module + Selbsttest Syscall-Bridge — echtes F$Fork auf ein │ CF
//         │      │ Q9_MOD_WASM-Modul (importiert q9.f_id/f_time/f_exit), F$Wait sammelt es ein│
// 26-07-04│ 3.60 │ 4.8: Selbsttest Pointer-Marshaling — Gastprogramm mit eigenem linearem       │ CF
//         │      │ Speicher oeffnet/schreibt/liest/schliesst /nil ueber q9.i_open/i_write/       │
//         │      │ i_read/i_close (Bytecode per wat2wasm/wabt gebaut, s. ARBEITSPLAN.md 4.8)     │
//═════════╧══════╧════════════════════════════════════════════════════════════════════════╧══════

#include "../hal/q9_hal.h"
#include "device.h"
#include "module.h"
#include "proc.h"
#include "syscall.h"
#include "vfs.h"
#include "kernel.h"
#ifdef Q9_HAVE_WASM3
#include "wasmrt.h"
#endif

static void repl_step(void);                            /* 4.1: Step-Funktion von PID 1 (s.u.)     */

//╔══════════════════════════════════════════════════════════════════════════════════════════════╗
//║ INTERNAL HELPERS (thin wrappers over the syscall layer)                                      ║
//╚══════════════════════════════════════════════════════════════════════════════════════════════╝

static uint32_t str_len(const char *s)
{
    uint32_t n = 0;
    while (s[n]) {
        n++;
    }
    return n;
}

//────────────────────────────────────────────────────────────────────────────────────────────────
// Function: kputs
// Desc.:    Gibt einen String über I$Write auf Pfad 1 (stdout) aus.
// Call:     kputs("text\n")
//────────────────────────────────────────────────────────────────────────────────────────────────
static void kputs(const char *s)
{
    q9_regs_t r = {0};

    r.d[0] = 1;                                        /* path 1 = stdout                        */
    r.d[1] = str_len(s);
    r.a[0] = (void *)s;
    q9_syscall(I_WRITE, &r);
}

//╔══════════════════════════════════════════════════════════════════════════════════════════════╗
//║ KERNEL API                                                                                   ║
//╚══════════════════════════════════════════════════════════════════════════════════════════════╝

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_kernel_init
// Desc.:    Kernel-Initialisierung, gibt das Boot-Banner aus (über die Syscall-Schicht).
// Call:     q9_kernel_init()
//════════════════════════════════════════════════════════════════════════════════════════════════
void q9_kernel_init(void)
{
    q9_dev_init();                                     /* device model first — kputs needs it    */
    q9_proc_init(repl_step);                           /* 4.1: PID 1 = REPL, siehe proc.h/.c      */

    kputs("\n");
    kputs("  ═══════════════════════════════════════\n");
    kputs("   Q9 v" Q9_VERSION " alpha\n");
    kputs("   modulares Mini-OS in OS-9-Tradition\n");
    kputs("  ═══════════════════════════════════════\n");
    kputs("  target: ");
    kputs(q9_hal_target());
    kputs("\n  syscalls: OS-9-ABI aktiv (docs/SYSCALLS.md)\n");
    kputs("  devices:  /term (Pfade 0/1/2), /nil, /d0\n\n");
    kputs("  Phase 1: REPL. Eingabe wird zurückgegeben, 'exit' beendet.\n\n");
    kputs("Q9> ");
}

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_kernel_step
// Desc.:    Ein Kernel-Tick — reicht an den Scheduler weiter (4.1: q9_proc_schedule()), der
//           reihum jeden aktiven Prozess einmal steppt. Bis F$Fork (4.2) existiert, ist das
//           genau ein Prozess (PID 1 = repl_step).
// Call:     q9_kernel_step()
//════════════════════════════════════════════════════════════════════════════════════════════════
void q9_kernel_step(void)
{
    q9_proc_schedule();
}

//────────────────────────────────────────────────────────────────────────────────────────────────
// Function: repl_step
// Desc.:    Step-Funktion von PID 1 (4.1: vorher der Koerper von q9_kernel_step direkt): I$ReadLn
//           pollen; komplette Zeile -> Antwort + neuer Prompt. "exit" ruft F$Exit (Proto-Prozess
//           haelt an — PID 1 verlaesst danach den ACTIVE-Zustand, der Scheduler ruft repl_step()
//           dann nie wieder auf (4.2: q9_proc_exit()/q9_proc_schedule() reichen als Guard).
//────────────────────────────────────────────────────────────────────────────────────────────────
static void repl_step(void)
{
    q9_regs_t r = {0};
    uint8_t   line[80];
    uint32_t  n;

    r.d[0] = 0;                                        /* path 0 = stdin                         */
    r.d[1] = sizeof(line) - 1;
    r.a[0] = line;
    if (q9_syscall(I_READLN, &r) != 0) {
        return;                                        /* E$NotRdy: line not complete yet        */
    }

    n = r.d[1];
    if (n > 0 && line[n - 1] == '\r') {                /* strip CR for comparing                 */
        n--;
    }
    line[n] = 0;

    if (n == 4 && line[0] == 'e' && line[1] == 'x' && line[2] == 'i' && line[3] == 't') {
        q9_regs_t ex = {0};
        kputs("Prozess beendet (F$Exit). Bis bald!\n");
        q9_syscall(F_EXIT, &ex);
        return;
    }

    if (n > 0) {
        kputs("echo: ");
        kputs((const char *)line);
        kputs("\n");
    }
    kputs("Q9> ");
}

//────────────────────────────────────────────────────────────────────────────────────────────────
// Function: build_module
// Desc.:    Baut ein minimales, gueltiges Q9-Modul (Header + nullterminierter Name direkt danach)
//           in buf und rechnet die CRC32 passend (Feld erst 0, dann q9_crc32 ueber alles).
//           Nur fuer den Selbsttest — buf muss mindestens Q9_MOD_HDRSIZE + strlen(name) + 1 Bytes
//           gross sein.
//────────────────────────────────────────────────────────────────────────────────────────────────
static void build_module(uint8_t *buf, const char *name, uint8_t type, uint8_t lang, uint8_t rev)
{
    q9_modhdr_t *h       = (q9_modhdr_t *)buf;
    uint32_t     namelen = str_len(name);
    uint32_t     modsize = Q9_MOD_HDRSIZE + namelen + 1;

    h->sync[0]  = Q9_MOD_SYNC0;
    h->sync[1]  = Q9_MOD_SYNC1;
    h->hdrsize  = Q9_MOD_HDRSIZE;
    h->modsize  = modsize;
    h->nameoff  = Q9_MOD_HDRSIZE;
    h->type     = type;
    h->lang     = lang;
    h->attr     = 0;
    h->rev      = rev;
    h->execoff  = Q9_MOD_HDRSIZE;                   /* Dummy-Einsprung: zeigt auf den Namen    */
    h->datasize = 0;
    h->crc32    = 0;
    for (uint32_t i = 0; i <= namelen; i++) {
        buf[Q9_MOD_HDRSIZE + i] = (uint8_t)name[i];
    }
    h->crc32 = q9_crc32(buf, modsize);
}

//────────────────────────────────────────────────────────────────────────────────────────────────
// Function: build_native_module
// Desc.:    4.2-Selbsttest: baut ein echtes Q9_MOD_NATIVE-Modul (Entscheidung E9, PROJECT.md) —
//           anders als build_module() steht direkt hinter dem Header (execoff) ein roher
//           q9_proc_step_fn-Funktionszeiger (gueltig nur in diesem laufenden Host-Prozess), danach
//           der Name. Nur so kann F$Fork/F$Chain einen Prozess starten, der wirklich etwas tut —
//           Q9 hat vor Phase 6 keine 68k/WASM-Runtime, die echten Byte-Code ausfuehren koennte.
//           buf muss mindestens Q9_MOD_HDRSIZE + sizeof(step) + strlen(name) + 1 Bytes gross sein.
//────────────────────────────────────────────────────────────────────────────────────────────────
static void build_native_module(uint8_t *buf, const char *name, uint8_t rev, q9_proc_step_fn step)
{
    q9_modhdr_t *h       = (q9_modhdr_t *)buf;
    uint32_t     fnoff    = Q9_MOD_HDRSIZE;
    uint32_t     nameoff  = fnoff + (uint32_t)sizeof(step);
    uint32_t     namelen  = str_len(name);
    uint32_t     modsize  = nameoff + namelen + 1;

    h->sync[0]  = Q9_MOD_SYNC0;
    h->sync[1]  = Q9_MOD_SYNC1;
    h->hdrsize  = Q9_MOD_HDRSIZE;
    h->modsize  = modsize;
    h->nameoff  = nameoff;
    h->type     = Q9_MOD_PRGRM;
    h->lang     = Q9_MOD_NATIVE;
    h->attr     = 0;
    h->rev      = rev;
    h->execoff  = fnoff;
    h->datasize = (uint32_t)sizeof(step);
    h->crc32    = 0;
    for (uint32_t i = 0; i < sizeof(step); i++) {
        buf[fnoff + i] = ((const uint8_t *)&step)[i];
    }
    for (uint32_t i = 0; i <= namelen; i++) {
        buf[nameoff + i] = (uint8_t)name[i];
    }
    h->crc32 = q9_crc32(buf, modsize);
}

#ifdef Q9_HAVE_WASM3
//────────────────────────────────────────────────────────────────────────────────────────────────
// Function: build_wasm_module
// Desc.:    4.7-Selbsttest: baut ein echtes Q9_MOD_WASM-Modul — analog zu build_native_module(),
//           aber statt eines rohen Funktionszeigers steht direkt hinter dem Header (execoff) der
//           komplette WASM-Bytecode (Laenge = datasize), danach der Name. q9_wasm_proc_step()
//           (wasmproc.c) liest genau diese Konvention wieder aus. buf muss mindestens
//           Q9_MOD_HDRSIZE + codelen + strlen(name) + 1 Bytes gross sein.
//────────────────────────────────────────────────────────────────────────────────────────────────
static void build_wasm_module(uint8_t *buf, const char *name, uint8_t rev,
                               const uint8_t *code, uint32_t codelen)
{
    q9_modhdr_t *h       = (q9_modhdr_t *)buf;
    uint32_t     codeoff = Q9_MOD_HDRSIZE;
    uint32_t     nameoff = codeoff + codelen;
    uint32_t     namelen = str_len(name);
    uint32_t     modsize = nameoff + namelen + 1;

    h->sync[0]  = Q9_MOD_SYNC0;
    h->sync[1]  = Q9_MOD_SYNC1;
    h->hdrsize  = Q9_MOD_HDRSIZE;
    h->modsize  = modsize;
    h->nameoff  = nameoff;
    h->type     = Q9_MOD_PRGRM;
    h->lang     = Q9_MOD_WASM;
    h->attr     = 0;
    h->rev      = rev;
    h->execoff  = codeoff;
    h->datasize = codelen;
    h->crc32    = 0;
    for (uint32_t i = 0; i < codelen; i++) {
        buf[codeoff + i] = code[i];
    }
    for (uint32_t i = 0; i <= namelen; i++) {
        buf[nameoff + i] = (uint8_t)name[i];
    }
    h->crc32 = q9_crc32(buf, modsize);
}
#endif

//────────────────────────────────────────────────────────────────────────────────────────────────
// Function: child_step
// Desc.:    4.2-Selbsttest: Step-Funktion eines per F$Fork gestarteten Kind-Prozesses. Beendet
//           sich beim ersten Aufruf sofort selbst mit Exit-Code 42 — beweist, dass der Scheduler
//           einen zweiten, echten Prozess parallel zur REPL steppt.
//────────────────────────────────────────────────────────────────────────────────────────────────
static void child_step(void)
{
    q9_regs_t ex = {0};
    ex.d[1] = 42;
    q9_syscall(F_EXIT, &ex);
}

//────────────────────────────────────────────────────────────────────────────────────────────────
// Function: child_after_chain_step / child_before_chain_step
// Desc.:    4.2-Selbsttest fuer F$Chain: "before" verkettet sich beim ersten Tick auf das Modul
//           "child_after" (eigene PID/Parent/Std-Pfade bleiben, nur Modul+Step wechseln) — der
//           naechste Scheduler-Tick ruft dann bereits die NEUE Step-Funktion auf, die sich mit
//           Exit-Code 77 beendet. Beweist, dass F$Chain den Prozess wirklich umbiegt statt nur
//           einen zweiten zu starten.
//────────────────────────────────────────────────────────────────────────────────────────────────
static void child_after_chain_step(void)
{
    q9_regs_t ex = {0};
    ex.d[1] = 77;
    q9_syscall(F_EXIT, &ex);
}

static void child_before_chain_step(void)
{
    q9_regs_t ch = {0};
    ch.a[0] = (void *)"child_after";
    ch.d[1] = Q9_MOD_PRGRM;
    ch.d[2] = Q9_MOD_NATIVE;
    q9_syscall(F_CHAIN, &ch);
}

//────────────────────────────────────────────────────────────────────────────────────────────────
// Function: readln_block_step
// Desc.:    4.3-Selbsttest: versucht bei jedem Aufruf I$ReadLn auf Pfad 0 (stdin, /term) —
//           zaehlt mit, wie oft der Scheduler die Step-Funktion tatsaechlich aufgerufen hat. Im
//           Testharness kommt nie Konsoleneingabe an (stdin nicht-blockierend, s. hal_posix.c),
//           also bleibt der Prozess nach dem ersten Aufruf WAITING/Q9_WAIT_DEVICE — der Zaehler
//           beweist, dass der Scheduler ihn danach NICHT mehr steppt.
//────────────────────────────────────────────────────────────────────────────────────────────────
static int readln_block_calls = 0;

static void readln_block_step(void)
{
    q9_regs_t r = {0};
    uint8_t   line[16];

    readln_block_calls++;
    r.d[0] = 0;                                        /* path 0 = stdin                          */
    r.d[1] = sizeof(line);
    r.a[0] = line;
    q9_syscall(I_READLN, &r);
}

//────────────────────────────────────────────────────────────────────────────────────────────────
// Function: wait_parent_step
// Desc.:    4.3-Selbsttest fuer echtes F$Wait-Blockieren: forkt beim ersten Aufruf einen Enkel
//           (Modul "wait_grandchild" -> child_step, beendet sich sofort mit Exit-Code 42) und
//           ruft direkt danach F$Wait auf — der noch nicht gestepte Enkel liefert garantiert
//           E$NotRdy (das passiert synchron VOR dem ersten Scheduler-Tick des Enkels). Bei jedem
//           weiteren Aufruf (nur nach dem Aufwachen aus WAITING/Q9_WAIT_CHILD moeglich) wird
//           erneut F$Wait gerufen, das den inzwischen zum Zombie gewordenen Enkel reapt.
//────────────────────────────────────────────────────────────────────────────────────────────────
static int      wait_parent_calls      = 0;
static int      wait_parent_result     = -1;
static uint32_t wait_parent_gcpid      = 0;
static uint32_t wait_parent_reaped_pid = 0;

static void wait_parent_step(void)
{
    q9_regs_t wt = {0};
    q9_pd_t  *me = q9_proc_current();

    wait_parent_calls++;
    if (wait_parent_calls == 1) {
        q9_proc_fork(me ? me->pid : 1, 0, child_step, &wait_parent_gcpid);
    }
    wait_parent_result = q9_syscall(F_WAIT, &wt);
    if (wait_parent_result == 0) {
        wait_parent_reaped_pid = wt.d[0];
    }
}

//────────────────────────────────────────────────────────────────────────────────────────────────
// Function: sspd_test_step
// Desc.:    4.4-Selbsttest fuer F$SSpd: zaehlt nur mit, wie oft der Scheduler ihn steppt — nach
//           F$SSpd (WAITING/Q9_WAIT_SIGNAL, bewusst ohne Weckcheck vor 4.5) darf der Zaehler nie
//           wieder steigen.
//────────────────────────────────────────────────────────────────────────────────────────────────
static int sspd_calls = 0;

static void sspd_test_step(void)
{
    sspd_calls++;
}

//────────────────────────────────────────────────────────────────────────────────────────────────
// Function: icpt_test_step / icpt_handler_step
// Desc.:    4.5-Selbsttest fuer F$Icpt/F$Send/F$RTE: icpt_test_step ist die normale Step-Funktion
//           (zaehlt jeden normalen Aufruf), installiert sich beim ersten Aufruf per F$Icpt selbst
//           als Intercept-Handler icpt_handler_step. Trifft danach per F$Send ein Signal ein,
//           ruft der Scheduler statt icpt_test_step den Handler — der liest das Signal ueber
//           q9_proc_current()->pending_signal, zaehlt mit und kehrt per F$RTE zur normalen
//           Step-Funktion zurueck.
//────────────────────────────────────────────────────────────────────────────────────────────────
static int      icpt_normal_calls    = 0;
static int      icpt_handler_calls   = 0;
static uint32_t icpt_received_signal = 0;

static void icpt_handler_step(void)
{
    q9_pd_t  *me = q9_proc_current();
    q9_regs_t rte = {0};

    icpt_handler_calls++;
    icpt_received_signal = me ? me->pending_signal : 0;
    q9_syscall(F_RTE, &rte);
}

static void icpt_test_step(void)
{
    icpt_normal_calls++;
    if (icpt_normal_calls == 1) {
        q9_regs_t ic = {0};
        ic.a[0] = (void *)icpt_handler_step;
        q9_syscall(F_ICPT, &ic);
    }
}

//────────────────────────────────────────────────────────────────────────────────────────────────
// Function: sleep_test_step
// Desc.:    4.3-Selbsttest fuer F$Sleep: legt sich beim ersten Aufruf per F$Sleep(2 Ticks)
//           schlafen, beendet sich beim zweiten Aufruf (nach dem Aufwachen) mit Exit-Code 55.
//────────────────────────────────────────────────────────────────────────────────────────────────
static int sleep_step_calls = 0;

static void sleep_test_step(void)
{
    sleep_step_calls++;
    if (sleep_step_calls == 1) {
        q9_regs_t sl = {0};
        sl.d[1] = 2;                                    /* F$Sleep: 2 Ticks                        */
        q9_syscall(F_SLEEP, &sl);
        return;
    }
    {
        q9_regs_t ex = {0};
        ex.d[1] = 55;
        q9_syscall(F_EXIT, &ex);
    }
}

//────────────────────────────────────────────────────────────────────────────────────────────────
// Function: test_fm_open
// Desc.:    Minimaler Test-File-Manager (3.2-Selbsttest): "open" akzeptiert nur den Rest-Pfad
//           "datei", legt dessen Laenge im Datei-Kontext (fmctx) ab. Alles andere -> E$PNNF.
//           Beweist NUR das Routing der VFS-Schicht (device.h/vfs.c) — kein echtes Dateisystem
//           (das kommt in 3.3/3.4 als FAT16-Manager).
//────────────────────────────────────────────────────────────────────────────────────────────────
static int test_fm_open(q9_dev_t *dev, q9_path_t *p, const char *restpath, uint32_t len, uint8_t mode)
{
    (void)dev; (void)mode;
    if (len != 5 || restpath[0] != 'd' || restpath[1] != 'a' || restpath[2] != 't' ||
        restpath[3] != 'e' || restpath[4] != 'i') {
        return E_PNNF;
    }
    p->fmctx[0] = (uint8_t)len;                        /* Beweis: Kontext liegt im Pfad, nicht im */
    return 0;                                          /*   Geraet — pro offenem Pfad eigener Stand */
}

static const q9_fm_t test_fm = { "testfm", test_fm_open, 0, 0, 0, 0, 0, 0 };

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_kernel_selftest
// Desc.:    Syscall-Selbsttest: prüft Erfolgs- und Fehlerpfade der Phase-1-ABI.
//           Rückgabe 0 = alle Checks ok, sonst Anzahl Fehler.
// Call:     fails = q9_kernel_selftest()
//════════════════════════════════════════════════════════════════════════════════════════════════
int q9_kernel_selftest(void)
{
    int fails = 0;

    struct {
        const char *name;
        int         ok;
    } checks[96];
    int nchecks = 0;

    {   /* I$WritLn on stdout succeeds and reports the byte count */
        q9_regs_t r = {0};
        static const char msg[] = "selftest: I$WritLn ok\r";
        r.d[0] = 1;
        r.d[1] = sizeof(msg) - 1;
        r.a[0] = (void *)msg;
        int err = q9_syscall(I_WRITLN, &r);
        checks[nchecks].name = "I$WritLn stdout";
        checks[nchecks++].ok = (err == 0 && r.d[1] == sizeof(msg) - 1);
    }
    {   /* bad path number is rejected */
        q9_regs_t r = {0};
        uint8_t   b = 'x';
        r.d[0] = 7;
        r.d[1] = 1;
        r.a[0] = &b;
        checks[nchecks].name = "I$Write Pfad 7 -> E$BPNum";
        checks[nchecks++].ok = (q9_syscall(I_WRITE, &r) == E_BPNUM);
    }
    {   /* unknown function code is rejected */
        q9_regs_t r = {0};
        checks[nchecks].name = "Func $7F -> E$UnkSvc";
        checks[nchecks++].ok = (q9_syscall(0x7f, &r) == E_UNKSVC);
    }
    {   /* read without pending input reports not-ready (phase 1 semantics) */
        q9_regs_t r = {0};
        uint8_t   buf[8];
        r.d[0] = 0;
        r.d[1] = sizeof(buf);
        r.a[0] = buf;
        checks[nchecks].name = "I$Read leer -> E$NotRdy";
        checks[nchecks++].ok = (q9_syscall(I_READ, &r) == E_NOTRDY);
    }
    {   /* F$ID returns the proto process */
        q9_regs_t r = {0};
        checks[nchecks].name = "F$ID -> PID 1";
        checks[nchecks++].ok = (q9_syscall(F_ID, &r) == 0 && r.d[0] == 1);
    }
    {   /* 4.1: Prozesstabelle — PID 1 (REPL) korrekt angelegt */
        q9_pd_t *pd = q9_proc_find(1);
        checks[nchecks].name = "Prozesstabelle: PID 1 (Parent 0, ACTIVE, Std-Pfade 0/1/2)";
        checks[nchecks++].ok = (pd != 0 && pd->parent == 0 && pd->module == 0 &&
                                 pd->state == Q9_PS_ACTIVE && pd->stdpath[0] == 0 &&
                                 pd->stdpath[1] == 1 && pd->stdpath[2] == 2);
    }
    {   /* 4.1: unbekannte PID liefert NULL */
        checks[nchecks].name = "Prozesstabelle: unbekannte PID -> NULL";
        checks[nchecks++].ok = (q9_proc_find(99) == 0);
    }
    {   /* 4.2: F$Fork startet ein Kind, das per F$Exit endet; F$Wait sammelt es ein */
        static uint8_t childbuf[64];
        int            ok;
        q9_regs_t      fk = {0};
        uint32_t       childpid;
        q9_regs_t      wt = {0};

        build_native_module(childbuf, "child", 1, child_step);
        ok = (q9_mod_register((const q9_modhdr_t *)childbuf) == 0);

        fk.a[0] = (void *)"child";
        fk.d[1] = Q9_MOD_PRGRM;
        fk.d[2] = Q9_MOD_NATIVE;
        ok = ok && (q9_syscall(F_FORK, &fk) == 0);
        childpid = fk.d[0];
        ok = ok && (childpid != 0 && childpid != 1);

        /* Kind ist ACTIVE, hat aber noch keinen Tick bekommen -> noch kein Exit -> E$NotRdy */
        ok = ok && (q9_syscall(F_WAIT, &wt) == E_NOTRDY);

        q9_kernel_step();                               /* ein Scheduler-Tick: repl_step + child_step */

        {
            q9_regs_t wt2 = {0};
            ok = ok && (q9_syscall(F_WAIT, &wt2) == 0 && wt2.d[0] == childpid && wt2.d[1] == 42);
        }
        {   /* kein Kind mehr uebrig -> E$NoChld */
            q9_regs_t wt3 = {0};
            ok = ok && (q9_syscall(F_WAIT, &wt3) == E_NOCHLD);
        }
        checks[nchecks].name = "F$Fork/F$Exit/F$Wait: Kind gestartet, beendet, eingesammelt";
        checks[nchecks++].ok = ok;
    }
    {   /* 4.2: F$Fork auf unbekanntes Modul -> E$MNF */
        q9_regs_t fk = {0};
        fk.a[0] = (void *)"nichtdaexistierendesmodul";
        fk.d[1] = Q9_MOD_PRGRM;
        fk.d[2] = Q9_MOD_NATIVE;
        checks[nchecks].name = "F$Fork unbekanntes Modul -> E$MNF";
        checks[nchecks++].ok = (q9_syscall(F_FORK, &fk) == E_MNF);
    }
    {   /* 4.2: F$Fork auf ein NICHT-natives Modul (kein Q9_MOD_NATIVE) -> E$NEMod */
        static uint8_t regbuf[48];
        q9_regs_t      fk = {0};
        int            ok;

        build_module(regbuf, "notnative", Q9_MOD_PRGRM, Q9_MOD_M68K, 1);
        ok = (q9_mod_register((const q9_modhdr_t *)regbuf) == 0);
        fk.a[0] = (void *)"notnative";
        fk.d[1] = Q9_MOD_PRGRM;
        fk.d[2] = Q9_MOD_M68K;
        ok = ok && (q9_syscall(F_FORK, &fk) == E_NEMOD);
        checks[nchecks].name = "F$Fork nicht-natives Modul -> E$NEMod";
        checks[nchecks++].ok = ok;
    }
    {   /* 4.2: F$Wait ohne jemals ein Kind gehabt zu haben -> E$NoChld */
        q9_regs_t wt = {0};
        checks[nchecks].name = "F$Wait ohne Kinder -> E$NoChld";
        checks[nchecks++].ok = (q9_syscall(F_WAIT, &wt) == E_NOCHLD);
    }
    {   /* 4.2: F$Chain — Kind verkettet sich auf ein anderes Modul, das dann wirklich laeuft */
        static uint8_t beforebuf[64];
        static uint8_t afterbuf[64];
        int            ok;
        q9_regs_t      fk = {0};
        uint32_t       childpid;

        build_native_module(beforebuf, "child_before", 1, child_before_chain_step);
        build_native_module(afterbuf, "child_after", 1, child_after_chain_step);
        ok = (q9_mod_register((const q9_modhdr_t *)beforebuf) == 0);
        ok = ok && (q9_mod_register((const q9_modhdr_t *)afterbuf) == 0);

        fk.a[0] = (void *)"child_before";
        fk.d[1] = Q9_MOD_PRGRM;
        fk.d[2] = Q9_MOD_NATIVE;
        ok = ok && (q9_syscall(F_FORK, &fk) == 0);
        childpid = fk.d[0];

        q9_kernel_step();                               /* Tick 1: child_before_chain_step -> Chain */
        {
            q9_regs_t wt = {0};
            ok = ok && (q9_syscall(F_WAIT, &wt) == E_NOTRDY); /* laeuft noch (jetzt als "child_after")*/
        }
        q9_kernel_step();                               /* Tick 2: child_after_chain_step -> Exit(77)*/
        {
            q9_regs_t wt2 = {0};
            ok = ok && (q9_syscall(F_WAIT, &wt2) == 0 &&
                        wt2.d[0] == childpid && wt2.d[1] == 77);
        }
        checks[nchecks].name = "F$Chain: Kind wechselt Modul, laeuft weiter, endet mit neuem Code";
        checks[nchecks++].ok = ok;
    }
    {   /* 4.3: I$ReadLn ohne Eingabe versetzt den Prozess in WAITING/Q9_WAIT_DEVICE — der       */
        /* Scheduler steppt ihn danach nicht mehr, bis /term SS.Ready meldet (im Testharness nie, */
        /* stdin ist nicht-blockierend ohne echte Eingabe, s. hal_posix.c). Forkt direkt ueber     */
        /* q9_proc_fork (proc.c) statt F$Fork/Modul-Directory — braucht kein registriertes Modul   */
        /* und belegt deshalb keinen der nur 8 Directory-Slots dauerhaft.                          */
        int            ok;
        uint32_t       childpid;
        q9_pd_t       *pd;
        q9_dev_t      *term;

        readln_block_calls = 0;
        ok = (q9_proc_fork(1, 0, readln_block_step, &childpid) == 0);

        q9_kernel_step();                               /* Tick 1: I$ReadLn -> E$NotRdy -> WAITING */
        pd   = q9_proc_find(childpid);
        term = q9_dev_find("term");
        ok = ok && (readln_block_calls == 1);
        ok = ok && (pd != 0 && pd->state == Q9_PS_WAITING &&
                    pd->wait_reason == Q9_WAIT_DEVICE && pd->wait_dev == term);

        q9_kernel_step();                               /* Tick 2/3: WAITING -> Scheduler skippt   */
        q9_kernel_step();
        ok = ok && (readln_block_calls == 1);           /* Step wurde NICHT erneut aufgerufen       */

        q9_proc_exit(childpid, 0);                       /* Testprozess aufraeumen (kein SS.Ready    */
        {                                                 /*   im Testharness moeglich)               */
            q9_regs_t wt = {0};
            ok = ok && (q9_syscall(F_WAIT, &wt) == 0 && wt.d[0] == childpid);
        }
        checks[nchecks].name = "4.3: I$ReadLn ohne Eingabe -> WAITING/Q9_WAIT_DEVICE, Scheduler skippt";
        checks[nchecks++].ok = ok;
    }
    {   /* 4.3: F$Wait ohne Zombie-Kind versetzt den wartenden Prozess ebenso in WAITING/          */
        /* Q9_WAIT_CHILD — sobald das Kind zum Zombie wird, weckt der Scheduler ihn automatisch.   */
        /* Ob Enkel oder Parent innerhalb desselben Ticks zuerst an der Reihe sind, haengt von      */
        /* ihrer Tabellenposition ab (Round-Robin) — die Schleife unten ist deshalb bewusst nicht  */
        /* auf einen festen Tick-Wert fixiert, sondern laeuft bis zum Aufwachen oder Abbruch.       */
        /* Wie beim I$ReadLn-Test oben: q9_proc_fork direkt statt F$Fork/Modul-Directory (weder     */
        /* Parent noch Enkel brauchen ein registriertes Modul, s.o.)                                */
        int            ok;
        uint32_t       parentpid;
        q9_pd_t       *ppd;

        wait_parent_calls  = 0;
        wait_parent_result = -1;
        ok = (q9_proc_fork(1, 0, wait_parent_step, &parentpid) == 0);

        q9_kernel_step();                               /* Tick 1: forkt Enkel, F$Wait -> E$NotRdy */
                                                         /*   -> WAITING/Q9_WAIT_CHILD (Enkel kann  */
                                                         /*   diesen Tick unmoeglich schon fertig   */
                                                         /*   sein — er wurde ja gerade erst gestartet)*/
        ppd = q9_proc_find(parentpid);
        ok = ok && (wait_parent_calls == 1 && wait_parent_result == E_NOTRDY);
        ok = ok && (ppd != 0 && ppd->state == Q9_PS_WAITING && ppd->wait_reason == Q9_WAIT_CHILD);

        for (int i = 0; i < Q9_NPROCS + 2 && wait_parent_calls < 2; i++) {
            q9_kernel_step();                           /* bis Enkel Zombie ist und Parent weckt    */
        }
        ok = ok && (wait_parent_calls == 2 && wait_parent_result == 0 &&
                    wait_parent_reaped_pid == wait_parent_gcpid);

        checks[nchecks].name = "4.3: F$Wait ohne Zombie -> WAITING/Q9_WAIT_CHILD, weckt bei Exit";
        checks[nchecks++].ok = ok;

        q9_proc_exit(parentpid, 0);                      /* Testprozess aufraeumen, bevor er als    */
        {                                                 /*   ACTIVE weiter F$Wait -> E$NoChld rufen*/
            q9_regs_t wt = {0};                           /*   wuerde                                 */
            q9_syscall(F_WAIT, &wt);
        }
    }
    {   /* 4.3: F$Sleep legt den Prozess fuer N Ticks schlafen (Q9_WAIT_TIMER) — der Scheduler     */
        /* steppt ihn erst wieder, wenn wake_tick erreicht ist. q9_proc_fork direkt, s.o.          */
        int            ok;
        uint32_t       childpid;
        q9_pd_t       *pd;

        sleep_step_calls = 0;
        ok = (q9_proc_fork(1, 0, sleep_test_step, &childpid) == 0);

        q9_kernel_step();                               /* Tick 1: F$Sleep(2) -> SLEEPING           */
        pd = q9_proc_find(childpid);
        ok = ok && (sleep_step_calls == 1 && pd != 0 &&
                    pd->state == Q9_PS_SLEEPING && pd->wait_reason == Q9_WAIT_TIMER);

        q9_kernel_step();                               /* Tick 2: noch schlafend (1 von 2 Ticks)   */
        ok = ok && (sleep_step_calls == 1);

        q9_kernel_step();                               /* Tick 3: wach, beendet sich mit Code 55   */
        ok = ok && (sleep_step_calls == 2);

        {
            q9_regs_t wt = {0};
            ok = ok && (q9_syscall(F_WAIT, &wt) == 0 && wt.d[0] == childpid && wt.d[1] == 55);
        }
        checks[nchecks].name = "4.3: F$Sleep legt Prozess fuer N Ticks schlafen (Q9_WAIT_TIMER)";
        checks[nchecks++].ok = ok;
    }
    {   /* 4.4: F$SSpd suspendiert einen Prozess dauerhaft (WAITING/Q9_WAIT_SIGNAL) — bewusst OHNE */
        /* Weckmechanismus vor F$Send (4.5): der Scheduler steppt ihn nie wieder von selbst.       */
        int            ok;
        uint32_t       childpid;
        q9_pd_t       *pd;
        q9_regs_t      sq = {0};

        sspd_calls = 0;
        ok = (q9_proc_fork(1, 0, sspd_test_step, &childpid) == 0);

        q9_kernel_step();                               /* Tick 1: laeuft normal                    */
        ok = ok && (sspd_calls == 1);

        sq.d[0] = childpid;
        ok = ok && (q9_syscall(F_SSPD, &sq) == 0);
        pd = q9_proc_find(childpid);
        ok = ok && (pd != 0 && pd->state == Q9_PS_WAITING && pd->wait_reason == Q9_WAIT_SIGNAL);

        q9_kernel_step();                               /* Tick 2/3: bleibt WAITING, wird nicht     */
        q9_kernel_step();                               /*   erneut gestept                         */
        ok = ok && (sspd_calls == 1);

        {
            q9_regs_t sq2 = {0};
            sq2.d[0] = 999;                              /* unbekannte PID */
            ok = ok && (q9_syscall(F_SSPD, &sq2) == E_IPRCID);
        }

        q9_proc_exit(childpid, 0);                       /* Testprozess aufraeumen                  */
        {
            q9_regs_t wt = {0};
            ok = ok && (q9_syscall(F_WAIT, &wt) == 0 && wt.d[0] == childpid);
        }
        checks[nchecks].name = "4.4: F$SSpd -> WAITING/Q9_WAIT_SIGNAL, Scheduler skippt dauerhaft";
        checks[nchecks++].ok = ok;
    }
    {   /* 4.4: F$SPrior setzt die Prioritaet (reines Datenfeld) und liefert den alten Wert zurueck */
        int       ok;
        q9_regs_t sp1 = {0};
        q9_regs_t sp2 = {0};
        q9_regs_t sp3 = {0};

        sp1.d[0] = 1;                                    /* PID 1 (REPL), Default-Prioritaet 0       */
        sp1.d[1] = 5;
        ok = (q9_syscall(F_SPRIOR, &sp1) == 0 && sp1.d[1] == 0);

        sp2.d[0] = 1;
        sp2.d[1] = 9;
        ok = ok && (q9_syscall(F_SPRIOR, &sp2) == 0 && sp2.d[1] == 5);

        sp3.d[0] = 999;                                  /* unbekannte PID -> E$IPrcID                */
        ok = ok && (q9_syscall(F_SPRIOR, &sp3) == E_IPRCID);

        checks[nchecks].name = "4.4: F$SPrior setzt Prioritaet, liefert alten Wert, E$IPrcID sonst";
        checks[nchecks++].ok = ok;
    }
    {   /* 4.5: F$Send lenkt bei installiertem F$Icpt-Handler den naechsten Scheduler-Aufruf auf   */
        /* diesen um (statt der normalen Step-Funktion); F$RTE schaltet danach zurueck             */
        int            ok;
        uint32_t       childpid;
        q9_pd_t       *pd;
        q9_regs_t      sd = {0};

        icpt_normal_calls    = 0;
        icpt_handler_calls   = 0;
        icpt_received_signal = 0;
        ok = (q9_proc_fork(1, 0, icpt_test_step, &childpid) == 0);

        q9_kernel_step();                               /* Tick 1: normal, installiert Handler      */
        ok = ok && (icpt_normal_calls == 1 && icpt_handler_calls == 0);

        sd.d[0] = childpid;
        sd.d[1] = 42;                                    /* Signal-Nummer                            */
        ok = ok && (q9_syscall(F_SEND, &sd) == 0);
        pd = q9_proc_find(childpid);
        ok = ok && (pd != 0 && pd->in_intercept == 1);

        q9_kernel_step();                               /* Tick 2: Scheduler ruft icpt_handler_step */
        ok = ok && (icpt_handler_calls == 1 && icpt_received_signal == 42 &&
                    icpt_normal_calls == 1 && pd->in_intercept == 0); /* F$RTE hat zurueckgeschaltet */

        q9_kernel_step();                               /* Tick 3: wieder normale Step-Funktion      */
        ok = ok && (icpt_normal_calls == 2 && icpt_handler_calls == 1);

        {
            q9_regs_t sd2 = {0};
            sd2.d[0] = 999;                              /* unbekannte PID -> E$IPrcID                */
            ok = ok && (q9_syscall(F_SEND, &sd2) == E_IPRCID);
        }

        q9_proc_exit(childpid, 0);                       /* Testprozess aufraeumen                  */
        {
            q9_regs_t wt = {0};
            ok = ok && (q9_syscall(F_WAIT, &wt) == 0 && wt.d[0] == childpid);
        }
        checks[nchecks].name = "4.5: F$Send lenkt auf F$Icpt-Handler um, F$RTE schaltet zurueck";
        checks[nchecks++].ok = ok;
    }
    {   /* 4.5: F$Send bricht WAITING (hier Q9_WAIT_DEVICE) ab, auch ohne installierten Intercept- */
        /* Handler — der Prozess wird dann einfach ACTIVE, kein Umweg ueber einen Handler           */
        int            ok;
        uint32_t       childpid;
        q9_pd_t       *pd;
        q9_regs_t      sd = {0};

        readln_block_calls = 0;
        ok = (q9_proc_fork(1, 0, readln_block_step, &childpid) == 0);

        q9_kernel_step();                               /* Tick 1: I$ReadLn -> E$NotRdy -> WAITING  */
        pd = q9_proc_find(childpid);
        ok = ok && (pd != 0 && pd->state == Q9_PS_WAITING);

        sd.d[0] = childpid;
        sd.d[1] = 7;
        ok = ok && (q9_syscall(F_SEND, &sd) == 0);
        ok = ok && (pd->state == Q9_PS_ACTIVE && pd->in_intercept == 0);

        q9_proc_exit(childpid, 0);                       /* Testprozess aufraeumen                  */
        {
            q9_regs_t wt = {0};
            ok = ok && (q9_syscall(F_WAIT, &wt) == 0 && wt.d[0] == childpid);
        }
        checks[nchecks].name = "4.5: F$Send bricht WAITING ab (ohne Handler bleibt Prozess ACTIVE)";
        checks[nchecks++].ok = ok;
    }
    {   /* 4.5: F$Icpt/F$RTE ausserhalb eines Prozesses (kein q9_proc_current()) -> E$IPrcID */
        q9_regs_t r1 = {0};
        q9_regs_t r2 = {0};
        int       ok = (q9_syscall(F_ICPT, &r1) == E_IPRCID);
        ok = ok && (q9_syscall(F_RTE, &r2) == E_IPRCID);
        checks[nchecks].name = "4.5: F$Icpt/F$RTE ausserhalb eines Prozesses -> E$IPrcID";
        checks[nchecks++].ok = ok;
    }
    {   /* F$Time delivers uptime */
        q9_regs_t r = {0};
        checks[nchecks].name = "F$Time ok";
        checks[nchecks++].ok = (q9_syscall(F_TIME, &r) == 0);
    }
    {   /* standard paths 0/1/2 sit on the /term device */
        q9_dev_t *term = q9_dev_find("term");
        int ok = (term != 0);
        for (uint32_t i = 0; i < 3; i++) {
            q9_path_t *p = q9_path_get(i);
            ok = ok && p && p->dev == term && p->mode == Q9_MODE_UPDATE;
        }
        checks[nchecks].name = "Pfade 0/1/2 -> /term";
        checks[nchecks++].ok = ok;
    }
    {   /* write on a read-only path is rejected with E$BMode */
        q9_regs_t r = {0};
        uint8_t   b = 'x';
        int path = q9_path_open("term", Q9_MODE_READ);
        r.d[0] = (uint32_t)path;
        r.d[1] = 1;
        r.a[0] = &b;
        checks[nchecks].name = "I$Write auf Lesepfad -> E$BMode";
        checks[nchecks++].ok = (path == 3 && q9_syscall(I_WRITE, &r) == E_BMODE);
    }
    {   /* closing frees the path: same write now yields E$BPNum */
        q9_regs_t r = {0};
        uint8_t   b = 'x';
        r.d[0] = 3;
        r.d[1] = 1;
        r.a[0] = &b;
        checks[nchecks].name = "q9_path_close -> Pfad 3 wieder E$BPNum";
        checks[nchecks++].ok = (q9_path_close(3) == 0 && q9_syscall(I_WRITE, &r) == E_BPNUM);
    }
    {   /* I$Dup clones stdout to the lowest free path, I$Close frees it again */
        q9_regs_t r = {0};
        static const char msg[] = "selftest: I$Dup ok\r";
        int ok;
        r.d[0] = 1;
        ok = (q9_syscall(I_DUP, &r) == 0 && r.d[0] == 3);
        r.d[1] = sizeof(msg) - 1;
        r.a[0] = (void *)msg;
        ok = ok && (q9_syscall(I_WRITLN, &r) == 0);     /* write via the duplicate               */
        checks[nchecks].name = "I$Dup Pfad 1 -> 3, schreibbar";
        checks[nchecks++].ok = ok;

        q9_regs_t c = {0};
        c.d[0] = 3;
        ok = (q9_syscall(I_CLOSE, &c) == 0);
        ok = ok && (q9_syscall(I_WRITLN, &r) == E_BPNUM);
        c.d[0] = 3;
        ok = ok && (q9_syscall(I_CLOSE, &c) == E_BPNUM);
        checks[nchecks].name = "I$Close gibt Duplikat frei";
        checks[nchecks++].ok = ok;
    }
    {   /* F$PrsNam splits "/term/xyz" into name "term" + delimiter '/' */
        q9_regs_t r = {0};
        static const char pl[] = "/term/xyz";
        r.a[0] = (void *)pl;
        int ok = (q9_syscall(F_PRSNAM, &r) == 0 &&
                  r.d[1] == 4 && (const char *)r.a[0] == pl + 1 &&
                  (const char *)r.a[1] == pl + 5 && r.d[0] == '/');
        r.a[0] = r.a[1];                               /* chain: parse the next element         */
        ok = ok && (q9_syscall(F_PRSNAM, &r) == 0 && r.d[1] == 3);
        checks[nchecks].name = "F$PrsNam '/term/xyz' -> term, xyz";
        checks[nchecks++].ok = ok;
    }
    {   /* empty pathlist element is rejected */
        q9_regs_t r = {0};
        r.a[0] = (void *)"//x";
        checks[nchecks].name = "F$PrsNam '//x' -> E$BPNam";
        checks[nchecks++].ok = (q9_syscall(F_PRSNAM, &r) == E_BPNAM);
    }
    {   /* F$CmpNam: case-insensitive match, mismatch -> E$Diff */
        q9_regs_t r = {0};
        r.a[0] = (void *)"TERM";
        r.a[1] = (void *)"term";
        r.d[1] = 4;
        int ok = (q9_syscall(F_CMPNAM, &r) == 0);
        r.a[1] = (void *)"trem";
        ok = ok && (q9_syscall(F_CMPNAM, &r) == E_DIFFER);
        checks[nchecks].name = "F$CmpNam TERM=term, TERM!=trem";
        checks[nchecks++].ok = ok;
    }
    {   /* I$Attach finds /term by pathlist name and bumps the link count */
        q9_regs_t  r = {0};
        q9_dev_t  *term  = q9_dev_find("term");
        uint8_t    links = term ? term->links : 0;
        r.a[0] = (void *)"/TERM";                      /* case-insensitive, leading slash        */
        int ok = (q9_syscall(I_ATTACH, &r) == 0 &&
                  (q9_dev_t *)r.a[2] == term && term->links == links + 1);
        checks[nchecks].name = "I$Attach /TERM -> Geraet term";
        checks[nchecks++].ok = ok;

        q9_regs_t d = {0};
        d.a[2] = r.a[2];
        ok = (q9_syscall(I_DETACH, &d) == 0 && term && term->links == links);
        d.a[2] = (void *)&d;                           /* not a device table entry               */
        ok = ok && (q9_syscall(I_DETACH, &d) == E_PARAM);
        checks[nchecks].name = "I$Detach gibt frei, Fremdzeiger -> E$Param";
        checks[nchecks++].ok = ok;
    }
    {   /* unknown device name is rejected */
        q9_regs_t r = {0};
        r.a[0] = (void *)"/disk0";
        checks[nchecks].name = "I$Attach /disk0 -> E$MNF";
        checks[nchecks++].ok = (q9_syscall(I_ATTACH, &r) == E_MNF);
    }
    {   /* second device /nil: write discards, read reports EOF */
        q9_regs_t r = {0};
        uint8_t   buf[4];
        int path = q9_path_open("/nil", Q9_MODE_UPDATE);
        int ok   = (path == 3);
        r.d[0] = (uint32_t)path;
        r.d[1] = sizeof(buf);
        r.a[0] = buf;
        ok = ok && (q9_syscall(I_WRITE, &r) == 0 && r.d[1] == sizeof(buf));
        r.d[1] = sizeof(buf);
        ok = ok && (q9_syscall(I_READ, &r) == E_EOF);
        ok = ok && (q9_path_close(3) == 0);
        checks[nchecks].name = "/nil: Write verwirft, Read -> E$EOF";
        checks[nchecks++].ok = ok;
    }
    {   /* I$GetStt: SS.Ready without input -> E$NotRdy, SS.EOF on /term -> ok */
        q9_regs_t r = {0};
        r.d[0] = 0;                                    /* stdin                                  */
        r.d[1] = SS_READY;
        int ok = (q9_syscall(I_GETSTT, &r) == E_NOTRDY);
        r.d[1] = SS_EOF;
        ok = ok && (q9_syscall(I_GETSTT, &r) == 0);
        r.d[1] = 0x7f;                                 /* unsupported SS code                    */
        ok = ok && (q9_syscall(I_GETSTT, &r) == E_UNKSVC);
        checks[nchecks].name = "I$GetStt SS.Ready/SS.EOF auf /term";
        checks[nchecks++].ok = ok;
    }
    {   /* I$GetStt SS.EOF on /nil reports E$EOF; I$SetStt has no codes yet */
        q9_regs_t r = {0};
        int path = q9_path_open("/nil", Q9_MODE_READ);
        r.d[0] = (uint32_t)path;
        r.d[1] = SS_EOF;
        int ok = (path == 3 && q9_syscall(I_GETSTT, &r) == E_EOF);
        ok = ok && (q9_syscall(I_SETSTT, &r) == E_UNKSVC);
        ok = ok && (q9_path_close(3) == 0);
        checks[nchecks].name = "I$GetStt /nil SS.EOF -> E$EOF, SetStt -> E$UnkSvc";
        checks[nchecks++].ok = ok;
    }
    {   /* F$STime/F$Time roundtrip: set 2026-07-03 20:15:00, expect it back (Friday) */
        q9_regs_t s = {0};
        q9_regs_t t = {0};
        s.d[0] = (20u << 16) | (15u << 8) | 0u;        /* Zeit: 20:15:00 (d0, MWOS-verifiziert)  */
        s.d[1] = (2026u << 16) | (7u << 8) | 3u;       /* Datum: 2026-07-03 (d1)                 */
        int ok = (q9_syscall(F_STIME, &s) == 0);
        ok = ok && (q9_syscall(F_TIME, &t) == 0);
        ok = ok && (t.d[1] == s.d[1]);                 /* gleiches Datum                         */
        ok = ok && ((t.d[0] >> 8) == (s.d[0] >> 8));   /* gleiche Stunde+Minute                  */
        ok = ok && (t.d[2] == 5);                      /* 2026-07-03 ist ein Freitag             */
        checks[nchecks].name = "F$STime/F$Time Roundtrip + Wochentag";
        checks[nchecks++].ok = ok;

        q9_regs_t b = {0};
        b.d[1] = (2026u << 16) | (13u << 8) | 3u;      /* Monat 13 im Datum (jetzt d1)           */
        checks[nchecks].name = "F$STime Monat 13 -> E$Param";
        checks[nchecks++].ok = (q9_syscall(F_STIME, &b) == E_PARAM);
    }
    {   /* q9_crc32: Standard-Referenzwert fuer CRC-32/ISO-HDLC ueber "123456789" */
        static const uint8_t ref[] = "123456789";
        checks[nchecks].name = "q9_crc32 Referenzwert (123456789 -> $CBF43926)";
        checks[nchecks++].ok = (q9_crc32(ref, sizeof(ref) - 1) == 0xCBF43926u);
    }
    {   /* q9_mod_scan_first/next: zwei Module lueckenlos im ROM-Image, Sprung um ModuleSize */
        static uint8_t rom[2 * Q9_MOD_HDRSIZE];
        q9_modhdr_t   *h1 = (q9_modhdr_t *)rom;
        q9_modhdr_t   *h2 = (q9_modhdr_t *)(rom + Q9_MOD_HDRSIZE);

        h1->sync[0] = Q9_MOD_SYNC0;
        h1->sync[1] = Q9_MOD_SYNC1;
        h1->modsize = Q9_MOD_HDRSIZE;                  /* zweites Modul folgt direkt              */
        h2->sync[0] = Q9_MOD_SYNC0;
        h2->sync[1] = Q9_MOD_SYNC1;
        h2->modsize = Q9_MOD_HDRSIZE;                  /* Sprung fuehrt aus dem Blob heraus        */

        const q9_modhdr_t *first = q9_mod_scan_first(rom, sizeof(rom));
        int ok = (first == h1);
        if (ok) {
            const q9_modhdr_t *second = q9_mod_scan_next(rom, sizeof(rom), first);
            ok = ok && (second == h2);
            ok = ok && second && (q9_mod_scan_next(rom, sizeof(rom), second) == 0);
        }
        checks[nchecks].name = "q9_mod_scan_first/next: 2 Module, Ende erkannt";
        checks[nchecks++].ok = ok;
    }
    {   /* q9_mod_scan_first: kein Sync im Blob -> NULL */
        static uint8_t rom[Q9_MOD_HDRSIZE] = {0};
        checks[nchecks].name = "q9_mod_scan_first ohne Sync -> NULL";
        checks[nchecks++].ok = (q9_mod_scan_first(rom, sizeof(rom)) == 0);
    }
    {   /* 2.3b: q9_mod_validate — gueltiges Modul (Groesse/Nameoffset/CRC ok) */
        static uint8_t buf[64];
        build_module(buf, "vmod", Q9_MOD_DRIVR, Q9_MOD_M68K, 1);
        checks[nchecks].name = "q9_mod_validate: gueltiges Modul -> 0";
        checks[nchecks++].ok = (q9_mod_validate(buf, sizeof(buf), (const q9_modhdr_t *)buf) == 0);
    }
    {   /* 2.3b: kaputte ModuleSize (kleiner als Header) -> E$BMHP */
        static uint8_t buf[64];
        build_module(buf, "vmod", Q9_MOD_DRIVR, Q9_MOD_M68K, 1);
        ((q9_modhdr_t *)buf)->modsize = Q9_MOD_HDRSIZE - 1;
        checks[nchecks].name = "q9_mod_validate: ModuleSize < Header -> E$BMHP";
        checks[nchecks++].ok = (q9_mod_validate(buf, sizeof(buf), (const q9_modhdr_t *)buf) == E_BMHP);
    }
    {   /* 2.3b: NameOffset zeigt aus dem Modul heraus -> E$BMHP */
        static uint8_t buf[64];
        build_module(buf, "vmod", Q9_MOD_DRIVR, Q9_MOD_M68K, 1);
        ((q9_modhdr_t *)buf)->nameoff = ((q9_modhdr_t *)buf)->modsize;
        checks[nchecks].name = "q9_mod_validate: NameOffset ausserhalb -> E$BMHP";
        checks[nchecks++].ok = (q9_mod_validate(buf, sizeof(buf), (const q9_modhdr_t *)buf) == E_BMHP);
    }
    {   /* 2.3b: Datenbyte nach dem Header kaputt -> CRC-Mismatch -> E$BMCRC */
        static uint8_t buf[64];
        build_module(buf, "vmod", Q9_MOD_DRIVR, Q9_MOD_M68K, 1);
        buf[Q9_MOD_HDRSIZE] ^= 0xff;                    /* erstes Namensbyte kippen                */
        checks[nchecks].name = "q9_mod_validate: kaputtes Byte -> E$BMCRC";
        checks[nchecks++].ok = (q9_mod_validate(buf, sizeof(buf), (const q9_modhdr_t *)buf) == E_BMCRC);
    }
    {   /* 2.3c: q9_mod_register + q9_mod_find — Basisfall, danach Revision-Tie-Break */
        static uint8_t bufA[64];
        static uint8_t bufB[64];
        build_module(bufA, "regmod", Q9_MOD_PRGRM, Q9_MOD_M68K, 1);
        build_module(bufB, "regmod", Q9_MOD_PRGRM, Q9_MOD_M68K, 2);   /* hoehere Revision           */

        int ok = (q9_mod_register((const q9_modhdr_t *)bufA) == 0);
        ok = ok && (q9_mod_find("regmod", Q9_MOD_PRGRM, 0) == (const q9_modhdr_t *)bufA);
        ok = ok && (q9_mod_register((const q9_modhdr_t *)bufB) == 0);
        ok = ok && (q9_mod_find("regmod", Q9_MOD_PRGRM, 0) == (const q9_modhdr_t *)bufB);
        checks[nchecks].name = "q9_mod_register/find: hoehere Revision gewinnt";
        checks[nchecks++].ok = ok;

        build_module(bufA, "regmod", Q9_MOD_PRGRM, Q9_MOD_M68K, 2);   /* Gleichstand: A neu gebaut  */
        ok = (q9_mod_register((const q9_modhdr_t *)bufA) == 0);
        ok = ok && (q9_mod_find("regmod", Q9_MOD_PRGRM, 0) == (const q9_modhdr_t *)bufB);
        checks[nchecks].name = "q9_mod_register: Gleichstand -> etabliertes Modul bleibt";
        checks[nchecks++].ok = ok;
    }
    {   /* 2.3d: F$Link/F$UnLink ueber die Directory (regmod ist seit dem vorigen Block bekannt) */
        q9_regs_t r = {0};
        static const char name[] = "regmod";
        r.a[0] = (void *)name;
        r.d[1] = Q9_MOD_PRGRM;
        r.d[2] = 0;                                      /* Language: beliebig                     */
        int ok = (q9_syscall(F_LINK, &r) == 0);
        ok = ok && (r.a[1] != 0) && (r.d[0] == 2);        /* Revision 2 (gewann den Tie-Break oben)  */
        checks[nchecks].name = "F$Link: regmod gefunden, Revision im d0";
        checks[nchecks++].ok = ok;

        q9_regs_t u = {0};
        u.a[1] = r.a[1];
        checks[nchecks].name = "F$UnLink: bekanntes Modul -> 0";
        checks[nchecks++].ok = (q9_syscall(F_UNLINK, &u) == 0);

        q9_regs_t miss = {0};
        static const char noname[] = "keinmodul";
        miss.a[0] = (void *)noname;
        checks[nchecks].name = "F$Link: unbekannter Name -> E$MNF";
        checks[nchecks++].ok = (q9_syscall(F_LINK, &miss) == E_MNF);
    }
    {   /* 3.1: /d0 Block-Device — SS.BlkWr/SS.BlkRd-Roundtrip ueber die HAL (q9disk.img).        */
        /*      LBA 1 wird dafuer benutzt und danach WIEDERHERGESTELLT (3.3: /d0 kann inzwischen  */
        /*      ein echtes FAT16-Image sein, dessen FAT typischerweise bei LBA 1 beginnt — dieser */
        /*      Test darf sie nicht dauerhaft mit Testmuster ueberschreiben). */
        static uint8_t wbuf[Q9_BLK_SIZE];
        static uint8_t rbuf[Q9_BLK_SIZE];
        static uint8_t origbuf[Q9_BLK_SIZE];
        q9_regs_t r = {0};
        int path = q9_path_open("/d0", Q9_MODE_UPDATE);
        int ok = (path == 3);
        int had_orig;

        r.d[0] = (uint32_t)path;
        r.d[1] = SS_BLKRD;
        r.d[2] = 1;                                     /* LBA 1 vorher sichern                   */
        r.a[0] = origbuf;
        had_orig = ok && (q9_syscall(I_GETSTT, &r) == 0);

        for (uint32_t i = 0; i < sizeof(wbuf); i++) {
            wbuf[i] = (uint8_t)(i * 7 + 3);
        }
        r.d[1] = SS_BLKWR;
        r.a[0] = wbuf;
        ok = ok && (q9_syscall(I_SETSTT, &r) == 0);

        r.d[1] = SS_BLKRD;
        r.a[0] = rbuf;
        ok = ok && (q9_syscall(I_GETSTT, &r) == 0);
        for (uint32_t i = 0; ok && i < sizeof(wbuf); i++) {
            ok = ok && (rbuf[i] == wbuf[i]);
        }
        checks[nchecks].name = "/d0: SS.BlkWr/SS.BlkRd Roundtrip (LBA 1)";
        checks[nchecks++].ok = ok;

        if (had_orig) {
            r.d[1] = SS_BLKWR;
            r.a[0] = origbuf;
            q9_syscall(I_SETSTT, &r);                    /* LBA 1 wiederherstellen                 */
        }

        r.d[1] = SS_BLKWR;
        r.a[0] = 0;
        ok = (q9_syscall(I_SETSTT, &r) == E_PARAM);
        r.d[1] = SS_READY;                               /* unbekannt fuer /d0                     */
        ok = ok && (q9_syscall(I_GETSTT, &r) == E_UNKSVC);
        ok = ok && (q9_path_close((uint32_t)path) == 0);
        checks[nchecks].name = "/d0: NULL-Puffer -> E$Param, unbek. SS-Code -> E$UnkSvc";
        checks[nchecks++].ok = ok;
    }
    {   /* 3.2: I$Open auf ein Geraet OHNE File-Manager — Rest-Pfad muss leer sein (Rueckwaerts-  */
        /*      Kompatibilitaet zu Phase 1/2: /term/xyz -> E$PNNF, /term allein -> normaler Pfad) */
        q9_regs_t r = {0};
        r.d[0] = Q9_MODE_UPDATE;
        r.a[0] = (void *)"/term/xyz";
        int ok = (q9_syscall(I_OPEN, &r) == E_PNNF);

        q9_regs_t r2 = {0};
        r2.d[0] = Q9_MODE_UPDATE;
        r2.a[0] = (void *)"/term";
        ok = ok && (q9_syscall(I_OPEN, &r2) == 0) && (r2.d[0] == 3);
        ok = ok && (q9_path_close(3) == 0);
        checks[nchecks].name = "I$Open: /term/xyz -> E$PNNF, /term -> Pfad 3";
        checks[nchecks++].ok = ok;
    }
    {   /* 3.2: I$Open auf ein Geraet MIT File-Manager — Rest-Pfad wird durchgereicht */
        q9_dev_t     *d0 = q9_dev_find("d0");
        const q9_fm_t *saved_fm = d0 ? d0->fm : 0;      /* 3.3 kann hier bereits FAT16 haengen —   */
        int ok = (d0 != 0);                             /*   nach dem Test wiederherstellen         */
        if (ok) {
            q9_dev_set_fm(d0, &test_fm);
        }
        q9_regs_t r = {0};
        r.d[0] = Q9_MODE_READ;
        r.a[0] = (void *)"/d0/datei";
        ok = ok && (q9_syscall(I_OPEN, &r) == 0) && (r.d[0] == 3);
        ok = ok && (q9_path_get(3) != 0) && (q9_path_get(3)->fmctx[0] == 5);
        ok = ok && (q9_path_close(3) == 0);

        q9_regs_t bad = {0};
        bad.d[0] = Q9_MODE_READ;
        bad.a[0] = (void *)"/d0/unbekannt";
        ok = ok && (q9_syscall(I_OPEN, &bad) == E_PNNF);
        if (d0) {
            q9_dev_set_fm(d0, saved_fm);                /* urspruenglichen File-Manager zurueck     */
        }
        checks[nchecks].name = "I$Open: /d0/datei ueber Test-File-Manager, /d0/unbekannt -> E$PNNF";
        checks[nchecks++].ok = ok;
    }
    {   /* 3.2: I$ChgDir setzt das globale Arbeitsverzeichnis, relative I$Open loest dagegen auf */
        q9_dev_t     *d0 = q9_dev_find("d0");
        const q9_fm_t *saved_fm = d0 ? d0->fm : 0;
        if (d0) {
            q9_dev_set_fm(d0, &test_fm);                /* Test-File-Manager fuer den relativen Fall */
        }

        q9_regs_t c = {0};
        c.a[0] = (void *)"/d0";
        int ok = (d0 != 0) && (q9_syscall(I_CHGDIR, &c) == 0);
        {
            const char *cwd = q9_vfs_cwd();
            ok = ok && cwd[0] == '/' && cwd[1] == 'd' && cwd[2] == '0' && cwd[3] == 0;
        }

        q9_regs_t r = {0};
        r.d[0] = Q9_MODE_READ;
        r.a[0] = (void *)"datei";                       /* relativ: kein fuehrender '/'            */
        ok = ok && (q9_syscall(I_OPEN, &r) == 0) && (r.d[0] == 3);
        ok = ok && (q9_path_close(3) == 0);

        q9_regs_t back = {0};
        back.a[0] = (void *)"/";
        ok = ok && (q9_syscall(I_CHGDIR, &back) == 0);
        if (d0) {
            q9_dev_set_fm(d0, saved_fm);                 /* urspruenglichen File-Manager zurueck     */
        }
        checks[nchecks].name = "I$ChgDir /d0 + relatives I$Open 'datei' -> Pfad 3";
        checks[nchecks++].ok = ok;
    }
    {   /* 3.4: I$Create/I$MakDir/I$Delete auf ein Geraet OHNE File-Manager (/term) -> E$UnkSvc
           (kein Dateisystem hinter dem Geraet, also kann es diese Operationen nicht anbieten) */
        q9_regs_t r = {0};
        r.d[0] = Q9_MODE_WRITE;                          /* gueltiger Modus, sonst E$BMode zuerst   */
        r.a[0] = (void *)"/term/neu.txt";
        int ok = (q9_syscall(I_CREATE, &r) == E_UNKSVC);
        q9_regs_t m = {0};
        m.a[0] = (void *)"/term/neudir";
        ok = ok && (q9_syscall(I_MAKDIR, &m) == E_UNKSVC);
        q9_regs_t d = {0};
        d.a[0] = (void *)"/term/xyz";
        ok = ok && (q9_syscall(I_DELETE, &d) == E_UNKSVC);
        checks[nchecks].name = "I$Create/I$MakDir/I$Delete ohne File-Manager -> E$UnkSvc";
        checks[nchecks++].ok = ok;
    }
    {   /* 3.3: FAT16 — nur relevant, wenn q9disk.img beim Boot als FAT16 erkannt wurde (von      */
        /*      test/06_test_fat16.py per Python vorbereitet; sonst hat /d0 kein fm, Checks       */
        /*      werden dann still uebersprungen — kein FEHLER, das Image ist z.B. bei Test 04/05  */
        /*      absichtlich kein FAT16). */
        q9_dev_t *d0 = q9_dev_find("d0");
        if (d0 && d0->fm) {
            q9_regs_t r = {0};
            uint8_t   buf[64];
            int       ok;

            r.d[0] = Q9_MODE_READ;
            r.a[0] = (void *)"/d0/HELLO.TXT";             /* 8.3-Name, per Python-Skript angelegt */
            ok = (q9_syscall(I_OPEN, &r) == 0);
            int path83 = ok ? (int)r.d[0] : -1;
            if (ok) {
                q9_regs_t rr = {0};
                rr.d[0] = (uint32_t)path83;
                rr.d[1] = sizeof(buf);
                rr.a[0] = buf;
                ok = ok && (q9_syscall(I_READ, &rr) == 0) && (rr.d[1] == 12);
                ok = ok && (buf[0] == 'H' && buf[1] == 'a' && buf[11] == '9');  /* "Hallo Q9!!!9" */
                ok = ok && (q9_path_close((uint32_t)path83) == 0);
            }
            checks[nchecks].name = "FAT16: 8.3-Datei /d0/HELLO.TXT lesen (Inhalt+Groesse)";
            checks[nchecks++].ok = ok;

            r = (q9_regs_t){0};
            r.d[0] = Q9_MODE_READ;
            r.a[0] = (void *)"/d0/This is a very long filename.txt";  /* erzwingt LFN-Eintraege   */
            ok = (q9_syscall(I_OPEN, &r) == 0);
            int pathlfn = ok ? (int)r.d[0] : -1;
            if (ok) {
                q9_regs_t rr = {0};
                rr.d[0] = (uint32_t)pathlfn;
                rr.d[1] = sizeof(buf);
                rr.a[0] = buf;
                ok = ok && (q9_syscall(I_READ, &rr) == 0) && (rr.d[1] == 9);
                ok = ok && (buf[0] == 'L' && buf[8] == '!');            /* "Langname!" */
                ok = ok && (q9_path_close((uint32_t)pathlfn) == 0);
            }
            checks[nchecks].name = "FAT16: LFN-Datei (langer Name) lesen";
            checks[nchecks++].ok = ok;

            r = (q9_regs_t){0};
            r.d[0] = Q9_MODE_READ;
            r.a[0] = (void *)"/d0/SUBDIR/NESTED.TXT";      /* Unterverzeichnis + Datei darin       */
            ok = (q9_syscall(I_OPEN, &r) == 0);
            int pathsub = ok ? (int)r.d[0] : -1;
            if (ok) {
                q9_regs_t rr = {0};
                rr.d[0] = (uint32_t)pathsub;
                rr.d[1] = sizeof(buf);
                rr.a[0] = buf;
                ok = ok && (q9_syscall(I_READ, &rr) == 0) && (rr.d[1] > 0);
                ok = ok && (q9_path_close((uint32_t)pathsub) == 0);
            }
            checks[nchecks].name = "FAT16: Datei in Unterverzeichnis lesen";
            checks[nchecks++].ok = ok;

            r = (q9_regs_t){0};
            r.d[0] = Q9_MODE_READ;
            r.a[0] = (void *)"/d0/HELLO.TXT";
            ok = (q9_syscall(I_OPEN, &r) == 0);
            int pathseek = ok ? (int)r.d[0] : -1;
            if (ok) {
                q9_regs_t sk = {0};
                q9_regs_t rr = {0};
                sk.d[0] = (uint32_t)pathseek;
                sk.d[1] = 6;                               /* I$Seek auf Position 6                */
                ok = ok && (q9_syscall(I_SEEK, &sk) == 0);
                rr.d[0] = (uint32_t)pathseek;
                rr.d[1] = sizeof(buf);
                rr.a[0] = buf;
                ok = ok && (q9_syscall(I_READ, &rr) == 0) && (rr.d[1] == 6);
                ok = ok && (buf[0] == 'Q');                 /* "Hallo Q9!!!9"[6..] == "Q9!!!9"       */
                ok = ok && (q9_path_close((uint32_t)pathseek) == 0);
            }
            checks[nchecks].name = "FAT16: I$Seek + I$Read ab Position 6";
            checks[nchecks++].ok = ok;

            r = (q9_regs_t){0};
            r.d[0] = Q9_MODE_READ;
            r.a[0] = (void *)"/d0/NICHTDA.TXT";
            checks[nchecks].name = "FAT16: unbekannte Datei -> E$PNNF";
            checks[nchecks++].ok = (q9_syscall(I_OPEN, &r) == E_PNNF);

            /* 3.4: FAT16 schreibend — I$Create, I$Write (ueber Cluster-Grenzen, um Allozieren zu   */
            /* pruefen), I$Open erneut lesend, I$MakDir, I$Delete. */
            {
                q9_regs_t cr = {0};
                cr.d[0] = Q9_MODE_WRITE;
                cr.a[0] = (void *)"/d0/NEU.TXT";
                ok = (q9_syscall(I_CREATE, &cr) == 0);
                int pathnew = ok ? (int)cr.d[0] : -1;
                if (ok) {
                    static const uint8_t payload[] = "Von Q9 geschrieben!";
                    q9_regs_t wr = {0};
                    wr.d[0] = (uint32_t)pathnew;
                    wr.d[1] = sizeof(payload) - 1;
                    wr.a[0] = (void *)payload;
                    ok = ok && (q9_syscall(I_WRITE, &wr) == 0) && (wr.d[1] == sizeof(payload) - 1);
                    ok = ok && (q9_path_close((uint32_t)pathnew) == 0);
                }
                checks[nchecks].name = "FAT16: I$Create + I$Write /d0/NEU.TXT";
                checks[nchecks++].ok = ok;

                r = (q9_regs_t){0};
                r.d[0] = Q9_MODE_READ;
                r.a[0] = (void *)"/d0/NEU.TXT";
                ok = (q9_syscall(I_OPEN, &r) == 0);
                int pathread = ok ? (int)r.d[0] : -1;
                if (ok) {
                    q9_regs_t rr = {0};
                    rr.d[0] = (uint32_t)pathread;
                    rr.d[1] = sizeof(buf);
                    rr.a[0] = buf;
                    ok = ok && (q9_syscall(I_READ, &rr) == 0) && (rr.d[1] == 19);
                    ok = ok && (buf[0] == 'V' && buf[18] == '!');
                    ok = ok && (q9_path_close((uint32_t)pathread) == 0);
                }
                checks[nchecks].name = "FAT16: neu geschriebene Datei zurueckgelesen";
                checks[nchecks++].ok = ok;

                {
                    q9_regs_t dup = {0};
                    dup.d[0] = Q9_MODE_WRITE;
                    dup.a[0] = (void *)"/d0/NEU.TXT";       /* existierender Name -> E$BPNam         */
                    checks[nchecks].name = "FAT16: I$Create auf existierenden Namen -> E$BPNam";
                    checks[nchecks++].ok = (q9_syscall(I_CREATE, &dup) == E_BPNAM);
                }

                {
                    q9_regs_t bad = {0};
                    bad.d[0] = Q9_MODE_WRITE;
                    bad.a[0] = (void *)"/d0/zulangerdateiname.txt"; /* kein gueltiger 8.3-Name */
                    checks[nchecks].name = "FAT16: I$Create mit LFN-pflichtigem Namen -> E$BPNam";
                    checks[nchecks++].ok = (q9_syscall(I_CREATE, &bad) == E_BPNAM);
                }

                {
                    /* NEUDIR bleibt bewusst bestehen (keine I$Delete-Aufraeumung hier) — die       */
                    /* Python-Nachvalidierung in test/06_test_fat16.py (post_validate) prueft nach   */
                    /* dem Selbsttest-Lauf explizit, dass NEUDIR als Verzeichnis mit korrekten       */
                    /* "."/".."-Eintraegen im Root steht (das ist der praktikable Ersatz fuer ein    */
                    /* echtes "am Mac mounten", ARBEITSPLAN.md 3.4). Das Image wird von test/06 vor  */
                    /* jedem Lauf komplett neu erzeugt (Python "wb"), daher keine Notwendigkeit,      */
                    /* NEUDIR danach wieder zu loeschen. */
                    q9_regs_t md = {0};
                    md.a[0] = (void *)"/d0/NEUDIR";
                    ok = (q9_syscall(I_MAKDIR, &md) == 0);
                    q9_regs_t r2 = {0};
                    r2.d[0] = Q9_MODE_READ;
                    r2.a[0] = (void *)"/d0/NEUDIR";
                    ok = ok && (q9_syscall(I_OPEN, &r2) == 0);
                    ok = ok && (q9_path_close(r2.d[0]) == 0);
                    checks[nchecks].name = "FAT16: I$MakDir /d0/NEUDIR + I$Open darauf";
                    checks[nchecks++].ok = ok;
                }

                /* 3.5: F$Load — Modul aus einer echten Datei (statt nur ROM-Image) laden, validieren,
                   registrieren. Q9 schreibt sich das Testmodul selbst per I$Create/I$Write auf /d0
                   (Nagelprobe Phase 2 + 3 zusammen: derselbe FAT16-Schreibpfad wie oben, diesmal mit
                   echtem Modul-Byte-Inhalt statt Text). Bewusst VOR dem I$Delete von /d0/NEU.TXT
                   platziert (statt danach): dir_alloc_slot (fat16.c) vergibt neue Directory-Slots
                   zuerst an bereits geloeschte/freie Eintraege — liefe dieser Block NACH dem
                   NEU.TXT-Delete, wuerde I$Create(/d0/LOADMOD.BIN) genau dessen frisch freigewordenen
                   Slot (und Cluster 6) wiederverwenden und damit den DIRENT_FREE-Marker ueberschreiben,
                   den die Python-Nachvalidierung in test/06_test_fat16.py (post_validate) anschliessend
                   erwartet. Aufraeumen per I$Delete am Ende dieses Blocks stellt den Zustand vor dem
                   Block wieder her (eigene Slots/Cluster wieder frei), sodass der NEU.TXT-Test danach
                   unveraendert funktioniert. */
                {
                    static uint8_t modbuf[128];
                    build_module(modbuf, "loadmod", Q9_MOD_PRGRM, Q9_MOD_M68K, 1);
                    uint32_t modlen = ((const q9_modhdr_t *)modbuf)->modsize;

                    q9_regs_t cr = {0};
                    cr.d[0] = Q9_MODE_WRITE;
                    cr.a[0] = (void *)"/d0/LOADMOD.BIN";
                    ok = (q9_syscall(I_CREATE, &cr) == 0);
                    int pathmod = ok ? (int)cr.d[0] : -1;
                    if (ok) {
                        q9_regs_t wr = {0};
                        wr.d[0] = (uint32_t)pathmod;
                        wr.d[1] = modlen;
                        wr.a[0] = (void *)modbuf;
                        ok = ok && (q9_syscall(I_WRITE, &wr) == 0) && (wr.d[1] == modlen);
                        ok = ok && (q9_path_close((uint32_t)pathmod) == 0);
                    }
                    checks[nchecks].name = "F$Load: Testmodul nach /d0/LOADMOD.BIN geschrieben";
                    checks[nchecks++].ok = ok;

                    q9_regs_t ld = {0};
                    ld.a[0] = (void *)"/d0/LOADMOD.BIN";
                    ok = (q9_syscall(F_LOAD, &ld) == 0);
                    ok = ok && (ld.a[1] != 0) && (ld.d[0] == 1);      /* Revision 1                */
                    ok = ok && (((const q9_modhdr_t *)ld.a[1])->execoff == Q9_MOD_HDRSIZE);
                    ok = ok && ((const uint8_t *)ld.a[2] == (const uint8_t *)ld.a[1] + Q9_MOD_HDRSIZE);
                    checks[nchecks].name = "F$Load: /d0/LOADMOD.BIN geladen, validiert, registriert";
                    checks[nchecks++].ok = ok;

                    /* Nagelprobe: das per F$Load registrierte Modul ist jetzt ganz normal per
                       F$Link ueber seinen Namen ansprechbar — genau wie ein ROM-Modul. */
                    q9_regs_t lk = {0};
                    static const char loadmodname[] = "loadmod";
                    lk.a[0] = (void *)loadmodname;
                    lk.d[1] = Q9_MOD_PRGRM;
                    ok = (q9_syscall(F_LINK, &lk) == 0);
                    ok = ok && (lk.a[1] == ld.a[1]) && (lk.d[0] == 1);
                    checks[nchecks].name = "F$Load + F$Link: dasselbe Modul ueber den Namen erreichbar";
                    checks[nchecks++].ok = ok;

                    q9_regs_t un1 = {0};
                    un1.a[1] = ld.a[1];
                    ok = (q9_syscall(F_UNLINK, &un1) == 0);           /* F$Load-Referenz senken     */
                    q9_regs_t un2 = {0};
                    un2.a[1] = lk.a[1];
                    ok = ok && (q9_syscall(F_UNLINK, &un2) == 0);     /* F$Link-Referenz senken     */
                    checks[nchecks].name = "F$UnLink: beide Referenzen auf loadmod sauber abgebaut";
                    checks[nchecks++].ok = ok;

                    q9_regs_t missing = {0};
                    missing.a[0] = (void *)"/d0/NICHTDA.MOD";
                    checks[nchecks].name = "F$Load: fehlende Datei -> E$PNNF";
                    checks[nchecks++].ok = (q9_syscall(F_LOAD, &missing) == E_PNNF);

                    /* Ungueltiges Modul (kaputte Sync-Bytes) -> E$BMHP, kein Directory-Eintrag */
                    {
                        static uint8_t badbuf[64];
                        build_module(badbuf, "badmod", Q9_MOD_PRGRM, Q9_MOD_M68K, 1);
                        badbuf[0] = 0x00;                              /* Sync kaputt -> Header ungueltig */
                        uint32_t badlen = ((const q9_modhdr_t *)badbuf)->modsize;

                        q9_regs_t bcr = {0};
                        bcr.d[0] = Q9_MODE_WRITE;
                        bcr.a[0] = (void *)"/d0/BADMOD.BIN";
                        int bok = (q9_syscall(I_CREATE, &bcr) == 0);
                        int pathbad = bok ? (int)bcr.d[0] : -1;
                        if (bok) {
                            q9_regs_t bwr = {0};
                            bwr.d[0] = (uint32_t)pathbad;
                            bwr.d[1] = badlen;
                            bwr.a[0] = (void *)badbuf;
                            bok = bok && (q9_syscall(I_WRITE, &bwr) == 0);
                            bok = bok && (q9_path_close((uint32_t)pathbad) == 0);
                        }
                        q9_regs_t bld = {0};
                        bld.a[0] = (void *)"/d0/BADMOD.BIN";
                        bok = bok && (q9_syscall(F_LOAD, &bld) == E_BMHP);
                        checks[nchecks].name = "F$Load: kaputter Sync -> E$BMHP, kein Directory-Eintrag";
                        checks[nchecks++].ok = bok;

                        q9_regs_t bdel = {0};
                        bdel.a[0] = (void *)"/d0/BADMOD.BIN";
                        q9_syscall(I_DELETE, &bdel);           /* Cluster/Directory-Slot wieder frei  */
                    }

                    /* Aufraeumen: /d0/LOADMOD.BIN wieder loeschen, damit dieser Block denselben
                       FAT16-Zustand (freie Cluster/Directory-Slots) hinterlaesst wie er ihn vorfand —
                       die Python-Nachvalidierung in test/06_test_fat16.py prueft feste Cluster-Nummern
                       fuer NEU.TXT/NEUDIR und wuerde sonst durch die zusaetzlichen Dateien hier
                       verfaelscht. */
                    q9_regs_t delmod = {0};
                    delmod.a[0] = (void *)"/d0/LOADMOD.BIN";
                    q9_syscall(I_DELETE, &delmod);
                }

                {
                    q9_regs_t del = {0};
                    del.a[0] = (void *)"/d0/NEU.TXT";
                    ok = (q9_syscall(I_DELETE, &del) == 0);
                    q9_regs_t r2 = {0};
                    r2.d[0] = Q9_MODE_READ;
                    r2.a[0] = (void *)"/d0/NEU.TXT";
                    ok = ok && (q9_syscall(I_OPEN, &r2) == E_PNNF);   /* wirklich weg */
                    checks[nchecks].name = "FAT16: I$Delete /d0/NEU.TXT, danach E$PNNF";
                    checks[nchecks++].ok = ok;
                }
            }
        }
    }

#ifdef Q9_HAVE_WASM3
    {
        /* 4.6: Grundbaustein wasm3-Runtime (noch OHNE Syscall-Bridge, kommt mit 4.7) — laedt ein
           von Hand gebautes .wasm-Modul (aequivalent zu `(func $add (param i32 i32) (result i32)
           local.get 0 local.get 1 i32.add)`, exportiert als "add") und ruft es mit (2, 3) auf.
           Nur im nativen Build vorhanden (s. Makefile/wasmrt.h) — im wasm-Build ist dieser
           komplette Block auskompiliert. */
        static const uint8_t wasm_add[] = {
            0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00,
            0x01, 0x07, 0x01, 0x60, 0x02, 0x7f, 0x7f, 0x01, 0x7f,
            0x03, 0x02, 0x01, 0x00,
            0x07, 0x07, 0x01, 0x03, 0x61, 0x64, 0x64, 0x00, 0x00,
            0x0a, 0x09, 0x01, 0x07, 0x00, 0x20, 0x00, 0x20, 0x01, 0x6a, 0x0b
        };

        q9_wasmrt_t rt;
        int ok = (q9_wasmrt_init(&rt) == Q9_WASMRT_OK);
        ok = ok && (q9_wasmrt_parse(&rt, wasm_add, sizeof(wasm_add)) == Q9_WASMRT_OK);
        ok = ok && (q9_wasmrt_load(&rt) == Q9_WASMRT_OK);
        int32_t result = 0;
        ok = ok && (q9_wasmrt_call_i32(&rt, "add", 2, 3, &result) == Q9_WASMRT_OK);
        ok = ok && (result == 5);
        q9_wasmrt_free(&rt);

        checks[nchecks].name = "4.6: wasm3 laedt add(a,b)-Modul und rechnet 2+3=5";
        checks[nchecks++].ok = ok;
    }
    {
        /* 4.7: Syscall-Bridge — F$Fork startet ein echtes Q9_MOD_WASM-Modul. Das Gastprogramm
           (per Hand als WASM-Bytecode gebaut, vgl. test/06_test_fat16.py, das FAT16-Images per
           Hand statt per externem Tool baut) importiert q9.f_id/q9.f_time/q9.f_exit, ruft f_id
           und f_time einmal auf (Ergebnis verworfen — beweist nur, dass der Aufruf klappt und
           echte Syscall-Werte zurueckkommen, ohne sie hier weiterzuverwenden) und beendet sich
           dann per f_exit(0). Aequivalent zu folgendem WAT:
             (module
               (import "q9" "f_id"   (func $f_id   (result i32)))
               (import "q9" "f_time" (func $f_time (result i64)))
               (import "q9" "f_exit" (func $f_exit (param i32)))
               (func (export "q9_main")
                 call $f_id   drop
                 call $f_time drop
                 i32.const 0  call $f_exit))
           Beweist den kompletten Weg Laden->Instanziieren->Laufen->Syscall->Zurueck end-to-end
           (das in ARBEITSPLAN.md 4.7 als Ziel genannte Kriterium), inklusive echtem F$Fork/
           F$Wait-Lebenszyklus (wie der 4.2-Test mit Q9_MOD_NATIVE, nur mit Q9_MOD_WASM). */
        static const uint8_t wasm_main[] = {
            0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00,
            0x01, 0x10, 0x04, 0x60, 0x00, 0x01, 0x7f, 0x60, 0x00, 0x01, 0x7e,
            0x60, 0x01, 0x7f, 0x00, 0x60, 0x00, 0x00,
            0x02, 0x23, 0x03,
            0x02, 0x71, 0x39, 0x04, 0x66, 0x5f, 0x69, 0x64, 0x00, 0x00,
            0x02, 0x71, 0x39, 0x06, 0x66, 0x5f, 0x74, 0x69, 0x6d, 0x65, 0x00, 0x01,
            0x02, 0x71, 0x39, 0x06, 0x66, 0x5f, 0x65, 0x78, 0x69, 0x74, 0x00, 0x02,
            0x03, 0x02, 0x01, 0x03,
            0x07, 0x0b, 0x01, 0x07, 0x71, 0x39, 0x5f, 0x6d, 0x61, 0x69, 0x6e, 0x00, 0x03,
            0x0a, 0x0e, 0x01, 0x0c, 0x00, 0x10, 0x00, 0x1a, 0x10, 0x01, 0x1a, 0x41, 0x00, 0x10, 0x02, 0x0b
        };
        static uint8_t wasmbuf[192];
        int            ok;
        q9_regs_t      fk = {0};
        uint32_t       childpid;

        build_wasm_module(wasmbuf, "wasmproc", 1, wasm_main, sizeof(wasm_main));
        ok = (q9_mod_register((const q9_modhdr_t *)wasmbuf) == 0);

        fk.a[0] = (void *)"wasmproc";
        fk.d[1] = Q9_MOD_PRGRM;
        fk.d[2] = Q9_MOD_WASM;
        ok = ok && (q9_syscall(F_FORK, &fk) == 0);
        childpid = fk.d[0];
        ok = ok && (childpid != 0 && childpid != 1);

        {   /* Kind ist ACTIVE, hat aber noch keinen Tick bekommen -> noch kein Exit           */
            q9_regs_t wt = {0};
            ok = ok && (q9_syscall(F_WAIT, &wt) == E_NOTRDY);
        }

        q9_kernel_step();   /* ein Scheduler-Tick: repl_step + q9_wasm_proc_step (laeuft komplett
                                durch bis zum f_exit-Trap, s. wasmproc.c-Kommentar zur 4.7-Vereinfachung) */

        {
            q9_regs_t wt2 = {0};
            ok = ok && (q9_syscall(F_WAIT, &wt2) == 0 && wt2.d[0] == childpid && wt2.d[1] == 0);
        }
        {   /* kein Kind mehr uebrig -> E$NoChld */
            q9_regs_t wt3 = {0};
            ok = ok && (q9_syscall(F_WAIT, &wt3) == E_NOCHLD);
        }

        checks[nchecks].name = "4.7: F$Fork startet Q9_MOD_WASM-Modul, Syscall-Bridge, F$Exit(0), F$Wait sammelt ein";
        checks[nchecks++].ok = ok;
    }
    {
        /* 4.8: Pointer-Marshaling — ein WASM-Gastprogramm mit eigenem linearen Speicher (1 Page)
           oeffnet "/nil" (Pfadname liegt im Gastspeicher, wird per Offset->Host-Zeiger uebersetzt),
           schreibt 2 Bytes hinein (/nil verwirft, meldet aber die volle Anzahl zurueck), liest
           danach (nil liefert immer E$EOF = $D3, negiert also -211) und schliesst den Pfad wieder.
           Bricht bei jedem unerwarteten Ergebnis selbst mit einem eigenen Exit-Code (1..4) ab.
           Aequivalent zu folgendem WAT (mit wat2wasm aus wabt gebaut — Werkzeug seit 4.8 auf
           diesem Mac Mini per Homebrew installiert, s. docs/HANDBUCH.md Abschnitt 2):
             (module
               (import "q9" "i_open"  (func $i_open  (param i32 i32) (result i32)))
               (import "q9" "i_write" (func $i_write (param i32 i32 i32) (result i32)))
               (import "q9" "i_read"  (func $i_read  (param i32 i32 i32) (result i32)))
               (import "q9" "i_close" (func $i_close (param i32) (result i32)))
               (import "q9" "f_exit"  (func $f_exit  (param i32)))
               (memory (export "memory") 1)
               (data (i32.const 8)  "/nil\00")           ;; Offset 0..7 bewusst frei: wasm3s
               (data (i32.const 32) "AB")                ;;   m3ApiIsNullPtr() behandelt Offset 0
               (func (export "q9_main")                  ;;   als Nullzeiger (Konvention wie bei
                 (local $path i32) (local $n i32) (local $code i32)      ;; realen Linkern)
                 (local.set $path (call $i_open (i32.const 8) (i32.const 3)))
                 (if (i32.lt_s (local.get $path) (i32.const 0))
                   (then (call $f_exit (i32.const 1)) unreachable))
                 (local.set $n (call $i_write (local.get $path) (i32.const 32) (i32.const 2)))
                 (if (i32.ne (local.get $n) (i32.const 2))
                   (then (call $f_exit (i32.const 2)) unreachable))
                 (local.set $code (call $i_read (local.get $path) (i32.const 64) (i32.const 4)))
                 (if (i32.ne (local.get $code) (i32.const -211))
                   (then (call $f_exit (i32.const 3)) unreachable))
                 (local.set $code (call $i_close (local.get $path)))
                 (if (i32.ne (local.get $code) (i32.const 0))
                   (then (call $f_exit (i32.const 4)) unreachable))
                 (call $f_exit (i32.const 0))))
           Beweist den kompletten Weg Gast-Offset -> Bounds-Check -> Host-Zeiger -> echter Syscall
           -> Ergebnis zurueck fuer I$Open/I$Write/I$Read/I$Close. */
        static const uint8_t wasm_ptr[] = {
            0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x01, 0x1a, 0x05, 0x60,
            0x02, 0x7f, 0x7f, 0x01, 0x7f, 0x60, 0x03, 0x7f, 0x7f, 0x7f, 0x01, 0x7f,
            0x60, 0x01, 0x7f, 0x01, 0x7f, 0x60, 0x01, 0x7f, 0x00, 0x60, 0x00, 0x00,
            0x02, 0x3f, 0x05, 0x02, 0x71, 0x39, 0x06, 0x69, 0x5f, 0x6f, 0x70, 0x65,
            0x6e, 0x00, 0x00, 0x02, 0x71, 0x39, 0x07, 0x69, 0x5f, 0x77, 0x72, 0x69,
            0x74, 0x65, 0x00, 0x01, 0x02, 0x71, 0x39, 0x06, 0x69, 0x5f, 0x72, 0x65,
            0x61, 0x64, 0x00, 0x01, 0x02, 0x71, 0x39, 0x07, 0x69, 0x5f, 0x63, 0x6c,
            0x6f, 0x73, 0x65, 0x00, 0x02, 0x02, 0x71, 0x39, 0x06, 0x66, 0x5f, 0x65,
            0x78, 0x69, 0x74, 0x00, 0x03, 0x03, 0x02, 0x01, 0x04, 0x05, 0x03, 0x01,
            0x00, 0x01, 0x07, 0x14, 0x02, 0x06, 0x6d, 0x65, 0x6d, 0x6f, 0x72, 0x79,
            0x02, 0x00, 0x07, 0x71, 0x39, 0x5f, 0x6d, 0x61, 0x69, 0x6e, 0x00, 0x05,
            0x0a, 0x62, 0x01, 0x60, 0x01, 0x03, 0x7f, 0x41, 0x08, 0x41, 0x03, 0x10,
            0x00, 0x21, 0x00, 0x20, 0x00, 0x41, 0x00, 0x48, 0x04, 0x40, 0x41, 0x01,
            0x10, 0x04, 0x00, 0x0b, 0x20, 0x00, 0x41, 0x20, 0x41, 0x02, 0x10, 0x01,
            0x21, 0x01, 0x20, 0x01, 0x41, 0x02, 0x47, 0x04, 0x40, 0x41, 0x02, 0x10,
            0x04, 0x00, 0x0b, 0x20, 0x00, 0x41, 0xc0, 0x00, 0x41, 0x04, 0x10, 0x02,
            0x21, 0x02, 0x20, 0x02, 0x41, 0xad, 0x7e, 0x47, 0x04, 0x40, 0x41, 0x03,
            0x10, 0x04, 0x00, 0x0b, 0x20, 0x00, 0x10, 0x03, 0x21, 0x02, 0x20, 0x02,
            0x41, 0x00, 0x47, 0x04, 0x40, 0x41, 0x04, 0x10, 0x04, 0x00, 0x0b, 0x41,
            0x00, 0x10, 0x04, 0x0b, 0x0b, 0x12, 0x02, 0x00, 0x41, 0x08, 0x0b, 0x05,
            0x2f, 0x6e, 0x69, 0x6c, 0x00, 0x00, 0x41, 0x20, 0x0b, 0x02, 0x41, 0x42
        };
        static uint8_t wasmbuf2[384];
        int            ok;
        q9_regs_t      fk = {0};
        uint32_t       childpid;

        build_wasm_module(wasmbuf2, "wasmptr", 1, wasm_ptr, sizeof(wasm_ptr));
        ok = (q9_mod_register((const q9_modhdr_t *)wasmbuf2) == 0);

        fk.a[0] = (void *)"wasmptr";
        fk.d[1] = Q9_MOD_PRGRM;
        fk.d[2] = Q9_MOD_WASM;
        ok = ok && (q9_syscall(F_FORK, &fk) == 0);
        childpid = fk.d[0];
        ok = ok && (childpid != 0 && childpid != 1);

        q9_kernel_step();

        {
            q9_regs_t wt = {0};
            ok = ok && (q9_syscall(F_WAIT, &wt) == 0 && wt.d[0] == childpid && wt.d[1] == 0);
        }

        checks[nchecks].name = "4.8: Pointer-Marshaling — Q9_MOD_WASM oeffnet/schreibt/liest/schliesst /nil ueber Gast-Offsets";
        checks[nchecks++].ok = ok;
    }
#endif

    for (int i = 0; i < nchecks; i++) {
        kputs(checks[i].ok ? "  [ok] " : "  [FEHLER] ");
        kputs(checks[i].name);
        kputs("\n");
        if (!checks[i].ok) {
            fails++;
        }
    }
    kputs(fails == 0 ? "SYSCALL TEST PASS\n" : "SYSCALL TEST FAIL\n");
    return fails;
}

//────────────────────────────────────────────────────────────────────────────────────────────────
// EOF kernel.c                                                                            Ver. 3.60
//────────────────────────────────────────────────────────────────────────────────────────────────
