```
#═════════════════════════════════════════════════════════════════════════════════════════════════
# File:   Q9FLUX_EDITOR_de.md                                                             Ver. 5.41
# Owner:  Claudia
# Desc.:  Planungsnotiz (Andreas + Claudia, 2026-08-13): Vision fuer einen interaktiven Q9-Flux-
#         Launcher/Config-Editor. REIN PLANUNG -- noch kein Code auf diesen Editor selbst, nur die
#         Bausteine devschema.h/.c (s. dort) sind bereits gebaut. Vorlaeufig NUR Deutsch (Token-
#         Spar-Vereinbarung, s. Memory feedback_token_sparsam) -- englische Fassung folgt gesammelt.
#
# Edition History
#─────────┬──────┬────────────────────────────────────────────────────────────────────────┬──────
# Date    │ Ver. │ Description                                                            │ By
#─────────┼──────┼────────────────────────────────────────────────────────────────────────┼──────
# 26-08-13│ 1.00 │ Erster Wurf -- Andreas' Startbildschirm-/Hardware-Formular-Vision       │ Cld
# 26-08-15│ 1.10 │ Netzwerk-Backend-Status geklaert (Abschnitt 8a), Speicher-Feld-Frage    │ Cld
#         │      │ "Initialisierung nur bei ROM?" ergaenzt (Abschnitt 4.2); eine parallel  │
#         │      │ neu angelegte docs/CONFIGURATOR_SPEC_de.md hier eingemergt statt als    │
#         │      │ eigene Datei fortgefuehrt zu werden (Andreas' Entscheidung)             │
# 26-08-15│ 1.20 │ 4.1 CPU-Auswahl: Backend-/Config-Seite FERTIG (q9_cpu_type_t, [board]    │ Cld
#         │      │ cpu-Key, devschema "board"-Schema) -- UI-Auswahlfeld selbst noch offen   │
# 26-08-16│ 1.30 │ 2. Config-Auswahl: Bildschirmpuffer-Grundlage FERTIG (q9_screenbuf.h/.c,  │ Cld
#         │      │ snapshot/restore fuer den modalen Dialog) -- Frame-/Widget-Zeichnung und  │
#         │      │ Tastatur-/Ereignisschleife noch offen                                     │
# 26-08-16│ 1.40 │ 2. Config-Auswahl: erster Widget-Baustein FERTIG (q9_widgets.h/.c,        │ Cld
#         │      │ draw_frame -- ASCII-Rahmen mit Titel) -- Buttons/Textfelder und die        │
#         │      │ Tastatur-/Ereignisschleife weiterhin offen                                │
# 26-08-16│ 1.50 │ 3. "Start"-Mechanik geklaert (Andreas: Kindprozess statt exec()) + Grund-  │ Cld
#         │      │ lage FERTIG (q9_procspawn.h/.c), echter End-to-End-Test mit q9.exe         │
# 26-08-16│ 1.60 │ 2. Tastatur-Rohmodus+-Erkennung FERTIG (q9_input.h/.c) -- letzter Grund-    │ Cld
#         │      │ baustein, per expect/pty end-to-end verifiziert. Alle Bausteine da, es      │
#         │      │ fehlt jetzt nur noch das Zusammensetzen zum eigentlichen Programm           │
# 26-08-16│ 1.70 │ 2. Scrollbare Listenansicht FERTIG (q9_listview.h/.c, inkl. Scrollbalken);   │ Cld
#         │      │ "Q9TUI"-Bibliothek-Idee (Andreas) notiert, bewusst vertagt bis Grundstock    │
#         │      │ steht                                                                        │
# 26-08-16│ 1.80 │ Dynamische Groessenanpassung FERTIG (q9_term_size + Q9_KEY_RESIZE, SIGWINCH); │ Cld
#         │      │ dabei echten macOS-signal()-vs-sigaction()-Bug per Pty-Test gefunden+behoben  │
# 26-08-17│ 1.90 │ Integrations-Demo FERTIG (demo/integration_demo.c); Andreas' Feedback danach  │ Cld
#         │      │ umgesetzt: Mindestgroesse, proportionaler Scrollbalken, eigene Statuszeile,   │
#         │      │ echte Unicode-Box-Drawing-Zeichen, warme Farbpalette. Maus bewusst vertagt     │
# 26-08-17│ 2.00 │ Zweite Feedback-Runde: Statuszeile jetzt ALS untere Rahmenkante, Resize-       │ Cld
#         │      │ Entprellung (~150ms) gegen Flackern, Scrollbalken-Abstand zur Auswahl          │
# 26-08-17│ 2.10 │ Statuszeile ueber die VOLLE Breite (keine Ecken mehr unten) -- Andreas: "wenn  │ Cld
#         │      │ das gut ist, machen wir das vielleicht oben auch"                              │
# 26-08-17│ 2.20 │ Dritte Feedback-Runde: Kopfzeile jetzt auch volle Breite (heller), feste       │ Cld
#         │      │ Feldbreiten in der Statuszeile, Resize-Overlay-Modus ersetzt die 150ms-        │
#         │      │ Entprellung komplett (LIVE-Groessenanzeige waehrend des Ziehens,               │
#         │      │ q9_input_read_key_timeout() neu, Vollansicht erst nach 1s Stille)              │
# 26-08-17│ 2.30 │ Vierte Feedback-Runde: q9_ansi_resize_window (XTWINOPS) fuer automatisches     │ Cld
#         │      │ Vergroessern bei anhaltend zu kleinem Fenster, Overlay-Reihenfolge Columns/    │
#         │      │ Rows getauscht, Kopfzeile linksbuendig + heller                                │
# 26-08-17│ 2.40 │ Fuenfte Runde, IN ARBEIT: Design fuer den Datei-Auswahl-Dialog abgestimmt;      │ Cld
#         │      │ Q9_KEY_SHIFT_TAB + q9_filelist.h/.c (Verzeichnis-Scan) fertig, die eigentliche  │
#         │      │ Dialog-Zusammensetzung folgt noch                                               │
# 26-08-17│ 2.50 │ q9_filedialog.h/.c FERTIG (task #20) -- der Dialog selbst ist komplett, logisch │ Cld
#         │      │ getestet; offen bleibt nur noch die Einbindung ins Integrations-Demo (task #22) │
# 26-08-17│ 2.60 │ task #22 FERTIG -- Dialog per Taste 'O' im Integrations-Demo, echter Pseudo-    │ Cld
#         │      │ Terminal-Rauchtest bestanden. Datei-Auswahl-Dialog damit komplett abgeschlossen  │
# 26-08-17│ 2.70 │ Sechste Runde: zwei Bugs aus Andreas' erstem echten Test behoben -- Dialog       │ Cld
#         │      │ zeigt jetzt den echten Hauptbildschirm dahinter (statt Vollbild-Fuellfarbe),     │
#         │      │ Fokuswechsel auf/von der Dateiliste ist jetzt sichtbar (unfocus_sel_* Farbpaar)  │
# 26-08-17│ 2.80 │ Siebte Runde: eigener Fussbereich (footer_bg), "richtige" Halbblock-Buttons     │ Cld
#         │      │ (▀/▄), Filter-Aufklapp-Menue mit ▾ statt reinem Durchschalten, Scan-Verzeichnis  │
#         │      │ auf HOME gestellt                                                                │
# 26-08-17│ 2.90 │ Achte Runde: Bildlaufleiste im Hauptfenster+Dialog IMMER als Linie (nicht nur    │ Cld
#         │      │ bei Scrollbedarf), linke Rahmenlinie im Dialog dazu, Buttons/Filter/Popup jetzt   │
#         │      │ buendig mit der rechten Linie, Name-Spalte dynamisch (name_col_width), Spalten-  │
#         │      │ titel-Zeile exakt so breit wie die Tabelle, Namens-Kaestchen bei "Datei:"          │
# 26-08-17│ 3.00 │ Neunte Runde: Fensterkante+Bildlaufleiste zu EINER Linie verschmolzen (Haupt-      │ Cld
#         │      │ fenster UND Dialog), Dialog-Rahmen ohne Randspalten GENAU auf der Kante, beginnt   │
#         │      │ schon bei der Spaltentitel-Zeile statt erst bei der Liste                          │
# 26-08-17│ 3.10 │ Zehnte Runde ("wirkt jetzt doch gequetscht"): Luftspalte vor beiden Linien,        │ Cld
#         │      │ schmalere Buttons, Filter jetzt eigene Zeile, neue Statuszeile ganz unten,         │
#         │      │ Rahmenlinien reichen jetzt bis dorthin (nicht mehr nur um die Liste)               │
# 26-08-17│ 3.20 │ Elfte Runde: Buttons teilen sich jetzt Namens-/Filterzeile (2 Zeilen gespart),     │ Cld
#         │      │ Namenskuerzung mit "...", Randlinien immer body_bg, X verschoben, Statuszeile      │
#         │      │ gekuerzt + mit Trennstrichen                                                       │
# 26-08-17│ 3.30 │ Zwoelfte Runde: komplette Farbpalette auf durchgehende Gelb-/Orange-Leiter          │ Cld
#         │      │ umgerechnet (ein Farbton/eine Saettigung, nur Helligkeit unterscheidet die         │
#         │      │ Ebenen), Tabellenhintergrund dunkler, mehr Kontrast zwischen den Ebenen             │
# 26-08-17│ 3.40 │ Dreizehnte Runde: Kopfzeilen-Text dunkler (Kontrast), neue PAL_DIALOG_SUB_FG fuer  │ Cld
#         │      │ Tabellenkopf/Buttons, Dialog-Footer referenziert PAL_STATUS_BG direkt, echter Bug │
#         │      │ behoben (Hauptfenster-Randlinie war zeilenabhaengig unterschiedlich eingefaerbt)   │
# 26-08-17│ 3.50 │ Vierzehnte Runde: Resize-Overlay-Text auf PAL_STATUS_FG umgestellt (war durch die  │ Cld
#         │      │ Kopfzeilen-Verdunklung der letzten Runde fast unlesbar geworden), neue             │
#         │      │ status_fg/bg-Felder fuer die untere Dialog-Statuszeile (Angleich ans Hauptfenster), │
#         │      │ Resize waehrend offenem Dialog behoben + automatische Neuzentrierung                │
# 26-08-18│ 3.60 │ Fuenfzehnte Runde: Resize waehrend offenem Dialog WIRKLICH behoben (Overlay-Settle- │ Cld
#         │      │ Muster wie main() statt vollem Redraw bei jedem Zwischenschritt), Groessen-Overlay- │
#         │      │ Text an fester Position (3,3) statt zentriert (huepfte sonst waehrend des Ziehens)  │
# 26-08-18│ 3.70 │ Sechzehnte Runde: erweiterbare Listeneintraege -- q9_listview_item_t + _item_rows()/│ Cld
#         │      │ _scroll_ex()/_move_ex()/_render_ex() (NEU, bestehende Funktionen unveraendert),     │
#         │      │ Q9_GLYPH_RIGHT_ARROW dazu, Enter klappt den ausgewaehlten Eintrag auf/zu             │
# 26-08-18│ 3.80 │ Siebzehnte Runde: neuer exp_bg-Parameter an render_ex() -- Kopfzeile eines           │ Cld
#         │      │ aufgeklappten, nicht ausgewaehlten Eintrags bekommt eigenen Hintergrund (Andreas:    │
#         │      │ "die Headerzeile geht ein wenig unter"); Feld-Bearbeitung IM Eintrag noch offen       │
# 26-08-18│ 3.90 │ Achtzehnte Runde (Phase 1/4): Feld-Navigation + Text-Bearbeitung -- q9_listview_     │ Cld
#         │      │ field_t (Label+editierbarer Wert), field_enter/_leave/_escape/_move/_putc/_backspace,│
#         │      │ Pfeil rechts/links/Esc; Dateiauswahl/Numerisch/Boolean noch offen                    │
# 26-08-18│ 4.00 │ Neunzehnte Runde (Phase 2/4): erster fester Eintrag "Emulator-Konfiguration" +       │ Cld
#         │      │ neuer BUTTON-Feldtyp (q9_listview_field_kind_t), run_file_dialog() liefert jetzt      │
#         │      │ optional den rohen Dateinamen zurueck; Numerisch/Boolean noch offen                  │
# 26-08-18│ 4.10 │ Zwanzigste Runde: Datei-Dialog scannt jetzt ~/.q9-flux (wird bei Bedarf angelegt)     │ Cld
#         │      │ statt HOME, Filter auf .q9 umgestellt (Glob-Stil "*.q9" traf NICHT -- ext_matches()  │
#         │      │ erwartet reine Endung, per pyte-Test gefunden+korrigiert)                             │
# 26-08-18│ 4.20 │ Einundzwanzigste Runde: TEXT+BUTTON-Paar jetzt Sonderfall in q9_listview.c -- Wert-  │ Cld
#         │      │ Box (box_fg/bg) + echter dreizeiliger Button mit Halbblock-Kappen "wie im Dialog",    │
#         │      │ vertikal zentriert neben dem Textfeld (Andreas' Wunsch, per Mockup praezisiert)       │
# 26-08-18│ 4.30 │ Zweiundzwanzigste Runde: Wert-Box-Breite von 20 auf 35 Zeichen (Andreas: "im Dialog   │ Cld
#         │      │ sind es ca. 35 Zeichen") -- dasselbe Mass wie das Namens-Kaestchen im Datei-Dialog     │
# 26-08-18│ 4.40 │ Dreiundzwanzigste Runde (Phase 3/4): numerische Feldtypen NUMERIC_DEC/_HEX (zwei     │ Cld
#         │      │ neue kind-Werte statt neuer Struct-Felder), "$"-Praefix automatisch, Bereichs-       │
#         │      │ Beispiel am Slot:-Feld (0-255); Boolean-Feldtyp (Phase 4/4) noch offen               │
# 26-08-18│ 4.50 │ Vierundzwanzigste Runde (Phase 4/4, letzte Phase): Boolean-Feldtyp fertig --          │ Cld
#         │      │ Q9_LISTVIEW_FIELD_BOOLEAN als weiterer neuer kind-Wert, field_toggle() schaltet      │
#         │      │ ja/nein um (Leertaste), alle bestehenden ja/nein-Felder umgestellt. Alle vier         │
#         │      │ Eingabearten (Text/Datei/Numerisch/Boolean) jetzt vorhanden                          │
# 26-08-18│ 4.60 │ Fuenfundzwanzigste Runde: echtes Laden der .q9-Datei -- integration_demo.c linkt      │ Cld
#         │      │ jetzt src/kernel/boardcfg.c (den ECHTEN Board-Config-Parser des Emulators), neue      │
#         │      │ Felder Name:/ROM:/Netz:/CPU: bei Emulator-Konfiguration werden nach Dateiauswahl      │
#         │      │ befuellt, Fehleranzeige bei kaputter Datei                                            │
# 26-08-18│ 4.70 │ Sechsundzwanzigste Runde: Speichern-Funktion -- q9_board_cfg_save() NEU in            │ Cld
#         │      │ boardcfg.h/.c (Gegenstueck zu q9_board_cfg_load(), voller Roundtrip inkl. [cfN] und   │
#         │      │ relativer Pfade), Taste S im Editor schreibt Name:/ROM:/Netz:/CPU: zurueck            │
# 26-08-18│ 4.80 │ Siebenundzwanzigste Runde: [cfN]-Abschnitte jetzt als eigene CF-Image-#N-Eintraege im  │ Cld
#         │      │ Editor sichtbar/editierbar (Typ:/Bus:/Unit:/Datei:), fest verankert am Ende der Liste │
#         │      │ (g_item_count waechst/schrumpft dynamisch), voll ins Speichern eingebunden            │
# 26-08-18│ 4.90 │ Achtundzwanzigste Runde: mechanische NUMERIC-Uebernahme -- CLUT "Eintraege:" auf       │ Cld
#         │      │ NUMERIC_DEC (einziger verbliebener reiner Zahlenwert), "Groesse:"-Felder bleiben       │
#         │      │ bewusst TEXT (Einheit im Wert, K/MB/Leerzeichen waeren sonst nicht mehr tippbar)       │
# 26-08-18│ 5.00 │ Neunundzwanzigste Runde: eigenes Symbol fuer BOOLEAN-Felder -- neue Glyphen             │ Cld
#         │      │ Q9_GLYPH_CHECKBOX_ON/_OFF (☑/☐, q9_screenbuf.h), render_ex() zeichnet sie vor dem      │
#         │      │ Wert (analog zum "$"-Praefix bei NUMERIC_HEX)                                          │
# 26-08-20│ 5.10 │ Dreissigste Runde ("Anlegen/Loeschen von Hardware-Instanzen", Andreas' Vorgabe: pro     │ Cld
#         │      │ Hardware ein eigenes Sourcefile, Fokus Ausfuehrungsgeschwindigkeit): Pilot-Vertical-    │
#         │      │ Slice am Beispiel "cf" -- neues q9_devdesc_t (devdesc.h) vereint Vtable+Feldschema+     │
#         │      │ Fast-Table-Flag, CF komplett nach src/devices/cf/cf.c verschoben, boardcfg.c-Schema-    │
#         │      │ Gegenprobe, echte Tasten N/D fuer die vier CF-Image-Slots im Editor. Abschnitt 4        │
#         │      │ ("Hardware hinzufuegen" fuer BELIEBIGE Typen) bleibt Folgeschritt, s. dort              │
# 26-08-21│ 5.20 │ Einunddreissigste Runde: dasselbe Muster fuer alle restlichen Typen -- quicc/mc6845/     │ Cld
#         │      │ framebuf/clut (nur q9_devdesc_t-Eintrag, lebten schon eigenstaendig) UND               │
#         │      │ duart68681/rtc72421/timer_irq (komplette Verschiebung aus q9board.c, bit-identisch      │
#         │      │ per Boot-Test bestaetigt). q9board.c auf reine RAM/ROM/REMAP-Logik geschrumpft.         │
#         │      │ Nur noch nettty (Sonderfall, s. Abschnitt 4) fehlt fuer volle Abdeckung                 │
# 26-08-21│ 5.30 │ Zweiunddreissigste Runde: nettty (letzter Typ) -- Andreas' Erinnerung "jedes Geraet     │ Cld
#         │      │ einzeln, eigener Descriptor, eigene Adresse, im Array" -- acht separate devreg-           │
#         │      │ Eintraege statt einem, Musashi-Entkopplung per Funktionszeiger-Hook. ALLE NEUN Typen      │
#         │      │ jetzt vereinheitlicht                                                                     │
# 26-08-21│ 5.31 │ Dreiunddreissigste Runde: REMAP-Trigger als zehntes Geraet -- Andreas' eigene Idee        │ Cld
#         │      │ (Bootflag/RAM-Umschaltung). Bewusst zweigeteilt: nur der Trigger selbst wandert nach       │
#         │      │ src/devices/remap/, die RAM/ROM-Interpretation bleibt im Performance-Fast-Path. Ein        │
#         │      │ neuer Testfall deckte einen echten Registrierungsfehler auf, echter Boot-Test bestaetigt   │
#         │      │ die Korrektur                                                                              │
# 26-08-21│ 5.32 │ Vierunddreissigste Runde: Editor zeigt echte Hardware-Eintraege statt Demo-Platzhalter --  │ Cld
#         │      │ zwanzig hartcodierte integration_demo.c-Eintraege entfallen, init_items() baut sie jetzt   │
#         │      │ zur Laufzeit aus q9_devdesc_get() (neun Typen, "cf" hat schon CF-Image-#N). Generische     │
#         │      │ boardcfg.c-Mehrfach-Instanziierung + Typ-Auswahl-Dialog bewusst zurueckgestellt (kein      │
#         │      │ akuter Nutzen -- nur "cf" braucht das ueberhaupt)                                          │
# 26-08-22│ 5.40 │ Fuenfunddreissigste Runde: die urspruengliche Vision endlich umgesetzt -- q9.exe ohne      │ Cld
#         │      │ Argumente startet jetzt den interaktiven Configurator (q9_launcher_run(), aus              │
#         │      │ integration_demo.c ausgelagert nach src/q9_launcher.c, EIN Binary statt Sub-Prozess).      │
#         │      │ Neue Taste B (Booten) bootet direkt im selben Prozess weiter. Echter End-to-End-Test:      │
#         │      │ Datei laden -> B -> "8 devices online"                                                     │
# 26-08-30│ 5.41 │ Sechsunddreissigste Runde: aktueller Verzeichnispfad im Datei-Dialog -- die 2026-08-17     │ Cld
#         │      │ zurueckgestellte optionale Erweiterung nachgeruestet, rechtsbuendig auf der Kopfzeile      │
#         │      │ vor "X", Titel hat Vorrang. Ausserdem: hal_windows.c fehlte im "q9.exe ohne Argumente"-    │
#         │      │ Umbau der letzten Runde, jetzt symmetrisch nachgezogen (PR #72)                            │
#═════════╧══════╧════════════════════════════════════════════════════════════════════════╧══════
```

# Q9-Flux-Launcher/Config-Editor — Planungsstand

**Status: reine Planung.** Es existiert noch kein Code fuer diesen Editor. Bereits gebaut, als
Grundstein fuer den Editor gedacht: `src/kernel/devschema.h/.c` (selbstbeschreibende Feld-Schemata
je Geraetetyp, Pilot fuer "cf") — s. dortigen Kopfkommentar.

## 0. Vorgeschichte: gescheiterter erster Versuch

`tools/q9-launcher-prototype/` (Turbo Vision + ncurses, C++, als Git-Submodule
`third_party/tvision` eingebunden, hinzugefuegt 2026-08-06) war ein erster Anlauf fuer einen
Startbildschirm. Er baut technisch nach wie vor fehlerfrei, aber **Andreas' Urteil (2026-08-13):
"eine Vollkatastrophe"** — die KI (Claude) hatte ueber das Framework keine verlaessliche Kontrolle
ueber Farben, Objektposition oder Verhalten der Widgets. Ursache vermutet: Turbo Vision
positioniert/faerbt ueber mehrere Indirektionsebenen (TRect relativ zum Elternobjekt,
TPalette-Indizes) — eine Bearbeitungsanweisung wie "3 Zeilen tiefer" muss durch diese Ebenen
hindurch verstanden werden, was fehleranfaellig war.

**Für den Neuanfang entschieden:** kein TUI-Framework (weder Turbo Vision noch ncurses noch
FTXUI), sondern rohe ANSI-/VT100-Escape-Codes direkt in C99 — Zeilen/Spalten/Farben stehen dann als
LITERALE Zahlen im Code, keine Framework-Indirektion. Ein kleiner Testfall dafuer (z.B. drei
Textzeilen mit fester Position/Farbe, Andreas gibt Verschiebe-Anweisungen zum Nachpruefen) ist
vorgeschlagen, aber noch NICHT gebaut (auf Wunsch verschoben, bis der Editor wirklich angegangen
wird). `tools/q9-launcher-prototype/` und das `tvision`-Submodule bleiben vorerst im Repo liegen
(Aufraeumen erst, wenn ein Nachfolger tatsaechlich steht).

## 1. Start-Ablauf

Q9-Flux ohne Argumente gestartet → interaktiver Launcher statt direktem Boot (heutiges
`--rom/--cf/--net`-CLI bzw. Config-Datei-als-Positionsparameter bleibt fuer Skripte/Tests
unveraendert nutzbar, s. Abschnitt 6 Autostart).

**FERTIG (2026-08-22, Fuenfunddreissigste Runde):** Andreas' Rueckfrage "wie starte ich den q9 mit
dem Configurationseditor" + Klaerung "ich wollte eigentlich das man in den Configurator kommt wenn
man den emulator ohne parameter aufruft" -- genau diese, seit Beginn geplante Vision, war bis dahin
nie umgesetzt (`q9.exe` ohne Argumente druckte nur eine Usage-Meldung + Abbruch). Andreas'
Architektur-Entscheidung (per `AskUserQuestion`): **EIN Binary** statt zweier sich gegenseitig
exec()ender Programme -- die komplette Launcher-Implementierung (bisher nur im eigenstaendigen
Demo-Tool `tools/q9-flux-editor/demo/integration_demo.c`) wanderte fast unveraendert nach
`tools/q9-flux-editor/src/q9_launcher.c`/`.h` (neue Funktion `q9_launcher_run()`), die TUI-Module
(q9_ansi/q9_screenbuf/q9_widgets/q9_listview/q9_input/q9_filelist/q9_filedialog -- alle
leichtgewichtig, kein Musashi/CPU-Kern noetig) werden seither zusaetzlich in `q9.exe` selbst
gelinkt (Root-Makefile `LAUNCHER_SRC`). `src/hal/posix/hal_posix.c`s `main()` ruft
`q9_launcher_run()` auf, wenn weder eine Config-Datei noch `--rom` uebergeben wurde; waehlt der
Nutzer dort die NEUE Taste **B** ("Booten"), baut `build_current_cfg()` (aus dem bisherigen
`save_q9_config()` ausgelagert, jetzt von Speichern UND Booten gemeinsam genutzt) die aktuellen
Feldwerte in eine `q9_board_cfg_t`, und `main()` bootet damit im SELBEN Prozess direkt weiter --
kein Speichern-Zwang, eine Config laesst sich "probeweise" starten. Strg-C/EOF im Launcher beendet
`q9.exe` sauber (Exit-Code 0, kein Fehler). Das eigenstaendige Demo-Tool bleibt als
leichtgewichtiger Testbed erhalten (zeigt bei "B" nur eine Meldung statt wirklich zu booten, kein
Musashi dort gelinkt). Verifiziert: `make test` (Root + Editor) komplett gruen, echter End-to-End-
Pseudo-Terminal-Test -- `q9.exe` ohne Argumente gestartet, Datei-Dialog geoeffnet, echte `.q9`-Datei
geladen (Felder korrekt befuellt inkl. automatisch erscheinendem CF-Image-#0-Eintrag), Taste B
gedrueckt -- der Emulator bootet daraufhin tatsaechlich bis "8 devices online".

**Titelkopf:** Name ("Q9 Flux"), kurze Vorstellung, Version — groesser/prominent dargestellt.

## 2. Config-Auswahl (Hauptbildschirm)

Gerahmtes Feld (Name noch offen — aktueller Arbeitsbegriff "Config-Auswahl"), zunaechst leer.
`<Button>` oeffnet einen **modalen** Dialog:

- ASCII-gerahmt, liegt ÜBER dem restlichen Bildschirminhalt (braucht: Bildschirmbereich vor dem
  Zeichnen sichern, nach dem Schliessen wiederherstellen — konkrete technische Anforderung an die
  ANSI-Implementierung, kein reines Layout-Detail)
- zeigt alle `.q9`-Dateien aus `~/.q9-flux` (s. Abschnitt 7)
- Navigation per Cursor-Tasten ODER Maus (Maus-Unterstuetzung ist eine echte Zusatzanforderung —
  xterm-Maus-Reporting-Escape-Codes, Terminal-/Plattform-Abdeckung noch zu pruefen)
- `<OK>` uebernimmt die Auswahl, `<Abbrechen>` verwirft — Dialog schliesst, alter Inhalt kommt
  zurueck

**Bildschirmpuffer-Grundlage FERTIG (2026-08-16):** `tools/q9-flux-editor/src/q9_screenbuf.h/.c`,
auf `q9_ansi.h` aufgesetzt. Fester 2D-Zellenpuffer (Zeichen + Vorder-/Hintergrundfarbe, kein
malloc), `snapshot()`/`restore()` fuer genau den oben beschriebenen Anwendungsfall (Rechteck
sichern, Dialog druebermalen, Rechteck wiederherstellen), `render()` erzeugt daraus die ANSI-
Byte-Folge (mit optionalem Origin-Versatz, damit nur ein wiederhergestelltes Rechteck neu gezeichnet
werden muss statt des ganzen Schirms). Verifiziert per `make test-screenbuf` (34 Checks, Teil von
`make test`) -- Ruecklese-Parse wie beim ANSI-Modul, plus der eigentliche Sichern/Ueberzeichnen/
Wiederherstellen-Ablauf als expliziter Testfall.

**Erster Widget-Baustein FERTIG (2026-08-16):** `tools/q9-flux-editor/src/q9_widgets.h/.c`,
`q9_screenbuf_draw_frame()` -- zeichnet den ASCII-Rahmen ('+'/'-'/'|', bewusst kein Unicode-
Box-Drawing, s. dortiger Kopfkommentar) mit optionalem mittigen Titel in der oberen Kante,
bounds-sicher wie alle q9_screenbuf-Funktionen. Verifiziert per `make test-widgets` (21 Checks).

**Tastatur-Rohmodus + Tastenerkennung FERTIG (2026-08-16):** `tools/q9-flux-editor/src/
q9_input.h/.c` -- letzter fehlender Grundbaustein, um die bisherigen Teile zu echten, bedienbaren
Bildschirmen zusammenzusetzen. Eigener, vom Emulator-`src/hal/*` UNABHAENGIGER Rohmodus (dort
transparente Durchreichung an OS-9, hier Navigations-Tastenerkennung -- andere Verwendung trotz
gleicher termios-Grundtechnik, s. dortiger Kopfkommentar). Erkennt Zeichen, Enter, Tab, Backspace,
Pfeiltasten (ANSI-Sequenzen), Strg-C (als Byte, ISIG aus) und EOF. Die eigentliche
Sequenz-Erkennung (`q9_input_decode`) ist eine REINE Funktion (Byte-Puffer rein, Taste raus) --
loest insbesondere die klassische ESC-Mehrdeutigkeit (einzelnes Escape vs. Beginn einer
Pfeiltasten-Sequenz) ueber einen kurzen Timeout (100ms) in der I/O-Huelle, waehrend die
Entscheidungslogik selbst ohne echtes Terminal testbar bleibt (`make test-input`, 26 Checks).
**Zusaetzlich echter End-to-End-Test** (ueber die reinen Selbsttests hinaus, wie schon bei
q9_procspawn): ein Wegwerf-Testprogramm ueber `expect`/ein echtes Pseudo-Terminal gefahren --
echte ANSI-Pfeiltasten-Sequenzen kommen korrekt als UP/DOWN/RIGHT/LEFT an, ein einzelnes ESC ohne
Folgebytes wird nach Ablauf des Timeouts korrekt als Escape erkannt, Strg-C kommt als Byte an
(kein SIGINT-Absturz), und ein Zeichen direkt nach einem einzelnen ESC wird sauber als eigene
Taste erkannt (kein Verschlucken durch den Pending-Byte-Mechanismus). Windows-Zweig (`_kbhit`/
`_getch`, Scan-Codes wie `src/hal/windows/hal_windows.c`) geschrieben, mangels Windows-Host hier
UNGETESTET -- inkl. eines offenen Fragezeichens im Code-Kommentar, ob Strg-C dort ueberhaupt als
Byte ankommt oder vom Standard-Handler abgefangen wird.

**Scrollbare Listenansicht FERTIG (2026-08-16):** Andreas' Frage "Könnte man einen Bereich
Scrollbar machen?" (mehr Felder/Hardware-Eintraege als zwischen Titel und den START/SAVE/EXIT-
Buttons sichtbar) -- `tools/q9-flux-editor/src/q9_listview.h/.c`. `q9_listview_scroll()` ist die
reine Kernlogik ("Auswahl bleibt immer im Sichtfenster", scrollt nie weiter als noetig), ohne
echten Bildschirmpuffer testbar (wie `q9_input_decode`). `q9_listview_render()` zeichnet die
sichtbaren Eintraege in einen `q9_screenbuf_t`, hebt die Auswahl farblich hervor, und zeichnet
einen echten Ein-Zeichen-Scrollbalken ('|' Spur, '#' Position) -- NUR wenn tatsaechlich mehr
Eintraege da sind als ins Sichtfenster passen (sonst keine Balken-Zeichnung, nichts zu scrollen).
Verifiziert per `make test-listview` (34 Checks).

**Dynamische Grössenanpassung FERTIG (2026-08-16):** Andreas' Wunsch "wäre ja toll wenn es
halbwegs dynamisch auf die aktuelle Grösse reagiert". `q9_input.h/.c` bekam `q9_term_size()`
(aktuelle Zeilen/Spalten) + `Q9_KEY_RESIZE` als neues Ereignis im selben `q9_key_t`-Kanal wie
Tastendruecke. POSIX: ein echtes SIGWINCH unterbricht den blockierenden `read()` SOFORT (kein
Polling, keine Verzoegerung ueber die Signal-Zustellzeit hinaus) -- die Hauptschleife bekommt
`Q9_KEY_RESIZE` wie jedes andere Ereignis und kann Groesse neu abfragen + alles neu zeichnen.
**Dabei ein echter, per Pseudo-Terminal-Test gefundener Bug behoben:** die urspruengliche
`signal(SIGWINCH, ...)`-Installation liess das Ereignis auf macOS unbemerkt, weil `signal()` dort
mit automatischem Syscall-Neustart installiert (BSD-Erbe) -- der blockierende `read()` wurde
NICHT mit EINTR unterbrochen, sondern lief transparent weiter, das Resize-Ereignis kam erst beim
naechsten ECHTEN Tastendruck an statt sofort. Fix: `sigaction()` mit `sa_flags=0` (explizit ohne
`SA_RESTART`) statt `signal()`. Verifiziert per `expect`/echtem Pty-Resize (`stty rows/columns`)
-- zwei aufeinanderfolgende Groessenaenderungen (30x100, dann 40x120) kommen beide sofort und mit
der exakt richtigen Groesse an, ganz ohne dazwischenliegenden Tastendruck. Windows: kein
SIGWINCH-Aequivalent, `q9_term_size()` funktioniert dort (`GetConsoleScreenBufferInfo`), aber
`Q9_KEY_RESIZE` wird nie von sich aus geliefert -- ein kuenftiger Windows-Zweig der Hauptschleife
muesste selbst regelmaessig nachfragen (mangels Windows-Host hier ungetestet).

**Integrations-Demo FERTIG (2026-08-17):** Andreas wollte sich das Ganze mal ansehen -- bis dahin
gab es nur einzelne, fuer sich getestete Module ohne zusammengesetztes Programm.
`tools/q9-flux-editor/demo/integration_demo.c` (`make demo-integration`) zeigt alle sechs
Bausteine zusammen in einem echten, bedienbaren Bildschirm: Rahmen mit Titel, scrollbare
20-Eintrag-Demo-Geraeteliste, Pfeiltasten-Navigation, Statuszeile, Reaktion auf Terminal-Resize.
KEIN echter Editor (keine Config-Anbindung, keine echten Hardware-Typen, keine Buttons) -- reine
Sichtpruefung, dass die Bausteine zusammenpassen. Per `expect`/Pseudo-Terminal-Rauchtest
verifiziert.

**Andreas' Feedback nach dem ersten Ansehen (2026-08-17), alles umgesetzt:**
- **Mindestgroesse**: unter 60x20 sah das Layout "sehr komisch" aus -- `integration_demo.c` zeigt
  darunter jetzt einen Hinweistext ("Fenster zu klein... mind. 20x60") statt eines verzerrten
  Rahmens. `MIN_ROWS`/`MIN_COLS`-Konstanten, Anwendungs-Policy (nicht Teil der Bibliothek selbst).
- **Scrollbalken-Griff war immer nur 1 Zeichen gross** -- jetzt PROPORTIONAL zum sichtbaren Anteil
  (`q9_listview.c`, z.B. 50% sichtbar -> Griff nimmt 50% der Balkenhoehe ein), mindestens 1 Zeile,
  hoechstens Balkenhoehe-1 (damit immer erkennbar bleibt, dass es ueberhaupt etwas zu scrollen
  gibt). Neue Tests inkl. genau Andreas' eigenem 50%-Beispiel.
- **"Kleine Linie ganz rechts unten"** -- war die Statuszeile, die bisher in die untere
  Rahmenkante hineingemischt wurde (Text ueberschrieb nur einen Teil der Kanten-Striche, der Rest
  blieb als kurzes Dashes-Stueck sichtbar). Behoben: Statuszeile bekommt jetzt eine EIGENE Zeile
  mit eigenem (gedecktem, "inversem") Hintergrund, die untere Rahmenkante bleibt durchgehend sauber.
- **"Volle Linien" statt ASCII** -- `q9_screenbuf.h` bekam `Q9_GLYPH_HLINE/VLINE/TL/TR/BL/BR/BLOCK`:
  Ein-Byte-Sentinels (kein Struktur-Umbau von `q9_screencell_t` noetig), die ERST beim Rendern in
  echtes UTF-8-Unicode-Box-Drawing (`─│┌┐└┘█`) uebersetzt werden. `draw_frame()` und der
  Scrollbalken nutzen sie jetzt statt `'+'/'-'/'|'/'#'`. BEKANNTER KOMPROMISS: aeltere
  Windows-Konsolen (vor UTF-8-Codepage/Windows Terminal) koennten das falsch darstellen --
  ungetestet, da kein Windows-Host hier verfuegbar.
- **Warme Farbpalette** ("aehnliche Toene", Vorbild Hermes-Farbschema) -- `integration_demo.c`
  nutzt jetzt konsequent Amber-/Beige-/Braun-Toene statt der urspruenglichen bunt gemischten
  Zufallsfarben. Eine "kuehle" Variante (Blau/Gruen/Cyan) waere nach demselben Muster moeglich.
  Eine echte UMSCHALTBARE Palette (mehrere Saetze + Laufzeit-Auswahl) ist als "Endausbau"-Idee
  vorgemerkt, noch nicht gebaut -- aktuell nur EIN fest verdrahteter Satz im Demo-Programm.

**Maus-Unterstuetzung -- weiterhin NICHT angefangen, bewusst als eigene Aufgabe vertagt:**
Andreas' Beobachtung ("mit dem Scrollrad schiebe ich momentan das ganze Fenster weg") ist genau
das erwartete Verhalten OHNE aktivierte Maus-Unterstuetzung: das Terminal-EMULATORPROGRAMM selbst
(nicht unser Code) faengt das Mausrad ab und scrollt seinen eigenen Scrollback-Puffer, weil wir
ihm nie gesagt haben, dass wir Mausereignisse stattdessen SELBST bekommen wollen (xterm-Mouse-
Reporting muss explizit per Escape-Sequenz angefordert werden). Bereits in Abschnitt 2 als "echte
Zusatzanforderung" markiert (SGR-Mausmodus aktivieren/deaktivieren, Escape-Sequenzen parsen,
Terminal-Abdeckung pruefen) -- aus Umfangsgruenden nicht Teil dieser Aenderungsrunde, eigener
naechster Schritt.

**Zweite Feedback-Runde nach dem ersten Ansehen (2026-08-17), alles umgesetzt:**
- **Fenster-Verkleinerung stoppen** -- Andreas fragte, ob sich das Verkleinern des Terminal-
  Fensters selbst unterbinden liesse. Antwort: nein, technisch nicht moeglich -- die
  Fenstergroesse kontrolliert das Terminal-Programm/der Fenstermanager des Hosts, nicht der darin
  laufende Prozess. Der Hinweistext bei Unterschreitung der Mindestgroesse bleibt die einzig
  moegliche Reaktion.
- **Statuszeile ALS untere Rahmenkante** ("quasi der untere Rahmenteil") -- statt einer eigenen
  Zeile ueber der Kante (vorherige Feedback-Runde) ueberschreibt die Statuszeile jetzt direkt die
  untere Kante SELBST. **Direkt danach noch einmal nachgebessert (Andreas: "aufgeraeumter, besser
  als dieser doppelte Strich"):** die Statuszeile geht jetzt ueber die VOLLE Breite (Spalte 0 bis
  cols-1), ueberschreibt auch die beiden unteren Eckzeichen -- keine Ecken mehr unten. Andreas'
  Ankuendigung: "wenn das gut ist, machen wir das vielleicht oben auch" (Titelzeile) -- **erledigt
  in der dritten Feedback-Runde, s.u.**
- **Resize-Flackern behoben (ERSTER Ansatz, s. dritte Feedback-Runde fuer die endgueltige
  Loesung)** -- `q9_input.c` bekam `wait_for_resize_settle()`: ein per Maus gezogenes Resize
  loest viele SIGWINCH kurz hintereinander aus, `Q9_KEY_RESIZE` wurde erst ~150ms NACH dem
  letzten davon geliefert (Entprellung), statt bei jeder Zwischengroesse einzeln neu zu zeichnen.
- **Scrollbalken-Abstand** -- die markierte Zeile wurde bisher bis direkt an den Scrollbalken
  herangezeichnet ("verschmilzt"). `q9_listview.c`: ein Zeichen Abstand ergaenzt.

**Dritte Feedback-Runde (2026-08-17), alles umgesetzt:**
- **Kopfzeile jetzt ebenfalls ueber die volle Breite** (analog zur Statuszeile, Andreas' eigene
  Ankuendigung von oben) -- etwas HELLER als die Statuszeile (`PAL_HEADER_BG` statt
  `PAL_STATUS_BG`), gleiche Farbfamilie, damit man Kopf/Fuss auf einen Blick unterscheidet.
- **Feste Feldbreiten in der Statuszeile** -- der Eintragsname huepfte je nach Laenge hin und her,
  das "Terminal:"-Feld sprang mit. `integration_demo.c`: `NAME_FIELD_WIDTH` (30 Zeichen,
  linksbuendig aufgefuellt/abgeschnitten per `%-*.*s`) + feste Ziffernbreiten fuer Zeilen/Spalten
  -- "Terminal:" steht jetzt immer an derselben Spalte, unabhaengig von Auswahl/Groesse.
- **Resize-Overlay-Modus ersetzt die 150ms-Entprellung komplett** -- Andreas: "das sieht einfach
  bloed aus, wenn man immer versucht den kompletten Inhalt darzustellen" WAEHREND des Ziehens, UND
  er wollte dabei eine LIVE aktualisierte Groessenanzeige sehen (kein bloss einmaliges Update am
  Ende). Beides zusammen widerspricht der reinen 150ms-Entprellung (die haette Zwischenwerte
  komplett verschluckt) -- deshalb grundlegend umgebaut:
  - `q9_input.c`: `wait_for_resize_settle()`/die 150ms-Entprellung komplett ENTFERNT,
    `Q9_KEY_RESIZE` kommt wieder sofort/unverzoegert bei JEDEM SIGWINCH (wie im allerersten
    Wurf). Neue Funktion `q9_input_read_key_timeout(ms)` -- wie `read_key()`, aber mit
    Zeitschranke (POSIX: `select()` statt blockierendem `read()`; Windows: Busy-Poll mit
    `_kbhit()`, ungetestet mangels Windows-Host). `q9_input_read_key()` ist jetzt nur noch eine
    duenne Huelle um `read_key_timeout(-1)`. Die eigentliche "wann ist Ruhe eingekehrt"-
    Entscheidung liegt damit beim AUFRUFER, nicht mehr in der Bibliothek selbst.
  - `integration_demo.c`: neuer Zustand `showing_overlay`. Jedes `Q9_KEY_RESIZE` schaltet SOFORT
    ins Overlay (`render_size_overlay()`, zentriert "R Rows - C Columns") und zeichnet es LIVE bei
    jedem weiteren Zwischenschritt neu -- kein Warten, keine Verzoegerung. Solange das Overlay
    aktiv ist, wird mit `q9_input_read_key_timeout(1000)` gewartet: laeuft die Sekunde OHNE
    weiteres Resize ab (`Q9_KEY_NONE`), kommt der volle Inhalt zurueck. Ist das Fenster dabei
    (weiterhin) kleiner als die Mindestgroesse, bleibt das Overlay DAUERHAFT stehen (plus
    "Fenster zu klein (mind. NxM)"-Zusatzzeile) -- ersetzt die fruehere separate
    `render_too_small()`-Anzeige vollstaendig, beide Faelle (aktives Resize / dauerhaft zu klein)
    nutzen jetzt dieselbe einfache Darstellung. Andreas' Frage "kannst du es dann auf die
    Mindestgroesse setzen?" so interpretiert: es wird NICHTS auf eine feste Groesse geklemmt
    (technisch bliebe das echte Fenster ja ohnehin so klein), sondern das Overlay bleibt einfach
    stabil sichtbar, bis von Hand wieder vergroessert wird -- falls das nicht die gemeinte
    Interpretation war, bitte zurueckmelden.
  - Verifiziert per `expect`/Pseudo-Terminal, inkl. Zeitmessung: Overlay erscheint sofort bei
    Resize, aktualisiert sich live bei einem zweiten Resize waehrend des Ziehens (ohne
    Zwischenwartezeit), und die Vollansicht kehrt exakt nach ~1000ms Stille zurueck (gemessen:
    1000ms). Zu-klein-Fall zeigt Groesse + Zusatzzeile wie vorgesehen.

**Vierte Feedback-Runde (2026-08-17) -- Andreas' Frage von oben genauer geklaert + weitere
Wuensche, alles umgesetzt:**
- **"Kannst du es dann auf die Mindestgroesse setzen?" -- jetzt genauer verstanden:** gemeint war
  ein ECHTER Versuch, das TERMINAL-FENSTER SELBST automatisch zu vergroessern (nicht nur eine
  interne Darstellungsentscheidung). Das geht tatsaechlich -- ueber die XTWINOPS-Escape-Sequenz
  `ESC[8;{rows};{cols}t`, die manche Terminals (v.a. xterm mit aktivierten "Window Ops") als
  Aufforderung verstehen, sich selbst auf die angegebene Zeichen-Groesse zu bringen. NEU:
  `q9_ansi_resize_window()` (`q9_ansi.h/.c`). `integration_demo.c`: sobald das Overlay wegen
  Unterschreitung der Mindestgroesse laenger als `RESIZE_SETTLE_MS` stabil bleibt, wird EINMAL
  (nicht bei jeder Wiederholung) versucht, das Terminal auf `MIN_ROWS`x`MIN_COLS` zu bringen.
  BEKANNTER KOMPROMISS: nicht universell unterstuetzt -- viele Terminals ignorieren die Sequenz
  oder haben sie aus Sicherheitsgruenden abgeschaltet (ein Programm, das beliebigen Text ausgibt,
  koennte sonst ungefragt fremde Fenster verschieben/resizen). Wirkt es, kommt ganz regulaer ein
  neues `Q9_KEY_RESIZE` mit der dann tatsaechlichen Groesse; wirkt es nicht, bleibt das Overlay
  unveraendert stehen -- kein Fehler, keine Endlosschleife von Versuchen.
- **Beobachtung zu einem harten 48-Spalten-Minimum:** Andreas bemerkte, dass sich sein Terminal
  per Maus nicht unter 48 Spalten ziehen laesst (Zeilen dagegen bis auf 1 zusammenschiebbar). Das
  ist sehr wahrscheinlich eine Eigenschaft des TERMINAL-PROGRAMMS SELBST (z.B. eine Mindestfenster-
  breite in den Einstellungen von Terminal.app/iTerm2/etc.), NICHT unseres Codes -- unser eigenes
  Minimum liegt bei 60 Spalten, unabhaengig davon. Welche genaue Einstellung dafuer verantwortlich
  ist, haengt vom jeweils verwendeten Terminal-Programm ab (nicht ermittelt, da unbekannt welches
  Andreas nutzt) -- ausserhalb dessen, was dieses Projekt beeinflussen kann.
- **Overlay-Reihenfolge getauscht:** "R Rows - C Columns" -> "C Columns - R Rows" (Andreas: "Und
  Columns und Rows solltest du bitte tauschen") -- entspricht der ueblichen "80x24"-Schreibweise
  (Spalten x Zeilen). Die "Fenster zu klein"-Zusatzzeile zeigt die Mindestgroesse jetzt ebenfalls
  in dieser Reihenfolge (`mind. 60x20` statt `mind. 20x60`).
- **Kopfzeile linksbuendig statt zentriert:** ab Spalte 3 (Andreas: "lass uns mal links versuchen,
  ab dem dritten Zeichen"), passt zur bestehenden Linksbuendigkeit von Listenansicht/Hinweistext.
  Zusaetzlich das Weiss der Kopfzeilen-Schrift heller gestellt (naeher an reinem Weiss, noch
  leicht warm getoent, s. `PAL_HEADER_FG_*`).
- Verifiziert: `make test` 0 Fails (neuer Test fuer `q9_ansi_resize_window` in `ansi_selftest.c`).
  Echter Rauchtest per `expect`/Pseudo-Terminal: "Columns - Rows"-Reihenfolge im Overlay bestaetigt,
  Kopfzeile beginnt nachweislich an Spalte 4 (= 0-indiziert Spalte 3, drei Leerzeichen davor), und
  die XTWINOPS-Sequenz `ESC[8;20;60t` erscheint nachweislich im Ausgabestrom nach Ablauf der
  Ruhephase bei anhaltend zu kleinem Fenster (Byte-Vergleich der erzeugten Escape-Sequenz -- ob ein
  ECHTES Terminal darauf tatsaechlich reagiert, kann von hier aus nicht geprueft werden, das muss
  Andreas selbst an seinem Terminal sehen).

**Fuenfte Runde (2026-08-17) -- der eigentliche Datei-Auswahl-Dialog, IN ARBEIT:** Andreas testete
per SSH von Windows 11/Windows Terminal auf den Mac (laeuft technisch also weiterhin ueber den
POSIX-Zweig, unabhaengig vom lokalen Windows-Terminal). Rueckmeldung: Grundaussehen bleibt so,
aber die Listenansicht "braucht noch ein paar Optionen" (noch nicht konkretisiert) und "unbedingt"
ein Datei-Auswahl-Dialog. Design abgestimmt (Vorschlag per ASCII-Mockup + `AskUserQuestion`
bestaetigt, dann von Andreas verfeinert):
- Kopfzeile wie die Hauptseite, oben rechts ein `[X]` als visuelle Geste (mangels Maus nicht
  eigenstaendig klickbar, Escape schliesst ohnehin immer, unabhaengig vom Fokus)
- Dateiliste mit ECHTEN Dateien (nicht nur Namen) -- Spalten Name/Datum/Groesse
- Auswahlzeile ("Ausgewaehlt: ...") + rechts daneben ein KOMPAKTER Extensions-UMSCHALTER (kein
  Freitext-Filterfeld -- zyklisch durch eine feste Liste vorgegebener Endungen + `*.*`)
- OK/Abbrechen-Buttons unten auf der Hauptfarbe (nicht der Listenfarbe)
- Rahmenlos -- nur die dunklere (aber klar von Schwarz unterscheidbare) Hintergrundfarbe grenzt ab
- Navigation: TAB vorwaerts / Shift-TAB rueckwaerts zyklisch durch Liste -> Extensions-Umschalter
  -> OK -> Abbrechen; Pfeiltasten navigieren INNERHALB der Liste, wenn sie den Fokus hat; Enter auf
  der Liste = OK; Escape = immer Abbrechen
- **Bewusst vertagt (Andreas: "faellt mir gerade ein... aber machen wir es erst mal ohne"):**
  Anzeige des aktuellen Verzeichnispfads im Dialog -- soll spaeter OPTIONAL moeglich sein.
  **FERTIG (2026-08-30, Sechsunddreissigste Runde):** rechtsbuendig auf der Kopfzeile, vor dem
  "X" -- Titel hat Vorrang, bei zu wenig Platz bleibt der Pfad einfach weg statt zu ueberlappen.

**Bisher fertig:**
- `q9_input.h/.c`: `Q9_KEY_SHIFT_TAB` (CBT, `ESC[Z`) fuer die rueckwaertige Fokus-Navigation.
  Ruecklese-Test bestaetigt, dass `decode()` die Sequenz korrekt erkennt -- ob reale Terminals
  beim Druecken von Shift-Tab tatsaechlich genau diese Sequenz senden, ist (wie bei allen echten
  Tastatureingaben) nur am echten Terminal durch Andreas selbst pruefbar.
- `q9_filelist.h/.c` (NEU): echter Verzeichnis-Scan (nur das direkte Verzeichnis, keine
  Unterordner-Navigation -- der Aufrufer gibt das Verzeichnis fest vor), liefert Name/Datum/
  Groesse (kompakte Kurzform, z.B. "3.0M") je Datei, alphabetisch sortiert, Erweiterungsfilter
  case-insensitiv mit/ohne fuehrenden Punkt, `*`/`*.*`/leer = kein Filter, versteckte Dateien
  werden uebersprungen. POSIX (opendir/readdir/stat) implementiert+getestet (21 Checks, echtes
  Scratch-Verzeichnis mit echten Testdateien), Windows (FindFirstFile/FindNextFile) geschrieben,
  mangels Windows-Host ungetestet.
- `q9_filedialog.h/.c` (NEU): die eigentliche Dialog-Zusammensetzung -- Kopfzeile+`X` (dekorativ),
  Spaltentitel-Zeile, Dateiliste (`q9_listview`, Zeilen vorformatiert zu einem Spalten-String, da
  `q9_listview_render` nur ein flaches String-Array kennt), Auswahl-/Filterzeile mit Extensions-
  Umschalter, OK/Abbrechen-Buttons. TAB/Shift-TAB zyklisch durch Liste -> Filter -> OK -> Abbrechen
  -> (wieder Liste), Pfeiltasten nur innerhalb des jeweils fokussierten Elements wirksam, Enter auf
  Liste/OK = Bestaetigen (nur wenn eine Datei ausgewaehlt ist), Escape = immer Abbrechen,
  unabhaengig vom Fokus. Rahmenlos, nur ueber eine eigene Hintergrundfarbe (Palette komplett vom
  Aufrufer uebergeben, s. `q9_filedialog_palette_t`) vom Rest des Bildschirms abgegrenzt. Rein
  logisch getestet (Fokus-/Filter-Zyklus, Escape/Enter-Verhalten, "eingefroren nach Entscheidung",
  echtes Scratch-Verzeichnis fuer den Filter-Rescan) -- das tatsaechliche Bildschirmbild selbst ist
  wie bei `q9_widgets`/`q9_listview` kein automatisierter Test, sondern erst im Integrations-Demo
  sichtpruefbar.
- `demo/integration_demo.c`: Taste `O` oeffnet den Dialog zentriert ueber dem Bildschirm (scannt
  `.`, das Arbeitsverzeichnis der Demo selbst, mit Beispielfiltern `*.*`/`.c`/`.h` -- reine
  Vorfuehrung, keine echte Config-Anbindung). Ergebnis (gewaehlte Datei bzw. Abbruch) ersetzt bis
  zur naechsten Dialog-Oeffnung den unteren Hinweistext. DEMO-GRENZE (bewusst, s. Kommentar in
  `run_file_dialog()`): ein Resize WAEHREND der Dialog offen ist, wird ignoriert (kein Nachziehen
  der Dialog-Geometrie) -- ein echter Editor muesste hier neu snapshot/restore + re-initialisieren.
  Echter Pseudo-Terminal-Rauchtest (expect, wie bei den fruehen Bausteinen dieser Session): Dialog
  oeffnen, Escape -> Abbruch, TAB-Zyklus bis Abbrechen + Enter -> Abbruch, Enter direkt auf der
  Liste -> Bestaetigung mit ECHTEM Dateinamen (alphabetisch erste Datei im Scan-Verzeichnis) --
  alle Pruefungen bestanden.
- **Damit ist task #20/#22 abgeschlossen** -- der Datei-Auswahl-Dialog ist fertig und im Demo
  sichtbar/bedienbar. **Noch offen** bleiben die von Andreas noch nicht konkretisierten "paar
  Optionen" fuer die Listenansicht (kein eigener Task bisher, s. Runde 5 oben) -- die vertagte
  Anzeige des aktuellen Verzeichnispfads im Dialog ist seit der Sechsunddreissigsten Runde
  (2026-08-30) FERTIG, s.o.

**Sechste Runde (2026-08-17) -- zwei Bugs aus Andreas' erstem echten Test:**
1. *"Auf volle Groesse hatte ich mir den jetzt nicht vorgestellt"* -- der Dialog WAR immer schon
   nur `DIALOG_ROWS x DIALOG_COLS` (14x50) gross, sah aber wie Vollbild aus: `run_file_dialog()`
   fuellte vorher den KOMPLETTEN Bildschirm mit der Dialog-Hintergrundfarbe, statt den echten
   Hauptbildschirm dahinter stehen zu lassen. Fix: `render_full_content()` in `build_full_content()`
   (baut nur in einen Puffer, kein stdout) + duennen Ausgabe-Wrapper aufgeteilt; `run_file_dialog()`
   nutzt `build_full_content()` jetzt als echten Hintergrund, der Dialog selbst ueberschreibt (via
   `q9_filedialog_render()`) weiterhin nur sein eigenes Rechteck -- der Hauptbildschirm ist jetzt
   sichtbar rundherum, per Rauchtest bestaetigt ("Netz-Terminal"-Eintrag bleibt hinter dem Dialog
   sichtbar).
2. *"wenn der Selektor auf die Dateiauswahl steht sehe ich nichts, es aendert sich jedenfalls
   nichts"* -- die markierte Zeile in der Dateiliste sah IMMER gleich aus, egal ob die Liste den
   Fokus hatte oder nicht; ein Fokuswechsel auf/von der Liste war dadurch unsichtbar (im Gegensatz
   zu Filter/OK/Abbrechen, die schon eine eigene Fokus-Hervorhebung hatten). Fix: neues Farbpaar
   `unfocus_sel_fg/bg` in `q9_filedialog_palette_t` -- die markierte Zeile bekommt die kraeftige
   `sel_fg/bg`-Hervorhebung nur noch, WENN die Liste tatsaechlich den Fokus hat, sonst eine
   gedaempfte Variante (im Demo: gleicher Ton wie die Spaltentitel-Zeile).

Beide Fixes in `q9_filedialog.h/.c` Ver. 1.10 und `integration_demo.c` Ver. 1.70, `make test`
weiterhin komplett gruen.

**Siebte Runde (2026-08-17) -- vier weitere Wuensche nach dem zweiten Test:**
1. *"Hintergrundfarbe im unteren Bereich noch mal aendern, ab da wo die Dateitabelle aufhoert...
   so dass sich der untere Teil etwas absetzt"* -- neuer eigenstaendiger Fussbereich (Auswahl-/
   Filterzeile + Buttons) mit eigener `footer_fg/bg`-Farbe in `q9_filedialog_palette_t`, deutlich
   sichtbar anders als die Dateiliste, bleibt aber in der Amber-/Braun-Farbfamilie.
2. *"stell den Pfad bitte mal auf das ~ Verzeichnis"* -- `run_file_dialog()` scannt jetzt
   `getenv("HOME")` statt `.` (Ruecksprung auf `.` falls `HOME` nicht gesetzt ist).
3. *"OK und Abbrechen mehr wie Buttons aussehen lassen... mit der Farbe der zweiten Zeile"* +
   *"Zeichen die etwas ein halbes Zeichen gross sind... den Button einmal nach oben und einmal
   nach unten vergroessern... gleichlang... rechts anordnen"* -- neue `q9_screenbuf`-Glyphen
   `Q9_GLYPH_UPPER_HALF`/`LOWER_HALF` (▀/▄, klassischer Halbblock-Button-Trick: eine Zeile ueber
   dem Button zeigt in der UNTEREN Haelfte die Button-Farbe, eine Zeile darunter in der OBEREN
   Haelfte -- der Button wirkt dadurch anderthalb Zeilen hoch). Buttons jetzt in der Spaltentitel-
   Farbe (`sub_fg/bg`, wie gewuenscht), gleich breit (`draw_button()`), rechtsbuendig im
   Fussbereich statt links.
4. *"Gibt es einen Pfeil nach unten fuer den Filter? Und da drauf bekommt man die komplette
   Auswahl des Filters"* -- Rueckfrage per `AskUserQuestion` ergab: RICHTIGES Aufklapp-Menue
   gewuenscht (nicht nur ein neues Zeichen). Neue Glyphe `Q9_GLYPH_DOWN_ARROW` (▾) ersetzt das
   bisherige `>`. Enter ODER Pfeil-runter auf dem Filter oeffnet jetzt `filter_popup_open` -- eine
   kleine Liste ALLER Filter (waechst nach oben in den Bereich der Dateiliste, da unterhalb kein
   Platz mehr ist), Pfeil hoch/runter navigiert DARIN, Enter uebernimmt + rescanned, Escape
   schliesst NUR das Popup (nicht gleich den ganzen Dialog -- "oberste Ueberlagerung zuerst").
   Pfeil links/rechts bleibt als schneller Einzel-Schritt erhalten (ohne Popup zu oeffnen).

`q9_screenbuf.h/.c` Ver. 1.20 (drei neue Glyphen), `q9_filedialog.h/.c` Ver. 1.20 (`layout_rows()`
als gemeinsame Geometrie-Quelle fuer init()/render(), da der Fussbereich jetzt 4 statt 3 Zeilen
braucht), `integration_demo.c` Ver. 1.80 (Dialoggroesse 17x54 statt 14x50 fuer den groesseren
Fussbereich). Neue Selbsttest-Faelle fuer das Aufklapp-Menue (oeffnen/navigieren/uebernehmen/nur-
Popup-schliessen), `make test` weiterhin komplett gruen, per Rauchtest bestaetigt (Halbblock-
Glyphen UND Dropdown-Pfeil tatsaechlich im ANSI-Output nachgewiesen).

**Achte Runde (2026-08-17) -- Bildlaufleiste/Rahmen-Umbau, sechs Wuensche:**
1. *"Die Bildlaufleiste wird jetzt der ganz rechte Strich... wird keine Laufleiste benoetigt ist es
   einfach der normale Strich... im Hauptfenster sowie im Dialog"* -- `q9_listview.c` zeichnet die
   rechte Spalte jetzt IMMER als Linie (`Q9_GLYPH_VLINE`), der Griff (`Q9_GLYPH_BLOCK`) kommt nur
   ZUSAETZLICH dazu, wenn tatsaechlich etwas zu scrollen ist. Ersetzt die fruehere "-2 nur bei
   Scrollbedarf"-Sonderregel -- gilt automatisch fuer Hauptfenster UND Dialog (beide nutzen
   `q9_listview_render`).
2. *"Im Dialog haben wir links dann auch einen einfachen Strich zwischen der Kopfzeile und der
   neuen Statuszeile"* -- `q9_filedialog.c` zeichnet jetzt selbst eine LINKE Linie (q9_listview
   kennt nur seine eigene rechte Spalte), spannt exakt die Listenhoehe (nicht den Fussbereich).
3. *"Die Buttons und der Dateifilter wandern etwas nach links damit Platz fuer den Rahmen/
   Bildlaufleiste ist"* -- Filter UND Buttons enden jetzt buendig mit der rechten Linie der
   Dateiliste (`right_border_col`), nicht mehr am absoluten Dialogrand. Das Filter-Aufklapp-Menue
   richtet sich ebenfalls danach aus.
4. *"Hinter 'Datei:' wo der Name erscheint, sollte die Hintergrundfarbe noch mal abgesetzt sein,
   als Kaestchen fuer den Namen"* -- der Namenswert bekommt jetzt ein eigenes `sub_fg/bg`-Kaestchen
   (dieselbe Farbe wie die Spaltentitel-Zeile), statt nur Text auf dem Fussbereich-Hintergrund.
5. *"Die Ueberschrift mit Name Datum Groesse sollte genau breit wie die Tabelle selber sein"* --
   die Spaltentitel-Zeile spannt jetzt exakt von der linken bis zur rechten Linie (nicht mehr die
   volle Dialogbreite).
6. *"Groesse sollte ganz rechts stehen, Datum links daneben, und der Dateiname dann so lang wie
   der Rest"* -- neues Feld `name_col_width` in `q9_filedialog_t`, EINMAL in `init()` aus der
   tatsaechlichen Listenbreite berechnet (Rest nach Bildlaufleiste/Trennzeichen/Datum/Groesse) --
   ersetzt die feste `Q9_FILEDIALOG_NAME_COL`-Konstante. Reine Spaltenreihenfolge (Name/Datum/
   Groesse links nach rechts) war schon vorher so, nur die Breiten waren fest statt dynamisch.

`q9_listview.h/.c` Ver. 1.20/1.30, `q9_filedialog.h/.c` Ver. 1.30 (neues Feld `name_col_width`,
`Q9_FILEDIALOG_NAME_COL` entfernt zugunsten dynamischer Berechnung). Ein Selbsttest in
`listview_selftest.c` musste angepasst werden (pruefte vorher "Spalte bleibt leer ohne Scrollbedarf"
-- jetzt "Spalte zeigt die Linie ohne Scrollbedarf"). `make test` komplett gruen. Sichtpruefung per
`pyte` (Terminal-Emulator-Bibliothek, einmalig fuer diese Runde installiert) bestaetigt exakte
Spaltenausrichtung: "Name" beginnt in derselben Spalte wie die Dateinamen darunter, "Groesse" endet
eine Spalte vor der Bildlaufleiste, Buttons/Filter enden buendig mit der rechten Linie, das
Namens-Kaestchen hat exakt `name_col_width` Spalten in `sub_bg`-Farbe.

**Neunte Runde (2026-08-17) -- Bildlaufleiste/Rahmenkante verschmelzen:**
*"Im Hauptfenster haben wir immer noch rechts die Fensterkante und zusaetzlich die Bildlauf-
leiste, das soll jetzt in einem sein, die Linie ganz aussen. Genauso beim Dialog, bei
Rahmenkanten kommen nach ganz aussen, und rechts zusaetzlich die Bildlaufleiste auf den
gleichen Strich wenn noetig. Die Tabelle bleibt aber so, der Strich geht dann allerdings ab
dem Header."* -- zwei getrennte Linien mit Luecke dazwischen (Fensterrahmen UND separate
Bildlaufleiste innen) sollten zu EINER verschmelzen:
- **Hauptfenster**: `integration_demo.c` -- die Listenbreite reicht jetzt bis zur rechten
  Rahmenkante von `draw_frame()` (`lv->width = cols - lv->col` statt `cols - 6`). `q9_listview`s
  eigene Linie/Bildlaufleiste liegt dadurch direkt AUF der Fensterkante, keine Luecke mehr.
- **Dialog**: `q9_filedialog.c` -- keine eigenen Randspalten mehr (`list.col = dlg->col + 1`,
  `list.width = dlg->cols - 1` statt vorher `+2`/`-3`), die Linien liegen jetzt GENAU auf der
  Dialogkante. "Der Strich geht ab dem Header": der Rahmen (links selbst gezeichnet, rechts von
  `q9_listview` UND fuer die Spaltentitel-Zeile zusaetzlich von diesem Modul) beginnt jetzt schon
  bei der Spaltentitel-Zeile, nicht erst bei der ersten Listenzeile. "Die Tabelle bleibt so": nur
  die Rahmen-/Randgeometrie hat sich verschoben, die Spaltenlogik (Name/Datum/Groesse,
  `name_col_width`) blieb unangetastet und passt sich automatisch an die neue (etwas breitere)
  Listenbreite an.

`q9_listview.h/.c` unveraendert (die "immer eine Linie"-Logik aus Runde acht traegt bereits),
`q9_filedialog.h/.c` Ver. 1.40, `integration_demo.c` Ver. 1.90. `make test` weiterhin komplett
gruen. Sichtpruefung per `pyte` bestaetigt: Hauptfenster zeigt nur noch EINE Linie rechts (vorher
zwei mit Luecke), Dialog-Rahmen beginnt exakt bei der Spaltentitel-Zeile und liegt an beiden
Seiten direkt auf der Dialogkante, Buttons-Hintergrund reicht bis genau dorthin.

**Zehnte Runde (2026-08-17) -- "wirkt jetzt doch gequetscht", sechs Wuensche:**
*"bitte zum Strich jeweils ein Leerzeichen. Der Tabellenkopf bitte rechts und links je ein Zeichen
schmaler machen, inkl. Tabellenkopf, da soll noch die dunklere Farbe vom Hintergrund sein. Die
beiden Buttons je rechts und links ein Zeichen schmaler und eine Reihe hoeher, links Platz fuer
den Strich schaffen. Datei mit dem Namen dahinter reicht nur ein Zeichen bis vor dem Button,
dadrunter Filter: und die Filterauswahl. Komplett um unteren Rand des Dialogs auch eine
Statuszeile wie im Hauptfenster und die Seitenstriche gehen bis zur Statusleiste."* -- nachdem in
der letzten Runde alle Randspalten entfielen, wirkte der Inhalt zu eng an den Linien. Sechs
zusammenhaengende Aenderungen:
1. **Luftspalte vor der Linie** -- `q9_listview.c`s `content_width` jetzt `width-2` statt
   `width-1` (Linie bleibt an derselben Stelle, nur der Inhalt bekommt mehr Abstand); im Dialog
   auf der linken Seite spiegelbildlich von Hand nachgebaut (`list.col = dlg->col + 2` statt `+1`).
   Wirkt automatisch auch im Hauptfenster mit (dort unproblematisch, da die Linienposition
   unveraendert bleibt).
2. **Spaltentitel-Zeile** -- der Text ruckt automatisch mit der neuen Luftspalte mit; die
   Hintergrundfarbe (`sub_bg`) spannt weiterhin die volle Dialogbreite (beruehrt also weiterhin
   beide Linien farblich, wie gewuenscht).
3. **Buttons** -- 1 Zeichen schmaler je Seite (`button_w` jetzt `strlen("Abbrechen")+2` statt
   `+4`), rechtsbuendig mit der neuen Luftspalte vor der rechten Linie.
4. **Filterzeile jetzt eigene Zeile** -- vorher rechtsbuendig auf derselben Zeile wie "Datei:",
   jetzt darunter mit eigener Beschriftung "Filter: ". Beide Beschriftungen ("Datei:"/"Filter:")
   auf `Q9_FILEDIALOG_LABEL_WIDTH` (8 Zeichen) aufgefuellt, damit ihre Werte (Namens-Kaestchen und
   Filter-Chip) untereinander in derselben Spalte anfangen.
5. **Neue Statuszeile ganz unten** -- volle Dialogbreite, eigene Farbe (dieselbe wie die
   Kopfzeile, fuer ein symmetrisches Erscheinungsbild), zeigt einen Tastatur-Kurzhinweis.
6. **Rahmenlinien reichen jetzt bis zur Statuszeile** -- vorher endeten sie am unteren Rand der
   Liste, jetzt laufen sie durch den kompletten Fussbereich (Namens-/Filterzeile, Halbblock-
   Kappen, Buttons) und enden erst direkt VOR der neuen Statuszeile. Dafuer wird der Rahmen jetzt
   ganz am Ende von `q9_filedialog_render()` gezeichnet (nach allen Hintergrund-Fuellungen), damit
   die Linie ueberall die richtige Hintergrundfarbe der jeweiligen Zeile "erbt" (puts() laesst den
   Hintergrund einer Zelle unangetastet).

**Ein Bug beim ersten Versuch, per pyte gefunden und noch vor dem Commit behoben:** die neuen
Fussbereich-Zeilen (`datei_row` etc.) waren in `layout_rows()` falsch berechnet und ueberlappten
mit den letzten beiden Listenzeilen -- die rechte Randlinie fehlte dadurch genau auf der
Namens-/Filterzeile (die Ausschluss-Logik "das macht schon q9_listview" griff faelschlich auch
dort). Korrigiert, mit pyte erneut ueberprueft.

`q9_listview.h/.c` Ver. 1.30/1.40 (Luftspalte), `q9_filedialog.h/.c` Ver. 1.50 (neue Statuszeile,
Filterzeile, durchgehende Rahmenlinien, neue Konstante `Q9_FILEDIALOG_LABEL_WIDTH`),
`integration_demo.c` Ver. 2.00 (`DIALOG_ROWS`/`_COLS` vergroessert: 20x56 statt 17x54, fuer die
zwei zusaetzlichen Fusszeilen plus etwas mehr Breite). `make test` komplett gruen. Sichtpruefung
per pyte bestaetigt: durchgehend 1 Zeichen Abstand zu beiden Linien, "Datei:"/"Filter:"-Werte exakt
in derselben Spalte, Buttons enden mit Luecke vor der Linie, neue Statuszeile farblich korrekt auf
die Dialogbreite begrenzt.

**Elfte Runde (2026-08-17) -- "Fast gut :-)", sechs weitere Wuensche:**
1. *"Ich wollte noch zwei Zeilen sparen, die beiden Buttons OK und Abbrechen kommen direkt unter
   die Dateitabelle, der Dateiname wird dann gekuerzt und ggf. mit den ... dargestellt. Genauso
   wird es oben in der Tabelle gemacht, wenn der Name zu lange ist."* -- Rueckfrage per
   `AskUserQuestion` ergab: "Filter bleibt so ist kurz genug", die Buttons wandern zwei Zeilen nach
   oben und TEILEN SICH die Namens-/Filterzeile mit ihren Halbblock-Kappen bzw. dem Button-Text
   selbst (rechtsbuendig, waehrend "Datei:"/"Filter:" links stehen) -- neue Funktion
   `truncate_ellipsis()` kuerzt Namen mit "..." statt hart abzuschneiden, sowohl im Namens-Kaestchen
   (jetzt schmaler, da es der oberen Halbblock-Kappe von OK ausweichen muss) als auch in der
   Tabelle selbst.
2. *"Die Zeichen mit den beiden senkrechten Strichen bekommen als Hintergrundfarbe die dunklere aus
   der Zeile darunter"* -- die Rahmenlinien-Zeichen (auf der Spaltentitel-Zeile und im
   Fussbereich, von diesem Modul selbst gezeichnet) bekommen jetzt IMMER `body_bg` (die dunkelste
   Farbe) als eigenen Zellhintergrund statt die jeweilige Zeilenfarbe zu erben -- dafuer `fill_rect`
   statt nur `puts()` (das den Hintergrund unangetastet laesst).
3. *"Das X in der oberen Zeile ein Zeichen weiter nach rechts"* -- erledigt.
4. *"Statuszeile, der Text ist zu lang, steht zwei Zeichen ueber, bitte auch dort mit senkrechten
   Strichen teilen"* -- Text gekuerzt ("TAB: weiter │ Enter: OK │ Esc: Abbruch" statt der langen
   Version) und mit `Q9_GLYPH_VLINE` als Trennzeichen zwischen den drei Feldern statt viel
   Leerraum, passt jetzt sauber in die Dialogbreite.
5. *"Warum ist in der Dateiliste einmal ein kompletter Pfad drin?"* -- kein Bug: `q9_filelist_scan`
   listet einfach ehrlich, was im gescannten Verzeichnis (Andreas' `HOME`) liegt; eine seiner
   echten Dateien hat dort tatsaechlich einen pfadartigen Namen als Dateiname (kein rekursiver
   Scan, keine Interpretation).
6. *"Du kannst gerne Umlaute verwenden... oder ist das ein Problem?"* -- ist aktuell ein Problem:
   `q9_screencell_t.ch` ist bewusst EIN Byte pro Zelle (s. `q9_screenbuf.h`), Umlaute sind
   mehrbytige UTF-8-Sequenzen wie die Box-Drawing-Zeichen -- bruechen ohne eigene
   `Q9_GLYPH_*`-Sentinels (analog zu ▾/▀/▄) das Rendering. Bisher bewusst ASCII-Transliteration
   (oe/ae/ue/ss) zur Vermeidung -- Umlaut-Unterstuetzung waere ein separates, ueberschaubares
   Erweiterungsprojekt (7-8 neue Sentinels), aber noch nicht umgesetzt.

`q9_filedialog.h/.c` Ver. 1.60 (Buttons teilen sich Zeilen, `truncate_ellipsis()`, Randlinien mit
`body_bg`, X verschoben, Statuszeile gekuerzt), `integration_demo.c` Ver. 2.10 (`DIALOG_ROWS`
wieder auf 18 verkleinert). `make test` komplett gruen. Sichtpruefung per pyte bestaetigt: Kuerzung
mit "..." funktioniert (echte lange Datei aus `HOME` als Beleg), Randlinien durchgehend in
`body_bg`, "X" eine Spalte weiter rechts, Statuszeile passt jetzt ohne Ueberlauf in die Dialogbreite.

**Zwoelfte Runde (2026-08-17) -- Farbpalette:**
*"Die ganzen Farben sind jetzt alle so in Richtung Braun abgerutscht, kannst du das bitte noch
etwas mehr in Richtung gelb orange bringen? Der Tabellenhintergrund bitte noch etwas dunkler, der
Tabellenkopf und der untere Teil bitte etwas mehr Kontrast... falls man das ueberein bringen
kann."* -- die komplette Palette (Hauptfenster UND Dialog) wurde auf eine durchgehende Farbleiter
umgerechnet: EIN Farbton (~42 Grad, Orange-Gelb) und EINE hohe Saettigung (~82%) fuer alle Toene,
nur die HELLIGKEIT (V) unterscheidet die Ebenen -- vorher waren es unterschiedlich helle
BRAUNTOENE (aehnlicher Farbton, aber niedrigere/wechselnde Saettigung), jetzt eine klare Leiter:

| Ebene | V | Beispiel |
|---|---|---|
| `PAL_DIALOG_BODY_BG` (Tabellenhintergrund) | 0.15 | dunkelster Schritt, "noch etwas dunkler" |
| `PAL_DIALOG_FOOTER_BG` / `PAL_STATUS_BG` ("der untere Teil") | 0.32 | |
| `PAL_DIALOG_SUB_BG` (Tabellenkopf, Button-Grundfarbe) | 0.44 | deutlicher Sprung zu beiden Nachbarn |
| `PAL_FRAME` (Rahmenlinien + Text auf SUB_BG/STATUS_BG) | 0.72 | |
| `PAL_HEADER_BG` | 0.80 | |
| `PAL_SEL_BG` (Auswahl-Hervorhebung) | 0.90 | hellster, kraeftigster Ton |

`integration_demo.c` Ver. 2.20 (reine Farbkonstanten-Aenderung, keine Geometrie/Logik betroffen).
`make test` komplett gruen (erwartungsgemaess unveraendert). Sichtpruefung per pyte bestaetigt alle
neuen Hex-Werte exakt an den erwarteten Stellen (Tabellenhintergrund, Spaltentitel-Zeile,
Fussbereich, Kopfzeile, Auswahl-Hervorhebung, Fliesstext).

**Dreizehnte Runde (2026-08-17) -- Kontrast + ein echter Bug:**
*"Zwei Sachen sind jetzt noch schlecht zu lesen... in den beiden Kopfzeile das weiss auf dem
hellen Gelb. Vielleicht die Schrift dunkler machen und im Dialog, im Tabellenheader und unter der
Tabelle 'Name Datum Groesse' zu wenig Kontrast und '<Dateiname>', OK und Abbrechen zu wenig
Kontrast. Kannst du bitte noch den Footer des Dialogs in der Farbe des Hauptfensters machen...
und mir faellt gerade auf, die Striche links und rechts am Hauptfenster sind unterschiedlich...
das oberste rechts ist noch mal anders."*

1. **Kopfzeilen-Text dunkler** -- `PAL_HEADER_FG` (beide Kopfzeilen, Hauptfenster UND Dialog
   teilen sich diese Konstante) war fast-weiss auf dem jetzt kraeftigeren `PAL_HEADER_BG` (V=0.80)
   -- schlechter Kontrast. Jetzt so dunkel wie `PAL_DIALOG_BODY_BG` (V=0.15).
2. **Tabellenkopf/Namens-Kaestchen/Buttons mehr Kontrast** -- alle drei nutzten `PAL_FRAME`
   (V=0.72) als Text auf `PAL_DIALOG_SUB_BG` (V=0.44) -- zu nah beieinander. `PAL_FRAME` selbst
   bewusst NICHT geaendert (dient auch als Rahmenlinienfarbe UND Hinweistext, haette dort
   unerwuenschte Nebenwirkungen) -- stattdessen neue eigene Konstante `PAL_DIALOG_SUB_FG` (fast-
   weiss, der alte `PAL_HEADER_FG`-Wert, durch Punkt 1 frei geworden) nur fuer diese Rolle.
3. **Dialog-Footer in Hauptfenster-Farbe** -- `PAL_DIALOG_FOOTER_BG` traf zufaellig schon
   `PAL_STATUS_BG` (beide V=0.32, gleiche Zahlen) -- jetzt ueber die Konstante selbst referenziert
   (`#define PAL_DIALOG_FOOTER_BG_R PAL_STATUS_BG_R` usw.), damit das strukturell garantiert
   bleibt statt zufaellig zu sein.
4. **Echter Bug, kein Farbwunsch**: die Rahmenlinien im Hauptfenster waren links (`q9_widgets.c`
   `draw_frame()`, durchgehend `PAL_FRAME`) und rechts (fuer Kopf-/Statuszeile ebenfalls
   `draw_frame()`, aber fuer den Listenbereich selbst `q9_listview_render()`, das bisher die
   normale Text-fg -- `PAL_LIST_FG` -- fuer die Linie mitverwendete) UNTERSCHIEDLICH eingefaerbt,
   obwohl beide auf derselben Bildschirmspalte liegen. Loesung: `q9_listview_render()` bekommt
   einen neuen, von der Text-fg UNABHAENGIGEN Parameter `line_fg` (s. `q9_listview.h/.c`) --
   Hauptfenster uebergibt jetzt `PAL_FRAME` (behebt die Inkonsistenz), der Dialog weiterhin
   `list_fg` (dort gab es den Bug nicht, die Linie war schon immer konsistent gefaerbt).

`q9_listview.h/.c` Ver. 1.40/1.50 (neuer `line_fg`-Parameter, neuer Selbsttest-Fall bestaetigt die
Entkopplung von der Text-fg), `q9_filedialog.h/.c` Ver. 1.70 (Aufruf angepasst, `list_fg` als
`line_fg`), `integration_demo.c` Ver. 2.30 (`PAL_FRAME` als `line_fg`, neue `PAL_DIALOG_SUB_FG`,
`PAL_HEADER_FG` dunkler, `PAL_DIALOG_FOOTER_BG` referenziert `PAL_STATUS_BG`). `make test` komplett
gruen. Sichtpruefung per pyte bestaetigt: Hauptfenster-Rand jetzt durchgehend `PAL_FRAME`-farbig
(vorher zeilenabhaengig unterschiedlich), "Name"/"64K"/Buttons zeigen die neue helle `SUB_FG`-Farbe,
Dialog-Footer und Hauptfenster-Statuszeile exakt derselbe Hex-Wert, Kopfzeilen-Text dunkel auf
hellem Hintergrund.

**Vierzehnte Runde (2026-08-17) -- ein Farbfehler durch die letzte Runde, Statuszeilen-Abgleich,
Resize waehrend offenem Dialog:**
*"die Schrift beim Groesse Aendern ist fast nicht mehr zu lesen, viel zu dunkel. Farben sind
ansonsten besser, alles ist gut zu lesen, nur die Statuszeilen sind noch unterschiedlich. Fenster
Groesse aendern waehrend ein Dialog auf ist funktioniert nicht richtig, waere auch gut wenn der
Dialog nach dem Positionieren immer wieder mittig positioniert wird."*

1. **Resize-Overlay unlesbar** -- `render_size_overlay()` (die "R Rows - C Columns"-Anzeige
   waehrend/nach einer Groessenaenderung) nutzte `PAL_HEADER_FG`, das in der letzten Runde bewusst
   DUNKEL wurde (Kontrast auf dem kraeftigen `PAL_HEADER_BG`) -- dort gibt es aber GAR KEINEN
   eigenen farbigen Hintergrund (nur Terminal-Default, typischerweise dunkel), der Text war
   dadurch praktisch unsichtbar. Jetzt `PAL_STATUS_FG` (weiterhin hell, unveraendert).
2. **Statuszeilen angeglichen** -- die Dialog-eigene untere Statuszeile nutzte bisher
   `header_fg/bg` (an die Dialog-Kopfzeile gekoppelt) und sah dadurch anders aus als die
   Hauptfenster-Statuszeile. Neue eigene Felder `status_fg/bg` in `q9_filedialog_palette_t`,
   `integration_demo.c` befuellt sie mit exakt `PAL_STATUS_FG/BG` (denselben Werten wie das
   Hauptfenster).
3. **Resize waehrend offenem Dialog** -- vorher eine dokumentierte "Demo-Grenze" (Resize-Ereignisse
   wurden im Dialog schlicht ignoriert). Jetzt: `run_file_dialog()` bekommt `rows`/`cols` als
   ZEIGER, fragt bei `Q9_KEY_RESIZE` die Terminal-Groesse neu ab und initialisiert den Dialog MIT
   DENSELBEN Filtern/Verzeichnis/Palette neu -- `compute_dialog_geometry()` (neu, aus der
   bisherigen Berechnung ausgelagert) zentriert dabei automatisch neu, erledigt also gleichzeitig
   den zweiten Wunsch ("immer wieder mittig"). Bewusste Demo-Grenze: das Neu-Initialisieren setzt
   Fokus/Auswahl/Filter auf ihre Startwerte zurueck (kein Nachziehen des bisherigen Zustands) --
   ein echter Editor wuerde hier gezielter nur die Geometrie aktualisieren, fuer die Demo ein
   akzeptabler Kompromiss (Resize mitten in der Dateiauswahl ist ein Randfall).

`q9_filedialog.h/.c` Ver. 1.70/1.80 (neue `status_fg/bg`-Felder), `integration_demo.c` Ver. 2.40
(`render_size_overlay()`-Fix, `status_fg/bg`-Befuellung, `run_file_dialog()`-Signatur auf Zeiger
umgestellt + Resize-Behandlung, `compute_dialog_geometry()` neu ausgelagert). `make test` komplett
gruen. Per echtem Pseudo-Terminal-Test bestaetigt: SIGWINCH waehrend offenem Dialog (simulierte
Terminal-Vergroesserung `stty rows 35 columns 120`) -- Dialog zeichnet sich bei der neuen Groesse
neu und zentriert, bleibt bedienbar (Enter bestaetigt weiterhin eine Auswahl), die aktualisierte
Terminal-Groesse kommt korrekt beim Aufrufer an (Hauptfenster-Statuszeile zeigt "Terminal: 35x120"
nach dem Schliessen des Dialogs).

**Fuenfzehnte Runde (2026-08-18) -- Resize waehrend offenem Dialog WIRKLICH behoben (voller
Redraw bei jedem Zwischenschritt), Overlay-Text an fester Position:**
*"ok, das verändern der Fenstergröße geht immer noch nicht wenn ein Dialog offen ist. da wird immer
alles neu gezeichnet. Kannst du den Text für die Fenstergröße beim ändern, but mal auf Position 3,3
setzen, da dürfte das auch nicht mehr so durch die Gegend hüpfen ..."*

1. **"Da wird immer alles neu gezeichnet"** -- der ERSTE Anlauf (vierzehnte Runde) hat bei JEDEM
   einzelnen `Q9_KEY_RESIZE`-Ereignis sofort den KOMPLETTEN Dialog per `q9_filedialog_init()` + vollem
   Redraw neu aufgebaut. Waehrend des Ziehens an der Terminal-Ecke kommen davon viele kurz
   hintereinander -- sichtbar ruckelig/flackernd bei jedem Zwischenschritt, genau Andreas' Beschreibung.
   `run_file_dialog()` nutzt jetzt dasselbe Overlay-Settle-Muster wie `main()` (s. dortiges
   `RESIZE_SETTLE_MS`): waehrend gezogen wird, nur das billige `render_size_overlay()` (kein
   Dialog-/Hintergrund-Redraw); der teure Dialog-Neuaufbau (`compute_dialog_geometry()` +
   `q9_filedialog_init()`, zentriert dabei weiterhin automatisch neu) passiert erst EINMAL, nach
   `RESIZE_SETTLE_MS` Stille.
2. **Overlay-Text an fester Position** -- vorher zentriert (`mid_row`/`mid_col`, haengt von
   rows/cols ab), dadurch sprang der Text bei JEDEM Zwischenschritt an eine andere Bildschirmstelle.
   Neue Konstanten `OVERLAY_ROW`/`OVERLAY_COL` (= 3,3, dieselbe Spalte wie Kopfzeilen-Titel/
   Hinweistext/Listenansicht) -- feste Position, kein Huepfen mehr.

**Fallstrick bei der Verifikation:** ein erster `expect`-Test schien einen echten Regressions-Bug
zu zeigen (Enter bestaetigte nach dem Resize nicht mehr) -- Ursache war aber die TESTMETHODE, nicht
das Programm: `after N` in `expect`-Skripten liest waehrenddessen NICHTS vom Kindprozess. Wenn dessen
PTY-Ausgabepuffer dadurch vollaeuft, blockiert sein naechstes `fflush(stdout)`, bis `expect` wieder
aktiv liest (z.B. bei `expect eof`) -- ein Testartefakt, das reale interaktive Terminals (immer aktiv
lesend) nie zeigen. Per `gettimeofday()`-Zeitstempeln direkt im Code nachgewiesen (Luecke von genau
der Summe der `after`-Wartezeiten zwischen zwei Log-Zeilen). Korrigierter Test liest waehrend der
Wartezeit aktiv weiter (`expect { -timeout N -re "." { exp_continue } timeout {} }`) statt zu
schlafen -- damit bestaetigt: Settle funktioniert korrekt, Dialog baut sich nach Stille wieder auf,
bleibt bedienbar. Andreas hat es anschliessend auch live im echten Terminal bestaetigt ("jetzt geht
es immer").

`integration_demo.c` Ver. 2.50 (`run_file_dialog()` mit eigenem `showing_overlay`-Zustand nach
main()-Vorbild, `render_size_overlay()` mit fester `OVERLAY_ROW/_COL`-Position statt Zentrierung).
`make test` komplett gruen. Per echtem Pseudo-Terminal-Test (aktiv lesend statt `after`) bestaetigt:
Overlay zeigt die Groesse waehrend des Resizes, Dialog baut sich nach `RESIZE_SETTLE_MS` Stille
vollstaendig neu auf, Enter bestaetigt danach weiterhin eine Auswahl. Bestehender
`filedialog_smoke3.exp` erneut gruen (keine Regression).

**Sechzehnte Runde (2026-08-18) -- erweiterbare Listeneintraege:**
*"Jetzt brauchen wir groessere Einträge in der Listbox, ich würde vorschlage erweiterbare Items,
Minimiert haben sie nur ein oder zwei Zeilen und wenn aktiviert, so viele wie sie brauchen."*

Andreas schlug zwei Stile vor (voller Rahmen um den aufgeklappten Eintrag; oder Dialog-Stil mit
Kopf-/Fusszeile + Seitenstrichen) und fragte nach Alternativen -- drei ASCII-Mockups zur Auswahl
vorgelegt (per `AskUserQuestion` mit Vorschau), Andreas' Wahl: die dritte, leichteste Variante (nur
Pfeil-Symbol + Einrueckung + eine Trennlinie danach, kein Rahmen -- spart am meisten Platz).

1. **`q9_listview_item_t`** (NEU, `q9_listview.h`) -- ein Eintrag ist jetzt Kopfzeile (`name`, immer
   sichtbar) + optionale Detailzeilen (`detail_lines`/`detail_count`, nur sichtbar wenn aufgeklappt).
2. **`q9_listview_item_rows()`** (NEU, reine Funktion) -- Zeilenzahl eines Eintrags: 1 zugeklappt/
   nicht erweiterbar, sonst 1 (Kopf) + detail_count (Details) + 1 (Trennlinie).
3. **`q9_listview_scroll_ex()`** (NEU, reine Funktion) -- wie `q9_listview_scroll()`, aber ROW-bewusst
   statt item-bewusst (Eintraege koennen jetzt mehr als eine Zeile brauchen). Garantiert nur, dass die
   KOPFZEILE der Auswahl sichtbar bleibt (bewusste Vereinfachung bei ueberlangen aufgeklappten
   Eintraegen -- kein Versuch, den kompletten Block ins Fenster zu quetschen).
4. **`q9_listview_move_ex()`**/**`q9_listview_render_ex()`** (NEU) -- Bewegen/Zeichnen fuer
   erweiterbare Eintraege; bestehende `q9_listview_scroll()`/`_move()`/`_render()` bleiben
   UNVERAENDERT (der Datei-Dialog nutzt weiterhin die einfachen Varianten, keine Aenderung dort).
5. **`Q9_GLYPH_RIGHT_ARROW`** (NEU, `q9_screenbuf.h`, ▸) -- Pendant zum bestehenden
   `Q9_GLYPH_DOWN_ARROW` (▾, bisher nur beim Filter-Dropdown im Dialog): zugeklappt zeigt ▸, aufgeklappt
   ▾, Eintraege ohne Detailzeilen bekommen kein Symbol (nichts zum Auf-/Zuklappen).
6. **`integration_demo.c`**: `g_items` (reine Namensliste) durch `g_list_items` (Name + zwei
   Detailzeilen je Eintrag, reine Vorfuehrdaten wie der Rest der Demo) ersetzt, neues `g_expanded[]`-
   Array (mehrere Eintraege gleichzeitig aufklappbar, kein Akkordeon), Enter auf der Hauptliste
   klappt den ausgewaehlten Eintrag auf/zu.

`q9_screenbuf.h/.c` Ver. 1.30 (`Q9_GLYPH_RIGHT_ARROW`), `q9_listview.h/.c` Ver. 1.50/1.60 (die vier
neuen `_ex`-Funktionen + `q9_listview_item_t`), `integration_demo.c` Ver. 2.60 (`g_list_items`,
`g_expanded`, Enter-Handler), `listview_selftest.c` Ver. 1.40 (Tests fuer alle vier neuen Funktionen,
inkl. gemischter Zeilenhoehen). `make test` komplett gruen. Per echtem Pseudo-Terminal-Test + pyte-
Sichtpruefung bestaetigt: Aufklappen zeigt Pfeil+Detailzeilen+Trennlinie exakt wie das gewaehlte
Mockup, Zuklappen stellt den Ausgangszustand exakt wieder her, Scrollen mit einem aufgeklappten
Eintrag mitten in einer laengeren, teilweise gescrollten Liste haelt die Auswahl sichtbar und zeigt
eine proportionale Bildlaufleiste (kleines Terminal, 20 Zeilen, Auswahl auf "ROM-Spiegel" per
Pfeiltasten). Bestehender `filedialog_smoke3.exp` (Datei-Dialog, nutzt weiter die einfachen
Funktionen) erneut gruen -- keine Regression.

**Siebzehnte Runde (2026-08-18) -- Kopfzeile eines aufgeklappten Eintrags besser hervorgehoben:**
*"Cool! flüssig zu bedienen. Es fände es gut wenn die Headerzeile noch besonders markiert wird wenn
das item offen ist, so geht sie ein wenig unter ..."*

Ohne Auswahl sah die Kopfzeile eines aufgeklappten Eintrags bisher genauso aus wie jeder andere,
zugeklappte Eintrag -- ging neben den (bewusst gedaempften) Detailzeilen direkt darunter optisch
unter. Neuer Parameter `exp_bg` an `q9_listview_render_ex()`: die Kopfzeile eines aufgeklappten,
aber NICHT ausgewaehlten Eintrags bekommt jetzt einen eigenen, gedaempften Hintergrund (V=0.44 auf
derselben Gelb-/Orange-Leiter, dieselben Zahlen wie `PAL_DIALOG_SUB_BG` im Datei-Dialog, aber als
eigene Konstante `PAL_LIST_EXP_BG` -- Hauptfenster und Dialog bleiben unabhaengige Aufrufer). Ist der
Eintrag ZUSAETZLICH ausgewaehlt, gewinnt weiterhin die staerkere Auswahl-Hervorhebung (`sel_bg`) --
Rangfolge Auswahl > aufgeklappt > normal.

`q9_listview.h/.c` Ver. 1.60/1.70 (neuer `exp_bg`-Parameter), `integration_demo.c` Ver. 2.70 (neue
`PAL_LIST_EXP_BG`-Konstante, an `render_ex()` uebergeben), `listview_selftest.c` Ver. 1.50 (neue
Checks: aufgeklappt+nicht ausgewaehlt zeigt `exp_bg`, ausgewaehlt+aufgeklappt zeigt weiterhin
`sel_bg`). `make test` komplett gruen. Per echtem Pseudo-Terminal-Test + direkter pyte-Farbpruefung
bestaetigt: Kopfzeile von "CF-Interface" (aufgeklappt) zeigt Hintergrund `#705514` (= 112,85,20,
`PAL_LIST_EXP_BG`), NACHDEM die Auswahl per Pfeiltaste weiter auf "Netz-Terminal x1" gewandert ist
(dessen Zeile zeigt korrekt den staerkeren `PAL_SEL_BG`, `#e6ad29`).

Andreas' zweite Frage in derselben Nachricht -- *"Wie komme ich dann in das item rein um dort Werte
zu Ändern?"* -- ist NOCH NICHT umgesetzt: Auf-/Zuklappen zeigt bisher nur STATISCHE Vorfuehrdaten
(reiner Text, keine echten, editierbaren Felder). Feld-Navigation/-Bearbeitung INNERHALB eines
aufgeklappten Eintrags waere der naechste groessere Baustein (eigene Planungsrunde noetig -- Fokus-
Modell zwischen Eintraegen und Feldern, Bearbeitungsmodus je Feldtyp text/enum/hex-Zahl, o.ae.),
noch nicht angefangen.

**Achtzehnte Runde (2026-08-18) -- Feld-Navigation + Text-Bearbeitung (Phase 1 von 4):**
*"Ja, die Markierung ist super... das mit den Pfeilen rechts links ist vielleicht nicht schlecht,
anstatt ENTER kann ich Pfeil rechts drücken, item geht auf, und ich komme auf den erste Eintrag,
dann kann ich nur innerhalb des Item navigieren. Kann dort alles ändern. mit ESC oder Pfeil links
komme ich wieder raus, bei ESC wir das idem auch geschlossen ? Ja ganu wir brauchen Texteingabe,
Dateiauswahl (mit dem Dialog), Numerische Eingabe Dezimal/Hex opt. mit Bereich, Boolean Eingabe...
denke das ist das Mindeste"*

Vier Feldtypen sind das Ziel (Text/Dateiauswahl/Numerisch/Boolean) -- zu gross fuer einen Rutsch,
deshalb bewusst in Phasen: **Phase 1 = Navigation + Text** (dieses Mal), Dateiauswahl/Numerisch/
Boolean folgen als eigene Runden auf demselben Fundament. Die Navigations-Semantik (Rechts/Links/
Esc) hat Claudia vorgeschlagen, Andreas hat sie mit der Frage nach dem ESC-Verhalten bestaetigt.

1. **Datenmodell umgestellt** -- `detail_lines`/`detail_count` (reine Anzeige-Strings) ersetzt durch
   echte Felder: `q9_listview_field_t` (Label konstant, `value` MUTABLE, `Q9_LISTVIEW_FIELD_VALUE_MAX`
   = 40 Zeichen). `q9_listview_item_t.fields` ist bewusst NICHT const (im Gegensatz zum Item-Array
   selbst) -- Werte werden direkt in place veraendert.
2. **Feld-Fokus** -- neues `lv->field_focus` (-1 = Fokus auf der Kopfzeile/Item-Ebene, sonst Index
   ins Feld-Array des AUSGEWAEHLTEN Eintrags). `q9_listview_init()`/`_move_ex()` setzen es defensiv
   zurueck (Feld-Fokus ergibt bei einem anderen Eintrag keinen Sinn).
3. **Navigation** (Andreas' Vorgabe, direkt umgesetzt):
   - **Pfeil rechts** (`q9_listview_field_enter()`) -- Eintrag aufklappen (falls noch zu) + Fokus
     aufs erste Feld, in einem Schritt.
   - **Pfeil hoch/runter WAEHREND ein Feld fokussiert ist** (`q9_listview_field_move()`) -- bewegt
     NUR zwischen den Feldern DIESES Eintrags, verlaesst ihn nicht ueber die Feldgrenzen hinaus.
   - **Pfeil links** (`q9_listview_field_leave()`) -- verlaesst NUR das Feld, Eintrag bleibt offen.
   - **Esc** (`q9_listview_field_escape()`) -- wie Pfeil links, klappt den Eintrag danach ZUSAETZLICH
     zu (Claudias Vorschlag, von Andreas bestaetigt: "ESC = ganz zurueck", passt zu "Esc: Abbruch"
     im Datei-Dialog).
   - **Enter** bleibt als Kurzform auf Item-Ebene erhalten (Auf-/Zuklappen ohne in die Felder zu
     springen, frueheres Verhalten aus der sechzehnten Runde).
4. **Text-Bearbeitung DIREKT, kein separater Modus** (Andreas: "kann dort alles ändern") --
   `q9_listview_field_putc()`/`_backspace()` aendern den Wert des fokussierten Feldes SOFORT bei
   jedem Tastendruck, kein Enter-zum-Bestaetigen/Esc-zum-Abbrechen-Zyklus wie beim "Datei:"-Kaestchen
   im Dialog (das ist ohnehin nur eine READ-ONLY-Anzeige der Listenauswahl, kein echtes Textfeld --
   Nachschau ergab: es gibt bisher GAR KEIN freies Text-Eingabefeld im Code, dies ist das erste).
   `'o'`/`'O'` oeffnet waehrend eines aktiven Feld-Fokus bewusst NICHT den Datei-Dialog (sonst liesse
   sich kein "o" in einen Wert tippen) -- per echtem Pseudo-Terminal-Test verifiziert (getippt "o" in
   ein Feld mit Wert "onboard" -> "onboardo", Dialog blieb geschlossen).
5. **Rendering** -- die fokussierte Feldzeile bekommt `sel_fg`/`sel_bg` (wie eine Auswahl), die
   Kopfzeile faellt dabei TROTZ Auswahl auf `exp_bg` zurueck (aus der vorigen Runde) -- zu jedem
   Zeitpunkt zeigt genau EINE Zeile die starke `sel_bg`-Hervorhebung, nie zwei gleichzeitig.

`q9_listview.h/.c` Ver. 1.70 (neues `q9_listview_field_t`, `field_focus`, sechs neue Funktionen:
`field_enter`/`_leave`/`_escape`/`_move`/`_putc`/`_backspace`), `integration_demo.c` Ver. 2.80 (alle
20 Eintraege auf echte Felder umgestellt, je 3-4 Felder statt zwei kombinierter Anzeige-Zeilen,
Tastatur-Verdrahtung fuer Rechts/Links/Esc/Zeichen/Backspace, Enter gegen Feld-Fokus abgesichert),
`listview_selftest.c` Ver. 1.60 (neue Tests fuer alle sechs Funktionen inkl. Ueberlauf-/NULL-
Randfaelle, render_ex-Tests fuer die fokussierte Feldzeile). `make test` komplett gruen. Per echtem
Pseudo-Terminal-Test + pyte bestaetigt: Pfeil rechts oeffnet + fokussiert erstes Feld, Tippen aendert
den Wert sofort sichtbar ("onboard" -> "onboardXY"), Pfeil runter wechselt zum naechsten Feld, Pfeil
links verlaesst das Feld (Kopfzeile zeigt wieder die starke Auswahlfarbe, Eintrag bleibt offen), Esc
klappt den Eintrag zusaetzlich zu (zurueck zum Ausgangszustand). Bestehender `filedialog_smoke3.exp`
erneut gruen -- keine Regression.

**Noch offen (naechste Runden auf diesem Fundament):** Dateiauswahl-Feldtyp (den bestehenden
Datei-Dialog aus einem Feld heraus oeffnen), numerische Eingabe (Dezimal/Hex, optional mit
Bereichspruefung), Boolean-Eingabe. Und laengerfristig: ein echtes, pro Hardware-Typ auswertbares
Datenfile (aehnlich `devschema.h/.c`) statt der fest verdrahteten Vorfuehrdaten in `integration_demo.c`.

**Neunzehnte Runde (2026-08-18) -- erster fester Eintrag "Emulator-Konfiguration" + Button-Feldtyp
(Phase 2 von 4):**
*"Ja dann lass uns den ersten festen Eintrag Anlegen, Konfigurationsdatei, auszuwählen mit dem
Dialog. Der Name ist aber blöd, weisst du was besseres als Konfigurationsdatei? Emulatortype?
passt auch nicht so richtig??? Dahinter auf jeden Fall das Namensfeld mit dem Dateinamen, dahinter
ein button um den Dialog zu öffnen. Als Überschrift schlage ich mal 'Emulator Konfiguration' vor,
Da drin dann einfach 'Datei:' ??? Sollen wir dort auch gleich CPU und Netzerk mit rein bringen? Die
sind natürlich erst da wenn die Datei geladen ist, oder ich geben sie von Hand ein und speichere
dann (fehlt uns auch noch)"*

Claudia hat Andreas' eigenen Vorschlag "Emulator-Konfiguration" (mit korrektem Bindestrich)
bestaetigt statt "Konfigurationsdatei"/"Emulatortyp" (beide trafen's nicht) -- Alternative
"Board-Konfiguration" (knuepft an das bestehende `boardcfg.c` im Kernel) angeboten, aber bei
Andreas' Vorschlag geblieben. CPU/Netzwerk BEWUSST NOCH NICHT dazugenommen (Claudias Empfehlung,
von Andreas mitgetragen) -- waeren bis zum echten Laden nur leere Platzhalter, vermischt sonst
"neuer Feldtyp" mit "mehr Platzhalter-Eintraege" in einer Runde. "Speichern" fehlt ebenfalls noch,
bleibt als offener Punkt vermerkt (s.u.).

1. **Neuer Feldtyp** -- `q9_listview_field_kind_t` (`Q9_LISTVIEW_FIELD_TEXT`/`_BUTTON`) als drittes
   Struct-Feld an `q9_listview_field_t`. Ein BUTTON-Feld ignoriert Tippen
   (`field_putc()`/`_backspace()` tun bei `kind!=TEXT` nichts) -- der AUFRUFER erkennt am Typ, dass
   Enter waehrend dieses Feldes eine eigene Aktion ausloesen soll (die Bibliothek selbst kennt keine
   Aktionen/Callbacks, bleibt "nur Buchhaltung"). Bestehende Feld-Initialisierer (`{label, value}`,
   ~108 Stellen in `integration_demo.c` + `listview_selftest.c`) automatisiert per Skript um
   `Q9_LISTVIEW_FIELD_TEXT` als explizites drittes Element ergaenzt (sonst haette `-Wextra`
   `-Wmissing-field-initializers` bei jeder Stelle angeschlagen -- C99-Aggregat-Initialisierung
   haette den Wert zwar korrekt auf 0 genullt, aber eben mit Warnung).
2. **`run_file_dialog()` liefert jetzt optional den rohen Dateinamen zurueck** (neue Parameter
   `out_name`/`out_name_size`, NULL = "interessiert nicht", wie beim bisherigen `'o'`-Tasten-Aufruf
   in `main()`) -- fuer den neuen Button-Feld-Anwendungsfall, der den Namen ins Feld DAVOR
   uebernehmen will (bisher gab die Funktion nur eine formatierte Hinweis-Nachricht zurueck).
3. **Erster fester Eintrag** "Emulator-Konfiguration" ganz oben in `g_list_items[]` -- zwei Felder:
   "Datei:" (normaler TEXT, zeigt/erlaubt den Dateinamen, Default "(keine ausgewaehlt)") und ein
   BUTTON-Feld ("[ Datei waehlen... ]") dahinter. `Q9_KEY_ENTER`-Behandlung in `main()` erweitert:
   bei `field_focus>=0` UND `kind==BUTTON` oeffnet Enter den Dialog, das Ergebnis geht ins Feld
   `field_focus-1` (Konvention: das Namensfeld liegt immer direkt vor seinem Button).

`q9_listview.h/.c` Ver. 1.90 (dabei auch einen Versionsbump-Fehler aus der achtzehnten Runde
nachgetragen -- der Original-Commit hatte sechs neue Funktionen ohne Versions-/Edition-History-
Eintrag verschifft, jetzt als eigene 1.80-Zeile nachgeholt), `integration_demo.c` Ver. 2.90 (neuer
`g_cfg_fields`, erster Listeneintrag, `run_file_dialog()`-Signatur erweitert, Enter-Handler fuer
BUTTON-Felder), `listview_selftest.c` Ver. 1.70 (Feld-Initialisierer ergaenzt, neuer Test: BUTTON-
Feld ignoriert `putc()`/`_backspace()`). `make test` komplett gruen, Build ohne jede Warnung. Per
echtem Pseudo-Terminal-Test + pyte bestaetigt: Pfeil rechts oeffnet "Emulator-Konfiguration" auf dem
"Datei:"-Feld, Pfeil runter wechselt zum Button, Enter oeffnet den Dialog, eine Auswahl (hier "64K")
landet korrekt im "Datei:"-Feld darueber ("(keine ausgewaehlt)" -> "64K"), der Button bleibt darunter
sichtbar. Bestehender `filedialog_smoke3.exp` erneut gruen -- keine Regression.

**Zwanzigste Runde (2026-08-18) -- echtes Zielverzeichnis + Dateifilter:**
*"Cool, Funktion erst mal gegeben :-) Als Verzeichnis sollte ~/.q9-flux gesetzt sein und dort
sollen alle *.q9 Dateien angezeigt werden."*

Der Datei-Dialog scannte bisher HOME mit C-Quelltext-Filtern (reine Demo-Bequemlichkeit, "mehr
Dateien zum Anschauen als im leeren Arbeitsverzeichnis"). Jetzt das ECHTE Zielverzeichnis:
`~/.q9-flux` (wird beim Oeffnen bei Bedarf per `mkdir()` angelegt, Fehler bewusst ignoriert --
existiert es schon, kein Problem), Standardfilter `.q9`, `*.*` bleibt als Ausweichoption im
Dropdown erhalten.

**Faustfalle unterwegs gefunden:** `filters[] = { "*.q9", "*.*" }` (Glob-Stil) zeigte beim ersten
Testlauf eine KOMPLETT LEERE Liste, obwohl `.q9`-Dateien im Verzeichnis lagen -- `q9_filelist.c`s
`ext_matches()` erwartet eine REINE Endung (mit oder ohne fuehrenden Punkt, z.B. `".c"`/`"c"`),
kein Glob-Muster wie `"*.c"`. Per echtem Pseudo-Terminal-Test + pyte sofort aufgefallen (Datei-
Liste blieb leer trotz vorhandener `.q9`-Dateien) und korrigiert auf `".q9"`.

Der bestehende `filedialog_smoke3.exp`-Rauchtest hatte bisher stillschweigend auf zufaelligen
HOME-Inhalt vertraut ("Enter auf der Liste waehlt eine echte Datei") -- seit `~/.q9-flux` das
Zielverzeichnis ist (frisch, meist leer), legt der Test jetzt selbst eine Fixture-Datei
(`smoke_test_fixture.q9`) an und raeumt sie danach wieder auf, statt implizit von vorhandenen
Dateien abzuhaengen.

`integration_demo.c` Ver. 3.00 (`dir`/`filters` in `run_file_dialog()` umgestellt, neues `<sys/
stat.h>`-Include fuer `mkdir()`). `make test` komplett gruen, Build ohne jede Warnung. Per echtem
Pseudo-Terminal-Test + pyte bestaetigt: Dialog zeigt nur `.q9`-Dateien aus `~/.q9-flux` (Testdatei
`ignored.txt` im selben Verzeichnis bleibt korrekt aussen vor), Filter-Anzeige zeigt `.q9` als
Standard. `filedialog_smoke3.exp` (mit eigener Fixture-Datei) erneut gruen.

**Einundzwanzigste Runde (2026-08-18) -- echter dreizeiliger Button + Wert-Box "wie im Dialog":**
*"Das Feld mit dem Dateinamen bitte einen anderen Farbton, so das man es erkennen kann wie lang es
ist. (wie im Dialog) Dateil Wählen wäre natürlich ein Button gut, versuch mal bitte die Zeile Datei
eine Zeile tiefer zu setzen und dahinter mit etwas Abstand ein Button 'Datei' mit der Darstellung
wie im Dislog. Und ersetzt bitte noch '(keine ausgewählt' durch '<leer>'"* -- nach Rueckfrage per
Mockup-Vorschau (leichte Variante vs. echter Dialog-Button) praezisiert: *"Ich dachte der
dreizeilige Button wie im Dialog, der aussieht wie zwei Zeilen 1/2 Balken oben, 1/2 Balken unten,
laesst sich dann zentrisch hinter Datei ausrichten."*

Neuer SONDERFALL in `q9_listview.c`: ein TEXT-Feld, DIREKT gefolgt von einem BUTTON-Feld, wird als
EINE Einheit gezeichnet (3 Zeilen statt 2 -- `q9_listview_item_rows()` und `render_ex()` teilen sich
dieselbe `is_text_button_pair()`-Erkennung, damit Zeilenzahl und tatsaechliches Rendering nie
auseinanderlaufen):
1. **Wert-Box** -- der TEXT-Feld-Wert bekommt (unfokussiert) eine feste, sichtbare Box (neue
   `box_fg`/`box_bg`-Parameter an `render_ex()`) statt reinem Fliesstext -- die feste Breite
   (`Q9_LISTVIEW_VALUE_BOX_WIDTH` = 20 Zeichen) macht die Laenge/Ausdehnung sofort erkennbar.
2. **Echter dreizeiliger Button** -- genau wie `draw_button()` im Datei-Dialog: Text in eigener
   Farbflaeche, darueber/darunter je eine Halbblock-Kappenzeile (`Q9_GLYPH_LOWER_HALF`/
   `UPPER_HALF`). Dadurch braucht das Paar INSGESAMT 3 Zeilen -- Kappe oben, gemeinsame Zeile
   (Label+Box links, Button-Text rechts mit Abstand), Kappe unten. Der Button steht dadurch
   vertikal ZENTRIERT auf Hoehe des Textfelds (erklaert auch "die Zeile Datei eine Zeile tiefer" --
   das ist einfach die Konsequenz der neuen Kappe-oben-Zeile).
3. **Farben identisch zum Dialog** -- `box_fg`/`box_bg` werden mit denselben Konstanten befuellt,
   die der Datei-Dialog fuer sein Namens-Kaestchen UND seine OK/Abbrechen-Buttons nutzt
   (`PAL_DIALOG_SUB_FG`/`PAL_DIALOG_SUB_BG`) -- wortwoertlich "wie im Dialog", keine neue Farbe
   erfunden. Fokussiert springt der Button (wie jedes andere Feld) auf `sel_fg`/`sel_bg` um.
4. Button-Feldwert von `"[ Datei waehlen... ]"` auf `"Datei"` gekuerzt (zentriert im Button), `"(keine
   ausgewaehlt)"` durch `"<leer>"` ersetzt.

BEKANNTE VEREINFACHUNG: die Wert-Box hat eine FESTE Breite -- ein sehr langer Dateiname (laenger als
20 Zeichen) laeuft ueber die Box hinaus in die Luftspalte vor dem Button hinein (im Test mit
`smoke_test_fixture.q9`, 22 Zeichen, beobachtet) -- bei Bedarf spaeter nachruestbar (z.B. Box
verbreitern oder den Wert in der Box abschneiden).

`q9_listview.h/.c` Ver. 2.00 (neue `box_fg`/`box_bg`-Parameter, `is_text_button_pair()`,
`Q9_LISTVIEW_VALUE_BOX_WIDTH`/`_BUTTON_GAP`/`_BUTTON_WIDTH`), `integration_demo.c` Ver. 3.10 (Button-
Text gekuerzt, Platzhalter ersetzt, `render_ex()`-Aufruf um `PAL_DIALOG_SUB_FG/BG` ergaenzt),
`listview_selftest.c` Ver. 1.80 (neue Tests: `item_rows()` fuer das Paar, Box-Farbe, Kappen-Position,
fokussierter vs. unfokussierter Button). `make test` komplett gruen, Build ohne jede Warnung. Per
echtem Pseudo-Terminal-Test + direkter pyte-Farbpruefung bestaetigt: Box zeigt `PAL_DIALOG_SUB_FG`/
`_BG` (unfokussiert), Button ebenso, Kappen exakt eine Zeile ueber/unter der gemeinsamen Zeile,
fokussierter Button zeigt korrekt `sel_fg`/`sel_bg`. Kompletter End-zu-Ende-Ablauf getestet: Pfeil
rechts -> Pfeil runter (Button fokussiert) -> Enter (Dialog oeffnet) -> Enter (Datei waehlen) ->
Dateiname landet in der Box. `filedialog_smoke3.exp` erneut gruen -- keine Regression.

**Zweiundzwanzigste Runde (2026-08-18) -- Wert-Box-Breite an den Dialog angeglichen:**
*"Im Dialog sind es ca. 35 Zeichen, sollen wir das hier auch nehmen?"* -- ja, `Q9_LISTVIEW_
VALUE_BOX_WIDTH` von 20 auf 35 Zeichen erhoeht (dasselbe Mass wie das Namens-Kaestchen im
Datei-Dialog, dort dynamisch berechnet, hier als fester Wert uebernommen). Behebt nebenbei den
zuvor dokumentierten Ueberlauf bei laengeren Dateinamen (`smoke_test_fixture.q9`, 22 Zeichen, passt
jetzt bequem hinein). `q9_listview.c` Ver. 2.10, `listview_selftest.c` Ver. 1.90 (Spaltenerwartungen
in den TEXT+BUTTON-Tests auf die neue Breite angepasst). `make test` komplett gruen.

**Dreiundzwanzigste Runde (2026-08-18) -- Numerische Feldtypen (Phase 3 von 4):**
*"würde sagen einfach der reihe nach ..."* -- naechster Feldtyp laut Fahrplan: numerische Eingabe
Dezimal/Hex, optional mit Bereich.

BEWUSST als zwei NEUE `kind`-WERTE (`Q9_LISTVIEW_FIELD_NUMERIC_DEC`/`_HEX`) statt neuer Struct-
Felder umgesetzt -- die vorige Runde (BUTTON-Feldtyp) hat gezeigt, wie muehsam ein neues Struct-Feld
ist (~108 bestehende Initialisierer mussten nachtraeglich ergaenzt werden). Mit dieser Loesung: KEIN
einziger bestehender Aufruf musste angefasst werden, `-Wextra` blieb von Anfang an stumm.

1. **Zeichenklassen-Filterung** -- `q9_listview_field_putc()` verwirft bei `NUMERIC_DEC` alles
   ausser `0`-`9`, bei `NUMERIC_HEX` zusaetzlich `a`-`f`/`A`-`F`. Das ist die Kernfunktion
   "Dezimal/Hex".
2. **"$"-Praefix automatisch** -- `NUMERIC_HEX`-Werte werden OHNE fuehrendes "$" gespeichert
   (reine Hex-Ziffern), `q9_listview_render_ex()` zeichnet das "$" davor.
3. **Bereichspruefung BEWUSST NICHT in der Bibliothek** -- welcher Bereich fuer welches Feld gilt,
   ist Anwendungswissen, keine Listenansicht-Zustaendigkeit. Stattdessen ein konkretes Beispiel in
   `integration_demo.c`: `clamp_field_range()` klemmt einen vollstaendigen (nicht: waehrend des
   Tippens unvollstaendigen) Wert beim Verlassen des Feldes (Pfeil links/Esc) auf `[lo,hi]`.
   Demonstriert am "Slot:"-Feld von RC2014-CF (0-255 -- spiegelt die bestehende `useSlot`/`slot`-
   Pruefung in `src/kernel/boardcfg.c`).
4. **Demo-Daten umgestellt** -- alle 11 `"Basis:"`-Felder (Hex-Adressen) auf `NUMERIC_HEX`, `"IRQ:"`-
   und `"Port:"`-Felder mit echter Zahl auf `NUMERIC_DEC` (Felder mit `"-"` als Platzhalter, z.B.
   RTC's IRQ oder CF-Interface's Slot bei onboard-Bus, bleiben bewusst TEXT).

`q9_listview.h/.c` Ver. 2.10/2.20 (neue kind-Werte, `is_allowed_numeric_char()`, "$"-Praefix-
Rendering), `integration_demo.c` Ver. 3.20 (Felddaten umgestellt, `clamp_field_range()` +
`maybe_clamp_focused_field()`), `listview_selftest.c` Ver. 2.00 (neue Tests: Zeichenklassen-
Filterung, "$"-Praefix). `make test` komplett gruen, Build ohne jede Warnung. Per echtem
Pseudo-Terminal-Test bestaetigt: CF-Interface's "Basis:" zeigt `$FFFFE000`, RC2014-CF's "Slot:"
(Eingabe "999" -- Zeichen werden akzeptiert, da field_putc() keine Bereichspruefung macht) wird
beim Verlassen des Feldes korrekt auf `255` geklemmt. `filedialog_smoke3.exp` erneut gruen.

**Noch offen (bis zur naechsten Runde):** Boolean-Feldtyp (Phase 4/4), echtes Laden/Auswerten der
`.q9`-Datei, Speichern-Funktion, mechanische Uebernahme von NUMERIC fuer die restlichen Felder
(`Eintraege:`, `Groesse:` u.ae. sind aktuell noch TEXT).

**Vierundzwanzigste Runde (2026-08-18) -- Boolean-Feldtyp (Phase 4 von 4, letzte Phase):**
*"ja bitte... mach einfach weiter :-)"* -- letzter der vier urspruenglich genannten Feldtypen
("Texteingabe, Dateiauswahl (mit dem Dialog), Numerische Eingabe Dezimal/Hex opt. mit Bereich,
Boolean Eingabe... denke das ist das Mindeste").

Wieder als NEUER `kind`-WERT (`Q9_LISTVIEW_FIELD_BOOLEAN`) statt neuem Struct-Feld umgesetzt --
derselbe Grund wie bei NUMERIC_DEC/_HEX (s. vorige Runde): kein einziger bestehender Initialisierer
musste angefasst werden.

1. **Fester Wertebereich** -- ein BOOLEAN-Feld ist IMMER genau `"ja"` oder `"nein"` (die Konvention,
   die schon alle bisherigen `"Aktiv:"`-Felder als reinen TEXT-Wert verwendet haben).
2. **Umschalten statt Tippen** -- neue Funktion `q9_listview_field_toggle()` kehrt den Wert um; der
   Aufrufer bindet sie an die Leertaste (`integration_demo.c`, `Q9_KEY_CHAR`-Behandlung, Sonderfall
   VOR der normalen `field_putc()`-Weiterleitung). `field_putc()`/`_backspace()` ignorieren
   BOOLEAN-Felder wie schon BUTTON-Felder -- Tippen/Loeschen wirkt dort nicht.
3. **Kein eigenes Rendering** -- ein BOOLEAN-Feld sieht optisch aus wie ein TEXT-Feld (kein
   Kaestchen-Symbol o.ae. in dieser ersten Fassung, bei Bedarf spaeter nachruestbar); der einzige
   Unterschied ist die Interaktion.
4. **Demo-Daten umgestellt** -- alle 14 bestehenden `"ja"`/`"nein"`-Felder (`Aktiv:` bei acht
   Eintraegen, `Link:`, `Parity:`, `Getestet:`, `Schreibschutz:`) auf `Q9_LISTVIEW_FIELD_BOOLEAN`.
   Hinweistext unten um "Leertaste: ja/nein" ergaenzt.

`q9_listview.h/.c` Ver. 2.20/2.30 (neuer kind-Wert, `field_toggle()`, `value_equals()`/
`value_assign()` als Hilfsfunktionen OHNE `<string.h>` -- die Datei kam bisher bewusst ganz ohne
aus), `integration_demo.c` Ver. 3.30 (Felddaten umgestellt, Leertaste-Sonderfall in `Q9_KEY_CHAR`),
`listview_selftest.c` Ver. 2.10 (neue Tests: ja↔nein-Umschaltung, No-op bei TEXT-Feldern,
unerwarteter Ausgangswert wird zu "ja", NULL-Sicherheit). `make test` komplett gruen, Build ohne
jede Warnung. Per echtem Pseudo-Terminal-Test bestaetigt: CF-Interface's "Aktiv:"-Feld springt nach
einer Leertaste sichtbar von "ja" auf "nein". `filedialog_smoke3.exp` erneut gruen.

Damit sind alle vier von Andreas genannten Eingabearten fertig (Text, Dateiauswahl, Numerisch,
Boolean).

**Noch offen (bis zur naechsten Runde):** echtes Laden/Auswerten der `.q9`-Datei, Speichern-
Funktion, mechanische Uebernahme von NUMERIC fuer die restlichen Felder (`Eintraege:`, `Groesse:`
u.ae. sind aktuell noch TEXT), eventuell ein eigenes Symbol fuer BOOLEAN-Felder statt reinem
"ja"/"nein"-Text.

**Fuenfundzwanzigste Runde (2026-08-18) -- echtes Laden der `.q9`-Datei:**
*"ja bitte ...."* -- naechster Punkt laut eigener Reihenfolge, nachdem alle vier Feldtypen standen.

Statt einen zweiten, eigenen INI-Parser fuer den Editor zu schreiben, wird der ECHTE Board-Config-
Parser des Emulators wiederverwendet: `src/kernel/boardcfg.c/.h` (derselbe, den `q9.exe` beim Start
liest, s. `docs/HWCONFIG.md` Abschnitt 3). Vorab geprueft, ob das ueberhaupt sauber geht (die
Sorge: der Kernel-Baum koennte schwere Abhaengigkeiten mitziehen) -- `boardcfg.c` braucht nur
`q9board.h`/`devreg.h` fuer ein paar Konstanten/Typen, ruft aber KEINE Funktionen aus anderen
Kernel-Modulen auf (per `nm -u` nachgewiesen: keine undefinierten Symbole ausser Libc). Laesst sich
daher unveraendert in das eigenstaendige Tool hineinlinken, garantiert dabei echte Format-
kompatibilitaet statt eines zweiten, potenziell driftenden Parsers.

1. **Neue Felder** -- `Name:`/`ROM:`/`CPU:` bei "Emulator-Konfiguration" (`g_cfg_fields[2..5]`,
   feste Positionen direkt nach dem bestehenden `Datei:`+Button-Paar), zunaechst `"<leer>"`.
2. **`load_q9_config_fields()`** -- nach erfolgreicher Dateiauswahl (im BUTTON-Enter-Handler)
   aufgerufen: baut den vollen Pfad (`~/.q9-flux/<Dateiname>`, ueber neuen gemeinsamen Helfer
   `q9flux_dir()` -- vorher in `run_file_dialog()` dupliziert, jetzt einmal), ruft
   `q9_board_cfg_default()` + `q9_board_cfg_load()` auf. Erfolg: die vier Felder zeigen `cfg.name`/
   `cfg.rom_path`/`cfg.net_mode`/`cfg.cpu` (leerer Config-Wert -> `"<leer>"`). ROM-Pfade werden vom
   echten Parser bereits relativ zur Config-DATEI aufgeloest (nicht zum CWD), s. Test unten.
3. **Fehlerfall** -- kaputte/unlesbare Datei (Test: `useSlot = yes` ohne `slot`): alle vier Felder
   zeigen `"<Fehler>"`, die Statuszeile die genaue Meldung von `q9_board_cfg_load()` (inkl.
   Zeilennummer).
4. **Reiner Anzeige-Zweck** -- von Hand in diesen Feldern tippen schreibt (noch) nicht in die Datei
   zurueck, das ist Sache der kommenden Speichern-Funktion.

`Makefile` Ver. 1.90 (linkt `../../src/kernel/boardcfg.c` zusaetzlich in `demo-integration`),
`integration_demo.c` Ver. 3.40. `make test` komplett gruen, Build ohne jede Warnung. Per echtem
Pseudo-Terminal-Test bestaetigt (zwei Testdateien in `~/.q9-flux` angelegt, danach wieder
geloescht): eine gueltige Datei fuellt Name:/ROM:/Netz:/CPU: korrekt (ROM-Pfad relativ zur
Config-Datei aufgeloest, z.B. `roms/testrom.bin` -> `/Users/afoe/.q9-flux/roms/testrom.bin`); eine
kaputte Datei (`useSlot = yes` ohne `slot`) zeigt in allen vier Feldern `<Fehler>` und die passende
Fehlermeldung in der Statuszeile. `filedialog_smoke3.exp` erneut gruen.

**Noch offen (bis zur naechsten Runde):** Speichern-Funktion (komplett unimplementiert), mechanische
Uebernahme von NUMERIC fuer die restlichen Felder, eventuell ein eigenes Symbol fuer BOOLEAN-Felder,
`[cfN]`-Abschnitte der geladenen Datei (bisher wird nur `[board]` ausgewertet/angezeigt).

**Sechsundzwanzigste Runde (2026-08-18) -- Speichern-Funktion:**
*"ja bitte, leg los"* -- letzter der drei zuletzt genannten offenen Punkte (echtes Laden stand
bereits, Speichern war von Andreas selbst mehrfach als offener Punkt genannt worden).

Symmetrisch zur Laden-Runde: statt eines eigenen Serialisierers bekommt der ECHTE Board-Config-
Parser des Emulators ein neues Gegenstueck, `q9_board_cfg_save()` (`src/kernel/boardcfg.h/.c`,
NICHT nur im Editor-Tool -- eine echte Erweiterung des Kernel-Moduls, kommt so auch `q9.exe` selbst
zugute). Vorher genau die Frage geklaert, die bei Speicherfunktionen leicht schiefgeht:

1. **Datenverlust-Falle erkannt und vermieden** -- der Editor zeigt bisher NUR `[board]`-Felder
   (Name:/ROM:/Netz:/CPU:), keine `[cfN]`-Abschnitte. Ein naiver Save haette beim Ueberschreiben
   einer geladenen Datei deren CF-Images stillschweigend geloescht. Loesung: `g_loaded_cfg` haelt
   die zuletzt erfolgreich geladene Konfiguration VOLLSTAENDIG im Speicher (inkl. `cf[]`); Save
   startet davon (statt bei leeren Defaults) und ueberschreibt nur die vier tatsaechlich
   angezeigten Felder. Per Pseudo-Terminal-Test verifiziert: eine Datei mit `[cf0]`-Abschnitt
   laden, nur `Name:` per Hand aendern, speichern -- `[cf0]` steht danach unveraendert in der
   Datei.
2. **Relative Pfade bleiben relativ** -- `q9_board_cfg_load()` loest Pfade beim Laden relativ zum
   Config-Verzeichnis auf (`cfg_resolve_rel()`); ein naives Zurueckschreiben dieser bereits
   aufgeloesten (oft absoluten) Pfade wuerde eine portable Config unportabel machen. Neue
   Hilfsfunktion `cfg_relativize()` kehrt das um: beginnt ein Pfad mit dem Config-Verzeichnis,
   wird dieser Praefix beim Speichern wieder abgeschnitten. Getestet mit einem echten
   Load-Save-Load-Roundtrip (Unit-Test `test/11_test_boardcfg_save.c`) UND live im Editor (ROM-Pfad
   wird angezeigt als `/Users/.../roms/orig.bin`, gespeichert aber wieder als `roms/orig.bin`).
3. **`vmnet_*`-Keys nur bei Abweichung vom Default** -- sonst waere jede gespeicherte Datei mit
   vier Zeilen vollgestellt, die nur bei `net=vmnet` wirken.
4. **Kein Dateiname gewaehlt** -- Save meldet dann einen klaren Hinweis statt zu versuchen, "ins
   Leere" zu schreiben.
5. **Taste S** (global wie `O`, kein Feld muss fokussiert sein) loest `save_q9_config()` aus.

`boardcfg.h/.c` Ver. 1.60/1.50 (neue Funktion `q9_board_cfg_save()` + Hilfsfunktion
`cfg_relativize()`), `Makefile` (Root) Ver. 4.40 (neues Test-Target `test-boardcfg-save`, dabei
NACHTRAG: die EOF-Fusszeile war seit laengerem auf einer alten Version stehen geblieben, jetzt
nachgezogen), `test/11_test_boardcfg_save.c` NEU (voller Roundtrip inkl. `[cfN]`, `vmnet_*`-
Default-Unterdrueckung, Fehlerfall), `integration_demo.c` Ver. 3.50 (`g_loaded_cfg`/`g_cfg_loaded`,
`save_q9_config()`, Taste S, Hinweistext ergaenzt). ROOT `make test` (nicht nur der tool-lokale)
komplett gruen, Build ohne jede Warnung. Per echtem Pseudo-Terminal-Test bestaetigt: Datei mit
`[cf0]`-Abschnitt laden, Name:-Feld per Hand erweitern, S druecken -- Datei zeigt danach den
geaenderten Namen UND den unveraenderten `[cf0]`-Abschnitt; Speichern ohne gewaehlte Datei zeigt
die erwartete Fehlermeldung. `filedialog_smoke3.exp` erneut gruen.

**Noch offen (bis zur naechsten Runde):** `[cfN]`-Abschnitte im Editor selbst anzeigen/bearbeiten
(bisher nur beim Speichern unangetastet durchgereicht), mechanische Uebernahme von NUMERIC fuer
die restlichen Felder, eventuell ein eigenes Symbol fuer BOOLEAN-Felder, "Neu anlegen" als
expliziter, vom Laden unabhaengiger Weg (aktuell nur implizit ueber einen noch nicht existierenden
Dateinamen im Datei:-Feld erreichbar).

**Siebenundzwanzigste Runde (2026-08-18) -- [cfN]-Abschnitte im Editor:**
*"1 dann 2 dann 3 würde ich sagen :-)"* -- von drei angebotenen offenen Punkten als erstes gewaehlt
(danach: NUMERIC-Umstellung restlicher Felder, dann ein eigenes Boolean-Symbol).

Die [cfN]-Abschnitte einer geladenen Datei (`g_loaded_cfg.cf[]`) waren bisher nur beim Speichern
unangetastet durchgereicht, im Editor selbst aber unsichtbar. Jetzt erscheinen sie als eigene
Listeneintraege "CF-Image #0".."CF-Image #3" (`Q9_CFG_MAX_CF`=4 feste Slots aus `boardcfg.h`).

1. **Feste Slots am ENDE der Liste** -- `g_list_items[]` bekommt vier zusaetzliche Eintraege ganz
   am Ende (nach den 20 hardcodierten Hardware-Demo-Eintraegen), mit eigenen Feld-Arrays
   `g_cfimg_fields[4][4]`. Dadurch braucht ein wechselnder `cf_count` KEINE Verschiebung anderer
   Eintraege -- nur eine neue Laufzeit-Variable `g_item_count` (statt der bisherigen Compile-Zeit-
   Konstante `ITEM_COUNT`, die zu `MAX_ITEM_COUNT`/`BASE_ITEM_COUNT` wurde) waechst/schrumpft nach
   jedem Laden um `cf_count` (0-4). `g_expanded[]` bleibt bei voller `MAX_ITEM_COUNT`-Groesse
   (auch unsichtbare Slots brauchen einen gueltigen Speicherplatz).
2. **Vier Felder je Slot** -- Typ:/Bus:/Unit: (die kurzen Enum-Werte aus `q9_cfg_cf_t`) und
   Datei: (der Image-Pfad, wie ROM: beim Laden bereits absolut aufgeloest angezeigt). BEKANNTE
   VEREINFACHUNG: Basis/Slot/Descriptor bleiben (noch) nicht editierbar -- das waere ein eigener,
   groesserer Schritt (Bereichspruefung, useSlot/base-Wechselspiel).
3. **String<->Enum-Umwandlung bewusst dupliziert** -- `cf_format_str()`/`_bus_str()`/`_unit_str()`
   (Anzeige) und ihre Umkehrung `parse_cf_format()`/`_bus()`/`_unit()` (Speichern) statt
   `boardcfg.c`s private `cfg_parse_*()`-Helfer freizulegen (die sind absichtlich `static`, kein
   Teil der oeffentlichen `boardcfg.h`-API). Duplikation hier klein/risikoarm (feste, stabile
   Wertelisten) -- im Unterschied zur vollen INI-Syntax, die deshalb WEITERHIN nicht dupliziert
   wird. Ein ungueltiger Wert faellt spaetestens beim naechsten Laden auf.
4. **Speichern erweitert** -- `save_q9_config()` schreibt jetzt zusaetzlich Typ:/Bus:/Unit:/Datei:
   der SICHTBAREN CF-Image-Eintraege zurueck (`cfg.cf_count` entsprechend gesetzt); alles darueber
   hinaus bleibt unberuehrt.

`integration_demo.c` Ver. 3.60 (dabei NACHTRAG: die EOF-Fusszeile war seit der numerischen Runde
auf 3.20 stehen geblieben, jetzt nachgezogen). `make test` (tool-lokal UND root) komplett gruen,
Build ohne jede Warnung. Per echtem Pseudo-Terminal-Test bestaetigt: Datei mit zwei `[cfN]`-
Abschnitten laden -- CF-Image #0 zeigt korrekt Typ: rbf/Bus: onboard/Unit: master/Datei: (absolut
aufgeloest), CF-Image #1 pcf/rc2014/slave; Datei:-Feld von CF-Image #0 per Hand auf einen neuen
Namen geaendert, gespeichert -- `[cf0].image` aendert sich, `[cf1]` UND alle anderen `[cf0]`-Werte
bleiben unveraendert. `filedialog_smoke3.exp` erneut gruen.

**Noch offen (bis zur naechsten Runde):** mechanische Uebernahme von NUMERIC fuer die restlichen
Felder, eventuell ein eigenes Symbol fuer BOOLEAN-Felder, Basis/Slot/Descriptor bei CF-Images
editierbar machen, "Neu anlegen" als expliziter Weg.

**Achtundzwanzigste Runde (2026-08-18) -- mechanische NUMERIC-Uebernahme:**
Zweiter von drei angebotenen offenen Punkten ("1 dann 2 dann 3"). Alle noch verbliebenen TEXT-
Felder durchgesehen: nur EIN einziges ist noch eine reine Dezimalzahl ohne Einheit/Platzhalter --
CLUT's `Eintraege:` (`"256"`) -- jetzt `NUMERIC_DEC`. Alle anderen Kandidaten stellten sich beim
genaueren Hinsehen als NICHT geeignet heraus:

- **`Groesse:`-Felder** (Framebuffer `512K`, RAM `4 MB`, ROM `256K`, NVRAM `2K`) -- Einheit im
  Wert, keine reine Zahl. `NUMERIC_DEC`s Zeichenklassen-Filter (`is_allowed_numeric_char()`,
  s. `q9_listview.c`) akzeptiert nur `0`-`9` -- K/MB/Leerzeichen waeren dann beim Tippen NICHT
  mehr eingebbar, ein echter Funktionsverlust statt einer reinen Verbesserung. Bleiben bewusst
  TEXT.
- **`IRQ:`-Felder mit `"-"`** (z.B. RTC) -- Platzhalter, bereits in der numerischen Runde als
  bewusste Ausnahme dokumentiert.

`integration_demo.c` Ver. 3.70. `make test` (tool-lokal UND root) komplett gruen, Build ohne
Warnung. Per Pseudo-Terminal-Test bestaetigt: Buchstabe wird im `Eintraege:`-Feld jetzt verworfen,
Ziffer akzeptiert (`x` verworfen, `9` uebernommen -- Wert `2569` statt `256x9`).
`filedialog_smoke3.exp` erneut gruen.

**Noch offen (bis zur naechsten Runde):** eigenes Symbol fuer BOOLEAN-Felder, Basis/Slot/
Descriptor bei CF-Images editierbar machen, "Neu anlegen" als expliziter Weg.

**Neunundzwanzigste Runde (2026-08-18) -- eigenes Symbol fuer BOOLEAN-Felder:**
Dritter und letzter der drei angebotenen offenen Punkte ("1 dann 2 dann 3"). BOOLEAN-Felder sahen
bisher genauso aus wie TEXT (reiner "ja"/"nein"-Fliesstext, bewusste Einfachheit der ersten
Fassung, s. Kommentar bei `Q9_LISTVIEW_FIELD_BOOLEAN`) -- jetzt ein echtes Kaestchen-Symbol davor.

1. **Zwei neue Glyphen** -- `Q9_GLYPH_CHECKBOX_ON`/`_OFF` (☑/☐, U+2611/U+2610 "Ballot Box"-Block)
   in `q9_screenbuf.h/.c`, derselbe Sentinel-Mechanismus wie alle bisherigen `Q9_GLYPH_*`
   (Ein-Byte-Marker im sonst ungenutzten Steuerzeichen-Bereich, `glyph_utf8()` uebersetzt beim
   Rendern in die echte 3-Byte-UTF-8-Folge). AUSSERHALB des bisherigen U+2500-U+257F-Bereichs,
   aber gleiche 3-Byte-Laenge -- kein Sonderfall im Rendering-Code noetig.
2. **Praefix-Mechanismus wie bei NUMERIC_HEX** -- `render_ex()` zeichnet das Symbol automatisch
   vor dem Wert (`Q9_GLYPH_CHECKBOX_ON` bei `"ja"`, sonst `_OFF`), der Wert-TEXT selbst bleibt
   unveraendert sichtbar daneben -- "☑ ja" / "☐ nein" statt reinem "ja"/"nein". Eine zusaetzliche
   Luftspalte zwischen Symbol und Wert (anders als beim kompakten "$FFFF..." bei NUMERIC_HEX) --
   "☑ja" ohne Leerzeichen waere schlechter lesbar.
3. **Kein Struct-/Kind-Aenderung** -- reine Rendering-Erweiterung, `value` bleibt weiterhin
   literal `"ja"`/`"nein"`, `field_toggle()` unveraendert.

`q9_screenbuf.h/.c` Ver. 1.40, `q9_listview.h/.c` Ver. 2.30/2.40 (dabei NACHTRAG:
`listview_selftest.c`s EOF-Fusszeile war seit der Boolean-Runde auf 2.00 stehen geblieben, jetzt
nachgezogen), `listview_selftest.c` Ver. 2.20 (neue Tests: Symbol- und Wert-Spalte fuer "ja"/
"nein"). `make test` (tool-lokal UND root) komplett gruen, Build ohne Warnung. Per echtem
Pseudo-Terminal-Test bestaetigt: CF-Interface's "Aktiv:"-Feld zeigt "☑ ja", nach Leertaste "☐ nein".
`filedialog_smoke3.exp` erneut gruen.

Damit sind alle drei von Andreas in dieser Reihenfolge angebotenen offenen Punkte abgearbeitet.

**Noch offen:** Basis/Slot/Descriptor bei CF-Images editierbar machen, "Neu anlegen" als
expliziter Weg, echtes pro-Hardware-Typ-Datenfile aehnlich `devschema.h/.c` statt hartcodierter
Demo-Felder.

**Dreissigste Runde (2026-08-20) -- "Anlegen/Loeschen von Hardware-Instanzen", der eigentliche
Auftrag dieser Runde:** Andreas' Rueckfrage ergab: gemeint war NICHT nur ein "Neu anlegen"-Weg fuer
Config-Dateien, sondern das generische `## 4. Hardware-Bereich`-Ziel ("Hardware hinzufuegen" fuer
beliebige Geraete-Instanzen) -- mit einer zusaetzlichen Vorgabe: Hardware-*Beschreibung* und
-*Aufruf* sollen vereinheitlicht werden, **pro Hardware-Typ ein eigenes Sourcefile**, mit Fokus auf
Ausfuehrungsgeschwindigkeit (bei vielen simulierten Geraeten soll nicht jeder Speicherzugriff alle
Geraete linear abfragen muessen -- ein neues Boolean-Feld je Typ steuert, ob er in eine schnelle
Adress-Index-Tabelle aufgenommen wird).

*Wichtiger Fund bei der Recherche:* Diese Fast-Table-Optimierung existierte bereits
(`io_table_build()`/`devreg_hit()`, `m68krt.c`, seit 2026-08-14/5.18) -- aber automatisch fuer den
festen I/O-Cluster, ohne explizites Pro-Geraet-Flag. Ebenso existierte bereits eine reine
Feldbeschreibungstabelle (`devschema.c`), aber NICHT ins Haupt-Binary gelinkt und NICHT mit dem
Parser verdrahtet. Die eigentliche Geraete-Instanziierung lief ueber zehn hartcodierte
`q9_devreg_add()`-Aufrufe in `m68krt.c`. Bereits gelebtes Vorbild fuer "ein Sourcefile je Typ":
mc6845/clut/framebuf/quicc liegen seit der "6.6"-Migration (2026-08-11) schon in `src/devices/<typ>/`
-- nur die vier aeltesten, 5.17-Ära-Geraete (duart68681/cf/rtc72421/timer_irq) steckten noch
gebuendelt in `q9board.c`.

**Umfang dieser Runde (Plan-Modus, Andreas' Freigabe):** ein vollstaendiger Pilot-Vertical-Slice am
Beispiel "cf" -- bewusst nicht alle neun Typen auf einmal:
1. **`q9_device_t.use_table`** (NEU, `devreg.h`) -- explizite Fast-Table-Teilnahme statt impliziter
   Cluster-Annahme. `io_table_build()` (`m68krt.c`) traegt Geraete mit `use_table==0` als
   "ambiguous" ein (erzwingt zuverlaessig den linearen Scan-Fallback). Alle zehn bestehenden
   `q9_devreg_add()`-Aufrufstellen explizit gesetzt (neun `=1`, Framebuffer `=0` -- liegt ohnehin
   unterhalb des Clusters, $FD000000 < $FFFF0000).
2. **`q9_devdesc_t`** (NEU, `src/kernel/devdesc.h/.c`) -- vereint Vtable (devreg-Stil) + typspezifische
   Feldbeschreibung (devschema-Stil) + `use_table_default` an einem Ort. Gemeinsame Basisfelder
   (Name/Basisadresse/Endadresse) sind KEINE eigene Tabelle -- die sind schon strukturelle
   `q9_device_t`-Member. Die zwei echten gemeinsamen Editor-Felder (`descriptor`/`descriptorName`)
   sind jetzt EINMAL in `q9_devschema_common_fields[]` definiert statt pro Typ wiederholt.
3. **CF komplett umgezogen**: `q9_cf_t`/`q9_cf_attach`/das ATA-PIO-Protokoll/`q9_devtype_cf`
   (~850 der 1077 Zeilen von `q9board.c`) nach `src/devices/cf/cf.c`+`cf.h` -- reine Verschiebung,
   KEINE Verhaltensaenderung (per `make test` UND echtem Boot-Test mit einer lokalen `.q9`-Datei bis
   zum Login bit-identisch bestaetigt). `devschema.c`s bisheriges `"cf"`-Schema entfaellt (jetzt
   `q9_devdesc_cf` in `cf.c`) -- neuer Test `test/12_test_devdesc.c` (34 Checks) uebernimmt die
   bisherigen `"cf"`-Testfaelle aus `test/08_test_devschema.c`.
4. **`boardcfg.c`-Schema-Gegenprobe**: `cfg_schema_confirm_invalid_enum()` (NEU) -- bewusst NUR auf
   dem bereits-ungueltig-Pfad von `type`/`bus`/`unit` (kein Eingriff in den Erfolgspfad/die
   bestehenden Fehlermeldungen), bestaetigt per `q9_devdesc_lookup("cf")`, dass das Schema
   denselben Wert ebenfalls ablehnt -- Beweis, dass das Schema jetzt load-bearing ist statt reiner
   Zukunftsmusik.
5. **Editor: echte Tasten `N`/`D`** fuer die vier bereits vorhandenen CF-Image-Slots (statt nur
   passivem Wachsen/Schrumpfen beim Laden) -- `add_cfimg_slot()`/`delete_cfimg_slot()`/
   `cfimg_reset_slot()` (NEU, `integration_demo.c`). `save_q9_config()` brauchte KEINE Aenderung
   (war laut eigenem Kommentar schon bewusst robust dafuer ausgelegt). Per echtem
   Pseudo-Terminal-Rauchtest verifiziert: vier Slots anlegen, fuenfter Versuch meldet korrekt "voll",
   alle vier wieder loeschen (Auswahl wandert dabei korrekt zum vorherigen Slot bzw. zurueck auf den
   letzten Hardware-Demo-Eintrag), `D` auf einem Nicht-CF-Eintrag ist ein sauberes No-op mit Meldung.

`make test` (Root, inkl. neuem `test-devdesc`) UND `make test`/`make demo-integration` (Editor)
komplett gruen, kein neues Warning. Zwei echte Boot-Tests (vor/nach der CF-Migration) bit-identisch
(8 devices online, Login, funktionierende Shell).

**Ausdruecklich NICHT Teil dieser Runde (naechste Schritte, dasselbe Muster):**
- Dieselbe Migration fuer die restlichen acht Typen (duart68681/rtc72421/timer_irq/nettty/quicc/
  mc6845/framebuf/clut) -- rein mechanische Wiederholung des jetzt bewiesenen Vorgehens.
- `boardcfg.c`: generische Abschnittserkennung fuer BELIEBIGE Typnamen (nicht nur die laxe
  `cf*`-Erkennung) ueber `q9_devdesc_lookup()`, inkl. generischer Instanziierungs-Schleife in
  `q9boardrun.c` (ersetzt die heutige CF-spezifische Sonderbehandlung).
- Editor: echtes **"Hardware hinzufuegen"** als Typ-Auswahl-Dialog ueber alle `q9_devdesc_get(i)`-
  Eintraege (nicht nur die vier CF-Slots) -- der eigentliche, generische Teil von Abschnitt 4 oben,
  inkl. Adress-Kollisionspruefung zwischen Instanzen unterschiedlichen Typs.
- `q9_board_cfg_save()` generisch fuer beliebige Geraete-Abschnitte statt nur `[cfN]`.
- Die 20 hartcodierten Demo-Hardware-Eintraege in `integration_demo.c` durch echte, aus
  `q9_devdesc_get()` abgeleitete Eintraege ersetzen.

**Einunddreissigste Runde (2026-08-21) -- dasselbe Muster fuer alle restlichen Typen:** Andreas'
Auftrag nach dem "cf"-Piloten: "kannst du... mit den weiteren Hardware-Typen nach dem gleichen
Muster umbauen". Zwei Gruppen, unterschiedlich viel Arbeit:

1. **quicc/mc6845/framebuf/clut** -- lebten dank der frueheren "6.6"-Migration (2026-08-11) bereits
   in eigenen Dateien (`src/devices/<typ>/`). Brauchten nur einen neuen `q9_devdesc_t`-Eintrag am
   Dateiende (noch OHNE `extra_fields` -- keiner dieser vier Typen hat bisher ein Config-Schema,
   sie werden weiterhin hartcodiert instanziiert). `framebuf` bekam bewusst `use_table_default=0`
   (liegt bei $FD000000, unterhalb des Fast-Table-Clusters -- die Tabelle greift dort nie).
2. **duart68681/rtc72421/timer_irq** -- die letzten drei Typen, die noch gebuendelt in `q9board.c`
   steckten. Komplette Verschiebung nach `src/devices/duart68681/`, `src/devices/rtc72421/`,
   `src/devices/timer_irq/`, exakt nach dem "cf"-Muster: reine Verschiebung der Register-/
   Dispatch-Logik, KEINE Verhaltensaenderung. **Wichtiger Unterschied zu "cf":** anders als CF
   (das schon vor der Migration eine eigene, mehrfach instanziierbare Struct `q9_cf_t` hatte)
   stecken DUART/RTC/Timer-Zustand weiterhin direkt IN `q9_board_t` -- eine Struct-Extraktion
   waere ein deutlich groesserer, riskanterer Schritt gewesen (viele Aufrufstellen von
   `q9_board_t` im ganzen Baum) und war fuer Andreas' eigentliche Anforderung ("ein eigenes
   Sourcefile") nicht noetig. `q9_board_uart_irq_pending()`/`q9_board_poll_timer()` wurden dabei
   `static` (verifiziert: kein externer Aufrufer ausser der jeweils eigenen Vtable). `q9board.c`
   ist dadurch von 1077 auf ca. 200 Zeilen geschrumpft -- enthaelt jetzt nur noch die reine
   RAM/ROM/REMAP-Speicherlogik (5.2a) plus `q9_board_init`/`_reset`/`_rom_load` und die duenne
   `q9_board_cf_attach`-Bruecke.

**Dabei gefundene/geloeste Kopplungsfalle:** `devdesc.c`s Registry `g_devdesc_registry[]`
referenziert JEDEN registrierten Typ unbedingt (statisches Array, kein Lazy-Loading) -- jeder
Aufrufer von `devdesc.c` (Root-Binary, mehrere Testziele, der Editor) braucht deshalb ALLE
gelisteten Geraete-Dateien im Link, nicht nur die, die er tatsaechlich benutzt. Neue
Makefile-Variable `DEVDESC_SRC` fasst das an einer Stelle zusammen (statt Wiederholung/Drift an
4+ Aufrufstellen). Zusaetzliche Folge: `duart68681.c`/`rtc72421.c` brauchen echte `q9_hal_*`-
Symbole (Konsole/Uhr) -- dafuer wird ueberall dort der bereits vorhandene `test/07_hal_stub.c`
(bisher nur fuer den CF-Sektortest gedacht, jetzt umbenannt/umgewidmet als allgemeiner Test-Stub)
mitgelinkt, NICHT die volle native HAL. Auch `devreg.c` (dessen alte, kaum genutzte Typ-Registry
`g_device_types[]` `q9_devtype_duart68681` fest referenziert) musste auf ein explizites Include
von `duart68681.h` statt der bisherigen transitiven Weiterreichung durch `q9board.h` umgestellt
werden (letzteres reicht seit dieser Runde bewusst NICHTS mehr transitiv weiter, s. dortiger
Kommentar -- vermeidet eine Include-Zirkel-Falle, da die neuen Geraete-Header umgekehrt `q9board.h`
brauchen, um `q9_board_t` zu sehen).

`make test` (Root, inkl. `test-devdesc` mit jetzt 8 registrierten Typen) UND `make test`/
`make demo-integration` (Editor) komplett gruen, kein neues Warning. Zwei weitere echte Boot-Tests
(vor/nach der DUART/RTC/Timer-Migration) bit-identisch -- RTC-Uhrzeit korrekt, DUART-Konsole/Login
funktioniert, Timer treibt OS-9 wie gewohnt an. N/D-Rauchtest (CF-Image-Slots) erneut gruen als
Regressionscheck.

**Noch offen (unveraendert seit der Dreissigsten Runde, s.o.):** `nettty` nachziehen (Sonderfall,
s. Abschnitt 4), generische `boardcfg.c`-Abschnittserkennung fuer beliebige Typnamen, echter
Typ-Auswahl-Dialog im Editor.

**Zweiunddreissigste Runde (2026-08-21) -- nettty, der letzte Typ:** Andreas' Erinnerung an eine
fruehere Absprache: "wir hatten schon mal besprochen dass wir jedes Geraet quasi einzeln behandeln
wollen, mit eigenem Descriptor mit einer eigenen Adresse, vorzugsweise in dem Array" (deckt sich
mit ARBEITSPLAN 5.18: die x1..x8-Basisadressen wurden schon am 2026-08-14 auf eigene 256-Byte-
Slots umgestellt, GENAU zu diesem Zweck -- nur der devreg-Eintrag selbst blieb bis jetzt EIN
gemeinsamer statt acht separater).

**Kernaenderung:** `q9_nettty_attach()` (`src/devices/nettty/nettty.c`, NEU) registriert jetzt ACHT
separate `q9_devreg_add()`-Aufrufe statt einem -- je einer pro Kanal mit `dev->state = &channels[i]`
(der eigene "Descriptor") und der eigenen, bereits vorhandenen Basisadresse. Vorteile, die sich
dabei von selbst ergaben:
- `nettty_dev_read8/write8/irq_pending` werden trivial (kein Suchen mehr ueber alle acht Kanaele --
  `dev->state` zeigt direkt auf den richtigen).
- Kein `irq_vector_fn` mehr noetig: jeder Kanal traegt seinen (weiterhin festen) Vektor direkt in
  `dev->irq_vector`. Der bereits bestehende generische IACK-Mechanismus
  (`devreg_pending_level_held()`, `m68krt.c`) liefert dadurch GANZ VON SELBST das schon vorher
  dokumentierte Verhalten "Vektor des ERSTEN Kanals mit gesetztem RX-Ready-Bit" -- ohne die
  bisherige manuelle Nachbildung in `nettty_dev_irq_vector()`.

**Gefundene Besonderheit -- Musashi-Entkopplung:** nettty ist der EINZIGE der neun Hardware-Typen,
der bisher (schon vor dieser Runde) `m68k_set_irq()` DIREKT aufruft -- nicht nur ueber die
generische devreg-Poll-Schleife, sondern zusaetzlich bei jedem einzelnen ankommenden Byte
(minimale Latenz). Ein direktes `#include "m68k.h"` in `nettty.c` haette JEDEN Aufrufer von
`devdesc.c` (den Editor, die leichten Testziele) gezwungen, die volle Musashi-CPU-Kernobjekte
mitzulinken -- nur wegen eines Typs, den diese Aufrufer nie ausfuehren. Geloest mit einem simplen
Funktionszeiger-Hook (`q9_nettty_set_irq_hook()`), den `m68krt.c` beim echten Attach mit
`q9_m68krt_set_irq` (dessen bereits vorhandenem duennen Musashi-Wrapper -- "damit [der Aufrufer]
nicht direkt gegen third_party/musashi linken muss", exakt dasselbe Muster, hier nur eine Ebene
weitergereicht) verdrahtet. Ungewurzelt (Hook `NULL`, z.B. in Tests) ist eine IRQ-Anforderung ein
stilles No-op.

**Verifikation:** `nettty.c` kompiliert eigenstaendig ohne Musashi/Warning. `make test` (Root +
Editor) komplett gruen. Echter Boot-Test bit-identisch (8 devices online, Login, funktionierende
Shell). `test/09_test_io_dispatch.c`s bereits vorhandener nettty-Abschnitt (5 Pruefungen: eigene
Adresse je Kanal, Isolation zwischen Kanaelen, alte Adresse liefert "kein Geraet") bleibt gruen --
direkter Nachweis, dass die acht neuen devreg-Eintraege auf Register-Ebene korrekt funktionieren.
**Interaktiver Telnet-Login-Test per Skript ergab eine Ueberraschung:** ein selbst geschriebener
Multi-Kanal-Test (zwei gleichzeitige Telnet-Verbindungen) bekam keinen Login-Prompt -- per
Gegenprobe (Stand VOR dieser Runde mit `git stash` gebaut) bestaetigt: **dasselbe Verhalten trat
bereits am Vorher-Stand auf**, also keine Regression dieser Runde, sondern eine vorbestehende
Eigenart der lokalen Testumgebung/des verwendeten Images (nicht weiter verfolgt, ausserhalb des
Rahmens dieser Runde).

Damit sind **alle neun heutigen Hardware-Typen** (cf/quicc/mc6845/framebuf/clut/duart68681/
rtc72421/timer_irq/nettty) auf das einheitliche `q9_devdesc_t`-Muster umgestellt.

**Dreiunddreissigste Runde (2026-08-21) -- der REMAP-Trigger als zehntes Geraet:** Andreas' eigene
Idee, unangekuendigt eingebracht: "da muss es noch so ein Bootflag geben, nach dem Starten liegt
das ROM ab Adresse 0 (und gespiegelt bis oben), und wenn auf eine bestimmte Adresse zugegriffen
wird, wird unten das RAM eingeblendet" -- ob sich das ebenfalls als simuliertes Geraet einbauen
liesse, und wie der dadurch entstehende Zustand an die anderen Module weitergegeben wird.

**Antwort auf die Zustandsfrage:** kein neuer Mechanismus noetig -- `dev->state` zeigt (wie schon
bei duart68681/rtc72421/timer_irq) direkt auf das gemeinsame `q9_board_t`, der bereits vorhandene
RAM-Fast-Path in `m68krt.c` liest denselben `remapped`-Wert weiterhin ueber den globalen
`g_board`-Zeiger.

**Bewusst zweigeteilter Vorschlag, nur Teil 1 umgesetzt:** der REMAP-Trigger selbst
($FFFF8000-$FFFF8FFF, reiner Adress-Trigger ohne Datenwert, kein IRQ) wandert nach
`src/devices/remap/remap.c` -- exakt das `timer_irq`-Muster ("reiner Adress-Trigger"). Die
RAM/ROM-**Interpretation** (welcher Speicherbereich nach dem Trigger wo erscheint) bleibt
ausdruecklich in `q9board.c`/`m68krt.c`s RAM-Fast-Path -- Performance (heissester Pfad der
gesamten Emulation) und die bestehende, bewaehrte Fast-Path-Logik waeren durch eine
Geraete-Indirektion nur verschlechtert worden, ohne einen Nutzen fuer die Editor-Zielsetzung
("Hardware hinzufuegen") zu bringen: REMAP ist kein Geraet, das ein Nutzer je an-/abwaehlen wuerde.

**Ergebnis:** `q9board.c`s `board_is_remap_reg()` entfaellt vollstaendig, `board_read_byte`/
`board_write_byte` kennen nur noch RAM/ROM. `test/09_test_io_dispatch.c` bekam einen sechsten
Pruefblock (Lese- UND Schreibzugriff auf Basis/oberes Fensterende schalten `board.remapped`
korrekt, der Trigger liefert dabei immer 0) -- **dieser Test deckte einen echten Fehler auf**: die
eigentliche `q9_devreg_add()`-Registrierung in `m68krt.c` fehlte zunaechst (nur Kommentare/Historie
waren schon angepasst), OS-9 waere ohne diesen Fund beim naechsten Boot an der RAM-Umschaltung
gescheitert. Nach dem Fix: `make test` (Root + Editor) komplett gruen, echter Boot-Test
(`docs/q9board.example.q9`-Profil, angepasste Pfade) im Transkript bis zum vollstaendigen Login UND
funktionierender Shell bestaetigt ("8 devices online", `Process #22 logged on`, `/dd/HOME/ROOT#`-
Prompt) -- die RAM/ROM-Umschaltung funktioniert im echten Betrieb weiterhin bit-identisch. (Das
automatisierte `expect`-Skript selbst meldete trotzdem "LOGIN FEHLGESCHLAGEN", weil sein
Prompt-Regex `\$` auf den tatsaechlichen Prompt `/dd/HOME/ROOT#` dieses Profils nicht passt --
Skript-/Profil-Mismatch, keine REMAP-Regression, per manueller Transkript-Pruefung verifiziert.)

Damit sind **alle zehn heutigen Hardware-/Adressraum-Typen** auf das einheitliche Muster umgestellt.

**Vierunddreissigste Runde (2026-08-21) -- Editor zeigt echte Hardware-Eintraege statt Demo-
Platzhalter:** Andreas' Rueckfrage "gibt es noch was zu tun an Q9-Flux" fuehrte zur Klaerung, was
das verbleibende "Hardware hinzufuegen"-Ziel ueberhaupt noch braucht: bei genauerem Hinsehen haben
von den zehn Hardware-Typen heute nur "cf" ueberhaupt Config-Felder -- die anderen neun sind fest
verdrahtete Board-Peripherie ohne eigene Config-Optionen, und CF kann schon per N/D-Taste angelegt/
geloescht werden (Dreissigste Runde). Andreas' Entscheidung (per `AskUserQuestion`-Rueckfrage):
**"Editor zeigt echte Config statt Demo-Platzhalter"** -- statt generisches Mehrfach-Anlegen fuer
ALLE Typen (ergibt fuer DUART/RTC/Timer/etc. physikalisch keinen Sinn, nur ein Chip auf dem Board)
werden die zwanzig hartcodierten `integration_demo.c`-Demo-Eintraege (erfundene Basisadressen/
Werte, s. Kopfkommentar-Historie) durch echte, aus `q9_devdesc_get()` abgeleitete Eintraege
ersetzt -- EIN Eintrag pro registriertem Typ (Name = `devdesc->desc`, keine zweite, drift-
anfaellige Kopie der Beschreibung mehr), AUSSER "cf" (hat schon seine eigene CF-Image-#N-
Darstellung). `g_list_items[]`/das fruehere Compile-Zeit-Macro `BASE_ITEM_COUNT` wurden dafuer zu
Laufzeit-Werten (`init_items()`, `g_base_item_count`) -- die Anzahl haengt jetzt von
`q9_devdesc_count()` ab, nicht mehr von einer festen Liste. **Bewusst OHNE Basis-/Endadresse im
Eintrag:** `devdesc.h`s eigener Kopfkommentar dokumentiert ausdruecklich, dass Basis/Groesse KEIN
Teil von `q9_devdesc_t` sind (das ist eine `devreg`-Laufzeit-Instanz-Eigenschaft) -- der Editor
bootet das Board nicht und haette dafuer keine echte Quelle, ein erfundener Wert waere wieder nur
ein Demo-Platzhalter gewesen. Verifiziert: `make test` (Root + Editor) komplett gruen, echter
Pseudo-Terminal-Rauchtest zeigt alle neun devdesc-Eintraege mit ihren echten Kernel-Beschreibungen
(z.B. "68681-DUART (Konsole, Kanal A)", "REMAP-Trigger (ROM-Spiegel -> RAM-Umschaltung)"), keinen
der alten erfundenen Eintraege mehr, UND bestaetigt, dass Taste N weiterhin korrekt einen
CF-Image-Slot direkt hinter den neun Hardware-Eintraegen anlegt.

## 3. Nach der Auswahl: weitere Bereiche

- Kurzbeschreibung der gewaehlten Config
- **"Allgemeine Einstellungen"** — Inhalt noch NICHT definiert (Andreas: "faellt mir im Moment
  gar nichts ein"), offener Platzhalter
- **Hardware** (s. Abschnitt 4)
- unten Buttons: **Start / Speichern / Beenden** (Andreas hat sich beim genauen Zuschnitt/
  Wortlaut noch nicht endgueltig festgelegt — "später umentschieden", als offen markiert)

**"Start"-Mechanik GEKLAERT + Grundlage FERTIG (2026-08-16):** Andreas' Entscheidung -- der
Emulator wird als **Kindprozess im selben Terminal** gestartet (nicht per `exec()` selbst zum
Emulator werden); nach dessen Ende kehrt die Kontrolle zum Editor zurueck (eigenen Bildschirm
per `q9_screenbuf` neu zeichnen, statt im nackten Shell-Prompt zu landen). Umgesetzt:
`tools/q9-flux-editor/src/q9_procspawn.h/.c` (`q9_procspawn_run()`) -- POSIX (`fork`/`execv`/
`waitpid`) implementiert UND per echtem End-to-End-Test verifiziert (`build/macos/q9.exe` als
Kindprozess gestartet, bis "8 devices online" gebootet, per SIGTERM beendet, Rueckgabewert `-2`
[abnormal beendet] korrekt erkannt -- nicht nur der isolierte Selbsttest per Selbstaufruf-Trick,
`make test-procspawn`, 4 Checks). Terminal-Rohmodus verschachtelt sich dabei von selbst richtig
(s. Header-Kommentar). **Windows-Zweig** (`CreateProcess`/`WaitForSingleObject`) nach demselben
Muster wie `src/hal/windows/hal_windows.c` geschrieben, mangels Windows-Host hier UNGETESTET.
**Noch offen:** die eigentliche Verdrahtung an einen "Start"-Knopf (braucht erst die Tastatur-/
Ereignisschleife, s.u.), und welchen Config-Pfad/welche Argumente genau uebergeben werden.

## 4. Hardware-Bereich

An erster Stelle immer **Speicher** (Memory), danach **CPU-Auswahl**, dann Button
**"Hardware hinzufuegen"** fuer beliebig viele weitere Geraete-Instanzen.

**Architektur-Grundstein FUER dieses Ziel FERTIG (2026-08-20/21):** `q9_devdesc_t`
(`src/kernel/devdesc.h/.c`) vereint pro Hardware-Typ Vtable + Feldbeschreibung + Fast-Table-Flag an
einem Ort. Nach dem Pilot am Beispiel "cf" (2026-08-20, "Dreissigste Runde" in Abschnitt 2) folgten
direkt danach ALLE verbliebenen Typen nach demselben Muster ("Einunddreissigste Runde",
2026-08-21): quicc/mc6845/framebuf/clut (lebten bereits in eigenen Dateien seit "6.6", brauchten
nur einen `q9_devdesc_t`-Eintrag) sowie duart68681/rtc72421/timer_irq (komplette Verschiebung aus
`q9board.c` nach `src/devices/duart68681/`, `src/devices/rtc72421/`, `src/devices/timer_irq/`,
analog zu "cf" -- bit-identisch per Boot-Test bestaetigt: RTC-Uhrzeit, DUART-Konsole, Timer-Tick
alle unveraendert). `q9board.c` ist damit auf die reine RAM/ROM/REMAP-Speicherlogik geschrumpft
(1077 -> ~200 Zeilen seit Beginn der Vereinheitlichung). **`nettty` (Netz-Terminals) ebenfalls
FERTIG (2026-08-21, "Zweiunddreissigste Runde"):** Andreas' Erinnerung an eine fruehere Absprache
("jedes Geraet einzeln behandeln, mit eigenem Descriptor, eigener Adresse, im Array") fuehrte zu
einer echten Verbesserung -- statt EINEM gemeinsamen devreg-Eintrag fuer alle acht Kanaele (mit
interner Suche bei jedem Zugriff) jetzt ACHT separate Eintraege, je einer mit eigener Adresse
(bereits seit 2026-08-14 vorhanden) und eigenem Zustand (`&channels[i]` als "Descriptor"). Dabei
gefunden: nettty ruft als EINZIGER der neun Typen `m68k_set_irq()` DIREKT auf (fuer minimale
Latenz bei ankommenden Bytes) -- ein Funktionszeiger-Hook (`q9_nettty_set_irq_hook()`) entkoppelt
das von Musashi, sonst haette jeder `devdesc.c`-Aufrufer (Editor, leichte Testziele) die volle
CPU-Kernobjekte mitlinken muessen. **Zehntes Geraet FERTIG (2026-08-21, "Dreiunddreissigste
Runde"):** der REMAP-Trigger ($FFFF8000-$FFFF8FFF, Boot-ROM-Spiegel -> RAM-Umschaltung), Andreas'
eigener Vorschlag -- nach `src/devices/remap/remap.c` umgezogen (reiner Adress-Trigger, kein
Datenwert, kein IRQ), die eigentliche RAM/ROM-Interpretation bleibt bewusst im Performance-
Fast-Path von `q9board.c`/`m68krt.c`. Damit sind **ALLE ZEHN heutigen Hardware-/Adressraum-Typen**
auf das einheitliche Muster umgestellt.

**Editor zeigt jetzt echte Hardware-Eintraege (2026-08-21, "Vierunddreissigste Runde"):** Andreas'
Rueckfrage "gibt es noch was zu tun" + Klaerung per `AskUserQuestion` ergab: von den zehn Typen hat
nur "cf" ueberhaupt Config-Felder (die anderen neun sind fest verdrahtete Board-Peripherie, genau
EIN Chip pro Typ) -- generisches Mehrfach-Anlegen fuer ALLE Typen ergaebe fuer DUART/RTC/Timer/etc.
keinen Sinn. Statt der grossen, noch offenen Architektur-Frage (generische `boardcfg.c`-
Abschnittserkennung fuer beliebige Typnamen + echter Typ-Auswahl-Dialog) wurde deshalb der kleinere,
sofort werthaltige Schritt umgesetzt: die zwanzig hartcodierten `integration_demo.c`-Demo-Eintraege
(erfundene Adressen/Werte) sind komplett durch echte, aus `q9_devdesc_get()` abgeleitete Eintraege
ersetzt (Name = `devdesc->desc`, EINE Quelle statt einer zweiten, drift-anfaelligen Kopie) --
"cf" ausgenommen (hat schon die echte CF-Image-#N-Darstellung mit Anlegen/Loeschen). **Weiterhin
offen, jetzt bewusst zurueckgestellt** (kein akuter Nutzen ohne echte Mehrfach-Instanziierung
ausserhalb von "cf"): generische `boardcfg.c`-Abschnittserkennung fuer beliebige Typnamen, und der
eigentliche Typ-Auswahl-Dialog im Editor.

### 4.1 CPU-Auswahl — technisch machbar, geringer Aufwand

Musashi unterstuetzt den CPU-Typ-Wechsel bereits nativ ueber `m68k_set_cpu_type()`
(`third_party/musashi/m68k.h`) — `m68krt.c` nutzt das schon HEUTE versteckt hinter der
Diagnose-Umgebungsvariable `Q9_CPU=ec030` (`q9_m68krt_init()`, s. dort). Eine echte Auswahl ist im
Kern nur, diesen einen Aufruf konfigurierbar zu machen statt ihn zu verstecken — **geringer
Aufwand, Andreas' Bedingung ("wenn nicht zu viel Arbeit") ist erfuellt.**

Verfuegbare Typen (Musashi-Enum): 68000, 68010, 68EC020, 68020, 68EC030, 68030, 68EC040, 68LC040,
68040.

**Ehrlicher Vorbehalt:** nicht jede Wahl bootet das echte OS-9-Image erfolgreich (OS-9/68030
erwartet eine PMMU, die z.B. 68000/68010 gar nicht haben) — das ist keine neue Einschraenkung
(heute schon so bei `Q9_CPU=ec030` gedacht als Diagnose-Vergleich, kein Vollbetrieb), wird durch
eine echte Auswahl im UI nur sichtbarer.

**Backend-Seite FERTIG (2026-08-15):** `m68krt.h` bekam `q9_cpu_type_t` (eigene, kleine
Aufzaehlung -- Musashis `M68K_CPU_TYPE_*` bleibt Implementierungsdetail von `m68krt.c`, kein
Header-Leck), `q9_m68krt_init()` nimmt jetzt einen echten `cpu`-Parameter statt der versteckten
Env-Var-Abfrage (die bleibt als Diagnose-Override erhalten, greift aber nur noch beim Default).
Neuer `[board]`-Key `cpu` in `boardcfg.c` (Whitelist-Validierung, klare Parse-Fehlermeldung bei
Tippfehlern), neues `devschema.c`-Schema `"board"` mit dem `cpu`-Feld (ENUM). Verifiziert: `make
test` 0 Fails (neue Checks in `test/08_test_devschema.c`), echter Boot-Test mit `cpu = 68030`
(bootet bis Login, identisch zum bisherigen Default) UND mit `cpu = 68000` (Config wird korrekt
uebernommen, bootet erwartungsgemaess NICHT bis Login -- PMMU-Vorbehalt s.o., kein Bug) UND mit
ungueltigem `cpu = 68060` (klare Parse-Fehlermeldung, sauberer Abbruch). **Noch offen:** das
eigentliche UI-Auswahlfeld selbst (das braucht erst das Dialog-/Bildschirmpuffer-System aus
Abschnitt 2, s. `tools/q9-flux-editor/` Stand) -- diese Ergaenzung deckt nur die Backend-/Config-
Seite ab.

### 4.2 Beispiel-Feldsatz: Speicher (RAM/ROM/NVRAM)

| Feld | Typ | Bemerkung |
|---|---|---|
| Name | String | |
| Description | Kurztext | z.B. "Systemspeicher mit direktem CPU-Zugriff" |
| Startadresse | Hex, 32 Bit | |
| Endadresse | Hex, 32 Bit | |
| ColorID | Zahl 0-15 | knuepft an "colored RAM" aus ARBEITSPLAN 5.19 an |
| Schreibzugriff | Bool Ja/Nein | Ja = RAM-Simulation, Nein = ROM-Simulation |
| Initialisierung | Dateiauswahl `~/.q9-flux/*.img` | fuer ROM-Simulation (Preload-Image) |
| Save after Session | Bool Ja/Nein | fuer NVRAM-Simulation (Inhalt nach Ende zuruecksichern) |
| Descriptor | (s. Abschnitt 5) | bei Speicher: "no" |

**Offen (2026-08-15):** ist "Initialisierung" nur bei ROM (Schreibzugriff=Nein) sichtbar/
relevant, oder soll auch RAM mit einem Preload-Image starten koennen? Noch nicht entschieden --
technisch waere Letzteres kein Mehraufwand (dieselbe Dateiauswahl-Logik), nur die Frage, ob es
im UI ueberhaupt als sinnvolle Kombination angeboten wird.

### 4.3 Beispiel-Feldsatz: CF-Interface ("cfide")

| Feld | Typ | Beispielwert |
|---|---|---|
| Name | String | cfide |
| Description | Kurztext | "CF Cardreader direkt on memory bus" |
| Startadresse | Hex, 32 Bit | 0xFFFFFF10 |
| Endadresse | Hex, 32 Bit | 0xFFFFFF3F |
| ColorID | Zahl 0-15 | 15 (IO-Bereich) |
| Master | Bool Ja/Nein | yes |
| Image | Dateiauswahl `~/.q9-flux/*.hda` | fuer CF-Image |
| Descriptor | (s. Abschnitt 5) | yes |

**Bestaetigt genau das devschema.c-Muster** (s. dortiger Kopfkommentar): zwei Geraetetypen, zwei
voellig unterschiedliche Feldlisten, generisch beschrieben statt hartkodiert. `devschema.c`s
Pilot-Schema fuer "cf" deckt die CF-Felder (bis auf `master` als Enum statt Bool) bereits ab; das
Speicher-Schema (`ram`/`rom`) fehlt dort noch.

## 5. Descriptor-Feld — bewusst vereinfacht

Urspruenglich gedacht als "kann ich hier auch gleich einen OS-9-Descriptor ERZEUGEN" — aber einen
generierten Descriptor tatsaechlich INS IMAGE zu bekommen ist ein eigener, nicht-trivialer
Workflow (MWOS-Build + ToolShed-Transfer, s. ARBEITSPLAN 5.20 "Descriptor-Generator", dort noch
💤/nicht begonnen). **Entscheidung (Andreas, 2026-08-13): fuer den Editor KEINE Abhaengigkeit
dorthin.** Das Feld bleibt eine reine Informations-Flagge ("wird fuer dieses Geraet ein
Descriptor gebraucht, ja/nein") — der Anwender ist selbst dafuer verantwortlich, dass passende
Descriptoren im Image vorhanden sind. Damit entfaellt auch die zuvor befuerchtete Notwendigkeit,
im Schema-Format bedingte Folgefelder ("wenn Descriptor=yes, zeige weitere Felder") abzubilden —
zumindest fuer diesen Fall.

**Nachtrag (2026-08-14):** Doch bedingte Folgefelder gebraucht — Andreas wollte zusaetzlich einen
Descriptor-NAMEN je Geraet ("descriptorName") einfuehren, fuer ALLE Geraetetypen, nicht nur CF.
`devschema.c` bekam dafuer einen minimalen, generischen `depends_on`/`depends_on_value`-
Mechanismus (`q9_devschema_field_relevant()`) -- "descriptorName" ist nur relevant, wenn
"descriptor"=yes. Dabei aufgefallen: `descriptor` war bei CF schon LANGE ein echtes, geparstes
`.q9`-Schluesselwort mit STRING-Bedeutung (Descriptor-Name, nicht Ja/Nein) -- die projektweite
Bool-Vereinheitlichung musste deshalb `boardcfg.c` UND alle 9 betroffenen `.q9`-Dateien im Repo
mitziehen (nicht nur das Schema), s. ARBEITSPLAN 6.7. `.q9`-Dateien ausserhalb dieses Repos mit
der alten Syntax brauchen manuelle Nacharbeit.

## 6. Autostart

Zusaetzlich zum interaktiven Launcher soll es einen Weg geben, ihn zu UEBERSPRINGEN und direkt zu
booten — per CLI-Options-Parameter oder per Checkbox/Key in der Config-Datei selbst (genau
festzulegen). Wichtig fuer Skript-/Testgebrauch (s. `make test-rvnuttx` & Co., die den Emulator
heute direkt mit Argumenten aufrufen) — der bestehende CLI-Weg (`q9.exe <config>[.q9] [optionen]`)
soll dadurch nicht verschwinden.

**FERTIG (2026-08-22, Fuenfunddreissigste Runde):** genau so umgesetzt -- der Launcher greift NUR
noch, wenn `q9.exe` OHNE jedes Argument aufgerufen wird (`src/hal/posix/hal_posix.c main()`); eine
Config-Datei ODER `--rom` als Argument bootet weiterhin sofort direkt, exakt wie bisher (kein
Verhaltensunterschied fuer bestehende Skripte/Tests). Kein separates CLI-Flag noetig -- die
Abwesenheit jedes Arguments IST das Signal.

## 7. `~/.q9-flux`-Verzeichnis

Andreas' Vorschlag, als einfachster Default akzeptiert: Config-Dateien liegen unter `~/.q9-flux/`.
**Plattform-Detail:** "~" muss zur Laufzeit plattformabhaengig aufgeloest werden — `HOME` unter
macOS/Linux, `USERPROFILE` unter Windows (kein natives "~" dort) — sonst inhaltlich identisch.

## 8. Ausdruecklich vertagt / noch offen

- **Netzwerk-Konfiguration im Editor** — Andreas: "wird dann wieder komplizierter, muessen wir
  noch klaeren was wir da genau einsetzen wollen". Kein Entwurf bisher. **Status der Backends
  geklaert (2026-08-15), Entwurf des Editor-Feldes selbst weiterhin offen:**

  | Backend | macOS | Windows | Linux |
  |---|---|---|---|
  | Mini-NAT (slirp) | bestaetigt (Boot-Test 2026-08-15) | Code da (`q9_sockcompat.h` Winsock2-Zweig), ungetestet | Code da (POSIX-Zweig), ungetestet |
  | vmnet | laeuft | -- (kein Windows-Aequivalent) | -- |
  | bridge/BPF | Code da, Bridging-Test mit echter 2. NIC noch offen | -- | -- (BSD-spezifisch) |
  | TAP (Linux) / npcap (Windows) | -- | nicht gebaut | nicht gebaut |

  Mini-NAT (slirp) ist damit das einzige Backend, das im Editor als "laeuft auf allen drei
  Zielplattformen" angeboten werden sollte (deckt sich mit ARBEITSPLAN 5.21, wo Mini-NAT
  ebenfalls als portabler Default vorgesehen ist); vmnet/bridge bleiben macOS-Extras. Der
  eigentliche Windows-/Linux-Testlauf ist weiterhin bewusst vertagt.
- **"Allgemeine Einstellungen"** — Inhalt unbekannt, Platzhalter.
- **Start/Speichern/Beenden-Buttons** — genauer Zuschnitt/Wortlaut nicht final.
- **Umfang: nur Editor oder auch Emulator-Start?** — laut Abschnitt 3 soll "Start" den Emulator
  direkt aus dem Editor heraus starten (wie beim alten Prototyp), aber nicht explizit erneut
  bestaetigt seit dem Neuanfang.
- **Neu anlegen vs. nur bearbeiten** — ob der Editor auch komplett neue Configs von Grund auf
  erzeugen kann (braeuchte Zugriff auf die Geraete-Typ-Registry aus `devreg.c`, nicht nur auf
  `devschema.c`) ist nicht gesondert bestaetigt, aber implizit durch "Hardware hinzufuegen" nahegelegt.
- ~~**Speicher-Schema in devschema.c**~~ — 2026-08-13 erledigt: Schema `memory` angelegt (9 Felder,
  s. `q9_field_kind_t` Q9_FIELD_BOOL fuer die Ja/Nein-Felder), vorausschauend ohne C-Struct-
  Gegenstueck (haengt an 5.19). **Dabei gefunden:** `descriptor` bedeutet bei `cf` etwas anderes
  (String, Descriptor-NAME) als hier vorgesehen (Bool, Info-Flagge) — beide Bedeutungen bestehen
  aktuell nebeneinander in `devschema.c`, noch nicht vereinheitlicht.
- ~~**ANSI-Testfall**~~ — 2026-08-14 erledigt: `tools/q9-flux-editor/` angelegt (Grundstein des
  eigentlichen Editors, C99, kein Framework). `src/q9_ansi.h/.c`: rohe Cursor-Position (CUP)/RGB-
  Vordergrund-Farbe (SGR-Truecolor)/Clear/Cursor ein-aus-blenden, jede Funktion schreibt nur Text
  in einen Puffer (kein direktes stdout). Automatischer Selbsttest (`make test-ansi`, jetzt Teil
  von `make test`): Byte-Vergleich gegen den ANSI/VT100-Standard UND Ruecklese-Parse ("Zeile X
  angefordert" == "Zeile X tatsaechlich in den erzeugten Bytes", inkl. der konkreten "3 Zeilen
  tiefer, 4 Zeichen nach links"-Verschiebung aus der urspruenglichen Frage), Farbwert-Klemmung,
  Pufferueberlauf-Schutz per Sentinel-Bytes. **`make -C tools/q9-flux-editor demo`** zeigt drei
  farbige Zeilen an festen Positionen in einem echten Terminal -- fuer Andreas' spaetere
  Sichtpruefung/Verschiebe-Anweisungen, noch nicht selbst durchgefuehrt (kein Terminal-Zugriff
  waehrend der autonomen Nachtsession). Noch KEIN Dialog-/Bildschirmpuffer-System (fuer den
  modalen Config-Auswahl-Dialog aus Abschnitt 2), keine Config-Anbindung, kein echter Launcher --
  reine Positions-/Farb-Grundlage.
- **Aufraeumen von `tools/q9-launcher-prototype/` + `third_party/tvision`** — erst wenn ein
  Nachfolger steht.

## Verwandt

- `src/kernel/devschema.h/.c` — Feld-Schema-Infrastruktur (Grundstein, bereits gebaut)
- ARBEITSPLAN 6.7 — Config-Schema-Generalisierung (uebergeordneter Kontext)
- ARBEITSPLAN 5.19/5.20 — Board-Config-Datei bzw. Descriptor-Generator (Bestand/Referenz)

#─────────────────────────────────────────────────────────────────────────────────────────────────
# EOF Q9FLUX_EDITOR_de.md                                                                 Ver. 1.00
#─────────────────────────────────────────────────────────────────────────────────────────────────
