#═════════════════════════════════════════════════════════════════════════════════════════════════
# File:   Q9FLUX_EDITOR_de.md                                                             Ver. 1.60
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
#═════════╧══════╧═════════════════════════════════════════════════════════════════════════╧══════

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

**Noch offen:** Buttons/Textfelder (Aussehen noch nicht mit Andreas geklaert, s. Abschnitt 3 unten
"genauer Zuschnitt/Wortlaut nicht final") und das eigentliche Zusammensetzen aller Bausteine
(q9_screenbuf/q9_widgets/q9_procspawn/q9_input) zu echten, bedienbaren Bildschirmen -- alle
Grundbausteine sind jetzt da, es fehlt noch das Programm selbst. Maus-Unterstuetzung weiterhin
nicht angefangen.

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
