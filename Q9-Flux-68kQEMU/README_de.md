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

Bisherige Meilensteine:

1. Eine eigene `q9board`-QEMU-Maschine (CPU+RAM-Grundgerüst) wurde aus
   dem Quellcode gebaut und verifiziert — ein von Hand geschriebenes
   68030-Testprogramm lief korrekt durch, bestätigt über den
   QEMU-Monitor.
2. **Erste echte Peripherie portiert: RTC72421.** Das Auslesen ihrer
   Register auf einer laufenden `q9board`-Instanz lieferte die korrekte,
   BCD-kodierte Host-Uhrzeit (UTC, passend zu QEMUs Standard-`-rtc`-
   Verhalten) — byte-genau übereinstimmend mit der Register-Semantik der
   Musashi-Implementierung (Latch-Auffrischung bei Register 0, das
   24h-Bit in Control F, Schreiben wird ignoriert).
3. **Zweite Peripherie portiert: Timer/IRQ3.** Die Adress-Trigger-Fenster
   ($FFFF9000/$FFFF9800) und der 100-Hz-Level-6-Autovektor-Interrupt
   funktionieren Ende-zu-Ende: ein von Hand assembliertes Testprogramm
   installierte einen eigenen Handler auf Vektor 30, aktivierte
   Interrupts, löste das ON-Fenster aus, und der Interrupt wurde korrekt
   zugestellt (per QEMU-Monitor bestätigt — der Handler lief und kehrte
   sauber per `rte` zurück). Anders als bei der RTC brauchte dieses
   Gerät echte IRQ-Zustellung, nicht nur einen Lese-Callback: QEMUs
   autovektorisierte Interrupts haben keinen IACK-Zeitpunkt-Haken, über
   den ein Gerät seine eigene Anforderung wieder senken könnte (echte
   autovektorisierte Hardware kennt ohnehin keinen solchen Buszyklus) —
   die Anforderung wird deshalb gepulst: pro Tick gesetzt, kurz danach
   per einmaligem Timer deutlich innerhalb der 10-ms-Periode wieder
   gesenkt, statt auf eine Quittierung zu warten, die es strukturell nie
   geben kann.

**Repository-Aufbau**, aufgeteilt danach, was die Dateien tatsächlich
sind, nicht danach, wo QEMU sie haben will:

- `devices/<gerät>/` — unsere eigenen Geräte-Simulationen, unabhängig
  von QEMUs Verzeichniskonvention benannt/organisiert (spiegelt
  `Q9-Flux-68k/src/devices/<gerät>/` aus dem Musashi-Zweig). Aktuell:
  `devices/rtc72421/q9_rtc72421.c`, `devices/timer_irq/q9_timer_irq.c`.
- `overlay/machine/q9board.c` — die Board-"Verdrahtung" selbst
  (instanziiert und hängt die obigen Geräte ein), das QEMU-seitige
  Gegenstück zu `Q9-Flux-68k/src/kernel/q9board.c`/`boardcfg.c`.
- `overlay/patches/` — Diffs gegen bestehende QEMU-Dateien, die wir
  anfassen mussten (`Kconfig`/`meson.build`-Registrierungen), als Patches
  statt Ganzdatei-Kopien, damit künftige QEMU-Versions-Updates nicht
  stillschweigend andere, neue Upstream-Ergänzungen an denselben Dateien
  verschlucken.
- `qemu-mapping.conf` — die kleine, explizite Konfiguration, die alles
  zusammenbindet: welche Datei unter `devices/`/`overlay/machine/`
  landet wo innerhalb von `third_party/qemu/`.
- `third_party/qemu/` — ein eigenes, dediziertes QEMU-Submodul (getrennt
  von Q9-Flux-x86s eigener Kopie, die ein unangetasteter, reiner
  Upstream-Checkout bleibt).

Nach dem Klonen (oder nach jedem `git submodule update`, das den
Submodul-Checkout zurücksetzt und unsere Ergänzungen sonst löschen
würde) `./setup-qemu-dev-tree.sh` ausführen, dann bauen mit
`cd third_party/qemu && mkdir build-m68k && cd build-m68k && ../configure --target-list=m68k-softmmu && ninja`.
Wiederholt komplett durchgetestet (zurücksetzen → Skript → Neu-Build →
Boot) mit identischem, korrektem Ergebnis.

QEMU selbst ist auf dem Entwicklungsrechner (macOS, Apple-Silicon)
installiert und getestet: `qemu-system-m68k`, Version 11.1.1, inkl. der
Standard-m68k-Maschinen `an5206`, `mcf5208evb`, `next-cube`, `q800`,
`virt` (keine davon entspricht dem eigenen CB030-/Vinculum-Zielboard —
daher das neue `q9board`-Grundgerüst).

**Nächster Schritt** (groß, mehrere Sitzungen): die verbleibenden
Geräte-Modelle — CF, QUICC, 68681-DUART, Remap-Trigger, Netz-Terminals,
MC6845/Framebuf/CLUT/Videobridge — von `Q9-Flux-68k/src/devices/` nach
QEMUs QOM-/`MemoryRegion`-Muster portieren, ein Gerät nach dem anderen.
Hinweis speziell zu `remap`: anders als jedes bisher portierte Gerät ist
es keine eigenständige Register-/IRQ-Quelle — auf dem Musashi-Board
schaltet es die Adressdekodierung des Boards selbst um (ROM bei 0
gespiegelt im Reset-Zustand vs. RAM bei 0/ROM an anderer Stelle nach dem
Remap), der QEMU-Port muss also `MemoryRegion`-Zuordnungen in
`q9board.c` selbst umkonfigurieren, nicht nur eine neue Gerätedatei
hinzufügen.

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
