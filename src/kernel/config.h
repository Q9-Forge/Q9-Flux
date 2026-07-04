//════════════════════════════════════════════════════════════════════════════════════════════════
// File:   config.h                                                                        Ver. 1.00
// Owner:  AF
// Desc.:  Erster Baustein einer Q9-Systemkonfiguration (Schritt 4.9, ARBEITSPLAN.md): Werte, die
//         das simulierte/emulierte Zielsystem als Ganzes betreffen — nicht Kernel-interne Limits
//         wie Q9_MOD_MAXDIR (module.h) oder Q9_CWD_MAXLEN (vfs.h), sondern Eigenschaften der
//         "Maschine", auf der Q9 laeuft. Bewusst nur EIN Wert bisher (Gesamtspeichergroesse fuer
//         die eingebettete wasm3-Runtime, s.u.) — weitere Werte (z.B. CPU-Takt) kommen erst, wenn
//         sie tatsaechlich gebraucht werden, kein Vorratsbau.
//
// Call:   wird nur vom Makefile eingebunden (-include, NUR fuer die WASM3_CFLAGS, s. Makefile),
//         damit third_party/wasm3/m3_config.h das d_m3FixedHeap-Define darauf abbilden kann, ohne
//         den vendorten Fremdcode selbst zu aendern.
//
// Edition History
//─────────┬──────┬────────────────────────────────────────────────────────────────────────┬──────
// Date    │ Ver. │ Description                                                            │ By
//─────────┼──────┼────────────────────────────────────────────────────────────────────────┼──────
// 26-07-04│ 1.00 │ 4.9: Q9_SYSTEM_MEM_BYTES (Fixed-Heap-Groesse fuer wasm3)                │ CF
//═════════╧══════╧════════════════════════════════════════════════════════════════════════╧══════
#ifndef Q9_CONFIG_H
#define Q9_CONFIG_H

/* Gesamtspeicher, den sich die eingebettete wasm3-Runtime beim Start selbst als statischen Block
   holt und darin komplett selbst verwaltet (third_party/wasm3/m3_config.h: d_m3FixedHeap, per
   Compiler-Define aus GENAU diesem Wert abgeleitet, s. Makefile WASM3_CFLAGS) — statt Host-malloc
   durchzureichen (Andreas' Einwand 2026-07-04 abends). Wichtig: wasm3s Fixed-Heap ist ein reiner
   Bump-Allocator (nur der jeweils zuletzt allozierte Block laesst sich wirklich freigeben, s.
   m3_core.c m3_Free_Impl) — er waechst effektiv ueber die Lebensdauer des GESAMTEN Prozesses, nicht
   nur pro wasm3-Runtime-Instanz, weil unabhaengig voneinander erzeugte/freigegebene Runtimes
   selten strikt LIFO ineinander verschachtelt sind. 128 KiB reichten fuer einen einzelnen
   Selbsttest, liefen aber ueber, sobald mehrere unabhaengige Runtimes im selben Prozesslauf
   hintereinander entstehen (Selbsttests 4.6/4.7/4.8) — 256 KiB genuegen dafuer mit Reserve, ohne
   schon eine echte Kapazitaetsplanung fuer reale Programme vorwegzunehmen (die kommt erst ab
   Phase 5, wenn reale Programme anstehen). */
#define Q9_SYSTEM_MEM_BYTES (256 * 1024)

#endif // Q9_CONFIG_H

//────────────────────────────────────────────────────────────────────────────────────────────────
// EOF config.h                                                                            Ver. 1.00
//────────────────────────────────────────────────────────────────────────────────────────────────
