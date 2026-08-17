#═════════════════════════════════════════════════════════════════════════════════════════════════
# File:   Q9FLUX_EDITOR_de.md                                                             Ver. 2.30
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
