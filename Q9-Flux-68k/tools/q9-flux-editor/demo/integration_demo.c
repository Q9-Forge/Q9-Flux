//════════════════════════════════════════════════════════════════════════════════════════════════
// File:   integration_demo.c                                                             Ver. 4.00
// Owner:  Claudia
// Desc.:  Duenner Wrapper um q9_launcher_run() (s. src/q9_launcher.h) -- bis Ver. 3.90 lebte hier
//         die komplette Launcher-/Editor-Implementierung selbst (volle Entstehungsgeschichte in
//         der Historie dieser Datei bis Ver. 3.90, s. Git). Fuenfunddreissigste Runde (2026-08-22,
//         Andreas: "ich wollte eigentlich das man in den Configurator kommt wenn man den emulator
//         ohne parameter aufruft"): der Code wanderte fast unveraendert nach src/q9_launcher.c,
//         damit q9.exe selbst (src/hal/posix/hal_posix.c) ihn OHNE separaten Sub-Prozess mitnutzen
//         kann (Andreas' Architektur-Entscheidung: EIN Binary). Dieses eigenstaendige Demo-
//         Programm bleibt als leichtgewichtiger Testbed erhalten (kein Musashi/CPU-Kern gelinkt,
//         s. Makefile) -- Taste 'B' (Booten) zeigt hier nur eine Meldung statt wirklich zu booten.
//
// Call:   make -C tools/q9-flux-editor demo-integration
//
// Edition History
//─────────┬──────┬────────────────────────────────────────────────────────────────────────┬──────
// Date    │ Ver. │ Description                                                            │ By
//─────────┼──────┼────────────────────────────────────────────────────────────────────────┬──────
// 26-08-17│ 1.00 │ Erster Wurf -- Andreas wollte sich das Ganze mal ansehen                │ Cld
// 26-08-17│ …    │ Runden 2-29 (Farbpalette, Resize-Overlay, Datei-Dialog, Feld-Typen,      │ Cld
//         │      │ echtes Laden/Speichern, CF-Image-Slots, ...) -- volle Historie s. Git-Log │
// 26-08-20│ 3.80 │ Dreissigste Runde: Tasten N/D fuer CF-Image-Slots                        │ Cld
// 26-08-21│ 3.90 │ Vierunddreissigste Runde: echte devdesc-Hardware-Eintraege statt Demo     │ Cld
// 26-08-22│ 4.00 │ Fuenfunddreissigste Runde: komplette Implementierung nach                 │ Cld
//         │      │ src/q9_launcher.c ausgelagert (q9.exe nutzt sie jetzt selbst, s. dort) --   │
//         │      │ diese Datei ist seither nur noch ein duenner main()-Wrapper                │
//═════════╧══════╧════════════════════════════════════════════════════════════════════════╧══════
#include <stdio.h>

#include "../src/q9_launcher.h"

int main(void)
{
    q9_board_cfg_t cfg;

    if (q9_launcher_run(&cfg)) {
        printf("Start gewaehlt -- Name: %s%s%s\n",
               cfg.name[0] ? cfg.name : "(kein Name)",
               cfg.rom_path[0] ? ", ROM: " : "",
               cfg.rom_path[0] ? cfg.rom_path : "");
        printf("(Dieser eigenstaendige Demo-Build bootet nicht wirklich -- kein Musashi/CPU-Kern\n"
               " gelinkt, s. Kopfkommentar. In q9.exe selbst startet der echte Boot direkt weiter.)\n");
    } else {
        printf("integration_demo beendet.\n");
    }
    return 0;
}

//────────────────────────────────────────────────────────────────────────────────────────────────
// EOF integration_demo.c                                                                  Ver. 4.00
//────────────────────────────────────────────────────────────────────────────────────────────────
