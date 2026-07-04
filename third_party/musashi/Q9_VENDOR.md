# Musashi — Vendor-Notiz

**Quelle**: https://github.com/kstenerud/Musashi (Commit `313ebf1bd9f4d0d93341eb5ce21fd8a119e9dbdd`, 2026-03-08)
**Lizenz**: MIT (Text direkt in `m68k.h` und den anderen Quelldateien, Copyright Karl Stenerud —
kein "nur nicht-kommerziell", das war eine ältere Fassung, siehe PROJECT.md Entscheidung E12)
**Vendoriert**: 2026-07-04

## Was ist hier drin (minimal, analog zu `third_party/wasm3/`)

Nur der Kern-Interpreter + Codegenerator + Softfloat-Unterbau — kein Disassembler
(`m68kdasm.c`), keine Testtreiber/Beispiele aus dem Originalrepo:

- `m68k.h`, `m68kconf.h` — öffentliche API + Konfiguration
- `m68kcpu.c/.h`, `m68kfpu.c`, `m68kmmu.h` — CPU-Kern
- `m68kmake.c`, `m68k_in.c` — Codegenerator: `m68kmake` liest `m68k_in.c` und erzeugt
  `m68kops.c`/`m68kops.h` (1967 Opcode-Handler aus 518 Primitiven) — **das sind
  Build-Artefakte, nicht Teil dieses Vendor-Verzeichnisses**, werden vom Makefile
  zur Bauzeit erzeugt (analog zum wasm3-Muster mit generierten .o-Dateien)
- `softfloat/` — Gleitkomma-Unterbau, den `m68kfpu.c` braucht

## Sanity-Check (2026-07-04, vor dem Commit)

Alle drei Kern-Übersetzungseinheiten kompilieren einzeln fehlerfrei (gcc -std=c99):
`m68kcpu.c`, `m68kops.c` (generiert), `softfloat/softfloat.c`. Noch NICHT in den
Q9-Makefile-Build integriert — das ist Teil des ersten Schritts der neuen Phase 5
(ARBEITSPLAN.md).

## Bewusste Wahl: 68030 als Ziel-CPU-Typ

Die reale Zielhardware (MC68EN360/QUICC) hat einen CPU32+-Kern, den Musashi nicht
kennt. Entscheidung E12 (PROJECT.md): 68030 als nächstliegender, gut unterstützter
Musashi-Typ — MMU bleibt einfach ungenutzt (kein PMOVE im generierten Code). Damit
das später nicht die Tür zur echten CPU32-Hardware zuschlägt: Der von uns
compilierte 68k-Code (vbcc) sollte sich auf einen gemeinsamen, konservativen
Befehlssatz beschränken, nicht auf 68030-exklusive Features verlassen.
