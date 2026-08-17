#═════════════════════════════════════════════════════════════════════════════════════════════════
# File:   Q9FLUX_EDITOR_de.md                                                             Ver. 3.40
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
#═════════╧══════╧════════════════════════════════════════════════════════════════════════╧══════

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
  Anzeige des aktuellen Verzeichnispfads im Dialog -- soll spaeter OPTIONAL moeglich sein

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
  Optionen" fuer die Listenansicht (kein eigener Task bisher, s. Runde 5 oben) und die bewusst
  vertagte Anzeige des aktuellen Verzeichnispfads im Dialog.

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
