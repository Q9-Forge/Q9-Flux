//════════════════════════════════════════════════════════════════════════════════════════════════
// File:   q9_launcher.h                                                                  Ver. 1.00
// Owner:  Cld
// Desc.:  Fuenfunddreissigste Runde (2026-08-22, Andreas: "ich wollte eigentlich das man in den
//         Configurator kommt wenn man den emulator ohne parameter aufruft"): der komplette
//         interaktive Launcher/Config-Editor (bisher direkt in demo/integration_demo.c) als
//         wiederverwendbare Funktion -- q9.exe selbst (src/hal/posix/hal_posix.c) ruft sie auf,
//         wenn es ohne Argumente gestartet wird, GENAU die ORIGINALE Vision aus
//         Q9FLUX_EDITOR_de.md Abschnitt 1 ("Q9-Flux ohne Argumente gestartet -> interaktiver
//         Launcher statt direktem Boot"). Andreas' Architektur-Entscheidung (per AskUserQuestion):
//         EIN Binary -- die TUI-Module (q9_ansi/q9_screenbuf/q9_widgets/q9_listview/q9_input/
//         q9_filelist/q9_filedialog, alle leichtgewichtig, kein Musashi noetig) werden zusaetzlich
//         in q9.exe gelinkt, kein separater Sub-Prozess/exec(). demo/integration_demo.c bleibt als
//         duenner, eigenstaendiger Testbed-Wrapper erhalten (kein echtes Booten dort, s. dortiger
//         Kopfkommentar) -- der gesamte bisherige Code dieser Datei (Farbpalette, Feld-Definitionen,
//         Datei-Dialog-Integration, Tastatur-/Ereignisschleife) ist REIN VERSCHOBEN, nicht neu
//         geschrieben (volle Historie s. demo/integration_demo.c Ver. 1.00-3.90).
//
// Call:   q9_board_cfg_t cfg;
//         if (q9_launcher_run(&cfg)) { /* "B" gedrueckt -- cfg bootet */ }
//         else                       { /* Strg-C/EOF -- sauber beenden, kein Boot */ }
//
// Edition History
//─────────┬──────┬────────────────────────────────────────────────────────────────────────┬──────
// Date    │ Ver. │ Description                                                            │ By
//─────────┼──────┼────────────────────────────────────────────────────────────────────────┼──────
// 26-08-22│ 1.00 │ Fuenfunddreissigste Runde: Erster Wurf -- Extraktion aus                 │ Cld
//         │      │ demo/integration_demo.c, neue Taste B (Booten)                           │
//═════════╧══════╧════════════════════════════════════════════════════════════════════════╧══════
#ifndef Q9_LAUNCHER_H
#define Q9_LAUNCHER_H

#include "../../../src/kernel/boardcfg.h"

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_launcher_run
// Desc.:    Blockierend -- uebernimmt das Terminal (Rohmodus via q9_input, wird VOR der Rueckkehr
//           wieder sauber zurueckgesetzt, egal welcher Rueckgabewert). Zeigt den Config-Auswahl-/
//           Hardware-Uebersichtsbildschirm, bis der Nutzer entweder 'B' (Booten -- baut die
//           aktuellen Feldwerte in out_cfg, s. build_current_cfg() in q9_launcher.c) drueckt ODER
//           Strg-C/EOF sendet.
// Call:     q9_board_cfg_t cfg; if (q9_launcher_run(&cfg)) { q9_board_boot(NULL, NULL, NULL, &cfg); }
// Ret.:     1 = "Booten" gewaehlt, out_cfg gueltig befuellt. 0 = Strg-C/EOF, out_cfg unveraendert.
//════════════════════════════════════════════════════════════════════════════════════════════════
int q9_launcher_run(q9_board_cfg_t *out_cfg);

#endif /* Q9_LAUNCHER_H */
//────────────────────────────────────────────────────────────────────────────────────────────────
// EOF q9_launcher.h                                                                       Ver. 1.00
//────────────────────────────────────────────────────────────────────────────────────────────────
