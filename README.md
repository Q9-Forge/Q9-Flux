# Q9-Flux

Q9-Flux is the emulator and hardware-virtualisation project family for Q9.

German version: [README_de.md](README_de.md)

## Components

- `Q9-Flux-68k/` — 68000/OS-9 emulator and board model
- `Q9-Flux-x86/` — x86/OS9000 emulator work
- `Q9-Flux-Devices/` — shared device projects such as Q9-Frame

Architecture-specific implementation remains inside the corresponding
component directory. Local emulator images, private profiles and archives
are ignored and must not be committed.

## License

Q9-Flux's own code is licensed under the [MIT License](LICENSE). Several
bundled or referenced third-party components (CPU emulation cores,
networking libraries, QEMU as an external process for Q9-Flux-x86, and
others) keep their own, original licenses — see
[THIRD-PARTY-LICENSES.md](THIRD-PARTY-LICENSES.md) for the full list.
