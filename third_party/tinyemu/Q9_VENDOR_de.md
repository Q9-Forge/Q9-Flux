# TinyEMU RISC-V-Kern — Vendor-Notiz

*English version: [Q9_VENDOR.md](Q9_VENDOR.md)*

**Quelle**: https://github.com/fernandotcl/TinyEMU (Commit `56ba49be40a3d24b65bcb3ec2180f2bdddd7b0c3`, 2020-05-25)
**Urheber**: Fabrice Bellard, gepflegter Fork von Fernando Tarlá Cardoso Lemos
**Lizenz**: MIT (`MIT-LICENSE.txt`, zusätzlich Lizenzkopf in jeder Quelldatei)
**Vendoriert**: 2026-08-12

## Was ist hier drin (nur der CPU-Kern)

Nur der RISC-V-**CPU-Kern** plus die zwei Hilfseinheiten, die er braucht — bewusst
*nicht* der umgebende Systememulator. Q9-Flux bringt Bus, Gerätemodell
(`src/devices/<name>/`), Board-Konfiguration und Syscall-Schicht selbst mit;
gefehlt hat allein die CPU, in genau der Rolle, die Musashi für 68k ausfüllt.

| Datei | Zweck |
|---|---|
| `riscv_cpu.c`, `riscv_cpu.h` | CPU-Kern + öffentliche Schnittstelle |
| `riscv_cpu_priv.h` | interner CPU-Zustand |
| `riscv_cpu_template.h` | der Dekoder, je Wortbreite einmal instanziiert |
| `riscv_cpu_fp_template.h` | Gleitkomma-Instruktionen, ebenso |
| `iomem.c`, `iomem.h` | physische Speicherkarte: RAM-Bereiche + MMIO-Rückrufe |
| `cutils.c`, `cutils.h` | kleine Helfer, die obige brauchen |
| `softfp.c`, `softfp.h`, `softfp_template.h`, `softfp_template_icvt.h` | Software-Gleitkomma für die F/D-Erweiterungen |

**Bewusst nicht übernommen**: `riscv_machine.c` (das Board), `virtio.c`, `vga.c`,
`ide.c`, `fs_*.c`, `sdl.c`, der Netzwerkteil — und der gesamte x86-Zweig
(`x86_cpu.c`, `x86_machine.c`). Genau diese Teile hat Q9-Flux bereits in eigener
Form.

## Warum dieser Kern

Entscheidend war nicht die Erweiterungsliste, sondern die **Form**. Musashi ist
*nur* eine CPU: Speicherzugriffe laufen über Rückrufe, die der Wirt stellt.
Deshalb steht es reibungsfrei neben `devreg.h` und `boardcfg`. Der TinyEMU-Kern
hat dieselbe Form, und `iomem.h` ist der ganze Vertrag:

```c
cpu_register_ram   (map, addr, size, ...)
cpu_register_device(map, addr, size, opaque, read_func, write_func, ...)
```

Wirt-eigene Lese-/Schreibrückrufe mit `opaque`-Zeiger — begrifflich das, was
`devreg.h` schon macht. Die CPU-Schnittstelle bildet die in ARBEITSPLAN-Schritt
6.5 geplante vtable fast eins zu eins ab:

| geplante vtable | TinyEMU | Musashi |
|---|---|---|
| `execute` | `riscv_cpu_interp(s, n_cycles)` | `m68k_execute(n)` |
| `set_irq` | `riscv_cpu_set_mip/reset_mip` | `m68k_set_irq()` |
| `is_stopped` | `riscv_cpu_get_power_down()` | `m68k_is_stopped()` (unsere Ergänzung) |
| `ctx` | `RISCVCPUState*` | globaler Zustand |

Geprüfte Alternative war **RVVM** (C99, aktiv gepflegt, mehr Erweiterungen heute,
MPL-2). Verworfen, weil es ein vollständiges eigenes Geräte- und
Maschinen-Framework mitbringt (`rvvm_mmio_dev_t` mit
remove/update/reset/suspend/resume-Lebenszyklus). Das stünde neben dem
Geräte-Framework von Q9-Flux — zwei Frameworks in einem Emulator — und 78
C-Dateien lassen sich nicht auf einen kleinen Kern eindampfen. **libriscv** war
vorher ausgeschieden: C++17, und solange QCC kein C++ übersetzt, wäre das ein
Teil des Baums, den wir nicht selbst übersetzen können.

## Die Wortbreite ist ein Übersetzungsschalter

`riscv_cpu.c` bindet `riscv_cpu_template.h` dreimal ein, mit `XLEN` 32, 64 und
128. Eine Instruktion wird **einmal** geschrieben, gegen den parametrisierten Typ
`intx_t`, und existiert dann in allen drei Breiten. `MAX_XLEN` wählt, wie viel
davon gebaut wird:

| `MAX_XLEN` | `riscv_cpu.o` | wofür |
|---|---:|---|
| 32 | 40,6 KB | nur RV32 — der kleine Bau für spätere Projekte, die bloß Q9-interne Syscall-Emulation brauchen |
| 64 | 66,9 KB | RV32 + RV64 |
| 128 | 141,2 KB | alles |

Das ist zugleich die Antwort zur Erweiterbarkeit: eine neue Erweiterung ist ein
`case` in einem lesbaren `switch`, einmal geschrieben für alle Wortbreiten — kein
neuer Emulator.

## Prüflauf (2026-08-12, vor dem Commit)

Jede Übersetzungseinheit übersetzt einzeln und **warnungsfrei** mit
`cc -std=c99 -Wall`:

```
MAX_XLEN=32    riscv_cpu.o     40648 Byte    0 Warnungen
MAX_XLEN=64    riscv_cpu.o     66872 Byte    0 Warnungen
MAX_XLEN=128   riscv_cpu.o    141240 Byte    0 Warnungen
               iomem.o          5376 Byte    0 Warnungen
               cutils.o         2464 Byte    0 Warnungen
               softfp.o        65160 Byte    0 Warnungen
```

`riscv_cpu.c` verlangt **beide** Schalter `-DMAX_XLEN=<n>` und
`-DCONFIG_RISCV_MAX_XLEN=<n>`; fehlt einer, bricht die Übersetzung sofort ab.
Noch NICHT in den Q9-Makefile-Bau eingebunden — das ist Schritt 6.8
(`TARGET=m68k|riscv32|x86_32`).

Anders als Musashi braucht dieser Kern **keinen Generatorschritt**: kein
`m68kmake`-Gegenstück, gewöhnliches C genügt.

## Bekannte Einschränkung: Upstream ruht

Der letzte Commit oben stammt vom 2020-05-25. Ab dem Vendoren gehört dieser Kern
uns. Das war eine bewusste Entscheidung: die RISC-V-Basis-ISA ist ratifiziert und
eingefroren, ein vollständiger Kern verrottet also nicht, und mit 6146 Zeilen ist
er klein genug, um ihn zu besitzen. Bei Musashi ist die Lage dieselbe.

## Gemessen gegen die offizielle ISA-Suite (2026-08-12)

`make test-riscv` lässt die RISC-V-Suite `riscv-tests` gegen diesen Kern laufen —
ganz ohne Board, nur RAM und das HTIF-Meldewort. Schlägt dort etwas fehl, kennt
man die *Instruktion*, nicht bloß „bootet nicht".

RV32, Basisgruppen: **95 von 104**.

| Gruppe | | Gruppe | |
|---|---|---|---|
| `rv32ui` Basis-Integer | **42/42** | `rv32uf` Float | 10/11 |
| `rv32um` Mul/Div | **8/8** | `rv32ud` Double | 9/10 |
| `rv32uc` Compressed | **1/1** | `rv32mi` Machine-Mode | 11/16 |
| `rv32ua` Atomics | 9/10 | `rv32si` Supervisor | 5/6 |

Der Nutzer-Instruktionssatz ist vollständig. Jeder Fehlschlag liegt in einer
**nicht implementierten Funktion**, nicht in falscher Instruktionsbedeutung — am
Quelltext nachgeprüft:

| Fehlschlagender Test | Ursache im Kern |
|---|---|
| `rv32mi-p-pmpaddr` | PMP gibt es gar nicht (0 Nennungen `pmpaddr`/`pmpcfg`) |
| `rv32mi-p-breakpoint` | Debug-Trigger `tdata1`/`tselect` fehlen |
| `rv32mi-p-mcsr`, `-instret_overflow` | `mcountinhibit` fehlt |
| `rv32si-p-dirty` | `PTE_A`/`PTE_D` vorhanden, aber unvollständig |
| `rv32ua-p-lrsc` | keine Reservierungsverwaltung für LR/SC |

**Für die nächsten Schritte wichtig**: die unvollständige `PTE_A`/`PTE_D`-Behandlung
wird relevant, sobald ein echtes Betriebssystem mit Paging läuft (xv6), und LR/SC
bei Mehrkernbetrieb. Die übrigen (PMP, Debug-Trigger, Zählersperre) sind für Q9
vorerst ohne Belang.

Die Erweiterungsgruppen Zba/Zbb/Zbc/Zbs/Zbkb/Zbkx/Zfh/Zicond (52 weitere Tests)
scheitern geschlossen mit „unerlaubte Instruktion" — dieser Kern ist älter (2017).
Sie gehören bewusst **nicht** zur Basis, denn eine dauerhaft rote Suite wird
ignoriert. `test/riscv/run-isa-tests.sh` hält deshalb den obigen Stand fest und
meldet nur *Veränderungen*, in beide Richtungen.

## Q9-eigene Änderungen am Vendor-Code

1. **`riscv_cpu.c` — `case 64:` in `riscv_cpu_init()` bedingt gemacht**
   (2026-08-12). Der `case 128:` darunter war bereits mit
   `#if CONFIG_RISCV_MAX_XLEN == 128` abgesichert, dieser nicht. Folge: ein
   reiner 32-Bit-Bau (`CONFIG_RISCV_MAX_XLEN=32`) **übersetzte, ließ sich aber
   nicht linken** — der Verteiler verwies auf `riscv_cpu_class64`, das in diesem
   Bau nicht entsteht. Der oben zugesagte kleine RV32-Bau funktionierte also bis
   zu dieser Korrektur gar nicht. Rein additive Bedingung, Verhalten für
   `CONFIG_RISCV_MAX_XLEN >= 64` unverändert. Im Code mit `Q9` markiert.

2. **`riscv_cpu.c` — `MIP_MEIP` zur Schreibmaske von CSR 0x304 (`mie`)
   ergänzt** (2026-08-12). Die Maske lautete `MIP_MSIP | MIP_MTIP | MIP_SSIP
   | MIP_STIP | MIP_SEIP` — jedes Freigabebit außer dem für externe
   Machine-Interrupts (Bit 11). Ein Gast konnte externe Interrupts damit NIE
   per `mie`-Schreibzugriff freigeben: jeder Versuch (`csrs mie, ...`/`csrw
   mie, ...`, dieses Bit zu setzen, wurde stillschweigend verworfen. Die
   Maske des PENDING-Registers (`mip`, CSR 0x344) schließt `MIP_MEIP` bewusst
   aus — das ist dort korrekt, eine echte Maschine setzt das
   Anliegend-Bit für externe Interrupts hardwareseitig (hier:
   `riscv_cpu_set_mip()`), nicht per Gast-Schreibzugriff. Das FREIGABE-Bit in
   `mie` ist aber immer softwaregesteuert und muss schreibbar sein.

   Gefunden beim Booten von NuttX (`rv-virt:nsh`, s. `docs/RISCV_de.md`): das
   System bootete bis zum Shell-Prompt, PLIC/CLINT auf der Q9-Seite waren
   nachweislich korrekt (`mip` zeigte Bit 11 genau dann gesetzt, wenn
   erwartet), aber keine Tastatureingabe kam an. Eine einzeilige
   Testausgabe in `raise_interrupt()` zeigte `mip=0x880` (MTIP UND MEIP
   anliegend) neben `mie=0x80` (nur MTIE freigegeben) — die obige Sperre war
   die Ursache, kein Fehler im Board-Code. Rein additiv zur Maske; jeder
   zuvor bestandene Fall (`mie`-Werte, die Bit 11 nie berührten) bleibt
   bitgleich. Im Code mit `Q9` markiert. Regressionsabsicherung, die nicht
   die volle NuttX-Werkzeugkette braucht: `make test-rvextirq`
   (`test/riscv/extirq/`), gegengeprüft, dass sie genau diesen Fehler
   tatsächlich fängt, sollte er wiederkehren.

Jede weitere Änderung sollte im Code mit `Q9` markiert und hier aufgeführt
werden, so wie es `third_party/musashi/Q9_VENDOR.md` vormacht.
