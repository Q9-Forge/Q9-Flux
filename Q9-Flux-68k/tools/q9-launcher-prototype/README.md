# Q9 Launcher Prototype

Standalone Turbo-Vision prototype for the Q9 Flux start screen. It deliberately does not start the emulator yet: **Starten** displays the exact future command instead.

## Build and run on macOS

```sh
cd tools/q9-launcher-prototype
make
./build/q9-launcher
```

The prototype finds all `*.q9` files in its current working directory. To test the repository configurations, run it from the repository root:

```sh
./tools/q9-launcher-prototype/build/q9-launcher
```

Use `+` to expand the config frame, select a config, then choose **Starten**. `Esc` or **Beenden** exits.
