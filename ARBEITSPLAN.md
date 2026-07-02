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
| 0.1 | Toolchain prüfen/festlegen: Emscripten vorhanden? clang nativ vorhanden? Installation dokumentieren | ⬜ | Votum: Emscripten für WASM, clang/gcc für nativ |
| 0.2 | `src/hal/q9_hal.h` ausformulieren (Konsole, Block-Device, Timer, Yield) | ⬜ | Entwurf steht in PROJECT.md |
| 0.3 | Kernel-Minimalgerüst: `src/kernel/main.c` mit `q9_init()`, Banner, Echo-Loop über HAL | ⬜ | |
| 0.4 | HAL `native/`: PC-CLI-Build (stdin/stdout), Makefile-Target `native` | ⬜ | zuerst nativ — schnellster Debugzyklus |
| 0.5 | HAL `wasm/`: Emscripten-Build, `web/index.html` mit xterm.js, Makefile-Target `wasm` | ⬜ | |
| 0.6 | Erster Test: `test/01_test_boot` — beide Targets booten, Banner erscheint | ⬜ | |
| 0.7 | Git-Repo initialisieren + erster Commit (nach Freigabe durch Andreas) | ⬜ | |

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

*(noch nichts — Projekt frisch angelegt am 2026-07-02)*

---

**Letzte Aktualisierung**: 2026-07-02 — Plan initial erstellt, noch kein Schritt begonnen.
