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
die klassische Termcap-API bereit. Der Header-Kommentar nennt `termlib.l` als
Implementierungsbibliothek. Der genaue Linkparameter muss noch anhand der
Make-Templates oder eines bestehenden Programms verifiziert werden.

q9edit benutzt keine Funktionen aus `curses.h`. Das Vorhandensein dieses
Headers im SDK bedeutet nicht, dass curses fuer q9edit gelinkt werden muss.

## Noch zu erzeugender Minimal-Build

Der erste Cross-Build soll bewusst noch nicht den Editor enthalten. Das Modul
`qeprobe` soll:

1. einen kurzen Text ausgeben,
2. mit Erfolg enden,
3. danach termcap linken und `TERM=q9` laden,
4. erst im dritten Schritt SCF-Optionen veraendern.

Diese Trennung lokalisiert Compiler-, Linker-, termcap- und Terminalprobleme,
statt sie gleichzeitig im Editor zu vermischen.

