//════════════════════════════════════════════════════════════════════════════════════════════════
// File:   wasmproc.c                                                                      Ver. 1.00
// Owner:  AF
// Desc.:  Implementierung der Q9_MOD_WASM-Syscall-Bridge, siehe wasmproc.h.
//
// Edition History
//─────────┬──────┬────────────────────────────────────────────────────────────────────────┬──────
// Date    │ Ver. │ Description                                                            │ By
//─────────┼──────┼────────────────────────────────────────────────────────────────────────┼──────
// 26-07-04│ 1.00 │ 4.7: Erste Syscall-Bridge (F$ID, F$Time, F$Exit)                        │ CF
//═════════╧══════╧════════════════════════════════════════════════════════════════════════╧══════
#include "wasmproc.h"
#include "wasmrt.h"
#include "wasm3.h"
#include "proc.h"
#include "syscall.h"
#include "module.h"

/* Rueckgabewert von import_f_exit()/m3_CallV: Zeiger-Identitaet (nicht Stringinhalt!) markiert
   den gewollten F$Exit-Trap, damit q9_wasm_proc_step() ihn von einem echten Laufzeitfehler
   unterscheiden kann (beide sind M3Result, also const char*). */
static const char *const g_exit_trap = "Q9: F$Exit aus WASM-Gastprogramm";

//────────────────────────────────────────────────────────────────────────────────────────────────
// Import: q9.f_id — Signatur "i()" (kein Argument, i32-Rueckgabe). Ruft den echten F$ID-Syscall
// im Kontext des aktuell gestepten Prozesses auf (q9_proc_current(), von aussen durch
// q9_proc_schedule() gesetzt, s. proc.c) — keine Sonderbehandlung noetig, der Syscall-Dispatcher
// weiss bereits, wer "der aktuelle Prozess" ist.
//────────────────────────────────────────────────────────────────────────────────────────────────
static const void *import_f_id(IM3Runtime runtime, IM3ImportContext _ctx, uint64_t *_sp, void *_mem)
{
    (void)runtime; (void)_ctx; (void)_mem;
    m3ApiReturnType(int32_t)

    q9_regs_t r = {0};
    q9_syscall(F_ID, &r);
    m3ApiReturn((int32_t)r.d[0]);
}

//────────────────────────────────────────────────────────────────────────────────────────────────
// Import: q9.f_time — Signatur "I()" (kein Argument, i64-Rueckgabe). Packt d0 (Zeit) und d1
// (Datum) aus dem echten F$Time-Syscall in ein i64 (d0 in den oberen 32 Bit) — WASM kennt kein
// Mehrfachrueckgabe-MVP, i64 ist der einfachste Weg, ohne gleich Zeiger/Speicher-Marshaling (4.8)
// vorzuziehen. d2 (Wochentag)/d3 (ms-Ticks) bleiben fuer 4.7 aussen vor.
//────────────────────────────────────────────────────────────────────────────────────────────────
static const void *import_f_time(IM3Runtime runtime, IM3ImportContext _ctx, uint64_t *_sp, void *_mem)
{
    (void)runtime; (void)_ctx; (void)_mem;
    m3ApiReturnType(int64_t)

    q9_regs_t r = {0};
    q9_syscall(F_TIME, &r);
    m3ApiReturn(((int64_t)(uint32_t)r.d[0] << 32) | (uint32_t)r.d[1]);
}

//────────────────────────────────────────────────────────────────────────────────────────────────
// Import: q9.f_exit — Signatur "v(i)" (i32-Argument Exit-Code, keine Rueckgabe). Ruft den echten
// F$Exit-Syscall auf (beendet den Prozess bereits an dieser Stelle vollstaendig — Zombie oder
// sofort freier Slot, je nach Parent, wie bei jedem anderen F$Exit) und bricht danach die
// WASM-Ausfuehrung per Trap ab: F$Exit kehrt bei echtem OS-9 nie zum Aufrufer zurueck, das
// WASM-Analogon dazu ist ein Trap statt eines normalen Returns.
//────────────────────────────────────────────────────────────────────────────────────────────────
static const void *import_f_exit(IM3Runtime runtime, IM3ImportContext _ctx, uint64_t *_sp, void *_mem)
{
    (void)runtime; (void)_ctx; (void)_mem;
    m3ApiGetArg(int32_t, code)

    q9_regs_t r = {0};
    r.d[1] = (uint32_t)code;
    q9_syscall(F_EXIT, &r);
    m3ApiTrap(g_exit_trap);
}

//────────────────────────────────────────────────────────────────────────────────────────────────
// Function: link_syscall_bridge
// Desc.:    Linkt alle drei Q9-Importe an das (bereits geladene, s. wasmrt.h) Modul. Ein Gast, der
//           nur einen Teil davon importiert, ist ausdruecklich in Ordnung — m3_LinkRawFunction
//           liefert dann fuer die uebrigen einen "nicht gefunden"-Fehler, den wir bewusst
//           ignorieren (kein Abbruch), weil das Modul die ungenutzten Importe schlicht nicht hat.
//────────────────────────────────────────────────────────────────────────────────────────────────
static void link_syscall_bridge(IM3Module module)
{
    m3_LinkRawFunction(module, "q9", "f_id",   "i()",  import_f_id);
    m3_LinkRawFunction(module, "q9", "f_time", "I()",  import_f_time);
    m3_LinkRawFunction(module, "q9", "f_exit", "v(i)", import_f_exit);
}

void q9_wasm_proc_step(void)
{
    q9_pd_t *me = q9_proc_current();
    if (!me || !me->module) {
        q9_proc_exit(me ? me->pid : 1, -1);           /* sollte nie passieren, s. syscall.c F_FORK */
        return;
    }

    const q9_modhdr_t *hdr = me->module;
    const uint8_t *code = (const uint8_t *)hdr + hdr->execoff;   /* Konvention wie Q9_MOD_NATIVE, */
    uint32_t codelen = hdr->datasize;                             /* siehe build_native_module()   */

    q9_wasmrt_t rt;
    int ok = (q9_wasmrt_init(&rt) == Q9_WASMRT_OK);
    ok = ok && (q9_wasmrt_parse(&rt, code, codelen) == Q9_WASMRT_OK);
    ok = ok && (q9_wasmrt_load(&rt) == Q9_WASMRT_OK);

    if (ok) {
        link_syscall_bridge((IM3Module)rt.module);

        const char *result = q9_wasmrt_call_raw(&rt, "q9_main");
        if (result == NULL) {
            q9_proc_exit(me->pid, 0);                  /* q9_main kehrte normal zurueck            */
        } else if (result != g_exit_trap) {
            q9_proc_exit(me->pid, -1);                  /* echter Trap/Fehler im Gastprogramm       */
        }
        /* result == g_exit_trap: import_f_exit hat den Prozess bereits sauber per F$Exit beendet  */
    } else {
        q9_proc_exit(me->pid, -1);                      /* Parsen/Laden des Moduls fehlgeschlagen    */
    }

    q9_wasmrt_free(&rt);
}

//────────────────────────────────────────────────────────────────────────────────────────────────
// EOF wasmproc.c                                                                          Ver. 1.00
//────────────────────────────────────────────────────────────────────────────────────────────────
