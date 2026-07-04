#═════════════════════════════════════════════════════════════════════════════════════════════════
# File:   HANDBUCH.md                                                                     Ver. 1.00
# Owner:  AF
# Desc.:  Zentrales Handbuch: Werkzeuge, Quellcode-Layout, Build je Target, Software-Architektur,
#         Referenzquellen samt Lizenzlage. Gedacht als Einstiegspunkt für jeden, der das Projekt
#         zum ersten Mal sieht — auch für den Fall einer künftigen Veröffentlichung. Verweist auf
#         die bestehenden Fachdokumente statt sie zu duplizieren.
#
# Edition History
#─────────┬──────┬─────────────────────────────────────────────────────────────────────────┬──────
# Date    │ Ver. │ Description                                                             │ By
#─────────┼──────┼─────────────────────────────────────────────────────────────────────────┼──────
# 26-07-04│ 1.00 │ Initiale Version, nach Abschluss Phase 3 (Filesystem)                   │ CF
#═════════╧══════╧═════════════════════════════════════════════════════════════════════════╧══════

# Q9 — Handbuch

Q9 ist ein modulares Mini-Betriebssystem in der Tradition von Microware OS-9:
gleiches Grundkonzept (Modulsystem, einheitliches I/O über Syscalls, Geräte als
Module), aber ein komplett neuer, portabler C-Kern — kein Assembler, keine
Binärkompatibilität zu OS-9. Der Kern läuft unverändert im Browser (WebAssembly)
und nativ auf dem Entwicklungsrechner (macOS/Linux/Windows); ein natives 68k-Ziel
ist für eine spätere Phase vorgesehen.

Dieses Handbuch bündelt alles, was jemand braucht, der das Projekt zum ersten Mal
aufschlägt: Werkzeuge, Quellcode-Aufbau, Build-Prozess, Architektur, Lizenzlage.
Für Details verweist es auf die Fachdokumente in `docs/` statt sie zu wiederholen.

**Verwandte Dokumente:**
- [`../PROJECT.md`](../PROJECT.md) — Vision, Entscheidungshistorie (E1–E8), offene Fragen (O1–O5)
- [`../ARBEITSPLAN.md`](../ARBEITSPLAN.md) — laufender Arbeitsstand, Schritt für Schritt (internes Arbeitsdokument, siehe unten)
- [`SYSCALLS.md`](SYSCALLS.md) — vollständige Syscall-ABI (Register, Fehlercodes)
- [`SYSCALL_ROADMAP.md`](SYSCALL_ROADMAP.md) — alle 97 OS-9-Syscalls mit Q9-Status
- [`DEVICES.md`](DEVICES.md) — Geräte-/Pfadtabelle, Treiber-Schnittstelle
- [`MODULES.md`](MODULES.md) — OS-9-Modulsystem als Referenz für Q9s eigenes
- [`TOOLCHAIN.md`](TOOLCHAIN.md) — Toolchain-Stand je Entwicklungsrechner (Versionen, Pfade)
- [`AUTONOMIE.md`](AUTONOMIE.md) — Setup für den automatisierten Arbeitsmodus (projektintern, für Aussenstehende irrelevant)

---

## Inhalt

1. [Lizenz und Rechtslage](#1-lizenz-und-rechtslage)
2. [Werkzeuge und Installation](#2-werkzeuge-und-installation)
3. [Quellcode-Layout](#3-quellcode-layout)
4. [Bauen für die einzelnen Targets](#4-bauen-für-die-einzelnen-targets)
5. [Software-Architektur](#5-software-architektur)
6. [Stand der Dinge](#6-stand-der-dinge)
7. [Referenzquellen und ihre Lizenzen](#7-referenzquellen-und-ihre-lizenzen)
8. [Glossar](#8-glossar)

---

## 1. Lizenz und Rechtslage

**Der Lizenzstatus von Q9 selbst ist noch offen** (README.md: „TBD"). Bevor das
Projekt öffentlich wird, muss das geklärt werden — insbesondere im Zusammenspiel
mit den in Abschnitt 7 gelisteten Referenzquellen, von denen manche frei
weitergebbar sind (GPL) und manche ausdrücklich **nicht** (private MWOS-SDK-Kopie,
urheberrechtlich geschütztes Buch).

**Wichtigste Regel bis zur Klärung:** Kein Code aus proprietären/privaten Quellen
(MWOS) und kein abgetippter Code aus urheberrechtlich geschütztem Material (Bücher)
landet im Q9-Quellbaum. Diese Quellen dienen ausschließlich als *Referenz zum
Verstehen* — implementiert wird eigenständig. Bei GPL-Quellen (NitrOS-9, OS9exec)
ist Übernehmen rechtlich zulässig, wird aber aktuell ebenfalls nicht praktiziert
(Q9 ist komplett eigenständig geschrieben); sollte sich das ändern, zieht die
GPL automatisch nach (Copyleft) — das muss bei der Lizenzentscheidung für Q9
mitgedacht werden.

Details und Kontext zu den einzelnen Quellen: Abschnitt 7.

---

## 2. Werkzeuge und Installation

Alle Werkzeuge werden **portabel installiert, ohne Admin-/Root-Rechte** —
Deinstallation heißt schlicht: Ordner löschen. Der volle, rechnerspezifische
Stand (genaue Pfade und Versionen je Entwicklungsrechner) steht in
[`TOOLCHAIN.md`](TOOLCHAIN.md); hier die Zusammenfassung nach Zweck.

### 2.1 Nativer Build (macOS / Linux)

| Werkzeug | Zweck | Installation |
|----------|-------|---------------|
| C-Compiler + make | Kernel + HAL für den Entwicklungsrechner übersetzen | macOS: Xcode Command Line Tools (`xcode-select --install`, liefert clang + make); Linux: übliches `build-essential`/`gcc`-Paket der Distribution |
| Python 3 | Testsuite (`test/*.py`) | meist vorinstalliert; sonst über die Distribution/Homebrew |

Kein Emscripten, kein sonstiges Spezialwerkzeug nötig — der native Build ist die
niedrigste Einstiegshürde und der Ort, an dem der Selbsttest läuft.

### 2.2 Nativer Build (Windows)

| Werkzeug | Zweck | Installation |
|----------|-------|---------------|
| w64devkit | gcc + make + busybox-sh, portabel | ZIP von der [w64devkit-Releaseseite](https://github.com/skeeto/w64devkit) entpacken, `bin/`-Ordner in den `PATH` der Shell aufnehmen |
| Python 3 | Testsuite | offizieller Python-Installer (portable ZIP-Variante ebenfalls möglich) |

### 2.3 Browser-Build (WebAssembly, alle Plattformen)

| Werkzeug | Zweck | Installation |
|----------|-------|---------------|
| Emscripten SDK (emsdk) | `emcc`, kompiliert den Kernel zu WASM + JS-Glue-Code | `git clone https://github.com/emscripten-core/emsdk.git`, darin `./emsdk install latest && ./emsdk activate latest` (Windows: `emsdk.bat`) |
| Node.js | wird von emsdk mitinstalliert, für den Build selbst gebraucht | kommt mit emsdk, keine separate Installation nötig |

Vor jedem `make wasm` muss die emsdk-Umgebung in der aktuellen Shell aktiviert
werden (**gilt nur für diese eine Shell-Session**, nicht dauerhaft):

```bash
# macOS/Linux
source <emsdk-pfad>/emsdk_env.sh
```
```powershell
# Windows PowerShell
<emsdk-pfad>\emsdk_env.ps1
```

Für den nativen Build (`make native`, `make test`) wird emsdk **nicht** gebraucht.

### 2.4 68k-Target (Vinculum / MC68EN360, geplant ab Phase 7)

| Werkzeug | Zweck | Stand |
|----------|-------|-------|
| vbcc (M68k-Backend) | erzeugt positionsunabhängigen 68k-Code für die HAL und für LANG_68K-Module | noch nicht installiert, kommt mit Phase 7 |

### 2.5 Versionskontrolle

Git, Remote auf GitHub (aktuell privates Repository). Kein Werkzeug-Sonderfall,
aber erwähnt, weil der komplette Arbeitsablauf (Autonomie-Betrieb, siehe
[`AUTONOMIE.md`](AUTONOMIE.md)) auf `git commit`/`git push` nach jedem
abgeschlossenen Schritt aufsetzt.

---

## 3. Quellcode-Layout

```
Q9/
├── PROJECT.md            Vision, Architektur-Entscheidungen (E1-E8), offene Fragen (O1-O5)
├── ARBEITSPLAN.md         laufender Arbeitsstand (internes Arbeitsdokument, siehe Hinweis unten)
├── context.txt            Kurz-Zusammenfassung für den Sessionwechsel (ebenfalls intern)
├── README.md              englische Kurzbeschreibung
├── Makefile               Build-System, siehe Abschnitt 4
├── .gitignore
├── src/
│   ├── kernel/            portabler Kern — KEIN Target-spezifischer Code, reines C99
│   │   ├── kernel.c/.h        Boot, Selbsttest, Kernel-Tick (q9_kernel_step)
│   │   ├── syscall.c/.h       Dispatcher, Register-Konvention, Funktions-/Fehlercodes
│   │   ├── device.c/.h        Geräte-/Pfadtabelle (IOMan-Vorbild)
│   │   ├── dev_term.c         Konsolen-Treiber (/term)
│   │   ├── dev_nil.c          Null-Device (/nil)
│   │   ├── dev_d0.c           Block-Geräte-Wrapper um die HAL (/d0)
│   │   ├── name.c/.h          F$PrsNam/F$CmpNam (Pfadnamen-Parsing nach OS-9-Regeln)
│   │   ├── module.c/.h        Modul-Header, CRC32, Modul-Directory, F$Link/F$UnLink/F$Load
│   │   ├── vfs.c/.h           Dateisystem-Routing, austauschbarer File-Manager (q9_fm_t)
│   │   ├── fat16.c/.h         FAT16-File-Manager (lesend + schreibend, LFN-Lesen)
│   │   └── proc.c/.h          Prozess-Descriptor-Tabelle + Round-Robin-Scheduler (Phase 4)
│   └── hal/               Hardware Abstraction Layer — hier UND NUR hier ist Code Target-spezifisch
│       ├── q9_hal.h           die schmale Schnittstelle, die jedes Target erfüllen muss
│       ├── native/            Windows-HAL (conio.h) — Host-Loop (main) liegt hier
│       ├── posix/             macOS/Linux-HAL (termios) — Host-Loop liegt hier
│       └── wasm/              Browser-HAL (Emscripten) — kein main(), stattdessen
│                               q9_kernel_init/q9_kernel_step als exportierte Funktionen
├── tools/                 PC-seitige Werkzeuge (z.B. künftig `q9mod`, Modul-Packer)
├── web/                   Browser-Frontend: index.html (xterm.js), worker.js (Kernel im Worker)
├── test/                  01_test_boot.py, 02_..., je Testskript PASS/FAIL, aus dem Projekt-Root
│                          aufrufbar nach `make native`
└── docs/                  Fachdokumente (dieses Handbuch + Detail-Spezifikationen)
```

### 3.1 Regeln, an die sich der Quellcode hält

Diese Regeln sind keine Stilfrage, sondern tragen die Portabilität des Projekts:

- **`src/kernel/` bleibt zu 100 % target-unabhängiges C99.** Kein `#ifdef _WIN32`,
  kein `#include <windows.h>` — jeder Unterschied zwischen Windows/macOS/Linux/
  Browser/(später)68k gehört ausschließlich in die jeweilige HAL-Datei unter
  `src/hal/`.
- **Kein `malloc` im Kernel.** Alle Tabellen (Geräte, Pfade, Modul-Directory,
  Load-Puffer) sind statische Arrays fester Größe — analog zu OS-9s
  ROM-orientiertem Design, und weil ein WASM-Modul ohne Heap-Fragmentierung
  auskommen soll.
- **Jede Datei trägt einen Box-Header** (Datei/Owner/Beschreibung/Aufruf) und
  eine **Edition History** (Datum, Version, Beschreibung, Kürzel) — siehe jede
  bestehende `.c`/`.h`-Datei als Vorlage. So bleibt die Entstehungsgeschichte
  auch ohne `git blame` lesbar.
- **Die HAL-Schnittstelle (`q9_hal.h`) ist bewusst schmal**: Konsole, Timer,
  Block-Device, Systemzeit, Target-Name. Alles andere (Geräte-Semantik,
  Pfadverwaltung, Dateisystem) ist Kernel-Sache und läuft überall gleich.
- **Syscall-Nummern, Fehlercodes und Registerkonvention folgen OS-9** (Quelle:
  MWOS-Referenz — private Quelle, siehe Abschnitt 7 — nicht Teil des
  Repositories). Details: [`SYSCALLS.md`](SYSCALLS.md).

`ARBEITSPLAN.md`, `context.txt` und `docs/AUTONOMIE.md` sind **Arbeitsprozess-
Dokumente** für die Zusammenarbeit zwischen Andreas und Claudia (Claude Code) —
sie dokumentieren *wie* gearbeitet wird (Freigabe-Workflow, automatisierte
Läufe), nicht *was* Q9 ist. Für eine öffentliche Version sind sie nicht
gedacht mitzugehen; dieses Handbuch und PROJECT.md sind die stabilen,
publikationsfähigen Dokumente.

---

## 4. Bauen für die einzelnen Targets

Alles über das `Makefile` im Projekt-Root:

```bash
make native   # -> build/native/q9.exe  (Windows: conio-HAL; macOS/Linux: POSIX-HAL, automatisch gewählt)
make wasm     # -> build/wasm/q9.js, q9.wasm, index.html, worker.js  (braucht aktivierte emsdk-Umgebung)
make test     # baut native + führt alle Testskripte aus test/ aus
make clean    # entfernt build/
```

Die Wahl zwischen Windows- und POSIX-HAL im `native`-Target passiert automatisch
über die Make-Variable `$(OS)` (unter Windows von `cmd`/PowerShell gesetzt) —
kein manuelles Umschalten nötig.

### 4.1 Nativer Build ausprobieren

```bash
make native
./build/native/q9.exe            # interaktive REPL
./build/native/q9.exe --selftest # Selbsttest, Exit-Code 0 = PASS
```

### 4.2 Browser-Build ausprobieren

```bash
source <emsdk-pfad>/emsdk_env.sh   # einmal pro Shell-Session
make wasm
python3 -m http.server 8000 -d build/wasm
# dann http://localhost:8000 im Browser öffnen
```

Der Kernel läuft im Browser in einem Web Worker (`worker.js`) — nötig, damit das
Origin Private File System (OPFS) für das Block-Device synchron zugreifen kann
und der UI-Thread frei bleibt (Entscheidung E6 in PROJECT.md).

### 4.3 Tests

`test/NN_test_*.py` sind eigenständige Python-Skripte (kein Test-Framework),
jedes meldet `PASS`/`FAIL` und einen Exit-Code. Sie rufen `build/native/q9.exe
--selftest` auf und prüfen Banner-Text, Exit-Code sowie die im Kernel-Selbsttest
mitlaufenden Prüf-Zähler. `make test` führt alle nacheinander aus.

### 4.4 68k-Target (Vinculum) — noch nicht umgesetzt

Ab Phase 7 vorgesehen: `src/hal/m68k/` (analog zu `native/`/`posix/`/`wasm/`),
Übersetzung mit vbcc, Boot auf dem MC68EN360-Board bzw. einem Board-Emulator.

---

## 5. Software-Architektur

### 5.1 Schichtenmodell

```
┌─────────────────────────────────────────────────────┐
│  User-Module (Shell, Tools, Anwendungen) — geplant   │
├─────────────────────────────────────────────────────┤
│  Q9-Kernel (portables C99):                          │
│  Syscall-Dispatcher · Geräte-/Pfadtabelle · Modul-   │
│  Directory · VFS/File-Manager · Prozess-Scheduler    │
├─────────────────────────────────────────────────────┤
│  HAL (pro Target implementiert)                      │
├──────────────┬──────────────┬────────────────────────┤
│  native/      │  posix/      │  wasm/                 │
│  Windows      │  macOS/Linux │  Browser + Worker/OPFS  │
│  (conio)      │  (termios)   │                         │
└──────────────┴──────────────┴────────────────────────┘
```

### 5.2 Syscall-Schicht

Ein Syscall ist ein Aufruf von `q9_syscall(nummer, &registersatz)`. Der
Registersatz (`q9_regs_t`: `d[0..7]`, `a[0..7]`) ahmt bewusst den 68k-
Registersatz nach — das hält eine spätere 68k-Runtime dünn (ein „TRAP" bildet
1:1 auf `q9_syscall` ab, ohne Register umzupacken). Nummern und Fehlercodes
folgen OS-9 (siehe Abschnitt 7 zur Quelle). Vollständige Referenz:
[`SYSCALLS.md`](SYSCALLS.md), [`SYSCALL_ROADMAP.md`](SYSCALL_ROADMAP.md).

### 5.3 Geräte-/Pfadmodell

Nach IOMan-Vorbild: eine Gerätetabelle (Name → Treiber-Funktionen) und eine
Pfadtabelle (offener Pfad → Gerät + Modus + Dateisystem-Kontext). Treiber sind
aktuell interne C-Funktionen; ab Phase 2.4 (geplant) werden sie echte Module.
Details: [`DEVICES.md`](DEVICES.md).

### 5.4 Modulsystem

Jedes Modul trägt einen 28-Byte-Header (Sync-Bytes "Q9", Größe, Name-Offset,
Typ, **Language-Byte**, Revision, Einsprungpunkt, CRC32). Das Language-Byte
entscheidet später, welche Runtime ein Modul ausführt (nativ als WASM-Instanz,
über eine 68k-Emulator-Runtime, perspektivisch 6809). Ein statisches
Modul-Directory verwaltet Name→Header-Zuordnung, Link-Counting und die
Revisions-Regel bei Namenskollisionen. Konzeptgetreu zu OS-9, aber **bewusst
nicht binärkompatibel** (Entscheidung E2). Referenz: [`MODULES.md`](MODULES.md).

### 5.5 Dateisystem (VFS)

Ein Pfad wie `/d0/ordner/datei` wird am Gerätenamen getrennt; der Rest geht an
den **File-Manager** des Geräts — eine austauschbare Funktionstabelle
(`q9_fm_t`: open/create/read/write/seek/makdir/remove), absichtlich schmal
geschnitten, damit später auch ein Adapter für einen 68k-basierten Manager
andocken könnte. Aktuell einziger File-Manager: **FAT16** (lesend und
schreibend, inklusive langer Dateinamen beim Lesen) — gewählt wegen
Interoperabilität: ein Q9-Disk-Image lässt sich am Host-Rechner mounten,
befüllen und mit Standard-Tools inspizieren.

### 5.6 Prozessmodell (Entscheidung E8/E9, vollständig seit Phase 4.5: Fork/Wait/Chain, Blockieren, Suspend/Priorität, Signale)

WebAssembly erlaubt kein Umschalten von Aufruf-Stacks — ein klassischer,
Stack-wechselnder Scheduler scheidet daher aus. Q9s Scheduler verwaltet
stattdessen **Prozess-Zustände** (Active/Waiting/Sleeping) und ruft pro
Kernel-Tick den nächsten aktiven Prozess als **Step-Funktion** auf; Blockieren
bedeutet Zustandswechsel, nie eingefrorenen Stack. Interne (C-)Prozesse müssen
dafür kooperativ „in Häppchen" geschrieben sein; ein späterer 68k-Prozess
bringt seinen Kontext (CPU-Register + emulierten Stack) ohnehin selbst mit und
lässt sich trivial umschalten. Details und Begründung: PROJECT.md, Entscheidung
E8.

**Seit Phase 4.1** existiert das Fundament: `src/kernel/proc.c/.h` verwaltet
eine statische Prozess-Descriptor-Tabelle (PID, Parent, Modul, Zustand,
Exit-Code, eigene Std-Pfade 0/1/2 — kein `malloc`, analog zur Gerätetabelle)
und einen Round-Robin-Scheduler (`q9_proc_schedule()`, aufgerufen aus
`q9_kernel_step()`), der jeden Prozess im Zustand `ACTIVE` einmal pro Tick
als Step-Funktion aufruft. Die bisherige REPL (Phase 1) ist der erste
registrierte Prozess (PID 1).

**Seit Phase 4.2** gibt es echte Mehrprozess-Semantik: `F$Fork` startet einen
neuen Prozess aus einem Modul-Directory-Eintrag (Parent = Aufrufer, Std-Pfade
geerbt), `F$Exit` beendet den aufrufenden Prozess (Zombie, falls ein Parent
reapen kann — sonst sofortiges Freigeben), `F$Wait` sammelt einen beendeten
Kind-Prozess beim Parent ein, `F$Chain` ersetzt das eigene Modul (PID/Parent/
Std-Pfade bleiben). Da Q9 vor Phase 6 keine 68k/WASM-Ausführungs-Engine hat,
gilt **Entscheidung E9** (PROJECT.md): ein neues Language-Byte
`Q9_MOD_NATIVE` markiert Module, die statt echtem Byte-Code einen rohen
`q9_proc_step_fn`-Funktionszeiger direkt hinter dem Header tragen (gültig
nur innerhalb desselben laufenden Host-Prozesses) — F$Fork/F$Chain lesen
genau diesen Zeiger.

**Seit Phase 4.3** löst der Scheduler das E$NotRdy-Poll-Provisorium (I$Read/
I$ReadLn seit 1.2, F$Wait seit 4.2) durch echtes Blockieren ab: ein neuer
Weckgrund (`q9_wait_reason_t` — `Q9_WAIT_DEVICE`/`Q9_WAIT_CHILD`/
`Q9_WAIT_TIMER`) je Prozess-Deskriptor sagt dem Scheduler, worauf ein
`WAITING`/`SLEEPING`-Prozess wartet; `q9_proc_schedule()` prüft das VOR jedem
Stepp-Aufruf und weckt bei Erfüllung auf `ACTIVE`. `syscall.c` liefert an den
Aufrufer weiterhin sofort `E$NotRdy` zurück (kein eingefrorener Stack möglich,
Entscheidung E8 gilt unverändert) — geblockt wird nur die Scheduler-Sicht: ein
wartender Prozess wird schlicht nicht mehr bei jedem Tick sinnlos erneut
gestept. `Q9_WAIT_DEVICE` prüft `SS.Ready` des Geräts (z.B. `/term`-Eingabe),
`Q9_WAIT_CHILD` ein Zombie-Kind (F$Wait), `Q9_WAIT_TIMER` einen Tick-Zähler
(neuer Syscall **F$Sleep**: Ticks schlafen, 0 = einmal yielden).

**Seit Phase 4.4** kommen zwei weitere, unabhängige Zusatzfunktionen dazu: ein
vierter Weckgrund `Q9_WAIT_SIGNAL` für den neuen Syscall **F$SSpd**
(suspendiert eine beliebige, bekannte PID nach `WAITING`) — bewusst **ohne**
eigenen Weckcheck in `q9_proc_schedule()`, ein so suspendierter Prozess bleibt
also dauerhaft stehen, bis `F$Send` (Phase 4.5) `WAITING`/`SLEEPING`
unabhängig vom Weckgrund gewaltsam abbricht. Daneben **F$SPrior**: setzt ein
neues `priority`-Feld im Prozess-Deskriptor und liefert den alten Wert zurück
— reines Datenfeld, der Scheduler bleibt Round-Robin (Priorisierung/Aging
lohnt sich erst bei echter Konkurrenz um Rechenzeit).

**Seit Phase 4.5** ist das Prozessmodell mit Signalen komplett: **F$Icpt**
installiert einen Intercept-Handler (`q9_proc_step_fn`, wie eine normale
Step-Funktion) für den AUFRUFENDEN Prozess — anders als F$SSpd/F$SPrior wirkt
F$Icpt bewusst nur auf sich selbst. **F$Send** stellt einer beliebigen PID
ein Signal zu: steht sie in `WAITING`/`SLEEPING`, wird sie unabhängig vom
Weckgrund sofort `ACTIVE` ("Signal bricht Waiting/Sleeping ab"); ist
zusätzlich ein Intercept-Handler installiert, ruft der Scheduler ab dem
nächsten Tick diesen Handler statt der normalen Step-Funktion
(`q9_pd_t.in_intercept`) — der Handler liest die Signal-Nummer über
`q9_proc_current()->pending_signal`. **F$RTE** beendet den Intercept-Modus
wieder, danach steppt der Scheduler erneut die normale Step-Funktion. Damit
ist Phase 4 (Prozesse) vollständig: Fork/Exit/Wait/Chain (4.2), echtes
Blockieren (4.3), Suspend/Priorität (4.4), Signale (4.5).

### 5.7 HAL-Schnittstelle

```c
void          q9_hal_init(void);
void          q9_hal_con_put(char c);
int           q9_hal_con_get(void);              /* -1 = nichts da, nicht-blockierend */
uint32_t      q9_hal_ticks_ms(void);
int           q9_hal_blk_read (uint32_t lba, void *buf);        /* 512-Byte-Block */
int           q9_hal_blk_write(uint32_t lba, const void *buf);
int           q9_hal_time(q9_datetime_t *dt);
const char   *q9_hal_target(void);
```

Jedes Target implementiert genau diese Funktionen; alles Weitere (Geräte,
Pfade, Dateisystem, Module) ist reiner Kernel-Code und läuft überall gleich.

---

## 6. Stand der Dinge

Kompletter, feingranularer Stand mit Begründungen: [`../ARBEITSPLAN.md`](../ARBEITSPLAN.md)
(Statusmodell 💡/💤/🟢/🔄/✅/⛔). Kurzfassung nach Phase (Stand 2026-07-04):

| Phase | Inhalt | Stand |
|-------|--------|-------|
| 0 | Fundament: Toolchain, HAL-Grundgerüst, Kernel-Minimalgerüst, beide Erst-Targets bauen | ✅ fertig |
| 1 | Kernel-Basis: Syscall-Dispatcher, Device-/Pfadmodell, Namensauflösung, POSIX-HAL | ✅ fertig |
| 2 | Modulsystem: Header, CRC32, Directory, F$Link/F$UnLink | ✅ fertig |
| 3 | Dateisystem: Block-Device, VFS, FAT16 lesend/schreibend, F$Load, OPFS-Backend | ✅ fertig, live mit macOS-Tooling gegengetestet |
| 4 | Prozesse: Descriptor-Tabelle, Scheduler, F$Fork/Exit/Wait/Chain, Blockieren, Suspend/Priorität, Signale | ✅ fertig |
| 5 | Shell | offen |
| 6 | 68k-Runtime (Emulator im Browser) | offen |
| 7 | 68k nativ (Vinculum-Hardware) | offen |
| 8 | Vision: 6809-Runtime, Netzwerk, Self-Hosting | offen |

Getestet wird auf allen drei aktuellen Targets (Windows nativ, macOS/Linux
nativ, Browser/WASM inkl. OPFS) bei jedem Schritt — siehe `test/` und die
Verifikations-Notizen in ARBEITSPLAN.md.

---

## 7. Referenzquellen und ihre Lizenzen

Q9 ist eigenständig entwickelt, orientiert sich aber am Vorbild OS-9. Folgende
Quellen dienen als fachliche Referenz — mit unterschiedlicher Rechtslage:

| Quelle | Art | Lizenz | Verwendung in Q9 | Weitergebbar? |
|--------|-----|--------|-------------------|----------------|
| **MWOS SDK** (Microware OS-9, private Kopie) | Original-Header/Definitionen (`funcs.h`, `errno.h`, ...) | proprietär, privat | Quelle der Syscall-Nummern, Fehlercodes, Registerkonventionen (Entscheidung E7) | **Nein** — nur zum Abgleich verwendet, keine Datei daraus liegt im Repo |
| **OS-9 Insights** (Peter Dibble) | Fachbuch, ältere Auflage enthält abgedruckten FAT16-File-Manager | Buch-Copyright | Design-Referenz für den Manager-Schnitt (VFS/File-Manager-Aufteilung), Phase 3 | **Nein** — nur gelesen/verstanden, kein Code übernommen |
| **NitrOS-9** ([github.com/nitros9project/nitros9](https://github.com/nitros9project/nitros9)) | Community-OS-9/6809, RBF in 6809-Assembler | **GPL** | bislang nur als Idee vorgemerkt (Ideenspeicher, ARBEITSPLAN.md) — mögliche spätere Quelle für einen RBF-Manager auf einer 6809-Runtime (Phase 8) | Ja (GPL, Copyleft beachten) |
| **ToolShed** (Teil des NitrOS-9-Projekts) | PC-Tools zum Lesen/Schreiben von RBF-Images, in C | vermutlich GPL (im Kontext von NitrOS-9 zu prüfen) | bislang nur als Idee vorgemerkt: RBF-Strukturwissen für einen künftigen nativen Q9-RBF-Manager | zu prüfen |
| **OS9exec** (Lukas Zeller/Beat Forster) | 68k-Emulator + OS-9-Kernel-Nachbau in C, Syscall-Ebene | **GPL** | bislang nur als Idee vorgemerkt: Referenz für die TRAP→Syscall-Bridge (Phase 6) | Ja (GPL, Copyleft beachten) |

**Konsequenz für eine künftige Veröffentlichung:** Der Q9-Quellbaum selbst
enthält aktuell keine Zeilen aus einer der obigen Quellen — alles ist eigene
Implementierung nach eigenem Verständnis der Konzepte. Sollte künftig doch
GPL-Code aus NitrOS-9/OS9exec übernommen werden, muss die Lizenz von Q9
(oder zumindest der betroffenen Module) GPL-kompatibel sein. Die MWOS- und
Buch-Referenzen dürfen so oder so nie als Code auftauchen, nur als
Verständnisgrundlage dienen.

---

## 8. Glossar

Kurzreferenz für OS-9-Begriffe, die im Projekt und den Fachdokumenten
vorausgesetzt werden:

| Begriff | Bedeutung |
|---------|-----------|
| **Modul** | Grundbaustein von OS-9/Q9: Programm, Treiber, Dateisystem-Manager oder Datenblock, jeweils mit einheitlichem Header (Name, Typ, Revision, CRC) |
| **Language-Byte** | Feld im Modul-Header, das die Ausführungsform festlegt (bei Q9: WASM/68k/perspektivisch 6809, dazu `Q9_MOD_NATIVE` als Übergangslösung, s.u.) |
| **`Q9_MOD_NATIVE`** | Q9-eigenes Language-Byte (Entscheidung E9): das Modul trägt statt Byte-Code einen rohen Funktionszeiger direkt hinter dem Header — Übergangslösung, solange es vor Phase 6 keine 68k/WASM-Ausführungs-Engine gibt |
| **Zombie-Prozess** | Prozess, der sich per F$Exit beendet hat, aber noch in der Prozesstabelle steht, weil sein Parent den Exit-Code per F$Wait noch nicht abgeholt hat |
| **Weckgrund** (`q9_wait_reason_t`) | seit Phase 4.3: Grund, aus dem ein `WAITING`/`SLEEPING`-Prozess vom Scheduler übersprungen wird — Geräte-Bereitschaft (`Q9_WAIT_DEVICE`, SS.Ready), ein Zombie-Kind (`Q9_WAIT_CHILD`, F$Wait) oder ein Tick-Zähler-Zielwert (`Q9_WAIT_TIMER`, F$Sleep) |
| **IOMan** | OS-9s I/O-Manager — Vorbild für Q9s Geräte-/Pfadtabelle |
| **File-Manager** | Komponente, die die Semantik eines Dateisystemtyps kennt (z.B. RBF für OS-9-Disketten, FAT16 bei Q9) — sitzt zwischen Pfad und Block-Treiber |
| **RBF** | „Random Block File" — OS-9s natives Dateisystem/File-Manager |
| **SCF** | „Sequential Character File" — OS-9s File-Manager für zeichenorientierte Geräte (Konsole, seriell) |
| **PCF** | „Pipe Character File" bzw. (im FAT-Kontext) der PC-kompatible File-Manager in manchen OS-9-Varianten |
| **F$...** / **I$...** | OS-9-Syscall-Namenskonvention: `F$` = Systemfunktion (z.B. F$Fork), `I$` = I/O-Operation (z.B. I$Read) |
| **Descriptor** | OS-9-Begriff für eine Verwaltungsstruktur (Path Descriptor, Process Descriptor, Device Descriptor) |
| **Link-Count** | Zähler, wie oft ein Modul aktuell referenziert ist; bei 0 kann es aus dem Speicher entfernt werden |
| **Sticky-Modul** | Modul, das auch bei Link-Count 0 im Speicher bleibt (OS-9-Optimierung) |
| **Superfloppy** | Datenträger-Image ohne Partitionstabelle — Boot-Sektor direkt bei Block 0 |
| **OPFS** | Origin Private File System — der private, persistente Dateispeicher eines Browser-Origins, genutzt als Block-Device-Backend im WASM-Target |

---

**Erstellt**: 2026-07-04
**Letzte Aktualisierung**: 2026-07-04 (Initiale Version, nach Abschluss Phase 3)

#─────────────────────────────────────────────────────────────────────────────────────────────────
# EOF HANDBUCH.md                                                                          Ver. 1.00
#─────────────────────────────────────────────────────────────────────────────────────────────────
