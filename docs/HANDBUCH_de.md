#═════════════════════════════════════════════════════════════════════════════════════════════════
# File:   HANDBUCH_de.md                                                                  Ver. 3.00
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
# 26-07-04│ 1.60 │ 5.2a: Board-Speicherlogik (RAM/ROM/Remap) — q9board.c/.h, neuer       │ CF
#         │      │ Abschnitt 5.10, Abschnitt 3/6 aktualisiert                                │
# 26-07-04│ 1.70 │ 5.2b-d: DUART/Compact-Flash/Timer-IRQ3 — q9board.c/.h + m68krt.c/.h         │ CF
#         │      │ (q9_m68krt_set_irq) erweitert, Abschnitt 5.10/6 aktualisiert. Phase 5.2   │
#         │      │ damit komplett (5.2a-d alle fertig)                                       │
# 26-07-05│ 1.80 │ 5.3: Musashi-Board-Verdrahtung (q9_m68krt_attach_board), ROM-Laden         │ CF
#         │      │ (q9_board_rom_load), Boot-Runner q9boardrun.c/.h (q9.exe --rom <rom>),     │
#         │      │ Spiegelgrenzen-Korrektur (bis 0xFEFF_FFFF, I/O vor Remap erreichbar)       │
# 26-07-07│ 1.90 │ OS9SYS-CF-Boot, os9gen /c0_fmt, Windows-Terminal-Keyfix und Toolshed/WSL    │ CF
#         │      │ Arbeitsregeln dokumentiert; alten 5.2a-Spiegelgrenzen-Satz korrigiert       │
# 26-07-10│ 1.91 │ 5.7: TX-Ringpuffer (hal_posix.c) — HAL-Schnittstelle (Abschnitt 5.7) um     │ CF
#         │      │ q9_hal_con_flush/tx_ready/tx_empty erweitert, veraltete "TxRDY immer        │
#         │      │ gesetzt"-Aussage in Abschnitt 5.10 (5.2b) korrigiert                        │
# 26-07-10│ 1.92 │ 5.9: Idle-Drossel Board-Runner — q9_hal_sleep_ms (Abschnitt 5.7),           │ CF
#         │      │ q9_m68krt_is_stopped (Abschnitt 5.9), Boot-Runner-Beschreibung in           │
#         │      │ Abschnitt 5.10 aktualisiert (Ctrl-] statt Ctrl-C, Idle-Drossel-Absatz)       │
# 26-07-14│ 2.00 │ 5.17: Geraete-Registry (Entscheidung E14) — devreg.c/.h neu, Abschnitt 3     │ CF
#         │      │ (Quellcode-Layout) um devreg.c/.h ergaenzt, neuer Abschnitt 5.12 (Konzept +  │
#         │      │ alle sechs migrierten Geraete)                                              │
# 26-07-16│ 2.10 │ 5.19a: Board-Config-Datei (boardcfg.c/.h) — erster Positionsparameter =      │ CF
#         │      │ Config (.q9), mehrere CF-Images rbf/pcf, RC2014-Zweitinterface; neuer        │
#         │      │ Abschnitt 5.13                                                              │
# 26-08-12│ 3.00 │ Grosse Bereinigung: veraltete Mini-Kernel-Abschnitte entfernt (Syscall-      │ AF
#         │      │ Schicht, Geraete-/Pfadmodell, Modulsystem, VFS, Prozessmodell, WASM-Runtime  │
#         │      │ -- dieser Code wurde bereits am 2026-07-31 nach Q9RESUME-Kernel ausgelagert, │
#         │      │ das Handbuch hinkte seither hinterher). Einleitung, Quellcode-Layout (Abschn.│
#         │      │ 3), Build-Anleitung (Abschn. 4) und "Stand der Dinge" (Abschn. 6) an den      │
#         │      │ tatsaechlichen aktuellen Emulator-Stand angepasst (src/devices/, .claude/,   │
#         │      │ echtes CLI `--rom/--cf/--net`/Config-Datei statt `--selftest`-REPL). Neuer   │
#         │      │ Abschnitt 5.8 "Weitere Subsysteme" als Kurzverweis fuer alles, was nach dem   │
#         │      │ letzten Handbuch-Update (2026-07-16) dazukam (Netzwerk-Backends, Video-      │
#         │      │ Pipeline, Telnet). Umbenennung HANDBUCH.md -> HANDBOOK.md (Englisch, neues    │
#         │      │ Original) + HANDBUCH_de.md (diese Datei).                                     │
#═════════╧══════╧═════════════════════════════════════════════════════════════════════════╧══════

# Q9 Flux — Handbuch

*English version: [HANDBOOK.md](HANDBOOK.md)*

Q9 Flux ist ein 68030-Hardware-Emulator für echtes OS-9/68K, Teil von Q9
Forge. Er bettet den 68000er-CPU-Kern **Musashi** ein und emuliert Board-
Hardware (RAM/ROM/Remap, 68681-DUART, CompactFlash/RBF/PCF-Speicher, QUICC-
Ethernet, Echtzeituhr, board-config-gesteuerte CF-Profile) nah genug an
echter Hardware, um reales, unverändertes OS-9/68K zu booten und laufen zu
lassen. Läuft nativ auf macOS, Linux und Windows.

**Geschichte:** Q9 begann als eigenständiges Mini-Betriebssystem in der
Tradition von OS-9 (eigener Kernel, Modulsystem, WebAssembly als
Implementierungssprache für ein Browser-Ziel). Dieser Teil war seit dem
2026-07-04 unangetastet, während die gesamte weitere Arbeit in den OS-9/68K-
Emulator floss; er wurde nach
[`Q9RESUME-Kernel`](https://github.com/foellmy51/Q9RESUME-Kernel) archiviert
(vollständige Historie erhalten). **Dieses Handbuch beschreibt den
Emulator-Zweig** — für den archivierten Mini-Kernel-Stand siehe
`docs/PROJECT_VISION_ARCHIV.md`.

Dieses Handbuch bündelt alles, was jemand braucht, der das Projekt zum ersten
Mal aufschlägt: Werkzeuge, Quellcode-Aufbau, Build-Prozess, Architektur,
Lizenzlage. Für Details verweist es auf die Fachdokumente in `docs/` statt
sie zu wiederholen.

**Verwandte Dokumente:**
- [`../.claude/ARBEITSPLAN.md`](../.claude/ARBEITSPLAN.md) — laufender Arbeitsstand, Schritt für Schritt (internes Arbeitsdokument, siehe unten)
- [`SYSCALLS.md`](SYSCALLS.md) — OS-9-Syscall-ABI (Register, Fehlercodes) — historische Referenz aus der Mini-Kernel-Zeit, für das Verständnis des emulierten Gast-OS-9 weiterhin nützlich
- [`DEVICES.md`](DEVICES.md) — Geräte-/Pfadtabelle, Treiber-Schnittstelle (Mini-Kernel-Ära)
- [`MODULES.md`](MODULES.md) — OS-9-Modulsystem als Referenz (Mini-Kernel-Ära)
- [`TOOLCHAIN.md`](TOOLCHAIN.md) — Toolchain-Stand je Entwicklungsrechner (Versionen, Pfade)
- [`AUTONOMIE.md`](AUTONOMIE.md) — Setup für den automatisierten Arbeitsmodus (projektintern, für Aussenstehende irrelevant)
- [`BOARD.md`](BOARD.md) — Hardware-Referenz (Speicherkarte, DUART/CF-Register) für die Musashi-Board-Emulation
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
der Lizenzentscheidung für Q9 mitgedacht werden. Tatsächlich vendorte Fremdquelle
ist **Musashi** (`third_party/musashi/`, MIT-lizenziert) — MIT ist unproblematisch
mit praktisch jeder Zielrezenz kombinierbar, eigene LICENSE-Datei liegt bei.

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
| C-Compiler + make | Emulator übersetzen | macOS: Xcode Command Line Tools (`xcode-select --install`, liefert clang + make); Linux: übliches `build-essential`/`gcc`-Paket der Distribution |
| Python 3 | Testsuite | meist vorinstalliert; sonst über die Distribution/Homebrew |
| libslirp (optional) | `--net slirp`-Backend | macOS: `brew install libslirp`; Linux: `apt install libslirp-dev` o.ä. — fehlt es, meldet sich `--net slirp` beim Start sauber ab |

Kein Emscripten, kein sonstiges Spezialwerkzeug nötig.

### 2.2 Nativer Build (Windows)

| Werkzeug | Zweck | Installation |
|----------|-------|---------------|
| w64devkit | gcc + make + busybox-sh, portabel | ZIP von der [w64devkit-Releaseseite](https://github.com/skeeto/w64devkit) entpacken, `bin/`-Ordner in den `PATH` der Shell aufnehmen |
| Python 3 | Testsuite | offizieller Python-Installer (portable ZIP-Variante ebenfalls möglich) |

### 2.3 68k-Target (Vinculum / MC68EN360, geplant)

| Werkzeug | Zweck | Stand |
|----------|-------|-------|
| vbcc (M68k-Backend) | erzeugt positionsunabhängigen 68k-Code | noch nicht installiert, geplant mit der Vinculum-Hardware (s. Q9-Forge/ROADMAP.md) |

### 2.4 Versionskontrolle

Git, Remote auf GitHub (aktuell privates Repository). Kein Werkzeug-Sonderfall,
aber erwähnt, weil der komplette Arbeitsablauf (Autonomie-Betrieb, siehe
[`AUTONOMIE.md`](AUTONOMIE.md)) auf `git commit`/`git push` nach jedem
abgeschlossenen Schritt aufsetzt.

---

## 3. Quellcode-Layout

```
Q9-Flux/
├── AGENTS.md              Verweis auf die Arbeitsdokumente (bleibt am Root, s. .claude/)
├── README.md              englische Kurzbeschreibung
├── LICENSE
├── Makefile                Build-System, siehe Abschnitt 4
├── .gitignore / .gitmodules
├── emu*.q9                 Board-Config-Profile (mehrere parallele Arbeitsstände/Personen)
├── .claude/                Arbeitsdokumente (s. Abschnitt 3.1): ARBEITSPLAN.md (+ _de/_ARCHIV),
│                           context.txt, Q9_CURRENT_STATUS.md, BUGFIX_CF_WRITE.md,
│                           TEST_CF_WRITE.md, COMMIT_MESSAGE.txt
├── src/
│   ├── hal/                Host-Abstraktion — WELCHE Maschine der Emulator selbst läuft
│   │   ├── q9_hal.h           gemeinsames Interface (Konsole, Timer, Block-Device, Zeit)
│   │   ├── windows/           Windows (conio, Winsock2)
│   │   └── posix/             macOS/Linux (termios, BSD-Sockets)
│   ├── kernel/              CPU-Kern + Board-Bus + Orchestrierung — alles, was KEIN
│   │   │                     eigenständiges Gerät ist, bleibt hier (Abschnitt 5.4)
│   │   ├── m68krt.c/.h         Wrapper um die eingebettete Musashi-68k-Emulation (5.3)
│   │   ├── q9board.c/.h        Board-Bus: RAM/ROM/Remap, 68681-DUART, Compact-Flash,
│   │   │                       RTC72421, Timer/IRQ3 (5.4) — bewusst gebündelt
│   │   ├── q9boardrun.c/.h      Boot-Runner/Hauptschleife (das eigentliche "mainFile", das
│   │   │                        Musashi aufruft — `main()` selbst liegt je HAL, ruft dies auf)
│   │   ├── devreg.c/.h          Geräte-Vtable + Registry (5.6)
│   │   ├── boardcfg.c/.h        `.q9`-Konfigurationsdatei-Parser (5.7)
│   │   └── q9_sockcompat.h      Winsock/BSD-Socket-Kompatibilitätsschicht
│   └── devices/             simulierte Board-Hardware — ein Unterordner je eigenständigem Gerät
│       ├── mc6845/             CRT-Controller (Video-Timing)
│       ├── clut/                 Farbpalette
│       ├── framebuf/             VRAM/Framebuffer-Gerät
│       ├── quicc/                 QUICC-Ethernet (5.5)
│       ├── videobridge/           Host-Protokoll für Q9-Frame (Remote-Framebuffer-Streaming)
│       └── net/                    Host-Netzwerk-Backends: slirp_net (plattformübergreifend),
│                                     vmnet_net/bpf_net (nur macOS)
├── third_party/
│   ├── musashi/             vendorter 68000/68030-CPU-Emulator (MIT, unverändert)
│   ├── slirp/               vendorte Windows-libslirp bzw. Anbindung ans System-libslirp
│   └── tvision/              Launcher-Prototyp (s. Git-Historie)
├── test/                    Testskripte/-programme, aus dem Projekt-Root aufrufbar nach `make host`
├── tools/                   Host-seitige Hilfswerkzeuge (Patch-/Diagnose-Skripte)
├── userland/                erste Q9-Userland-Tools (Codex-Baustelle, s. ARBEITSPLAN.md Phase U)
└── docs/                    Fachdokumente (dieses Handbuch + Detail-Spezifikationen)
```

### 3.1 Regeln, an die sich der Quellcode hält

Diese Regeln sind keine Stilfrage, sondern tragen die Portabilität des Projekts:

- **`src/kernel/` und `src/devices/` bleiben zu 100 % host-unabhängiges C99.**
  Kein `#ifdef _WIN32`, kein `#include <windows.h>` — jeder Unterschied
  zwischen Windows/macOS/Linux gehört ausschließlich in die jeweilige
  HAL-Datei unter `src/hal/` (Windows-Sockets sind die eine bewusste
  Ausnahme, abstrahiert über `q9_sockcompat.h`, nicht per verstreuten
  `#ifdef`s).
- **Kein `malloc` im Kernel/in den Geräten.** Zustand (Register, Puffer,
  Tabellen) liegt in statischen Structs fester Größe (`q9_board_t`,
  `q9_cf_t`, die Geräte-Registry als statisches Array) — analog zu OS-9s
  ROM-orientiertem Design.
- **Jede Datei trägt einen Box-Header** (Datei/Owner/Beschreibung/Aufruf) und
  eine **Edition History** (Datum, Version, Beschreibung, Kürzel) — siehe jede
  bestehende `.c`/`.h`-Datei als Vorlage. So bleibt die Entstehungsgeschichte
  auch ohne `git blame` lesbar.
- **Die HAL-Schnittstelle (`q9_hal.h`) ist bewusst schmal**: Konsole, Timer,
  Block-Device, Systemzeit, Target-Name. Alles andere (Board-Bus,
  Geräte-Emulation) ist Kernel-/Device-Sache und läuft überall gleich.

`.claude/ARBEITSPLAN.md`, `.claude/context.txt` und `docs/AUTONOMIE.md` sind
**Arbeitsprozess-Dokumente** für die Zusammenarbeit zwischen Andreas und
Claudia (Claude Code) — sie dokumentieren *wie* gearbeitet wird
(Freigabe-Workflow, automatisierte Läufe), nicht *was* Q9 Flux ist. Für eine
öffentliche Version sind sie nicht gedacht mitzugehen; dieses Handbuch ist
das stabile, publikationsfähige Dokument.

---

## 4. Bauen für die einzelnen Targets

Alles über das `Makefile` im Projekt-Root:

```bash
make host   # -> build/<platform>/q9.exe   ("make native" läuft als stiller Alias weiter)  (Windows: conio-HAL; macOS/Linux: POSIX-HAL, automatisch gewählt)
make test     # baut native + führt die Testprogramme aus test/ aus
make clean    # entfernt build/
```

Die Wahl zwischen Windows- und POSIX-HAL im `native`-Target passiert automatisch
über die Make-Variable `$(OS)` (unter Windows von `cmd`/PowerShell gesetzt) —
kein manuelles Umschalten nötig.

Das Ausgabeverzeichnis ist plattform-spezifisch benannt (`build/windows/`,
`build/macos/`, `build/linux/`) — baut man denselben Checkout (z.B. über eine
Netzwerkfreigabe) auf mehreren Betriebssystemen, überschreiben sich die
Objektdateien/Binaries so nicht gegenseitig.

### 4.1 Nativer Build ausprobieren

```bash
make host
./build/<platform>/q9.exe mysystem.q9                                  # Board-Config-Datei
./build/<platform>/q9.exe --rom <rom> [--cf <image>] [--net nat|vmnet|bridge:<if>|slirp]
```

Ohne Config-Datei UND ohne `--rom` bricht der Start mit einer Usage-Meldung
ab (kein Kernel-Fallback mehr, seit der Mini-Kernel nach `Q9RESUME-Kernel`
ausgelagert wurde).

Windows PowerShell auf dem lokalen AF-PC:

```powershell
cd D:\projekts\Q9
$env:PATH = "C:\Users\AF\w64devkit\bin;$env:PATH"
$env:OS = "Windows_NT"
make host
.\build\native\q9.exe --rom .\local_images\roms\romimage.dev.running.BIN --cf .\local_images\OS9SYS.hda
```

Auf dem AF-PC zeigt `python3.exe` auf den Microsoft-Store-Alias. Fuer die
Tests daher explizit das echte Python verwenden:

```powershell
make test PYTHON=python
```

Vor Neubuilds oder Image-Arbeiten pruefen, ob noch alte Emulatorprozesse laufen:

```powershell
Get-CimInstance Win32_Process |
  Where-Object { $_.ExecutablePath -like 'D:\projekts\Q9\build\native\q9*.exe' } |
  Select-Object ProcessId,Name,ExecutablePath,CommandLine
```

Fenster/Tab schließen beendet `q9*.exe` nicht immer zuverlaessig; ein alter
Prozess kann die EXE sperren oder zu falschen Testergebnissen fuehren.

### 4.2 Tests

`make test` baut nativ und führt `test/07_test_cf_sector512.c` aus: ein
eigenständiges, CPU-/OS-9-unabhängiges ATA-PIO-Sektor-Roundtrip gegen die
Compact-Flash-Emulation (`q9board.c`/`devreg.c`) — RBF/256-Byte, RBF/512-Byte
und PCF/512-Byte je mit 10 Prüfungen (Einzel-/Mehrfach-Sektor, 8/16/32-Bit-
Pfade, Master/Slave-Isolation). Kein Testframework, ein einziges C-Programm,
Ausgabe `PASS`/`FAIL` je Prüfung plus Zusammenfassung.

### 4.3 68k-Target (Vinculum) — noch nicht umgesetzt

Ab geplanter Vinculum-Phase vorgesehen: `src/hal/m68k/` (analog zu
`windows/`/`posix/`) für einen nativen Build auf echter 68360-Hardware. Siehe
Q9-Forge/ROADMAP.md, Abschnitt "Vinculum".

---

## 5. Software-Architektur

### 5.1 Schichtenmodell

```
┌───────────────────────────────────────────────────────────────┐
│  Gast: echtes, unverändertes OS-9/68K (proprietäres Boot-ROM)  │
├───────────────────────────────────────────────────────────────┤
│  CPU-Kern: Musashi (68030) — src/kernel/m68krt.c/.h             │
├───────────────────────────────────────────────────────────────┤
│  Board-Bus: RAM/ROM/Remap, DUART, Compact-Flash, RTC, Timer     │
│  — src/kernel/q9board.c/.h                                       │
├───────────────────────────────────────────────────────────────┤
│  Geräte-Registry (src/kernel/devreg.c/.h) + Geräte              │
│  (src/devices/*): MC6845 · CLUT · Framebuffer · QUICC-Ethernet  │
│  · Videobridge (Q9-Frame) · Netzwerk-Backends (slirp/vmnet/bpf) │
├───────────────────────────────────────────────────────────────┤
│  Board-Runner/Hauptschleife — src/kernel/q9boardrun.c/.h         │
├───────────────────────────────────────────────────────────────┤
│  HAL (pro Host-Betriebssystem, src/hal/)                        │
├──────────────────────────────┬────────────────────────────────┤
│  windows/ — Windows             │  posix/ — macOS/Linux          │
│  (conio, Winsock2)              │  (termios, BSD-Sockets)         │
└──────────────────────────────┴────────────────────────────────┘
```

### 5.2 HAL-Schnittstelle

```c
void          q9_hal_init(void);
void          q9_hal_con_put(char c);
int           q9_hal_con_get(void);              /* -1 = nichts da, nicht-blockierend */
void          q9_hal_con_flush(void);            /* TX-Puffer-Rest nachliefern (Haupt-Loop) */
int           q9_hal_con_tx_ready(void);          /* Platz fuer mind. 1 weiteres Byte (TxRDY) */
int           q9_hal_con_tx_empty(void);          /* Puffer vollstaendig geleert (TxEMT)      */
void          q9_hal_sleep_ms(uint32_t ms);       /* Idle-Drossel im Host-Loop                */
uint32_t      q9_hal_ticks_ms(void);
int           q9_hal_blk_read (uint32_t lba, void *buf);        /* 512-Byte-Block */
int           q9_hal_blk_write(uint32_t lba, const void *buf);
int           q9_hal_time(q9_datetime_t *dt);
const char   *q9_hal_target(void);
```

Jedes Target implementiert genau diese Funktionen; alles Weitere (Board-Bus,
Geräte) ist reiner Kernel-/Device-Code und läuft überall gleich.
`q9_hal_con_flush/tx_ready/tx_empty` sind nur auf POSIX ein echter TX-
Ringpuffer (`hal_posix.c`) — native (Windows) bleibt synchron und liefert
trivial "immer leer/bereit".

### 5.3 Musashi-68k-Emulation (native Build, Entscheidung E12)

Q9 Flux braucht einen eingebetteten 68k-Interpreter, um im nativen PC-Build
(nicht nur auf echter 68k-Hardware) laufen zu können — Wahl: **Musashi** (MIT,
Karl Stenerud), vendored unter `third_party/musashi/` (E12). Ziel-CPU-Typ ist
**68030** (nächstliegender, gut unterstützter Musashi-Typ zur realen
Zielhardware MC68EN360/QUICC mit CPU32+-Kern, den Musashi nicht kennt) — MMU
bleibt ungenutzt, damit der emulierte 68k-Code nicht versehentlich von
68030-Exklusivfeatures abhängt, die auf CPU32-Hardware fehlen würden.

Musashi hat einen **Zweistufen-Build**: das Host-Tool `m68kmake` liest
`third_party/musashi/m68k_in.c` (518 handgeschriebene Opcode-Primitive) und
generiert daraus `m68kops.c/.h` (1967 Opcode-Handler) — reine Build-Artefakte,
landen zur Bauzeit unter `build/<platform>/musashi_gen/` und werden nicht
versioniert. Musashis eigener Kern-Interpreter (`m68kcpu.c`, das intern
bereits `m68kfpu.c` per `#include` einbindet — `m68kfpu.c` darf deshalb NICHT
zusätzlich separat übersetzt werden, sonst doppelte Symbole beim Linken) +
der Softfloat-Unterbau (`softfloat/softfloat.c`) werden mit eigenen, laxeren
Flags übersetzt (unveränderter Fremdcode).

`src/kernel/m68krt.c/.h` ist der schmale Q9-Wrapper. Musashi hält seinen
kompletten CPU-Zustand in eigenen globalen Variablen — die von Musashi
verlangten Speicherzugriffsfunktionen (`m68k_read/write_memory_8/16/32`)
bekommen keinen Kontext-Zeiger übergeben. `q9_m68krt_t` ist deshalb bewusst
kein mehrfach instanzierbares Handle, sondern nur eine dünne
Buchhaltungsstruktur neben einem globalen RAM-Zeiger in `m68krt.c` — es kann
je Prozesslauf immer nur EINE Musashi-Instanz aktiv sein.

```c
// m68krt.h — Grundbaustein
int      q9_m68krt_init(q9_m68krt_t *rt, uint8_t *ram, uint32_t ram_len);
void     q9_m68krt_reset(q9_m68krt_t *rt);
int      q9_m68krt_execute(q9_m68krt_t *rt, int cycles);
uint32_t q9_m68krt_get_d(q9_m68krt_t *rt, int n);   // Dn, n=0..7
void     q9_m68krt_free(q9_m68krt_t *rt);
int      q9_m68krt_is_stopped(void);                // CPU per STOP angehalten?
```

**6.5: `src/kernel/cpu_backend.h`** ist eine kleine Vtable (`reset`/`execute`/
`set_irq`/`is_stopped`/`ctx`), analog zu `devreg.h`'s Geräte-Vtable.
`q9_m68krt_get_backend(rt, &backend)` befüllt eine solche mit dünnen
Wrappern um die obigen Funktionen; `q9boardrun.c`s Hauptschleife ruft nur
noch über diese Vtable, nie direkt `q9_m68krt_*`. Reine Diagnosefunktionen
(`q9_m68krt_debug_state`, `q9_m68krt_quicc_acks`) bleiben bewusst außerhalb
der Vtable — sie sind 68k-spezifisch (Status-Register, QUICC-Zähler), keine
allgemeine CPU-Eigenschaft. Das ist Vorbereitung für eine zweite
Zielarchitektur, noch nicht an eine angeschlossen (s. den vendorten
TinyEMU-RISC-V-Kern, `third_party/tinyemu/Q9_VENDOR_de.md`, dessen API fast
1:1 auf diese Vtable-Form passt).

Mit `M68K_SEPARATE_READS` aus (third_party/musashi/m68kconf.h) genügen genau
diese sechs Speicherfunktionen — Musashis interne
`m68k_read_immediate_*`/`m68k_read_pcrelative_*` fallen ohnehin auf dieselben
sechs zurück (s. `m68kcpu.h`).

### 5.4 Board-Emulation (`src/kernel/q9board.c/.h`)

Das **Q9-Board** dient als Bootstrap/Validierungs-Zwischenschritt für die
Musashi-Integration — mit dem originalen, proprietären OS-9-Boot-ROM
statt nur mit handassemblierten Testprogrammen. Ändert nichts an der
eigentlichen Q9-Zielhardware (MC68EN360/QUICC, s. Q9-Forge/ROADMAP.md
"Vinculum"). Speicherkarte + Peripherie-Register: [`docs/BOARD.md`](BOARD.md).
Das reale Boot-ROM bleibt wie die MWOS-SDK-Kopie proprietär und NICHT im
Repository — nur die Hardware-Dokumentation selbst und der Emulationscode
sind es.

**RAM/ROM/Remap-Speicherlogik:** reiner Adress-Dekoder als if/else-Kette (RAM
zuerst geprüft), unabhängig von Musashis eigenem CPU-Zustand — der
REMAP-Merker sitzt in einem eigenen `q9_board_t`-Handle:

```c
// q9board.h — Adress-Dispatch
int      q9_board_init(q9_board_t *b, const uint8_t *rom, uint32_t rom_len,
                        uint8_t *ram, uint32_t ram_len);
void     q9_board_reset(q9_board_t *b);
uint8_t  q9_board_read8(q9_board_t *b, uint32_t addr);   // + read16/read32
void     q9_board_write8(q9_board_t *b, uint32_t addr, uint8_t val);  // + write16/write32
```

Reset-Zustand: ROM bei Adresse 0, gespiegelt bis `0xFEFF_FFFF`. Ein einzelner
Buszugriff (lesend oder schreibend) auf den REMAP-Registerbereich
(`0xFFFF_8000`–`0xFFFF_8FFF`) schaltet dauerhaft um — reiner Adress-Trigger,
kein Datenwert; danach liegt RAM ab Adresse 0 und das ROM nur noch einmal,
unmirrored, bei `0xFE00_0000`–`0xFE07_FFFF` (read-only).

**68681-DUART:** Minimalansatz — nur SRA (Status, `UART_BASE+0x02`) und
THRA/RHRA (Zeichenpuffer, `UART_BASE+0x06`) sind wirklich aktiv, der
restliche Registersatz wird sauber angenommen (liest 0, verwirft
Schreibzugriffe). SRA-Bits: RxRDY (`0x01`), TxRDY (`0x04`) und TxEMT
(`0x08`), die den Füllstand des HAL-seitigen TX-Ringpuffers ehrlich
widerspiegeln (`q9_hal_con_tx_ready`/`q9_hal_con_tx_empty`, Abschnitt 5.2).
THRA-Schreibzugriff → `q9_hal_con_put`.

**Compact-Flash:** ATA-PIO-Minimalprotokoll — Register `CF_BASE+0` (Data, 1
Byte/Zugriff), `+2` (Sectcount), `+3..+5` (LBA0-2), `+7` (Kommando/Status).
Unterstützte Kommandos: READ SECTOR(S) (`0x20`), WRITE SECTOR(S) (`0x30`),
Statusbits BSY/DRQ/RDY/ERR wie in `docs/BOARD.md` festgelegt. Backing Store
ist eine lazy geöffnete Host-Datei (`q9_board_cf_attach(board, path)`,
Multi-Image seit der Board-Konfigurationsdatei, s. Abschnitt 5.7).

**Timer/IRQ3:** kooperative Umsetzung statt echtem Host-Timerinterrupt (ein
Signal-Handler oder separater Thread wäre nicht threadsicher gegen Musashis
globalen, nicht-reentranten Zustand). `q9_board_poll_timer(board, now_ms)`
prüft, ob der Timer per `TI_IRQ_ON`/`TI_IRQ_OFF` (reine Adress-Trigger,
`0xFFFF_9000`-`0xFFFF_9FFF`) aktiv ist und seit dem letzten Auslösen ≥10ms
(100 Hz) Host-Zeit (`q9_hal_ticks_ms()`) vergangen sind — der Aufrufer muss
dann selbst `q9_m68krt_set_irq(3)` aufrufen. `q9board.c` kennt Musashi
bewusst nicht; `q9_m68krt_set_irq()` ist ein schmaler Wrapper in `m68krt.h/.c`
um `m68k_set_irq()` — Musashi erledigt die eigentliche Interrupt-Mechanik
(Stack/Vektorsprung) vollständig selbst.

**Musashi ↔ Board verdrahten + Boot-Runner:** `q9_m68krt_attach_board(&board)`
schaltet die sechs Musashi-Speicher-Hooks vom nackten RAM-Block auf den
Board-Adress-Dispatch um — ab dann laufen ALLE CPU-Zugriffe (inkl. der
Reset-Vektoren) über `q9_board_read/write8/16/32`. Der Interrupt-Acknowledge
läuft im Board-Betrieb als Autovector mit Puls-Verhalten (die IRQ-Leitung wird
beim Annehmen losgelassen, sonst würde der level-gehaltene IRQ3 endlos erneut
unterbrechen). Der Boot-Runner (`q9boardrun.c`) macht daraus ein Kommando
(s. Abschnitt 4.1 für die volle Optionsliste):

```
./build/<platform>/q9.exe --rom <pfad-zum-rom-image> [--cf <pfad-zum-cf-image>]
```

Lädt das ROM (max. 512 KByte, `q9_board_rom_load`), stellt 16 MByte
emuliertes RAM, hängt die CF-Backing-Datei an und lässt die CPU laufen
(Ende: Ctrl-] als Host-Escape, s. `q9_hal_con_get`). **Idle-Drossel:** Ist die
CPU per `STOP` angehalten (`q9_m68krt_is_stopped`, z.B. OS-9s Leerlauf am
Login-Prompt) UND liegt kein IRQ an, schläft der Host-Loop kurz
(`q9_hal_sleep_ms(1)`) statt den nächsten Slice sofort "leer" zu verbrennen —
senkt die Host-CPU-Last im Leerlauf von ~100 % auf ca. 1–2 %, ohne die
OS-9-Uhr zu verfälschen. Das echte Boot-ROM ist proprietär und bleibt lokal
— `.gitignore` deckt `boardrom*.bin`/`*.rom` ab.

**Windows-Terminal-Eingabe:** Der native Windows-HAL normalisiert erweiterte
`_getch()`-Tasten. **Seit 2026-08-09** ist der Standard nicht mehr ANSI,
sondern **WinEds eigene Emacs-Bindungen**: WinEd 3.9 (und `umacs`) verarbeiten
ANSI-Cursorfolgen nicht als einzelne Tasten, sondern erwarten für die vier
Richtungen direkt ihre Steuercodes (`Hoch=^P`, `Runter=^N`, `Links=^B`,
`Rechts=^F`) — das ist jetzt der Windows-Default. Für ein Programm, das
klassische ANSI-Sequenzen braucht, `Q9_KEYMODE=ansi` (oder `vt100`) setzen;
Beispiel im ANSI-Modus: Cursor hoch kommt von Windows als `0xE0 0x48`, wird
im HAL zu `ESC [ A`, und `SYS/termcap` beschreibt fuer `q9|q9term`
entsprechend `ku=\E[A` — `q9term` selbst wird von diesem Umschalter nicht
berührt. Windows-Terminal verarbeitet Mausmarkierung, `Ctrl+V` und manche
F-Tasten selbst; diese Host-Funktionen werden deshalb nicht im Emulator auf
WinEd-Befehle umgebogen. `Ctrl-C` wird vom Windows-HAL abgefangen, damit es
nicht den Hostprozess beendet.

**OS9SYS-CF-Boot und `os9gen`:** Das lokale Arbeitsimage
`local_images/OS9SYS.hda` bootet direkt von CompactFlash und fuehrt danach das
CF-`startup` aus. Fuer Bootfile-Experimente muss der formatierbare Descriptor
`/c0_fmt` verwendet werden; `/c0` ist formatgeschuetzt und fuehrt beim finalen
Rename von `OS9Boot` zu `E$Format` (`000:255`). Details und Arbeitsregeln:
[`BOARD.md`](BOARD.md), Abschnitt "Erkenntnisse aus dem produktiven CF-Boot".

**Toolshed/WSL fuer Images:** RBF-/OS-9-Images werden lokal ueber Toolshed in
Debian WSL bearbeitet (`~/.local/bin/os9`). Nicht gleichzeitig mit Toolshed
und im laufenden Emulator auf dasselbe `.hda` schreiben.

---

### 5.5 QUICC-Ethernet-Emulation (`src/devices/quicc/quicc.c/.h`)

Details: der Quellcode selbst (`quicc.c/.h`) und die Netzwerk-Backend-Wahl
in Abschnitt 5.8. QUICC ist ein CB030/Board-spezifischer Ethernet-Controller
(Motorola-Kommunikationsprozessor) — die Emulation reicht für OS-9s
`enet0`-Treiber aus, ohne den vollen QUICC-Befehlssatz nachzubilden.

### 5.6 Geräte-Registry (`src/kernel/devreg.c/.h`, Entscheidung E14)

Vor der Registry waren Board-Peripheriegeräte als sechs hartkodierte
if/switch-Ketten an drei Stellen verdrahtet (Speicher-Dispatch in `m68krt.c`,
Hauptschleifen-Poll in `q9boardrun.c`, IRQ-Ack/Reassert in `m68krt.c`).
`devreg.h` bündelt das in ein generisches Geräte-Interface: `q9_device_t`
trägt Adressfenster (base/size), IRQ-Zuordnung (level/vector) und eine
Vtable mit den Zugriffsfunktionen (`read8/write8` Pflicht, 16/32-Bit sowie
`poll`/`irq_pending`/`reset` optional). Eine statische Registry
(`q9_devreg_add/get/count`) hält die Instanzen; eine separate, ebenfalls
statisch kompilierte Typ-Registry (`q9_devtype_lookup`) bildet Typnamen
("duart68681", "cf", ...) auf ihre Vtable ab — Grundlage dafür, dass die
Board-Konfigurationsdatei (Abschnitt 5.7) Geräte künftig per Typname
instanziieren kann.

### 5.7 Board-Konfigurationsdatei (`src/kernel/boardcfg.c/.h`)

Der Emulator nimmt als **ersten Positionsparameter (ohne führendes `-`) eine
Board-Config-Datei** an; fehlt die Extension, wird `.q9` angenommen:

```sh
./build/<platform>/q9.exe mysystem            # lädt mysystem.q9
./build/<platform>/q9.exe mysystem.q9 --net vmnet
```

Die bestehenden CLI-Optionen bleiben unverändert und **überschreiben** die
Config (Vorrang: eingebaute Defaults < Config-Datei < CLI). Ohne Config UND
ohne `--rom` bricht der Start mit einer Usage-Meldung ab (Abschnitt 4.1).
Format (INI-artig, C99-Parser ohne Fremdbibliothek, Kommentare `;`/`#`,
Pfade **relativ zur Config-Datei**):

```ini
[board]
name = Q9-Board
rom  = roms/romimage.dev.running.BIN     ; Boot-ROM (statt --rom)
net  = nat                               ; nat | vmnet | bridge:<ifname> | slirp
; Nur fuer net = vmnet/slirp: zentrale Adressdaten des virtuellen Q9-Netzes
vmnet_ip       = 192.168.200.2            ; statische Gast-IP in OS-9
vmnet_gateway  = 192.168.200.1            ; Shared-Mode-Gateway
vmnet_netmask  = 255.255.255.0
vmnet_dhcp_end = 192.168.200.254

[cf0]                                     ; beliebig viele [cfN]-Abschnitte
type  = rbf                              ; rbf (OS-9-RBF) | pcf (FAT12/16)
bus   = onboard                          ; onboard ($FFFFE000) | rc2014 ($FFFFC010)
unit  = master                          ; master | slave
image = OS9SYS.hda

[cf1]
type  = pcf
bus   = rc2014
unit  = slave
image = q9-fat16.img
```

Die Compact-Flash-Emulation ist **mehrfach instanziierbar**: der CF-Zustand
steckt in einem eigenständigen Typ `q9_cf_t` (mit zwei `q9_cf_unit_t`,
Master/Slave). Damit gibt es zwei CF-**Interfaces**: die Onboard-CF
(`$FFFFE000`, Descriptoren `c0..c3`) und das **RC2014-SC145-Zweitinterface**
(`$FFFFC010`, Descriptoren `e0`/`f0`). Welche Einheit ein ATA-Kommando
bedient, entscheidet — wie bei echter ATA-Hardware — das DEV-Bit (Bit 4) im
LBA3-Register. Der `type`-Schlüssel steuert die Sektorgrößen-Heuristik: `rbf`
(bzw. `auto`) erkennt alte 256-Byte-LSN-Images an LSN0, `pcf` **schaltet
diese Heuristik ab** (sonst würde der FAT-Bootsektor als OS-9-LSN0
fehlgedeutet) und behandelt das Image als reine 512-Byte-Sektoren.

Test-Images erzeugt man mit ToolShed (RBF: `os9 format -bs512 -c32 …`) bzw.
`tools/make_fat_image.py` (FAT12/16-Superfloppy). Beispiel-Config:
[`q9board.example.q9`](q9board.example.q9).

### 5.8 Weitere Subsysteme (Kurzüberblick)

Dieses Handbuch wurde zuletzt am 2026-07-16 inhaltlich fortgeschrieben (bis
Schritt 5.19a); danach ist im Emulator einiges dazugekommen, das hier nur
kurz benannt wird — vollständige Details stehen in `.claude/ARBEITSPLAN.md`:

| Subsystem | Kurzbeschreibung | ARBEITSPLAN-Bereich |
|---|---|---|
| Netzwerk-Backends | `--net nat` (eingebautes Mini-NAT) · `vmnet`/`bridge:<if>` (macOS, echtes Netz) · `slirp` (plattformübergreifend, `libslirp`) inkl. `net_hostfwd` (Host→Gast-Portweiterleitung, z.B. Telnet) | Schritte 5.10–5.16 |
| Telnet-Terminals | Acht virtuelle Netzwerk-Terminals `/x1`–`/x8` über TCP (Port 2000+), Telnet-NVT-Normalisierung | Schritt 5.10 (Erweiterung 8 Kanäle) |
| Video-Pipeline | MC6845-Registermodell (`src/devices/mc6845/`) + Framebuffer/VRAM (`src/devices/framebuf/`) + CLUT-Farbpalette (`src/devices/clut/`) + Q9-Frame-Netzwerkprotokoll für Remote-Anzeige (`src/devices/videobridge/`) | Schritte 5.24–5.29 |
| RTC72421 | Echtzeituhr (Epson-Baustein), liest die Host-Uhr | Schritt 5.6 |
| Debug-Sondertaste | Ctrl-^ dumpt physischen RAM-Inhalt (Q9-OS-Reverse-Engineering-Hilfe) | s. `q9_dbg_dump_requested` in `q9boardrun.h` |
| Mehrarchitektur-Planung | RISC-V32/ARM64/x86-32-Bit als weitere Zielarchitekturen, Board-Emulator vs. native Runtime — noch reine Planung | Phase 6 |

---

## 6. Stand der Dinge

Kompletter, feingranularer Stand mit Begründungen:
[`../.claude/ARBEITSPLAN.md`](../.claude/ARBEITSPLAN.md)
(Statusmodell 💡/💤/🟢/🔄/✅/⛔). Kurzfassung:

| Phase | Inhalt | Stand |
|-------|--------|-------|
| 0–4 | Mini-Kernel: Fundament, Syscall-Dispatcher, Modulsystem, Dateisystem (FAT16), Prozesse, WASM-Runtime | ✅ historisch abgeschlossen, aber **nicht mehr Teil dieses Quellbaums** — seit 2026-07-04 unangetastet, nach `Q9RESUME-Kernel` ausgelagert (s. README.md „History") |
| 5 | 68k-Board-Emulator (Musashi) | 🔄 sehr weit fortgeschritten — de facto der aktuelle Kern des Projekts. Bootet echtes, unverändertes OS-9/68K vollständig: Compact-Flash (RBF/PCF), DUART, Telnet-Terminals, Netzwerk (nat/vmnet/bridge/slirp), Framebuffer/Video (MC6845/CLUT/Q9-Frame), Geräte-Registry, Board-Konfigurationsdateien. Feindetails: ARBEITSPLAN.md Schritte 5.1–5.32+ |
| 6 | Mehrere Zielarchitekturen (RISC-V32, ARM64, x86 32-Bit) | 💡 in Planung (2026-08-11), Struktur (Board-Emulator vs. native Runtime) besprochen, noch keine Umsetzung |
| U | Userland-Werkzeuge | 🔄 mehrere Q9-Userland-Tools existieren (Codex-Baustelle, `userland/`) |

Getestet wird nativ auf Windows/macOS/Linux bei jedem Schritt — siehe
`test/` und die Verifikations-Notizen in `.claude/ARBEITSPLAN.md`.

---

## 7. Referenzquellen und ihre Lizenzen

Q9 ist eigenständig entwickelt, orientiert sich aber am Vorbild OS-9. Folgende
Quellen dienen als fachliche Referenz — mit unterschiedlicher Rechtslage:

| Quelle | Art | Lizenz | Verwendung in Q9 | Weitergebbar? |
|--------|-----|--------|-------------------|----------------|
| **MWOS SDK** (Microware OS-9, private Kopie) | Original-Header/Definitionen (`funcs.h`, `errno.h`, ...) | proprietär, privat | Referenz für Syscall-Nummern, Fehlercodes, Registerkonventionen des emulierten Gast-OS-9, sowie Adressen/Descriptoren fürs Board (CB030-Q9-Port) | **Nein** — nur zum Abgleich verwendet, keine Datei daraus liegt im Repo |
| **OS-9 Insights** (Peter Dibble) | Fachbuch, ältere Auflage enthält abgedruckten FAT16-File-Manager | Buch-Copyright | Design-Referenz, historisch (Mini-Kernel-Ära) | **Nein** — nur gelesen/verstanden, kein Code übernommen |
| **NitrOS-9** ([github.com/nitros9project/nitros9](https://github.com/nitros9project/nitros9)) | Community-OS-9/6809, RBF in 6809-Assembler | **GPL** | bislang nur als Idee vorgemerkt (Ideenspeicher, ARBEITSPLAN.md) | Ja (GPL, Copyleft beachten) |
| **ToolShed** (Teil des NitrOS-9-Projekts) | PC-Tools zum Lesen/Schreiben von RBF-Images, in C | vermutlich GPL (im Kontext von NitrOS-9 zu prüfen) | aktiv genutzt zur Image-Bearbeitung (WSL/Toolshed, s. Abschnitt 5.4) | zu prüfen |
| **OS9exec** (Lukas Zeller/Beat Forster) | 68k-Emulator + OS-9-Kernel-Nachbau in C, Syscall-Ebene | **GPL** | bislang nur als Idee vorgemerkt: Referenz für spätere Syscall-Bridge-Fragen | Ja (GPL, Copyleft beachten) |
| **Musashi** ([github.com/kstenerud/Musashi](https://github.com/kstenerud/Musashi), Commit `313ebf1`) | 68000/68030-Emulator, C | **MIT** | Nur Kern-Interpreter + Codegenerator + Softfloat vendored als `third_party/musashi/` (kein Disassembler, keine Testtreiber), in `make host` mitgebaut (Entscheidung E12) | Ja (MIT, Lizenztext in `m68k.h` u.a.) |
| **TinyEMU** ([github.com/fernandotcl/TinyEMU](https://github.com/fernandotcl/TinyEMU), Commit `56ba49b`) | RISC-V-Emulator, C (Fabrice Bellard) | **MIT** | Nur der CPU-Kern vendored als `third_party/tinyemu/` (kein Board/Geräte/x86) — das RISC-V-Gegenstück zu Musashis Rolle, s. `docs/RISCV_de.md` | Ja (MIT, Lizenzkopf in jeder Quelldatei) |
| **libslirp** | User-Mode-Netzwerkstack, C | **BSD-2-Clause** | `--net slirp`-Backend: Windows vendored unter `third_party/slirp/windows/`, macOS/Linux gegen System-libslirp per pkg-config | Ja |

**Konsequenz für eine künftige Veröffentlichung:** Der Q9-eigene Quellbaum
(`src/`, `tools/`, `test/`) enthält keine Zeilen aus einer der obigen
GPL/proprietären Quellen — alles ist eigene Implementierung nach eigenem
Verständnis der Konzepte. Tatsächlich übernommene Fremdquellen sind
**Musashi** unter `third_party/musashi/` und **libslirp** (MIT bzw.
BSD-2-Clause, beide unverändert) — kompatibel mit jeder künftigen
Q9-Lizenzwahl. Sollte künftig doch GPL-Code aus NitrOS-9/OS9exec übernommen
werden, muss die Lizenz von Q9 (oder zumindest der betroffenen Module)
GPL-kompatibel sein. Die MWOS- und Buch-Referenzen dürfen so oder so nie als
Code auftauchen, nur als Verständnisgrundlage dienen.

---

## 8. Glossar

Kurzreferenz für Begriffe, die im Projekt und den Fachdokumenten
vorausgesetzt werden:

| Begriff | Bedeutung |
|---------|-----------|
| **RBF** | „Random Block File" — OS-9s natives Dateisystem/File-Manager, ein CF-Image-Format in der Board-Konfiguration |
| **PCF** | Im Q9-Board-Kontext: FAT12/16-formatiertes CF-Image (Gegenstück zu RBF), von Windows/macOS direkt mountbar |
| **DUART** | Dual UART — hier der emulierte Motorola 68681, liefert die serielle Konsole des Board |
| **QUICC** | Motorola-Kommunikationsprozessor mit u.a. Ethernet-MAC — hier die emulierte Netzwerkschnittstelle des Board |
| **CLUT** | Color Look-Up Table — Farbpalette zwischen Framebuffer-Indexwerten und tatsächlichen RGB-Werten |
| **F$...** / **I$...** | OS-9-Syscall-Namenskonvention des GAST-Betriebssystems: `F$` = Systemfunktion (z.B. F$Fork), `I$` = I/O-Operation (z.B. I$Read) — Q9 Flux implementiert diese nicht selbst, sondern emuliert die Hardware, auf der das echte OS-9 sie ausführt |
| **Descriptor** | OS-9-Begriff für eine Verwaltungsstruktur (Path Descriptor, Device Descriptor) — im Board-Kontext z.B. die CF-Descriptoren `c0`/`e0`/`f0` |
| **Superfloppy** | Datenträger-Image ohne Partitionstabelle — Boot-Sektor direkt bei Block 0 |
| **Musashi** | Vendorter 68000/68030-CPU-Emulator (MIT, `third_party/musashi/`), löst den CPU-Teil der Board-Emulation im nativen Build (Entscheidung E12) |
| **Reset-Vektor** | Die ersten 8 Byte des 68k-Adressraums: initialer Stackpointer (Adresse 0) + initialer Programmzähler (Adresse 4), je 4 Byte big-endian |
| **Board-Runner** | `q9boardrun.c`, die Hauptschleife, die Musashi + Board + Geräte zusammenführt und ausführt |
| **HAL** | Hardware Abstraction Layer — hier: Abstraktion der Host-Maschine (Konsole/Timer/Sockets), NICHT der emulierten Gast-Hardware (das ist die Geräte-Registry, s. Abschnitt 5.6) |

---

**Erstellt**: 2026-07-04
**Letzte Aktualisierung**: 2026-08-12 (grosse Bereinigung, s. Edition History oben)

#─────────────────────────────────────────────────────────────────────────────────────────────────
# EOF HANDBUCH_de.md                                                                      Ver. 3.00
#─────────────────────────────────────────────────────────────────────────────────────────────────
