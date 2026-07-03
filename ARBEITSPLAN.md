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
| 1.7 | Zweites Gerät /nil (Null-Device) als Mini-Treiber | ✅ | Claudia | dev_nil.c; q9_path_open jetzt namensbasiert ("/nil" mit Slash ok) |
| 1.8 | I$GetStt/I$SetStt Grundgerüst: SS-Codes für /term (z.B. SS.Ready = Eingabe wartet?) | ✅ | Claudia | getstat/setstat-Ops im Treiber-IF; SS.Ready+SS.EOF; SS-Nummern beim MWOS-Abgleich prüfen |
| 1.9 | HAL-Erweiterung Echtzeit (q9_hal_time): native = localtime, wasm = Date.now → F$Time liefert echte Uhrzeit + F$STime | ✅ | Claudia | Kalenderlogik 2000–2136, Wochentag in d2; wasm-Teil ungetestet (emsdk fehlt hier) |
| 1.10 | POSIX-HAL (`src/hal/posix/`, termios statt conio) + Makefile-Target, damit Q9 auf macOS/Linux baut | 💡 | — | Voraussetzung für 24/7-Betrieb auf dem Mac Mini (docs/AUTONOMIE.md) und spätere Cloud-Läufe; am besten direkt auf dem Mac umsetzen + testen |

### Phase 2 — Modulsystem

Feinplanung 2026-07-03 abends mit Andreas besprochen. Reihenfolge folgt dem
OS-9-Boot-Ablauf aus docs/MODULES.md: **Suchen → Validieren → Bekanntmachen →
Link/Unlink.** Kein Dateisystem nötig — Module werden aus einem einkompilierten
„ROM-Image"-Blob **in-place referenziert** (wie OS-9s ROM-Module: PIC-Code
läuft direkt aus dem ROM, keine Kopie nötig) — deshalb auch **keine
Speicherverwaltung** für Phase 2 nötig, die kommt erst mit Phase 3 (Laden von
echter Datei) bzw. Phase 4 (Prozess-Stacks/Heaps).

| # | Schritt | Status | Wer | Notizen |
|---|---------|--------|-----|---------|
| 2.1 | Modul-Header + CRC32-Routine im Kernel | 🟢 | Claudia | Entwurf aus PROJECT.md/docs/MODULES.md wird beim Implementieren von 2.3a live finalisiert, keine separate Vorab-Spezifikation |
| 2.3a | Suchfunktion: ROM-Image-Blob nach Sync-Bytes durchsuchen, nach Fund um ModuleSize zum nächsten Modul springen | 🟢 | Claudia | Testmodule: zur Laufzeit im Selbsttest gebaut (eigene CRC32-Routine berechnet die CRC selbst — testet beides zugleich), plus kleines statisches Test-ROM-Image (gültiges Modul / kaputte CRC / kein Sync als Negativtests) |
| 2.3b | Validierung: Sync/Größe plausibilisieren, CRC32 nachrechnen | 🟢 | Claudia | Zwei-Stufen-Check (Header-Parity vor CRC) wie bei OS-9 bewusst NICHT übernommen — Q9-Module sind klein genug |
| 2.3c | Bekanntmachen: Directory-Eintrag anlegen, Namenskollisions-/Revision-Regel (höhere Revision gewinnt, bei Gleichstand bleibt das etablierte Modul) | 🟢 | Claudia | Directory als statisches Array (kein malloc im Kernel, wie devtab/pathtab) |
| 2.3d | F$Link + F$UnLink als Syscalls: Suche nach Name+Type+Language, Link-Count rauf/runter | 🟢 | Claudia | |
| 2.2 | `tools/q9mod`: Compiler-Output → Q9-Modul (Header, CRC, Custom Section für WASM) | 💤 | — | sinnvoll, sobald echte (nicht handgebaute) Module gebraucht werden — nach 2.3 |
| 2.4 | dev_term als echtes Typ-2-Modul (Treiber) aus dem ROM-Image laden | 💤 | — | Nagelprobe: internes Modul → echtes Modul; nach 2.2 |

**Entschieden (2026-07-03 abends):**
- **Modul-Gruppen** (gemeinsames Unlink mehrerer Module): erstmal nicht — braucht Multi-Modul-Dateien, die es noch nicht gibt. → Ideenspeicher.
- **OS-9-Dreiklang File-Manager/Treiber/Descriptor** vs. kombiniertes `q9_dev_t`: bleibt kombiniert — lohnt sich erst bei mehreren Instanzen desselben Treibers mit unterschiedlicher Konfiguration. → Ideenspeicher, falls der Bedarf mal auftaucht.
- **Speicherverwaltung**: nicht Teil von Phase 2 (siehe oben, in-place Referenzierung reicht).
- **PC-seitiges Inspektions-Tool** (`ident`/`dump`-artig): nicht nötig, Selbsttest-Muster reicht zur Verifikation. → Ideenspeicher, als `--dump`-Modus in `q9mod` statt eigenes Tool, falls später gebraucht.

---

## 💭 Ideenspeicher (noch nicht eingeplant)

Ideen, die während der Arbeit auftauchen, aber (noch) kein Teil der Planung
sind — keine 💡-Vorschläge zur Freigabe, sondern eine reine Merkliste ohne
Zeitdruck. Kein Status, keine Phase, kein „Wer" — schaffen es vielleicht
irgendwann als 💡-Vorschlag in eine echte Phase, müssen aber nicht. Unterschied
zu „⛔ Geparkt": dort stehen akute Klärungsbedarfe für laufende Arbeit, hier
Zukunftsideen ohne Handlungsdruck.

| Idee | Kontext | Warum (noch) nicht eingeplant |
|------|---------|-------------------------------|
| Kernel-Tabellen als Datenmodule verpacken (analog OS-9s `F$DatMod`) | docs/MODULES.md, Abschnitt 5 | Setzt das Modulsystem (Phase 2) voraus; passt nur zu read-mostly Daten, nicht zu häufig mutierenden Tabellen (Pfad/Geräte); keine MMU-Durchsetzung vorhanden (weder WASM noch aktuelles 68k-Ziel) |
| Modul-Gruppen (gemeinsames Unlink mehrerer zusammen geladener Module) | docs/MODULES.md, Abschnitt 4 | Braucht Multi-Modul-Dateien, die es noch nicht gibt (Phase-2-Besprechung 2026-07-03) |
| OS-9-Dreiklang File-Manager/Treiber/Descriptor statt kombiniertem `q9_dev_t` | docs/MODULES.md, Abschnitt 3 | Lohnt sich erst bei mehreren Instanzen desselben Treibers mit unterschiedlicher Konfiguration (Phase-2-Besprechung 2026-07-03) |
| `--dump`-Modus in `q9mod` (Modul-Header lesbar anzeigen, "ident"-artig) | Phase-2-Besprechung 2026-07-03 | Selbsttest-Muster reicht zur Verifikation während der Entwicklung; kein PC-Tool nötig, bis mal ein echtes produziertes Modul von Hand inspiziert werden muss |

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
- **2026-07-03 — Phase 1.4–1.9 komplett** ✅: I$Dup/I$Close, F$PrsNam/F$CmpNam (name.c),
  I$Attach/I$Detach (namensbasiert, E$MNF), /nil als zweiter Treiber, I$GetStt/I$SetStt
  (SS.Ready/SS.EOF), echte Uhrzeit (q9_hal_time, F$Time/F$STime, Kalender 2000–2136).
  22 Selbsttest-Checks, Tests 01–03 PASS. **Phase 1 damit abgeschlossen** (Kernel v1.80).
  Offen für MWOS-Abgleich: E$Diff-Nummer ($E2), SS-Nummern, F$Time-Packung.
- **2026-07-03 — Syscall-Roadmap + Bugfix** ✅: docs/SYSCALL_ROADMAP.md — alle 97
  OS-9-Syscalls (MWOS Professional V3.0) mit Status/Phase erfasst. Dabei gefunden
  und sofort behoben: E$Diff war $E2 (hätte mit E$NoChld/F$Wait in Phase 4
  kollidiert), korrekt lt. MWOS ist E$Differ = $A5. Kernel v1.90, Tests PASS.
- **2026-07-03 — Rest-MWOS-Abgleich abgeschlossen** ✅: SS-Codes (SS.Opt/Ready/
  Size/EOF) waren schon korrekt (`sg_codes.h` verifiziert). F$Time/F$STime
  hatten d0/d1 vertauscht (echtes OS-9: d0=Zeit, d1=Datum — bestätigt im
  OS-9 for 68K Technical Reference Manual) — behoben, Selbsttest angepasst.
  Kernel v2.00, Tests PASS. **Damit ist der komplette Phase-1-MWOS-Abgleich
  durch, keine offenen Punkte mehr aus Phase 1.**

---

**Letzte Aktualisierung**: 2026-07-03 abends — **Phase-2-Besprechung
abgeschlossen.** Reihenfolge steht (Suchen→Validieren→Bekanntmachen→
Link/Unlink, kein Dateisystem/keine Speicherverwaltung nötig), 2.1+2.3a–d auf
🟢 Ready gesetzt, drei Design-Fragen entschieden (Modul-Gruppen nein, Dreiklang
File-Manager/Treiber/Descriptor nein, PC-Ident-Tool nein — alle drei in den
Ideenspeicher). Nächster Schritt: 2.1/2.3a implementieren.
