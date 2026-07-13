#═════════════════════════════════════════════════════════════════════════════════════════════════
# File:   HANDBUCH.md                                                                     Ver. 1.92
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
# 26-07-04│ 1.10 │ 4.6: wasm3-Runtime (Entscheidung E10) — third_party/wasm3, wasmrt.c/.h, │ CF
#         │      │ neuer Abschnitt 5.8, Lizenz-/Referenzquellen-Tabelle ergänzt            │
# 26-07-04│ 1.20 │ 4.7: Syscall-Bridge (native Seite) — wasmproc.c/.h, Abschnitt 5.8        │ CF
#         │      │ erweitert, Phase-4-Status in Abschnitt 6 aktualisiert                   │
# 26-07-04│ 1.30 │ 4.8: Zeiger-/Speicher-Marshaling — Abschnitt 2.1 (wabt/wat2wasm neu     │ CF
#         │      │ installiert), Abschnitt 5.8 erweitert, Phase-4-Status in Abschnitt 6     │
# 26-07-04│ 1.40 │ 4.9: Fixed-Heap fuer wasm3 (config.h) — Abschnitt 3 (config.h), 5.8      │ CF
#         │      │ erweitert, Phase-4-Status in Abschnitt 6 (Anschluss 4.6-4.9 komplett)     │
# 26-07-04│ 1.50 │ 5.1: Musashi-68k-Emulation (Entscheidung E12) — third_party/musashi/,    │ CF
#         │      │ m68krt.c/.h, neuer Abschnitt 5.9, Abschnitt 3/6/7 aktualisiert            │
# 26-07-04│ 1.60 │ 5.2a: CB030-Board-Speicherlogik (RAM/ROM/Remap) — cb030.c/.h, neuer       │ CF
#         │      │ Abschnitt 5.10, Abschnitt 3/6 aktualisiert                                │
# 26-07-04│ 1.70 │ 5.2b-d: DUART/Compact-Flash/Timer-IRQ3 — cb030.c/.h + m68krt.c/.h         │ CF
#         │      │ (q9_m68krt_set_irq) erweitert, Abschnitt 5.10/6 aktualisiert. Phase 5.2   │
#         │      │ damit komplett (5.2a-d alle fertig)                                       │
# 26-07-05│ 1.80 │ 5.3: Musashi-CB030-Verdrahtung (q9_m68krt_attach_board), ROM-Laden         │ CF
#         │      │ (q9_cb030_rom_load), Boot-Runner cb030run.c/.h (q9.exe --cb030 <rom>),     │
#         │      │ Spiegelgrenzen-Korrektur (bis 0xFEFF_FFFF, I/O vor Remap erreichbar)       │
# 26-07-07│ 1.90 │ OS9SYS-CF-Boot, os9gen /c0_fmt, Windows-Terminal-Keyfix und Toolshed/WSL    │ CF
#         │      │ Arbeitsregeln dokumentiert; alten 5.2a-Spiegelgrenzen-Satz korrigiert       │
# 26-07-10│ 1.91 │ 5.7: TX-Ringpuffer (hal_posix.c) — HAL-Schnittstelle (Abschnitt 5.7) um     │ CF
#         │      │ q9_hal_con_flush/tx_ready/tx_empty erweitert, veraltete "TxRDY immer        │
#         │      │ gesetzt"-Aussage in Abschnitt 5.10 (5.2b) korrigiert                        │
# 26-07-10│ 1.92 │ 5.9: Idle-Drossel CB030-Runner — q9_hal_sleep_ms (Abschnitt 5.7),           │ CF
#         │      │ q9_m68krt_is_stopped (Abschnitt 5.9), Boot-Runner-Beschreibung in           │
#         │      │ Abschnitt 5.10 aktualisiert (Ctrl-] statt Ctrl-C, Idle-Drossel-Absatz)       │
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
- [`../PROJECT.md`](../PROJECT.md) — Vision, Entscheidungshistorie (E1–E12), offene Fragen (O1–O6)
- [`../ARBEITSPLAN.md`](../ARBEITSPLAN.md) — laufender Arbeitsstand, Schritt für Schritt (internes Arbeitsdokument, siehe unten)
- [`SYSCALLS.md`](SYSCALLS.md) — vollständige Syscall-ABI (Register, Fehlercodes)
- [`SYSCALL_ROADMAP.md`](SYSCALL_ROADMAP.md) — alle 97 OS-9-Syscalls mit Q9-Status
- [`DEVICES.md`](DEVICES.md) — Geräte-/Pfadtabelle, Treiber-Schnittstelle
- [`MODULES.md`](MODULES.md) — OS-9-Modulsystem als Referenz für Q9s eigenes
- [`TOOLCHAIN.md`](TOOLCHAIN.md) — Toolchain-Stand je Entwicklungsrechner (Versionen, Pfade)
- [`AUTONOMIE.md`](AUTONOMIE.md) — Setup für den automatisierten Arbeitsmodus (projektintern, für Aussenstehende irrelevant)
- [`CB030.md`](CB030.md) — Hardware-Referenz (Speicherkarte, DUART/CF-Register) für die Musashi-Board-Emulation (Phase 5.2)
- [`OS9SYS_BOOT.md`](OS9SYS_BOOT.md) — lokales Runbook fuer OS9SYS-CF-Boot, `os9gen /c0_fmt`, Toolshed/WSL und Terminal-Keyfix

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
ist Übernehmen rechtlich zulässig, wird aber aktuell ebenfalls nicht praktiziert;
sollte sich das ändern, zieht die GPL automatisch nach (Copyleft) — das muss bei
der Lizenzentscheidung für Q9 mitgedacht werden. Einzige tatsächlich vendorte
Fremdquelle ist bislang **wasm3** (`third_party/wasm3/`, MIT-lizenziert, seit
Phase 4.6) — MIT ist unproblematisch mit praktisch jeder Zielrezenz kombinierbar,
eigene LICENSE-Datei liegt bei.

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

Optional, nur für die Arbeit an `Q9_MOD_WASM`-Gastprogrammen (Selbsttests ab
Schritt 4.8, `src/kernel/kernel.c`): **wabt** (Google, Apache-2.0, enthält
`wat2wasm`) übersetzt lesbaren WAT-Quelltext in `.wasm`-Bytecode, statt ihn
von Hand als Byte-Array zusammenzusetzen. Installation z.B. per Homebrew
(`brew install wabt`) oder als Release-ZIP von der
[wabt-Releaseseite](https://github.com/WebAssembly/wabt); wird nicht vom
Kernel-Build selbst gebraucht, nur beim Erzeugen neuer Testmodule.

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
├── PROJECT.md            Vision, Architektur-Entscheidungen (E1-E12), offene Fragen (O1-O6)
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
│   │   ├── proc.c/.h          Prozess-Descriptor-Tabelle + Round-Robin-Scheduler (Phase 4)
│   │   ├── wasmrt.c/.h        Wrapper um die eingebettete wasm3-Runtime (Phase 4.6,
│   │   │                       NUR im nativen Build, s. Abschnitt 5.8) — kapselt wasm3.h nach
│   │   │                       aussen, damit kein Aufrufer third_party/wasm3 einbinden muss
│   │   ├── wasmproc.c/.h      Syscall-Bridge fuer Q9_MOD_WASM-Prozesse (Phase 4.7/4.8, NUR im
│   │   │                       nativen Build) — Importe q9.f_id/f_time/f_exit (4.7) sowie
│   │   │                       q9.i_open/i_close/i_read/i_write (4.8, Zeiger-/Speicher-Marshaling
│   │   │                       ueber Gast-Offsets) + Step-Trampolin q9_wasm_proc_step, das
│   │   │                       F$Fork/F$Chain fuer WASM-Module eintragen
│   │   ├── config.h           Erster Baustein einer Q9-Systemkonfiguration (Phase 4.9) — bisher
│   │   │                       nur Q9_SYSTEM_MEM_BYTES (Fixed-Heap-Groesse fuer wasm3, s. Abschnitt
│   │   │                       5.8), wird NUR ins native-Target eingebunden (-include, Makefile)
│   │   ├── m68krt.c/.h        Wrapper um die eingebettete Musashi-68k-Emulation (Phase 5.1/5.3,
│   │   │                       NUR im nativen Build, s. Abschnitt 5.9) — definiert die von
│   │   │                       Musashi verlangten m68k_read/write_memory_*-Funktionen: wahlweise
│   │   │                       gegen einen nackten RAM-Block (5.1) oder via
│   │   │                       q9_m68krt_attach_board ueber den CB030-Adress-Dispatch (5.3);
│   │   │                       noch OHNE Scheduler-/Syscall-Bridge (kommt mit der Detailplanung
│   │   │                       von Phase 5)
│   │   ├── cb030.c/.h         CB030-Board-Emulation (Schritte 5.2a-d + 5.3, NUR im nativen
│   │   │                       Build, s. Abschnitt 5.10) — RAM/ROM/Remap-Adress-Dekoder,
│   │   │                       68681-DUART, Compact-Flash (ATA-PIO), Timer/IRQ3 (kooperativ)
│   │   │                       und ROM-Datei-Lader (q9_cb030_rom_load), alles als eigenes
│   │   │                       Handle (q9_cb030_t) ohne Musashi-Abhaengigkeit
│   │   ├── quicc.c/.h         QUICC-Ethernet-Emulation (Schritt 5.11, NUR im nativen Build,
│   │   │                       s. Abschnitt 5.11) — MC68360-SCC1 im Ethernet-Modus als 8K-Fenster
│   │   │                       $FFFF2000-$FFFF3FFF (DPRAM, SCC1-Parameter-RAM, Registerbank),
│   │   │                       TX-/RX-Buffer-Descriptor-Ringe, IRQ Level 5/Vektor 254 und ein
│   │   │                       User-Mode-Mini-NAT-Backend (ARP/ICMP als Gegenstelle 192.168.200.1);
│   │   │                       Gegenstueck zum originalen Microware-SPF-Treiber sp360 im
│   │   │                       MWOS-Q9-Port
│   │   └── cb030run.c/.h      CB030-Boot-Runner (Schritte 5.3/5.5a, NUR im nativen Build) —
│   │                           Einstiegspunkt fuer `q9.exe --cb030 <rom-datei> [--cf <image>]`:
│   │                           ROM laden, Board+Musashi verdrahten, CPU-Endlosschleife mit
│   │                           Timer-Polling (das echte Microware-ROM bleibt lokal, NIE im
│   │                           Repository); `--cf` waehlt optional ein eigenes CF-Backing-Image
│   │                           statt des Default `cb030_cf.img`
│   └── hal/               Hardware Abstraction Layer — hier UND NUR hier ist Code Target-spezifisch
│       ├── q9_hal.h           die schmale Schnittstelle, die jedes Target erfüllen muss
│       ├── native/            Windows-HAL (conio.h) — Host-Loop (main) liegt hier
│       ├── posix/             macOS/Linux-HAL (termios) — Host-Loop liegt hier
│       └── wasm/              Browser-HAL (Emscripten) — kein main(), stattdessen
│                               q9_kernel_init/q9_kernel_step als exportierte Funktionen
├── third_party/
│   ├── wasm3/              vendorte WASM-Interpreter-Bibliothek (MIT, unverändert), nur im
│   │                        nativen Build gelinkt — siehe third_party/wasm3/README.md und
│   │                        Abschnitt 7
│   └── musashi/            vendorte 68k-CPU-Emulation (MIT, unverändert, Entscheidung E12),
│                            nur im nativen Build gelinkt — Zweistufen-Build (m68kmake generiert
│                            m68kops.c/.h aus m68k_in.c zur Bauzeit) — siehe
│                            third_party/musashi/Q9_VENDOR.md und Abschnitt 7
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
  auskommen soll. Einzige, bewusst eng begrenzte Ausnahme: die vendorte
  wasm3-Bibliothek (`third_party/wasm3/`, Entscheidung E10 in PROJECT.md)
  nutzt intern `malloc`/`free` — das betrifft nur diesen Fremdcode selbst,
  nicht `wasmrt.c` oder den restlichen Kernel.
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

Windows PowerShell auf dem lokalen AF-PC:

```powershell
cd D:\projekts\Q9
$env:PATH = "C:\Users\AF\w64devkit\bin;$env:PATH"
$env:OS = "Windows_NT"
make native
.\build\native\q9.exe --selftest
```

Auf dem AF-PC zeigt `python3.exe` auf den Microsoft-Store-Alias. Fuer die
Tests daher explizit das echte Python verwenden:

```powershell
make test PYTHON=python
```

CB030/OS-9-Arbeitsimage unter Windows:

```powershell
cd D:\projekts\Q9
.\build\native\q9.exe --cb030 .\local_images\roms\romimage.dev.running.BIN --cf .\local_images\OS9SYS.hda
```

Vor Neubuilds oder Image-Arbeiten pruefen, ob noch alte Emulatorprozesse laufen:

```powershell
Get-CimInstance Win32_Process |
  Where-Object { $_.ExecutablePath -like 'D:\projekts\Q9\build\native\q9*.exe' } |
  Select-Object ProcessId,Name,ExecutablePath,CommandLine
```

Fenster/Tab schließen beendet `q9*.exe` nicht immer zuverlaessig; ein alter
Prozess kann die EXE sperren oder zu falschen Testergebnissen fuehren.

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
void          q9_hal_con_flush(void);            /* 5.7: TX-Puffer-Rest nachliefern (Haupt-Loop) */
int           q9_hal_con_tx_ready(void);          /* 5.7: Platz fuer mind. 1 weiteres Byte (TxRDY) */
int           q9_hal_con_tx_empty(void);          /* 5.7: Puffer vollstaendig geleert (TxEMT)      */
uint32_t      q9_hal_ticks_ms(void);
int           q9_hal_blk_read (uint32_t lba, void *buf);        /* 512-Byte-Block */
int           q9_hal_blk_write(uint32_t lba, const void *buf);
int           q9_hal_time(q9_datetime_t *dt);
const char   *q9_hal_target(void);
```

Jedes Target implementiert genau diese Funktionen; alles Weitere (Geräte,
Pfade, Dateisystem, Module) ist reiner Kernel-Code und läuft überall gleich.
`q9_hal_con_flush/tx_ready/tx_empty` sind nur auf POSIX ein echter TX-
Ringpuffer (5.7, `hal_posix.c`) — native (Windows) und wasm bleiben synchron
und liefern trivial "immer leer/bereit".

### 5.8 WASM-Runtime + Syscall-Bridge (native Build, Phase 4.6-4.9, Entscheidung E10)

Damit `Q9_MOD_WASM`-Module (s. Abschnitt 5.4) nicht nur im Browser laufen
(dort instanziiert JS sie direkt über `WebAssembly.instantiate`), braucht der
native PC-Build einen eigenen, eingebetteten WASM-Interpreter — sonst wäre
die native Version nur zu Debug-Zwecken benutzbar (löst O5 in PROJECT.md).
Kandidat und Wahl: **wasm3** (MIT, klein, embeddable, kein WASI/Betriebs-
systembezug nötig) — vendored, unverändert, unter `third_party/wasm3/`.

`src/kernel/wasmrt.c/.h` ist die schmale, generische Q9-Schnittstelle über
wasm3 (Parsen/Laden/Aufrufen/Freigeben), `src/kernel/wasmproc.c/.h` (4.7)
baut darauf die eigentliche Prozess-Integration:

```c
// wasmrt.h — generischer Wrapper
int  q9_wasmrt_init(q9_wasmrt_t *rt);
int  q9_wasmrt_parse(q9_wasmrt_t *rt, const uint8_t *bytes, uint32_t len);
int  q9_wasmrt_load(q9_wasmrt_t *rt);
int  q9_wasmrt_call_i32(q9_wasmrt_t *rt, const char *funcname,
                         int32_t a, int32_t b, int32_t *out);
const char *q9_wasmrt_call_raw(q9_wasmrt_t *rt, const char *funcname);
void q9_wasmrt_free(q9_wasmrt_t *rt);

// wasmproc.h — Q9-Prozessintegration (4.7)
void q9_wasm_proc_step(void);   // Step-Funktion fuer Q9_MOD_WASM-Prozesse
```

**4.6** (Grundbaustein): ein `.wasm`-Modul laden und eine exportierte
Funktion mit zwei `i32`-Parametern aufrufen, noch ohne Syscall-Bridge.
Bewiesen durch einen Selbsttest, der ein handgebautes `add(a,b)`-Modul lädt
und rechnet.

**4.7** (Syscall-Bridge): `entry_step_for()` in `syscall.c` lässt `F$Fork`/
`F$Chain` jetzt auch Module mit Language-Byte `Q9_MOD_WASM` starten (analog
zu `Q9_MOD_NATIVE`/Entscheidung E9) — Step-Funktion ist immer die feste
`q9_wasm_proc_step()`, die ihr Modul selbst aus der Prozesstabelle liest.
Drei Importe im wasm3-Namespace `"q9"` verdrahten echte Q9-Syscalls: `f_id`
(`i()`), `f_time` (`I()`, packt d0/d1 in ein i64 — WASM-MVP kennt keine
Mehrfachrückgabe), `f_exit` (`v(i)`, ruft `F$Exit` auf und bricht die
WASM-Ausführung per Trap ab, kein Rückkehrpfad in den Gastcode). Bewusste
Vereinfachung: das Gastprogramm läuft beim ersten Scheduler-Tick synchron
bis zum Ende durch (kein kooperatives Unterbrechen mitten in der
Ausführung — bräuchte Asyncify o.ä., außerhalb des 4.7-Rahmens). Modul-
Konvention wie `Q9_MOD_NATIVE`: der WASM-Bytecode steht direkt hinter dem
Header (`execoff`/`datasize`). Bewiesen durch einen Selbsttest mit echtem
`F$Fork`/`F$Wait`-Lebenszyklus. **Nicht Teil von 4.7**: die Browser-Seite
(`WebAssembly.instantiate` im Worker für ein verschachteltes Gastmodul) —
zurückgestellt, s. ARBEITSPLAN.md „Geparkt".

**4.8** (Zeiger-/Speicher-Marshaling): vier weitere Importe im Namespace
`"q9"` — `i_open` (`i(ii)`), `i_close` (`i(i)`), `i_read`/`i_write`
(`i(iii)`) —, die jetzt auch Zeiger-Parameter über die Modulgrenze
transportieren. Ein Gastprogramm kennt keine Host-Adressen, sondern nur
Offsets in seine **eigene** lineare Speicherinstanz; wasm3 stellt dafür die
Makros `m3ApiGetArgMem`/`m3ApiOffsetToPtr` bereit, die einen Gast-Offset in
einen echten Host-Zeiger relativ zur Speicherbasis (`_mem`) übersetzen.
Jeder Import bounds-checkt den übersetzten Bereich selbst (eigene Funktion
`wasm_range_ok()` in `wasmproc.c`, dieselbe Grenzformel wie wasm3s
`m3ApiCheckMem`-Makro, aber ohne dessen impliziten `return` — ein Makro mit
eingebautem `return` lässt sich nicht in einem gemeinsamen Helfer für
mehrere Importe wiederverwenden). Liegt der Bereich außerhalb der
Speicherinstanz, bricht der Import per Trap ab (`m3Err_trapOutOfBoundsMemoryAccess`)
— ein Programmierfehler im Gast, kein regulärer I/O-Fehler. Pfadnamen werden
zusätzlich Byte für Byte auf ein abschließendes NUL abgesucht (`wasm_str_len()`,
Obergrenze `Q9_WASM_PATH_MAX` = 128 Byte), damit die Suche nicht unbegrenzt
über fremden Speicher läuft. Rückgabekonvention aller vier neuen Importe:
`>= 0` ist der Erfolgswert (Pfadnummer bzw. übertragene Bytes), `< 0` ein
negierter Q9-Fehlercode — WASM kennt kein Carry-Bit wie das reale 68k-ABI
(docs/SYSCALLS.md), diese Bridge-Konvention ersetzt es für Gastcode. Bewiesen
durch einen Selbsttest, dessen Gastprogramm `/nil` öffnet, hineinschreibt,
daraus liest (immer `E$EOF`) und wieder schließt — vollständig über
Gast-Offsets, kein Host-Zeiger wird dem Gastcode je direkt sichtbar gemacht.
Nebenbefund beim Bau des Testmoduls: WASM-Adresse `0` gilt in wasm3 als
Nullzeiger (`m3ApiIsNullPtr`), analog zur Konvention realer Linker, die die
Nullseite reservieren — das Testmodul platziert seine Daten deshalb bewusst
ab Offset 8, nicht ab 0.

Das `.wasm`-Testmodul für 4.8 ist wie schon bei 4.6/4.7 von Hand als
Bytecode eingebettet (`kernel.c`), diesmal aber aus echtem WAT-Quelltext
gebaut statt komplett per Hand assembliert — **wat2wasm** (Teil von
**wabt**, Google, Apache-2.0) ist seit 4.8 auf diesem Mac Mini per Homebrew
installiert (s. Abschnitt 2).

`wasmrt.c`/`wasmproc.c` selbst bleiben warnungsfrei und ohne eigenes
`malloc` — bis Schritt 4.9 steckte die Heap-Nutzung vollständig in wasm3
selbst (via Host-`malloc`/`calloc`/`realloc`, bewusste, eng begrenzte
Ausnahme von "Kein malloc im Kernel", s. Abschnitt 3.1 und Entscheidung
E10). Nur Teil von `make native`; `make wasm` bindet weder diese Dateien
noch `third_party/wasm3/` ein (per `#ifdef Q9_HAVE_WASM3` in
`kernel.c`/`syscall.c` und separater Makefile-Quellliste).

**4.9** (Fixed-Heap statt Host-`malloc`): wasm3 bringt für genau diesen
Fall bereits einen fertigen Baustein mit — `d_m3FixedHeap`
(`third_party/wasm3/m3_config.h`), ein Compile-Define, das `m3_Malloc_Impl`/
`m3_Free_Impl`/`m3_Realloc_Impl` (`m3_core.c`) von echtem Host-`malloc` auf
ein **statisches Array + Bump-Allocator** umschaltet — nur bislang nicht
aktiviert. Neue Konfigurationsstelle **`src/kernel/config.h`**: erster
Baustein einer Q9-Systemkonfiguration, bewusst nur EIN Wert
(`Q9_SYSTEM_MEM_BYTES`, Gesamtspeicher des simulierten/emulierten
Zielsystems — Grundstein für spätere Werte wie CPU-Takt, die NICHT jetzt
schon dazukommen). Das Makefile leitet `d_m3FixedHeap` per
`-Dd_m3FixedHeap=Q9_SYSTEM_MEM_BYTES` (kombiniert mit `-include
src/kernel/config.h`, nur in `WASM3_CFLAGS`) aus genau diesem einen Wert ab
— der vendorte Fremdcode selbst bleibt dabei unverändert, weil
`d_m3FixedHeap` in `m3_config.h` per `#ifndef` geschützt ist und das
Kommandozeilen-Define gewinnt. **Wichtige Eigenschaft des Bump-Allocators**:
er kann nur den jeweils ZULETZT allozierten Block wirklich freigeben
(`m3_Free_Impl`) — unabhängig voneinander erzeugte `q9_wasmrt_t`-Instanzen
(wie in den Selbsttests 4.6/4.7/4.8, die im selben Prozesslauf
nacheinander laufen) geben ihren Speicher deshalb nicht zuverlässig
vollständig frei; der Heap wächst effektiv über die Lebensdauer des
GESAMTEN Prozesses. `Q9_SYSTEM_MEM_BYTES` = 256 KiB berücksichtigt das
(128 KiB liefen im Selbsttest über). Verifiziert über `nm` auf die
kompilierten `wasm3_*.o`: keine undefinierten Symbole
`malloc`/`calloc`/`realloc`/`free` mehr.

### 5.9 Musashi-68k-Emulation (native Build, Phase 5.1, Entscheidung E12)

Phase 5 bringt echten 68k-Maschinencode nach Q9 (Ziel: per **vbcc** compilierte
Programme laufen als eigener Prozesstyp, analog zu `Q9_MOD_NATIVE`/
`Q9_MOD_WASM`). Damit das auch im nativen PC-Build funktioniert (nicht nur auf
echter 68k-Hardware), braucht Q9 einen eingebetteten 68k-Interpreter — Wahl:
**Musashi** (MIT, Karl Stenerud), vendored unter `third_party/musashi/` (E12).
Ziel-CPU-Typ ist **68030** (nächstliegender, gut unterstützter Musashi-Typ zur
realen Zielhardware MC68EN360/QUICC mit CPU32+-Kern, den Musashi nicht kennt) —
MMU bleibt ungenutzt, damit der emulierte 68k-Code nicht versehentlich von
68030-Exklusivfeatures abhängt, die auf CPU32-Hardware fehlen würden.

Musashi hat einen **Zweistufen-Build**, anders als wasm3: das Host-Tool
`m68kmake` liest `third_party/musashi/m68k_in.c` (518 handgeschriebene
Opcode-Primitive) und generiert daraus `m68kops.c/.h` (1967 Opcode-Handler) —
reine Build-Artefakte, landen zur Bauzeit unter `build/native/musashi_gen/`
und werden nicht versioniert (analog zu den `wasm3_*.o` aus Abschnitt 5.8).
Musashis eigener Kern-Interpreter (`m68kcpu.c`, das intern bereits
`m68kfpu.c` per `#include` einbindet — `m68kfpu.c` darf deshalb NICHT
zusätzlich separat übersetzt werden, sonst doppelte Symbole beim Linken) +
der Softfloat-Unterbau (`softfloat/softfloat.c`) werden mit eigenen, laxeren
Flags übersetzt (unveränderter Fremdcode, analog `WASM3_CFLAGS`).

`src/kernel/m68krt.c/.h` ist der schmale Q9-Wrapper. Anders als wasm3 (das
über `IM3Environment`/`IM3Runtime`-Handles mehrere unabhängige Instanzen
erlaubt) hält Musashi seinen kompletten CPU-Zustand in eigenen globalen
Variablen — die von Musashi verlangten Speicherzugriffsfunktionen
(`m68k_read/write_memory_8/16/32`) bekommen keinen Kontext-Zeiger übergeben.
`q9_m68krt_t` ist deshalb bewusst kein mehrfach instanzierbares Handle wie
`q9_wasmrt_t`, sondern nur eine dünne Buchhaltungs-Struktur neben einem
globalen RAM-Zeiger in `m68krt.c` — es kann je Prozesslauf immer nur EINE
Musashi-Instanz aktiv sein.

```c
// m68krt.h — Grundbaustein (5.1)
int      q9_m68krt_init(q9_m68krt_t *rt, uint8_t *ram, uint32_t ram_len);
void     q9_m68krt_reset(q9_m68krt_t *rt);
int      q9_m68krt_execute(q9_m68krt_t *rt, int cycles);
uint32_t q9_m68krt_get_d(q9_m68krt_t *rt, int n);   // Dn, n=0..7
void     q9_m68krt_free(q9_m68krt_t *rt);
int      q9_m68krt_is_stopped(void);                // 5.9: CPU per STOP angehalten?
```

Mit `M68K_SEPARATE_READS` aus (third_party/musashi/m68kconf.h) genügen genau
diese sechs Speicherfunktionen — Musashis interne
`m68k_read_immediate_*`/`m68k_read_pcrelative_*` fallen ohnehin auf dieselben
sechs zurück (s. `m68kcpu.h`).

**5.1** (Grundbaustein, noch OHNE Scheduler-/Syscall-Bridge — die kommt erst
mit der Detailplanung von Phase 5): Makefile-Integration des Zweistufen-Builds
+ Wrapper. Bewiesen durch einen Selbsttest (`-DQ9_HAVE_M68K`, native-only, wie
`Q9_HAVE_WASM3` in Abschnitt 5.8): ein von Hand assembliertes Programm
(`MOVEQ #2,D0`; `ADDI.W #3,D0`; `BRA.S *-2` als Endlosschleife, damit
`m68k_execute()` nicht in unbeschriebenen Speicher läuft) liegt zusammen mit
den Reset-Vektoren (SP bei Adresse 0, PC bei Adresse 4, big-endian) in einem
emulierten RAM-Block; nach `q9_m68krt_reset()` + `q9_m68krt_execute()` steht
`D0 == 5`.

### 5.10 CB030-Board-Emulation (Schritt 5.2, `src/kernel/cb030.c/.h`)

Das **CB030-Board** dient als Bootstrap/Validierungs-Zwischenschritt für die
Musashi-Integration — mit dem originalen, proprietären Microware-OS-9-Boot-ROM
statt nur mit handassemblierten Testprogrammen (Andreas' Vorschlag,
2026-07-04 abends). Ändert nichts an der eigentlichen Q9-Zielhardware
(MC68EN360/QUICC bleibt Ziel für Phase 7, s. PROJECT.md O4). Speicherkarte +
Peripherie-Register: [`docs/CB030.md`](CB030.md). Das reale Boot-ROM bleibt
wie die MWOS-SDK-Kopie proprietär und NICHT im Repository — nur die
Hardware-Dokumentation selbst und der Emulationscode sind es.

**5.2a** (RAM/ROM/Remap-Speicherlogik, erster Baustein): reiner
Adress-Dekoder als if/else-Kette (RAM zuerst geprüft), unabhängig von
Musashis eigenem CPU-Zustand — der REMAP-Merker sitzt in einem eigenen
`q9_cb030_t`-Handle:

```c
// cb030.h — Adress-Dispatch (5.2a)
int      q9_cb030_init(q9_cb030_t *b, const uint8_t *rom, uint32_t rom_len,
                        uint8_t *ram, uint32_t ram_len);
void     q9_cb030_reset(q9_cb030_t *b);
uint8_t  q9_cb030_read8(q9_cb030_t *b, uint32_t addr);   // + read16/read32
void     q9_cb030_write8(q9_cb030_t *b, uint32_t addr, uint8_t val);  // + write16/write32
```

Reset-Zustand: ROM bei Adresse 0, gespiegelt bis `0xFEFF_FFFF`. Ein einzelner
Buszugriff (lesend oder schreibend) auf den REMAP-Registerbereich
(`0xFFFF_8000`–`0xFFFF_8FFF`) schaltet dauerhaft um — reiner Adress-Trigger,
kein Datenwert; danach liegt RAM ab Adresse 0 und das ROM nur noch einmal,
unmirrored, bei `0xFE00_0000`–`0xFE07_FFFF` (read-only). Peripherie (UART/
CF/Timer, Schritte 5.2b–d) ist hier bewusst noch NICHT angebunden — der
I/O-Adressbereich liefert vorerst 0 bzw. verwirft Schreibzugriffe
kommentarlos. Ebenso offen: die eigentliche Verdrahtung an Musashis
`m68k_read/write_memory_*`-Hooks (`m68krt.c`, Abschnitt 5.9) — dieser
Schritt liefert nur den reinen Adress-Dekoder samt Selbsttest (`-DQ9_HAVE_M68K`,
native-only, synthetisches 4-Byte-Test-ROM statt des echten Boot-ROMs).

**5.2b** (68681-DUART): Minimalansatz — nur SRA (Status, `UART_BASE+0x02`)
und THRA/RHRA (Zeichenpuffer, `UART_BASE+0x06`) sind wirklich aktiv, der
restliche Registersatz wird sauber angenommen (liest 0, verwirft
Schreibzugriffe). SRA-Bits: RxRDY (`0x01`, gesetzt wenn ein Zeichen im
Empfangs-FIFO wartet) sowie **seit 5.7** TxRDY (`0x04`) und TxEMT (`0x08`),
die den Füllstand des HAL-seitigen TX-Ringpuffers ehrlich widerspiegeln
(`q9_hal_con_tx_ready`/`q9_hal_con_tx_empty`, Abschnitt 5.7) statt wie zu
Beginn immer "sofort bereit" zu melden. THRA-Schreibzugriff → `q9_hal_con_put`.

**5.2c** (Compact-Flash): ATA-PIO-Minimalprotokoll — Register `CF_BASE+0`
(Data, 1 Byte/Zugriff), `+2` (Sectcount), `+3..+5` (LBA0-2), `+7`
(Kommando/Status). Unterstützte Kommandos: READ SECTOR(S) (`0x20`), WRITE
SECTOR(S) (`0x30`), Statusbits BSY/DRQ/RDY/ERR wie in `docs/CB030.md`
festgelegt. Backing Store ist eine lazy geöffnete Host-Datei
(`q9_cb030_cf_attach(board, path)`, Muster wie `q9disk.img`, Abschnitt 3.1) —
für den Selbsttest `cb030_cf_test.img` (neu in `.gitignore`, wird vom
`make test`-Target vorher gelöscht).

**5.2d** (Timer/IRQ3): kooperative Umsetzung statt echtem
Host-Timerinterrupt (ein Signal-Handler oder separater Thread wäre nicht
threadsicher gegen Musashis globalen, nicht-reentranten Zustand und würde
die kooperative Grundausrichtung aus Entscheidung E8 durchbrechen).
`q9_cb030_poll_timer(board, now_ms)` prüft, ob der Timer per
`TI_IRQ_ON`/`TI_IRQ_OFF` (reine Adress-Trigger, `0xFFFF_9000`-`0xFFFF_9FFF`)
aktiv ist und seit dem letzten Auslösen ≥10ms (100 Hz) Host-Zeit
(`q9_hal_ticks_ms()`) vergangen sind — der Aufrufer muss dann selbst
`q9_m68krt_set_irq(3)` aufrufen. `cb030.c` kennt Musashi bewusst nicht;
`q9_m68krt_set_irq()` ist ein neuer schmaler Wrapper in `m68krt.h/.c` um
`m68k_set_irq()` — Musashi erledigt die eigentliche Interrupt-Mechanik
(Stack/Vektorsprung) vollständig selbst.

Damit ist Phase 5.2 (CB030-Board-Emulation) komplett: 5.2a–d alle ✅.

**5.3** (Musashi ↔ CB030 verdrahten + Boot-Runner): `q9_m68krt_attach_board(&board)`
schaltet die sechs Musashi-Speicher-Hooks vom nackten RAM-Block (5.1) auf den
CB030-Adress-Dispatch um — ab dann laufen ALLE CPU-Zugriffe (inkl. der
Reset-Vektoren) über `q9_cb030_read/write8/16/32`. Der Interrupt-Acknowledge
läuft im Board-Betrieb als Autovector mit Puls-Verhalten (die IRQ-Leitung wird
beim Annehmen losgelassen, sonst würde der level-gehaltene IRQ3 endlos erneut
unterbrechen). Zwei Korrekturen am 5.2a-Dekoder, ohne die das echte Boot-ROM
nicht gebootet hätte: die ROM-Spiegelung im Reset-Zustand reicht bis
`0xFEFF_FFFF` (nicht `0x0800_0000` — das ROM springt vor dem REMAP-Trigger
hoch nach `0xFE00_xxxx`), und die I/O-Region ist in beiden Zuständen
erreichbar (die DUART wird vor dem Remap initialisiert). Der Boot-Runner
(`cb030run.c`) macht daraus ein Kommando:

```
./build/native/q9.exe --cb030 <pfad-zum-rom-image> [--cf <pfad-zum-cf-image>]
```

Lädt das ROM (max. 512 KByte, `q9_cb030_rom_load`), stellt 16 MByte
emuliertes RAM, hängt die CF-Backing-Datei an (lazy angelegt — standardmäßig
`cb030_cf.img`, mit `--cf` ein beliebiger anderer Pfad, 5.5a) und lässt die
CPU laufen (Ende: Ctrl-] als Host-Escape, s. `q9_hal_con_get`). **Idle-Drossel
(5.9):** Ist die CPU per `STOP` angehalten (`q9_m68krt_is_stopped`, z.B. OS-9s
Leerlauf am Login-Prompt) UND liegt kein IRQ an, schläft der Host-Loop kurz
(`q9_hal_sleep_ms(1)`) statt den nächsten Slice sofort "leer" zu verbrennen —
senkt die Host-CPU-Last im Leerlauf von ~100 % auf ca. 1–2 %, ohne die
OS-9-Uhr zu verfälschen (`q9_hal_ticks_ms()` bleibt Wanduhr-basiert, der
nächste 10-ms-Timer-Tick weckt die CPU regulär). Das echte Microware-Boot-ROM ist proprietär und
bleibt lokal — `.gitignore` deckt `cb030rom*.bin`/`*.rom` ab. Der Selbsttest
bootet stattdessen ein synthetisches 32-Byte-ROM über exakt dasselbe
Bootmuster (Vektoren aus dem ROM, Sprung hoch, REMAP, RAM-Schreiben). Die
CF-Emulation (`cb030_cf_read/write`) zählt READ/WRITE SECTOR(S) seit 5.5a
echt über mehrere Sektoren durch (Sector-Count 0 = 256 Sektoren, ATA-
Konvention) statt nur einen Sektor pro Kommando zu bedienen.

**Windows-Terminal-Eingabe (2026-07-07):** Der native Windows-HAL normalisiert
erweiterte `_getch()`-Tasten auf ANSI-Sequenzen, damit OS-9-Programme wie
`umacs` sie ueber `termcap` auswerten koennen. Beispiel: Cursor hoch kommt von
Windows als `0xE0 0x48`, wird im HAL zu `ESC [ A`, und `SYS/termcap` beschreibt
fuer `q9|q9term` entsprechend `ku=\E[A`. Direkte `termcap`-Versuche mit
echtem `0xE0` oder `\340H` waren nicht stabil; sichtbares Symptom waren die
zweiten Bytes (`H`, `M`, `P`, `K`) im Editor. `Ctrl-C` wird vom Windows-HAL
abgefangen, damit es nicht den Hostprozess beendet.

**OS9SYS-CF-Boot und `os9gen` (2026-07-07):** Das lokale Arbeitsimage
`local_images/OS9SYS.hda` bootet direkt von CompactFlash und fuehrt danach das
CF-`startup` aus. Fuer Bootfile-Experimente muss der formatierbare Descriptor
`/c0_fmt` verwendet werden; `/c0` ist formatgeschuetzt und fuehrt beim finalen
Rename von `OS9Boot` zu `E$Format` (`000:255`). Eine fehlerhafte Bootlist kann
ohne auffaellige `os9gen`-Meldung ein zu kleines/unbrauchbares `OS9Boot`
erzeugen; typisches Bootsymptom ist `Sysgo can't chx to 'CMDS'`, `Sysgo can't
open 'startup' file`, danach `E$BPNam` (`000:215`). Details und Arbeitsregeln:
[`CB030.md`](CB030.md), Abschnitt "Erkenntnisse aus dem produktiven CF-Boot".

**Toolshed/WSL fuer Images:** RBF-/OS-9-Images werden lokal ueber Toolshed in
Debian WSL bearbeitet (`~/.local/bin/os9`). Nicht gleichzeitig mit Toolshed
und im laufenden Emulator auf dasselbe `.hda` schreiben.

---

### 5.11 QUICC-Ethernet-Emulation (Schritt 5.11, `src/kernel/quicc.c/.h`)

Das im Q9-Emulator laufende OS-9 bekommt echtes TCP/IP: Im MWOS-Q9-Port
(`OS9/68030/PORTS/Q9/SPF/`) wurde der originale Microware-SPF-Ethernet-Treiber
`sp360` fuer den MC68360/QUICC uebernommen und fuer 68020/030 uebersetzt —
`quicc.c` emuliert die Hardware-Seite, die dieser Treiber programmiert:

- **Fenster** `$FFFF2000`–`$FFFF3FFF` (8K, kollisionsfrei zwischen den
  Netz-Terminals und dem REMAP-Register): Dual-Port-RAM mit den
  Buffer-Descriptor-Ringen, SCC1-Parameter-RAM ab `+$C00`, Registerbank ab
  `+$1000` (Offsets per `offsetof` aus Motorolas `quicc.h` verifiziert).
  Der Descriptor `spqe0` liefert dem Treiber die PRAM-Adresse `$FFFF2C00`;
  die QUICC-Basis rechnet sich der Treiber per `& $FFFFF000` selbst aus.
- **BD-Ringe/SDMA**: TX-Kick per TODR (`$8000`), Frames werden direkt aus dem
  bzw. in das Gast-RAM kopiert (die BD-Puffer sind OS-9-mbufs); `tbptr`/`rbptr`
  im PRAM werden wie vom echten CP fortgeschrieben, RX-Laengen inklusive der
  4 CRC-Bytes (der Treiber zieht sie wieder ab).
- **Interrupts**: SCCE/SCCM-Ereignislogik, CIPR/CIMR-Pegel, IRQ Level 5 mit
  Vektor 254 ueber den IACK-Callback in `m68krt.c`.
- **Host-Backend** (User-Mode-Mini-NAT, kein Root/TAP): der Emulator ist die
  Gegenstelle `192.168.200.1` — beantwortet ARP (Proxy-ARP) und ICMP-Echo;
  TCP/UDP-NAT ist eine geplante Ausbaustufe.

Verifiziert end-to-end (2026-07-12): OS-9 laedt den SPF-Stack zur Laufzeit
(`load netmods` + `mbinstall` + `ipstart` — `sysmbuf` ist ein
Coldstart-Systemmodul, beim Laufzeit-Weg uebernimmt `mbinstall` die
Installation), `ping 192.168.200.1` bekommt Antworten, `netstat -i` zeigt enet0
mit 0 Fehlern. Testskript: `test_quicc_net.exp`. (Adressierung 2026-07-13 von
10.0.0.0/24 auf 192.168.200.0/16 umgestellt, s. ARBEITSPLAN 5.13.)

## 6. Stand der Dinge

Kompletter, feingranularer Stand mit Begründungen: [`../ARBEITSPLAN.md`](../ARBEITSPLAN.md)
(Statusmodell 💡/💤/🟢/🔄/✅/⛔). Kurzfassung nach Phase (Stand 2026-07-07):

| Phase | Inhalt | Stand |
|-------|--------|-------|
| 0 | Fundament: Toolchain, HAL-Grundgerüst, Kernel-Minimalgerüst, beide Erst-Targets bauen | ✅ fertig |
| 1 | Kernel-Basis: Syscall-Dispatcher, Device-/Pfadmodell, Namensauflösung, POSIX-HAL | ✅ fertig |
| 2 | Modulsystem: Header, CRC32, Directory, F$Link/F$UnLink | ✅ fertig |
| 3 | Dateisystem: Block-Device, VFS, FAT16 lesend/schreibend, F$Load, OPFS-Backend | ✅ fertig, live mit macOS-Tooling gegengetestet |
| 4 | Prozesse: Descriptor-Tabelle, Scheduler, F$Fork/Exit/Wait/Chain, Blockieren, Suspend/Priorität, Signale | ✅ fertig, inkl. Anschluss 4.6-4.9 (echte WASM-Ausführungs-Engine, s. Entscheidung E10/O6): 4.6 (Grundbaustein wasm3), 4.7 (Syscall-Bridge, native Seite), 4.8 (Zeiger-/Speicher-Marshaling: I$Open/I$Read/I$Write/I$Close für WASM-Module), 4.9 (Fixed-Heap statt Host-malloc in wasm3, Q9-Systemkonfiguration `config.h`); Browser-Seite von 4.7 zurückgestellt (s. ARBEITSPLAN.md „Geparkt") |
| 5 | 68k-Runtime (Musashi, native) — umnummeriert 2026-07-04 abends vor Phase 6/Shell (Entscheidung E11: Shell braucht eine echte Ausführungs-Engine für reale Programme, sonst bliebe sie ein Geflecht aus Vorwegnahmen) | 🔄 begonnen: 5.1 (Grundbaustein Musashi + Makefile-Integration + Rauchtest, Abschnitt 5.9), 5.2 komplett (CB030-Board-Emulation: 5.2a RAM/ROM/Remap, 5.2b DUART, 5.2c Compact-Flash, 5.2d Timer/IRQ3, Abschnitt 5.10), 5.3/5.5a (Musashi↔CB030-Verdrahtung, Boot-Runner `q9.exe --cb030 <rom> [--cf <image>]`, CF-Multisektor) — OS9SYS-CF-Boot funktioniert lokal mit `OS9Boot`, `startup`, `q9term` und `umacs`; 5.7-5.9 (TX-Ringpuffer, Ctrl-]/DEL-Mapping, Idle-Drossel, Abschnitt 5.7/5.10) seither ebenfalls fertig |
| 6 | Shell | offen |
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
| **wasm3** ([github.com/wasm3/wasm3](https://github.com/wasm3/wasm3), Commit `d77cd814`) | WASM-Interpreter, C | **MIT** | Unverändert als `third_party/wasm3/` vendored und in `make native` mitgebaut (Entscheidung E10, Phase 4.6) — löst O5 (WASM-Runtime für den nativen Build) | Ja (MIT, LICENSE-Datei liegt bei) |
| **Musashi** ([github.com/kstenerud/Musashi](https://github.com/kstenerud/Musashi), Commit `313ebf1`) | 68000/68030-Emulator, C | **MIT** | Nur Kern-Interpreter + Codegenerator + Softfloat vendored als `third_party/musashi/` (kein Disassembler, keine Testtreiber), in `make native` mitgebaut (Entscheidung E12, Phase 5.1) — löst den CPU-Teil von O4 (68k-Board-Emulation) | Ja (MIT, Lizenztext in `m68k.h` u.a.) |

**Konsequenz für eine künftige Veröffentlichung:** Der Q9-eigene Quellbaum
(`src/`, `tools/`, `test/`) enthält keine Zeilen aus einer der obigen
GPL/proprietären Quellen — alles ist eigene Implementierung nach eigenem
Verständnis der Konzepte. Tatsächlich übernommene Fremdquellen sind
**wasm3** unter `third_party/wasm3/` und **Musashi** unter
`third_party/musashi/` (beide MIT-lizenziert, unverändert) — MIT ist mit
jeder künftigen Q9-Lizenzwahl kompatibel. Sollte künftig doch GPL-Code aus
NitrOS-9/OS9exec übernommen werden, muss die Lizenz von Q9 (oder zumindest
der betroffenen Module) GPL-kompatibel sein. Die MWOS- und Buch-Referenzen
dürfen so oder so nie als Code auftauchen, nur als Verständnisgrundlage
dienen.

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
| **Musashi** | Vendorter 68000/68030-CPU-Emulator (MIT, `third_party/musashi/`), löst den CPU-Teil der Board-Emulation im nativen Build (Entscheidung E12) |
| **Reset-Vektor** | Die ersten 8 Byte des 68k-Adressraums: initialer Stackpointer (Adresse 0) + initialer Programmzähler (Adresse 4), je 4 Byte big-endian |

---

**Erstellt**: 2026-07-04
**Letzte Aktualisierung**: 2026-07-04 (Schritt 5.1: Musashi-Grundbaustein + Makefile-Integration + Rauchtest)

#─────────────────────────────────────────────────────────────────────────────────────────────────
# EOF HANDBUCH.md                                                                          Ver. 1.50
#─────────────────────────────────────────────────────────────────────────────────────────────────
