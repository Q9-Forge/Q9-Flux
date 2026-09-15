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
5. **Vierte Peripherie portiert: das Compact-Flash-Interface** (ATA-PIO).
   Bewusst weiterhin klassisches stdio-Datei-I/O wie im Original, nicht
   QEMUs Block-Layer — die RBF/PCF-Sektorgrößen-Heuristik muss rohe
   Header-Bytes direkt aus der Backing-Datei lesen, wofür QEMUs
   Block-Layer einem Gerät keinen Haken bietet. Gegen ein echtes
   Produktiv-Image verifiziert: ein von Hand assembliertes Testprogramm
   setzte LBA/SECCNT, sendete READ SECTOR(S), pollte DRQ und las 512
   Byte aus dem Datenregister aus — das Ergebnis war **byte-genau
   identisch** zu denselben 512 Byte, direkt aus der Backing-Datei in
   Python gelesen. Bestätigt: bestehende Q9-Disk-Images (`OS9SYS.hda`
   usw.) brauchen keine Änderungen, um angehängt zu funktionieren
   (`-global q9-cf.image=/pfad/zum/image.hda`, optional `-global
   q9-cf.format=rbf|pcf|auto`). Kein IRQ, wie im Original. Sowohl die
   Slave-Einheit (`-global q9-cf.slave-image=...`) als auch das zweite
   RC2014-SC145-Interface (eine zweite Instanz desselben Geräts,
   registriert unter eigenem QOM-Typnamen `q9-cf2` — eine echte
   QOM-*Subklasse* von `q9-cf`, nicht ein kopiertes Geschwister-Typinfo,
   da `-global` am Typnamen ansetzt und zwei gleichtypige Instanzen sich
   sonst nicht unabhängig konfigurieren ließen, und da die Typprüfung
   in der gemeinsamen `realize()` einen unverwandten Typ sonst schlicht
   zurückgewiesen hätte) kamen in einem Nachtrag dazu und wurden
   gemeinsam verifiziert: drei unterschiedliche Marker-Images
   (Onboard-Master/-Slave, Zweitinterface-Master) kamen jeweils korrekt
   und unabhängig über ihre eigene Einheiten-/Interface-Auswahl zurück.
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
8. **Neunte Peripherie portiert: QUICC-Ethernet** (MC68360 SCC1). Das
   Register-/PRAM-Fenster, das CP-Kommandoregister, die
   Buffer-Descriptor-Ringe und der SDMA-artige Frame-Transfer von/zum
   Gast-RAM sind 1:1 portiert (dieselben Offsets, dieselbe
   Write-1-to-Clear-Semantik für SCCE/CISR). Bewusst *nicht* portiert
   sind die vier handgeschriebenen Host-Netzwerk-Backends des Originals
   (nat/vmnet/bridge/slirp, ~500 der 872 Zeilen von quicc.c) — dieses
   Gerät ist stattdessen ein Standard-QEMU-NIC-Frontend
   (`qemu_new_nic()`), sodass die übliche
   `-netdev user/tap/socket/...`-Maschinerie ihm ein Netz verschafft,
   und der gesamte ~100-Zeilen-pro-Backend-Dispatch `q_backend_tx` des
   Originals zu einem einzigen `qemu_send_packet()`-Aufruf
   zusammenschrumpft. Level-5-IRQ mit *festem* Vektor (254, anders als
   der vom Treiber programmierte IVR der DUART) — gehalten, nicht
   gepulst, dasselbe Modell wie bei der DUART. Mit einem von Hand
   aufgebauten BD-Ring in beide Richtungen über einen echten
   `-netdev socket`-UDP-Tunnel verifiziert: ein eingereihter TX-Deskriptor
   erzeugte ein **byte-genau exaktes** 14-Byte-Frame, auf der Leitung
   abgefangen, und ein eingespeistes eingehendes Frame landete im
   RX-Deskriptor mit korrektem WRAP/FIRST/LAST-Status, korrekter
   `Länge+4` (CRC-Berücksichtigung), korrektem SCCE.RXF-Ereignis und
   **byte-genau exakten** Daten im Gast-RAM.
9. **Zehnte Peripherie portiert: Netzwerk-Terminals** (8 Kanäle,
   `/x1`..`/x8`). Ein 256-Byte-I/O-Block je Kanal (Status/RX/TX),
   zusammenhängendes Fenster, per Kanalindex dispatcht — EIN
   QOM-Gerät intern, nicht acht. Gemeinsamer Level-4-IRQ, vektorisiert
   auf den ersten Kanal (Indexreihenfolge) mit unabgeholtem Byte, genau
   das `network_irq_resync()` des Originals. Die
   Telnet-Optionsverhandlung wird nicht mehr selbst gebaut: QEMUs
   eigener Socket-Chardev erledigt bereits volle
   RFC854-Verhandlung (`telnet=on`), Bytes, die dieses Gerät erreichen,
   sind also schon saubere Daten. Der einzelne dynamische
   Port-Dispatcher des Originals (ein Listen-Socket, der eingehende
   Verbindungen an den jeweils freien Kanal übergibt) wird durch acht
   unabhängige `chardevN`-qdev-Properties ersetzt — ein fester Port je
   Leitung, auf die übliche Art angehängt (z.B. `-chardev
   socket,id=x1,port=2001,server=on,wait=off,telnet=on -global
   q9-nettty.chardev0=x1`). Die CR-dann-LF-Unterdrückung (echtes
   OS-9-seitiges Protokollverhalten, keine Telnet-Mechanik) ist weiterhin
   portiert. Über eine echte TCP-Verbindung verifiziert: TX landete
   byte-genau (`"Hi"`), und ein eingespeistes Byte wurde empfangen,
   korrekt zurückgelesen und löschte RX-Ready beim Lesen korrekt.
10. **Elfte und letzte Komponente portiert: die Host-Video-Bridge** (Q9-
    Frame-Protokoll). Anders als jede echte Peripherie oben war das
    auch im Original nie Teil der gastsichtbaren Hardware (kein
    devreg-Eintrag, keine Vtable) — ein Host-seitiger Dienst, der
    Framebuffer/MC6845/CLUT über schlichte dateiübergreifende
    Zugriffsfunktionen liest und HELLO/VIDEO_INFO/PALETTE/FRAME_FULL/
    FRAME_UPDATE an einen externen Q9-Frame-Viewer streamt, hier als
    einfaches QOM-Gerät ohne `MemoryRegion` portiert (reines
    Lifecycle-/Property-Management), angetrieben von einem
    Fest-Takt-`QEMUTimer` statt der Poll-einmal-pro-Hauptschleifenrunde-
    Aufrufstelle des Originals (das eigentliche Frame-Sende-Drosseln
    kommt weiterhin aus dem R19 des CRTC, wie im Original). Socket-
    Einrichtungsfehler (meist: eine andere Q9-Flux-Instanz belegt den
    Port schon) sind Warnungen, keine fatalen
    Maschinenstart-Fehler — passend zur eigenen
    Graceful-Degradation-Philosophie des Originals, anders als eine
    frühere Version dieses Ports, die bei einem Portkonflikt die ganze
    Maschine abstürzen ließ. Gegen einen echten Python-Client verifiziert,
    der das tatsächliche Wire-Protokoll spricht: VIDEO_INFO meldete
    genau die im MC6845 programmierte Geometrie (inklusive der
    bpp<8-Stride-zu-Pixelbreite-Umrechnung), PALETTE meldete genau den
    geschriebenen CLUT-Eintrag, und die Nutzlast von FRAME_FULL war
    **byte-genau identisch** zum vom Gast-Testprogramm geschriebenen
    Framebuffer-Inhalt.

Alle elf ursprünglichen Musashi-`src/devices/`-Komponenten (zehn
gastsichtbare Peripheriegeräte plus die Host-seitige Video-Bridge) haben
jetzt ein verifiziertes QEMU-Gegenstück.

**Repository-Aufbau**, aufgeteilt danach, was die Dateien tatsächlich
sind, nicht danach, wo QEMU sie haben will:

- `devices/<gerät>/` — unsere eigenen Geräte-Simulationen, unabhängig
  von QEMUs Verzeichniskonvention benannt/organisiert (spiegelt
  `Q9-Flux-68k/src/devices/<gerät>/` aus dem Musashi-Zweig). Aktuell:
  `devices/rtc72421/q9_rtc72421.c`, `devices/timer_irq/q9_timer_irq.c`,
  `devices/duart68681/q9_duart68681.c`, `devices/cf/q9_cf.c`,
  `devices/remap/q9_remap.c`, `devices/mc6845/q9_mc6845.c`,
  `devices/clut/q9_clut.c`, `devices/framebuf/q9_framebuf.c`,
  `devices/quicc/q9_quicc.c`, `devices/nettty/q9_nettty.c`,
  `devices/videobridge/q9_videobridge.c`.
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

**Echter Reset-Vektor-Boot-Pfad ergänzt**, damit eine echte Boot-ROM
ohne `-kernel`-Krücke laufen kann: wird `-bios` übergeben und `-kernel`
nicht, liest `q9board.c` jetzt die initiale SSP/PC aus den
ROM-Adressen 0/4 (beide Big-Endian-Longs, über den noch nicht
ausgelösten ROM-Mirror) — genau wie echte 68k-Hardware beim
Einschalten — statt wie bei jedem bisherigen Gerätetest `env->pc` aus
einem `-kernel`-Ladevorgang zu erzwingen.

**Erster echter Firmware-Boot-Versuch** (2026-09-15, am selben Tag wie
die obige Geräte-Portierungsarbeit): die tatsächliche
Produktiv-Boot-ROM (`local_images/roms/romimage.dev.running.BIN`,
dieselbe, die der Musashi-Zweig heute bootet) mit einer echten
`OS9SYS.hda` als Onboard-CF-Master-Einheit angehängt (`-global
q9-cf.image=...`, auf einer Wegwerf-APFS-Klon-Kopie, nie dem
Master-Image). Teilerfolg: die ROM lief weit genug, um REMAP zu
erreichen und korrekt auszulösen — per Monitor bestätigt, Adresse 0
schaltete von den eigenen ersten ROM-Bytes auf genulltes RAM um,
während $FE000000 begann, den fest verdrahteten
Remap-Positions-Inhalt der ROM zu zeigen — genau der Mechanismus, der
zuvor isoliert verifiziert wurde. Danach schloss der Boot aber noch
nicht sauber ab, unten diagnostiziert.

**Ursache 1, gefunden und behoben**: ein `-d int`-Trace plus eine
statische Disassemblierung der ROM (capstone) verortete den ersten
Absturz auf eine sehr frühe, beabsichtigte FPU-Erkennungssonde
(`fmove.l (a0),fp0`), deren F-Line-Trap eigentlich vom eigenen
ROM-Handler abgefangen und als "keine FPU vorhanden" behandelt werden
sollte — nur landete der Trap, während der F-Line-Vektor des Gasts
noch unbelegt war (Adresse 0, direkt nach REMAP, bevor die ROM mehr
als die Reset-SSP/PC ins RAM geschrieben hatte), und schickte die CPU
in genulltes RAM und schließlich in eine echte Address-Error-Lawine.
Die eigentliche Ursache lag eine Ebene tiefer: die FPU-Opcodes dieses
Boards verwenden Koprozessor-ID 0 (`$F0xx`), aber QEMUs m68k-Kern
erkennt nur Koprozessor-ID 1 (`$F2xx`, die übliche
MC68881/68882-Verdrahtung) als FPU — alles andere im
`$F0xx`-`$FFxx`-Bereich fällt durch auf "undefiniert", auch
CpId 0 (bestätigt anderweitig unbenutzt in QEMUs eigener
Decodier-Tabelle: die 68030-PMMU-Unterstützung nutzt CpId 2). Der
FPU-Kern des originalen Musashi-basierten Emulators kennt gar keine
solche CpId-Prüfung, weshalb diese ROM dort schon immer funktioniert
hat. Behoben mit einer minimalen, gezielten Ergänzung an QEMUs eigener
Opcode-Decodier-Tabelle — `overlay/patches/0002-m68k-fpu-pre68040.patch`
gegen `target/m68k/translate.c`, die CpId 0 auf denselben Handler wie
CpId 1 aliast, ohne CpId 1s eigenen Eintrag oder sonst etwas
anzufassen (bestätigt: kein anderer Decodier-Eintrag beansprucht
CpId 0 für diese CPU-Familie). **Behebung bestätigt**: nach dem
Neu-Bauen kam der Boot deutlich weiter, und die DUART-Konsole begann
echten ROM-Diagnosetext auszugeben, statt still zu entgleisen.

**Ursache 2, gefunden und behoben**: der Konsolentext selbst zeigte
genau drauf — `Exception Error, vector offset $0010 addr $FE001E84`,
wiederholt ("Fatal System Error; rebooting system"). Diese Adresse ist
eine `frestore`-Instruktion, die einen NULL-FPU-Zustands-Rahmen
wiederherstellt (Format-Byte `$00`, der laut 68881/68882-Architektur
immer trivial erfolgreiche Fall). QEMUs
`DISAS_INSN(frestore)`/`fsave`-Implementierungen waren aber an
`M68K_FEATURE_M68040` gebunden und fielen für jeden anderen CPU-Typ,
`m68030` eingeschlossen, direkt auf `disas_undef()` durch — QEMUs
m68k-Kern hatte FSAVE/FRESTORE-Unterstützung nur für das interne
FPU-Zustandsformat des 68040, nicht für das klassische
68020/68030-plus-externem-68881/68882-Koprozessor-Protokoll, auf das
sich diese ROM (und, ihrem Erfolg dort nach zu urteilen, Musashis
Emulation) verlässt. `overlay/patches/0002-m68k-fpu-pre68040.patch`
(dieselbe Datei wie bei Ursache 1s Fix, dasselbe zugrundeliegende
Thema: FPU-Koprozessor-Lücken für Nicht-68040-CPUs) um einen
`M68K_FEATURE_FPU`-Zweig für beide Instruktionen erweitert — `FRESTORE`
liest ein Long-Wort und verwirft es, genau wie der bestehende
68040-Fall es bereits tut (der validiert den Rahmeninhalt ebenfalls
nicht, s. dessen eigenes "FIXME: check the state frame"), und `FSAVE`
schreibt einen NULL-Rahmen (Format-Byte 0, das Gegenstück der
68881/68882-Familie zum "immer IDLE schreiben" des 68040-Falls) —
korrekt und ausreichend, da die FPU dieses Boards QEMUs eigene
synchrone Software-Gleitkomma-Implementierung ist, ohne echten
Koprozessor-Pipeline-Zustand, der je zu melden wäre.

**Behebung bestätigt, mit dramatischem weiteren Fortschritt**:
Neu gebaut und denselben Boot mit echter ROM + echter `OS9SYS.hda`
erneut versucht. Die DUART-Konsole zeigt jetzt das echte
OS-9-Bootstrap-Banner, findet und validiert erfolgreich eine
Boot-Datei auf dem CF-Image, und **der OS-9-Kernel selbst beginnt zu
laden und zu laufen** — eine Ebene jenseits der ROM:

```
OS-9/68K System Bootstrap
Now trying to boot from CompactFlash.
A valid OS-9 bootfile was found.
ioman: can't open console device: Error $00CB
Error: system state exception; vector $00DC at addr $0000FB1A
--> System Reset <--
```

Das bestätigt: der CF-Port liest echte OS-9-Dateisystemstrukturen
korrekt unter echter Firmware, nicht nur in isolierten
Register-Ebenen-Tests. Der neue Fehler (`ioman` — OS-9s
Ein-/Ausgabe-Manager — kann das Konsolengerät nicht öffnen) ist ein
eigenständiges, späteres Problem, das sich als zwei getrennte Ursachen
herausstellte.

**Ursache 3a, gefunden und behoben (samt einer Selbstkorrektur
unterwegs)**: eine `-d int`-Spur des Fehlers zeigte direkt nach der
`ioman`-Meldung eine zweite Exception — `INT 31: FP Unimplemented Data
Type(0xdc) pc=0000fb1a`. Live-Disassemblierung über den QEMU-Monitor
(`xp`, da diese PC-Adresse im von der CF geladenen Kernel-RAM liegt,
nicht im ROM) zeigte die tatsächliche Instruktion: `pmove %a1@,%crp`
(Opcode `$F011`) — eine echte PMMU-Instruktion, keine FPU-Instruktion,
an derselben Koprozessor-ID 0, die Ursache 1s Fix auf den
FPU-Handler geliehen hatte. Dessen breitere Ursprungsform
(`$F000`-`$F03F`, bevor sie auf ein exaktes `$F010`-Match verengt
wurde) hatte `pmove`s Nicht-FPU-Extension-Wort in den FPU-Decoder
umgeleitet und dabei verfälscht — eine Regression, die der eigene Fix
von Ursache 1 selbst eingeführt hatte, kein vorbestehender Bug. Das
Verengen auf den einen nachweislich benötigten Opcode behob *dieses*
Fehlleiten, aber ein erneutes Hinterfragen der ganzen Prämisse förderte
die eigentliche Geschichte zutage: Koprozessor-ID 0 ist nicht "die FPU
dieses Boards, unkonventionell verdrahtet" — sie ist die eingebaute
**PMMU** des MC68030, die echte 68030-Silizium per Hardware
bedingungslos unter CpId 0 beantwortet, unabhängig von der
Board-Verdrahtung. Die echte FPU (externe 68881/68882, falls
vorhanden) ist CpId 1 (`$F2xx`), bereits separat behandelt und von all
dem unberührt. Der schmale FPU-Alias-Hack wurde durch einen eigenen
CpId-0-Stub ersetzt (`DISAS_INSN(pmove_cpid0)` in
`overlay/patches/0002-m68k-fpu-pre68040.patch`), der den gesamten
cpGEN-CpId-0-Bereich als "annehmen und verwerfen" behandelt — das
(Handler-interne) Extension-Wort und einen EA-Operanden konsumierend,
dieselbe "kein echter Koprozessor-Pipeline-Zustand zu modellieren"-
Vereinfachung, die für FRESTORE/FSAVE bereits etabliert ist —, ohne je
den echten FPU-Dispatch-Code zu berühren. Das entspricht auch dem
originalen Musashi-basierten Emulator, der ebenfalls kein `pmove`
unterstützt und es demselben generischen `EXCEPTION_1111`
(F-Line)-Fallback überlässt wie jeden anderen unimplementierten
Koprozessor-Opcode. **Behebung bestätigt**: die
F-Line/FP-Exception-Absturzschleife ist weg; die CPU erreicht jetzt
eine saubere `stop #$3000`-Scheduler-Leerlaufwartung, statt in
uninitialisiertes RAM zu entgleisen.

**Ursache 3b, gefunden und behoben**: mit dem behobenen Absturz
erschien die Meldung `ioman: can't open console device: Error $00CB`
weiterhin. Eine Register-Zugriffsspur des DUART (temporäre
`Q9_DUART_TRACE`-Instrumentierung, nicht committet) zeigte die genaue
Sequenz direkt vor dem Fehler: ein Lesezugriff auf das IVR
(Interrupt Vector Register), der `$00` liefert, unmittelbar gefolgt
vom Deaktivieren der Interrupts und der Fehlerausgabe. Echtes
MC68681-Silizium startet mit IVR = `$0F` ("uninitialisierter Vektor"),
nicht `$00` — und der OS-9-Treiber `sc68681` liest IVR beim Öffnen
genau deshalb zurück, um exakt diesen Reset-Wert zu prüfen und so zu
bestätigen, dass er es mit einem echten Chip zu tun hat. Der originale
Musashi-basierte Emulator weiß das bereits: `Q9-Flux-68k/src/kernel/
q9board.c`s `q9_board_init` setzt `uart_ivr` explizit auf `0x0F`, mit
einem Kommentar, der genau diese Prüfung benennt. Unser QEMU-Port
hatte für `Q9Duart68681State` keine solche Initialisierung und blieb
beim Nullwert aus QOMs Standard-Initialisierung. Behoben durch
`s->ivr = 0x0F` in `q9_duart68681_realize()`
(`devices/duart68681/q9_duart68681.c`). **Behebung bestätigt**: der
`ioman`-Konsolen-Öffnen-Fehler ist weg; der Boot läuft daran vorbei
weiter und beginnt das nächste Kernel-Modul zu laden:

```
OS-9/68K System Bootstrap
Now trying to boot from CompactFlash.
A valid OS-9 bootfile was found.
CompactFlash driver build 42
```

**Nächste Schritte**: der Boot erreicht jetzt eine *andere*
`stop #$3000`-Scheduler-Leerlaufwartung (derselbe Leerlaufschleifen-
Code wie bei Ursache 3as eigenem Vorher/Nachher-Vergleich, bei
`$a2e4`/`$a2e8`), ein Stück weiter im Laden des CF/RBF-Moduls, und
diese wacht nie wieder auf — vereinbar damit, dass ein Gerät, von dem
das Startup-Skript jetzt abhängt, (noch) keinen Interrupt auslöst, den
QEMU liefern sollte (der Completion-IRQ des CF-Controllers ist der
naheliegendste Verdächtige, noch nicht abschließend als Ursache
bestätigt); dann schauen, wie weit dieselbe ROM/CF-Kombination von
dort aus kommt; danach die grundsätzlichere Board-Ebene —
config-gesteuerte Geräte-Instanziierung (heute ist alles in
`q9board.c` noch fest verdrahtet/per `-global` konfiguriert, passend
zum eigenen Vor-boardcfg.c-Zustand des Originals, statt über eine
deklarative Board-Config-Datei gesteuert zu werden, wie `boardcfg.c`
das im Musashi-Zweig tut) — und irgendwann der eigentliche
[Host-Passthrough-Dateisystem-Manager](docs/HOSTFS_MANAGER_de.md), für
den diese ganze Umstellung ursprünglich begonnen wurde (s. "Warum QEMU"
oben).

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
