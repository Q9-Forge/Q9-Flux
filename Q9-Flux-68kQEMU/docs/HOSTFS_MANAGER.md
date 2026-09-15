# Host-passthrough filesystem manager — architecture design

*German version: [HOSTFS_MANAGER_de.md](HOSTFS_MANAGER_de.md)*

Emerged on 2026-09-15 from the question of how to avoid the RBF
conversion hassle during testing. **Not started yet** — a pure
architecture design, written down for a later implementation session,
after the [Musashi→QEMU migration](../README.md).

## Why not RBF/PCF/NFS/FTP/Samba

RBF (OS-9's native filesystem) requires dedicated conversion tools
because its on-disk format (LSN/allocation map) isn't understood
directly by any host tool. PCF (reads/writes FAT) was attempted in the
past and never got working. NFS, FTP and Samba were also all attempted
for Q9-Flux before — none worked (marked with "?" in
`.github/ROADMAP.md`/`ROADMAP_de.md`: "FTP support, issue (FTP still
uses the old TCP socket path)" / "NFS support, issue" / "Samba support,
issue"). This matches the known `telnetd` hang finding (missing
`SS_SEvent` in the SCF driver, documented in
`Q9-Flux-68k/docs/ARBEITSPLAN.md`) — the TCP/network application-layer
path on the emulated OS-9 seems fundamentally fragile.
**Recommendation: don't pursue NFS/FTP/Samba further.**

## The core idea: a custom manager, not a new network protocol

OS-9 layering is Manager → Driver → Descriptor. RBF and PCF are both
Managers (standard filesystem API) that internally decode an on-disk
format. A new Q9 manager would serve the same standard interface, but
without its own format parsing — it passes calls through to a driver,
which translates them into a simple command protocol ("open path X",
"read N bytes", "list directory").

### Legal note

The book *OS-9 Insights, 3rd Edition* (Microware) contains, in appendix
C ("Building a File Manager", starting page 519), a complete PC-DOS-
compatible file manager in source code. It carries an explicit
copyright notice ("proprietary confidential property of Microware...
Reproduction, publication, or distribution... strictly prohibited").
Copyright also protects structure/organization, not just the literal
text — rewording/obfuscating the code does not change its nature as a
derivative work. **The book may serve as a private learning resource
for the interface the OS-9 kernel dictates anyway (and which is
officially documented, not proprietary) — the actual manager must be
written from scratch, not derived from the book's code.**

## The 13 standard manager entry points

The official OS-9 kernel jump table (a pure interface convention, not
proprietary):

| # | Function | Purpose |
|---|---|---|
| 1 | `Create` | create a new file |
| 2 | `Open` | open an existing path |
| 3 | `MakDir` | create a new directory |
| 4 | `ChgDir` | change the current default directory |
| 5 | `Delete` | delete a file/directory |
| 6 | `Seek` | set the byte position of a path |
| 7 | `Read` | read bytes |
| 8 | `Write` | write bytes |
| 9 | `ReadLn` | read a line (until CR or a count limit) |
| 10 | `WriteLn` | write a line |
| 11 | `GetStat` | query status/extended info |
| 12 | `SetStat` | set status/extended info |
| 13 | `Close` | close a path |

Entry convention: `a1` points to the Path Descriptor, `a4` to the
Process Descriptor, `a5` to the user's register set, `a6` to the system
global area.

For the passthrough manager, most of these are just thin translators
that pack the call plus path/data pointer into the GetStat/SetStat
command protocol to the driver. `GetStat`/`SetStat` themselves become
the most interesting part.

## Directory listing: synthetic RBF records (option A)

For standard OS-9 tools (`dir`, `copy`, `list`, `del`, `makdir`) to work
unmodified, the manager must, on `I$Read` against an open directory
path, internally fetch the next real host directory entry from the
driver via GetStat/SetStat and build a synthetic RBF-like directory
record from it. From the outside everything looks like a normal
directory; the GetStat channel stays entirely internal to
manager↔driver communication.

(An alternative "option B" — custom tools instead of standard commands
— was considered and rejected, since standard-tool compatibility is
the actual goal.)

## Multiple drives / host path mapping

The host path (where the respective virtual drive begins) lives as an
additional descriptor field (options area, analogous to
`descriptor`/`descriptorName` in `devschema.h`). Multiple drives = 
multiple descriptor/driver pairs, each with its own host path and MMIO
base address — exactly the existing `cf`/`nettty` pattern (`devreg`/
`devdesc` registry), no new mechanism in the kernel/registry needed.

**Important for portability:** the host path should be resolvable
relative to the config file, analogous to `cfg_resolve_rel()`/
`cfg_relativize()` for ROM paths in `boardcfg.c` — otherwise a `.q9`
config's portability breaks on another machine/directory (an absolute
path in the config would resolve to nothing there).

## Concurrency

Not automatically solved by the kernel/manager framework — every
manager (RBF included) must protect shared resources itself. Path
Descriptors are separate per process (safe), but the shared device
static storage (host path, MMIO command channel) is not.

**Solution:** `F$Event`/`Ev$Wait`/`Ev$Signal` as a mutex around the
host round-trip (the same primitive as the `telnetd` SS_SEvent finding).
No interrupt masking, since the round-trip takes "real time" — other
processes should keep running in the meantime.

File/record locking (two processes, the same file) is deliberately
deferred — even the book's PCFM doesn't have it.

### Directory watching as a locking substitute: evaluated and rejected

The idea was to watch an entire host directory tree via
FSEvents/inotify/`ReadDirectoryChangesW` and drive a mutex from that.
**Fundamental problem:** directory watching is reactive (reports
changes after they happened, sometimes coalesced/delayed) — a real
mutex needs a before-the-fact promise. It can report a race condition
at best, not prevent one. Also inconsistent across platforms: Linux's
`inotify` has real `IN_OPEN`/`IN_CLOSE` events, while macOS FSEvents and
Windows change notifications essentially only report content changes.

Two problems kept cleanly separate:

1. **Two OS-9 guest processes at once** — already solved via the own
   driver + the `F$Event` mutex above, no host mechanism needed (both
   accesses necessarily go through our own code).
2. **A host program accesses the same file at the same time** (e.g. a
   text editor on the development machine) — the same unsolved problem
   as VirtualBox shared folders/Docker bind mounts/network shares. Real
   OS locks (`flock`/`fcntl`/`LockFileEx`) are only *advisory* — they
   only protect against other programs that actively check the lock.
   **Decision:** accept this as a known, documented limitation,
   optionally take an advisory lock while a path is open. Directory
   watching, at most, as a *diagnostic* feature ("file was changed
   externally, cache may be stale"), not as access protection.

## Attribute handling

### The "Single" bit

OS-9 files have a "Single" attribute (only one concurrent open
allowed). Host filesystems have no equivalent concept — so it doesn't
help directly. But: enforcement in OS-9 isn't magically done by the
kernel or at the media level anyway — it happens in the manager itself.
Even RBF just keeps its own bookkeeping (open file descriptors with a
counter) and refuses a second open when the Single bit is set. Our own
manager therefore only needs to track "is file X open, is Single set?"
in the same shared device static storage already protected by
`F$Event`.

### Where the attribute bits live in real RBF

Not in the directory entry itself — that only contains the filename +
LSN (a pointer to the File Descriptor (FD) sector). The attribute byte
(including Single, Read/Write/Execute/Public/Directory) lives in the FD
sector, together with owner ID, link count, size, segment list and
timestamps — one level of indirection down.

**Practical consequence:** since there are no real LSNs/FD sectors, our
own manager must maintain a synthetic FD-like structure per host file
(attribute byte including the Single flag, size, date) and return it
via the appropriate GetStat calls (for tools like `attr`/`dsave`, which
under real RBF also only go through GetStat, not raw media reads). This
is the same data structure as the open-bookkeeping for Single-bit
enforcement — no extra work, one piece of state serving two purposes.

## Cross-platform requirement: Mac, Linux, Windows

Q9-Flux targets all three platforms anyway. Only the emulator side is
affected — the OS-9 guest side (manager + driver) is emulator-core-
independent 68K code.

- **Case sensitivity**: OS-9 is, like APFS and NTFS, "case-insensitive
  but case-preserving" (`chd cmds` finds a created `CMDS`) — matches
  Mac and Windows automatically. **Only on Linux**, ext4 and friends
  are genuinely case-sensitive — there the driver must implement
  case-insensitive directory lookup itself instead of relying on the
  host filesystem. Unicode normalization also remains a consideration
  (macOS NFD vs. OS-9's simple byte comparison).
- **Windows-specific**: reserved names (`CON`/`PRN`/`AUX`/`NUL`/
  `COM1`–`9`/`LPT1`–`9`) are forbidden, forbidden characters
  (`< > : " | ? *`), the historical 260-character `MAX_PATH` limit, and
  files must always be opened in binary mode (otherwise the Windows CRT
  automatically translates LF↔CRLF and corrupts bytes).
- **Don't reinvent the directory-scanning logic**: `q9_filelist.c`
  (Q9-Flux Editor) already scans name/date/size cross-platform (Mac/
  Linux/Windows) for the file dialog — a possible starting point/
  reference for the host side of the new driver.

## Further points not immediately obvious

- **Prevent path escape**: `../../..` must not lead out of the
  configured host root directory.
- **Filename length**: OS-9 names are classically limited to ~28/29
  characters — cf. the ToolShed 29-character bug from our own project
  history.
- **Attribute bit mapping**: OS-9 Read/Write/Execute/Public/Directory
  vs. macOS permissions — the executable bit was already a real bug
  with ToolShed (`os9 copy -r` doesn't set owner-execute).
- **Timestamp conversion**: OS-9's packed date format vs. Unix
  timestamps.
- **Error code translation**: ENOENT/EACCES/ENOSPC → the matching OS-9
  error codes (`E$PNNF`, `E$FNA`, …), otherwise programs get nonsensical
  error messages.
- **Host file-handle leaks on process abort**: path-close cleanup must
  close the host handle too.
- **Write robustness**: write-through immediately vs. buffering —
  parallels the already-fixed CF write bug (256-byte sectors).
- **Directory listing during concurrent host-side changes.**
- **Missing/invalid host path at init** → a clean attach failure
  instead of a crash.
- **Relevant later if host access ever becomes asynchronous**: as long
  as everything runs synchronously in the CPU-step loop, this is a
  non-issue — for later performance optimizations with background I/O
  (to avoid freezing emulation during a slow host disk operation), the
  emulator side itself would need additional thread-safety.

## Reusability across multiple drivers

Same manager, only the driver + transport protocol changes:

1. **Q9-Flux emulator** (first): driver writes commands into MMIO
   registers (analogous to the existing `src/devices/` pattern: `cf`,
   `quicc`, `rtc72421`, etc.). The new emulator-side device performs
   real host calls behind it (`open()`/`read()`/`opendir()`).
2. **CH375/CH376 chip** (real hardware, later): the chip has FAT12/16/32
   parsing built in already and itself speaks file-level commands (not
   just raw sectors) over UART/SPI/parallel — structurally a 1:1 match
   for the same driver contract. Fitting, e.g., for the Vinculum board
   (our own 68360 hardware) as a simple USB-stick connection without our
   own USB/FAT implementation.
3. **Teensy 4.1 / ESP32 as a coprocessor**: handle FAT via a software
   library on an SD card. Again, just another driver, no new manager.
   Conceivable later even an ESP32 with a WiFi network share behind it.

**Core point:** in the end, the manager only knows a single simple
contract ("ask the driver for file X, get data back"); the "intelligent
partner" behind it (emulator, chip, coprocessor) does the actual
filesystem work.

## QEMU reference points

See [`QEMU_DEVICE_MODEL.md`](QEMU_DEVICE_MODEL.md) — in particular
`vvfat` and `virtio-9p` as existing, thematically related QEMU building
blocks worth looking at before designing our own protocol (as
inspiration, not to copy code — QEMU is GPL-licensed).
