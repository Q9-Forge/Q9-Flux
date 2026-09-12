//════════════════════════════════════════════════════════════════════════════════════════════════
// File:   q9_procspawn.h                                                                  Ver. 1.00
// Owner:  Claudia
// Desc.:  Startet ein externes Programm als KINDPROZESS im selben Terminal und wartet, bis es
//         beendet ist -- Grundlage fuer den "Start"-Button aus Q9FLUX_EDITOR_de.md Abschnitt 3/8
//         ("laut Abschnitt 3 soll 'Start' den Emulator direkt aus dem Editor heraus starten").
//         Andreas' Entscheidung (2026-08-16): Kindprozess+Warten statt exec() -- nach Beendigung
//         des Emulators kehrt die Kontrolle zum Editor zurueck (Config-Auswahlbildschirm neu
//         zeichnen ueber q9_screenbuf, statt im nackten Shell-Prompt zu landen).
//
//         Terminal-Rohmodus verschachtelt sich dabei von selbst richtig: der Kindprozess (Emulator)
//         sichert beim eigenen Start den dann aktuellen (bereits vom Editor gesetzten) Terminal-
//         Zustand und stellt GENAU DEN beim eigenen Beenden wieder her -- der Editor muss den
//         Rohmodus nicht erneut setzen, nur seinen eigenen Bildschirminhalt neu rendern.
//
// Call:   char *const argv[] = { "q9.exe", "meinesystem.q9", NULL };
//         int code = q9_procspawn_run("../../build/macos/q9.exe", argv);
//         if (code < 0) { /* Spawn fehlgeschlagen bzw. Kind per Signal beendet */ }
//
// Edition History
//─────────┬──────┬────────────────────────────────────────────────────────────────────────┬──────
// Date    │ Ver. │ Description                                                            │ By
//─────────┼──────┼────────────────────────────────────────────────────────────────────────┬──────
// 26-08-16│ 1.00 │ Erster Wurf -- POSIX (fork/execv/waitpid) implementiert+getestet,        │ Cld
//         │      │ Windows-Zweig (CreateProcess/WaitForSingleObject) geschrieben nach dem   │
//         │      │ Muster von src/hal/windows/hal_windows.c, mangels Windows-Host hier      │
//         │      │ UNGETESTET                                                               │
//═════════╧══════╧════════════════════════════════════════════════════════════════════════╧══════
#ifndef Q9_PROCSPAWN_H
#define Q9_PROCSPAWN_H

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_procspawn_run
// Desc.:    Startet path mit den Argumenten argv (NULL-terminiertes Array, argv[0] ist per Konvention
//           der Programmname selbst -- wie bei execv()) und wartet blockierend, bis es beendet ist.
//           Rueckgabe:
//             >= 0  Kind normal beendet, Wert ist dessen Exit-Code (0..255)
//             -1    Spawn selbst fehlgeschlagen (fork()/execv()/CreateProcess() -- Kind lief nie)
//             -2    Kind wurde abnormal beendet (nur POSIX: durch ein Signal, kein regulaerer
//                   Exit-Code -- z.B. abgestuerzt oder von aussen gekillt)
//           Blockiert den Aufrufer komplett, bis das Kind fertig ist (kein asynchrones Starten,
//           genau das gewuenschte Verhalten fuer "Emulator starten, warten, danach weitermachen").
// Call:     int code = q9_procspawn_run(path, argv)
//════════════════════════════════════════════════════════════════════════════════════════════════
int q9_procspawn_run(const char *path, char *const argv[]);

#endif /* Q9_PROCSPAWN_H */
//────────────────────────────────────────────────────────────────────────────────────────────────
// EOF q9_procspawn.h                                                                      Ver. 1.00
//────────────────────────────────────────────────────────────────────────────────────────────────
