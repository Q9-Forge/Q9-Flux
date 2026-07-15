# xcc- und OS-9-SDK-Buildnotizen

Dieses Dokument beschreibt nur nachgepruefte Fakten. Vermutungen werden als
offen markiert, damit eine Fortsetzung keine scheinbaren Gewissheiten erbt.

## Umgebung laden

```sh
source /Volumes/SSD1TB/projects/MWOS/tools/macos/env/os9-toolchain.sh
```

Danach liegt der Wrapper `os9make` im `PATH`. Er startet
`DOS/BIN/os9make.exe` ueber Wine; dieses findet `xcc.exe` im selben DOS-BIN-
Verzeichnis.

Alternativ kapselt folgender Helfer Umgebung, Arbeitsverzeichnis und Logdatei:

```sh
/Volumes/SSD1TB/projects/MWOS/tools/macos/bin/mwos-build PROJEKTVERZEICHNIS
```

## Relevante Make-Variablen aus dem Q9-Port

Der Q9-Port definiert unter anderem:

```text
CC       = xcc
RC       = r68
LC/LD    = l68
ARCHROOT = $(MWOS)/OS9/68000
SYSRELS  = $(MWOS)/OS9/68000/LIB
OSDEFS   = $(MWOS)/OS9/SRC/DEFS
CDEFS    = $(MWOS)/SRC/DEFS
```

Quelle: `MWOS/OS9/68030/PORTS/Q9/makefile`.

## Termcap

Der Header `MWOS/OS9/SRC/DEFS/termcap.h` ist ANSI-prototypisiert und stellt
die klassische Termcap-API bereit. Die MWOS-Zieltemplates definieren:

```text
TERMLIB  = -l=$(MWOS_DFTCLIB)/termlib.l
TERMLIBS = $(TERMLIB)
```

q9edit setzt deshalb `LIBS = $(TERMLIBS) $(MWOS_CSLLIBS)`.

q9edit benutzt keine Funktionen aus `curses.h`. Das Vorhandensein dieses
Headers im SDK bedeutet nicht, dass curses fuer q9edit gelinkt werden muss.

## Minimal-Build

Der erste Cross-Build enthaelt bewusst noch nicht den Editor. Das Modul
`qeprobe` wird mit folgendem Befehl gebaut:

```sh
make -C q9edit os9
```

Der reine xcc-/Linker-Test war am 2026-07-15 erfolgreich. Die folgende
Probeversion soll:

1. einen kurzen Text ausgeben,
2. mit Erfolg enden,
3. danach termcap linken und `TERM=q9` laden,
4. erst im dritten Schritt SCF-Optionen veraendern.

Diese Trennung lokalisiert Compiler-, Linker-, termcap- und Terminalprobleme,
statt sie gleichzeitig im Editor zu vermischen.

Der komplette Gasttest war am 2026-07-15 ebenfalls erfolgreich. Wichtigster
Portierungsbefund: Der 2048-Byte-Termcap-Puffer darf wegen des kleinen
OS-9-Programmstack nicht lokal angelegt werden. Als statischer Puffer liefen
`tgetent("q9")`, `tgetnum("co") == 80`, `tgetnum("li") == 24` und ANSI-Farbe
fehlerfrei.

## Sicheres Deployment

Nach erfolgreichem Build wird ein Modul ausschliesslich in ein explizites
Entwicklungsimage kopiert:

```sh
q9edit/tools/deploy-os9.sh /absoluter/pfad/zu/einem-klon.hda
```

Das Skript verweigert `local_images/OS9SYS.hda` und Images, die ein laufender
Q9-Prozess in seiner Kommandozeile verwendet. Es nutzt ToolShed `os9 copy`
und setzt danach die OS-9-Ausfuehrungsattribute. Den Emulator vor jedem
Schreibzugriff trotzdem bewusst beenden; die Pruefung ist eine zweite
Sicherheitsbarriere, kein Ersatz fuer diesen Arbeitsablauf.
