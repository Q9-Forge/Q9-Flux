# 68k ROF and linker reference

This document is intentionally retained in the x86 project.  The 68k pipeline
is Q9's concrete proof that an open compiler-to-native-module route can be
built incrementally and verified against original tools.  It is the preferred
methodological reference for a later x86 module packer and for the remaining
68k self-hosting work.

## Working Q9 68k pipeline

```text
QCC / qcpp
  -> 68k assembly (.a)
  -> qr68
  -> Microware ROF relocatable object (.r)
  -> l68
  -> loadable OS-9/68k module
```

`Q9-qr68` replaces Microware `r68`.  Its tested outputs are byte-identical to
`r68` (including complete QCC modules and Q9-OS kernel assembly, apart from
the controlled timestamp field).  `l68` remains the required final linker in
the present pipeline.

Authoritative current implementation and format notes:

- `../Q9-qr68/README.md`
- `../Q9-qr68/src/qr68.c`
- `../Q9-qr68/tools/rofcmp.py`

## ROF facts verified against r68

- ROF starts with sync `$DEADFACE` and has a 56-byte **big-endian** header.
- The header records module type/language, attributes/revision, assembler
  metadata, edition, static storage, initialized data, code, stack, entry,
  trap, remote data and debug sizes.
- It is followed by the NUL-terminated psect name, globals, code,
  initialized data, external references, local references, and trailing
  fields.
- Global and external-name counts are 32-bit, despite an older disassembler
  source reading them as 16-bit values.
- Globals and external names are alphabetically sorted by `r68`; references
  for an external are offset-sorted.
- Code is padded according to the observed `r68` rules, including NOP padding
  to a four-byte multiple.
- Initialized and uninitialized `vsect` data occupy separate address spaces,
  both starting at zero.
- Reference type words encode location (code/data), width, PC relativity,
  subtraction and target section.  Only combinations measured against `r68`
  should be emitted.

The complete measured field table and the evidence for each rule are in
`Q9-qr68/README.md`; do not reproduce them from memory when implementing a
linker.

## Lessons for a future Q9 linker

1. Treat the original linker/assembler as an oracle.  Generate focused probes,
   compare byte-for-byte, and document each discrepancy.
2. Start with a narrow, explicit relocation subset.  A rejected relocation is
   safer than a module that loads at an incorrect address.
3. Keep output deterministic; isolate timestamps and other unavoidable
   metadata so byte-comparison remains useful.
4. Test both structural output and a live load/run on the target OS.
5. Keep assembler object format, final module format, and loader conventions
   separate.  ROF is an intermediate 68k object format, not ELF and not the
   final module.

## Important x86 distinction

The 68k ROF layout cannot be reused byte-for-byte for OS-9000/x86.  The latter
uses a separate little-endian module and relocation format.  What transfers is
the engineering pattern:

```text
compiler object -> documented relocation transform -> native module ->
structural comparison -> live boot/load/run test
```

For 68k, the next open replacement after `qr68` is the `l68`-equivalent final
linker.  Its specification should be derived from ROF inputs and original
module outputs using the same probe-driven method.
