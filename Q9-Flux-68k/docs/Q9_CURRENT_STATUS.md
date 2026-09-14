# Q9 Current Status

Stand: 2026-07-07, nach dem erfolgreichen OS9SYS-CF-Boot und dem Windows-Keyfix.

## Aktueller Windows-Arbeitsstand

Projektpfad:

`D:\projekts\Q9`

Lokale Testartefakte, bewusst nicht im Git:

- `local_images/OS9SYS.hda`
- `local_images/Q9-fresh-cf.hda`
- `local_images/roms/romimage.dev.running.BIN`

Startzeile unter Windows PowerShell:

```powershell
cd D:\projekts\Q9
.\build\native\q9.exe --cb030 .\local_images\roms\romimage.dev.running.BIN --cf .\local_images\OS9SYS.hda
```

Build unter Windows:

```powershell
cd D:\projekts\Q9
$env:PATH = "C:\Users\AF\w64devkit\bin;$env:PATH"
$env:OS = "Windows_NT"
make native
```

Wichtig: Vor einem Neubuild pruefen, ob noch alte Emulatorprozesse laufen, sonst
bleibt `build\native\q9.exe` gesperrt oder man testet versehentlich einen alten
Prozess weiter:

```powershell
Get-CimInstance Win32_Process |
  Where-Object { $_.ExecutablePath -like 'D:\projekts\Q9\build\native\q9*.exe' } |
  Select-Object ProcessId,Name,ExecutablePath,CommandLine
```

## Gesichert seit 2026-07-06/07

- `OS9SYS.hda` bootet direkt von CompactFlash.
- Das Bootfile wird vom ROM erkannt; danach laeuft `startup` vom CF-Dateisystem.
- Bootauszug nach erfolgreicher Wiederherstellung:

```text
OS-9/68K System Bootstrap
Now trying to boot from CompactFlash.
A valid OS-9 bootfile was found.
CompactFlash driver build 42
iniz /dd
iniz /c0
chx /c0/CMDS
chd /c0
setenv TERM q9
list /c0/SYS/motd
$
```

- Der erfolgreiche Bootfile-Aufbau benoetigt den formatierbaren Descriptor
  `/c0_fmt`; der normale `/c0` ist formatgeschuetzt.
- Der konkrete Fehler beim Versuch mit `/c0` war:

```text
os9gen: can't rename "/c0/OS9Boot"
Error #000:255
```

`000:255` ist `E$Format` = "Device is format protected".

- Ein spaeterer Bootfehler

```text
Sysgo can't chx to 'CMDS'
Sysgo can't open 'startup' file
Error #000:215
```

war kein Emulatorstartproblem, sondern ein unbrauchbar erzeugtes/zu kleines
`OS9Boot`. `000:215` ist `E$BPNam` = "Bad pathlist specified".

- Ursache des letztlich geloesten Bootfile-Problems: Schreibfehler in der
  Bootlist plus der falsche Descriptor. Mit korrigierter Bootlist und
  `/c0_fmt` bootet das Image.

## Terminal/Editor-Stand

- `SYS/termcap` enthaelt `q9|q9term` und wird in den Images mit OS-9-CR-
  Zeilenenden geschrieben. LF-Zeilenenden fuehrten dazu, dass `umacs` den
  Terminaltyp nicht fand.
- `TERM=q9` bzw. `TERM=q9term` funktioniert.
- `umacs` nutzt die Cursor-Tasten nach dem Windows-HAL-Keyfix.
- Windows `_getch()` liefert fuer Cursor/Sondertasten erweiterte Sequenzen
  wie `0xE0 H`. Der native Windows-HAL normalisiert diese auf ANSI-Sequenzen
  (`ESC [ A` usw.); `termcap` beschreibt entsprechend ANSI:

```text
ku=\E[A:kd=\E[B:kl=\E[D:kr=\E[C
```

- Der reine Versuch, `0xE0` als echtes Byte oder als `\340` direkt in `termcap`
  zu beschreiben, war fuer `umacs` nicht stabil. Sichtbares Symptom: auf dem
  Bildschirm erschienen nur noch `H`, `M`, `P`, `K`.
- `Ctrl-C` wird im Windows-HAL abgefangen und als Gast-Eingabe behandelt, damit
  der Emulator nicht durch den Host beendet wird.

## Toolshed / WSL

Die OS-9-Images werden nicht direkt mit Windows-Tools bearbeitet. Unter Debian
WSL ist Toolshed installiert:

```bash
~/.local/bin/os9
```

Beispiele:

```bash
wsl -d Debian -- bash -lc 'cd /mnt/d/projekts/Q9 && os9 dir local_images/OS9SYS.hda,'
wsl -d Debian -- bash -lc 'cd /mnt/d/projekts/Q9 && os9 list local_images/OS9SYS.hda,SYS/termcap'
```

Wichtig: Nicht gleichzeitig hostseitig mit Toolshed und im laufenden Emulator auf
dasselbe Image schreiben.

## Gesichert

- CF-Emulation fuer 256-Byte-RBF-Images wurde untersucht und gilt fuer Lesen/Schreiben als grundsaetzlich funktionsfaehig.
- Erfolgreich getestet:
  - `iniz /dd` mit dem CB030-ROM.
  - `save` in ein kleines Verzeichnis.
  - Hostseitiges `dcheck` nach kleinen Emulator-Schreibtests blieb sauber.
  - `make test` war nach den CF-Aenderungen gruen.
- Wichtiges Bedienungsdetail fuer automatisierte Emulator-Kommandos:
  - OS-9 erwartet CR (`\r`), nicht nur LF (`\n`).
  - Mehrere Kommandos zu schnell hintereinander koennen die serielle Konsole vermischen.

## Aktueller Image-Build

Arbeitsimage-Pfad:

`/Volumes/SSD1TB/projects/Q9/local_images/Q9-cb030-work-max.hda`

Build-Log:

`/Volumes/SSD1TB/projects/Q9/local_images/Q9-cb030-work-max-build.log`

Build-Skript:

`/Volumes/SSD1TB/projects/Q9/tools/build_q9_work_image.py`

Ziel:

- maximales RBF-Image mit `DD.TOT = 16777215` logischen 256-Byte-Sektoren.
- Inhalt aus Referenz-Toolchain, OS-9 Professional 3.0 Disketten und CB030-Modulen.
- Edition-Check: pro Modul/Tool gewinnt die hoechste Edition.
- Verzeichnisse bewusst klein halten, weil der ROM/RBF-Pfad grosse Directories nur teilweise sieht.

## Wichtige Inhalte

- Root: `CMDS`, `C`, `DEFS`, `LIB`, `IO`, `SYS`, `DIST`.
- `CMDS` ist bewusst klein gehalten.
- Compiler liegt unter `CMDS/C`:
  - `cc` ed 44
  - `c68` ed 357
  - `cpp` ed 41
  - `o68` ed 19
  - `r68` ed 175
  - `l68` ed 104
  - `make`
- CB030-spezifische Module liegen unter `CMDS/BOOTOBJS`.
- ROMRAM-Module wurden in kleine Unterverzeichnisse unter `CMDS/BOOTOBJS/ROMRAM/Rxx` verteilt.

## Offene Probleme

- Das Image-Building ist noch nicht endgueltig stabil.
- Beobachtungen:
  - 256-Sektor-Cluster: `dcheck` sauber, aber der Emulator/ROM-RBF sieht in groesseren Directories nur den ersten Teil.
  - 64/128-Sektor-Cluster: Emulator-Directory-Verhalten besser, aber `dcheck` meldete bei grossen Dateien verlorene Cluster.
- Der naechste sinnvolle Schritt ist, ein kleineres, konservativeres Arbeitsimage zu erzeugen:
  - 64-Sektor-Cluster.
  - Nur Kern-Tools, Compiler, Standard-Header und Standard-Libs.
  - Grosse optionale Libraries/Quellen erstmal weglassen.
  - Danach `dcheck` muss null verlorene Cluster melden.
  - Danach im Emulator `iniz /dd` und einzelne `dir`-Kommandos mit CR testen.

## Startzeile

```sh
./build/native/q9.exe --cb030 /Volumes/SSD1TB/projects/REF/OS9/68030/PORTS/CB030/CMDS/BOOTOBJS/ROMBUG/romimage.dev.running.BIN --cf /Volumes/SSD1TB/projects/Q9/local_images/Q9-cb030-work-max.hda
```

## Merksatz

Nicht gleichzeitig hostseitig mit Toolshed und im Emulator auf dasselbe Image schreiben.

---

## Nachtrag 2026-07-09: Referenz-Toolchain auf OS9SYS.hda + "Directory-Grenze" aufgeklaert (Claudia)

### Referenz-Toolchain ist jetzt im Image

Komplette Referenz-Toolchain (ohne DOC, 710 MB PDFs) hostseitig per Toolshed nach
`OS9SYS.hda,REF` kopiert: Top-Level-Dateien, DIST, DOS, MAKETMPL, APPS, SRC
und OS9 mit ALLEN CPU-Verzeichnissen (68000/68020/68030/68040/68060/CPU32/SRC —
68030 fehlte vorher komplett). Ausgelassen: 2956 "(2)"-Duplikate (2913 exakt
identisch, 43 nur CRLF/LF-Unterschied — bleiben nur auf dem Mac), Windows-Junk
(Zone.Identifier, .DS_Store). Drei Dateien mit ECHTEN Unterschieden zwischen
Original und "(2)"-Version wurden in BEIDEN Versionen kopiert (Andreas'
Entscheidung; Herkunft der (2)-Variante ungeklaert, sysgo_nodisk (2).a enthaelt
u.a. einen undokumentierten Patch mit Hardware-Zugriff auf $a00040):
`OS9/SRC/SYSMODS/GCLOCK/tickgeneric.a`, `OS9/SRC/SYSMODS/SYSGO/sysgo_nodisk.a`,
`OS9/SRC/SYSMODS/SYSGO/makefile`.

### "Directory-Grows-Grenze" existiert NICHT (auf diesem Image)

Die alte Beobachtung "der Emulator/ROM-RBF sieht in groesseren Directories nur
den ersten Teil" wurde geprueft und ist fuer OS9SYS.hda (32-Sektor-Cluster)
WIDERLEGT — Gast und Host byte-genau verglichen:

- `/dd/REF/OS9/68000/CMDS`: 179 Eintraege im Gast = 179 im Host, identisch.
- `/dd/REF/OS9/SRC/DEFS`: 81 = 81, identisch.

Was tatsaechlich kaputt ist: **Toolshed `dsave` hat einen Bug** — es bricht
beim Fuellen groesserer Verzeichnisse nach ~72 Eintraegen mit "pathname not
found" ab (reproduzierbar, auch in ein frisches leeres Zielverzeichnis).
Einzelne `os9 copy`-Aufrufe fuer DIESELBEN Dateien in DASSELBE Verzeichnis
funktionieren dagegen fehlerfrei bis 180+ Eintraege. Workaround-Skript
(rekursiver Merge per Einzel-copy, ueberspringt Vorhandenes und nicht
kopierbare Dateien): Muster in der Session vom 2026-07-09, bei Bedarf neu
erzeugbar (~60 Zeilen Python).

Weitere Toolshed-Stolperfallen (gleiche Sitzung):
- Dateinamen mit `~` (Editor-Backups wie `send.c~`) → "badly formed pathname".
- `os9 copy` stuerzt bei mind. einer grossen Binaerdatei ab
  (`free-virtual-serial-ports.exe`, 5,4 MB, Exit-Code 133, keine Meldung).
- `dcheck` meldet nach der Aktion ~4576 verwaiste Cluster (~73 MB, durch
  zwischenzeitlich angelegte/gelöschte Verzeichnisse beim Debugging) —
  "file structure is intact", nur verlorener Platz, kein Defekt.

Die aeltere 256-Sektor-Cluster-Beobachtung (Q9-cb030-work-max.hda) ist damit
nicht automatisch erklaert — moeglicherweise war auch dort dsave der
eigentliche Schuldige. Falls das Thema wieder aufkommt: zuerst mit Einzel-copy
gegentesten, bevor RBF/Emulator verdaechtigt werden.
