#═════════════════════════════════════════════════════════════════════════════════════════════════
# File:   Q9FLUX_EDITOR_de.md                                                             Ver. 1.00
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

## 3. Nach der Auswahl: weitere Bereiche

- Kurzbeschreibung der gewaehlten Config
- **"Allgemeine Einstellungen"** — Inhalt noch NICHT definiert (Andreas: "faellt mir im Moment
  gar nichts ein"), offener Platzhalter
- **Hardware** (s. Abschnitt 4)
- unten Buttons: **Start / Speichern / Beenden** (Andreas hat sich beim genauen Zuschnitt/
  Wortlaut noch nicht endgueltig festgelegt — "später umentschieden", als offen markiert)

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
Descriptor-NAMEN je Geraet ("descriptor_name") einfuehren, fuer ALLE Geraetetypen, nicht nur CF.
`devschema.c` bekam dafuer einen minimalen, generischen `depends_on`/`depends_on_value`-
Mechanismus (`q9_devschema_field_relevant()`) -- "descriptor_name" ist nur relevant, wenn
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
  noch klaeren was wir da genau einsetzen wollen". Kein Entwurf bisher.
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
