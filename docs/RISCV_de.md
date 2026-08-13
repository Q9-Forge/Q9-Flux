# RISC-V-Bring-up

*English version: [RISCV.md](RISCV.md)*

Stand: **2026-08-12 -- Stufe 3 abgeschlossen. Ein echtes, unverändertes
Gastbetriebssystem (NuttX, `rv-virt:nsh`) bootet und läuft interaktiv auf dem
Q9-RISC-V-Kern: Shell-Prompt, Tastatureingabe, ein echtes Programm (`hello`)
wird ausgeführt.**

## Warum dieses Dokument

Vorarbeit für ARBEITSPLAN.md Phase 6 ("Mehrarchitektur"): bevor ein zweiter
CPU-Backend seine eigene `cpu_backend.h`-vtable bekommt (Schritt 6.5), muss
erst etwas darauf laufen. Genau das ist hier entstanden, samt allem dabei
Gelernten. Vor dem Anfassen von `src/devices/{clint,plic,uart16550}/` oder
`third_party/tinyemu/` lesen -- drei echte Fehler stehen hier, jeder teuer
neu zu entdecken.

## Die gestufte Vorgehensweise, und warum sie wichtig war

Der naheliegende erste Versuch war ein echtes Betriebssystem (xv6 oder
RT-Thread Smart). Beide erwiesen sich als Sackgasse für einen *ersten*
Schritt: xv6 braucht RV64 + Sv39-Paging; RT-Thread Smarts MMU-/
Userspace-Schicht (`components/lwp/arch/risc-v/`) ist im Quellbaum RV64-only
(am Baum selbst geprüft, nicht einer Marketingaussage geglaubt, die "32- und
64-Bit, ARM und RISC-V" behauptet, ohne zu sagen *welche Kombination*).
Beide hätten zudem sofort eine bekannte Lücke im vendorierten Kern getroffen
(`PTE_A`/`PTE_D` unvollständig, s. `third_party/tinyemu/Q9_VENDOR_de.md`) --
ein schlechter Startpunkt, denn ein Fehler im Seitenfehler-Handler eines
echten Betriebssystems ist weit schwerer zu diagnostizieren als einer in
einem isolierten CPU-Test.

Der Weg ging deshalb von unten nach oben, jede Stufe bewies eine Sache
isoliert, bevor die naechste sie in etwas Groesserem haette verstecken
koennen:

| Stufe | Beweist | Geräte | Test |
|---|---|---|---|
| 1 -- ISA-Suite | den CPU-Kern selbst, Instruktion für Instruktion | keine (nur RAM) | `make test-riscv` |
| 2 -- UART-Lebenszeichen | den Geräte-Rückrufpfad (`cpu_register_device`), einen echten Gastbau | RAM, UART | `make test-rvboard` |
| 2b -- CLINT-Timer | Interrupt-Zustellung (`mip`/`mie`, `wfi`) | RAM, UART, CLINT | `make test-rvtimer` |
| 3 -- NuttX-Boot | ein echtes OS, externe Interrupts, ein zweiter Interruptcontroller | RAM, UART, CLINT, PLIC | `make test-rvnuttx` |
| 3-leicht -- `mie.MEIE`-Regression | genau der in Stufe 3 gefundene Kernfehler, isoliert, ohne NuttX-Werkzeugkette | RAM, UART, PLIC (kein CLINT) | `make test-rvextirq` |

Jede Stufe fand etwas, das die vorherige nicht hätte zeigen können. Das ist
der Sinn dieses Aufbaus, statt direkt auf ein Betriebssystem zu zielen.

## Stufe 1 -- ISA-Testsuite (`make test-riscv`)

Lässt die offizielle `riscv-tests`-Suite gegen den vendorierten CPU-Kern
laufen, bewusst **ganz ohne Board** -- nur RAM und das HTIF-Meldewort (s.
`riscv-tests/env/p/riscv_test.h`). Ein Fehlschlag hier nennt die
*Instruktion*, nicht "bootet nicht".

RV32-Ergebnis Basisgruppen: **95 von 104**. Der Nutzer-Instruktionssatz ist
vollständig; jeder Fehlschlag geht auf eine echte fehlende Funktion zurück
(PMP gibt es nicht, Debug-Trigger fehlen, `mcountinhibit` fehlt, `PTE_A`/
`PTE_D` sind unvollständig, LR/SC hat keine Reservierungsverwaltung) -- nicht
auf falsche Instruktionsbedeutung. Vollständige Aufschlüsselung, inklusive
welche davon für spätere Stufen wichtig sind, in
`third_party/tinyemu/Q9_VENDOR_de.md`.

`test/riscv/fetch-isa-tests.sh 32` holt und baut die Suite (nicht
eingecheckt, Fremdmaterial); `test/riscv/run-isa-tests.sh` vergleicht gegen
einen festgehaltenen Stand und meldet nur *Veränderungen* -- eine dauerhaft
rote Suite wird ignoriert, deshalb bleiben die Erweiterungsgruppen, die der
Kern nicht kennt (Zba/Zbb/Zbc/Zbs/Zbkb/Zbkx/Zfh/Zicond, 52 weitere Tests,
alle mit "unerlaubte Instruktion", der Kern ist von 2017), bewusst außerhalb
der Basis.

## Stufe 2 -- UART-Lebenszeichen (`make test-rvboard`)

Erstes eigenes Gastprogramm (`test/riscv/hello/`), erste echte
Geräteanbindung (`cpu_register_device` mit eigenen Lese-/Schreibrückrufen).
Kein libc -- Homebrews `riscv64-elf-gcc` bringt keines mit, und ein
Lebenszeichen braucht keines.

## Stufe 2b -- CLINT-Timer-Interrupts (`make test-rvtimer`)

Erster funktionierender Interrupt: ein Bare-Metal-Trap-Handler zählt fünf
periodische Timer-Interrupts und hält sauber an. Zwei Fehler wurden dabei
gefunden und behoben, beide sind die tragenden Lehren für die
PLIC-Arbeit in Stufe 3:

1. **`wfi` wacht nur über einen expliziten API-Aufruf auf.**
   `riscv_cpu_interp()` wertet `power_down_flag` nie von selbst neu aus; nur
   `riscv_cpu_set_mip()` setzt es zurück (wenn `mip & mie` nach dem Aufruf
   ungleich null wird). Ein Board-Treiber, der nur die Zeit voranstellt und
   hofft, dass die CPU es bemerkt, ist falsch -- er muss `set_mip`/
   `reset_mip` selbst aufrufen.
2. **Ein Interrupt-Sturm durch ein veraltetes `mip`-Bit.** `mip.MTIP` ist in
   diesem Kern ein *Zwischenspeicher*, nicht das dauerhaft aktuelle Signal
   `mtime >= mtimecmp`, das echte Hardware hat. Ein Abgleich nur einmal pro
   Wirt-Zeitabschnitt ließ den Trap-Handler Dutzende Male nachfeuern, bevor
   der nächste Abgleich aufholte (gemessen: 33 Interrupts statt 5). Der Fix:
   **sofort** bei jedem Ereignis abgleichen, das den Anspruchszustand ändern
   kann -- hier: direkt im MMIO-Schreibrückruf für `mtimecmp`, nicht nur
   periodisch in der Hauptschleife.

Volle Einzelheiten und die Gegenprobe (bewusstes Zurückdrehen des Fixes
reproduziert zuverlässig 33) in `src/devices/clint/clint.h` und der
Commit-Historie.

## Stufe 3 -- NuttX-Boot (`make test-rvnuttx`)

Ergänzt PLIC und brachte die zwei folgenreichsten Funde dieser ganzen
Arbeit.

### PLIC ist nicht optional

NuttX' 16550-Treiber hängt sich per `irq_attach()` in den Interrupt-Pfad;
ohne funktionierenden Interrupt-Pfad bootet das System bis zum
Shell-Prompt (TX ist gepollt), aber **keine Tastatureingabe kommt je an** --
empirisch bewiesen, nicht nur aus dem Quelltext gelesen: "help\n" über
stdin eingespeist ließ den Zyklenzähler mit und ohne Eingabe identisch.

### Die IRQ-Nummern-Falle

`CONFIG_16550_UART0_IRQ=37` in der `.config` ist **nicht** die rohe
PLIC-Quellennummer. NuttX zählt IRQ-Nummern durchgehend -- erst die
internen Ausnahmen/Traps (`RISCV_MAX_EXCEPTION=15`), dann
`RISCV_IRQ_ASYNC=16` als erste asynchrone Nummer, dann externe
PLIC-Quellen ab `RISCV_IRQ_MEXT = RISCV_IRQ_ASYNC+11 = 27` (M-Mode; im
S-Mode wäre es `RISCV_IRQ_SEXT = RISCV_IRQ_ASYNC+9 = 25`). Die echte
PLIC-Quelle ist `irq - RISCV_IRQ_EXT`, also **10**, nicht 37. Bestätigt
durch Verfolgen eines echten `ENABLE1`-Schreibzugriffs beim Booten: Bit 10
(`0x00000400`), nicht Bit 5 (das wäre Quelle 37 gewesen). Wird das
übersehen, schaltet PLIC eine Quelle frei, die nie jemand anhebt -- kein
Fehler, nur Stille, ohne Verfolgen der tatsächlichen Registerzugriffe nicht
von "PLIC funktioniert überhaupt nicht" zu unterscheiden.

### Der Kernfehler: `mie` konnte externe Interrupts nie freigeben

Der tiefste Fund. Selbst mit korrigierter IRQ-Nummer, korrekter
PLIC-Verdrahtung und nachweislich feuerndem `riscv_cpu_set_mip(MIP_MEIP)`
(`mip` zeigte Bit 11 genau dann gesetzt, wenn erwartet) kam nie ein
Interrupt beim Gast an. Eine einzeilige Testausgabe in der `raise_interrupt()`
des Kerns zeigte warum: `mip=0x880` (MTIP UND MEIP anliegend) neben
`mie=0x80` (nur MTIE freigegeben).

Die Ursache lag im CSR-Schreibpfad für `mie` (CSR `0x304`) in
`third_party/tinyemu/riscv_cpu.c`: die Schreibmaske lautete `MIP_MSIP |
MIP_MTIP | MIP_SSIP | MIP_STIP | MIP_SEIP` -- jedes Freigabebit **außer**
`MIP_MEIP` (Bit 11, Machine External Interrupt Enable). Jeder Versuch des
Gastes, dieses Bit per `csrs mie, ...` zu setzen, wurde stillschweigend
verworfen. Das ist kein NuttX-spezifisches Missverständnis: die Maske bei
`mip` (dem *Anliegend*-Register, CSR `0x344`) schließt `MIP_MEIP` zu Recht
aus -- eine echte Maschine setzt dieses Bit hardwareseitig, nicht per
Gast-Schreibzugriff -- aber genau diese Begründung gilt **nicht** für `mie`
(das *Freigabe*-Register), das immer softwareschreibbar sein muss.
Kopieren zwischen den beiden Masken ließ ein Bit fallen, das bei `mie` nie
hätte fallen dürfen. Behoben und dokumentiert (mit `Q9` markiert) in
`third_party/tinyemu/Q9_VENDOR_de.md`; rein additiv, kein vorher
bestandener Fall ändert sich.

Ohne Stufe 1s isolierte, reine CPU-ISA-Suite als gedankliches Modell wäre
dieser Fehler weit schwerer einzuordnen gewesen -- "nie feuert ein externer
Interrupt" sieht identisch aus, egal ob der Fehler im Treiber des
Gastbetriebssystems, in der PLIC-Verdrahtung des Boards, in der
IRQ-Nummern-Umrechnung oder in der CSR-Behandlung des CPU-Kerns liegt. Vom
Kern nach außen zu verfolgen (eine Testausgabe genau dort, wo `mip & mie`
ausgewertet wird), statt auf Board-Ebene zu raten, klärte es in einem
Schritt.

### Regressionsabsicherung ohne NuttX-Abhängigkeit

`make test-rvnuttx` ist der volle Nachweis, läuft aber nur dort, wo die
gesamte NuttX-Werkzeugkette (xPack `riscv-none-elf-gcc`, `kconfig-tweak`,
`genromfs`, `flock`) zufällig installiert ist -- genau die Art Umgebung,
die nicht immer verfügbar sein wird. `make test-rvextirq` isoliert
denselben Fehler eigenständig: ein kleiner Bare-Metal-Gast
(`test/riscv/extirq/`), der PLIC direkt einrichtet (mit einer selbst
gewählten Quellennummer, keine NuttX-IRQ-Umrechnung beteiligt) und
`mie.MEIE` per `csrs mie, ...` setzt -- genau die Zeile, die der Fehler
stillschweigend zunichtemachte. Bewusst ohne CLINT auf diesem Board, damit
nur der externe Interruptpfad geprüft wird. Läuft in deutlich unter einer
Sekunde, ohne mehr als dasselbe `riscv64-elf-gcc`, das die ISA-Suite
ohnehin braucht.

Gegenprobe beim Schreiben durchgeführt: das Zurückdrehen des
`mie`-Masken-Fixes in einer Testkopie des Kerns führt **nicht** dazu, dass
der Test bloß die falsche Zahl meldet -- das `wfi` des Gastes wacht nie auf
(der Trap wird nie genommen), und die begrenzte Abschnittsschleife des
Wirts (`test/rvboard_extirq.c`) erschöpft einfach ihr Budget und endet mit
nur der Startbanner-Zeile, ganz ohne "Interrupts behandelt". In beiden
Fällen sauberes Ende bestätigt (kein Risiko eines hängenden Testprozesses):
~30 ms Realzeit, um 400 Millionen simulierte Zyklen im Leerlauf zu
erschöpfen, denn ein Stromsparzustand-Abschnitt kostet den Wirt praktisch
nichts.

## Geräte-Entwurf: warum nichts hier `src/kernel/devreg.h` benutzt

`src/devices/{uart16550,clint,plic}/` sind bewusst **nicht** über die
bestehende 68k-seitige Geräte-Vtable angebunden. Deren 16/32-Bit-Synthese
aus `read8`/`write8` ist dort als big-endian dokumentiert -- richtig für
68k, falsch für RISC-V (little-endian). Die Verallgemeinerung von
`devreg.h` ist ARBEITSPLAN.md-Schritt 6.7; bis dahin bieten diese drei
Geräte eine schmale byte-/wortorientierte Schnittstelle, die beide
Architekturen nutzen können, ohne die falsche Byte-Reihenfolge in eine von
beiden einzubacken. Jedes Gerät kennt außerdem die CPU nicht (kein
`RISCVCPUState*` in `clint.c`/`plic.c`/`uart16550.c`) -- der Board-Treiber
(`test/rvboard_*.c`) ist die einzige Stelle, die Geräte-Zustand mit
`riscv_cpu_set_mip()`/`reset_mip()` verbindet, genau wie `q9boardrun.c` die
einzige Stelle ist, die sowohl `devreg.h` als auch Musashi kennt.

## Speicherkarte (alle Boards dieses Bring-up)

Folgt bewusst der QEMU-"virt"-Maschine -- damit laufen dieselben
Gastabbilder auch gegen `qemu-system-riscv32`/`64` als Gegenprobe, und
spätere Arbeit (virtio-blk, mehr vom Adressraum) hat eine echte Referenz
statt einer erfundenen.

| Adresse | Gerät | Seit |
|---|---|---|
| `0x00001000` | Reset-Stub (Sprung zum ELF-Einsprung, da der Kern bei `0x1000` startet, aber jeder Gast hier bei `0x80000000` gebunden ist) | Stufe 2 |
| `0x02000000` | CLINT (Timer-/Software-Interrupts) | Stufe 2b |
| `0x0c000000` | PLIC (externe Interruptverteilung) | Stufe 3 |
| `0x10000000` | 16550-UART | Stufe 2 |
| `0x80000000` | RAM (16 MiB Stufe 2/2b, 32 MiB Stufe 3 -- NuttX' `rv-virt:nsh`-Defconfig verlangt es) | Stufe 2 |

## Toolchains

Zwei, aus zwei verschiedenen Gründen -- **nicht ohne erneuten Test
vereinheitlichen.**

- **`riscv64-elf-gcc`** (Homebrew, Multilib: `rv32i` bis `rv64imafdc`) --
  für die ISA-Suite und unsere eigenen Bare-Metal-Gäste
  (`test/riscv/hello/`, `test/riscv/timer/`). Funktioniert sauber für
  `-nostdlib`-Bare-Metal-Code ohne libc-Abhängigkeit.
- **xPack `riscv-none-elf-gcc`** -- speziell für NuttX nötig. Der
  Homebrew-Toolchain zog beim Binden die falsche `libgcc.a`-Breite
  (`ELFCLASS64` in einen 32-Bit-Link); nachdem das durch Erzwingen des
  richtigen Multilib-Pfads umgangen war, trat ein *zweiter*, anderer
  Fehlschlag auf (Gleitkomma-ABI-Widerspruch innerhalb von `libgcc.a`
  selbst). NuttX' eigenes `Toolchain.defs` nennt xPacks `riscv-none-elf-gcc`
  genau aus diesem Grund als bevorzugte Toolchain -- das ist, wogegen NuttX'
  eigene CI tatsächlich testet. Der Wechsel behob beide Fehlschläge auf
  einmal. Zu beziehen von
  <https://github.com/xpack-dev-tools/riscv-none-elf-gcc-xpack/releases>
  (`...-darwin-arm64.tar.gz` auf macOS Apple Silicon), kein Installer nötig,
  nur `bin/` in den `PATH`.

Der Bau von NuttX selbst braucht zusätzlich `kconfig-tweak`, `genromfs` und
`flock` -- keines davon zum Zeitpunkt dieser Zeilen in Homebrews
Haupt-Formeln unter funktionierenden URLs. Vollständiges Rezept, inklusive
einer echten macOS-Falle (eine verirrte `MAKE=os9make -e`-Umgebungsvariable
aus fremder Cross-Toolchain-Arbeit wurde von `./configure` fest in die
erzeugte `Makefile` geschrieben und brach den Bau auf zwei verschiedene
Arten, bevor die Ursache gefunden war), im Kopf von
`test/riscv/fetch-nuttx.sh`.

## Was als Nächstes ansteht

- **6.7** (`devreg.h` verallgemeinern): danach könnten diese drei Geräte
  unter das vereinheitlichte Schema wandern -- nicht dringend, sie
  funktionieren so, wie sie sind, korrekt.
- **`PTE_A`/`PTE_D`** (die noch offene ISA-Lücke aus Stufe 1): nötig, bevor
  ein Sv32-Paging-Gast (NuttX' `knsh32`-Konfiguration, eine echte
  RV32-Alternative zu xv6s RV64/Sv39, die innerhalb des `riscv32`-Ziels
  dieses Projekts bleibt) versucht werden kann, ohne dieselbe Art
  vergrabenen, schwer einzuordnenden Fehler zu riskieren, den der
  Stufe-3-Abschnitt dieses Dokuments beschreibt.
- **virtio-blk**: nötig für NuttX-Konfigurationen, die ELF-Anwendungen aus
  einem echten Dateisystem laden statt aus der hier verwendeten,
  eingebauten `nsh`/`knsh`-Anwendungstabelle.
