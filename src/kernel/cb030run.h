//════════════════════════════════════════════════════════════════════════════════════════════════
// File:   cb030run.h                                                                      Ver. 1.00
// Owner:  AF
// Desc.:  5.3: CB030-Boot-Runner — laedt ein Boot-ROM-Image von der Platte, verdrahtet Musashi
//         mit dem CB030-Board (q9_m68krt_attach_board) und laesst die emulierte CPU laufen
//         (Endlosschleife mit Timer-Polling, Abbruch per Ctrl-C). Das ist der Einstiegspunkt
//         fuer `q9.exe --cb030 <rom-datei>` (s. main() in hal_posix.c/hal_native.c) — der
//         normale Q9-Kernel wird in diesem Modus NICHT gestartet, es laeuft ausschliesslich
//         das emulierte Board. Native-only (wie cb030.h/m68krt.h).
//
//         Das echte Microware-Boot-ROM ist proprietaer und bleibt lokal — der Pfad kommt
//         deshalb per Kommandozeile, nichts davon liegt im Repository (docs/CB030.md).
//
// Call:   return q9_cb030_boot("cb030rom.bin");
//
// Edition History
//─────────┬──────┬────────────────────────────────────────────────────────────────────────┬──────
// Date    │ Ver. │ Description                                                            │ By
//─────────┼──────┼────────────────────────────────────────────────────────────────────────┬──────
// 26-07-05│ 1.00 │ 5.3: Erster Boot-Runner (ROM laden, Board verdrahten, laufen lassen)    │ CF
//═════════╧══════╧════════════════════════════════════════════════════════════════════════╧══════
#ifndef Q9_CB030RUN_H
#define Q9_CB030RUN_H

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_cb030_boot
// Desc.:    Laedt die ROM-Datei (max. 512 KByte Flash, s. docs/CB030.md), initialisiert das Board
//           (16 MByte RAM, CF-Backing-Datei "cb030_cf.img" lazy), verdrahtet Musashi und laesst
//           die CPU in einer Endlosschleife laufen: Bloecke von CPU-Takten ausfuehren, dazwischen
//           den 100Hz-Timer kooperativ pollen (q9_cb030_poll_timer -> q9_m68krt_set_irq(3),
//           s. 5.2d). Kehrt nur bei Ladefehler zurueck (Exit-Code fuer main); beendet wird der
//           Lauf per Ctrl-C (die HAL stellt das Terminal via atexit-Handler zurueck).
// Call:     return q9_cb030_boot(argv[2]);
//════════════════════════════════════════════════════════════════════════════════════════════════
int q9_cb030_boot(const char *rom_path);

#endif // Q9_CB030RUN_H

//────────────────────────────────────────────────────────────────────────────────────────────────
// EOF cb030run.h                                                                          Ver. 1.00
//────────────────────────────────────────────────────────────────────────────────────────────────
