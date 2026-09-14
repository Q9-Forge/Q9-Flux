# Q9-Flux 68k

Q9-Flux 68k is a 68030 hardware emulator for genuine OS-9/68K and is part of
the [Q9-Forge](https://github.com/Q9-Forge) project family.

German version: [README_de.md](README_de.md)

It embeds the [Musashi](third_party/musashi/) 68000-family CPU core and
emulates the Q9 board hardware, including RAM/ROM, remapping, 68681 DUART,
CompactFlash/RBF storage, QUICC Ethernet and the real-time clock. The main
goal is to boot and run unmodified OS-9/68K software.

## Build and run

```sh
make host
./build/<platform>/q9.exe <config.q9>
```

The full manual is in [`docs/HANDBOOK.md`](docs/HANDBOOK.md). Local emulator
images, private profiles and archives are intentionally ignored by Git.

## Status

The 68k emulator is the currently most mature Q9-Flux component. Additional
boards and device models are planned.

## License

See [LICENSE](LICENSE) (MIT, for this component's own code). Bundled
third-party components under `third_party/` (Musashi, SoftFloat, TinyEMU,
the slirp networking DLLs, Turbo Vision) keep their own, original
licenses — see [../THIRD-PARTY-LICENSES.md](../THIRD-PARTY-LICENSES.md)
for the full overview.
