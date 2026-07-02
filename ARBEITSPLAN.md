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

### Phase 1 — Kernel-Basis (Vorschau, wird nach Phase 0 detailliert)

| # | Schritt | Status | Notizen |
|---|---------|--------|---------|
| 1.1 | Syscall-Mechanismus + Nummernraum-Entscheidung (O3) | ⬜ | vorher mit Andreas besprechen |
| 1.2 | Device-Modell + Konsolen-Treiber als erstes internes Modul | ⬜ | |

---

## ⛔ Geparkt / mit Andreas zu besprechen

*(aktuell nichts)*

---

## Erledigt

- **2026-07-02 — Phase 0 komplett** ✅: Toolchain, HAL, Kernel-Gerüst, beide Targets bauen,
  nativer Selftest PASS, Browser-Boot mit Echo verifiziert. Q9 v0.01 alpha läuft.

---

**Letzte Aktualisierung**: 2026-07-02 — Phase 0 abgeschlossen. Nächster Schritt: 1.1
(Syscall-Nummernraum O3 vor Implementierung mit Andreas besprechen!).
