# ARBEITSPLAN — Q9

Claudias Arbeitsplan: nächste Schritte, aktueller Status, geparkte Probleme.
Wird bei jeder Arbeitssession aktualisiert (feiner granular als context.txt).

**Status-Legende**:
💡 Vorschlag · 💤 Idle · 🟢 Ready · 🔄 in Arbeit · ✅ fertig · ⛔ blockiert/geparkt

| Status | Bedeutung |
|--------|-----------|
| 💡 Vorschlag | von Claudia geplant, **noch nicht besprochen** — wird nicht bearbeitet, bis Andreas entscheidet (→ Idle, Ready oder gestrichen) |
| 💤 Idle | gültiger Punkt, wird irgendwann bearbeitet — aber nicht jetzt |
| 🟢 Ready | freigegeben — darf vom eingetragenen Bearbeiter angegangen werden |
| 🔄 in Arbeit | wird gerade bearbeitet (immer nur EIN Schritt) |
| ✅ fertig | abgeschlossen, getestet, committet |
| ⛔ blockiert | geparkt, Problem unter "Geparkt" dokumentiert |

**Wer**: Claudia (Claude Code) · Codex (geplant) · Andreas · — (offen, wird bei Freigabe festgelegt)

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
5. **Nur 🟢 Ready wird bearbeitet** — 💡 Vorschläge und 💤 Idle bleiben liegen,
   bis Andreas sie freigibt.

---

## Nächste Arbeitsschritte

### Phase 0 — Fundament

| # | Schritt | Status | Wer | Notizen |
|---|---------|--------|-----|---------|
| 0.1 | Toolchain installieren: w64devkit 2.8.0 (gcc 16.1.0 + make) + emsdk latest | ✅ | Claudia | portabel, ohne Admin; Doku: docs/TOOLCHAIN.md |
| 0.2 | `src/hal/q9_hal.h` ausformulieren (Konsole, Block-Device, Timer, Target-Info) | ✅ | Claudia | yield gestrichen: Host treibt q9_kernel_step() |
| 0.3 | Kernel-Minimalgerüst: `src/kernel/kernel.c`, Banner, Echo-Loop über HAL | ✅ | Claudia | nicht-blockierendes Step-Design |
| 0.4 | HAL `native/`: PC-Build (conio, Disk-Image-Stub), Makefile-Target `native` | ✅ | Claudia | Windows-only (conio); POSIX-Variante später |
| 0.5 | HAL `wasm/`: Emscripten-Build, `web/index.html` mit xterm.js, Makefile-Target `wasm` | ✅ | Claudia | q9.wasm = 1,3 KB 😄 |
| 0.6 | Erster Test: `test/01_test_boot.py` (nativ) + Browser-Boot verifiziert | ✅ | Claudia | PASS; Bugfix: Konsole ist UTF-8-Bytestrom (C1-Falle) |
| 0.7 | Git-Repo initialisieren + erster Commit + GitHub | ✅ | Claudia | github.com/foellmy51/Q9 (privat) |

### Phase 1 — Kernel-Basis

| # | Schritt | Status | Wer | Notizen |
|---|---------|--------|-----|---------|
| 1.1 | Syscall-Design entschieden (E7): OS-9-Nummern + Registerkonventionen, ABI-Spez in docs/SYSCALLS.md | ✅ | Claudia | Nummern/Fehlercodes aus Andreas' MWOS-SDK (M:\MWOS) verifiziert |
| 1.2 | Dispatcher + erste Calls: I$Read/Write/ReadLn/WritLn, F$Exit/ID/Time; Kernel-REPL nutzt eigene Syscalls; Selbsttest + Test 02 | ✅ | Claudia | E$NotRdy statt Blockieren bis Phase 4 (dokumentiert) |
| 1.3 | Device-Modell + Konsolen-Treiber als internes Modul (löst fest verdrahtete Pfade 0/1/2 ab) | ✅ | Claudia | device.c/dev_term.c, Mode-Check E$BMode, Test 03; Doku: docs/DEVICES.md |
| 1.4 | I$Dup + I$Close als Syscalls (Pfadtabelle nach außen nutzbar machen, OS-9-Semantik: Dup liefert niedrigste freie Nummer) | ✅ | Claudia | q9_path_dup, 2 neue Selbsttest-Checks, SYSCALLS.md |
| 1.5 | F$PrsNam + F$CmpNam (Pfadnamen-Parsing nach OS-9-Regeln) | ✅ | Claudia | name.c/h; E$Diff-Nummer ($E2) vorläufig → MWOS-Abgleich heute Abend |
| 1.6 | I$Attach + I$Detach: Geräte per Name ("/term") an-/abmelden, nutzt F$PrsNam | ✅ | Claudia | q9_dev_attach/detach, E$MNF neu; Link-Count-Verwaltung |
| 1.7 | Zweites Gerät /nil (Null-Device) als Mini-Treiber | 🟢 | Claudia | beweist, dass das Device-Modell trägt (2 Treiber, 1 Schnittstelle); winzig — später ggf. Codex |
| 1.8 | I$GetStt/I$SetStt Grundgerüst: SS-Codes für /term (z.B. SS.Ready = Eingabe wartet?) | 🟢 | Claudia | sauberer Weg fürs Polling statt E$NotRdy-Trick; SS-Codes aus MWOS übernehmen (Abgleich M:\MWOS auf Laptop) |
| 1.9 | HAL-Erweiterung Echtzeit (q9_hal_time): native = localtime, wasm = Date.now → F$Time liefert echte Uhrzeit + F$STime | 🟢 | Claudia | räumt die dokumentierte F$Time-Abweichung ab |

### Phase 2 — Modulsystem (Grobskizze, Feinplanung nach Phase-1-Abschluss)

| # | Schritt | Status | Wer | Notizen |
|---|---------|--------|-----|---------|
| 2.1 | Modul-Header final spezifizieren (docs/MODULES.md), CRC32-Routine im Kernel | 💤 | — | Header-Entwurf steht in PROJECT.md; Feinplanung nach Phase-1-Abschluss |
| 2.2 | `tools/q9mod`: Compiler-Output → Q9-Modul (Header, CRC, Custom Section für WASM) | 💤 | — | portables C, läuft auf dem PC |
| 2.3 | Modul-Directory im Kernel + F$Link/F$UnLink; Module aus eingebautem ROM-Image | 💤 | — | |
| 2.4 | dev_term als echtes Typ-2-Modul (Treiber) aus dem ROM-Image laden | 💤 | — | Nagelprobe: internes Modul → echtes Modul |

---

## ⛔ Geparkt / mit Andreas zu besprechen

- **emsdk fehlt auf dem Desktop AF-PC**: wasm-Build/Browser-Test dort aktuell nicht
  möglich (nur nativ + Tests). Bei Bedarf installieren (~1 GB) oder wasm-Checks auf
  dem Laptop machen.

---

## Erledigt

- **2026-07-02 — Phase 0 komplett** ✅: Toolchain, HAL, Kernel-Gerüst, beide Targets bauen,
  nativer Selftest PASS, Browser-Boot mit Echo verifiziert. Q9 v0.01 alpha läuft.
- **2026-07-03 — Phase 1.1 + 1.2** ✅: OS-9-Syscall-ABI (E7) spezifiziert und implementiert,
  REPL über eigene Syscalls, Tests 01+02 PASS, Browser verifiziert.
  Fix: Timer-Drosselung in versteckten Tabs → Input-getriebenes Stepping + rAF.
- **2026-07-03 — Phase 1.3** ✅: Device-Modell (Geräte-/Pfadtabelle, IOMan-Vorbild) +
  /term-Treiber als internes Modul (SCF-artig), Mode-Check E$BMode, Tests 01–03 PASS.
  Nebenbei: w64devkit auf Desktop AF-PC installiert (docs/TOOLCHAIN.md).

---

**Letzte Aktualisierung**: 2026-07-03 — Andreas hat die Vorschläge freigegeben:
1.4–1.9 → 🟢 Ready (Claudia), Phase-2-Skizze → 💤 Idle. Nächster Schritt: 1.4.
