# Q9-Flux 68k — QEMU-Variante

*Englische Version: [README.md](README.md)*

Dieses Verzeichnis ist der Ausgangspunkt für eine künftige Umstellung des
68K-Emulator-Kerns von [Musashi](../Q9-Flux-68k/third_party/musashi/) auf
[QEMU](https://www.qemu.org/). **Der bestehende Musashi-basierte Emulator
unter [`Q9-Flux-68k/`](../Q9-Flux-68k/) bleibt vollständig nutzbar und wird
durch dieses Verzeichnis nicht verändert.** Beide Zweige existieren
parallel, bis die QEMU-Variante so weit ist, dass sie den Musashi-Zweig
ersetzen kann — oder auf Dauer nebeneinander bestehen bleiben.

## Warum QEMU

QEMU bringt ein eigenes, ausgereiftes Geräte-Modell (QOM/qdev,
`MemoryRegion`-API) mit, das für einige geplante Erweiterungen — allen
voran den [Host-Passthrough-Dateisystem-Manager](docs/HOSTFS_MANAGER_de.md)
— besser geeignet erscheint als das aktuelle, um Musashi herum
handgeschriebene `src/devices/`-Muster. Details zum QEMU-Geräte-Modell
und zu bereits vorhandenen, thematisch verwandten QEMU-Bausteinen
(`vvfat`, `virtio-9p`) stehen in
[`docs/QEMU_DEVICE_MODEL_de.md`](docs/QEMU_DEVICE_MODEL_de.md).

## Stand (2026-09-15)

Noch nicht begonnen. QEMU ist auf dem Entwicklungsrechner (macOS,
Apple-Silicon) installiert und getestet (`qemu-system-m68k`, Version
11.1.1, inkl. der Standard-m68k-Maschinen `an5206`, `mcf5208evb`,
`next-cube`, `q800`, `virt`). Keine davon entspricht dem eigenen
CB030-/Vinculum-Zielboard — ein eigener QEMU-Maschinentyp wird nötig
sein, analog zu `q9board.c`/`boardcfg.c` im Musashi-Zweig.

## QEMU installieren

### macOS

```sh
brew install qemu
```

Installiert alle QEMU-Zielarchitekturen, inklusive `qemu-system-m68k`.
Aktualisieren mit `brew upgrade qemu`.

### Windows 11

Offizieller Installer: <https://qemu.weilnetz.de/> (inoffizielle, aber
etablierte Windows-Builds) oder über den Paketmanager:

```powershell
winget install qemu
```

### Linux

Paketname je nach Distribution, meist `qemu-system-m68k` oder das
Sammelpaket `qemu-system`:

```sh
# Debian/Ubuntu
sudo apt install qemu-system-m68k

# Fedora
sudo dnf install qemu-system-m68k

# Arch
sudo pacman -S qemu-system-m68k
```

## Weitere Dokumente

- [`docs/QEMU_DEVICE_MODEL_de.md`](docs/QEMU_DEVICE_MODEL_de.md) — QOM/qdev,
  `MemoryRegion`-API, Vergleichspunkte `vvfat`/`virtio-9p`
- [`docs/HOSTFS_MANAGER_de.md`](docs/HOSTFS_MANAGER_de.md) — vollständiger
  Architektur-Entwurf für den geplanten Host-Passthrough-Dateisystem-Manager
  (Manager/Driver/Descriptor-Aufteilung, die 13 Standard-Einstiegspunkte,
  Nebenläufigkeit, Attribut-Handling, Cross-Platform-Fallstricke)
