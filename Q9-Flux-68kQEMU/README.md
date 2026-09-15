# Q9-Flux 68k — QEMU variant

*German version: [README_de.md](README_de.md)*

This directory is the starting point for a future migration of the 68K
emulator core from [Musashi](../Q9-Flux-68k/third_party/musashi/) to
[QEMU](https://www.qemu.org/). **The existing Musashi-based emulator
under [`Q9-Flux-68k/`](../Q9-Flux-68k/) remains fully usable and is not
touched by this directory.** Both branches exist in parallel until the
QEMU variant is far enough along to replace the Musashi branch — or the
two continue to coexist long-term.

## Why QEMU

QEMU brings its own mature device model (QOM/qdev, the `MemoryRegion`
API) that looks better suited for some planned extensions — most notably
the [host-passthrough filesystem manager](docs/HOSTFS_MANAGER.md) — than
the current, hand-written `src/devices/` pattern built around Musashi.
Details on QEMU's device model and on already-existing, thematically
related QEMU building blocks (`vvfat`, `virtio-9p`) are in
[`docs/QEMU_DEVICE_MODEL.md`](docs/QEMU_DEVICE_MODEL.md).

## Status (2026-09-15)

Milestones reached so far:

1. A custom `q9board` QEMU machine (CPU + RAM skeleton) was built from
   source and verified — a hand-crafted 68030 test program executed
   correctly, confirmed via the QEMU monitor.
2. **First real peripheral ported: RTC72421.** Reading its registers on
   a running `q9board` returned the correct BCD-encoded host time (UTC,
   matching QEMU's default `-rtc` behaviour), byte-for-byte matching the
   Musashi implementation's register semantics (latch-on-register-0,
   Control F's 24h bit, writes ignored).
3. **Second peripheral ported: Timer/IRQ3.** The address-trigger windows
   ($FFFF9000/$FFFF9800) and the 100 Hz level-6 autovector interrupt now
   work end-to-end: a hand-assembled test program installed its own
   handler at vector 30, enabled interrupts, triggered the ON window, and
   the interrupt was delivered correctly (confirmed via the QEMU monitor
   -- the handler ran and returned cleanly via `rte`). Unlike the RTC,
   this device needed real IRQ delivery, not just a read callback: QEMU's
   autovectored interrupts have no IACK-time hook a device could use to
   lower its own request (real autovectored hardware has no such cycle
   either), so the request is pulsed -- raised on each tick, lowered
   again by a short one-shot timer well inside the 10 ms period -- rather
   than held until an acknowledgement that, structurally, can never come.
4. **Third peripheral ported: the 68681 DUART** (console, channel A
   only, matching the Musashi original's scope). Both directions
   verified end-to-end with a hand-assembled test program: TX landed
   byte-exact ("Hi\r\n") in a `-chardev file` backend, and a byte sent
   in from the host side over a `-chardev pty` was correctly received,
   read out of D0 and stored to memory. Unlike the timer, this device's
   interrupt (level 3, vector taken from the driver-programmed IVR
   register) maps directly onto QEMU's held-interrupt model -- no
   pulsing needed, `m68k_set_irq_level()` is simply called again
   whenever the TxRDY/RxRDY-with-IMR-enabled condition changes.
   Structurally modelled on QEMU's own `hw/char/mcf_uart.c`.
5. **Fourth peripheral ported: the Compact-Flash interface** (ATA-PIO).
   Deliberately still plain stdio file I/O like the original, not
   QEMU's block layer -- the RBF/PCF sector-size heuristic needs to
   read raw header bytes from the real backing file itself, which
   QEMU's block layer gives a device no hook for. Verified against a
   real production image: a hand-assembled test program set up
   LBA/SECCNT, issued READ SECTOR(S), polled DRQ, and drained 512 bytes
   from the data register -- the result was **byte-for-byte identical**
   to the same 512 bytes read directly from the backing file in Python.
   Confirms existing Q9 disk images (`OS9SYS.hda` etc.) need no changes
   to work once attached (`-global q9-cf.image=/path/to/image.hda`,
   optionally `-global q9-cf.format=rbf|pcf|auto`). No IRQ, matching the
   original. Both the slave unit (`-global q9-cf.slave-image=...`) and
   the RC2014-SC145 second interface (a second instance of the same
   device, registered under its own QOM type name `q9-cf2` -- a true
   QOM *subclass* of `q9-cf` rather than a copy-pasted sibling, since
   "-global" keys off the type name and two same-typed instances
   couldn't otherwise take independent property values, and since the
   type check inside the shared `realize()` would reject an unrelated
   type outright) were added in a follow-up and verified together: three
   distinct marker images (onboard master/slave, second-interface
   master) each came back correctly and independently through their own
   unit/interface selection.
6. **Fifth peripheral ported: the REMAP register.** Unlike every device
   so far, this one isn't a standalone register/IRQ source -- on the
   real board it gates the board's own address decode (ROM mirrored at
   address 0 in the reset state vs. RAM at 0 / ROM once at
   $FE000000-$FE07FFFF once remapped). The trigger window itself
   (`devices/remap/q9_remap.c`) stays as small as the original; the
   actual ROM-mirror/ROM-window `MemoryRegion`s live in q9board.c
   (matching the original's own header comment: this is board topology,
   not window peripherality) and are only created when a ROM/firmware
   image is given via `-bios` -- without one, RAM stays directly visible
   at 0 as before, so every earlier device's `-kernel`-based test is
   unaffected. Verified with a synthetic ROM image via the QEMU monitor
   (bypassing the CPU's own instruction cache, so the check is
   independent of any TCG translation-block subtleties): a marker byte
   was visible at address 0 and, through the mirror's modulo indexing,
   at the `-kernel` load address before the trigger; after a guest
   access to the trigger register, the marker was gone from address 0
   (RAM now shows through) and appeared instead at $FE000000 (the ROM's
   fixed remapped position) -- exactly the original's address-decode
   swap. Note for future ROM-boot work: a `-kernel` payload loaded while
   the mirror is still enabled is written through it and silently
   discarded (the mirror's write callback drops writes, matching the
   original's own `if (!remapped) { return; }`) -- real firmware has to
   load into RAM only *after* triggering the remap itself, same as the
   original hardware.
7. **Sixth through eighth peripherals ported: the graphics trio** --
   MC6845 CRT controller (register geometry base), CLUT (indexed-mode
   colour table), and the VRAM framebuffer, all three straightforward
   register-map ports with no IRQ. The framebuffer links to the MC6845
   purely to read its stride register for dirty-rectangle row mapping
   (`q9_mc6845_get_stride()`, a plain cross-file C function like
   `q9_remap_set_targets()` -- this devices/ tree still has no shared
   headers). One real wiring subtlety: the framebuffer's address
   ($FD000000) falls *inside* the REMAP device's ROM-mirror span
   (0..$FEFFFFFF), unlike every other device's window -- q9board.c maps
   it with `memory_region_add_subregion_overlap()` at a priority above
   the mirror's, so the framebuffer always wins whether or not "-bios"
   is given, matching the original's own device-dispatch-before-board-
   fallback ordering (a plain `add_subregion()` there would have
   asserted on the overlap). Verified with a single combined test
   program: wrote MC6845 R1/R6/R18 (stride/height/mode), a CLUT entry's
   R/G/B, and an 8-byte framebuffer pattern, then read all of it back
   through the same register interface into RAM -- the QEMU monitor's
   memory dump matched every written value byte-for-byte.
8. **Ninth peripheral ported: QUICC Ethernet** (MC68360 SCC1). The
   register/PRAM window, CP command register, buffer-descriptor rings
   and the SDMA-style frame transfer to/from guest RAM are ported 1:1
   (same offsets, same write-1-to-clear SCCE/CISR semantics). What's
   deliberately *not* ported is the original's four hand-rolled host
   network backends (nat/vmnet/bridge/slirp, ~500 of quicc.c's 872
   lines) -- this device is instead a standard QEMU NIC frontend
   (`qemu_new_nic()`), so the usual `-netdev user/tap/socket/...`
   machinery gives it a network, and the original's entire
   ~100-line-per-backend `q_backend_tx` dispatch collapses to one
   `qemu_send_packet()` call. Level-5 IRQ with a *fixed* vector (254,
   unlike DUART's driver-programmed IVR) -- held, not pulsed, same
   model as DUART. Verified with a hand-built BD ring in both
   directions over a real `-netdev socket` UDP tunnel: a queued TX
   descriptor produced a **byte-for-byte exact** 14-byte frame captured
   on the wire, and an injected inbound frame landed in the RX
   descriptor with the correct WRAP/FIRST/LAST status, the correct
   `length+4` (CRC accounting), the correct SCCE.RXF event, and
   **byte-for-byte exact** data in guest RAM.
9. **Tenth peripheral ported: network terminals** (8 channels,
   `/x1`..`/x8`). One 256-byte I/O block per channel (status/RX/TX),
   contiguous window dispatched by channel index -- one QOM device
   internally, not eight. Shared level-4 IRQ, vectored to the first
   channel (index order) with an unread byte, exactly the original's
   `network_irq_resync()`. Telnet option negotiation is no longer
   hand-rolled: QEMU's own socket chardev already does full RFC854
   negotiation (`telnet=on`), so bytes reaching this device are already
   clean data. The original's single dynamic-port dispatcher (one
   listen socket handing connections to whichever channel is free) is
   replaced by 8 independent `chardevN` qdev properties -- a fixed port
   per line, attached the usual way (e.g. `-chardev
   socket,id=x1,port=2001,server=on,wait=off,telnet=on -global
   q9-nettty.chardev0=x1`). CR-then-LF suppression (genuine OS-9-side
   protocol behaviour, not telnet plumbing) is still ported. Verified
   over a real TCP connection: TX landed byte-exact (`"Hi"`), and an
   injected byte was received, correctly read back, and correctly
   cleared RX-ready on read.
10. **Eleventh and final component ported: the host video bridge** (Q9
    Frame protocol). Unlike every actual peripheral above, this one was
    never part of the guest-visible hardware in the original either (no
    devreg entry, no vtable) -- a host-side service that reads the
    already-ported framebuffer/MC6845/CLUT via small cross-file
    accessors and streams HELLO/VIDEO_INFO/PALETTE/FRAME_FULL/
    FRAME_UPDATE to an external Q9 Frame viewer, ported here as a plain
    QOM device with no `MemoryRegion` (pure lifecycle/property
    management) driven by a fixed-rate `QEMUTimer` instead of the
    original's poll-once-per-main-loop-round call site (real frame-send
    throttling still comes from the CRTC's R19, same as the original).
    Socket setup failures (most commonly: another Q9-Flux instance
    already holding the port) are warnings, not fatal machine-startup
    errors -- matching the original's own graceful-degradation
    philosophy, unlike an earlier version of this port that crashed the
    whole machine on a port conflict. Verified against a real Python
    client speaking the actual wire protocol: VIDEO_INFO reported the
    exact geometry programmed into the MC6845 (including the bpp<8
    stride-to-pixel-width conversion), PALETTE reported the exact CLUT
    entry written, and FRAME_FULL's payload was **byte-for-byte
    identical** to the framebuffer content written by a guest test
    program.

All eleven original Musashi `src/devices/` components (ten guest-visible
peripherals plus the host-side video bridge) now have a verified QEMU
counterpart.

**Repository layout**, split by what the files actually are, not by
where QEMU wants them:

- `devices/<name>/` — our own peripheral simulations, named/organized
  independently of QEMU's directory conventions (mirrors
  `Q9-Flux-68k/src/devices/<name>/` from the Musashi branch). Currently:
  `devices/rtc72421/q9_rtc72421.c`, `devices/timer_irq/q9_timer_irq.c`,
  `devices/duart68681/q9_duart68681.c`, `devices/cf/q9_cf.c`,
  `devices/remap/q9_remap.c`, `devices/mc6845/q9_mc6845.c`,
  `devices/clut/q9_clut.c`, `devices/framebuf/q9_framebuf.c`,
  `devices/quicc/q9_quicc.c`, `devices/nettty/q9_nettty.c`,
  `devices/videobridge/q9_videobridge.c`.
- `overlay/machine/q9board.c` — the board "wiring" itself (instantiates
  and maps the devices above), the QEMU-side counterpart to
  `Q9-Flux-68k/src/kernel/q9board.c`/`boardcfg.c`.
- `overlay/patches/` — diffs against existing QEMU files we needed to
  touch (`Kconfig`/`meson.build` registrations), kept as patches rather
  than full-file copies so future QEMU version bumps don't silently
  drop unrelated upstream additions to those files.
- `qemu-mapping.conf` — the small, explicit config that ties it
  together: which file under `devices/`/`overlay/machine/` lands at
  which path inside `third_party/qemu/`.
- `third_party/qemu/` — a dedicated QEMU submodule (separate from
  Q9-Flux-x86's own copy, which stays a pristine, unmodified upstream
  checkout).

Run `./setup-qemu-dev-tree.sh` after cloning (or after any
`git submodule update`, which resets the submodule and would otherwise
wipe our additions), then build with
`cd third_party/qemu && mkdir build-m68k && cd build-m68k && ../configure --target-list=m68k-softmmu && ninja`.
Verified end-to-end repeatedly (reset → script → rebuild → boot) with
identical, correct results.

QEMU itself is installed and tested on the development machine (macOS,
Apple Silicon): `qemu-system-m68k`, version 11.1.1, including the
standard m68k machines `an5206`, `mcf5208evb`, `next-cube`, `q800`,
`virt` (none of which match our own CB030/Vinculum target board — hence
the new `q9board` skeleton).

**Next steps**: with every `src/devices/` component ported and verified
(CF now including its slave unit and RC2014-SC145 second interface),
what's left is board-level rather than device-level work -- config-
driven device instantiation (today everything in `q9board.c` is still
hardcoded/`-global`-configured, mirroring the original's own
pre-boardcfg.c state, rather than driven by a declarative board config
file the way `boardcfg.c` drives the Musashi branch), and eventually
the actual [host-passthrough filesystem manager](docs/HOSTFS_MANAGER.md)
this whole migration was started for in the first place (s. "Why QEMU"
above).

## Installing QEMU

### macOS

```sh
brew install qemu
```

Installs all QEMU target architectures, including `qemu-system-m68k`.
Update with `brew upgrade qemu`.

### Windows 11

Official-ish installer builds: <https://qemu.weilnetz.de/> (unofficial
but well-established Windows builds), or via a package manager:

```powershell
winget install qemu
```

### Linux

Package name varies by distribution, usually `qemu-system-m68k` or the
combined `qemu-system` package:

```sh
# Debian/Ubuntu
sudo apt install qemu-system-m68k

# Fedora
sudo dnf install qemu-system-m68k

# Arch
sudo pacman -S qemu-system-m68k
```

## Further documents

- [`docs/QEMU_DEVICE_MODEL.md`](docs/QEMU_DEVICE_MODEL.md) — QOM/qdev,
  the `MemoryRegion` API, reference points `vvfat`/`virtio-9p`
- [`docs/HOSTFS_MANAGER.md`](docs/HOSTFS_MANAGER.md) — the full
  architecture design for the planned host-passthrough filesystem
  manager (Manager/Driver/Descriptor split, the 13 standard entry
  points, concurrency, attribute handling, cross-platform pitfalls)
