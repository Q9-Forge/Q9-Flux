# From an i386 compiler to an OS-9000/x86 module

## The practical route

```text
C source
  -> i686-elf GCC or Clang
  -> ELF relocatable object (.o)
  -> Q9 OS-9000/x86 module packer
  -> OS-9000/x86 executable or driver module
```

This route is attractive because the compiler and code generator already
exist.  It does **not** remove the need for target work.  The packer must
understand both ELF relocations and the OS-9000/x86 loader's relocation and
module conventions.

## Required compatibility work

1. Read ELF32/i386 sections, symbols, alignment, and relocation records.
2. Select an initial supported relocation subset and reject unsupported records
   loudly.  Do not silently emit a non-loadable module.
3. Lay out text, initialized data, zero-filled/static storage, symbol imports,
   and relocation records in the observed OS-9000/x86 module layout.
4. Emit a valid module header, name, entry point, edition and integrity data.
5. Supply the appropriate startup object and a small runtime.  The compiler's
   normal hosted C runtime is not an OS-9000 runtime.
6. Follow the OS-9000/x86 calling convention and system-service ABI, including
   any registers reserved by the original toolchain.
7. Validate with original module samples, then with an actual QEMU boot and a
   deliberately tiny test module.

## Why a loader alone is not enough

An ELF loader loads a program for the environment it implements.  OS-9000's
native loader expects its own module header and relocation representation.
For a usable compiler path we need a **host-side converter/packer**, not just
an ELF parser inside the guest.

## Where the format evidence belongs

The canonical current reverse-engineering notes are kept in Q9-OS:

- `../Q9-OS/modules/os9000-x86/docs/KERNEL_INIT.md`
- `../Q9-OS/modules/os9000-x86/docs/FINDINGS.md`

When a field is inferred or tested, record its source module, offset, and boot
test here or in a dedicated format specification before depending on it.
