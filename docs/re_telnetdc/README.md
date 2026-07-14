# Reverse-Engineering: telnetd/telnetdc Hänger (ARBEITSPLAN 5.15)

Analyse-Artefakte zum Telnet-Hänger bei langer Ausgabe über eth0.

## Root-Cause (Kurzfassung)

`telnetd` forkt pro Session `telnetdc`. `telnetdc` relayt tty↔net in einer
ereignisgesteuerten Schleife und blockiert in **`F$Event`/Ev$Wait (timeoutlos)**.
Es registriert zwei Aufweck-Events asymmetrisch:
- **Netz:** `I$SetStt` Code `0x48 = SS_SPF` → vom SPF-Stack bedient (QUICC-IRQ) → **funktioniert**.
- **tty:** `I$SetStt` Code `0x3e = SS_SEvent` ("set event on data ready") → **der pty-SCF-Treiber
  implementiert SS_SEvent nicht** → tty-Event feuert nie → telnetdc parkt für immer.

Beweis: Hänger auslösen, ein Byte an den Client-Socket senden → gestaute Ausgabe fließt sofort
byte-genau weiter (das Netz-Event weckt telnetdc, es findet die tty-Daten). Kein Emulator-Bug,
sondern OS-9-Treiber-Fähigkeitslücke.

## Werkzeuge / Reproduktion

Ghidra 12.1.2 + OpenJDK 21 (`brew install ghidra`). Setup:
```bash
export JAVA_HOME=/opt/homebrew/opt/openjdk@21
GHIDRA=/opt/homebrew/Cellar/ghidra/12.1.2/libexec/support/analyzeHeadless

# Modul aus dem Image ziehen:
os9 copy "local_images/OS9SYS.hda,CMDS/telnetdc" telnetdc.bin

# Import (Raw-Loader, echtes CPU-Ziel):
"$GHIDRA" <proj> q9 -import telnetdc.bin -loader BinaryLoader -processor 68000:BE:32:MC68030

# Echten Einsprungpunkt setzen (Modul-Header exec-Offset, telnetdc = 0x52) + disassemblieren:
"$GHIDRA" <proj> q9 -process telnetdc.bin -scriptPath . -postScript FixEntry.java 0x52 out.txt -noanalysis
```

`os9module.h` (ToolShed): OSK-Modul-Header ist 48 Byte, `size`@4 (4B), Namensoffset@12 (4B),
Name null-terminiert. `modbust` stürzt an `netmods` mit SIGBUS ab → eigener Python-Parser nötig.

## Dateien
- `FixEntry.java` — disassembliert ab echtem Modul-Einsprungpunkt (Ghidra fängt sonst falsch an)
- `ExportListing.java` / `ExportStrings.java` / `FindRefs.java` — Listing/Strings/Xref-Export
- `telnetdc_strings.txt` / `telnetd_strings.txt` — extrahierte String-Literale
