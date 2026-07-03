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
| 1.10 | POSIX-HAL (`src/hal/posix/`, termios statt conio) + Makefile-Target, damit Q9 auf macOS/Linux baut | ✅ | Claudia | src/hal/posix/hal_posix.c (termios raw+nonblocking, clock_gettime, localtime); Makefile waehlt HAL per `$(OS)` (Windows_NT = conio, sonst POSIX), PYTHON-Erkennung (python3/python); Test 01 generalisiert ("native-" statt "native-win64"); `make test` PASS auf macOS (native-macos), warnungsfrei |

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
| 2.1 | Modul-Header + CRC32-Routine im Kernel | ✅ | Claudia | src/kernel/module.h/.c neu: q9_modhdr_t (28 Byte, #pragma pack, Offsets exakt wie PROJECT.md), q9_crc32 (bitweise CRC-32/ISO-HDLC, kein Table, kein malloc). Selbsttest: Referenzwert "123456789" -> $CBF43926. make test PASS, warnungsfrei |
| 2.3a | Suchfunktion: ROM-Image-Blob nach Sync-Bytes durchsuchen, nach Fund um ModuleSize zum nächsten Modul springen | ✅ | Claudia | `q9_mod_scan_first`/`q9_mod_scan_next` in module.h/.c; reine Sync-Suche, Größe/CRC-Plausibilisierung bewusst noch nicht hier (kommt in 2.3b) — nur Schutz vor Endlosschleife/Overflow bei ModuleSize. Selbsttest: zwei Module lückenlos im ROM-Image (Sprung + Ende erkannt), Negativtest ohne Sync. `make test` PASS, warnungsfrei |
| 2.3b | Validierung: Sync/Größe plausibilisieren, CRC32 nachrechnen | ✅ | Claudia | q9_mod_validate in module.h/.c: HeaderSize/ModuleSize/NameOffset-Strukturchecks vor der vollen CRC32 (billig vor teuer); Zwei-Stufen-Check (Header-Parity vor CRC) wie bei OS-9 bewusst NICHT übernommen — Q9-Module sind klein genug. Neue Fehlercodes E$BMHP($EC)/E$BMCRC($E8), MWOS-verifiziert (lokale Kopie unter /Volumes/SSD1TB/projects/MWOS gefunden). 4 Selbsttest-Checks |
| 2.3c | Bekanntmachen: Directory-Eintrag anlegen, Namenskollisions-/Revision-Regel (höhere Revision gewinnt, bei Gleichstand bleibt das etablierte Modul) | ✅ | Claudia | q9_mod_register/q9_mod_find in module.c: Directory als statisches Array (Q9_MOD_MAXDIR=8, kein malloc, wie devtab/pathtab). E$DirFul($CE) bei vollem Directory. 2 Selbsttest-Checks (Revision-Gewinn + Gleichstand) |
| 2.3d | F$Link + F$UnLink als Syscalls: Suche nach Name+Type+Language, Link-Count rauf/runter | ✅ | Claudia | q9_mod_link/q9_mod_unlink in module.c + Dispatcher-Cases F_LINK/F_UNLINK in syscall.c (a0=Name,d1.b=Type,d2.b=Lang -> a1=Header,a2=Einsprung,d0.b=Rev; a1=Header -> UnLink). Bewusst NICHT binärkompatibel zu OS-9s A1/A2-Belegung (Entscheidung E2). E$MNF bei unbekanntem Namen. docs/SYSCALLS.md + SYSCALL_ROADMAP.md aktualisiert. 3 Selbsttest-Checks |
| 2.2 | `tools/q9mod`: Compiler-Output → Q9-Modul (Header, CRC, Custom Section für WASM) | 💤 | — | sinnvoll, sobald echte (nicht handgebaute) Module gebraucht werden — nach 2.3 |
| 2.4 | dev_term als echtes Typ-2-Modul (Treiber) aus dem ROM-Image laden | 💤 | — | Nagelprobe: internes Modul → echtes Modul; nach 2.2 |

**Entschieden (2026-07-03 abends):**
- **Modul-Gruppen** (gemeinsames Unlink mehrerer Module): erstmal nicht — braucht Multi-Modul-Dateien, die es noch nicht gibt. → Ideenspeicher.
- **OS-9-Dreiklang File-Manager/Treiber/Descriptor** vs. kombiniertes `q9_dev_t`: bleibt kombiniert — lohnt sich erst bei mehreren Instanzen desselben Treibers mit unterschiedlicher Konfiguration. → Ideenspeicher, falls der Bedarf mal auftaucht.
- **Speicherverwaltung**: nicht Teil von Phase 2 (siehe oben, in-place Referenzierung reicht).
- **PC-seitiges Inspektions-Tool** (`ident`/`dump`-artig): nicht nötig, Selbsttest-Muster reicht zur Verifikation. → Ideenspeicher, als `--dump`-Modus in `q9mod` statt eigenes Tool, falls später gebraucht.

### Phase 3 — Filesystem (Vorschläge, 2026-07-03 spät mit Andreas vorbesprochen)

Grundentscheidungen aus der Besprechung: **O1 = FAT16** als erster FS-Typ
(Interop: dasselbe Image lässt sich am Mac/PC mounten und befüllen — `hdiutil
attach -imagekey diskimage-class=CRawDiskImage q9disk.img`, formatieren mit
`newfs_msdos -F 16`). Andreas' Bedenken gegen 8.3-Namen ist durch LFN-Lesen
ausgeräumt (macOS erzeugt beim Mount ohnehin LFN-Einträge); LFN-**Schreiben**
ist deutlich fummeliger (8.3-Alias-Generierung, Mehrfach-Einträge) → erstmal
nicht (Ideenspeicher). Reihenfolge lesend vor schreibend, weil `F$Load` (der
wichtigste Kunde) nur Lesen braucht. Image-Format zunächst **Superfloppy**
(Boot-Sektor bei LBA 0, keine Partitionstabelle); MBR-Partitionen später als
Schicht zwischen Block-Device und VFS (Ideenspeicher, spätestens Phase 7 mit
SD-Karte nötig). Design-Referenz für den File-Manager-Schnitt: Buch **„OS-9
Insights" (Peter Dibble)** — ältere Auflage enthält einen FAT16-File-Manager;
als Vorlage lesen, Code aber NICHT übernehmen (Copyright), eigene
C99-Implementierung. Test-Images erzeugen die test/-Skripte selbst per Python
(kein eigenes PC-Tool nötig).

| # | Schritt | Status | Wer | Notizen |
|---|---------|--------|-----|---------|
| 3.1 | Block-Device `/d0` als Q9-Gerät (nutzt q9_hal_blk_read/write), Roh-Blockzugriff über GetStt/SetStt-SS-Codes; Test-Image per Python in test/ | 🟢 | Claudia | kleinster Schritt, testbar ganz ohne FS; HAL-Seite existiert schon (nativ: q9disk.img) |
| 3.2 | VFS-Schicht: Pfad-Routing `/d0/pfad/datei` (F$PrsNam trennt Gerät/Rest), File-Manager als austauschbare Einheit hinter schmaler Schnittstelle, Datei-Kontext pro Pfad, globales Arbeitsverzeichnis für I$ChgDir (pro-Prozess erst Phase 4) | 🟢 | Claudia | Manager-Schnitt vorm Festlegen mit dem Dibble-Buch abgleichen; Schnittstelle so schneiden, dass später ein 68k-Manager-Adapter andocken kann (siehe Ideenspeicher) |
| 3.3 | FAT16 lesend: Boot-Sektor/Root-Dir/Cluster-Ketten, I$Open + I$Read + I$Seek, Verzeichnis lesen; 8.3 **und** LFN-Namen lesen | 🟢 | Claudia | nur Superfloppy; Test: am Mac befülltes Image, Dateien aus Q9 heraus lesen |
| 3.4 | FAT16 schreibend: I$Create, I$Delete, I$MakDir, FAT-Ketten allozieren/freigeben; neue Namen nur 8.3 (LFN-Schreiben → Ideenspeicher) | 🟢 | Claudia | nach 3.3; Gegentest: von Q9 geschriebene Datei am Mac mounten und lesen |
| 3.5 | F$Load komplettieren: Modul aus Datei laden (statt nur ROM-Image), validieren, registrieren | 🟢 | Claudia | Nagelprobe Phase 2 + 3 zusammen; braucht erste Speicherverwaltung (Modul-Puffer) — Umfang beim Design klären |
| 3.6 | wasm-HAL: Block-Backend via OPFS (FileSystemSyncAccessHandle im Worker) + Image-Upload/-Download im Frontend | 🟢 | Claudia | kann nach hinten rutschen, nativ reicht zum Entwickeln von 3.1–3.5 |

### Phase 4 — Prozesse (Vorschläge, 2026-07-03 spät mit Andreas besprochen)

Grundsatzentscheidung **E8** (PROJECT.md): **Step-Modell statt Stack-Umschaltung** —
WASM kennt keinen Stack-Wechsel, also verwaltet der Scheduler Prozess-**Zustände**
(Active/Waiting/Sleeping) und ruft pro `q9_kernel_step()`-Tick den nächsten aktiven
Prozess als Step-Funktion. Blockieren = Zustandswechsel + Weckgrund, nie ein
eingefrorener Stack. Ausführung ist pro Language-Byte austauschbar (intern/C =
Step-Funktion auf dem Kernel-Stack; 68k später = Musashi-Kontext; WASM = Instanz) —
die emulierten Prozesse werden also die einfachsten. F$Fork ist OS-9-semantisch
(neuer Prozess aus Modul, kein Unix-Fork) und baut direkt auf dem Modul-Directory
aus Phase 2 auf (Fork = F$Link + Descriptor + Active-Queue). Scheduler-Interna
(F$AllPD & Co.) bleiben 🚫 wie in docs/SYSCALL_ROADMAP.md aussortiert.

| # | Schritt | Status | Wer | Notizen |
|---|---------|--------|-----|---------|
| 4.1 | Prozess-Descriptor-Tabelle (statisch, wie devtab): PID, Parent, Modul, Zustand, Exit-Code, eigene Std-Pfade 0/1/2; Scheduler als Round-Robin über Active in q9_kernel_step() | 💡 | — | Kern von allem; REPL wird erster echter Prozess |
| 4.2 | F$Fork + F$Exit + F$Wait + F$Chain: Prozess aus Modul starten (via Modul-Directory), beenden, auf Kind warten (E$NoChld $E2), verketten | 💡 | — | F$Exit existiert als Stub aus 1.2 — bekommt jetzt echte Semantik |
| 4.3 | Echtes Blockieren: E$NotRdy-Provisorium (1.2) ersetzen — Waiting-Zustand + Weckgrund, /term weckt bei Eingabe (SS.Ready-Mechanik), F$Sleep (Ticks, 0 = yield) | 💡 | — | danach fühlt sich I$ReadLn blockierend an, ohne je einen Stack einzufrieren |
| 4.4 | F$SSpd (suspendieren) + F$SPrior (Prioritätsfeld setzen) | 💡 | — | Scheduler bleibt Round-Robin, Priorität erstmal nur Datenfeld — Aging lohnt erst bei echter Konkurrenz |
| 4.5 | Signale: F$Send, F$Icpt, F$RTE (Signal bricht Waiting/Sleeping ab, Intercept-Handler als Step-Aufruf) | 💡 | — | konzeptionell unabhängig vom Kern, bewusst eigener Schritt |

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
| LFN-Schreiben im FAT16-Manager (lange Namen beim Anlegen neuer Dateien) | Phase-3-Besprechung 2026-07-03 | 8.3-Alias-Generierung + Mehrfach-Directory-Einträge = viel Kleinkram; LFN-Lesen (3.3) deckt den Alltagsfall (am Mac befüllte Images) schon ab |
| MBR-Partitionstabelle: Partitionen als Sub-Block-Devices (z.B. `/d0.1`), MBR-Parser als dünne Schicht zwischen Block-Device und VFS | Phase-3-Besprechung 2026-07-03 | Superfloppy reicht für Phase 3; spätestens Phase 7 (SD-Karte am 68k-Board ist praktisch immer partitioniert) nötig — Einstiegspunkt im Code bleibt frei, gut testbar mit hdiutil/fdisk-Images |
| Original-OS-9-File-Manager (RBF, PCF) via Emulator laufen lassen, nativer Übergang erst am Treiber-Entry (HLE-Muster). **Update 2026-07-03 nachts (Andreas' Fund): NitrOS-9 ist GPL und quelloffen (github.com/nitros9project/nitros9)** — RBF dort in 6809-Assembler: (a) rechtlich saubere Modul-Quelle für die 6809-Runtime (Phase 8), GPL-Module dürfen anders als MWOS weitergegeben werden; (b) lesbare Referenz für eine eigene native C-Reimplementierung (nur Logik verstehen, nicht abschreiben — GPL färbt ab); (c) ToolShed-Tools des Projekts lesen/schreiben RBF-Images am PC → Test-Images ohne CoCo-Hardware | Phase-3-Besprechung 2026-07-03; PROJECT.md Phase 6/8 | Braucht bit-exakten Nachbau der OS-9-Kernelstrukturen (Path-Descriptor, Static Storage, System-State-Calls) — lohnt v.a. für RBF (echte OS-9-Disketten lesen!), für FAT ist nativ schneller. Frühestens Phase 6. ACHTUNG Lizenz: MWOS-Module dürfen nicht weitergegeben werden — nur für Andreas' privaten Gebrauch |

---

## ⛔ Geparkt / mit Andreas zu besprechen

- **emsdk fehlt auf dem Desktop AF-PC**: wasm-Build/Browser-Test dort aktuell nicht
  möglich (nur nativ + Tests). Bei Bedarf installieren (~1 GB) oder wasm-Checks auf
  dem Laptop machen.

---

## Erledigt

- **2026-07-03 — Phase 2.3b+c+d (Validieren, Bekanntmachen, Link/Unlink)** ✅: Boot-Pipeline
  aus docs/MODULES.md komplett (Suchen 2.3a -> Validieren -> Bekanntmachen -> Link/Unlink).
  `q9_mod_validate` prüft HeaderSize/ModuleSize/NameOffset (billig) vor der vollen CRC32 (teuer) —
  OS-9s zusätzliche Header-Parity-Vorabstufe bewusst nicht nachgebaut. `q9_mod_register`/
  `q9_mod_find` verwalten ein statisches Directory (Q9_MOD_MAXDIR=8, kein malloc) mit der
  OS-9-Revision-Tie-Break-Regel (höhere Revision gewinnt, bei Gleichstand bleibt das etablierte
  Modul). `q9_mod_link`/`q9_mod_unlink` + neue Dispatcher-Cases F$Link($00)/F$UnLink($02) in
  syscall.c (a0=Name, d1.b=Type, d2.b=Lang -> a1=Header, a2=Einsprung, d0.b=Revision) — Q9s
  eigene a1/a2-Belegung, bewusst nicht binärkompatibel zu OS-9 (Entscheidung E2). Neue,
  MWOS-verifizierte Fehlercodes E$BMHP($EC), E$BMCRC($E8), E$DirFul($CE), E$ModBsy($D1, aktuell
  unbenutzt) — Quelle: lokale MWOS-Kopie unter /Volumes/SSD1TB/projects/MWOS/SRC/DEFS/errno.h
  (kein Zugriff auf den Desktop-AF-PC-Pfad M:\MWOS nötig). docs/SYSCALLS.md (neuer F$Link/
  F$UnLink-Abschnitt) und docs/SYSCALL_ROADMAP.md aktualisiert. 9 neue Selbsttest-Checks
  (34 insgesamt), `make test` PASS, warnungsfrei. **Damit ist Phase 2.3 (Modul-Directory)
  komplett** — offen für Phase 2 sind nur noch 2.2 (`q9mod`-Tool) und 2.4 (dev_term als echtes
  Modul), beide 💤 bis freigegeben.
- **2026-07-03 — Phase 2.3a (Sync-Suche im ROM-Image)** ✅: `q9_mod_scan_first(rom, romlen)`
  durchsucht ein Blob byteweise nach den Sync-Bytes ($51 $39); `q9_mod_scan_next(rom, romlen, cur)`
  springt exakt um `modsize` weiter und prüft dort erneut den Sync (Module liegen im ROM-Image
  lückenlos hintereinander, OS-9-Vorbild). Beide Funktionen prüfen bewusst NUR den Sync — Größen-/
  CRC-Plausibilisierung ist 2.3b; einzige Ausnahme: ein Mindest-Check auf `modsize >= Q9_MOD_HDRSIZE`
  plus Overflow-Schutz, damit ein kaputter Header nicht zur Endlosschleife im Aufrufer führt.
  Selbsttest: zwei Module hintereinander im Blob (Sprung korrekt, Ende danach erkannt), Negativtest
  ohne Sync-Bytes. `make test` PASS, warnungsfrei. **Nächster Schritt: 2.3b** (Validierung: Größe
  plausibilisieren, CRC32 nachrechnen).
- **2026-07-03 — Phase 2.1 (Modul-Header + CRC32)** ✅: `src/kernel/module.h` neu (q9_modhdr_t,
  28 Byte via `#pragma pack(push,1)`, Offsets exakt wie PROJECT.md-Entwurf: Sync/HeaderSize/
  ModuleSize/NameOffset/Type/Language/Attribute/Revision/ExecOffset/DataSize/CRC32; Type-/Language-/
  Attribute-Konstanten aus PROJECT.md übernommen). `src/kernel/module.c` neu: `q9_crc32` — bitweise
  CRC-32/ISO-HDLC (Poly $EDB88320 reflektiert, Init/Final $FFFFFFFF, wie ZIP/Ethernet), bewusst ohne
  Tabelle (kein malloc im Kernel, Q9-Module klein genug). Selbsttest-Check gegen den bekannten
  Referenzwert für "123456789" ($CBF43926). Makefile KSRC/HDRS ergänzt. `make test` PASS,
  warnungsfrei. **Nächster Schritt: 2.3a** (Suchfunktion im ROM-Image-Blob).
- **2026-07-03 — Phase 1.10 (POSIX-HAL)** ✅: `src/hal/posix/hal_posix.c` neu (termios
  raw+nonblocking statt conio, clock_gettime statt GetTickCount64, sonst identisch zur
  Windows-HAL: Disk-Image, localtime). Makefile: `native`-Target waehlt HAL automatisch
  per `$(OS)` (Windows_NT → hal_native.c, sonst → hal_posix.c), PYTHON-Variable erkennt
  python3/python. Test 01 generalisiert (Target-Check "native-" statt "native-win64").
  `make test` PASS auf dem Mac Mini (native-macos), Build warnungsfrei. **Phase 1 damit
  auch auf macOS/Linux baubar — Voraussetzung fuer den autonomen Betrieb erfuellt.**
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

**Letzte Aktualisierung**: 2026-07-03 spät, Mac Mini — **Phase 3 freigegeben:
3.1–3.6 auf 🟢 Ready (Wer=Claudia) gestellt.** Davor: Phase-3-Vorbesprechung
mit Andreas**: Schritte 3.1–3.6 als 💡 eingetragen (O1 = FAT16 entschieden,
LFN lesen ja / schreiben Ideenspeicher, Superfloppy zuerst, Dibble-Buch als
Design-Referenz), drei neue Ideenspeicher-Einträge (LFN-Schreiben,
MBR-Partitionen, Original-RBF/PCF via Musashi). Davor: **Phase 2.3
(Modul-Directory) komplett abgeschlossen** — 2.3b (q9_mod_validate), 2.3c
(q9_mod_register/find, Directory), 2.3d (F$Link/F$UnLink), `make test` PASS,
warnungsfrei (34 Selbsttest-Checks). Kein Ready-Schritt offen für Claudia —
2.2/2.4 sind 💤, 3.1–3.6 sind 💡 bis Andreas freigibt.
