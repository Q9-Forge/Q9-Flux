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

## Q9-eigene Änderungen am Vendor-Code

Bisher keine. Jede Änderung sollte im Code mit `Q9` markiert und hier aufgeführt
werden, so wie es `third_party/musashi/Q9_VENDOR.md` vormacht.
