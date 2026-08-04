# Q9-Flux

Der gemeinsame projektübergreifende Kontext und die verbindlichen Namen
stehen in [Q9Forge/AI_CONTEXT.md](../Q9Forge/AI_CONTEXT.md).

Q9-Flux is a 68030 hardware emulator for genuine Microware OS-9, part of
[Q9-Forge](https://github.com/Q9-Forge). It embeds the
[Musashi](third_party/musashi/) 68000-family CPU core and emulates board
hardware (RAM/ROM/remap, 68681 DUART, CompactFlash/RBF/PCF storage, QUICC
Ethernet, real-time clock, board-config-driven CF profiles) closely enough
to boot and run real, unmodified Microware OS-9/68K. The first fully
supported board is the CB030; further boards (MC68000, Vinculum, ...) are
planned, see [Q9-Forge's roadmap](https://github.com/Q9-Forge/.github/blob/main/ROADMAP.md).

## Usage

```sh
make native
./build/native/q9.exe <config.q9>
# or: ./build/native/q9.exe --cb030 <rom> [--cf <image>] [--net nat|vmnet|bridge:<if>]
```

See [`docs/HANDBUCH.md`](docs/HANDBUCH.md) (German) for the full manual:
tools, build instructions, source layout, architecture, and licensing
notes for third-party reference material.

## History

Q9 started as a from-scratch mini operating system in the tradition of
Microware OS-9 (own kernel, module system, WASM-as-implementation-language
for a browser target). That subsystem was untouched from 2026-07-04
onward while all further work went into the CB030/Microware-OS-9
emulator; it has been archived to
[`Q9RESUME-Kernel`](https://github.com/foellmy51/Q9RESUME-Kernel) (full
history preserved) should the original mini-OS idea be revisited.
`docs/PROJECT_VISION_ARCHIV.md` keeps the original vision document for
reference.

## License

[MIT](LICENSE)
