//════════════════════════════════════════════════════════════════════════════════════════════════
// File:   wasmproc.c                                                                      Ver. 1.10
// Owner:  AF
// Desc.:  Implementierung der Q9_MOD_WASM-Syscall-Bridge, siehe wasmproc.h.
//
// Edition History
//─────────┬──────┬────────────────────────────────────────────────────────────────────────┬──────
// Date    │ Ver. │ Description                                                            │ By
//─────────┼──────┼────────────────────────────────────────────────────────────────────────┼──────
// 26-07-04│ 1.00 │ 4.7: Erste Syscall-Bridge (F$ID, F$Time, F$Exit)                        │ CF
// 26-07-04│ 1.10 │ 4.8: Pointer-Marshaling — q9.i_open/i_close/i_read/i_write. Guest-a0    │ CF
//         │      │ ist ein Offset in die eigene lineare Speicherinstanz, kein Host-Zeiger;  │
//         │      │ wasm_off_to_ptr() bounds-checkt und uebersetzt vor jedem q9_syscall()     │
//═════════╧══════╧════════════════════════════════════════════════════════════════════════╧══════
#include "wasmproc.h"
#include "wasmrt.h"
#include "wasm3.h"
#include "proc.h"
#include "syscall.h"
#include "module.h"

/* Maximale Laenge eines Pfadnamens, den die Bridge aus dem Gastspeicher liest (inkl. NUL). Kein
   Q9-weiter Wert (Q9_CWD_MAXLEN in vfs.h betrifft nur das Arbeitsverzeichnis) — rein eine Grenze,
   damit die Suche nach dem abschliessenden NUL nicht unbegrenzt ueber den Gastspeicher laeuft. */
#define Q9_WASM_PATH_MAX 128

//────────────────────────────────────────────────────────────────────────────────────────────────
// Function: wasm_range_ok
// Desc.:    Prueft, ob [addr, addr+len) vollstaendig innerhalb der linearen Speicherinstanz des
//           Gastmoduls liegt (gleiche Grenzformel wie das wasm3-Makro m3ApiCheckMem, s. wasm3.h) —
//           als eigenstaendige Funktion statt Makro, weil m3ApiCheckMem per "return" trapt und
//           damit nur direkt im Rumpf einer m3ApiRawFunction verwendbar waere, nicht in einem
//           gemeinsamen Helfer fuer mehrere Imports.
// Call:     if (!wasm_range_ok(mem, runtime, ptr, len)) m3ApiTrap(m3Err_trapOutOfBoundsMemoryAccess);
//────────────────────────────────────────────────────────────────────────────────────────────────
static int wasm_range_ok(void *mem, IM3Runtime runtime, const void *addr, uint32_t len)
{
    uint64_t end     = (uint64_t)(uintptr_t)addr + len;
    uint64_t memend  = (uint64_t)(uintptr_t)mem + m3_GetMemorySize(runtime);
    return addr >= mem && end <= memend;
}

//────────────────────────────────────────────────────────────────────────────────────────────────
// Function: wasm_str_len
// Desc.:    Sucht das abschliessende NUL einer aus dem Gastspeicher kommenden Pfadnamen-Zeichen-
//           kette, Byte fuer Byte bounds-geprueft (kein Vorab-Check auf volle Q9_WASM_PATH_MAX-
//           Laenge, damit ein kurzer String nahe am Ende der Speicherinstanz nicht faelschlich als
//           out-of-bounds gilt). Liefert die Laenge (ohne NUL) bei Erfolg, -1 bei echtem Out-of-
//           Bounds-Zugriff, -2 wenn kein NUL innerhalb von Q9_WASM_PATH_MAX Bytes gefunden wurde.
// Call:     len = wasm_str_len(mem, runtime, ptr)
//────────────────────────────────────────────────────────────────────────────────────────────────
static int32_t wasm_str_len(void *mem, IM3Runtime runtime, const char *ptr)
{
    uint32_t i;
    for (i = 0; i < Q9_WASM_PATH_MAX; i++) {
        if (!wasm_range_ok(mem, runtime, ptr + i, 1)) {
            return -1;
        }
        if (ptr[i] == '\0') {
            return (int32_t)i;
        }
    }
    return -2;
}

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
// Import: q9.i_open — Signatur "i(ii)" (i32 Pfadnamen-Offset, i32 Modus -> i32 Ergebnis). Uebersetzt
// den Gast-Offset in einen echten Host-Zeiger (Pfadname muss NUL-terminiert innerhalb der eigenen
// Speicherinstanz liegen, sonst Trap bzw. E$BPNam) und ruft I$Open auf. Rueckgabekonvention fuer
// alle vier neuen Imports (analog zu POSIX-Syscall-Bridges): >= 0 ist der Erfolgswert (hier:
// Pfadnummer), < 0 ist ein negierter Q9-Fehlercode (E$BPADDR etc.) — WASM kennt kein "Carry-Bit"
// wie das reale 68k-ABI (docs/SYSCALLS.md), diese eine Bridge-Konvention ersetzt es fuer Gastcode.
//────────────────────────────────────────────────────────────────────────────────────────────────
static const void *import_i_open(IM3Runtime runtime, IM3ImportContext _ctx, uint64_t *_sp, void *_mem)
{
    (void)_ctx;
    m3ApiReturnType(int32_t)
    m3ApiGetArgMem(const char *, path)
    m3ApiGetArg(int32_t, mode)

    if (m3ApiIsNullPtr(path)) {
        m3ApiReturn(-(int32_t)E_BPADDR);
    }
    int32_t len = wasm_str_len(_mem, runtime, path);
    if (len == -1) {
        m3ApiTrap(m3Err_trapOutOfBoundsMemoryAccess);
    }
    if (len == -2) {
        m3ApiReturn(-(int32_t)E_BPNAM);
    }

    q9_regs_t r = {0};
    r.d[0] = (uint32_t)(uint8_t)mode;
    r.a[0] = (void *)path;
    int err = q9_syscall(I_OPEN, &r);
    m3ApiReturn(err == 0 ? (int32_t)r.d[0] : -(int32_t)err);
}

//────────────────────────────────────────────────────────────────────────────────────────────────
// Import: q9.i_close — Signatur "i(i)" (i32 Pfadnummer -> i32 Ergebnis, s.o. Rueckgabekonvention).
//────────────────────────────────────────────────────────────────────────────────────────────────
static const void *import_i_close(IM3Runtime runtime, IM3ImportContext _ctx, uint64_t *_sp, void *_mem)
{
    (void)runtime; (void)_ctx; (void)_mem;
    m3ApiReturnType(int32_t)
    m3ApiGetArg(int32_t, path)

    q9_regs_t r = {0};
    r.d[0] = (uint32_t)path;
    int err = q9_syscall(I_CLOSE, &r);
    m3ApiReturn(-(int32_t)err);                        /* err == 0 -> 0, sonst negierter Fehlercode */
}

//────────────────────────────────────────────────────────────────────────────────────────────────
// Import: q9.i_read — Signatur "i(iii)" (i32 Pfadnummer, i32 Puffer-Offset, i32 max -> i32
// Ergebnis): >= 0 = gelesene Bytes, < 0 = negierter Fehlercode (u.a. E$NotRdy — der aufrufende
// Prozess wurde von sc_read bereits per q9_proc_wait_device in WAITING versetzt, s. syscall.c;
// das Gastprogramm sieht denselben Poll-Fehler wie ein nativer Prozess, kein Sonderfall noetig).
// Der Puffer muss vollstaendig (max Bytes) in der Speicherinstanz liegen, sonst Trap — anders als
// bei Pfadnamen gibt es hier keine "logische" Fehlerantwort, ein zu kleiner Puffer ist ein
// Programmierfehler im Gast, kein regulaerer I/O-Fehlerfall.
//────────────────────────────────────────────────────────────────────────────────────────────────
static const void *import_i_read(IM3Runtime runtime, IM3ImportContext _ctx, uint64_t *_sp, void *_mem)
{
    (void)_ctx;
    m3ApiReturnType(int32_t)
    m3ApiGetArg(int32_t, path)
    m3ApiGetArgMem(uint8_t *, buf)
    m3ApiGetArg(uint32_t, max)

    if (!wasm_range_ok(_mem, runtime, buf, max)) {
        m3ApiTrap(m3Err_trapOutOfBoundsMemoryAccess);
    }

    q9_regs_t r = {0};
    r.d[0] = (uint32_t)path;
    r.d[1] = max;
    r.a[0] = buf;
    int err = q9_syscall(I_READ, &r);
    m3ApiReturn(err == 0 ? (int32_t)r.d[1] : -(int32_t)err);
}

//────────────────────────────────────────────────────────────────────────────────────────────────
// Import: q9.i_write — Signatur "i(iii)" (i32 Pfadnummer, i32 Puffer-Offset, i32 Anzahl -> i32
// Ergebnis): >= 0 = geschriebene Bytes, < 0 = negierter Fehlercode. Bounds-Check wie i_read.
//────────────────────────────────────────────────────────────────────────────────────────────────
static const void *import_i_write(IM3Runtime runtime, IM3ImportContext _ctx, uint64_t *_sp, void *_mem)
{
    (void)_ctx;
    m3ApiReturnType(int32_t)
    m3ApiGetArg(int32_t, path)
    m3ApiGetArgMem(const uint8_t *, buf)
    m3ApiGetArg(uint32_t, count)

    if (!wasm_range_ok(_mem, runtime, buf, count)) {
        m3ApiTrap(m3Err_trapOutOfBoundsMemoryAccess);
    }

    q9_regs_t r = {0};
    r.d[0] = (uint32_t)path;
    r.d[1] = count;
    r.a[0] = (void *)buf;
    int err = q9_syscall(I_WRITE, &r);
    m3ApiReturn(err == 0 ? (int32_t)r.d[1] : -(int32_t)err);
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
    m3_LinkRawFunction(module, "q9", "f_id",    "i()",    import_f_id);
    m3_LinkRawFunction(module, "q9", "f_time",  "I()",    import_f_time);
    m3_LinkRawFunction(module, "q9", "f_exit",  "v(i)",   import_f_exit);
    m3_LinkRawFunction(module, "q9", "i_open",  "i(ii)",  import_i_open);
    m3_LinkRawFunction(module, "q9", "i_close", "i(i)",   import_i_close);
    m3_LinkRawFunction(module, "q9", "i_read",  "i(iii)", import_i_read);
    m3_LinkRawFunction(module, "q9", "i_write", "i(iii)", import_i_write);
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
// EOF wasmproc.c                                                                          Ver. 1.10
//────────────────────────────────────────────────────────────────────────────────────────────────
