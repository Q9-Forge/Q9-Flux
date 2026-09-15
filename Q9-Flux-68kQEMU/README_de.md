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
4. **Dritte Peripherie portiert: die 68681-DUART** (Konsole, nur Kanal
   A, wie schon beim Musashi-Original). Beide Richtungen mit einem von
   Hand assemblierten Testprogramm Ende-zu-Ende verifiziert: TX landete
   byte-genau ("Hi\r\n") in einem `-chardev file`-Backend, und ein vom
   Host über `-chardev pty` gesendetes Byte wurde korrekt empfangen, aus
   D0 ausgelesen und im Speicher abgelegt. Anders als beim Timer bildet
   der Interrupt dieses Geräts (Level 3, Vektor aus dem vom Treiber
   programmierten IVR-Register) QEMUs gehaltenes Interrupt-Modell direkt
   ab — kein Pulsen nötig, `m68k_set_irq_level()` wird einfach erneut
   aufgerufen, sobald sich die Bedingung TxRDY/RxRDY-mit-aktiviertem-IMR
   ändert. Strukturell an QEMUs eigenem `hw/char/mcf_uart.c` orientiert.
5. **Vierte Peripherie portiert: das Compact-Flash-Interface** (ATA-PIO,
   nur die Onboard-Master-Einheit). Bewusst weiterhin klassisches
   stdio-Datei-I/O wie im Original, nicht QEMUs Block-Layer — die
   RBF/PCF-Sektorgrößen-Heuristik muss rohe Header-Bytes direkt aus der
   Backing-Datei lesen, wofür QEMUs Block-Layer einem Gerät keinen Haken
   bietet. Gegen ein echtes Produktiv-Image verifiziert: ein von Hand
   assembliertes Testprogramm setzte LBA/SECCNT, sendete READ SECTOR(S),
   pollte DRQ und las 512 Byte aus dem Datenregister aus — das Ergebnis
   war **byte-genau identisch** zu denselben 512 Byte, direkt aus der
   Backing-Datei in Python gelesen. Bestätigt: bestehende Q9-Disk-Images
   (`OS9SYS.hda` usw.) brauchen keine Änderungen, um angehängt zu
   funktionieren (`-global q9-cf.image=/pfad/zum/image.hda`, optional
   `-global q9-cf.format=rbf|pcf|auto`). Kein IRQ, wie im Original; das
   zweite RC2014-SC145-Interface und die Slave-Einheit bleiben für
   später, zusammen mit der insgesamt noch fest verdrahteten (nicht
   config-gesteuerten) Geräte-Instanziierung in q9board.c.
6. **Fünfte Peripherie portiert: das REMAP-Register.** Anders als jedes
   bisherige Gerät ist das keine eigenständige Register-/IRQ-Quelle —
   auf dem echten Board schaltet es die Adressdekodierung des Boards
   selbst um (ROM bei Adresse 0 gespiegelt im Reset-Zustand vs. RAM bei
   0 / ROM einmalig bei $FE000000-$FE07FFFF nach dem Remap). Das
   Trigger-Fenster selbst (`devices/remap/q9_remap.c`) bleibt so klein
   wie das Original; die eigentlichen ROM-Mirror-/ROM-Fenster-
   `MemoryRegion`s liegen in q9board.c (passend zum Kopfkommentar des
   Originals: das ist Board-Topologie, keine Fenster-Peripherie) und
   werden nur angelegt, wenn per `-bios` tatsächlich ein ROM-/Firmware-
   Image übergeben wird — ohne eins bleibt RAM wie bisher direkt bei 0
   sichtbar, sodass jeder frühere `-kernel`-basierte Gerätetest
   unangetastet bleibt. Mit einem synthetischen ROM-Image über den
   QEMU-Monitor verifiziert (der umgeht den eigenen Instruktions-Cache
   der CPU, die Prüfung ist also unabhängig von TCG-
   Übersetzungsblock-Feinheiten): ein Marker-Byte war vor dem Trigger
   sowohl bei Adresse 0 als auch — über die Modulo-Indizierung des
   Mirrors — an der `-kernel`-Ladeadresse sichtbar; nach einem
   Gast-Zugriff auf das Trigger-Register war der Marker bei Adresse 0
   verschwunden (RAM scheint jetzt durch) und stattdessen bei
   $FE000000 aufgetaucht (die feste Remap-Position des ROMs) — genau
   die Adressdekodierungs-Umschaltung des Originals. Hinweis für
   künftige ROM-Boot-Arbeit: eine `-kernel`-Nutzlast, die geladen wird,
   während der Mirror noch aktiv ist, wird durch ihn hindurch
   geschrieben und dabei stillschweigend verworfen (der Schreib-
   Callback des Mirrors verwirft Schreibzugriffe, passend zum
   `if (!remapped) { return; }` des Originals) — echte Firmware muss
   RAM erst *nach* dem eigenen Remap-Trigger beladen, genau wie auf der
   echten Hardware.
7. **Sechste bis achte Peripherie portiert: das Grafik-Trio** — MC6845-
   CRT-Controller (Register-Geometrie-Grundlage), CLUT (Farbtabelle für
   indizierte Modi) und der VRAM-Framebuffer, alle drei unkomplizierte
   Register-Ports ohne IRQ. Der Framebuffer verlinkt sich mit dem
   MC6845 einzig um dessen Stride-Register für die Dirty-Rechteck-
   Zeilenzuordnung zu lesen (`q9_mc6845_get_stride()`, eine schlichte
   dateiübergreifende C-Funktion wie schon `q9_remap_set_targets()` —
   dieser devices/-Baum hat weiterhin keine gemeinsamen Header). Eine
   echte Verdrahtungs-Feinheit: die Framebuffer-Adresse ($FD000000)
   liegt *innerhalb* der ROM-Mirror-Spanne des REMAP-Geräts
   (0..$FEFFFFFF), anders als jedes andere Geräte-Fenster — q9board.c
   bindet ihn deshalb per `memory_region_add_subregion_overlap()` mit
   einer Priorität oberhalb des Mirrors ein, sodass der Framebuffer
   immer gewinnt, egal ob `-bios` übergeben wird oder nicht — passend
   zur Geräte-Dispatch-vor-Board-Fallback-Reihenfolge des Originals
   (ein einfaches `add_subregion()` hätte dort bei der Überlappung
   assertiert). Mit einem einzigen kombinierten Testprogramm
   verifiziert: MC6845-R1/R6/R18 (Stride/Höhe/Modus), der R/G/B-Eintrag
   eines CLUT-Eintrags und ein 8-Byte-Framebuffer-Muster geschrieben,
   dann alles über dieselbe Register-Schnittstelle zurück ins RAM
   gelesen — der Speicherauszug des QEMU-Monitors stimmte byte-genau
   mit jedem geschriebenen Wert überein.

**Repository-Aufbau**, aufgeteilt danach, was die Dateien tatsächlich
sind, nicht danach, wo QEMU sie haben will:

- `devices/<gerät>/` — unsere eigenen Geräte-Simulationen, unabhängig
  von QEMUs Verzeichniskonvention benannt/organisiert (spiegelt
  `Q9-Flux-68k/src/devices/<gerät>/` aus dem Musashi-Zweig). Aktuell:
  `devices/rtc72421/q9_rtc72421.c`, `devices/timer_irq/q9_timer_irq.c`,
  `devices/duart68681/q9_duart68681.c`, `devices/cf/q9_cf.c`,
  `devices/remap/q9_remap.c`, `devices/mc6845/q9_mc6845.c`,
  `devices/clut/q9_clut.c`, `devices/framebuf/q9_framebuf.c`.
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
Geräte-Modelle — QUICC (Ethernet, schon im Original ein eigenes
Mehrsitzungs-Vorhaben mit mehreren Host-Netzwerk-Backends —
NAT/vmnet/bridge/slirp), Netz-Terminals und Videobridge (streamt
Framebuffer/CLUT zu Q9 Frame) — von `Q9-Flux-68k/src/devices/` nach
QEMUs QOM-/`MemoryRegion`-Muster portieren, ein Gerät nach dem anderen.

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
