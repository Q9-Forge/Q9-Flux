# Q9-Flux 68k — QEMU variant

*German version: [README_de.md](README_de.md)*

This directory is the starting point for a future migration of the 68K
emulator core from [Musashi](../Q9-Flux-68k/third_party/musashi/) to
[QEMU](https://www.qemu.org/). **The existing Musashi-based emulator
under [`Q9-Flux-68k/`](../Q9-Flux-68k/) remains fully usable and is not
touched by this directory.** Both branches exist in parallel until the
QEMU variant is far enough along to replace the Musashi branch — or the
two continue to coexist long-term.

## Why QEMU

QEMU brings its own mature device model (QOM/qdev, the `MemoryRegion`
API) that looks better suited for some planned extensions — most notably
the [host-passthrough filesystem manager](docs/HOSTFS_MANAGER.md) — than
the current, hand-written `src/devices/` pattern built around Musashi.
Details on QEMU's device model and on already-existing, thematically
related QEMU building blocks (`vvfat`, `virtio-9p`) are in
[`docs/QEMU_DEVICE_MODEL.md`](docs/QEMU_DEVICE_MODEL.md).

## Status (2026-09-15)

First real milestone reached: a custom `q9board` QEMU machine (CPU + RAM
skeleton, no peripherals yet) has been built from source and verified —
a hand-crafted 68030 test program (a few `moveq`/`add` instructions plus
`stop`) loaded via `-kernel` executed correctly, confirmed via the QEMU
monitor (`info registers`: `D0=0x2b` (43, the expected sum), `SR=0x2700`
matching the `stop` operand, `PC` exactly past the last instruction).

The source is properly versioned now, split across a dedicated QEMU
submodule (`third_party/qemu/`, separate from Q9-Flux-x86's own copy —
that one stays a pristine, unmodified upstream checkout) and our own
additions on top, applied by `setup-qemu-dev-tree.sh`:

- `overlay/new-files/hw/m68k/q9board.c` — the machine itself
- `overlay/patches/0001-add-q9board-machine.patch` — the two-line
  `Kconfig`/`meson.build` registration, as a proper diff so future QEMU
  version bumps don't silently drop unrelated upstream additions to
  those files

Run `./setup-qemu-dev-tree.sh` after cloning (or after any
`git submodule update`, which resets the submodule and would otherwise
wipe the overlay), then build with
`cd third_party/qemu && mkdir build-m68k && cd build-m68k && ../configure --target-list=m68k-softmmu && ninja`.
Verified end-to-end twice (reset → script → rebuild → boot) with
identical, correct results.

QEMU itself is installed and tested on the development machine (macOS,
Apple Silicon): `qemu-system-m68k`, version 11.1.1, including the
standard m68k machines `an5206`, `mcf5208evb`, `next-cube`, `q800`,
`virt` (none of which match our own CB030/Vinculum target board — hence
the new `q9board` skeleton).

**Next step** (large, multi-session): port the real device models —
CF, QUICC, RTC72421, 68681 DUART, timer IRQ3, the remap trigger, nettty,
MC6845/framebuf/CLUT/videobridge — from `Q9-Flux-68k/src/devices/` to
QEMU's QOM/`MemoryRegion` pattern, one device at a time.

## Installing QEMU

### macOS

```sh
brew install qemu
```

Installs all QEMU target architectures, including `qemu-system-m68k`.
Update with `brew upgrade qemu`.

### Windows 11

Official-ish installer builds: <https://qemu.weilnetz.de/> (unofficial
but well-established Windows builds), or via a package manager:

```powershell
winget install qemu
```

### Linux

Package name varies by distribution, usually `qemu-system-m68k` or the
combined `qemu-system` package:

```sh
# Debian/Ubuntu
sudo apt install qemu-system-m68k

# Fedora
sudo dnf install qemu-system-m68k

# Arch
sudo pacman -S qemu-system-m68k
```

## Further documents

- [`docs/QEMU_DEVICE_MODEL.md`](docs/QEMU_DEVICE_MODEL.md) — QOM/qdev,
  the `MemoryRegion` API, reference points `vvfat`/`virtio-9p`
- [`docs/HOSTFS_MANAGER.md`](docs/HOSTFS_MANAGER.md) — the full
  architecture design for the planned host-passthrough filesystem
  manager (Manager/Driver/Descriptor split, the 13 standard entry
  points, concurrency, attribute handling, cross-platform pitfalls)
