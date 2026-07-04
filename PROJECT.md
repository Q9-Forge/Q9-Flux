# PROJECT.md — Q9

> Praktisches Handbuch (Werkzeuge, Build je Target, Quellcode-Layout, Architektur,
> Lizenzlage der Referenzquellen): [`docs/HANDBUCH.md`](docs/HANDBUCH.md).
> Dieses Dokument hier ist die Vision samt Entscheidungshistorie.

**Q9** ist ein modulares Mini-Betriebssystem in der Tradition von OS-9.
Der Kern ist portables C mit einer schmalen HAL. Primäres Executable-Format ist
WebAssembly; native 68k-Module laufen über eine eingebaute Emulator-Runtime.

**Name**: Anlehnung an OS-9. Wofür das Q steht, ist bewusst offen. 🙂

---

## 🎯 Vision

Ein OS, das die Kernideen von OS-9 (Modulsystem, einheitliches I/O, alles ist
ein Modul) in die Gegenwart holt:

- **Läuft im Browser** (WASM-Host) — sofort testbar, überall verfügbar
- **Läuft später nativ auf 68k** (Vinculum / MC68EN360) — gleicher Quellcode
- **Mischbetrieb**: WASM-Module und 68k-Module gleichberechtigt nebeneinander,
  unterschieden nur durch das Language-Byte im Modul-Header
- **Fernziele**: OS-9/6809-Binärkompatibilität per 6809-Runtime, Netzwerk,
  Self-Hosting-Toolchain

---

## 🏗️ Architektur

### Schichtenmodell

```
┌─────────────────────────────────────────────────────┐
│  User-Module (Shell, Tools, Anwendungen)            │
│  LANG_WASM │ LANG_68K │ LANG_6809 (Vision)          │
├─────────────────────────────────────────────────────┤
│  Runtimes:  nativ (WASM-Instanz) │ Musashi │ 6809   │
├─────────────────────────────────────────────────────┤
│  Q9-Kernel (portables C):                           │
│  Syscalls · Modul-Directory · Scheduler · VFS       │
├─────────────────────────────────────────────────────┤
│  HAL (pro Target implementiert)                     │
├──────────────────────────┬──────────────────────────┤
│  Target 1: WASM/Browser  │  Target 2: 68k/Vinculum  │
│  xterm.js, Canvas, OPFS/ │  UART (QUICC SMC), RAM,  │
│  File System Access API  │  CompactFlash/SCSI       │
└──────────────────────────┴──────────────────────────┘
```

### Grundsätze

1. **Kernel ist portables C (C99)** — kein Target-spezifischer Code außerhalb
   der HAL. Muss mit clang (→WASM) und vbcc (→68k) übersetzbar bleiben.
2. **HAL bleibt schmal** — Konsole, Block-Device, Timer, Yield. Alles andere
   ist Kernel-Sache.
3. **Syscall-Semantik nah an OS-9** (F$/I$-Philosophie) — das hält die
   spätere 6809-Runtime dünn und ehrt das Vorbild. Q9-Präfix: `Q$...`
4. **Kein WASM-Interpreter auf dem 68k-Target** — dort läuft 68k nativ,
   WASM-Module ggf. per wasm2c+vbcc vorkompiliert. (Interpreter höchstens
   temporär für Bootstrap-Experimente.)

### HAL-Schnittstelle (Entwurf, wird in Phase 0 festgezurrt)

```c
void     q9_hal_init(void);
void     q9_hal_con_put(char c);            /* Konsole            */
int      q9_hal_con_get(void);              /* -1 = nichts da     */
uint32_t q9_hal_ticks_ms(void);             /* Timer              */
int      q9_hal_blk_read(uint32_t lba, void *buf);   /* 512 Byte  */
int      q9_hal_blk_write(uint32_t lba, const void *buf);
void     q9_hal_yield(void);                /* Host-Kooperation   */
```

---

## 📦 Modulsystem (Herzstück, OS-9-inspiriert)

**Konzeptgetreu, aber NICHT binärkompatibel zu OS-9** (bewusste Entscheidung —
Binärkompatibilität würde die komplette OS-9-API erzwingen).
Vollständige OS-9-Referenz (Header-Felder, Modul-Directory, Link-Counting,
Sticky-Module, Namensauflösung): **docs/MODULES.md**.

### Modul-Header (Entwurf)

| Offset | Größe | Feld         | Bemerkung                                |
|--------|-------|--------------|------------------------------------------|
| $00    | 2     | Sync         | `$51 $39` = ASCII "Q9"                   |
| $02    | 2     | HeaderSize   |                                          |
| $04    | 4     | ModuleSize   | gesamt, inkl. Header + CRC               |
| $08    | 4     | NameOffset   | → nullterminierter Modulname             |
| $0C    | 1     | Type         | 1=Programm 2=Treiber 3=FileMgr 4=Data 5=Runtime |
| $0D    | 1     | Language     | 1=WASM 2=M68K 3=MC6809 (reserviert)      |
| $0E    | 1     | Attribute    | Bit0=reentrant ...                       |
| $0F    | 1     | Revision     |                                          |
| $10    | 4     | ExecOffset   | Einsprung (bei WASM: Offset der .wasm-Daten) |
| $14    | 4     | DataSize     | statischer Datenbedarf                   |
| $18    | 4     | CRC32        | über das gesamte Modul (Feld = 0 gerechnet) |

- **WASM-Module**: **Entschieden 2026-07-04** (Phase 4.7, `build_wasm_module` in
  `kernel.c` als Referenzimplementierung) — Q9-Header direkt gefolgt von den
  rohen `.wasm`-Bytes ab Offset `execoff` (Länge `datasize`), genau wie beim
  `Q9_MOD_NATIVE`-Stopgap (E9). KEINE Custom-Section-Einbettung (ursprünglicher
  Entwurf verworfen) — die Datei ist damit NICHT gleichzeitig ein eigenständig
  gültiges `.wasm`, sondern ein eigenständiges Q9-Format mit WASM-Nutzlast.
  Bewusst im Sinne von OS-9 (eigenes, in sich geschlossenes Modulformat statt
  Dual-Kompatibilität zu einem fremden Containerformat).
- **68k-Module**: Header + positionsunabhängiger 68k-Code (vbcc, PC-relativ).
- **Modul-Directory** im Kernel: Name → Adresse, Link-Count, Revision-Update,
  CRC-Prüfung beim Laden.
- **Start-Dispatch**: `Q$Fork` liest das Language-Byte und wählt die Runtime
  (WASM-Instanz nativ / Musashi / später 6809). Vorbild: OS-9s Language-Byte
  für 68k-Objektcode vs. Basic09-I-Code.

### Werkzeug: `q9mod` (tools/)

Packt Compiler-Output in das Modulformat, rechnet CRC, setzt Header-Felder.
Portables C, läuft auf dem PC (und eines Tages unter Q9 selbst).

---

## 🗺️ Phasenplan

| Phase | Inhalt | Status |
|-------|--------|--------|
| 0 | **Fundament**: Build-System (clang→WASM), HAL-Definition, "Hello Kernel" im Browser (xterm.js), nativer PC-Testbuild | offen |
| 1 | **Kernel-Basis**: initSystem, Syscall-Mechanismus (`Q$...`), Device-Modell, Konsolen-I/O | offen |
| 2 | **Modulsystem**: Header-Format final, `q9mod`-Tool, Modul-Directory, Loader (Module zunächst aus eingebautem "ROM-Image") | offen |
| 3 | **Filesystem**: VFS-Layer (RBF-artige Pfad-Semantik), erster FS-Typ | offen |
| 4 | **Prozesse**: kooperativer Scheduler, `Q$Fork`/`Q$Exit`/`Q$Wait`, Prozess-Directory | offen |
| 5 | **Shell**: erstes echtes User-Modul, Kommandos, Modul-Tools (`mdir`, `procs`...) | offen |
| 6 | **68k-Runtime**: Musashi (als WASM kompiliert) als Runtime-Modul, TRAP→Syscall-Bridge, erste LANG_68K-Module | offen |
| 7 | **68k-Target nativ**: vbcc-Build der HAL für Vinculum, Boot auf echter Hardware bzw. Board-Emulator | offen |
| 8 | **Vision**: 6809-Runtime (original OS-9/6809-Binaries!), Netzwerk (CH9121 real / WebSocket-Proxy im Browser), wasm2c-Pfad, Self-Hosting | offen |

**Arbeitsweise**: Jede Phase endet mit lauffähigem, getestetem Stand.
Tests in `test/` (01_test_..., PASS/FAIL, standalone).

---

## ⚖️ Getroffene Entscheidungen

| # | Entscheidung | Begründung |
|---|-------------|------------|
| E1 | Kern in portablem C, Dual-Target WASM+68k | einmal schreiben, sofort testen, später nativ |
| E2 | Modulsystem nach OS-9-Vorbild, nicht binärkompatibel | Konzept ja, API-Ballast nein |
| E3 | Syscalls semantisch OS-9-nah (`Q$`-Präfix) | 6809-Runtime bleibt dünn, bewährtes Design |
| E4 | Kooperativer Scheduler zuerst | einzige sauber portable Variante (WASM kennt keine Preemption); Preemption später 68k-seitig möglich |
| E5 | Kein WASM-Interpreter auf 68k | Performance; wasm2c+vbcc als Weg zu nativem Code |
| E6 | CPU-Emulation im Browser via Web Worker | UI-Thread bleibt frei; auch nötig wegen Timer-Drosselung in versteckten Tabs |
| E7 | Syscall-Nummern, Fehlercodes und Registerkonventionen = OS-9 (Quelle: MWOS funcs.h/errno.h); virtueller Registersatz im 68k-Layout (d0-d7/a0-a7), Spez. in docs/SYSCALLS.md | OS-9-Software (68k+6809) mechanisch abbildbar, Runtimes werden dünne Register-Mapper |
| E8 | Prozessmodell = Step-Modell ohne Stack-Umschaltung (2026-07-03): Scheduler verwaltet Zustände (Active/Waiting/Sleeping) und ruft pro Tick den nächsten aktiven Prozess als Step-Funktion; Blockieren = Zustandswechsel, kein eingefrorener Stack. Prozess-Ausführung pro Language-Byte austauschbar: intern/C = Step-Funktion auf dem Kernel-Stack (kein eigener Stack nötig), 68k später = Musashi-CPU-Kontext, WASM = Instanz | WASM erlaubt keine Stack-Umschaltung (Asyncify zu teuer/komplex); konsequente Fortsetzung des nicht-blockierenden q9_kernel_step()-Designs aus 0.2; interne C-Prozesse müssen dafür kooperativ in Häppchen geschrieben werden (kein blockierendes while(1)) |
| E9 | "Native" Prozessmodule (neues Language-Byte `Q9_MOD_NATIVE`, 2026-07-04, Phase 4.2): Q9 hat vor Phase 6 keine 68k/WASM-Ausführungs-Engine, die aus einem geladenen Modul heraus echten Byte-Code starten könnte. Bis dahin trägt ein `Q9_MOD_NATIVE`-Modul direkt hinter dem Header (Offset `execoff`) einen rohen `q9_proc_step_fn`-Funktionszeiger (gültig nur innerhalb desselben laufenden Host-Prozesses — NICHT Teil eines portablen Moduldateiformats). F$Fork/F$Chain lesen genau diesen Zeiger und tragen ihn als Step-Funktion in die Prozesstabelle ein | Erlaubt, F$Fork/F$Chain/F$Wait (4.2) über das echte Modul-Directory (Phase 2) end-to-end und mit echten (wenn auch nur intern lauffähigen) Prozessen zu testen, ohne auf die 68k-Runtime (Phase 6) warten zu müssen; sobald `Q9_MOD_M68K`/`Q9_MOD_WASM` einen echten Interpreter bekommen, wird `Q9_MOD_NATIVE` zum reinen Kernel-internen Sonderfall (z.B. für die Shell selbst) |
| E10 | WASM-Runtime für den nativen Build = **wasm3** (MIT-lizenziert, vendored als `third_party/wasm3/`, Commit `d77cd814`, 2026-07-04, Phase 4.6): löst O5. Nur der Kern-Interpreter ist vendored (kein WASI/libc-Layer — die künftige Syscall-Bridge in 4.7 verdrahtet eigene Q9-Importe direkt). `src/kernel/wasmrt.c/.h` kapselt wasm3 als schmale Q9-API (`q9_wasmrt_init/load/call_i32/free`), sodass Aufrufer nie `wasm3.h` einbinden müssen. Nur Teil des **nativen** Builds (`make native`) — `make wasm` bindet wasm3 bewusst nicht ein, weil Q9 im Browser selbst schon als WASM läuft (dort übernimmt `WebAssembly.instantiate` die Rolle, s. O6) | wasm3 nutzt intern `malloc`/`realloc`/`free` (Environment/Runtime/Modul-Allokation) — bewusste, eng begrenzte Ausnahme von Q9s "kein malloc im Kernel"-Regel: sie betrifft NUR den vendorten wasm3-Code selbst (third_party/wasm3/README.md dokumentiert das), nicht `wasmrt.c` oder den restlichen Kernel, dessen eigene Tabellen weiter statisch bleiben. Vendorter Code wird bewusst NICHT mit `-Wall -Wextra` übersetzt (58 Warnungen im unveränderten Original) — nur Q9-eigener Code muss warnungsfrei bleiben |

## ❓ Offene Entscheidungen

| # | Frage | Stand |
|---|-------|-------|
| O1 | Erster Filesystem-Typ: FAT16 (Interop) vs. eigenes FS (Lehrreich) | **Entschieden 2026-07-03: FAT16** (inkl. LFN-Lesen); VFS hält weitere FS-Typen offen (Details: ARBEITSPLAN.md Phase 3) |
| O2 | Grafik-Device: Framebuffer-Layout, Auflösung, Register — im Emulator entwerfen, später in Hardware (CPLD/FPGA)? | Design steht aus, Phase ≥5 |
| O4 | 68k-Board-Emulation: nur CPU (Musashi) oder auch QUICC-Peripherie für Phase 7 | zu klären in Phase 6/7 |
| O5 | WASM-Runtime für den nativen PC-Build: WAMR vs. wasm3 vs. wasmtime (eingebettet als LANG_WASM-Runtime, damit die native Version voll benutzbar ist, nicht nur Debug) | **Entschieden 2026-07-04: wasm3, siehe Entscheidung E10.** Grundbaustein (Laden + Ausführen ohne Syscall-Bridge) steht seit Schritt 4.6; die eigentliche Import-/Syscall-Anbindung folgt mit 4.7/4.8 |
| O6 | Wie werden Userland-Programme (z.B. die in `userland/` vorbereiteten Tools) zu echten ladbaren Q9-Modulen? Zwei getrennte Probleme je Language-Byte: **WASM** — kein klassisches Relozierbarkeits-Problem (adressiert nur linearen Speicher), sondern ein Instanziierungs-/Import-Problem: ein geladenes `.wasm`-Modul braucht eine Import-Tabelle, die den Rücksprung zu Q9-Syscalls bereitstellt; im Browser kann JS beliebige `.wasm`-Blobs instanziieren, nativ bräuchte es die eingebettete Runtime aus O5. **68k** (Phase 6/7) — klassisches PIC-Problem, vbcc mit PC-relativer Codeerzeugung, dazu ein definierter Rücksprung-Mechanismus für Syscalls (bei echtem OS-9 ein TRAP) | aufgeworfen 2026-07-04 (Andreas, während der `userland/`-Vorarbeiten mit Codex); **WASM-Teil, native Seite: 4.6 (Runtime) + 4.7 (Syscall-Bridge F$ID/F$Time/F$Exit über `wasmproc.c`) fertig seit 2026-07-04** — löst E9s Stopgap für native WASM-Prozesse ab; **Browser-Seite (WebAssembly.instantiate im Worker für Gastmodule) bewusst zurückgestellt** (ARBEITSPLAN.md, Abschnitt „Geparkt" — verschachteltes WASM-in-WASM ist eine eigene Design-/Implementierungsaufgabe); Zeiger-/Speicher-Marshaling folgt mit 4.8; 68k-Teil weiterhin offen bis Phase 6/7 |

---

## 📂 Verzeichnisstruktur

```
Q9/
├── PROJECT.md          # dieses Dokument
├── context.txt         # Arbeitsstand
├── README.md           # englische Kurzbeschreibung
├── src/
│   ├── kernel/         # portabler Kern (Syscalls, Module, Scheduler, VFS)
│   └── hal/
│       ├── wasm/       # HAL für Browser/Emscripten
│       ├── native/     # HAL für PC-Testbuild (CLI)
│       └── m68k/       # HAL für Vinculum (Phase 7)
├── tools/              # q9mod, mkrom (PC-Werkzeuge)
├── web/                # Browser-Frontend (xterm.js, Canvas, Loader-JS)
├── test/               # 01_test_..., standalone, PASS/FAIL
└── docs/               # Detail-Spezifikationen (Modulformat, Syscalls, HAL)
```

## 🔧 Toolchain

- **WASM**: Emscripten (emcc) oder clang + wasi-sdk — Festlegung in Phase 0
- **PC-Test**: clang/gcc nativ
- **68k**: vbcc mit M68k-Backend (PC-relativer Code), ab Phase 7
- **Standards**: Header-System und Versionierung gemäß `C:\projects\PROJECT.md`

---

**Erstellt**: 2026-07-02
**Letzte Aktualisierung**: 2026-07-02 (Initiale Version nach Design-Diskussion)
