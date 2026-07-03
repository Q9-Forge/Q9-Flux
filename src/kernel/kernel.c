//════════════════════════════════════════════════════════════════════════════════════════════════
// File:   kernel.c                                                                        Ver. 1.20
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
//═════════╧══════╧════════════════════════════════════════════════════════════════════════╧══════

#include "../hal/q9_hal.h"
#include "device.h"
#include "syscall.h"
#include "kernel.h"

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

    kputs("\n");
    kputs("  ═══════════════════════════════════════\n");
    kputs("   Q9 v" Q9_VERSION " alpha\n");
    kputs("   modulares Mini-OS in OS-9-Tradition\n");
    kputs("  ═══════════════════════════════════════\n");
    kputs("  target: ");
    kputs(q9_hal_target());
    kputs("\n  syscalls: OS-9-ABI aktiv (docs/SYSCALLS.md)\n");
    kputs("  devices:  /term (Pfade 0/1/2)\n\n");
    kputs("  Phase 1: REPL. Eingabe wird zurückgegeben, 'exit' beendet.\n\n");
    kputs("Q9> ");
}

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_kernel_step
// Desc.:    Ein Kernel-Tick: I$ReadLn pollen; komplette Zeile -> Antwort + neuer Prompt.
//           "exit" ruft F$Exit (Proto-Prozess hält an).
// Call:     q9_kernel_step()
//════════════════════════════════════════════════════════════════════════════════════════════════
void q9_kernel_step(void)
{
    q9_regs_t r = {0};
    uint8_t   line[80];
    uint32_t  n;

    if (q9_proc_halted()) {
        return;
    }

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
    } checks[10];
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
// EOF kernel.c                                                                            Ver. 1.20
//────────────────────────────────────────────────────────────────────────────────────────────────
