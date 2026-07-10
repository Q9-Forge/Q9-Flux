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

## Q9-eigene Änderungen am Vendor-Code (2026-07-05, Schritt 5.4)

Beim ersten Boot des echten Microware-CB030-ROMs (OS-9/68K mit ssm851) stießen wir
auf mehrere Lücken in Musashis FPU-/MMU-/Interrupt-Emulation. Alle Änderungen sind
im Code mit `Q9/CB030` markiert und bewusst minimal-invasiv (MIT-Lizenz erlaubt
Modifikation; Upstream-tauglich, falls je gewünscht):

1. **`m68kfpu.c` — FRESTORE/FSAVE-Adressierungsarten ergänzt**: FRESTORE `(d16,PC)`
   (Boot-ROM macht `FRESTORE nullframe(PC)` als FPU-Reset), FRESTORE `(d16,An)` und
   FSAVE `(d16,An)` (OS-9-Kernel-FPU-Kontextwechsel). Vorher: `fatalerror` → Prozess-
   Abbruch. Gleiche NULL-Frame-Logik wie die vorhandenen Modi, kein Inc/Dec noetig.
2. **`m68kmmu.h` — Root-Deskriptor DT=1 (Direct Mapping)**: Der Root-Pointer selbst
   ist der Page-Deskriptor → flache Abbildung (OS-9 bootet mit so einer 1:1-Super-
   visor-Map, bevor ssm die Tabellen aufbaut). Vorher `fatalerror` trotz Kommentar
   "should cause direct mapping".
3. **`m68kmmu.h` — vierte Tabellenebene (TID)**: Der 68851/68030-Walk hat bis zu vier
   Ebenen (A/B/C/D, TC-Felder TIA-TID) — Musashi konnte nur drei, OS-9s ssm851 nutzt
   alle vier. Table-C-Eintraege mit DT=2/3 laufen jetzt in einen D-Level-Walk statt
   in einen (falsch beschrifteten) `fatalerror`.
4. **`m68kmmu.h` — Diagnostik**: Die verbliebenen `fatalerror`-Meldungen enthalten
   jetzt Adresse/TC/SRP/CRP/SR/PC bzw. Deskriptor-Inhalt (und die "Table B"-Meldung
   im C-Zweig druckte vorher die falsche Variable).
5. **`m68kconf.h` — `M68K_EMULATE_INT_ACK` ON**: Die 68681-DUART des CB030 liefert
   ihren Vektor (IVR, z.B. 0x50) im IACK-Zyklus — der OS-9-Treiber registriert
   seinen Handler auf genau diesem Vektor. Mit OFF (Default) waere alles autovektor-
   isiert und der DUART-Handler nie angesprungen. Die Vektor-Auswahl (DUART = IVR,
   Timer = Autovector) macht der Callback in `src/kernel/m68krt.c`.

Ergebnis: Das unveraenderte Microware-ROM-Image bootet OS-9/68K bis zur interaktiven
mshell (`$`-Prompt, `mdir` funktioniert) — s. ARBEITSPLAN.md 5.4.

## Q9-eigene Änderung am Vendor-Code (2026-07-10, Schritt 5.9)

6. **`m68kcpu.c`/`m68k.h` — `m68k_is_stopped(void)` neu**: schmaler Accessor, der
   zurückgibt, ob `CPU_STOPPED` (internes Flag, wird u.a. von der `STOP`-Instruktion
   gesetzt) ungleich 0 ist. Grund: OS-9 idlet im Login-Prompt per `STOP #$3000` —
   ohne diesen Accessor "verbrennt" `cb030run.c` die angeforderten Zyklen einer
   gestoppten CPU sofort wieder (100 % Host-CPU im Leerlauf), weil es von aussen
   keine Möglichkeit gab, den Stopp-Zustand abzufragen. Rein additiv (keine
   bestehende Funktion geändert), markiert mit `Q9/CB030`.
