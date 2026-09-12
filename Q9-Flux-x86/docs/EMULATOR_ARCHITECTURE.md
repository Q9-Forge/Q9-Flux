# Emulator architecture

## Decision: QEMU core, Q9-Flux-x86 control plane

Q9-Flux-x86 initially uses the upstream QEMU i386 system emulator for CPU
execution and the standard PC platform.  The project owns the profile,
automation interface, test expectations, and all later Q9-specific devices.

```text
q9flux-x86 launcher
  |-- fixed i386 PC profile
  |-- image and runtime configuration
  |-- QMP endpoint for automation
  '-- QEMU system emulator
        |-- i386 CPU (TCG initially)
        |-- RAM, BIOS, PIC/PIT, PCI
        |-- IDE disk, standard VGA, keyboard
        '-- user-mode network NIC
```

This is an own emulator product and interface, but not a premature rewrite of
QEMU's mature x86 core.

## Why this boundary

- Existing OS-9000/x86 images already boot on this hardware profile.
- QEMU is portable to the development hosts and can use TCG where hardware
  acceleration is unavailable.
- The QMP socket gives the project a stable automation/control interface.
- QEMU source changes are only needed when a new hardware model cannot be
  expressed through existing devices and QMP.

## Initial PC profile

| Capability | Initial implementation |
|---|---|
| CPU | 32-bit i386-compatible PC, QEMU TCG |
| RAM | 64 MiB default, configurable |
| Disk | IDE image supplied by the user |
| Display | standard VGA |
| Timer/interrupts | standard PC PIT/PIC supplied by QEMU |
| Input | standard PC keyboard controller |
| Network | QEMU user-mode networking and default emulated NIC |
| Automation | QMP Unix socket |

## Configuration layers

1. A profile in `profiles/*.conf` fixes a reproducible guest-machine variant.
2. `bin/flux-x86 --config <profile>` loads it.
3. Command-line options override a profile for experiments without editing it.
4. QMP controls the already-running machine but does not redefine its core
   chipset or CPU.

The first bundled profile is `profiles/os9000-4.9.conf`.  Its disk image is
kept in `images/flux-x86.img` locally and is intentionally ignored by Git.

## Evolution path

1. Keep the standard devices until the OS-9000 boot smoke test is repeatable.
2. Define an architecture-neutral Q9 device API at the launcher/control level.
   Do not reuse Flux's current big-endian 68k device-register contract as an
   x86 bus contract.
3. Add Q9-specific devices as QEMU device models when they require MMIO, IRQ,
   DMA, migration state or precise emulated timing.
4. Maintain the QMP adapter so test code and tooling are not coupled to QEMU
   command-line details.
5. Fork/vendor the QEMU source only when such a device is implemented.  QEMU
   is GPLv2; distributed modified binaries must be accompanied by their
   corresponding source and license obligations.

## Non-decision

This does not rule out a future CPU backend shared with Q9-Flux.  It says only
that a working OS-9000 machine and its device contract come first, using QEMU
as the compatibility oracle.
