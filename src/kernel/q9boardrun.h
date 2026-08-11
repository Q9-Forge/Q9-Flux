//════════════════════════════════════════════════════════════════════════════════════════════════
// File:   q9boardrun.h                                                                      Ver. 1.10
// Owner:  AF
// Desc.:  5.3: Board-Boot-Runner — laedt ein Boot-ROM-Image von der Platte, verdrahtet Musashi
//         mit dem Board (q9_m68krt_attach_board) und laesst die emulierte CPU laufen
//         (Endlosschleife mit Timer-Polling, Abbruch per Ctrl-C). Das ist der Einstiegspunkt
//         fuer `q9.exe --rom <rom-datei> [--cf <image>]` (s. main() in hal_posix.c/
//         hal_native.c) — der normale Q9-Kernel wird in diesem Modus NICHT gestartet, es laeuft
//         ausschliesslich das emulierte Board. Native-only (wie q9board.h/m68krt.h).
//
//         Das echte Microware-Boot-ROM ist proprietaer und bleibt lokal — der Pfad kommt
//         deshalb per Kommandozeile, nichts davon liegt im Repository (docs/BOARD.md).
//
// Call:   return q9_board_boot("boardrom.bin", NULL, NULL);
//
// Edition History
//─────────┬──────┬────────────────────────────────────────────────────────────────────────┬──────
// Date    │ Ver. │ Description                                                            │ By
//─────────┼──────┼────────────────────────────────────────────────────────────────────────┬──────
// 26-07-05│ 1.00 │ 5.3: Erster Boot-Runner (ROM laden, Board verdrahten, laufen lassen)    │ CF
// 26-07-05│ 1.10 │ 5.5a: cf_path-Parameter (--cf <pfad>), NULL = Default "board_cf.img"    │ CF
// 26-07-13│ 1.20 │ 5.12: net_mode-Parameter (--net nat|vmnet), NULL = "nat"                │ CF
// 26-07-13│ 1.30 │ 5.13: net_mode "bridge:<ifname>" (BPF an physischer NIC)                │ CF
// 26-07-16│ 1.40 │ 5.19: Board-Config-Datei (boardcfg.h) — cfg-Parameter, mehrere CF-Images  │ CF
//═════════╧══════╧════════════════════════════════════════════════════════════════════════╧══════
#ifndef Q9_BOARDRUN_H
#define Q9_BOARDRUN_H

#include "boardcfg.h"

//════════════════════════════════════════════════════════════════════════════════════════════════
// Var:      q9_dbg_dump_requested
// Desc.:    Debug-Sondertaste (Ctrl-^, 0x1E) analog zum Ctrl-]-Host-Escape (hal_posix.c):
//           q9_hal_con_get() setzt dieses Flag und schluckt die Taste, statt sie an den Gast
//           weiterzureichen. Der Board-Runner (q9boardrun.c) prueft es einmal pro Hauptschleifen-
//           Durchlauf und dumpt bei Bedarf physischen RAM-Inhalt (direkt ueber q9_board_read32,
//           OHNE MMU-Uebersetzung -- Debug-Werkzeug fuer die Q9-OS-Kernel-RE-Arbeit, s.
//           Q9-OS/docs/REVERSE_ENGINEERING.md).
//════════════════════════════════════════════════════════════════════════════════════════════════
extern volatile int q9_dbg_dump_requested;

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_board_boot
// Desc.:    Laedt die ROM-Datei (max. 512 KByte Flash, s. docs/BOARD.md), initialisiert das Board
//           (16 MByte RAM, CF-Backing-Datei cf_path bzw. Default "local_images/board_cf.img" lazy, falls
//           cf_path NULL ist), verdrahtet Musashi und laesst die CPU in einer Endlosschleife
//           laufen: Bloecke von CPU-Takten ausfuehren, dazwischen den 100Hz-Timer kooperativ
//           pollen (q9_board_poll_timer -> q9_m68krt_set_irq(3), s. 5.2d). Kehrt nur bei
//           Ladefehler zurueck (Exit-Code fuer main); beendet wird der Lauf per Ctrl-C (die HAL
//           stellt das Terminal via atexit-Handler zurueck).
//           net_mode waehlt das Ethernet-Backend (s. q9_quicc_net_mode): NULL/"nat" =
//           eingebautes Mini-NAT, "vmnet" = echtes Netz via vmnet.framework (macOS, sudo/
//           Entitlement), "bridge:<ifname>" = echtes Netz via BPF an physischer NIC (macOS,
//           kein root, s. bpf_net.h).
//           5.19: cfg (optional, NULL = keine Config-Datei) liefert ROM/Netz und — neu — MEHRERE
//           CF-Images (rbf/pcf) auf Onboard- und RC2014-Interface. Vorrang: eingebaute Defaults
//           < Config-Datei < explizite CLI-Argumente (rom_path/cf_path/net_mode, sofern gesetzt).
// Call:     return q9_board_boot(argv[2], cf_path_or_NULL, net_mode_or_NULL, cfg_or_NULL);
//════════════════════════════════════════════════════════════════════════════════════════════════
int q9_board_boot(const char *rom_path, const char *cf_path, const char *net_mode,
                  const q9_board_cfg_t *cfg);

#endif // Q9_BOARDRUN_H

//────────────────────────────────────────────────────────────────────────────────────────────────
// EOF q9boardrun.h                                                                          Ver. 1.10
//────────────────────────────────────────────────────────────────────────────────────────────────
