#═════════════════════════════════════════════════════════════════════════════════════════════════
# File:   HANDBOOK.md                                                                    Ver. 3.00
# Owner:  AF
# Desc.:  Central handbook: tools, source-code layout, build per target, software architecture,
#         reference sources including licensing status. Meant as the entry point for anyone
#         seeing the project for the first time — including in the event of a future public
#         release. Points to the existing specialist documents instead of duplicating them.
#
# Edition History
#─────────┬──────┬─────────────────────────────────────────────────────────────────────────┬──────
# Date    │ Ver. │ Description                                                             │ By
#─────────┼──────┼─────────────────────────────────────────────────────────────────────────┼──────
# 26-07-04│ 1.00 │ Initial version, after completing Phase 3 (filesystem)                  │ CF
# 26-07-04│ 1.10 │ 4.6: wasm3 runtime (decision E10) — third_party/wasm3, wasmrt.c/.h,      │ CF
#         │      │ new section 5.8, license/reference-source table added                   │
# 26-07-04│ 1.20 │ 4.7: syscall bridge (native side) — wasmproc.c/.h, section 5.8 extended, │ CF
#         │      │ Phase-4 status in section 6 updated                                      │
# 26-07-04│ 1.30 │ 4.8: pointer/memory marshaling — section 2.1 (wabt/wat2wasm freshly      │ CF
#         │      │ installed), section 5.8 extended, Phase-4 status in section 6           │
# 26-07-04│ 1.40 │ 4.9: fixed heap for wasm3 (config.h) — section 3 (config.h), 5.8         │ CF
#         │      │ extended, Phase-4 status in section 6 (4.6-4.9 now fully connected)      │
# 26-07-04│ 1.50 │ 5.1: Musashi 68k emulation (decision E12) — third_party/musashi/,        │ CF
#         │      │ m68krt.c/.h, new section 5.9, sections 3/6/7 updated                     │
# 26-07-04│ 1.60 │ 5.2a: board memory logic (RAM/ROM/remap) — q9board.c/.h, new             │ CF
#         │      │ section 5.10, sections 3/6 updated                                       │
# 26-07-04│ 1.70 │ 5.2b-d: DUART/Compact-Flash/timer IRQ3 — q9board.c/.h + m68krt.c/.h       │ CF
#         │      │ (q9_m68krt_set_irq) extended, section 5.10/6 updated. Phase 5.2 thereby   │
#         │      │ complete (5.2a-d all done)                                                │
# 26-07-05│ 1.80 │ 5.3: wiring Musashi to the board (q9_m68krt_attach_board), ROM loading    │ CF
#         │      │ (q9_board_rom_load), boot runner q9boardrun.c/.h (q9.exe --rom <rom>),    │
#         │      │ mirror-boundary fix (up to 0xFEFF_FFFF, I/O reachable before remap)       │
# 26-07-07│ 1.90 │ OS9SYS CF boot, os9gen /c0_fmt, Windows terminal key fix, and Toolshed/WSL │ CF
#         │      │ working rules documented; corrected the old 5.2a mirror-boundary sentence  │
# 26-07-10│ 1.91 │ 5.7: TX ring buffer (hal_posix.c) — HAL interface (section 5.7) extended  │ CF
#         │      │ with q9_hal_con_flush/tx_ready/tx_empty, corrected the stale "TxRDY       │
#         │      │ always set" statement in section 5.10 (5.2b)                              │
# 26-07-10│ 1.92 │ 5.9: idle throttle for the board runner — q9_hal_sleep_ms (section 5.7),   │ CF
#         │      │ q9_m68krt_is_stopped (section 5.9), boot-runner description in            │
#         │      │ section 5.10 updated (Ctrl-] instead of Ctrl-C, idle-throttle paragraph)   │
# 26-07-14│ 2.00 │ 5.17: device registry (decision E14) — new devreg.c/.h, section 3          │ CF
#         │      │ (source-code layout) extended with devreg.c/.h, new section 5.12          │
#         │      │ (concept + all six migrated devices)                                      │
# 26-07-16│ 2.10 │ 5.19a: board config file (boardcfg.c/.h) — first positional parameter =    │ CF
#         │      │ config (.q9), multiple CF images rbf/pcf, RC2014 secondary interface;      │
#         │      │ new section 5.13                                                          │
# 26-08-12│ 3.00 │ Major cleanup: removed stale mini-kernel sections (syscall layer,          │ AF
#         │      │ device/path model, module system, VFS, process model, WASM runtime —       │
#         │      │ this code had already been moved out to Q9RESUME-Kernel on 2026-07-31,     │
#         │      │ and the handbook had been lagging behind ever since). Introduction,        │
#         │      │ source-code layout (section 3), build instructions (section 4), and        │
#         │      │ "Current status" (section 6) brought in line with the actual current       │
#         │      │ emulator state (src/devices/, .claude/, the real CLI                       │
#         │      │ `--rom/--cf/--net`/config file instead of the `--selftest` REPL). New       │
#         │      │ section 5.8 "Other subsystems" as a short pointer to everything added      │
#         │      │ since the last handbook update (2026-07-16) (network backends, video       │
#         │      │ pipeline, Telnet). Renamed HANDBUCH.md -> HANDBOOK.md (English, new         │
#         │      │ original) + HANDBUCH_de.md (this file's German counterpart).               │
#═════════╧══════╧═════════════════════════════════════════════════════════════════════════╧══════

# Q9 Flux — Handbook

*German version: [HANDBUCH_de.md](HANDBUCH_de.md)*

Q9 Flux is a 68030 hardware emulator for real OS-9/68K, part of Q9
Forge. It embeds the 68000-family CPU core **Musashi** and emulates board
hardware (RAM/ROM/remap, 68681 DUART, CompactFlash/RBF/PCF storage, QUICC
Ethernet, real-time clock, board-config-driven CF profiles) closely enough
to real hardware to boot and run real, unmodified OS-9/68K. Runs natively
on macOS, Linux, and Windows.

**History:** Q9 started out as a standalone mini operating system in the
OS-9 tradition (its own kernel, module system, WebAssembly as the
implementation language for a browser target). That part had been
untouched since 2026-07-04, while all further work went into the OS-9/68K
emulator; it was archived to
[`Q9RESUME-Kernel`](https://github.com/foellmy51/Q9RESUME-Kernel) (full
history preserved). **This handbook describes the emulator branch** — for
the archived mini-kernel state, see `docs/PROJECT_VISION_ARCHIV.md`.

This handbook bundles everything someone needs when opening the project for
the first time: tools, source-code structure, build process, architecture,
licensing status. For details it points to the specialist documents in
`docs/` instead of repeating them.

**Related documents:**
- [`../.claude/ARBEITSPLAN.md`](../.claude/ARBEITSPLAN.md) — current work status, step by step (internal working document, see below)
- [`SYSCALLS.md`](SYSCALLS.md) — OS-9 syscall ABI (registers, error codes) — a historical reference from the mini-kernel era, still useful for understanding the emulated guest OS-9
- [`DEVICES.md`](DEVICES.md) — device/path table, driver interface (mini-kernel era)
- [`MODULES.md`](MODULES.md) — OS-9 module system as a reference (mini-kernel era)
- [`TOOLCHAIN.md`](TOOLCHAIN.md) — toolchain status per development machine (versions, paths)
- [`AUTONOMIE.md`](AUTONOMIE.md) — setup for the automated working mode (internal to the project, irrelevant to outsiders)
- [`BOARD.md`](BOARD.md) — hardware reference (memory map, DUART/CF registers) for the Musashi board emulation
- [`OS9SYS_BOOT.md`](OS9SYS_BOOT.md) — local runbook for OS9SYS CF boot, `os9gen /c0_fmt`, Toolshed/WSL, and the terminal key fix

---

## Contents

1. [License and Legal Status](#1-license-and-legal-status)
2. [Tools and Installation](#2-tools-and-installation)
3. [Source-Code Layout](#3-source-code-layout)
4. [Building the Individual Targets](#4-building-the-individual-targets)
5. [Software Architecture](#5-software-architecture)
6. [Current Status](#6-current-status)
7. [Reference Sources and Their Licenses](#7-reference-sources-and-their-licenses)
8. [Glossary](#8-glossary)

---

## 1. License and Legal Status

**Q9's own license status is still open** (README.md: "TBD"). This needs
to be resolved before the project goes public — especially in light of
the reference sources listed in section 7, some of which are freely
redistributable (GPL) and some of which explicitly **are not** (the
private MWOS SDK copy, a copyrighted book).

**Most important rule until this is resolved:** No code from
proprietary/private sources (MWOS) and no code copied out of copyrighted
material (books) goes into the Q9 source tree. These sources serve
exclusively as a *reference for understanding* — everything is implemented
independently. For GPL sources (NitrOS-9, OS9exec), taking code over would
be legally permitted, but that isn't currently done either; should that
change, the GPL follows automatically (copyleft) — which has to be factored
into Q9's eventual license decision. The one piece of third-party source
actually vendored in is **Musashi** (`third_party/musashi/`,
MIT-licensed) — MIT combines cleanly with practically any target license,
and its own LICENSE file is included.

Details and context on the individual sources: section 7.

---

## 2. Tools and Installation

All tools are installed **portably, without admin/root privileges** —
uninstalling simply means deleting the folder. The full, machine-specific
state (exact paths and versions per development machine) is in
[`TOOLCHAIN.md`](TOOLCHAIN.md); here's the summary by purpose.

### 2.1 Native Build (macOS / Linux)

| Tool | Purpose | Installation |
|----------|-------|---------------|
| C compiler + make | Compile the emulator | macOS: Xcode Command Line Tools (`xcode-select --install`, provides clang + make); Linux: the distribution's usual `build-essential`/`gcc` package |
| Python 3 | Test suite | usually preinstalled; otherwise via the distribution/Homebrew |
| libslirp (optional) | `--net slirp` backend | macOS: `brew install libslirp`; Linux: `apt install libslirp-dev` or similar — if missing, `--net slirp` fails cleanly at startup |

No Emscripten, no other special tooling needed.

### 2.2 Native Build (Windows)

| Tool | Purpose | Installation |
|----------|-------|---------------|
| w64devkit | gcc + make + busybox-sh, portable | Unzip the ZIP from the [w64devkit release page](https://github.com/skeeto/w64devkit), add the `bin/` folder to the shell's `PATH` |
| Python 3 | Test suite | the official Python installer (a portable ZIP variant also works) |

### 2.3 68k Target (Vinculum / MC68EN360, planned)

| Tool | Purpose | Status |
|----------|-------|---------------|
| vbcc (M68k backend) | generates position-independent 68k code | not yet installed, planned alongside the Vinculum hardware (see Q9-Forge/ROADMAP.md) |

### 2.4 Version Control

Git, remote on GitHub (currently a private repository). Not a special
tooling case, but worth mentioning because the entire workflow (autonomous
operation, see [`AUTONOMIE.md`](AUTONOMIE.md)) is built on `git
commit`/`git push` after every completed step.

---

## 3. Source-Code Layout

```
Q9-Flux/
├── AGENTS.md              pointer to the working documents (stays at the root, see .claude/)
├── README.md              short English project description
├── LICENSE
├── Makefile                build system, see section 4
├── .gitignore / .gitmodules
├── emu*.q9                 board config profiles (several parallel work states/people)
├── .claude/                working documents (see section 3.1): ARBEITSPLAN.md (+ _de/_ARCHIV),
│                           context.txt, Q9_CURRENT_STATUS.md, BUGFIX_CF_WRITE.md,
│                           TEST_CF_WRITE.md, COMMIT_MESSAGE.txt
├── src/
│   ├── hal/                host abstraction — WHICH machine the emulator itself runs on
│   │   ├── q9_hal.h           shared interface (console, timer, block device, time)
│   │   ├── native/            Windows (conio, Winsock2)
│   │   └── posix/             macOS/Linux (termios, BSD sockets)
│   ├── kernel/              CPU core + board bus + orchestration — everything that is NOT
│   │   │                     its own standalone device stays here (section 5.4)
│   │   ├── m68krt.c/.h         wrapper around the embedded Musashi 68k emulation (5.3)
│   │   ├── q9board.c/.h        board bus: RAM/ROM/remap, 68681 DUART, Compact Flash,
│   │   │                       RTC72421, timer/IRQ3 (5.4) — deliberately bundled together
│   │   ├── q9boardrun.c/.h      boot runner/main loop (the actual "mainFile" that calls
│   │   │                        Musashi — `main()` itself lives per-HAL, and calls into this)
│   │   ├── devreg.c/.h          device vtable + registry (5.6)
│   │   ├── boardcfg.c/.h        `.q9` config-file parser (5.7)
│   │   └── q9_sockcompat.h      Winsock/BSD socket compatibility layer
│   └── devices/             simulated board hardware — one subfolder per standalone device
│       ├── mc6845/             CRT controller (video timing)
│       ├── clut/                 color palette
│       ├── framebuf/             VRAM/framebuffer device
│       ├── quicc/                 QUICC Ethernet (5.5)
│       ├── videobridge/           host protocol for Q9-Frame (remote framebuffer streaming)
│       └── net/                    host network backends: slirp_net (cross-platform),
│                                     vmnet_net/bpf_net (macOS only)
├── third_party/
│   ├── musashi/             vendored 68000/68030 CPU emulator (MIT, unmodified)
│   ├── slirp/               vendored Windows libslirp, resp. hookup to the system libslirp
│   └── tvision/              launcher prototype (see git history)
├── test/                    test scripts/programs, runnable from the project root after `make native`
├── tools/                   host-side helper tools (patch/diagnostic scripts)
├── userland/                first Q9 userland tools (Codex's work area, see ARBEITSPLAN.md Phase U)
└── docs/                    specialist documents (this handbook + detailed specs)
```

### 3.1 Rules the Source Code Follows

These rules aren't a matter of style — they're what carries the project's
portability:

- **`src/kernel/` and `src/devices/` stay 100% host-independent C99.** No
  `#ifdef _WIN32`, no `#include <windows.h>` — every difference between
  Windows/macOS/Linux belongs exclusively in the respective HAL file under
  `src/hal/` (Windows sockets are the one deliberate exception, abstracted
  via `q9_sockcompat.h` rather than scattered `#ifdef`s).
- **No `malloc` in the kernel or in the devices.** State (registers,
  buffers, tables) lives in fixed-size static structs (`q9_board_t`,
  `q9_cf_t`, the device registry as a static array) — analogous to OS-9's
  own ROM-oriented design.
- **Every file carries a box header** (file/owner/description/usage) and
  an **edition history** (date, version, description, initials) — see any
  existing `.c`/`.h` file as a template. This keeps the file's history
  readable even without `git blame`.
- **The HAL interface (`q9_hal.h`) is deliberately narrow**: console,
  timer, block device, system time, target name. Everything else (board
  bus, device emulation) is kernel/device business and runs identically
  everywhere.

`.claude/ARBEITSPLAN.md`, `.claude/context.txt`, and `docs/AUTONOMIE.md`
are **working-process documents** for the collaboration between Andreas
and Claudia (Claude Code) — they document *how* the work happens (the
approval workflow, automated runs), not *what* Q9 Flux is. They aren't
meant to ship with a public version; this handbook is the stable,
publication-ready document.

---

## 4. Building the Individual Targets

Everything goes through the `Makefile` at the project root:

```bash
make native   # -> build/<platform>/q9.exe  (Windows: conio HAL; macOS/Linux: POSIX HAL, chosen automatically)
make test     # builds native + runs the test programs from test/
make clean    # removes build/
```

The choice between the Windows and POSIX HAL in the `native` target
happens automatically via the make variable `$(OS)` (set by
`cmd`/PowerShell on Windows) — no manual switching needed.

The output directory is named per-platform (`build/windows/`,
`build/macos/`, `build/linux/`) — if the same checkout is built on
multiple operating systems (e.g. over a network share), the object
files/binaries don't overwrite each other.

### 4.1 Trying Out the Native Build

```bash
make native
./build/<platform>/q9.exe mysystem.q9                                  # board config file
./build/<platform>/q9.exe --rom <rom> [--cf <image>] [--net nat|vmnet|bridge:<if>|slirp]
```

Without a config file AND without `--rom`, startup aborts with a usage
message (no more kernel fallback, since the mini-kernel was moved out to
`Q9RESUME-Kernel`).

Windows PowerShell on the local AF-PC:

```powershell
cd D:\projekts\Q9
$env:PATH = "C:\Users\AF\w64devkit\bin;$env:PATH"
$env:OS = "Windows_NT"
make native
.\build\native\q9.exe --rom .\local_images\roms\romimage.dev.running.BIN --cf .\local_images\OS9SYS.hda
```

On the AF-PC, `python3.exe` points to the Microsoft Store alias. For
tests, therefore, explicitly use the real Python:

```powershell
make test PYTHON=python
```

Before rebuilding or working on images, check whether old emulator
processes are still running:

```powershell
Get-CimInstance Win32_Process |
  Where-Object { $_.ExecutablePath -like 'D:\projekts\Q9\build\native\q9*.exe' } |
  Select-Object ProcessId,Name,ExecutablePath,CommandLine
```

Closing the window/tab doesn't always reliably terminate `q9*.exe`; a
leftover process can lock the EXE or lead to incorrect test results.

### 4.2 Tests

`make test` builds natively and runs `test/07_test_cf_sector512.c`: a
standalone ATA-PIO sector round-trip against the Compact Flash emulation
(`q9board.c`/`devreg.c`), independent of the CPU/OS-9 — RBF 256-byte, RBF
512-byte, and PCF 512-byte, each with 10 checks (single-/multi-sector,
8/16/32-bit paths, master/slave isolation). No test framework, a single C
program, output `PASS`/`FAIL` per check plus a summary.

### 4.3 68k Target (Vinculum) — Not Yet Implemented

Planned for the upcoming Vinculum phase: `src/hal/m68k/` (analogous to
`native/`/`posix/`) for a native build on real 68360 hardware. See
Q9-Forge/ROADMAP.md, section "Vinculum".

---

## 5. Software Architecture

### 5.1 Layer Model

```
┌───────────────────────────────────────────────────────────────┐
│  Guest: real, unmodified OS-9/68K (proprietary boot ROM)      │
├───────────────────────────────────────────────────────────────┤
│  CPU core: Musashi (68030) — src/kernel/m68krt.c/.h            │
├───────────────────────────────────────────────────────────────┤
│  Board bus: RAM/ROM/remap, DUART, Compact Flash, RTC, timer    │
│  — src/kernel/q9board.c/.h                                       │
├───────────────────────────────────────────────────────────────┤
│  Device registry (src/kernel/devreg.c/.h) + devices             │
│  (src/devices/*): MC6845 · CLUT · framebuffer · QUICC Ethernet  │
│  · videobridge (Q9-Frame) · network backends (slirp/vmnet/bpf)  │
├───────────────────────────────────────────────────────────────┤
│  Board runner/main loop — src/kernel/q9boardrun.c/.h             │
├───────────────────────────────────────────────────────────────┤
│  HAL (per host operating system, src/hal/)                     │
├──────────────────────────────┬────────────────────────────────┤
│  native/ — Windows             │  posix/ — macOS/Linux          │
│  (conio, Winsock2)              │  (termios, BSD sockets)         │
└──────────────────────────────┴────────────────────────────────┘
```

### 5.2 HAL Interface

```c
void          q9_hal_init(void);
void          q9_hal_con_put(char c);
int           q9_hal_con_get(void);              /* -1 = nothing available, non-blocking */
void          q9_hal_con_flush(void);            /* flush the remaining TX buffer (main loop) */
int           q9_hal_con_tx_ready(void);          /* room for at least 1 more byte (TxRDY) */
int           q9_hal_con_tx_empty(void);          /* buffer fully drained (TxEMT)          */
void          q9_hal_sleep_ms(uint32_t ms);       /* idle throttle in the host loop         */
uint32_t      q9_hal_ticks_ms(void);
int           q9_hal_blk_read (uint32_t lba, void *buf);        /* 512-byte block */
int           q9_hal_blk_write(uint32_t lba, const void *buf);
int           q9_hal_time(q9_datetime_t *dt);
const char   *q9_hal_target(void);
```

Every target implements exactly these functions; everything else (board
bus, devices) is pure kernel/device code and runs identically everywhere.
`q9_hal_con_flush/tx_ready/tx_empty` are only a real TX ring buffer on
POSIX (`hal_posix.c`) — native (Windows) stays synchronous and trivially
reports "always empty/ready".

### 5.3 Musashi 68k Emulation (Native Build, Decision E12)

Q9 Flux needs an embedded 68k interpreter to run in the native PC build
(not just on real 68k hardware) — choice: **Musashi** (MIT, Karl
Stenerud), vendored under `third_party/musashi/` (E12). The target CPU
type is **68030** (the closest well-supported Musashi type to the real
target hardware MC68EN360/QUICC with its CPU32+ core, which Musashi
doesn't know about) — the MMU stays unused, so the emulated 68k code
doesn't accidentally end up depending on 68030-exclusive features that
would be missing on CPU32 hardware.

Musashi has a **two-stage build**: the host tool `m68kmake` reads
`third_party/musashi/m68k_in.c` (518 hand-written opcode primitives) and
generates `m68kops.c/.h` (1967 opcode handlers) from it — pure build
artifacts, land under `build/<platform>/musashi_gen/` at build time and
aren't checked in. Musashi's own core interpreter (`m68kcpu.c`, which
internally already pulls in `m68kfpu.c` via `#include` — `m68kfpu.c`
therefore must NOT additionally be compiled separately, or you get
duplicate symbols at link time) plus the softfloat substrate
(`softfloat/softfloat.c`) are compiled with their own, looser flags
(unmodified third-party code).

`src/kernel/m68krt.c/.h` is the thin Q9 wrapper. Musashi keeps its entire
CPU state in its own global variables — the memory-access functions
Musashi requires (`m68k_read/write_memory_8/16/32`) don't get passed a
context pointer. `q9_m68krt_t` is therefore deliberately not a handle you
can instantiate multiple times, just a thin bookkeeping struct alongside a
global RAM pointer in `m68krt.c` — only ONE Musashi instance can ever be
active per process run.

```c
// m68krt.h — basic building block
int      q9_m68krt_init(q9_m68krt_t *rt, uint8_t *ram, uint32_t ram_len);
void     q9_m68krt_reset(q9_m68krt_t *rt);
int      q9_m68krt_execute(q9_m68krt_t *rt, int cycles);
uint32_t q9_m68krt_get_d(q9_m68krt_t *rt, int n);   // Dn, n=0..7
void     q9_m68krt_free(q9_m68krt_t *rt);
int      q9_m68krt_is_stopped(void);                // CPU halted via STOP?
```

With `M68K_SEPARATE_READS` off (third_party/musashi/m68kconf.h), exactly
these six memory functions suffice — Musashi's internal
`m68k_read_immediate_*`/`m68k_read_pcrelative_*` fall back to the same six
anyway (see `m68kcpu.h`).

### 5.4 Board Emulation (`src/kernel/q9board.c/.h`)

The **Q9 board** serves as a bootstrap/validation intermediate step for
the Musashi integration — using the original, proprietary OS-9 boot ROM
instead of just hand-assembled test programs. Doesn't change anything
about the actual Q9 target hardware (MC68EN360/QUICC, see
Q9-Forge/ROADMAP.md "Vinculum"). Memory map + peripheral registers:
[`docs/BOARD.md`](BOARD.md). The real boot ROM, like the MWOS SDK copy,
stays proprietary and is NOT in the repository — only the hardware
documentation itself and the emulation code are.

**RAM/ROM/remap memory logic:** a pure address decoder implemented as an
if/else chain (RAM checked first), independent of Musashi's own CPU
state — the REMAP flag lives in its own `q9_board_t` handle:

```c
// q9board.h — address dispatch
int      q9_board_init(q9_board_t *b, const uint8_t *rom, uint32_t rom_len,
                        uint8_t *ram, uint32_t ram_len);
void     q9_board_reset(q9_board_t *b);
uint8_t  q9_board_read8(q9_board_t *b, uint32_t addr);   // + read16/read32
void     q9_board_write8(q9_board_t *b, uint32_t addr, uint8_t val);  // + write16/write32
```

Reset state: ROM at address 0, mirrored up to `0xFEFF_FFFF`. A single bus
access (read or write) to the REMAP register range (`0xFFFF_8000`–
`0xFFFF_8FFF`) switches it permanently — a pure address trigger, no data
value involved; after that, RAM sits at address 0 and the ROM appears only
once, unmirrored, at `0xFE00_0000`–`0xFE07_FFFF` (read-only).

**68681 DUART:** minimal approach — only SRA (status, `UART_BASE+0x02`)
and THRA/RHRA (character buffer, `UART_BASE+0x06`) are really active; the
rest of the register set is cleanly stubbed (reads 0, discards writes).
SRA bits: RxRDY (`0x01`), TxRDY (`0x04`), and TxEMT (`0x08`), which
honestly reflect the fill level of the HAL-side TX ring buffer
(`q9_hal_con_tx_ready`/`q9_hal_con_tx_empty`, section 5.2). A THRA write →
`q9_hal_con_put`.

**Compact Flash:** a minimal ATA-PIO protocol — registers `CF_BASE+0`
(data, 1 byte/access), `+2` (sector count), `+3..+5` (LBA0-2), `+7`
(command/status). Supported commands: READ SECTOR(S) (`0x20`), WRITE
SECTOR(S) (`0x30`), status bits BSY/DRQ/RDY/ERR as fixed in
`docs/BOARD.md`. The backing store is a lazily opened host file
(`q9_board_cf_attach(board, path)`, multi-image support since the board
config file, see section 5.7).

**Timer/IRQ3:** a cooperative implementation instead of a real host timer
interrupt (a signal handler or a separate thread wouldn't be thread-safe
against Musashi's global, non-reentrant state). `q9_board_poll_timer(board,
now_ms)` checks whether the timer is active via `TI_IRQ_ON`/`TI_IRQ_OFF`
(pure address triggers, `0xFFFF_9000`-`0xFFFF_9FFF`) and whether ≥10ms
(100 Hz) of host time (`q9_hal_ticks_ms()`) has passed since it last
fired — the caller then has to call `q9_m68krt_set_irq(3)` itself.
`q9board.c` deliberately doesn't know about Musashi; `q9_m68krt_set_irq()`
is a thin wrapper in `m68krt.h/.c` around `m68k_set_irq()` — Musashi
handles the actual interrupt mechanics (stack/vector jump) entirely on its
own.

**Wiring Musashi to the board + boot runner:**
`q9_m68krt_attach_board(&board)` switches the six Musashi memory hooks
from a bare RAM block over to the board address dispatch — from then on
ALL CPU accesses (including the reset vectors) go through
`q9_board_read/write8/16/32`. In board mode, interrupt acknowledge runs as
an autovector with pulse behavior (the IRQ line is released on
acknowledge, otherwise the level-held IRQ3 would keep re-interrupting
endlessly). The boot runner (`q9boardrun.c`) turns this into a command
(see section 4.1 for the full option list):

```
./build/<platform>/q9.exe --rom <pfad-zum-rom-image> [--cf <pfad-zum-cf-image>]
```

Loads the ROM (max. 512 KB, `q9_board_rom_load`), sets up 16 MB of
emulated RAM, attaches the CF backing file, and lets the CPU run (exit:
Ctrl-] as the host escape, see `q9_hal_con_get`). **Idle throttle:** if
the CPU is halted via `STOP` (`q9_m68krt_is_stopped`, e.g. OS-9 idling at
the login prompt) AND no IRQ is pending, the host loop briefly sleeps
(`q9_hal_sleep_ms(1)`) instead of immediately burning the next slice
"empty" — this drops idle host CPU load from ~100% to about 1–2% without
throwing off the OS-9 clock. The real boot ROM is proprietary and stays
local — `.gitignore` covers `boardrom*.bin`/`*.rom`.

**Windows terminal input:** the native Windows HAL normalizes extended
`_getch()` keys to ANSI sequences so that OS-9 programs like `umacs` can
interpret them via `termcap`. Example: cursor-up arrives from Windows as
`0xE0 0x48`, gets turned into `ESC [ A` by the HAL, and `SYS/termcap`
describes `ku=\E[A` for `q9|q9term` accordingly. `Ctrl-C` is intercepted
by the Windows HAL so it doesn't terminate the host process.

**OS9SYS CF boot and `os9gen`:** the local working image
`local_images/OS9SYS.hda` boots directly from CompactFlash and then runs
the CF `startup`. For bootfile experiments, the formattable descriptor
`/c0_fmt` has to be used; `/c0` is format-protected and produces
`E$Format` (`000:255`) on the final rename from `OS9Boot`. Details and
working rules: [`BOARD.md`](BOARD.md), section "Findings from production
CF boot".

**Toolshed/WSL for images:** RBF/OS-9 images are edited locally via
Toolshed in Debian WSL (`~/.local/bin/os9`). Don't write to the same
`.hda` simultaneously from Toolshed and from the running emulator.

---

### 5.5 QUICC Ethernet Emulation (`src/devices/quicc/quicc.c/.h`)

Details: the source code itself (`quicc.c/.h`) and the network-backend
choice in section 5.8. QUICC is a CB030/board-specific Ethernet
controller (a Motorola communications processor) — the emulation is
enough for OS-9's `enet0` driver, without reproducing the full QUICC
instruction set.

### 5.6 Device Registry (`src/kernel/devreg.c/.h`, Decision E14)

Before the registry, board peripheral devices were wired up as six
hardcoded if/switch chains in three places (memory dispatch in
`m68krt.c`, main-loop polling in `q9boardrun.c`, IRQ ack/reassert in
`m68krt.c`). `devreg.h` bundles this into a generic device interface:
`q9_device_t` carries an address window (base/size), IRQ assignment
(level/vector), and a vtable with the access functions (`read8/write8`
mandatory, 16/32-bit as well as `poll`/`irq_pending`/`reset` optional). A
static registry (`q9_devreg_add/get/count`) holds the instances; a
separate, likewise statically compiled type registry
(`q9_devtype_lookup`) maps type names ("duart68681", "cf", ...) to their
vtable — the basis for the board config file (section 5.7) being able to
instantiate devices by type name in the future.

### 5.7 Board Config File (`src/kernel/boardcfg.c/.h`)

The emulator accepts a **board config file as its first positional
parameter (without a leading `-`)**; if the extension is missing, `.q9`
is assumed:

```sh
./build/<platform>/q9.exe mysystem            # loads mysystem.q9
./build/<platform>/q9.exe mysystem.q9 --net vmnet
```

The existing CLI options remain unchanged and **override** the config
(precedence: built-in defaults < config file < CLI). Without a config AND
without `--rom`, startup aborts with a usage message (section 4.1). Format
(INI-style, a C99 parser with no third-party library, comments `;`/`#`,
paths **relative to the config file**):

```ini
[board]
name = Q9-Board
rom  = roms/romimage.dev.running.BIN     ; boot ROM (instead of --rom)
net  = nat                               ; nat | vmnet | bridge:<ifname> | slirp
; Only for net = vmnet/slirp: central address data for the virtual Q9 network
vmnet_ip       = 192.168.200.2            ; static guest IP in OS-9
vmnet_gateway  = 192.168.200.1            ; shared-mode gateway
vmnet_netmask  = 255.255.255.0
vmnet_dhcp_end = 192.168.200.254

[cf0]                                     ; any number of [cfN] sections
type  = rbf                              ; rbf (OS-9 RBF) | pcf (FAT12/16)
bus   = onboard                          ; onboard ($FFFFE000) | rc2014 ($FFFFC010)
unit  = master                          ; master | slave
image = OS9SYS.hda

[cf1]
type  = pcf
bus   = rc2014
unit  = slave
image = q9-fat16.img
```

The Compact Flash emulation is **multiply instantiable**: CF state lives
in its own type `q9_cf_t` (with two `q9_cf_unit_t`, master/slave). This
gives two CF **interfaces**: the onboard CF (`$FFFFE000`, descriptors
`c0..c3`) and the **RC2014 SC145 secondary interface** (`$FFFFC010`,
descriptors `e0`/`f0`). Which unit serves a given ATA command is
decided — as on real ATA hardware — by the DEV bit (bit 4) in the LBA3
register. The `type` key controls the sector-size heuristic: `rbf` (or
`auto`) recognizes old 256-byte-LSN images by LSN0; `pcf` **disables this
heuristic** (otherwise the FAT boot sector would be misread as an OS-9
LSN0) and treats the image as plain 512-byte sectors.

Test images are created with ToolShed (RBF: `os9 format -bs512 -c32 …`)
or `tools/make_fat_image.py` (FAT12/16 superfloppy). Example config:
[`q9board.example.q9`](q9board.example.q9).

### 5.8 Other Subsystems (Brief Overview)

This handbook's content was last updated on 2026-07-16 (through step
5.19a); a fair amount has been added to the emulator since then, which is
only briefly named here — full details are in `.claude/ARBEITSPLAN.md`:

| Subsystem | Brief description | ARBEITSPLAN area |
|---|---|---|
| Network backends | `--net nat` (built-in mini-NAT) · `vmnet`/`bridge:<if>` (macOS, real network) · `slirp` (cross-platform, `libslirp`) incl. `net_hostfwd` (host→guest port forwarding, e.g. Telnet) | Steps 5.10–5.16 |
| Telnet terminals | Eight virtual network terminals `/x1`–`/x8` over TCP (port 2000+), Telnet NVT normalization | Step 5.10 (extension to 8 channels) |
| Video pipeline | MC6845 register model (`src/devices/mc6845/`) + framebuffer/VRAM (`src/devices/framebuf/`) + CLUT color palette (`src/devices/clut/`) + Q9-Frame network protocol for remote display (`src/devices/videobridge/`) | Steps 5.24–5.29 |
| RTC72421 | Real-time clock (Epson chip), reads the host clock | Step 5.6 |
| Debug hotkey | Ctrl-^ dumps physical RAM content (a Q9-OS reverse-engineering aid) | see `q9_dbg_dump_requested` in `q9boardrun.h` |
| Multi-architecture planning | RISC-V32/ARM64/x86 32-bit as further target architectures, board emulator vs. native runtime — still pure planning | Phase 6 |

---

## 6. Current Status

Complete, fine-grained status with rationale:
[`../.claude/ARBEITSPLAN.md`](../.claude/ARBEITSPLAN.md)
(status model 💡/💤/🟢/🔄/✅/⛔). Summary:

| Phase | Contents | Status |
|-------|--------|-------|
| 0–4 | Mini-kernel: foundation, syscall dispatcher, module system, filesystem (FAT16), processes, WASM runtime | ✅ historically complete, but **no longer part of this source tree** — untouched since 2026-07-04, moved out to `Q9RESUME-Kernel` (see README.md "History") |
| 5 | 68k board emulator (Musashi) | 🔄 very far along — de facto the current core of the project. Fully boots real, unmodified OS-9/68K: Compact Flash (RBF/PCF), DUART, Telnet terminals, networking (nat/vmnet/bridge/slirp), framebuffer/video (MC6845/CLUT/Q9-Frame), device registry, board config files. Fine-grained details: ARBEITSPLAN.md steps 5.1–5.32+ |
| 6 | Multiple target architectures (RISC-V32, ARM64, x86 32-bit) | 💡 in planning (2026-08-11), structure (board emulator vs. native runtime) discussed, not yet implemented |
| U | Userland tools | 🔄 several Q9 userland tools exist (Codex's work area, `userland/`) |

Testing happens natively on Windows/macOS/Linux at every step — see
`test/` and the verification notes in `.claude/ARBEITSPLAN.md`.

---

## 7. Reference Sources and Their Licenses

Q9 is developed independently, but takes its inspiration from OS-9. The
following sources serve as technical reference material — with differing
legal status:

| Source | Type | License | Use in Q9 | Redistributable? |
|--------|-----|--------|-------------------|----------------|
| **MWOS SDK** (Microware OS-9, private copy) | original headers/definitions (`funcs.h`, `errno.h`, ...) | proprietary, private | reference for syscall numbers, error codes, register conventions of the emulated guest OS-9, as well as addresses/descriptors for the board (CB030-Q9 port) | **No** — used only for cross-checking, no file from it is in the repo |
| **OS-9 Insights** (Peter Dibble) | technical book, an older edition includes a printed FAT16 file manager | book copyright | design reference, historical (mini-kernel era) | **No** — only read/understood, no code taken over |
| **NitrOS-9** ([github.com/nitros9project/nitros9](https://github.com/nitros9project/nitros9)) | community OS-9/6809, RBF in 6809 assembler | **GPL** | so far only noted as an idea (idea backlog, ARBEITSPLAN.md) | Yes (GPL, mind the copyleft) |
| **ToolShed** (part of the NitrOS-9 project) | PC tools for reading/writing RBF images, in C | presumably GPL (needs checking in the NitrOS-9 context) | actively used for image editing (WSL/Toolshed, see section 5.4) | to be checked |
| **OS9exec** (Lukas Zeller/Beat Forster) | 68k emulator + an OS-9 kernel reimplementation in C, syscall level | **GPL** | so far only noted as an idea: reference for future syscall-bridge questions | Yes (GPL, mind the copyleft) |
| **Musashi** ([github.com/kstenerud/Musashi](https://github.com/kstenerud/Musashi), commit `313ebf1`) | 68000/68030 emulator, C | **MIT** | only the core interpreter + code generator + softfloat vendored as `third_party/musashi/` (no disassembler, no test drivers), built as part of `make native` (decision E12) | Yes (MIT, license text in `m68k.h` among others) |
| **libslirp** | user-mode network stack, C | **BSD-2-Clause** | `--net slirp` backend: vendored for Windows under `third_party/slirp/windows/`, linked against the system libslirp via pkg-config on macOS/Linux | Yes |

**Consequence for a future release:** Q9's own source tree (`src/`,
`tools/`, `test/`) contains no lines from any of the GPL/proprietary
sources above — everything is an independent implementation based on the
team's own understanding of the concepts. The third-party sources actually
taken over are **Musashi** under `third_party/musashi/` and **libslirp**
(MIT resp. BSD-2-Clause, both unmodified) — compatible with any future Q9
license choice. Should GPL code from NitrOS-9/OS9exec be taken over after
all at some point, Q9's license (or at least that of the affected modules)
will have to be GPL-compatible. The MWOS and book references must never
show up as code either way — they may only serve as background
understanding.

---

## 8. Glossary

A quick reference for terms assumed throughout the project and its
specialist documents:

| Term | Meaning |
|---------|-----------|
| **RBF** | "Random Block File" — OS-9's native filesystem/file manager, a CF image format in the board configuration |
| **PCF** | in the Q9-board context: a FAT12/16-formatted CF image (the counterpart to RBF), directly mountable from Windows/macOS |
| **DUART** | Dual UART — here the emulated Motorola 68681, providing the board's serial console |
| **QUICC** | a Motorola communications processor with, among other things, an Ethernet MAC — here the board's emulated network interface |
| **CLUT** | Color Look-Up Table — the color palette between framebuffer index values and actual RGB values |
| **F$...** / **I$...** | the OS-9 syscall naming convention of the GUEST operating system: `F$` = system function (e.g. F$Fork), `I$` = I/O operation (e.g. I$Read) — Q9 Flux doesn't implement these itself, it emulates the hardware that the real OS-9 executes them on |
| **Descriptor** | an OS-9 term for a management structure (path descriptor, device descriptor) — in the board context, e.g. the CF descriptors `c0`/`e0`/`f0` |
| **Superfloppy** | a media image with no partition table — boot sector directly at block 0 |
| **Musashi** | the vendored 68000/68030 CPU emulator (MIT, `third_party/musashi/`), handling the CPU side of the board emulation in the native build (decision E12) |
| **Reset vector** | the first 8 bytes of the 68k address space: the initial stack pointer (address 0) + the initial program counter (address 4), each 4 bytes, big-endian |
| **Board runner** | `q9boardrun.c`, the main loop that brings together and runs Musashi + board + devices |
| **HAL** | Hardware Abstraction Layer — here: the abstraction of the host machine (console/timer/sockets), NOT of the emulated guest hardware (that's the device registry, see section 5.6) |

---

**Created**: 2026-07-04
**Last updated**: 2026-08-12 (major cleanup, see edition history above)

#─────────────────────────────────────────────────────────────────────────────────────────────────
# EOF HANDBOOK.md                                                                        Ver. 3.00
#─────────────────────────────────────────────────────────────────────────────────────────────────
