# Research status: x86 Q9-Flux and OS-9000

Last reviewed: 2026-09-03

## Established facts

1. **The existing OS-9000/x86 images already boot with stock QEMU.**
   Q9-OS contains repeatable evidence for OS-9000 U4.9 and OS-9 6.1.  The
   4.9 image boots on a 32-bit PC configuration with IDE storage and standard
   VGA.  The 6.1 appliance likewise boots as a pure 32-bit guest; its OVF does
   not request PAE or long mode.

   Primary project evidence:

   - `../Q9-OS/modules/os9000-x86/docs/FINDINGS.md`
   - `../Q9-OS/modules/os9000-x86-6.1/docs/FINDINGS.md`

2. **Therefore the first deliverable is not a new CPU emulator.**  QEMU is the
   working hardware reference for the required PC devices: CPU, memory, IDE,
   VGA, timer/interrupt controller and NIC.  The initial Q9-Flux-x86 work is a
   reproducible profile and an architecture-neutral hardware-simulation
   boundary compatible with the direction of Q9-Flux.

3. **The first guest is i386 protected mode.**  This lets us use the available
   images immediately and avoids an unnecessary long-mode bootstrap.  The
   subsequent x86-64 platform must explicitly preserve real mode / 16-bit and
   protected-mode / 32-bit execution where the selected CPU backend supports
   them.

4. **OS-9000/x86 executable modules are not ELF.**  Local reverse-engineering
   shows a proprietary little-endian module header and relocation scheme.  A
   generic `i686-elf-gcc` or Clang can produce useful machine code, but cannot
   alone create a loadable OS-9000 module.

   Primary project evidence:

   - `../Q9-OS/modules/os9000-x86/docs/KERNEL_INIT.md`
   - `../Q9-OS/modules/os9000-x86/docs/FINDINGS.md`

## Original tooling found locally

The official OS-9000 evaluation CD archive contains executable commands named
`cc`, `make`, `link`, `libgen`, and `csl`.  Strings in `cc` include `_UCC` and
`be386`, consistent with the historical Ultra C/C++ compiler and its x86
backend.

However the CD's `RESIDENT/HOWTO.TXT` explicitly says that its full
`mw86res.tar` package (sources + Ultra C++ compiler) is **not present in the
evaluation version**.  The commands in the evaluation archive must therefore
not be treated as proof that all compiler phases, headers, and libraries are
available.  A complete, lawfully held development installation would be the
best way to build native OS-9000/x86 applications or drivers.

## Existing Q9 assets

| Asset | What it does | Relevance |
|---|---|---|
| `Q9-Tools/System/qid` | Identifies OS-9 modules, ROF objects, and ELF32/ELF64 files | Parser/reference only; it is not a converter. |
| `Q9-qr68` | Assembles 68k source to Microware-compatible ROF, byte-identical to `r68` for the tested corpus | Strong model for a future open Q9 linker path. |
| Q9-QCC | Produces 68k assembly and already runs on genuine OS-9/68k | The compiler architecture is reusable, but the x86 backend is a separate task. |
| Q9-OS x86 research | Documents original module format and live QEMU boot behaviour | Primary compatibility oracle for any x86 module packer. |

## Recommended sequence

1. Add a checked-in, image-free QEMU smoke-test specification: machine type,
   RAM, IDE drives, VGA, timer/interrupt, NIC, expected boot markers.
2. Design a small architecture-neutral device interface for Q9-Flux-x86.
   Existing Flux's 68k device registry is explicitly big-endian, so it cannot
   be copied unchanged; x86 accesses must use a neutral byte/bus contract.
3. Locate or inventory a complete original Ultra-C installation.  If found,
   compile and run one minimal native OS-9000 program before attempting any
   new toolchain.
4. If the original toolchain is unavailable, build an **ELF relocatable object
   to OS-9000/x86 module packer** around `i686-elf-gcc` or Clang.  Start with
   static, minimal modules and use observed original modules as test vectors.
5. Only after this path is proven, decide whether a custom portable x86 CPU
   backend brings enough value to replace QEMU for a given use case.

## Non-goals for the first milestone

- No x86-64 guest kernel port.
- No attempt to rebuild proprietary OS-9000 kernel sources.
- No embedding or redistribution of proprietary images/toolchains.
- No claim that an ELF file can simply be wrapped in an OS-9000 header; the
  relocation and ABI work is essential.
