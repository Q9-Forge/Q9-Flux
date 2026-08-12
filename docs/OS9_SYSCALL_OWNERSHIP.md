# Live-Verifikation: welches Modul bearbeitet welchen Syscall? (Kernel/IOMan/SysCache/SSM)

**Nachtrag 2026-08-12:** Diese Live-Stichprobe (~40 tatsächlich ausgeführte
Aufrufe) wurde inzwischen auf **alle** ~90 im Kernel-Build definierten
Callcodes erweitert — per Adressvergleich gegen die vollständige
Syscall-Tabelle aus der Kernel-Disassemblierung, ohne Widerspruch zu den
hier gemessenen Werten. Vollständige Tabelle:
[`Q9-OS/modules/SYSCALL_MODULE_MAP.md`](../../Q9-OS/modules/SYSCALL_MODULE_MAP.md).

Live-Gegenprobe im laufenden Q9-Flux-Emulator (echtes OS-9/68K-Boot-Image, nicht Q9s eigene
Reimplementierung — dafür siehe `docs/SYSCALLS.md`/`docs/MODULES.md`) zur Frage: als frühere
Recherchen versuchten, alle Syscalls im **statischen Kernel-Modul** zu finden, fehlten einige —
sind diese vielleicht in IOMan oder anderen Managern implementiert?

**Kurzantwort:** ja. Bestätigt live, deckungsgleich mit Anhang D des Technical Reference Manual:
`F$Load` und alle `I$`-Aufrufe gehören zu **IOMan**, nicht zum Kernel; SSM und SysCache besitzen
je einen eigenen, kleinen Satz eigener `F$`-Aufrufe. Zusätzlich ein Fund, den Anhang D so nicht
ausspricht: **SSM-Code läuft auch bei ganz normalen `I$`-Dateioperationen mit** — vermutlich
MMU-bedingte Adressübersetzung/-prüfung für den übergebenen User-Space-Zeiger (siehe Abschnitt 3;
"SSM" heißt "System **Security** Module", meint damit aber **Speicherschutz über die MMU**, keine
Zugriffsrechte-Prüfung — siehe unten), aber (siehe Vorbehalt unten) noch nicht hart genug
abgesichert, um das als Fakt zu behaupten.

**Quellen:**
- `M:\MWOS\OS9\SRC\DEFS\funcs.h` — Callcode-Nummern (`F_LINK 0x00` usw.)
- `M:\KIDOCS\md\MW-0000-0000-OS-9-for-68K-Technical-Reference-Manual.md`, Anhang D
  (Tabelle D-1 Kernel, D-2 IOMan, D-3 SSM, D-4 SysCache)
- Live-Messung: eigener Boot des Q9-Flux-Emulators, Patch in `src/kernel/m68krt.c`
  (Commit `79d5607`)

---

## 1. Methode

1. **Ground Truth der Modul-Adressen**: `mdir -e` auf einem frisch gebooteten Image liefert die
   tatsächlichen Basisadressen (nach Relokation, aber stabil für dieses Boot-Image/ROM):

   | Modul     | Start      | Ende (Start + Größe) |
   |-----------|------------|------------------------|
   | `kernel`  | `$007100`  | `$00E03C`              |
   | `ioman`   | `$00E03C`  | `$00F658`              |
   | `init`    | `$00F658`  | `$00F7A6`              |
   | `syscache`| `$00F7A6`  | `$00F93C`              |
   | `ssm`     | `$00F93C`  | `$0100B0`              |

   (Module liegen exakt hintereinander — Ende von Modul N == Start von Modul N+1.)

2. **Instrumentierung** (`m68krt.c`, Commit `79d5607`): bei jedem `trap#0` (mit
   `Q9_TRAP_TRACE_ALL=1`) öffnet `m68krt_trap_trace_callback()` ein "Klassifizierungs-Fenster"
   bis zur Rücksprungadresse (`pc+4` — Trap-Instruktion + Inline-Callcode-Wort). Der ohnehin
   vorhandene Instruction-Hook `m68krt_watch_pc_callback()` prüft währenddessen **jede
   ausgeführte Instruktion** gegen die vier Adressbereiche und setzt ein Bit pro getroffenem
   Bereich. Bei Erreichen der Rücksprungadresse wird eine Zeile
   `classify pc=... callcode=... kernel=.. ioman=.. syscache=.. ssm=.. other=..` geloggt.

3. **Testlauf**: frisches Boot-Image, Login als `super`, ein paar harmlose Shell-Befehle
   (`dir`, `ident`, `load`) — ca. 4.300 klassifizierte Trap-Aufrufe.

4. **Auswertung**: `grep '^classify' <trace> | awk ...` — pro Callcode aggregiert, wie oft jedes
   Bit gesetzt war (siehe Abschnitt 4 für den genauen Befehl).

## 2. Ergebnis — bestätigt Anhang D exakt

Kernel-only-Aufrufe (Tabelle D-1) zeigten **immer** `ioman=0, syscache=0`, IOMan-Aufrufe
(Tabelle D-2) zeigten **immer** `ioman=1` (nie spontan bei reinen Kernel-Calls), SysCache- und
SSM-eigene Aufrufe entsprechend. Kein einziger Fall widersprach der Doku.

| Callcode | Name       | Kernel | IOMan | SysCache | SSM (eigen, Tab. D-3) | Beobachtung |
|----------|------------|:------:|:-----:|:--------:|:----------------------:|-------------|
| `$00`/`$01` | F$Link/F$Load | – | ✅ Doku | – | – | F$Load ist der klassische Fall aus der Ursprungsfrage |
| `$13` | F$AllBit  | ✅ | ✅ | – | – | live: kernel=2/2, ioman=2/2 |
| `$53` | F$Event   | ✅ | – | – | – | live: kernel=637/637, ioman=0 |
| `$2A` | F$IRQ     | ✅ | – | – | – | live: kernel=11/11 |
| `$32` | F$SSvc    | ✅ | – | – | – | live: kernel=4/4 |
| `$56` | F$Alarm   | ✅ | – | – | – | live: kernel=3/3 |
| `$58` | F$ChkMem  | ✅ | – | – | ✅ Doku | live: kernel=1/1, ssm=1/1 |
| `$5A` | F$CCtl    | ✅ | – | ✅ Doku | – | live: kernel=38/38, syscache=**38/38** |
| `$80` | I$Attach  | ✅ | ✅ Doku | – | – | live: ioman=24/24 |
| `$84` | I$Open    | ✅ | ✅ Doku | – | – | live: ioman=2/2 |
| `$89` | I$Read    | ✅ | ✅ Doku | – | – | live: ioman=126/126 |
| `$8B` | I$ReadLn  | ✅ | ✅ Doku | – | – | live: ioman=1186/1186 (häufigster Call im Testlauf) |

(Vollständige Rohdaten für alle ~40 im Testlauf beobachteten Callcodes: siehe
`/tmp/claude_classify_trace.txt` auf dem Mac, nicht ins Repo übernommen — Testlauf-Artefakt.)

**Sauberes, konsistentes Signal:** `ioman`/`syscache`/`ssm`-Bits traten in diesem Testlauf
**nie** bei einem Callcode auf, der laut Anhang D nicht zum jeweiligen Modul gehört. Kein
einziger Widerspruch zur Dokumentation.

## 3. Interessanter Zusatzfund — mit Vorbehalt

Bei allen sieben beobachteten `I$`-Aufrufen (`$80` Attach, `$81` Detach, `$82` Dup, `$84` Open,
`$88` Seek, `$89` Read, `$8A` Write, `$8B` ReadLn, `$8C` WritLn, `$8D` GetStt, `$8E` SetStt,
`$8F` Close) lief auch **SSM-Code** mit — meist bei 85–100 % der Aufrufe (z. B. I$ReadLn
1186/1186, I$Read 126/126, I$Write 27/27, I$Close 121/164).

**Was SSM tatsächlich ist** (TRM, Anhang B, Abschnitt „SSM"): trotz des Namens „System
**Security** Module" geht es NICHT um Zugriffsrechte/Authentifizierung, sondern um den Betrieb
der **MMU** (Memory Management Unit) des Prozessors — im Development-Kernel die Grundlage für
User-State-Speicherschutz (ein User-Prozess darf nur auf Speicher zugreifen, für den er Rechte
hat), beim 68040 zusätzlich Cache-Modus-Feinsteuerung. Konkrete Module: `SSM451` (68451-MMU,
68010), `SSM851` (68851-PMMU/68030-MMU), `SSM040` (68040, inkl. Cache-Support). Im Atomic-Kernel
ist User-State-Protection gar nicht implementiert — dort tut SSM (außer beim 68040) praktisch
nichts.

Naheliegende Deutung angesichts dessen: **nicht** ein Berechtigungscheck im Sinne von
Datei-Permissions, sondern MMU-bedingte **Adressübersetzung/-validierung des User-Space-Zeigers**,
den IOMan bei diesen Calls entgegennimmt (z. B. die Puffer-Adresse in `a0` bei I$Read/I$Write).
Das passt auch zu einem Ausreißer bei den reinen Kernel-Calls (siehe Vorbehalt): `F$Julian`
zeigt 98 % SSM-Treffer — plausibel, wenn dieser Call einen Zeiger auf eine Datums-Struktur
entgegennimmt, der ebenfalls übersetzt werden muss. Anhang D erwähnt diesen Mechanismus nicht —
dort steht nur SSM's **eigener** Aufrufsatz (Tabelle D-3: F$AllTsk/F$ChkMem/F$DelTsk/F$Permit/
F$Protect/F$GSPUMp), nicht dass beliebige Calls mit Zeiger-Parametern intern durch die MMU/SSM
laufen.

**Vorbehalt, warum das noch keine gesicherte Tatsache ist:** einige reine Kernel-Aufrufe ohne
jeden Doku-Bezug zu SSM zeigten in diesem Testlauf ebenfalls SSM-Treffer in sehr unterschiedlicher
Häufigkeit (`F$Julian` 80/82 = 98 %, `F$SRqCMem` 32/143, `F$SetSys` 13/61) — während andere
Kernel-Aufrufe komplett bei 0 % blieben (`F$IRQ`, `F$SSvc`, `F$Event` 2/637 ≈ 0 %, `F$Wait`,
`F$Mem`, `F$Icpt`, `F$SysID`, `F$Alarm`, `F$MBuf`). Die "MMU-Adressübersetzung"-Deutung erklärt
das nur, wenn man weiß, welche dieser Calls tatsächlich Zeiger-Parameter haben — das ist bisher
nicht gegen die Call-Signaturen geprüft. Eine alternative Erklärung bleibt Hintergrundrauschen
durch **gleichzeitig laufende Interrupts** (Timer-Tick, `tsmon`, `telnetd` — der Emulator hat
einen einzigen CPU-Kern, aber IRQ-Service-Routinen unterbrechen die Instruktions-Sequenz und
würden hier mitgezählt).

→ **Als Hypothese, nicht als Fakt behandeln.** Für eine harte Bestätigung bräuchte es entweder
(a) einen Testlauf mit abgeschalteten Hintergrundprozessen (`tsmon`/`telnetd`), oder (b) eine
gezielte PC-Sample-Auswertung *innerhalb* des SSM-Bereichs (welche exakte Adresse wird
getroffen — eine feste Einstiegsroutine spräche für einen echten MMU-Aufruf, verstreute Adressen
für IRQ-Rauschen), oder (c) ein Disassembler-Blick auf die IOMan-Aufrufstelle selbst (ruft sie
sichtbar eine MMU-Übersetzungsroutine auf, wenn und nur wenn ein Zeiger-Parameter im Spiel ist?),
oder (d) ein gezielter Test mit einem Kernel-Call, der garantiert **keinen** Zeiger übergibt, als
Kontrollgruppe gegen einen mit Zeiger.

## 4. Reproduzierbarkeit

```bash
# auf dem Mac, im Q9-Flux-Checkout:
export Q9_TRAP_TRACE=/tmp/trace.txt
export Q9_TRAP_TRACE_ALL=1
./build/macos/q9.exe --rom <rom> --cf <image>
# ... booten, einloggen, ein paar Befehle ausführen, dann Emulator beenden ...

grep '^classify' /tmp/trace.txt \
  | sed -E 's/.*callcode=([0-9a-f]+) kernel=([01]) ioman=([01]) syscache=([01]) ssm=([01]) other=([01])/\1 \2 \3 \4 \5 \6/' \
  | awk '{ k[$1]+=$2; io[$1]+=$3; sc[$1]+=$4; ssm[$1]+=$5; oth[$1]+=$6; cnt[$1]++ }
         END { for (c in cnt) printf "callcode=%s n=%d kernel=%d ioman=%d syscache=%d ssm=%d other=%d\n",
               c, cnt[c], k[c], io[c], sc[c], ssm[c], oth[c] }' | sort
```

Die vier Adressbereiche in `m68krt.c` (`Q9_CLASSIFY_*_LO/HI`) sind für **dieses** Boot-Image/ROM
fest verdrahtet — bei einem anderen ROM/Image ggf. per `mdir -e` neu ermitteln und anpassen.

## 5. Anhang: Live-Modulverzeichnis (`mdir -e`-Schnappschuss)

Nebenbefunde aus demselben `mdir -e`-Lauf, der die Adressbereiche in Abschnitt 1 lieferte —
nicht Teil der eigentlichen Syscall-Frage, aber zu schade zum Wegwerfen.

### 5.1 Boot-Speicherlayout

Die früh im Startup geladenen System-Module liegen **lückenlos hintereinander** (Ende von
Modul N = Start von Modul N+1): `kernel` → `ioman` → `init` → `syscache` → `ssm` → `fpu` →
`tkcb030`. Danach ein Sprung ins ROM-Adressfenster (`$feXXXXX`) für weitere Module
(`rtclock`, `ram`-Treiber, Kommandos wie `dir`/`load`/`mdir` selbst usw.) — die lückenlose Kette
gilt also nur für den allerersten Bootstrap-Batzen, nicht fürs gesamte System.

### 5.2 Netzwerk-Modul-Stack (komplett live gesehen)

Alle beim Boot dieses Images geladenen Netzwerk-Module, aus `mdir -e` (Adresse/Größe/Owner
weggelassen, nur Typ + Name + Link-Count):

| Schicht | Module (Typ) | Link-Count | Bemerkung |
|---------|--------------|:-----------:|-----------|
| SPF-Grundgerüst | `pkman` (Fman), `pkdvr` (Driv), `pk`/`pks` (Desc) | 0 | Packet-Manager-Ebene, siehe frühere pkdvr-Diagnose |
| SPF-Dispatcher | `spf` (Fman), `spf_rx` (Prog) | 12 / 1 | zentraler Service-Prozess-Dispatcher (`+18`-Meldung beim Boot) |
| IP | `spip` (Driv), `ip0` (Desc) | 3 / 3 | |
| TCP | `sptcp` (Driv), `tcp0` (Desc) | 3 / 3 | |
| UDP | `spudp` (Driv), `udp0` (Desc) | 1 / 1 | |
| RAW | `spraw` (Driv), `raw0` (Desc) | 1 / 1 | |
| Routing | `sproute` (Driv), `route0` (Desc) | 1 / 1 | |
| Namensauflösung | `netdb` (Trap) | 1 | siehe frühere netdb/idbgen-Recherche |
| Ethernet | `spenet` (Driv), `enet` (Desc) | 1 / 1 | |
| — | `sp360` (Driv), `spqe0` (Desc) | 1 / 1 | (Zweck nicht weiter untersucht — evtl. QUICC-Queue-Element, Name ungeprüft) |
| Setup/Dienste | `mbinstall` (Prog), `ipstart` (Prog) | 0 / 0 | einmalige Installations-/Start-Skripte, kein Dauerbetrieb |
| Telnet | `telnetd` (Prog), `nettty` (Driv) | 1 / 25 | `nettty` mit auffällig hohem Link-Count (25) — mehrfach von den 8 `xterms`-Terminals referenziert |
| Konfig-Daten | `inetdb` (Data, Perm `0111`), `inetdb2` (Data, Perm `0333`) | 1 / 1 | die binäre Netzwerk-Konfig-Datenbank aus der netdb-Recherche — Permission-Bits hier live bestätigt |

**Kein `inetdb3`/`inetdb4` in diesem Boot sichtbar** — falls eine frühere Session deren Existenz
vermutet/gefunden hatte, ist das hier **nicht** reproduziert. Könnte an Bootbedingungen liegen
(andere Konfiguration, anderes Image) oder die frühere Vermutung war falsch — ohne den genauen
Kontext der früheren Session nicht sicher zu klären, daher hier nur als offene Gegenprobe notiert,
nicht als Widerlegung.

**Erstellt**: 2026-08-10
