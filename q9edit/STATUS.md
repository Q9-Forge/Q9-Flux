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
- vorgesehener Editor-Kern: antirez/kilo, BSD-2-Clause
- noch kein Upstream-Code importiert
- erstes OS-9-Probeprogramm `qeprobe` mit xcc erfolgreich gebaut

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

1. OS-9-C-Aufrufe fuer `SS_Opt`, Einzelzeicheneingabe und Wiederherstellung
   der SCF-Optionen.
2. Terminalgroessenabfrage; termcap `co`/`li` ist der erste Weg, 80x24 der
   erlaubte Rueckfallwert.
3. Verfuegbarkeit von `snprintf`, `ftruncate` und den benoetigten
   Speicherfunktionen in der verwendeten C-Library.

## Naechste Schritte

1. `qeprobe` ins Testimage deployen und im Emulator ausfuehren.
2. SCF-Optionsstruktur und C-Library-Wrapper im SDK identifizieren.
3. Terminalprobe mit Farbe und Pfeiltasten im Emulator testen.
4. Erst danach Kilo mit fester Commit-ID und BSD-Lizenz importieren.

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
