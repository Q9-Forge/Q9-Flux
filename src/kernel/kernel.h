//════════════════════════════════════════════════════════════════════════════════════════════════
// File:   kernel.h                                                                        Ver. 1.10
// Owner:  AF
// Desc.:  Öffentliche Kernel-Schnittstelle für den Host (native main / JS-Loader).
//         Der Kernel ist nicht blockierend: der Host ruft q9_kernel_step() zyklisch auf.
//
// Call:   #include "kernel/kernel.h"
//
// Edition History
//─────────┬──────┬────────────────────────────────────────────────────────────────────────┬──────
// Date    │ Ver. │ Description                                                            │ By
//─────────┼──────┼────────────────────────────────────────────────────────────────────────┼──────
// 26-07-02│ 1.00 │ Initiale Version                                                       │ CF
// 26-07-03│ 1.10 │ q9_kernel_selftest() ergänzt                                           │ CF
//═════════╧══════╧════════════════════════════════════════════════════════════════════════╧══════
#ifndef Q9_KERNEL_H
#define Q9_KERNEL_H

#define Q9_VERSION "0.01"

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_kernel_init
// Desc.:    Initialisiert den Kernel und gibt das Boot-Banner aus. Einmalig nach q9_hal_init().
// Call:     q9_kernel_init()
//════════════════════════════════════════════════════════════════════════════════════════════════
void q9_kernel_init(void);

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_kernel_step
// Desc.:    Ein kooperativer Kernel-Tick. Muss vom Host zyklisch aufgerufen werden und kehrt
//           immer schnell zurück (nie blockieren!).
// Call:     q9_kernel_step()
//════════════════════════════════════════════════════════════════════════════════════════════════
void q9_kernel_step(void);

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_kernel_selftest
// Desc.:    Führt die Kernel-Selbsttests aus (Syscall-ABI). Rückgabe 0 = ok, sonst Fehlerzahl.
// Call:     fails = q9_kernel_selftest()
//════════════════════════════════════════════════════════════════════════════════════════════════
int q9_kernel_selftest(void);

#endif // Q9_KERNEL_H

//────────────────────────────────────────────────────────────────────────────────────────────────
// EOF kernel.h                                                                            Ver. 1.10
//────────────────────────────────────────────────────────────────────────────────────────────────
