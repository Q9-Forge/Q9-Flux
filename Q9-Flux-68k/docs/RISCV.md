# RISC-V bring-up

*German version: [RISCV_de.md](RISCV_de.md)*

Status: **2026-08-12 -- Stage 3 complete. A real, unmodified guest OS (NuttX,
`rv-virt:nsh`) boots and runs interactively on the Q9 RISC-V core: shell
prompt, keyboard input, a real program (`hello`) executing.**

## Why this document exists

Preparation for ARBEITSPLAN.md Phase 6 ("Multiple target architectures"):
before a second CPU backend gets its own `cpu_backend.h` vtable (step 6.5),
something has to actually run on it. This is that something, plus everything
learned building it. Read this before touching `src/devices/{clint,plic,
uart16550}/` or `third_party/tinyemu/` -- three real bugs are documented
here, each costly to rediscover.

## The staged approach, and why it matters

The obvious first target was a real OS (xv6, or RT-Thread Smart). Both turned
out to be dead ends for a *first* step: xv6 needs RV64 + Sv39 paging; RT-Thread
Smart's MMU/userspace layer (`components/lwp/arch/risc-v/`) is RV64-only in
its source tree (confirmed by reading the tree, not by trusting a marketing
claim that says "supports 32-bit and 64-bit, ARM and RISC-V" without saying
*which combination*). Both would also immediately hit a known gap in the
vendored core (`PTE_A`/`PTE_D` incomplete, see `third_party/tinyemu/
Q9_VENDOR.md`) -- a bad place to start, because a failure inside a real OS's
page-fault handler is much harder to diagnose than a failure in an isolated
CPU test.

So the path went bottom-up instead, each stage proving one thing in
isolation before the next stage could hide it inside something bigger:

| Stage | Proves | Devices | Test |
|---|---|---|---|
| 1 -- ISA suite | the CPU core itself, instruction by instruction | none (RAM only) | `make test-riscv` |
| 2 -- UART life sign | the *device* callback path (`cpu_register_device`), a real guest program build | RAM, UART | `make test-rvboard` |
| 2b -- CLINT timer | interrupt delivery (`mip`/`mie`, `wfi`) | RAM, UART, CLINT | `make test-rvtimer` |
| 3 -- NuttX boot | a real OS, external interrupts, a second interrupt controller | RAM, UART, CLINT, PLIC | `make test-rvnuttx` |
| 3-light -- `mie.MEIE` regression | the exact core bug found in stage 3, in isolation, no NuttX toolchain needed | RAM, UART, PLIC (no CLINT) | `make test-rvextirq` |

Each stage found something the one before it could not have shown. That is
the point of building it this way rather than aiming straight at an OS.

## Stage 1 -- ISA test suite (`make test-riscv`)

Runs the official `riscv-tests` ISA suite against the vendored CPU core,
deliberately with **no board at all** -- just RAM and the HTIF report word
(see `riscv-tests/env/p/riscv_test.h`). A failure here names the *instruction*,
not "doesn't boot".

RV32 baseline result: **95 of 104**. The user-mode integer ISA is complete;
every failure traces to a genuinely unimplemented feature (PMP does not
exist, debug triggers are missing, `mcountinhibit` is missing, `PTE_A`/`PTE_D`
are incomplete, LR/SC has no reservation bookkeeping) -- not to wrong
instruction semantics. Full breakdown, including which of these matter for
later stages, in `third_party/tinyemu/Q9_VENDOR.md`.

`test/riscv/fetch-isa-tests.sh 32` fetches and builds the suite (not
committed, foreign material); `test/riscv/run-isa-tests.sh` compares against
a pinned baseline and reports only *changes* -- a permanently-red suite gets
ignored, so the extension groups the core predates (Zba/Zbb/Zbc/Zbs/Zbkb/
Zbkx/Zfh/Zicond, 52 further tests, all failing with "illegal instruction"
since this core is from 2017) are deliberately excluded from the baseline.

## Stage 2 -- UART life sign (`make test-rvboard`)

First own guest program (`test/riscv/hello/`), first real device wiring
(`cpu_register_device` with our own read/write callbacks). No libc --
Homebrew's `riscv64-elf-gcc` ships none, and a life sign does not need one.

## Stage 2b -- CLINT timer interrupts (`make test-rvtimer`)

First working interrupt: a bare-metal trap handler counts five periodic
timer interrupts and stops cleanly. Two bugs were found and fixed while
building this, both are the load-bearing lessons for stage 3's PLIC work:

1. **`wfi` only wakes through an explicit API call.** `riscv_cpu_interp()`
   never re-evaluates `power_down_flag` on its own; only
   `riscv_cpu_set_mip()` clears it (when `mip & mie` becomes non-zero after
   the call). A board driver that just advances time and hopes the CPU
   notices is wrong -- it has to call `set_mip`/`reset_mip` itself.
2. **An interrupt storm from a stale `mip` bit.** `mip.MTIP` is a *latch* in
   this core, not the continuously live `mtime >= mtimecmp` signal real
   hardware has. Syncing it only once per host time-slice let the trap
   handler re-fire dozens of times before the next sync caught up (measured:
   33 interrupts instead of 5). The fix: sync **immediately** on every event
   that can change the pending condition -- in this case, right inside the
   `mtimecmp` MMIO write callback, not just periodically in the main loop.

Full detail and the counter-probe (deliberately reverting the fix reproduces
33 every time) in `src/devices/clint/clint.h` and the commit history.

## Stage 3 -- NuttX boot (`make test-rvnuttx`)

Adds PLIC, and turned up the two most consequential findings of this whole
effort.

### PLIC is not optional

NuttX's 16550 driver attaches its ISR via `irq_attach()`; without a working
interrupt path, the system boots to a shell prompt (TX is polled) but **no
keystroke ever arrives** -- proven empirically, not just read from source:
feeding "help\n" via stdin left the cycle counter identical with and without
input.

### The IRQ-number gotcha

`CONFIG_16550_UART0_IRQ=37` in `.config` is **not** the raw PLIC source
number. NuttX numbers IRQs contiguously: internal exceptions/traps first
(`RISCV_MAX_EXCEPTION=15`), then `RISCV_IRQ_ASYNC=16` as the first
asynchronous number, then external PLIC sources starting at
`RISCV_IRQ_MEXT = RISCV_IRQ_ASYNC+11 = 27` (M-mode; S-mode would be
`RISCV_IRQ_SEXT = RISCV_IRQ_ASYNC+9 = 25`). The real PLIC source is
`irq - RISCV_IRQ_EXT`, i.e. **10**, not 37. Confirmed by tracing an actual
PLIC `ENABLE1` write during boot: bit 10 (`0x00000400`), not bit 5 (which
source 37 would have been). Get this wrong and PLIC enables a source nothing
ever raises -- no error, just silence, indistinguishable from "PLIC doesn't
work at all" without tracing the actual register writes.

### The core bug: `mie` could never enable external interrupts

The deepest find. Even with the IRQ number fixed, PLIC wiring correct, and
`riscv_cpu_set_mip(MIP_MEIP)` provably firing (`mip` showed bit 11 set
exactly when expected), no interrupt was ever delivered to the guest. A
one-line trace inside the core's `raise_interrupt()` showed why: `mip=0x880`
(both `MTIP` and `MEIP` pending) next to `mie=0x80` (only `MTIE` enabled).

The cause was in `third_party/tinyemu/riscv_cpu.c`'s CSR write handler for
`mie` (CSR `0x304`): the writable-bit mask was `MIP_MSIP | MIP_MTIP |
MIP_SSIP | MIP_STIP | MIP_SEIP` -- every enable bit **except** `MIP_MEIP`
(bit 11, Machine External Interrupt Enable). Any guest attempt to set that
bit via `csrs mie, ...` was silently dropped. This is not a mistake specific
to NuttX's approach: the mask on `mip` (the *pending* register, CSR `0x344`)
correctly excludes `MIP_MEIP` -- a real machine sets that bit from hardware,
not from guest writes -- but the exact same reasoning does **not** apply to
`mie` (the *enable* register), which must always be software-writable.
Copy-paste between the two masks dropped a bit it should never have dropped
from `mie`. Fixed and documented (`Q9`-marked) in
`third_party/tinyemu/Q9_VENDOR.md`; purely additive, does not change any
previously-passing case.

Without stage 1's isolated, CPU-only ISA suite as a mental model, this bug
would have been much harder to place -- "no external interrupts ever fire"
looks identical whether the fault is in the guest OS's driver, the board's
PLIC wiring, the IRQ-number translation, or the CPU core's CSR handling.
Tracing outward from the core (add a print exactly where `mip & mie` gets
evaluated) rather than guessing at the board layer settled it in one step.

### Regression coverage that does not depend on NuttX

`make test-rvnuttx` is the full proof, but it only runs where the entire
NuttX toolchain (xPack `riscv-none-elf-gcc`, `kconfig-tweak`, `genromfs`,
`flock`) happens to be installed -- exactly the kind of environment that
will not always be available. `make test-rvextirq` isolates the same bug on
its own: a small bare-metal guest (`test/riscv/extirq/`) that programs PLIC
directly (its own, self-chosen source number, no NuttX IRQ-numbering
translation involved) and sets `mie.MEIE` via `csrs mie, ...` -- the exact
line the bug silently defeated. No CLINT on this board, deliberately, so
only the external-interrupt path is exercised. Runs in well under a second
with nothing beyond the same `riscv64-elf-gcc` the ISA suite already needs.

Counter-probe performed while writing it: reverting the `mie` mask fix in a
scratch copy of the core does **not** make the test merely report the wrong
count -- the guest's `wfi` never wakes (the trap is never taken), and the
host's bounded slice loop (`test/rvboard_extirq.c`) simply runs out its
budget and exits with only the startup banner printed, no "Interrupts
behandelt" line at all. Confirmed to terminate cleanly either way (no risk
of a hung test process): ~30 ms real time to exhaust 400 million simulated
cycles doing nothing, since an idle `power_down` slice costs the host next
to nothing.

## Device design: why nothing here uses `src/kernel/devreg.h`

`src/devices/{uart16550,clint,plic}/` are deliberately **not** wired through
the existing 68k-side device vtable. Its 16/32-bit synthesis from `read8`/
`write8` is documented as big-endian -- correct for 68k, wrong for RISC-V
(little-endian). Generalising `devreg.h` is ARBEITSPLAN.md step 6.7; until
then these three devices expose a narrow byte/word-oriented interface that
both architectures can use without the wrong endianness being baked into
either. Each device also knows nothing about the CPU (no `RISCVCPUState*`
inside `clint.c`/`plic.c`/`uart16550.c`) -- the board driver (`test/
rvboard_*.c`) is the only place that connects device state to
`riscv_cpu_set_mip()`/`reset_mip()`, mirroring how `q9boardrun.c` is the only
place that knows about both `devreg.h` and Musashi.

## Address map (all boards in this bring-up)

Matches QEMU's `virt` machine, deliberately -- so the same guest images work
against `qemu-system-riscv32`/`64` as a cross-check, and later work (virtio-blk,
more of the address space) has a real reference instead of an invented one.

| Address | Device | Present since |
|---|---|---|
| `0x00001000` | reset stub (jump to ELF entry, since the core resets at `0x1000` but every guest here is linked at `0x80000000`) | stage 2 |
| `0x02000000` | CLINT (timer/software interrupts) | stage 2b |
| `0x0c000000` | PLIC (external interrupt routing) | stage 3 |
| `0x10000000` | 16550 UART | stage 2 |
| `0x80000000` | RAM (16 MiB stage 2/2b, 32 MiB stage 3 -- NuttX's `rv-virt:nsh` defconfig requires it) | stage 2 |

## Toolchains

Two, for two different reasons -- **do not consolidate without re-testing.**

- **`riscv64-elf-gcc`** (Homebrew, multilib: `rv32i` through `rv64imafdc`) --
  for the ISA suite and our own bare-metal guests (`test/riscv/hello/`,
  `test/riscv/timer/`). Works cleanly for `-nostdlib` bare-metal code with no
  libc dependency.
- **xPack `riscv-none-elf-gcc`** -- required for NuttX specifically. The
  Homebrew toolchain's final link pulled in the wrong-width `libgcc.a`
  (`ELFCLASS64` into a 32-bit link), and once that was worked around by
  forcing the correct multilib path, a *second*, different failure appeared
  (soft-float/double-float ABI mismatch inside `libgcc.a` itself). NuttX's
  own `Toolchain.defs` names xPack's `riscv-none-elf-gcc` as the preferred
  toolchain for exactly this reason -- it is what NuttX's own CI actually
  tests against. Switching resolved both failures at once. Get it from
  <https://github.com/xpack-dev-tools/riscv-none-elf-gcc-xpack/releases>
  (`...-darwin-arm64.tar.gz` on macOS Apple Silicon), no installer needed,
  just put `bin/` on `PATH`.

Building NuttX itself additionally needs `kconfig-tweak`, `genromfs`, and
`flock` -- none of them in Homebrew's main formulae under working URLs at
the time of writing. Full recipe, including a real macOS gotcha (a stray
`MAKE=os9make -e` environment variable from unrelated cross-toolchain work
got baked into the generated `Makefile` by `./configure` and silently broke
the build two different ways before the cause was found), in the header of
`test/riscv/fetch-nuttx.sh`.

## What's next

- **6.7** (generalise `devreg.h`): once done, these three devices could move
  under the unified scheme -- not urgent, they work correctly as they are.
- **`PTE_A`/`PTE_D`** (the still-open ISA gap from stage 1): needed before
  any Sv32-paging guest (NuttX's `knsh32` config, a genuine RV32 alternative
  to xv6's RV64/Sv39 that stays within this project's `riscv32` target) can
  be attempted without risking the same kind of buried, hard-to-place bug
  this document's stage 3 section describes.
- **virtio-blk**: needed for NuttX configs that load ELF apps from a real
  filesystem rather than the compiled-in `nsh`/`knsh` builtin-app table used
  here.
