//════════════════════════════════════════════════════════════════════════════════════════════════
// File:   wasmproc.h                                                                      Ver. 1.10
// Owner:  AF
// Desc.:  Q9_MOD_WASM-Prozessausfuehrung (Schritt 4.7, ARBEITSPLAN.md): Syscall-Bridge (Import-
//         Tabelle Richtung Q9-Kernel) + Step-Trampolin, das F$Fork/F$Chain (syscall.c) fuer
//         Module mit Language-Byte Q9_MOD_WASM als Step-Funktion eintragen. Native-Build-only,
//         baut auf der wasm3-Runtime aus wasmrt.h/4.6 auf.
//
//         4.8: Pointer-Marshaling — q9.i_open/i_close/i_read/i_write nehmen KEINE Host-Zeiger
//         entgegen, sondern Offsets in die lineare Speicherinstanz des Gastmoduls (m3ApiGetArgMem
//         uebersetzt via _mem-Basiszeiger, wasm_range_ok bounds-checkt gegen die tatsaechliche
//         Speichergroesse). Rueckgabekonvention aller vier: >= 0 Erfolgswert, < 0 negierter Q9-
//         Fehlercode (kein Carry-Bit wie im realen 68k-ABI). Ein aus dem Bounds-Check fallender
//         Zugriff (Puffer/Pfadname reicht ueber die eigene Speicherinstanz hinaus) ist ein Trap
//         (bricht die WASM-Ausfuehrung ab), keine Fehlerantwort — das ist ein Programmierfehler
//         im Gast, kein regulaerer I/O-Fehlerfall.
//
//         Bewusste Vereinfachung fuer 4.7 ("Erstmal NUR Syscalls ohne Zeiger-Parameter"): das
//         komplette Gastprogramm laeuft beim ERSTEN Scheduler-Tick synchron bis zum Ende durch
//         (kein kooperatives Unterbrechen mitten in der WASM-Ausfuehrung — das wuerde Asyncify
//         oder aehnliches brauchen, weit ausserhalb des 4.7-Rahmens). Der Exportname der
//         Einsprungfunktion ist fest "q9_main" mit Signatur () -> () (keine Argumente, kein
//         Rueckgabewert) — kehrt sie normal zurueck, gilt das als F$Exit(0); ruft sie eine der
//         importierten Q9-Funktionen falsch auf oder bricht wasm3 mit einem echten Fehler ab,
//         gilt das als F$Exit(-1). F$Exit selbst (importierte Funktion "q9.f_exit") beendet den
//         Prozess sofort per echtem q9_syscall(F_EXIT) und bricht die WASM-Ausfuehrung per Trap
//         ab (kein Rueckkehrpfad in den Gastcode, wie bei echtem OS-9 F$Exit).
//
// Call:   Wird NICHT direkt aufgerufen — syscall.c traegt q9_wasm_proc_step als Step-Funktion in
//         die Prozesstabelle ein (proc.c), wenn F$Fork/F$Chain ein Q9_MOD_WASM-Modul verlinken.
//
// Edition History
//─────────┬──────┬────────────────────────────────────────────────────────────────────────┬──────
// Date    │ Ver. │ Description                                                            │ By
//─────────┼──────┼────────────────────────────────────────────────────────────────────────┼──────
// 26-07-04│ 1.00 │ 4.7: Erste Syscall-Bridge (F$ID, F$Time, F$Exit)                        │ CF
// 26-07-04│ 1.10 │ 4.8: Pointer-Marshaling — q9.i_open/i_close/i_read/i_write             │ CF
//═════════╧══════╧════════════════════════════════════════════════════════════════════════╧══════
#ifndef Q9_WASMPROC_H
#define Q9_WASMPROC_H

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_wasm_proc_step
// Desc.:    Step-Funktion (q9_proc_step_fn) fuer Prozesse, deren Modul Q9_MOD_WASM ist. Liest das
//           Modul des aktuell gestepten Prozesses (q9_proc_current()->module), fuehrt dessen
//           WASM-Bytecode ueber die 4.6-Runtime aus (mit gelinkten Q9-Syscall-Importen), und
//           beendet den Prozess per q9_proc_exit — entweder ueber F$Exit aus dem Gastprogramm
//           selbst, oder implizit nach normaler Rueckkehr aus "q9_main"/nach einem echten Fehler.
// Call:     wird nur als Funktionszeiger uebergeben, s.o.
//════════════════════════════════════════════════════════════════════════════════════════════════
void q9_wasm_proc_step(void);

#endif // Q9_WASMPROC_H

//────────────────────────────────────────────────────────────────────────────────────────────────
// EOF wasmproc.h                                                                          Ver. 1.10
//────────────────────────────────────────────────────────────────────────────────────────────────
