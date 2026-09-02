# Q9-MMU-/SSM-Workflow

## Zweck

Dieses Dokument beschreibt die Reihenfolge, in der Q9-Hardwaremodell,
Boot-ROM, OS-9-Kernel und SSM zusammenarbeiten. Die Bus-Simulation stellt
physische Bereiche bereit; sie darf daraus nicht automatisch eine globale
User-State-Freigabe machen.

## Q9-Adresskarte

| Bereich | Funktion | Emulator | normale User-Abbildung |
|---|---|---|---|
| `$00000000-$00FFFFFF` | 16 MiB RAM | `board_ram` | durch OS-9/SSM je nach Prozess |
| `$FD000000-...` | VRAM, konfigurierbare Größe | Framebuffer-Gerät | nur nach expliziter Abbildung/Treiberfreigabe |
| `$FE000000-$FE07FFFF` | ROM nach Remap | Board-ROM-Fallback | normalerweise System-State |
| `$FFFF0000-$FFFFFFFF` | I/O-Cluster | Geräte-Registry/Board-Fallback | normalerweise System-State/Treiber |
| `$FFFF8000-$FFFF8FFF` | REMAP-Trigger | reiner Adress-Trigger | kein normaler Speicherbereich |

Aktuelle Gerätefenster liegen unter anderem bei `$FFFF2000` (QUICC),
`$FFFFA000` (MC6845), `$FFFFA010` (CLUT) und `$FFFFE000` (Onboard-CF).

## Initialisierungsreihenfolge

1. **Emulator/Board-Modell**

   RAM, ROM und Gerätefenster werden physisch registriert. Der Emulator muss
   Zugriffe außerhalb dieser Fenster ablehnen oder als Busfehler behandeln.
   Diese Ebene entscheidet nicht über OS-9-Userrechte.

2. **Boot-ROM / `sysinit`**

   Die Q9-Portdatei
   `MWOS/OS9/68030/PORTS/Q9/ROM_CBOOT/sysinit.a` setzt den PMMU-TC zunächst
   auf `$02C08444`. Das `E`-Bit ist dabei noch 0: MMU-Übersetzung ist zunächst
   abgeschaltet. Zusätzlich werden RAM-Remap, VBR und Cache-Grundzustand
   eingerichtet.

3. **Kernel und SSM**

   Erst SSM erzeugt die Root-/Seitentabellen und aktiviert später die MMU.
   Im laufenden Q9-Test sehen wir bereits `TC=$82C08444`; die MMU ist dann
   aktiv. Ein Table-B- oder Table-C-Fehler bedeutet daher normalerweise eine
   fehlende oder ungültige Seitentabelle, nicht einen fehlenden Emulator-RAM-
   Puffer.

4. **User-Prozess**

   Beim Start eines Programms müssen Code, Daten und Stack in der User-Tabelle
   abgebildet werden. Große Module müssen über ihre gesamte belegte Länge
   abgebildet werden; `M$Mem` beschreibt nur den zusätzlichen Datenbedarf und
   ersetzt nicht die Abbildung des Modul-Codes.

## Erste Bootstrap-Konfiguration

Für die frühe QCC-/Kernel-Entwicklung kann im Init-Modul `M$SysConf` das Flag
`SSM_NoProt` (`1<<4`) gesetzt werden. OS-9/SSM baut dann eine gemeinsame
User-Seitentabelle für den bekannten Speicher auf. Das ist praktisch für die
Fehlersuche, aber keine endgültige Schutzkonfiguration.

Im aktuell verwendeten Q9-Bootimage `OS9SYS.qcc-xcc-test.hda` ist der Wert des
Init-Feldes `M$SysConf` momentan `$0000`. `SSM_NoProt` ist dort also nicht
gesetzt; die normale geschützte Per-Prozess-Abbildung ist aktiv. Das ist der
aktuelle Ausgangszustand, kein bereits aktivierter Vollzugriffsmodus.

Für den Gegenversuch wurde ein separates Image
`local_images/OS9SYS.qcc-xcc-noprot-test-v2.hda` erzeugt. Sein Init-Modul hat
`M$SysConf=$0010`, CRC und Header-Parität sind gültig, und das Image bootet bis
zum Login. Der QCC-Prozess scheitert dort trotzdem weiterhin beim ersten
ungültigen Datenzugriff. `SSM_NoProt` allein behebt das Problem also nicht.
Der aktuelle PMMU-Lauf
zeigt inzwischen, dass die Tabellenwalk-Logik bis zum fehlerhaften Zugriff
läuft. Der Abbruch ist kein fehlendes RAM-/ROM-/I/O-Mapping: Bei `PC=$008102DE`
steht der Opcode `$1010` (`move.b (a0),d0`), während `A0=$00000000` ist. QCC
greift dort also tatsächlich über einen Nullzeiger auf den User-Adressraum zu.
Die frühere Diagnoseadresse `$008102DC` war ein veralteter globaler
Zugriffswert, der durch den verschachtelten MMU-Tabellenwalk überschrieben
wurde. Für weitere Fehlerberichte muss deshalb die beim Eintritt in den
PMMU-Walk übergebene Adresse verwendet werden.

Die dauerhafte Konfiguration sollte danach wieder pro Prozess abbilden:

- RAM-Seiten für Code, Daten und Stack des Prozesses;
- gemeinsame, schreibgeschützte ROM-Seiten nur falls erforderlich;
- VRAM und I/O nur über einen passenden System-State-Treiber oder eine bewusst
  eingerichtete Geräteabbildung;
- Gerätebereiche cache-inhibit bzw. mit dem passenden Cache-Modus;
- nicht vorhandene Bereiche als ungültig markieren.

## Supervisor-Zugriff

Supervisor ist kein normaler Freischalt-Systemcall. Das 68030-S-Bit wählt den
Supervisor-Kontext; bei getrennter Root-Tabelle verwendet die MMU dafür den
Supervisor-Root-Pointer. Kernel und System-State-Treiber laufen in diesem
Kontext. Ein User-Programm darf nicht einfach selbst TC, Root-Pointer oder
Seitentabellen ändern.

Relevante OS-9-Dienste sind:

- `F$ChkMem`: prüfen, ob ein User-Prozess einen Bereich verwenden darf;
- `F$SRqMem`: Speicher anfordern;
- `F$MapBlk`: einen konkreten Speicherblock abbilden;
- `F$Trans`: Adressübersetzung zum externen Bus.

Diese Dienste ersetzen keine beliebige User-Freigabe von Kernel-I/O. Für
VRAM/I/O ist der normale Weg ein System-State-Treiber, der die Hardware kennt
und sichere Zugriffe anbietet.

## Prüfplan im Emulator

Für jeden Bootlauf sollten wir mindestens protokollieren:

1. TC, SRP und CRP beim Übergang von `sysinit` zu SSM;
2. die `M$SysConf`-Flags des geladenen Init-Moduls;
3. die Root-/Table-A-/B-/C-/D-Deskriptoren für eine RAM-Adresse im Modul,
   eine Adresse am Modulende, VRAM und I/O;
4. Function-Code und Zugriffsart: User/Supervisor, Instruction/Data,
   Read/Write;
5. ob die physische Zieladresse im Board-RAM oder in einem registrierten Gerät
   landet.

Für den aktuellen QCC-Fehler ist besonders der Bereich um `PC=$008102DE`
zu prüfen. Dort muss QCC zunächst den Nullzeigerpfad beziehungsweise die
fehlende Initialisierung des verwendeten Zeigers erklären; eine Erweiterung
der globalen Geräte- oder RAM-Freigaben wäre dafür die falsche Lösung.

## Sicherheitsregel für die Hardware-Simulation

Die Hardware-Simulation darf physische Geräte vollständig bereitstellen, aber
die MMU-/SSM-Schicht muss weiterhin zwischen System-State und User-State
unterscheiden. Ein globales "alles les- und schreibbar" ist nur als zeitlich
begrenzter `SSM_NoProt`-Bootstrapmodus sinnvoll.
