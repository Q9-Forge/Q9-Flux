#═════════════════════════════════════════════════════════════════════════════════════════════════
# File:   BOARD.md                                                                        Ver. 1.00
# Owner:  AF
# Desc.:  Hardware-Referenz fuer das CB030-Board (68030-SBC) — Speicherkarte + Peripherie-Register.
#         Grundlage fuer Schritt 5.2 (ARBEITSPLAN.md): Musashi-Board-Emulation zur Bootstrap-
#         Validierung mit dem originalen, proprietaeren OS-9-Boot-ROM, bevor eigene Q9-Module treten.
#
# Edition History
#─────────┬──────┬─────────────────────────────────────────────────────────────────────────┬──────
# Date    │ Ver. │ Description                                                             │ By
#─────────┼──────┼─────────────────────────────────────────────────────────────────────────┼──────
# 26-07-04│ 1.00 │ Initiale Version: Speicherkarte, 68681-DUART, CF-Interface (Andreas)     │ AF
#═════════╧══════╧═════════════════════════════════════════════════════════════════════════╧══════

# CB030 — Hardware-Referenz

Bezug: [ARBEITSPLAN.md](../ARBEITSPLAN.md) Schritt 5.2. Das CB030-Board dient als
**Bootstrap/Validierungs-Zwischenschritt** für die Musashi-Integration (Schritt 5.1) —
mit einem echten, produktiven OS-9-Boot-ROM testen, statt nur mit
handassemblierten Testprogrammen. Ändert nichts an der eigentlichen Q9-Zielhardware
(MC68EN360/QUICC bleibt Ziel für Phase 7, siehe PROJECT.md O4).

**Lizenzhinweis**: Das Microware-Boot-ROM/die Microware-Module selbst sind proprietär —
dieselbe Regel wie bei der privaten MWOS-SDK-Kopie (siehe HANDBUCH.md Abschnitt 1/7):
NICHT ins Repository, nur lokal für Andreas' eigene Validierung. Diese Speicherkarten-
und Register-Beschreibung selbst ist reine Hardware-Dokumentation (keine Microware-
Quelle) und unproblematisch.

---

## Speicherkarte (Memory Map)

Zwei Zustände: **Reset** (unmittelbar nach Reset, Flash-ROM liegt bei Adresse 0) und
**Remap** (nach Umschalten — Flash-ROM wandert nach `0xFE00_0000`, RAM liegt bei 0).

| Gerät | Reset-Zustand | Remap-Zustand | Größe |
|-------|---------------|---------------|-------|
| EDO-RAM (SIM-Modul, 72-pin) | kein RAM im Reset-Zustand | `0x0000_0000`–`0x00FF_FFFF` (bei 16 MB-Bestückung) | 16 MByte |
| — | | `0x0000_0000`–`0x01FF_FFFF` | 32 MByte |
| — | | `0x0000_0000`–`0x03FF_FFFF` | 64 MByte |
| — | | `0x0000_0000`–`0x07FF_FFFF` | 128 MByte |
| Flash-ROM (29F040, 32-pin) | `0x0000_0000`–`0x0007_FFFF`, **gespiegelt bis zum oberen Byte des Adressraums** (`0xFEFF_FFFF`) | **liegt EINMAL bei `0xFE00_0000`–`0xFE07_FFFF`, KEINE Spiegelung mehr** | 512 KByte |
| I/O (gesamt) | `0xFFFF_0000`–`0xFFFF_FFFF` | `0xFFFF_0000`–`0xFFFF_FFFF` | 64 KByte |
| — REMAP-Register | `0xFFFF_8000`–`0xFFFF_8FFF` | gleich | 4 KByte |
| — TI_IRQ_OFF | `0xFFFF_9000`–`0xFFFF_97FF` | gleich | 2 KByte |
| — TI_IRQ_ON | `0xFFFF_9800`–`0xFFFF_9FFF` | gleich | 2 KByte |
| — CF-Card | `0xFFFF_E000`–`0xFFFF_EFFF` | gleich | 4 KByte |
| — UART (68681 DUART) | `0xFFFF_F000`–`0xFFFF_FFFF` | gleich | 4 KByte |

Die RAM-Größe hängt von der bestückten SIM-Speicherkarte ab (16/32/64/128 MB) —
kein fester Wert, muss beim Board-Setup konfigurierbar sein (passt zur Idee einer
Q9-Systemkonfiguration, `src/kernel/config.h`, Schritt 4.9).

### Reset → Remap-Übergang (Andreas, 2026-07-04)

- **Sofort nach Reset**: Flash-ROM liegt bei Adresse 0, gespiegelt über den
  gesamten Adressraum bis zum oberen Byte (`0xFEFF_FFFF`) — noch kein RAM
  sichtbar. Der 68030 holt seinen Reset-Vektor (SSP + PC) also aus dem ROM.
- **Auslöser für den Umschlag**: Ein **einziger** Buszugriff (lesend oder
  schreibend, kein bestimmter Datenwert nötig) auf die REMAP-Adresse
  (`0xFFFF_8000`-Bereich) schaltet **dauerhaft** um — kein Bit-Layout, kein
  Register-Wert, reiner Adress-Trigger.
- **Danach (Remap-Zustand)**: Ab Adresse 0 liegen 16 MByte RAM (bzw. mehr, je
  SIM-Bestückung); das Flash-ROM liegt nur noch **einmal**, unmittelbar bei
  `0xFE00_0000`–`0xFE07_FFFF`, keine Spiegelung mehr.

### Timer/Interrupt IRQ3 (Andreas, 2026-07-04)

`TI_IRQ_ON`/`TI_IRQ_OFF` sind **keine Datenregister**, sondern reine
Adress-Trigger (wie REMAP): Ein einfacher Buszugriff auf die `TI_IRQ_ON`-Adresse
schaltet einen Timer ein, der einen Interrupt auf **IRQ3** auslöst; ein
Buszugriff auf `TI_IRQ_OFF` schaltet ihn wieder aus. Kein Wert wird gelesen
oder geschrieben — der Adresszugriff selbst ist die Aktion. 100 Hz (10 ms
Periode).

**Umsetzung — kooperativ, kein echter Host-Interrupt (Andreas + Claudia,
2026-07-04):** Musashi bringt die eigentliche Interrupt-Mechanik bereits mit
— `m68k_set_irq(3)` genügt, Musashi legt PC+Statusregister intern selbst auf
den Stack und springt zum Vektor (Autovektor, da `M68K_EMULATE_INT_ACK` in
unserer Konfiguration aus ist — Annahme, bei Bedarf beim ersten echten
Boot-Test mit vektorisiertem Acknowledge nachzujustieren). Wir müssen also
nur wissen, WANN 10 ms vergangen sind — dafür bewusst **kein echter
Host-Timerinterrupt** (Signal/Thread), weil das echte Nebenläufigkeit in
Musashis Zustand einführen würde (nicht thread-sicher) und Q9 überall sonst
bewusst einfädig/kooperativ ist (Entscheidung E8). Stattdessen: Host-Uhrzeit
zählen, an einer sicheren Stelle im Kontrollfluss geprüft (analog zu einem
echten Interrupt, der nur ein Flag setzt, das zur richtigen Zeit im Zyklus
abgefragt wird — nur ohne die echte Asynchronität):

```
q9_board_t bekommt: timer_active (an/aus, durch TI_IRQ_ON/OFF umgeschaltet)
                    last_tick_ms (Host-Zeit beim letzten Auslösen)

q9_board_poll_timer(board, jetzt_ms):
    wenn timer_active UND (jetzt_ms - last_tick_ms) >= 10:
        m68k_set_irq(3)
        last_tick_ms = jetzt_ms
```

`q9_board_poll_timer` wird von der Stelle aufgerufen, die auch `m68k_execute()`
antreibt — Häufigkeit hängt am noch offenen Zyklenbudget pro Aufruf, ist aber
für Software-Timer-Zwecke unkritisch (keine harte Echtzeitanforderung).

---

## 68681 DUART (serielle Schnittstelle)

Basisadresse `0xFFFF_F000`. 2 Ports, Oszillator 3,842 MHz.

| Register | Adresse | Bedeutung |
|----------|---------|-----------|
| MRA | `0xFFFF_F000` | Mode Register A |
| SRA (Lesen) / CSRA (Schreiben) | `0xFFFF_F002` | Status A / Clock-Select-Register |
| CRA | `0xFFFF_F004` | Command Register (nur Schreiben) |
| RHRA (Lesen) / THRA (Schreiben) | `0xFFFF_F006` | Rx-Hold-Register A / Tx-Hold-Register |
| IPCR (Lesen) / ACR (Schreiben) | `0xFFFF_F008` | Input-Port-Change-Register / Aux-Control-Register |
| ISRD (Lesen) / IMRD (Schreiben) | `0xFFFF_F00A` | Interrupt-Status-Register / Interrupt-Mask-Register |
| CTU / CTUR | `0xFFFF_F00C` | Counter/Timer oberes Byte (Lesen) / Preload oberes Byte (Schreiben) |
| CTL / CTLR | `0xFFFF_F00E` | Counter/Timer unteres Byte (Lesen) / Preload unteres Byte (Schreiben) |
| MR1B | `0xFFFF_F010` | Mode Register B |
| SRB | `0xFFFF_F012` | Status Register B |
| RHRB | `0xFFFF_F016` | Rx-Holding-Register B |
| IVRD | `0xFFFF_F018` | Interrupt-Vector-Register |
| STARTCTR (Lesen) / SETOPR (Schreiben) | `0xFFFF_F01C` | Start-Counter-Kommando / Output-Register-Bits setzen |
| STOPCTR (Lesen) / CLROPR (Schreiben) | `0xFFFF_F01E` | Stop-Counter-Kommando / Output-Register-Bits löschen |
| OPCR (Schreiben) | `0xFFFF_F01A` | Output-Configuration-Register |

---

## Compact-Flash-Interface

Basisadresse `0xFFFF_E000`.

| Register | Adresse | Bedeutung |
|----------|---------|-----------|
| CFdata | `0xFFFF_E000` | CF-Datenregister |
| CFerr | `0xFFFF_E001` | CF-Fehlerregister |
| CFsectcnt | `0xFFFF_E002` | CF-Sektorzähler |
| CF07 | `0xFFFF_E003` | CF LBA-Adresse Bits 0–7 |
| CF815 | `0xFFFF_E004` | CF LBA-Adresse Bits 8–15 |
| CF1623 | `0xFFFF_E005` | CF LBA-Adresse Bits 16–23 |
| CF2427 | `0xFFFF_E006` | CF LBA-Adresse Bits 24–27 |
| CFstat | `0xFFFF_E007` | CF-Status-/Kommandoregister |

---

## Emulations-Architektur (Andreas + Claudia, 2026-07-04 abends)

Musashi ruft bei jedem Speicherzugriff eine Host-Funktion auf (`m68k_read_memory_8/16/32`,
`m68k_write_memory_8/16/32`, s. Schritt 5.1). Diese Funktionen müssen anhand der Adresse
entscheiden, welches Gerät gemeint ist — als einfache if/else-Kette. **I/O wird zuerst
geprüft** (der 0xFFFF_xxxx-Bereich ist in beiden REMAP-Zuständen erreichbar — das Boot-ROM
initialisiert die DUART vor dem Remap), danach die Zustandsweiche:

```
adr in [0xFFFF_8000, 0xFFFF_8FFF] → REMAP-Trigger (jeder Zugriff, Wert egal)
adr in [0xFFFF_9000, 0xFFFF_97FF] → TI_IRQ_OFF-Trigger
adr in [0xFFFF_9800, 0xFFFF_9FFF] → TI_IRQ_ON-Trigger
adr in [0xFFFF_E000, 0xFFFF_EFFF] → CF-Interface
adr in [0xFFFF_F000, 0xFFFF_FFFF] → UART (68681)
wenn NICHT remapped:
    adr <= 0xFEFF_FFFF          → ROM, gespiegelt (adr modulo ROM-Größe) — bis zum
                                  oberen Byte des Adressraums, s. Speicherkarte oben!
                                  (Korrektur 2026-07-05: der 5.2a-Erstwurf hatte hier
                                  fälschlich 0x0800_0000 — das echte Boot-ROM springt
                                  vor dem REMAP-Trigger hoch nach 0xFE00_xxxx, sonst
                                  würde ihm der Code unter dem PC wegremappt)
sonst:                          (nach dem einen REMAP-Zugriff)
    adr < RAM_GROESSE           → RAM (häufigster Fall)
    adr in [0xFE00_0000, 0xFE07_FFFF] → ROM, EINMAL (kein Spiegeln mehr)
```

Der REMAP-Zustand ("schon umgeschaltet: ja/nein") ist ein einzelner Merker in einer
neuen Board-Zustandsstruktur — unabhängig von Musashis eigenem CPU-Zustand.

**ROM-Inhalt**: Kommt aus einer Datei (das reale Boot-ROM-Image, proprietär —
bleibt lokal, NICHT ins Repo, siehe Lizenzhinweis oben), beim Board-Start einmal komplett
eingelesen und im Speicher gehalten (512 KByte, passt locker). Dateipfad wird konfigurierbar
sein, nicht fest einprogrammiert.

## Erkenntnisse aus dem ersten echten OS-9-Boot (2026-07-05, Schritt 5.4)

Das unveränderte `romimage.dev.running.BIN` bootet im Emulator bis zur interaktiven
mshell. Hardware-Details, die erst beim Debuggen sichtbar wurden (Quelle: Verhalten
des ROMs/der Treiber + sc68681-Quelltext in Andreas' MWOS-Kopie):

- **Reset-Vektor**: Initial-PC = `0xFE00_0494` — das ROM startet direkt im hohen
  Spiegelbereich und remappt erst danach. Die Spiegelung bis `0xFEFF_FFFF` ist also
  essenziell (nicht nur kosmetisch).
- **DUART IVR**: Reset-Wert ist `0x0F` ("uninitialisierter Vektor"). Der sc68681-
  Treiber prüft beim Init GENAU darauf (bzw. auf den Descriptor-Vektor) und bricht
  sonst mit E$BMode ab — deshalb muss die Emulation den Reset-Wert liefern.
- **DUART-Interrupt ist VEKTORISIERT**: Der Treiber schreibt den Descriptor-Vektor
  (hier `0x50`) ins IVR und registriert seinen Handler per F$IRQ auf diesem Vektor.
  Beim IACK-Zyklus muss die DUART ihren IVR-Inhalt auf den Bus legen (kein
  Autovector!). Der 100Hz-Timer dagegen läuft über Autovector 27 (= Level 3).
  Beide teilen sich IRQ-Level 3.
- **DUART MR1/MR2**: einziges Register, das der Treiber zurückliest (Zeiger-
  Semantik: jeder Zugriff schaltet auf MR2, CR-Kommando `0x1x` zurück auf MR1).
- **CF**: Der Boot-Treiber (cfide) setzt zuerst 8-Bit-Mode per SET FEATURES
  (`0xEF`) — muss angenommen werden, sonst "error setting 8-bit mode".
- **FPU/MMU**: Boot-ROM und Kernel nutzen FRESTORE/FSAVE-Adressierungsarten und
  MMU-Funktionen (Root-Direct-Mapping, vier Tabellenebenen), die in Musashi
  fehlten — nachgerüstet, s. `third_party/musashi/Q9_VENDOR.md`.
- **Offen**: PFLUSH/PTEST werden nur geloggt (unkritisch: die Emulation hat keinen
  TLB, jeder Zugriff übersetzt frisch); MMU-Schreibschutz/-Faults fehlen (keine
  wirksame Prozess-Isolation im Emulator); CF-Image ist leer (Boot fällt sauber
  auf den ROM-Bootpfad zurück) — ein bespielbares CF-Image wäre der nächste
  logische Schritt (dd/os9gen).

**UART — Minimalansatz**: Nicht jedes Register muss "echt" etwas tun, aber JEDER
Registerzugriff muss sauber angenommen werden (auch wenn der Wert nur verworfen wird) —
das Boot-ROM schreibt beim Start vermutlich mehrere Init-Register (Mode, Clock-Select,
Command), bevor es die UART benutzt; ein ignorierter/fehlerhafter Zugriff könnte das
Boot-ROM zum Hängen bringen. Wirklich funktionieren müssen nur: **SRA** (Status: Zeichen
da? Sender frei?) und **THRA/RHRA** (der eigentliche Zeichenpuffer, THRA-Schreibzugriff
→ Zeichen an die Q9-Konsole ausgeben).

**CF-Interface**: Backing Store = eine große Datei (Vorschlag: 4 GByte — unkritisch, die
LBA-Register CF07/815/1623/2427 decken 28 Bit ab, bis 128 GByte theoretisch), Zugriff über
normale Host-Datei-I/O. Genau das Muster von `q9disk.img` (Schritt 3.1) — kein neues Konzept.
Im Unterschied zur UART reicht rohes Registerdurchreichen aber NICHT: Das Boot-ROM schreibt
vermutlich ein echtes ATA-Kommando ins `CFstat`-Register (z.B. Sektor lesen/schreiben) und
pollt danach auf Busy-/Fertig-Bits — dieses Kommando-Protokoll muss mindestens für
Lesen+Schreiben eines Sektors nachgebildet werden, nicht nur die Datenregister selbst.

## Erkenntnisse aus dem produktiven CF-Boot (2026-07-07)

Das lokale Arbeitsimage `local_images/OS9SYS.hda` bootet inzwischen direkt vom
CompactFlash-Backing-Image. Das ROM findet ein gueltiges `OS9Boot`, der CF-Treiber
meldet sich, und danach laeuft das `startup` aus dem CF-Dateisystem:

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

### Bootfile-Erzeugung mit `os9gen`

Der Bootfile-Aufbau braucht den formatierbaren Descriptor `/c0_fmt`. Der normale
Descriptor `/c0` ist absichtlich formatgeschuetzt; ein Versuch, mit `/c0` zu
arbeiten, endet beim finalen Rename typischerweise so:

```text
os9gen: can't rename "/c0/OS9Boot"
Error #000:255
```

`000:255` ist `E$Format` ("Device is format protected"). Der Descriptor
`/c0_fmt` ist fuer `format`/`os9gen` vorgesehen.

Ein zweiter Fallstrick ist die Bootlist selbst. Ein Schreibfehler oder eine zu
kurze/inkonsistente Bootlist kann dazu fuehren, dass `os9gen` ohne auffaellige
Meldung zurueckkehrt, aber ein zu kleines und unbrauchbares `OS9Boot` erzeugt.
Symptom beim naechsten Boot:

```text
Sysgo can't chx to 'CMDS'
Sysgo can't open 'startup' file
Error #000:215
```

`000:215` ist `E$BPNam` ("Bad pathlist specified"). In diesem Zustand ist nicht
der Emulatorstart selbst kaputt, sondern das erzeugte Bootfile findet beim
Sysgo-Start seine erwarteten Pfade nicht. Das letzte bekannte funktionierende
Bootfile hatte lokal 295408 Byte; ein fehlerhaft erzeugtes Bootfile hatte nur
163192 Byte.

Praktische Arbeitsregel: Vor jedem `os9gen`-Experiment `OS9Boot` sichern, und
nach einem Fehlschlag nur `OS9Boot` aus einer Sicherung zurueckspielen, statt
das ganze Arbeitsimage zu verwerfen.

### `q9term`, `umacs` und Windows-Eingabe

`SYS/termcap` enthaelt einen `q9|q9term`-Eintrag. Die Datei muss im OS-9-
Textformat mit CR-Zeilenenden gespeichert werden; LF-Zeilenenden wurden von
Toolshed zwar lesbar angezeigt, fuehrten aber dazu, dass `umacs` den Terminaltyp
nicht fand.

Der Windows-native HAL normalisiert erweiterte `_getch()`-Tasten auf ANSI-
Sequenzen:

| Windows `_getch()` | Gast-Sequenz | Termcap |
|--------------------|--------------|---------|
| `0xE0 0x48` | `ESC [ A` | `ku=\E[A` |
| `0xE0 0x50` | `ESC [ B` | `kd=\E[B` |
| `0xE0 0x4B` | `ESC [ D` | `kl=\E[D` |
| `0xE0 0x4D` | `ESC [ C` | `kr=\E[C` |

Direkte `termcap`-Versuche mit echtem `0xE0` oder `\340H` waren fuer `umacs`
nicht stabil; sichtbares Symptom waren einzelne Buchstaben (`H`, `M`, `P`, `K`)
im Editor. Die stabile Grenze ist daher: Host-spezifische Windows-Tastencodes
bleiben im Windows-HAL, OS-9-Programme sehen normale Terminalsequenzen.

---

## Offene Punkte (für die Phase-5-Detailplanung, s. ARBEITSPLAN.md 5.2)

- ~~REMAP-Verhalten~~ **geklärt** (s.o.): einmaliger Adresszugriff, kein Bit-Layout.
- ~~TI_IRQ_ON/OFF~~ **geklärt** (s.o.): Adress-Trigger für einen Timer, IRQ3.
- ~~Adress-Dispatch-Architektur~~ **geklärt** (s.o.): if/else-Kette, RAM zuerst.
- Welche RAM-Bestückung nimmt das konkrete Boot-ROM an (16/32/64/128 MB)? Default-Annahme
  für den ersten Versuch: 16 MB (kleinste Bestückung), konfigurierbar.
- Welche CF-Kommandos (ATA-Subset) das Boot-ROM tatsächlich benutzt.
- IRQ3-Timer: Frequenz/Periode noch nicht bekannt (nur dass er auf IRQ3 auslöst) — hängt
  auch am noch offenen Zyklen-Budget pro `q9_kernel_step()`-Aufruf.

#─────────────────────────────────────────────────────────────────────────────────────────────────
# EOF CB030.md                                                                            Ver. 1.00
#─────────────────────────────────────────────────────────────────────────────────────────────────
