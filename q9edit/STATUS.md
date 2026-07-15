# q9edit — Arbeitsstand fuer Fortsetzungen

Letzte Aktualisierung: 2026-07-15

Diese Datei ist der kurze Einstieg fuer Menschen und andere KI-Agenten. Vor
weiterer Arbeit zusaetzlich `README.md` und `docs/PROJEKTZIELE.md` lesen.

## Aktueller Stand

- Projektname: `q9edit`
- OS-9-Kommando/Modulname: `qe`
- Zielcompiler: Microware `xcc`, ANSI C89
- Ziel: OS-9/68000, zunaechst Q9-CB030 mit emuliertem 68030
- Terminal: OS-9 SCF + termcap; ANSI-Farben fuer `TERM=q9`
- curses ist keine Voraussetzung
- Editor-Kern: antirez/kilo, BSD-2-Clause
- unveraenderter Upstream-Stand `323d93b29bd89a2cb446de90c4ed4fea1764176e`
  unter `vendor/kilo/` importiert und per SHA-256 dokumentiert
- `qeprobe` und `qetermprobe` mit xcc erfolgreich gebaut und im Gast getestet
- OS-9-Terminalschicht fuer SCF, termcap und ANSI-Tasten ist implementiert
- Pfeiltasten sind ueber `/x1`/Port 2000 end-to-end nachgewiesen
- der komplette Editor wird als `q9edit/os9/CMDS/qe` mit xcc gelinkt

## Verifizierte SDK-Fakten

SDK-Wurzel auf dem aktuellen Host:

```text
/Volumes/SSD1TB/projects/MWOS
```

Werkzeuge und Wrapper:

```text
DOS/BIN/xcc.exe
DOS/BIN/os9make.exe
tools/macos/bin/os9make
tools/macos/bin/mwos-build
tools/macos/env/os9-toolchain.sh
```

Der macOS-Weg startet die Windows-Werkzeuge mit Wine. Die Umgebung setzt
`MWOS` auf den fuer Wine sichtbaren Pfad
`Z:/Volumes/SSD1TB/projects/MWOS`.

Relevante SDK-Dateien:

```text
MWOS/OS9/68030/PORTS/Q9/makefile
MWOS/OS9/SRC/DEFS/termcap.h
MWOS/SRC/DEFS/curses.h
```

`termcap.h` deklariert `tgetent`, `tgetflag`, `tgetnum`, `tgetstr`, `tgoto`
und `tputs`. `TERMLIBS` aus den MWOS-Templates bindet
`OS9/68000/LIB/termlib.l` ein.

Ein brauchbares xcc-Programm-Makefile-Muster ist:

```text
MWOS/SRC/SPF/INET/EXAMPLES/AF_INET.TCP/makefile
```

Es zeigt den normalen Ablauf C-Quelle -> `.i` -> `.r` -> OS-9-Programm-Modul
mit `CSTART`, `xcc` und dem OS-9-Linker.

## Noch nicht geklaert

1. Verhalten und Speicherbedarf des kompletten Editors im echten Gast.
2. Sichere Speicherroutine fuer einen spaeteren Stand (der erste Port schreibt
   bewusst einfach ueber C89-`fopen`/`fwrite`).

## Naechste Schritte

1. Das echte `qe` ueber `/x1` starten und interaktiv pruefen.
2. Laden, Aendern und Speichern automatisiert im Entwicklungsimage pruefen.
3. Dabei Speicherbedarf, lange Zeilen und OS-9-CR-Zeilenenden kontrollieren.

## Terminal-Plattformgrenze

Die API `src/platform/qe_platform.h` kapselt jetzt Raw-Modus, Tastendekodierung,
Fenstergroesse und Bildschirmausgabe. Der bisherige POSIX-Code wurde aus
`src/editor/qe.c` nach `qe_platform_posix.c` verschoben. Der strenge C89-Build
bleibt warnungsfrei. `make -C q9edit test-host` startet den Editor in einem
80x24-PTY, erkennt den farbig aufgebauten Bildschirm und beendet ihn mit
Ctrl-Q; der Test ist PASS. Vertrag und weitere Arbeit stehen in
`docs/PLATFORM_API.md`.

## ANSI-C89-Arbeitskopie

`vendor/kilo/kilo.c` bleibt unveraendert. Die Arbeitskopie liegt jetzt unter
`src/editor/qe.c`. Alle vom strengen Hostcompiler gefundenen C99-Sprachstellen
wurden ohne beabsichtigte Verhaltensaenderung auf C89 umgestellt. Folgender
Build ist warnungsfrei:

```text
make -C q9edit host
```

Die noch enthaltenen POSIX-Aufrufe sind in `docs/C89_PORT.md` aufgelistet und
bilden den naechsten Arbeitsschritt; der C89-Hostbuild ist noch kein xcc-Build
des Editors.

## Erfolgreicher Build 2026-07-15

Aufruf:

```text
make -C q9edit os9
```

Ergebnis:

```text
q9edit/os9/CMDS/qeprobe
```

Der reale Ablauf war `qeprobe.c` -> `RELS/k68k/qeprobe.i` ->
`RELS/k020/qeprobe.r` -> `CMDS/qeprobe`. Der erste Versuch deckte eine
doppelte Pfadangabe bei `-eas` plus `-fd` auf; im funktionierenden Makefile
lautet das Zwischenziel deshalb `-fd=qeprobe.r`, nicht der komplette RDIR-Pfad.

Der anschliessende Build mit `#include <termcap.h>`, `tgetent`, `tgetnum` und
`LIBS = $(TERMLIBS) $(MWOS_CSLLIBS)` war ebenfalls erfolgreich. Damit sind
xcc-Kompilierung, Programmlink und Termcap-Link auf dem Host nachgewiesen;
der Lauf im Gast steht noch aus.

## Sicherheits- und Repo-Regeln

- Proprietaere MWOS-Header, Bibliotheken und Binaries nicht in dieses Repo
  kopieren.
- Nur oeffentlichen BSD-Kilo-Code duerfen wir spaeter mit Lizenz vendoren.
- OS-9-Images bleiben lokal und werden nicht committet.
- ToolShed und Emulator duerfen nie gleichzeitig dasselbe Image schreiben.
- Fuer Entwicklung ein Klon-Image verwenden; `OS9SYS.hda` nicht ohne
  ausdrueckliche Freigabe veraendern.
- Bestehende untracked Testergebnisdateien im Q9-Root gehoeren zum laufenden
  Kernel-Arbeitsstand und bleiben unangetastet.

Das Skript `tools/deploy-os9.sh` erzwingt diese Regeln: Es verweigert das
Produktivimage und ein Image, dessen Pfad in der Kommandozeile eines laufenden
Q9-Emulators vorkommt. Standardmaessig deployt es den aktuellen `qeprobe` nach
`CMDS` eines explizit angegebenen Entwicklungsimages.

## Laufende Instanz beim letzten Status

Beim letzten Check am 2026-07-15 lief der Emulator mit
`local_images/OS9SYS.hda --net vmnet`. Dieses Produktivimage wurde von q9edit
nicht veraendert. Vor einem spaeteren Deployment Prozessliste erneut pruefen.

Danach wurde der Emulator beendet und ein eigener APFS-Klon angelegt:

```text
local_images/OS9SYS.q9edit-dev.hda
```

`qeprobe` wurde erfolgreich mit ToolShed nach `CMDS` dieses Klons kopiert.
Der erste Gastlauf startete und lud das Modul korrekt. In der vom automatischen
Test neu erzeugten **Konsolen-Login-Shell** lieferte `getenv("TERM")` keinen
Wert, obwohl das Bootskript zuvor `setenv TERM q9term` ausgegeben hatte.
Andreas hat klargestellt, dass `TERM` in den regulaer laufenden virtuellen
Shells gesetzt ist. Das ist daher ein Unterschied des Konsolen-Testwegs und
kein allgemeiner Befund ueber die `/x1`-bis-`/x8`-Shells. Der Konsolentest setzt
`TERM=q9` explizit; der verbindliche Editor-Test erfolgt spaeter ueber `/x1`
auf Port 2000 mit der dort real vorhandenen Umgebung.

Der zweite Gastlauf erreichte `tgetent`, stuerzte dort aber ab. Ursache ist mit
hoher Sicherheit der 2048-Byte-Termcap-Puffer als lokale Variable auf dem
kleinen OS-9-Programmstack. Er wurde daraufhin in statischen Speicher
verschoben. Diese Regel gilt auch fuer den Editor: grosse Zeilen-, Bildschirm-
und Termcap-Puffer nie ungeprueft auf den Stack legen.

Der dritte Lauf war vollstaendig erfolgreich:

```text
qeprobe: xcc OS-9 build works
qeprobe: termcap 'q9', 80 columns, 24 lines
qeprobe: red ANSI color
```

Damit sind xcc, OS-9-Modulstart, C-Library, `getenv`, `termlib.l`, der lokale
`q9`-Termcap-Eintrag (80x24) und ANSI-Farbausgabe im echten Gast nachgewiesen.

## OS-9-Terminalschicht und Port-2000-Test

`src/platform/qe_platform_os9.c` verwendet die SDK-Aufrufe `_gs_opt`,
`_ss_opt` und `_gs_rdy`. Der Raw-Modus folgt dem Microware-SCF-Muster und
stellt die zuvor gesicherten Optionen nach dem Programmende wieder her.
Fenstergroesse kommt aus termcap `co`/`li`, mit 80x24 als Rueckfallwert.

`qetermprobe` wurde auf der emulierten Konsole erfolgreich getestet: Raw-Modus,
ANSI-Ausgabe und Wiederherstellung der Shell-Eingabe funktionieren. Ein zweiter
End-to-End-Test meldet sich ueber den echten TCP-Terminalpfad auf `/x1` an und
sendet Pfeiltasten. Das Ergebnis im Gast lautet:

```text
qetermprobe: keys 1002 1003 1000 1001 113
qetermprobe x1 keys: PASS
```

Wichtig fuer Fortsetzungen: `/x1` kann die Bytes einer einzelnen ANSI-
Escape-Sequenz ueber mehrere Scheduler-Durchlaeufe bereitstellen. Drei Ticks
Wartezeit waren zu kurz und zerlegten `ESC [ A`; 20 Ticks funktionieren auf
dem realen Port-2000-Pfad und lassen eine einzelne Escape-Taste weiterhin mit
begrenzter Wartezeit zu. Der Testclient spricht Telnet-IAC selbst; das
macOS-`telnet`-Programm puffert bzw. verhandelt fuer diesen Rohdaten-Test
ungeeignet.
