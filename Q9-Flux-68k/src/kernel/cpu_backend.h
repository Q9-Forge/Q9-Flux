//════════════════════════════════════════════════════════════════════════════════════════════════
// File:   cpu_backend.h                                                                   Ver. 1.00
// Owner:  Claudia
// Desc.:  6.5: Kleine Vtable fuer die emulierte CPU (reset/execute/set_irq/is_stopped/ctx), analog
//         zu devreg.h's Geraete-Vtable. q9boardrun.c ruft ab jetzt nur noch ueber diese Vtable,
//         statt Musashi/m68krt-Funktionen direkt beim Namen zu kennen -- reines Refactoring, keine
//         Verhaltensaenderung, solange m68krt.c die Vtable nur befuellt (s. m68krt.h,
//         q9_m68krt_get_backend).
//
//         Bewusst KLEIN gehalten: nur die vier Operationen, die die Hauptschleife tatsaechlich
//         braucht. Diagnosefunktionen wie q9_m68krt_debug_state/quicc_acks bleiben ausserhalb --
//         die sind 68k-spezifische Fehlersuche (SR-Register, QUICC-Zaehler), keine allgemeine
//         CPU-Eigenschaft, und muessten fuer eine zweite Architektur ohnehin anders aussehen.
//
//         Wie bei m68krt.c/Musashi ist die zugrunde liegende CPU-Emulation (Musashi, spaeter evtl.
//         der vendorte TinyEMU-RISC-V-Kern, s. third_party/tinyemu/Q9_VENDOR.md) meist ein
//         Singleton mit eigenen Globals -- 'ctx' existiert trotzdem in der Vtable, damit ein
//         Backend, das KEIN Singleton ist (mehrere Instanzen gleichzeitig), ohne Vtable-Aenderung
//         moeglich bleibt.
//
// Call:   q9_cpu_backend_t cpu;
//         q9_m68krt_get_backend(&rt, &cpu);
//         cpu.reset(cpu.ctx);
//         spent = cpu.execute(cpu.ctx, cycles);
//         cpu.set_irq(cpu.ctx, level);
//         if (cpu.is_stopped(cpu.ctx)) ...
//
// Edition History
//─────────┬──────┬────────────────────────────────────────────────────────────────────────┬──────
// Date    │ Ver. │ Description                                                            │ By
//─────────┼──────┼────────────────────────────────────────────────────────────────────────┼──────
// 26-08-13│ 1.00 │ 6.5: Erster Wurf -- Vtable-Typ, noch ohne Fuellung (die kommt in         │ Cld
//         │      │ m68krt.h/.c)                                                            │
//═════════╧══════╧════════════════════════════════════════════════════════════════════════╧══════
#ifndef Q9_CPU_BACKEND_H
#define Q9_CPU_BACKEND_H

typedef void (*q9_cpu_reset_fn)     (void *ctx);
typedef int  (*q9_cpu_execute_fn)   (void *ctx, int cycles);   /* liefert tatsaechlich verbrauchte
                                                                    Takte zurueck                  */
typedef void (*q9_cpu_set_irq_fn)   (void *ctx, int level);    /* level = 0 loescht die Anforderung */
typedef int  (*q9_cpu_is_stopped_fn)(void *ctx);                /* 1 = CPU haelt an (z.B. STOP)     */

typedef struct {
    q9_cpu_reset_fn      reset;       /* Pflicht */
    q9_cpu_execute_fn    execute;     /* Pflicht */
    q9_cpu_set_irq_fn    set_irq;     /* Pflicht */
    q9_cpu_is_stopped_fn is_stopped;  /* Pflicht */
    void                 *ctx;        /* an jede der vier Funktionen durchgereicht */
} q9_cpu_backend_t;

#endif // Q9_CPU_BACKEND_H

//────────────────────────────────────────────────────────────────────────────────────────────────
// EOF cpu_backend.h                                                                       Ver. 1.00
//────────────────────────────────────────────────────────────────────────────────────────────────
