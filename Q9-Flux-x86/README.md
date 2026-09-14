# Q9-Flux-x86

Research and implementation project for an x86 Q9-Flux platform.

The first guest target is **32-bit x86 protected mode**.  This is deliberate:
the existing OS-9000 images are 32-bit guests, and their successful QEMU boots
give this project an immediately testable hardware baseline.  A later 64-bit
platform must keep 16- and 32-bit guest execution as explicit compatibility
modes; it is not the first bootstrap target.

## Current decision

Use QEMU's i386 system emulator as the reference implementation and first
execution platform.  Do not begin by writing a new x86 CPU emulator.  The
initial project work is to specify a Q9-Flux-compatible device boundary and to
make a repeatable OS-9000 boot test.  A native Flux CPU backend remains a
possible later implementation, provided it conforms to the same boundary.

The first runnable deliverable is `bin/flux-x86`: a Q9-Flux-x86 launcher
that owns the stable machine profile while delegating instruction execution and
standard PC devices to QEMU.  It deliberately accepts an image path at run
time; no proprietary guest image is stored in this repository.

## Documentation

- [Research status](docs/RESEARCH_STATUS.md): verified OS-9000/QEMU evidence,
  existing tools, limits, and recommended sequence.
- [OS-9000 x86 module path](docs/OS9000_X86_MODULE_PATH.md): what is required
  to turn compiler output into a native OS-9000/x86 module.
- [Toolchain research](report-source.md): historical Microware and GNU
  `i386os9k` findings, plus the ELF32 conversion plan.
- [68k ROF and linker reference](docs/68K_ROF_LINKER_REFERENCE.md): the
  verified Q9 68k route, retained as the model for a future self-hosted linker.
- [Emulator architecture](docs/EMULATOR_ARCHITECTURE.md): the QEMU integration
  boundary and the path to Q9-specific devices.
- [Boot smoke tests](docs/BOOT_SMOKE_TESTS.md): verified XiBase9 boot and the
  current, driver-limited OS-9000 network baseline.

## First boot

```sh
bin/flux-x86 --config profiles/os9000-4.9.conf
```

Use `--headless` for an automated boot run.  `--help` lists the deliberately
small initial option set.  Profiles are text files and may set the PC machine,
RAM, disk, VGA display, NIC model and networking mode.  Command-line options
override the selected profile.

The default QEMU user network is a `/16`: the guest receives
`192.168.77.15/16` and sees `192.168.123.250` as its gateway.  These values
are configurable for other test networks:

```sh
bin/flux-x86 --config profiles/os9000-4.9.conf \
  --network-net 192.168.0.0/16 \
  --network-host 192.168.123.250 \
  --dhcp-start 192.168.77.15
```

`--transfer-disk PATH` attaches a host-created FAT image as the secondary IDE
drive.  It is the intended bootstrap path for guest files once the OS-9000
image contains its PCF file manager and descriptor.  The current XiBase image
does not yet include those modules; see the smoke-test record.

For an image patched with `tools/patch_x86_sysboot.py` and a corrected
`/SYS/startup`, networking starts internally during the normal OS-9000 boot;
no host-side command is required.  The post-boot helper remains available as a
fallback for the unpatched vendor image or for diagnostics:

```sh
python3 tools/qmp_network_bootstrap.py /tmp/flux-x86.qmp
```

The helper types the same tested `ndbmod` rebind and background DHCP sequence
into the guest console.  It does not modify the guest image.

The first ELF32/i386 toolchain milestone is the read-only inspector:

```sh
python3 tools/elf_os9k.py path/to/object.o
```

It validates relocatable ELF32/i386 input and lists sections, symbols and
relocations.  The experimental writer currently emits only a no-relocation
candidate module; external symbols and relocation records are rejected until
their OS-9000 mapping has been validated against real modules.

## Third-party components

QEMU (`third_party/qemu`, a git submodule pointing at the unmodified
upstream project) is used as an external i386 system emulator process,
launched and driven via QMP/the command line — its own code is not
compiled into or linked with `bin/flux-x86`. QEMU itself is
GPL-2.0-or-later; see [../THIRD-PARTY-LICENSES.md](../THIRD-PARTY-LICENSES.md)
for the full third-party license overview.

## Scope boundary

This repository must not contain proprietary OS-9000 images, extracted binary
modules, or Microware compiler components.  They are test inputs held outside
the repository.  The project may contain scripts and documentation that refer
to user-provided local paths.
