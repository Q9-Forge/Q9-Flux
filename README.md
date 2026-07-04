# Q9

Q9 is a modular mini operating system in the tradition of Microware OS-9.

The portable C kernel runs on two targets from a single code base:

- **WebAssembly / Browser** — boots in any modern browser with an
  xterm.js console; instant testing, zero install
- **M68k / Vinculum** (MC68EN360 SBC) — native build via vbcc (planned)

Its OS-9-inspired module system tags every module with a language byte:
WASM modules are instantiated natively, 68k modules transparently run on
an embedded Musashi CPU emulator, and a future 6809 runtime aims at
binary compatibility with original OS-9/6809 software.

## Status

Kernel boots on native (Windows/macOS/Linux) and WebAssembly targets, with
a working module system and a FAT16-backed filesystem. See
[`docs/HANDBUCH.md`](docs/HANDBUCH.md) (German) for the full manual: tools,
build instructions per target, source layout, architecture, and licensing
notes for third-party reference material. `PROJECT.md` (German) has the
vision, architecture decisions and roadmap.

## License

TBD
