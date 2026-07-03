# ARBEITSPLAN — Q9

Claudias Arbeitsplan: nächste Schritte, aktueller Status, geparkte Probleme.
Wird bei jeder Arbeitssession aktualisiert (feiner granular als context.txt).

**Status-Legende**: ⬜ offen · 🔄 in Arbeit · ✅ fertig · ⛔ blockiert/geparkt

## Arbeitsregeln

1. Immer nur EIN Schritt 🔄 gleichzeitig.
2. **Timebox bei Problemen**: Wenn ein Schritt richtig klemmt → nicht verbeißen,
   sondern ⛔ setzen, Problem unter "Geparkt" dokumentieren, mit Andreas
   besprechen und solange am nächsten unabhängigen Schritt weiterarbeiten.
3. Nach jedem abgeschlossenen Schritt: Status hier aktualisieren,
   am Session-Ende zusätzlich context.txt.
4. **Sofort sichern (Limit-Schutz)**: Nach jedem abgeschlossenen Schritt und
   nach jeder Änderung an ARBEITSPLAN.md/context.txt sofort `git commit` +
   `git push` — nicht bis zum Session-Ende warten. Session-Limits kommen
   ohne Vorwarnung; alles was nur lokal oder nur im Chat existiert, kann
   verloren gehen. WIP-Commits sind ausdrücklich erlaubt.

---

## Nächste Arbeitsschritte

### Phase 0 — Fundament

| # | Schritt | Status | Notizen |
|---|---------|--------|---------|
| 0.1 | Toolchain installieren: w64devkit 2.8.0 (gcc 16.1.0 + make) + emsdk latest | ✅ | portabel, ohne Admin; Doku: docs/TOOLCHAIN.md |
| 0.2 | `src/hal/q9_hal.h` ausformulieren (Konsole, Block-Device, Timer, Target-Info) | ✅ | yield gestrichen: Host treibt q9_kernel_step() |
| 0.3 | Kernel-Minimalgerüst: `src/kernel/kernel.c`, Banner, Echo-Loop über HAL | ✅ | nicht-blockierendes Step-Design |
| 0.4 | HAL `native/`: PC-Build (conio, Disk-Image-Stub), Makefile-Target `native` | ✅ | Windows-only (conio); POSIX-Variante später |
| 0.5 | HAL `wasm/`: Emscripten-Build, `web/index.html` mit xterm.js, Makefile-Target `wasm` | ✅ | q9.wasm = 1,3 KB 😄 |
| 0.6 | Erster Test: `test/01_test_boot.py` (nativ) + Browser-Boot verifiziert | ✅ | PASS; Bugfix: Konsole ist UTF-8-Bytestrom (C1-Falle) |
| 0.7 | Git-Repo initialisieren + erster Commit + GitHub | ✅ | github.com/foellmy51/Q9 (privat) |

### Phase 1 — Kernel-Basis

| # | Schritt | Status | Notizen |
|---|---------|--------|---------|
| 1.1 | Syscall-Design entschieden (E7): OS-9-Nummern + Registerkonventionen, ABI-Spez in docs/SYSCALLS.md | ✅ | Nummern/Fehlercodes aus Andreas' MWOS-SDK (M:\MWOS) verifiziert |
| 1.2 | Dispatcher + erste Calls: I$Read/Write/ReadLn/WritLn, F$Exit/ID/Time; Kernel-REPL nutzt eigene Syscalls; Selbsttest + Test 02 | ✅ | E$NotRdy statt Blockieren bis Phase 4 (dokumentiert) |
| 1.3 | Device-Modell + Konsolen-Treiber als internes Modul (löst fest verdrahtete Pfade 0/1/2 ab) | ✅ | device.c/dev_term.c, Mode-Check E$BMode, Test 03; Doku: docs/DEVICES.md |

---

## ⛔ Geparkt / mit Andreas zu besprechen

- **Phase-1-Plan unvollständig**: Am 2026-07-02 waren die Schritte bis ca. 1.6/1.7
  definiert, sind aber durch ein Session-Limit nicht mehr ins Repo gelangt.
  Weitere Projektunterlagen liegen auf Andreas' Laptop (zu Hause).
  → Schritte 1.4 ff. gemeinsam rekonstruieren bzw. Unterlagen nachreichen.

---

## Erledigt

- **2026-07-02 — Phase 0 komplett** ✅: Toolchain, HAL, Kernel-Gerüst, beide Targets bauen,
  nativer Selftest PASS, Browser-Boot mit Echo verifiziert. Q9 v0.01 alpha läuft.
- **2026-07-03 — Phase 1.1 + 1.2** ✅: OS-9-Syscall-ABI (E7) spezifiziert und implementiert,
  REPL über eigene Syscalls, Tests 01+02 PASS, Browser verifiziert.
  Fix: Timer-Drosselung in versteckten Tabs → Input-getriebenes Stepping + rAF.

---

**Letzte Aktualisierung**: 2026-07-03 — Phase 1.2 fertig. Nächster Schritt: 1.3 Device-Modell.
