# TinyEMU RISC-V core — vendor note

*German version: [Q9_VENDOR_de.md](Q9_VENDOR_de.md)*

**Source**: https://github.com/fernandotcl/TinyEMU (commit `56ba49be40a3d24b65bcb3ec2180f2bdddd7b0c3`, 2020-05-25)
**Upstream author**: Fabrice Bellard, maintained fork by Fernando Tarlá Cardoso Lemos
**Licence**: MIT (`MIT-LICENSE.txt`, plus a licence header in every source file)
**Vendored**: 2026-08-12

## What is in here (CPU core only)

Only the RISC-V **CPU core** plus the two support units it needs — deliberately
*not* the surrounding system emulator. Q9-Flux brings its own bus, device model
(`src/devices/<name>/`), board configuration and syscall layer; what was missing
was purely the CPU, in the same role Musashi plays for 68k.

| File | Purpose |
|---|---|
| `riscv_cpu.c`, `riscv_cpu.h` | CPU core + public API |
| `riscv_cpu_priv.h` | internal CPU state |
| `riscv_cpu_template.h` | the decoder, instantiated once per word width |
| `riscv_cpu_fp_template.h` | floating-point instructions, likewise |
| `iomem.c`, `iomem.h` | physical memory map: RAM regions + MMIO callbacks |
| `cutils.c`, `cutils.h` | small helpers used by the above |
| `softfp.c`, `softfp.h`, `softfp_template.h`, `softfp_template_icvt.h` | soft float, needed by the F/D extensions |

**Deliberately not taken**: `riscv_machine.c` (the board), `virtio.c`, `vga.c`,
`ide.c`, `fs_*.c`, `sdl.c`, the network stack — and the entire x86 part
(`x86_cpu.c`, `x86_machine.c`). Those are exactly the parts Q9-Flux already has
in its own form.

## Why this core

Decisive was not the extension list but the **shape**. Musashi is *only* a CPU:
memory access happens through callbacks the host provides. That is why it sits
next to `devreg.h` and `boardcfg` without friction. TinyEMU's core has the same
shape, and `iomem.h` is the whole contract:

```c
cpu_register_ram   (map, addr, size, ...)
cpu_register_device(map, addr, size, opaque, read_func, write_func, ...)
```

Host-provided read/write callbacks with an `opaque` pointer — conceptually what
`devreg.h` already does. The CPU API maps almost one-to-one onto the CPU vtable
planned in ARBEITSPLAN step 6.5:

| planned vtable | TinyEMU | Musashi |
|---|---|---|
| `execute` | `riscv_cpu_interp(s, n_cycles)` | `m68k_execute(n)` |
| `set_irq` | `riscv_cpu_set_mip/reset_mip` | `m68k_set_irq()` |
| `is_stopped` | `riscv_cpu_get_power_down()` | `m68k_is_stopped()` (our addition) |
| `ctx` | `RISCVCPUState*` | global state |

The alternative examined was **RVVM** (C99, actively maintained, more extensions
today, MPL-2). Rejected because it brings a complete device and machine
framework of its own (`rvvm_mmio_dev_t` with a remove/update/reset/suspend/resume
lifecycle). That would stand next to Q9-Flux's own device framework — two
frameworks in one emulator — and 78 C files cannot be reduced to a small core.
**libriscv** was rejected earlier: C++17, and as long as QCC cannot compile C++
that would be a part of the tree we could not translate ourselves.

## Word width is a compile-time switch

`riscv_cpu.c` includes `riscv_cpu_template.h` three times, with `XLEN` 32, 64
and 128. An instruction is written **once**, against the parameterised type
`intx_t`, and then exists in all three widths. `MAX_XLEN` selects how much gets
built:

| `MAX_XLEN` | `riscv_cpu.o` | for |
|---|---:|---|
| 32 | 40.6 KB | RV32 only — the small build for later projects that just need Q9-internal syscall emulation |
| 64 | 66.9 KB | RV32 + RV64 |
| 128 | 141.2 KB | everything |

This is also the answer to extensibility: a new extension is a `case` in a
readable `switch`, written once for all word widths — not a new emulator.

## Sanity check (2026-08-12, before the commit)

Every translation unit compiles standalone, **free of warnings**, with
`cc -std=c99 -Wall`:

```
MAX_XLEN=32    riscv_cpu.o     40648 bytes    0 warnings
MAX_XLEN=64    riscv_cpu.o     66872 bytes    0 warnings
MAX_XLEN=128   riscv_cpu.o    141240 bytes    0 warnings
               iomem.o          5376 bytes    0 warnings
               cutils.o         2464 bytes    0 warnings
               softfp.o        65160 bytes    0 warnings
```

`riscv_cpu.c` requires **both** `-DMAX_XLEN=<n>` and `-DCONFIG_RISCV_MAX_XLEN=<n>`;
it errors out at once if either is missing. Not yet integrated into the Q9
Makefile — that is step 6.8 (`TARGET=m68k|riscv32|x86_32`).

Unlike Musashi, this core needs **no code generation step**: no `m68kmake`
equivalent, plain C is enough.

## Known limitation: upstream is dormant

The last upstream commit is from 2020-05-25. From vendoring onwards this core is
ours to maintain. That was a conscious decision: the RISC-V base ISA is ratified
and frozen, so a complete core does not rot, and at 6146 lines it is small
enough to own. The situation with Musashi is the same.

## Q9-specific changes to the vendor code

None so far. Any change should be marked with `Q9` in the code and listed here,
the way it is done in `third_party/musashi/Q9_VENDOR.md`.
