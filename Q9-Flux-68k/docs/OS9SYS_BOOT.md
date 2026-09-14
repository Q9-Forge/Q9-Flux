# OS9SYS-CF-Boot Runbook

Stand: 2026-07-07

Dieses Runbook beschreibt den lokalen, erfolgreichen OS-9-Bootpfad mit
`local_images/OS9SYS.hda`. ROMs, `.hda`-Images und extrahierte Microware-Module
sind proprietaer bzw. gross und bleiben lokal; sie duerfen nicht ins Repository.

## Start unter Windows

```powershell
cd D:\projekts\Q9
.\build\native\q9.exe --rom .\local_images\roms\romimage.dev.running.BIN --cf .\local_images\OS9SYS.hda
```

Vor Neubuilds oder Image-Arbeiten alte Emulatorprozesse suchen:

```powershell
Get-CimInstance Win32_Process |
  Where-Object { $_.ExecutablePath -like 'D:\projekts\Q9\build\native\q9*.exe' } |
  Select-Object ProcessId,Name,ExecutablePath,CommandLine
```

Gezielt beenden:

```powershell
Get-CimInstance Win32_Process |
  Where-Object { $_.ExecutablePath -like 'D:\projekts\Q9\build\native\q9*.exe' } |
  ForEach-Object { Stop-Process -Id $_.ProcessId -Force }
```

## Erfolgreicher Boot

Ein gesunder Boot sieht so aus:

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

## Bootfile mit `os9gen`

Fuer Bootfile-Experimente den formatierbaren Descriptor verwenden:

```sh
os9gen /c0_fmt -z=<bootlist> -eb=300
```

Der normale Descriptor `/c0` ist formatgeschuetzt. Bei `/c0` kann `os9gen` beim
finalen Rename scheitern:

```text
os9gen: can't rename "/c0/OS9Boot"
Error #000:255
```

`000:255` ist `E$Format` = "Device is format protected".

Ein zu kleines oder falsch zusammengesetztes Bootfile kann trotzdem geschrieben
werden. Symptom beim naechsten Boot:

```text
Sysgo can't chx to 'CMDS'
Sysgo can't open 'startup' file
Error #000:215
```

`000:215` ist `E$BPNam` = "Bad pathlist specified". Dann ist der Emulatorstart
nicht kaputt; das Bootfile enthaelt nicht den erwarteten Boot-/Sysgo-Pfad.

Arbeitsregel:

1. Vor jedem `os9gen`-Experiment `OS9Boot` sichern.
2. Nach einem Fehlschlag nur `OS9Boot` aus einer Sicherung wiederherstellen.
3. Nicht gleichzeitig mit Toolshed und im laufenden Emulator auf dasselbe Image
   schreiben.

## Toolshed / WSL

Toolshed ist in Debian WSL installiert:

```bash
~/.local/bin/os9
```

Beispiele:

```bash
wsl -d Debian -- bash -lc 'cd /mnt/d/projekts/Q9 && os9 dir local_images/OS9SYS.hda,'
wsl -d Debian -- bash -lc 'cd /mnt/d/projekts/Q9 && os9 list local_images/OS9SYS.hda,SYS/termcap'
```

## Terminal und Editor

`SYS/termcap` enthaelt `q9|q9term`. Die Datei muss im OS-9-Textformat mit CR-
Zeilenenden geschrieben werden.

Unter Windows liefert `_getch()` fuer Cursor/Sondertasten erweiterte Codes wie
`0xE0 0x48`. Der Windows-HAL uebersetzt diese Codes auf ANSI-Sequenzen, damit
OS-9-Programme normale `termcap`-Eintraege benutzen koennen:

```text
ku=\E[A:kd=\E[B:kl=\E[D:kr=\E[C
```

`TERM=q9` und `TERM=q9term` sind Aliasnamen desselben Eintrags.

## C-Compiler auf OS9SYS.hda (verifiziert 2026-07-09)

Live-Test auf einem APFS-Clone (`local_images/OS9SYS.claudia-test.hda`),
Emulator per expect gesteuert:

- Compiler ist der klassische K&R-cc (`cc`, `cpp`, `c68`, `o68`, `r68`,
  `l68`), liegt direkt in `/dd/CMDS`; Header in `/dd/DEFS`, Libs (`clib.l`,
  `cstart.r`) in `/dd/LIB`.
- Benoetigte Umgebungsvariablen (klassischer cc, NICHT Ultra C):

  ```text
  setenv DEFS /dd/DEFS
  setenv LIB /dd/LIB
  ```

  (Ultra C wuerde stattdessen `CDEF`/`CLIB`/`REF`/`TMPDIR`/`CC` nutzen —
  s. `ultrac_use.pdf` Kap. 3 im Referenz-Toolchain.)
- Kompletter Durchlauf getestet: `build hello.c` → `cc hello.c` (alle fuenf
  Phasen) → `hello` laeuft und druckt. Das Ausgabemodul landet im
  **Execution Directory** (`chx`, hier `/dd/CMDS`), nicht im
  Datenverzeichnis.
- Login des Images: User `super`, Passwort `Al35uUbC` (weitere User: `afoe`,
  `q9`, `tools`, s. `/dd/SYS/password`).
- Automatisierungs-Hinweise: nach "1 devices online" erst ein CR senden,
  damit tsmon den Login-Prompt zeigt; Eingaben gebremst senden (expect
  `send -s`, ~50 ms/Zeichen), sonst gehen Zeichen verloren. Muster:
  `test/expect/test_paste_burst.exp`/`test/expect/test_paste_paced.exp` im Testverzeichnis.
