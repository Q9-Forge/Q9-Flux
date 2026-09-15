# QEMU's device model

*German version: [QEMU_DEVICE_MODEL_de.md](QEMU_DEVICE_MODEL_de.md)*

Short overview of QEMU's building blocks, as orientation for the planned
migration from Musashi to QEMU.

## QOM (QEMU Object Model)

QEMU's own object-oriented framework, built in C (not C++). Every device
type is a `TypeInfo` struct with `class_init`/`instance_init` functions,
a parent type (inheritance), and instance/class struct sizes. Virtual
methods go through function-pointer vtables in the class struct —
essentially C++ inheritance hand-rolled in C.

## qdev (device model)

Built on top of QOM, specifically for hardware devices. Devices inherit
from `DeviceState`, declare **properties** (configurable parameters —
this is where a host path for the planned
[hostfs manager](HOSTFS_MANAGER.md) would fit), and have a `realize()`
function that runs once all properties are set (essentially the actual
constructor).

## MemoryRegion API

The actual core mechanism for MMIO access: a device creates a
`MemoryRegion`, gives it read/write callback functions
(`MemoryRegionOps`, taking address + size), and the region then gets
mapped into the guest's address space. When the CPU accesses an address
within that region, QEMU automatically invokes the registered callback.

Functionally the same principle as the current Musashi trap-and-emulate
approach (`src/devices/` + the `devreg`/`devdesc` registry), just cast
into a formalized API instead of hand-written.

## Board/machine file

`hw/<arch>/<board>.c` instantiates and wires together all devices for a
specific board — matches the role `q9board.c`/`boardcfg.c` play for us
today.

## Already-installed m68k machines (as of 2026-09-15, QEMU 11.1.1)

```
an5206               Arnewsh 5206
mcf5208evb           MCF5208EVB (default)
next-cube            NeXT Cube
q800                 Macintosh Quadra 800
virt                 QEMU M68K Virtual Machine
```

None of these match our own CB030/Vinculum target board — a custom QEMU
machine type will be needed, analogous to `q9board.c` in the Musashi
branch.

## Two existing QEMU features as reference points

Before the hostfs manager designs its own protocol, it's worth looking
at two existing, thematically very close QEMU building blocks — not to
copy code (QEMU is GPL-licensed), but to spot early what our own
planning might be missing:

- **`vvfat`** — a QEMU block device that presents a host directory as a
  virtual FAT image, purely in software, without a real image file.
  Conceptually related, just at the FAT level instead of a custom
  protocol.
- **`virtio-9p`** — QEMU's standard mechanism for host folder sharing in
  "real" VMs: a guest-side 9P protocol driver talks over a virtio queue
  to the host, which performs the actual open/read/write/readdir calls.
  At its core, exactly our own manager/driver concept — already fully
  thought through and proven in the field.
