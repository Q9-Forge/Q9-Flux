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
| 3.1 | Block-Device `/d0` als Q9-Gerät (nutzt q9_hal_blk_read/write), Roh-Blockzugriff über GetStt/SetStt-SS-Codes; Test-Image per Python in test/ | ✅ | Claudia | dev_d0.c neu: SS.BlkRd($14)/SS.BlkWr($15) aus MWOS sg_codes.h übernommen (RBF-Vorbild), reine Ops read/write/readln/writln bewusst E$UnkSvc (kein Byte-Strom ohne VFS, kommt in 3.2). q9disk.img bleibt HAL-seitig (lazy erzeugt), jetzt .gitignore't. Test 04 (04_test_blkdev.py) + 2 neue Selbsttest-Checks (Roundtrip LBA 1, E$Param/E$UnkSvc). docs/DEVICES.md + SYSCALLS.md aktualisiert. `make test` PASS, warnungsfrei |
| 3.2 | VFS-Schicht: Pfad-Routing `/d0/pfad/datei` (F$PrsNam trennt Gerät/Rest), File-Manager als austauschbare Einheit hinter schmaler Schnittstelle, Datei-Kontext pro Pfad, globales Arbeitsverzeichnis für I$ChgDir (pro-Prozess erst Phase 4) | ✅ | Claudia | `src/kernel/vfs.c/.h` neu: `q9_fm_t` (open/create/makdir/remove, restpath-String statt OS-9-Pfaddeskriptor-Internas — schmal genug für einen künftigen 68k-Manager-Adapter). `q9_dev_t.fm` (device.h, NULL = kein Dateisystem) + `q9_path_t.fmctx[16]` (Datei-Kontext pro Pfad, kein malloc). `q9_vfs_open` löst relative Pfade gegen ein globales `cwd` auf, trennt Gerät/Rest per F$PrsNam, routet an `dev->fm->open` oder (kein fm) ans alte `q9_path_open`-Verhalten (Rest muss leer sein, sonst `E$PNNF` neu — MWOS-verifiziert). `I$Open`/`I$ChgDir` im Dispatcher (syscall.c) verdrahtet; `I$Create`/`I$MakDir`/`I$Delete` bewusst nur Gerüst (`E$UnkSvc`, echte Semantik erst 3.4). Kein FAT16 hier (kommt separat in 3.3/3.4). 5 neue Selbsttest-Checks (Test-File-Manager im Selbsttest beweist Routing) + neues `test/05_test_vfs.py`. docs/SYSCALLS.md + SYSCALL_ROADMAP.md + DEVICES.md aktualisiert. `make test` PASS, warnungsfrei. wasm ungetestet (emsdk fehlt weiterhin lokal). |
| 3.3 | FAT16 lesend: Boot-Sektor/Root-Dir/Cluster-Ketten, I$Open + I$Read + I$Seek, Verzeichnis lesen; 8.3 **und** LFN-Namen lesen | ✅ | Claudia | `src/kernel/fat16.c/.h` neu — Boot-Sektor (BPB) plausibilisieren, Root-Dir + Unterverzeichnisse durchsuchen (8.3 UND LFN, Namensteile rückwärts zusammengesetzt), FAT16-Cluster-Ketten folgen ($FFF8-$FFFF = Ende), Datei-Kontext `fat16_ctx_t` (16 Byte, passt exakt in `Q9_FMCTX_SIZE`: start_cluster/cur_cluster/pos/size). **Design-Entscheidung**: `q9_fm_t` (vfs.h) um `read`/`seek`-Funktionszeiger erweitert (zusätzlich zu open/create/makdir/remove aus 3.2) — I$Read/I$Seek brauchen den Datei-Kontext im Pfad, das kann nur der File-Manager interpretieren, nicht der Block-Treiber. Dispatcher (syscall.c): I$Read routet auf `fm->read()`, wenn ein File-Manager mit read-Op hinter dem Pfad steht (sonst wie bisher an den Treiber); I$Seek ist komplett neu und liefert ohne passenden File-Manager `E$UnkSvc`. Verdrahtung: `q9_dev_init()` (device.c) versucht `q9_fat16_mount()` direkt nach der `/d0`-Registrierung und setzt `q9_dev_set_fm(d0, &q9_fat16_fm)` NUR bei erkanntem FAT16-Superfloppy (Boot-Sektor-Plausibilisierung: BytesPerSector/SectorsPerCluster/FATSize16/RootEntryCount/Boot-Signatur $55AA) — sonst bleibt `/d0` wie bisher ohne Dateisystem. **Nebenbei gefundener und behobener Bug**: der ältere 3.1-Selbsttest (SS.BlkWr/SS.BlkRd-Roundtrip auf LBA 1) überschrieb dauerhaft Testdaten auf LBA 1 — bei einem echten FAT16-Image liegt dort typischerweise die erste FAT-Kopie, der Roundtrip hätte ein gemountetes Dateisystem im Selbsttest zerstört. Jetzt sichert/stellt der Test LBA 1 wieder her. Ebenso mussten die 3.2-VFS-Selbsttests (Test-File-Manager) den *vorherigen* File-Manager von `/d0` merken und zurücksetzen statt hart auf NULL zu setzen (sonst hätte der VFS-Test den produktiven FAT16-Manager aus dem Selbsttest herausgerissen). 5 neue Selbsttest-Checks (nur aktiv, wenn `/d0` beim Boot als FAT16 erkannt wurde — sonst kein FEHLER, sondern stiller Skip) + neues `test/06_test_fat16.py` (baut ein FAT16-Superfloppy-Image komplett per Python-Stdlib von Hand: Boot-Sektor/BPB, 2 FAT-Kopien, Root-Directory mit einer 8.3-Datei, einer LFN-Datei mit langem Namen und einem Unterverzeichnis samt verschachtelter Datei; kein externes Tool wie hdiutil/newfs_msdos nötig). docs/SYSCALLS.md (I$Read-Routing, neuer I$Seek-Abschnitt, Funktionsnummern-Tabelle), docs/SYSCALL_ROADMAP.md, docs/DEVICES.md (q9_fm_t-Erweiterung, fmctx-FAT16-Layout-Tabelle, neuer FAT16-File-Manager-Abschnitt) aktualisiert. `make test` PASS, warnungsfrei (jetzt 6 Testskripte). wasm ungetestet (emsdk fehlt weiterhin lokal, siehe Geparkt). **Nächster Schritt: 3.4** (FAT16 schreibend) — diese Session macht damit NICHT weiter. |
| 3.4 | FAT16 schreibend: I$Create, I$Delete, I$MakDir, FAT-Ketten allozieren/freigeben; neue Namen nur 8.3 (LFN-Schreiben → Ideenspeicher) | ✅ | Claudia | fat16.c/.h: I$Create/I$MakDir/I$Delete/I$Write echt implementiert, FAT-Ketten allozieren/freigeben (beide FAT-Kopien synchron), nur 8.3-Namen (E$BPNam bei LFN-Bedarf/Duplikat). Details siehe „Erledigt" unten |
| 3.5 | F$Load komplettieren: Modul aus Datei laden (statt nur ROM-Image), validieren, registrieren | ✅ | Claudia | q9_mod_load (module.c) — statischer Load-Puffer-Pool (4x4096 Byte, kein malloc), Directory-Eintraege merken Puffer-Herkunft, automatische Freigabe bei Link-Count 0. Details siehe „Erledigt" unten |
| 3.6 | wasm-HAL: Block-Backend via OPFS (FileSystemSyncAccessHandle im Worker) + Image-Upload/-Download im Frontend | ✅ | Claudia | Kernel läuft jetzt im Worker (Pflicht für Sync-Access-Handle); Details siehe „Erledigt". wasm ungetestet (emsdk fehlt lokal) |
| 3.7 | FAT16-Directory-Einträge bekommen echtes Datum/Uhrzeit (q9_hal_time) statt Nullfeldern bei I$Create/I$MakDir | ✅ | Claudia | `fat_pack_datetime()` (fat16.c) — Details siehe „Erledigt" unten |

### Phase 4 — Prozesse (freigegeben 2026-07-04, Konzept 2026-07-03 spät mit Andreas besprochen)

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
| 4.1 | Prozess-Descriptor-Tabelle (statisch, wie devtab): PID, Parent, Modul, Zustand, Exit-Code, eigene Std-Pfade 0/1/2; Scheduler als Round-Robin über Active in q9_kernel_step() | ✅ | Claudia | `src/kernel/proc.c/.h` neu — Details siehe „Erledigt" unten |
| 4.2 | F$Fork + F$Exit + F$Wait + F$Chain: Prozess aus Modul starten (via Modul-Directory), beenden, auf Kind warten (E$NoChld $E2), verketten | ✅ | Claudia | Entscheidung E9 (Q9_MOD_NATIVE) — Details siehe „Erledigt" unten |
| 4.3 | Echtes Blockieren: E$NotRdy-Provisorium (1.2) ersetzen — Waiting-Zustand + Weckgrund, /term weckt bei Eingabe (SS.Ready-Mechanik), F$Sleep (Ticks, 0 = yield) | ✅ | Claudia | Details siehe „Erledigt" unten |
| 4.4 | F$SSpd (suspendieren) + F$SPrior (Prioritätsfeld setzen) | ✅ | Claudia | Details siehe „Erledigt" unten |
| 4.5 | Signale: F$Send, F$Icpt, F$RTE (Signal bricht Waiting/Sleeping ab, Intercept-Handler als Step-Aufruf) | ✅ | Claudia | Details siehe „Erledigt" unten — damit ist Phase 4 vollständig |

**Anschluss 4.6–4.8 (freigegeben 2026-07-04 abends, Andreas' Einwand):**
Andreas' berechtigter Punkt: `F$Fork` "führt" bislang keinen echten geladenen
Code aus — Entscheidung **E9** trägt nur einen rohen C-Funktionszeiger direkt im
`Q9_MOD_NATIVE`-Modul ein (gültig nur im selben Host-Prozess, kein portables
Moduldateiformat). Bevor Phase 5 (Shell) sinnvoll ist, braucht es eine echte
Ausführungs-Engine — sonst wäre die Shell nur derselbe Etikettenschwindel wie
der jetzige Kernel-REPL. Anders als beim 68k-Ziel (klassisches PIC-Problem,
erst Phase 6/7) ist das für **WASM** (unser Primärformat) kein
Relozierungsproblem — WASM-Code referenziert nie absolute Host-Adressen.
Es fehlen stattdessen: eine Instanziierungs-Engine, eine Syscall-Rückverbindung
und die Übersetzung von Zeigern über die Modulgrenze (siehe PROJECT.md O5/O6).

| # | Schritt | Status | Wer | Notizen |
|---|---------|--------|-----|---------|
| 4.6 | Eingebettete WASM-Runtime für den nativen Build (löst O5): eine Bibliothek (Kandidat wasm3 — klein, embeddable, keine harte malloc-Pflicht) einbinden, die ein beliebiges `.wasm`-Modul zur Laufzeit instanziieren und eine exportierte Funktion aufrufen kann. Noch OHNE Syscall-Bridge — reiner Grundbaustein | ✅ | Claudia | wasm3 (MIT, Commit `d77cd814`) vendored unter `third_party/wasm3/` (nur Kern-Interpreter, kein WASI/libc); `src/kernel/wasmrt.c/.h` als schmaler Wrapper (init/load/call_i32/free). Details siehe „Erledigt" unten |
| 4.7 | `Q9_MOD_WASM`-Ausführung: F$Fork/F$Chain erkennen das Language-Byte `Q9_MOD_WASM`, instanziieren das Modul über die Runtime aus 4.6 (bzw. `WebAssembly.instantiate` im Browser) mit einem Import/Host-Funktion als Syscall-Bridge. Erstmal NUR Syscalls ohne Zeiger-Parameter (F$Time, F$ID, F$Exit) | 🟢 | Claudia | Beweist den kompletten Weg Laden→Instanziieren→Laufen→Syscall→Zurück end-to-end, bevor Zeiger dazukommen |
| 4.8 | Speicher-/Pointer-Marshaling über die Modulgrenze: a0–a7-Register als Offsets in die modul-eigene lineare Speicherinstanz interpretieren (statt rohe Host-Zeiger), mit Bounds-Check. Damit werden auch I$Read/I$Write/I$Open (Puffer-/Pfadnamen-Zeiger) für WASM-Module nutzbar | 🟢 | Claudia | `Q9_MOD_NATIVE` (E9-Stopgap) bleibt als Sonderfall bestehen (z.B. kernelinterne Prozesse), wird für "normale" Programme aber überflüssig |

---

### Phase U — Userland-Werkzeuge (Codex-Baustelle, separater Nebenschauplatz)

**Nicht Teil des normalen Phasenablaufs** — läuft parallel zu Phase 3/4, auf
einem eigenen Branch `codex-userland` (eigenes Arbeitsverzeichnis
`../Q9-codex-userland`, Basis war Stand nach 4.1), bearbeitet von **Codex**
(OpenAI, `codex exec`, Modell GPT-5.5) im Auftrag von Claudia/Andreas.
**Isolationsprinzip**: Arbeitsbereich strikt auf `userland/` begrenzt, kein
`git commit`/`push` durch Codex selbst — Claudia baut nach jeder Runde
unabhängig nach und verifiziert (PASS + warnungsfrei + Diff nur in
`userland/`), bevor gepusht wird. Kein Einfluss auf `main`/die
Autonomie-Routine. Ziel: eine kleine C-Bibliothek (`libq9`), die die bereits
implementierten Syscalls kapselt, plus darauf aufbauende Datei-Werkzeuge —
als Quellcode-Vorarbeit, NICHT als ladbare Q9-Module (das ist die offene
Architekturfrage **O6** in PROJECT.md: WASM-Import-ABI vs. 68k-PIC-Code,
zu klären ab Phase 5/6).

| # | Schritt | Status | Wer | Notizen |
|---|---------|--------|-----|---------|
| U.1 | `libq9`: Wrapper um die fertigen Syscalls + `q9cat`/`q9copy` + nativer Testharness | ✅ | Codex | Testharness bootet den echten Kernel nativ (POSIX-HAL) gegen ein selbst gebautes FAT16-Image, statt zu mocken |
| U.2 | `q9dir`: Verzeichnis auflisten | ✅ | Codex | Liest rohe 32-Byte-FAT16-Directory-Einträge über `I$Read` auf einen Verzeichnispfad (Kernel-Unterstützung existierte schon seit 3.3); Parsing in `userland/` selbst, da die interne `fat16_dirent_t` privat ist |
| U.3 | `q9mkdir` + `q9rm` | ✅ | Codex | Rundet das ursprünglich gewünschte "dir/list/copy"-Set ab; kein rekursives Löschen |
| U.4 | `q9touch` + `q9stat` + Testumgebung konsolidiert | ✅ | Codex | Sieben Tools insgesamt (cat/copy/dir/mkdir/rm/touch/stat); README vollständig; Test-Redundanzen aufgeräumt |
| U.5 | Funktions-Kommentare nachtragen (Andreas' Review: Code gut, aber unkommentiert) | ✅ | Codex | 88 Funktionen in 16 Dateien im Q9-Standardstil (Function/Desc/Call-Header) kommentiert; reine Ergänzung, +520/-0 Zeilen, kein Verhalten geändert (per Diff verifiziert) |
| U.6 (Idee) | Lange Dateinamen (LFN) auch in `q9dir`/`q9stat` anzeigen | 💭 | — | LFN-Parse-Logik steckt privat in `fat16.c` — Duplizieren oder Kernel müsste sie exportieren |
| U.7 (Idee) | Tools als echte ladbare Q9-Module, sobald Phase 5/6 (Modul-Ausführung) und O6 (Verpackungsformat) stehen | 💭 | — | heute nur Design-Vorgriff |

**Stand 2026-07-04**: U.1–U.5 fertig, jede Runde von Claudia unabhängig
nachgebaut (`userland/build.sh`, PASS, `-Wall -Wextra` warnungsfrei), Branch
`codex-userland` gepusht (Commits `bf5d494`, `a94ca93`). Nebenbefund bei der
Kommentar-Durchsicht: auch der Hauptkernel wurde stichprobenartig geprüft
(fast lückenlos kommentiert, eine echte Lücke `is_leap()` in `syscall.c`
gefunden und behoben, Commit `1caf2b3`). **Merge nach `main` steht noch aus**
— wartet auf Andreas' eigene Durchsicht, dann gemeinsame Entscheidung über
Merge-Weg und Anbindung an O6.

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
| **OS9exec** (Lukas Zeller/Beat Forster, GPL, sourceforge.net/projects/os9exec) als Referenz fuer Phase 6: 68k-Emulation + OS-9-Kernel-Nachbau in C auf Syscall-Ebene (TRAP->Syscall-Bridge, genau Q9s Phase-6-Architektur); Semantik-Nachschlagewerk fuer F$/I$-Randfaelle; evtl. auch RBF-Strukturcode (pruefen, ob Image-Mount unterstuetzt wird oder nur Host-FS-Mapping) | Andreas' Hinweis 2026-07-03 nachts; PROJECT.md Phase 6 | Erst relevant, wenn die Musashi-Runtime ansteht; GPL: Logik verstehen, nicht abschreiben |

---

## ⛔ Geparkt / mit Andreas zu besprechen

- **emsdk fehlt auf dem Desktop AF-PC** (noch unverändert): wasm-Build/Browser-Test dort
  weiterhin nicht möglich. Auf dem Mac Mini seit 2026-07-04 behoben (siehe unten) — betrifft
  jetzt nur noch den Desktop-Rechner, nicht mehr den Autonomie-Betrieb.

---

## Erledigt

- **2026-07-04 — Phase 4.6 (Grundbaustein wasm3-Runtime, Entscheidung E10)** ✅: erster Schritt
  des Anschlusses 4.6–4.8, löst Architekturfrage O5. **Vendoring**: `third_party/wasm3/` — nur
  der Kern-Interpreter von wasm3 (Commit `d77cd814`, MIT-Lizenz), unverändert übernommen; WASI-/
  libc-/Tracing-Dateien bewusst NICHT vendored (Q9 braucht keine WASI-Umgebung, die Syscall-Bridge
  in 4.7 verdrahtet eigene Q9-Importe direkt über `m3_LinkRawFunction`). LICENSE + README.md (mit
  Commit-Referenz, Scope, Build-Hinweis) liegen bei. **Wrapper**: `src/kernel/wasmrt.c/.h` neu —
  kapselt wasm3 als schmale, Q9-eigene API (`q9_wasmrt_init/load/call_i32/free`), alle wasm3-Typen
  bleiben als `void*` in `q9_wasmrt_t` verborgen, kein Aufrufer muss `wasm3.h` einbinden.
  **Design-Entscheidung**: wasm3 nutzt intern `malloc`/`free` (Environment/Runtime/Modul-Allokation)
  — bewusste, eng begrenzte Ausnahme von Q9s "kein malloc im Kernel"-Regel, NUR für den vendorten
  Fremdcode selbst (dokumentiert in `third_party/wasm3/README.md` + PROJECT.md Entscheidung E10);
  `wasmrt.c` und der restliche Kernel bleiben frei von eigenem `malloc`.
  **Makefile**: `WASM3_SRC`/`WASM3_OBJS` nur im `native`-Ziel (eigene, laxere `WASM3_CFLAGS` ohne
  `-Wall -Wextra` — 58 Warnungen im unveränderten Original, die wir nicht pflegen), `WASMRT_SRC`
  mit vollen `CFLAGS` (bleibt warnungsfrei); `-DQ9_HAVE_WASM3` aktiviert den Selbsttest-Zweig in
  `kernel.c`. `make wasm` bindet weder wasm3 noch wasmrt.c ein — im Browser laeuft Q9 selbst
  schon als WASM, `WebAssembly.instantiate` uebernimmt dort die Rolle (O6).
  **Selbsttest** (kernel.c, nur `#ifdef Q9_HAVE_WASM3`): handgebautes `.wasm`-Modul (Byte-Array,
  Aequivalent zu `(func $add (param i32 i32) (result i32) local.get 0 local.get 1 i32.add)`,
  exportiert als "add") wird geladen und mit (2,3) aufgerufen, Ergebnis 5 geprueft — beweist den
  kompletten Weg Laden→Instanziieren→Aufrufen→Ergebnis nativ, noch ohne Syscall-Bridge (kommt mit
  4.7). PROJECT.md (Entscheidung E10, O5 auf "entschieden" gesetzt), docs/HANDBUCH.md (Abschnitt
  1 Lizenz, 3 Quellcode-Layout inkl. `third_party/`, neuer Abschnitt 5.8, 6 Phase-4-Status, 7
  Referenzquellen-Tabelle) aktualisiert. `make clean && make native && make test` PASS,
  warnungsfrei; zusaetzlich mit AddressSanitizer/UBSan gegenverifiziert (sauber, inkl. wasm3
  selbst). `make wasm` (emsdk auf diesem Mac Mini) baut weiterhin warnungsfrei, unveraendert.
  **Nächster Ready-Schritt: 4.7** (`Q9_MOD_WASM`-Ausführung: F$Fork/F$Chain erkennen das
  Language-Byte, instanziieren ueber die 4.6-Runtime mit Syscall-Bridge fuer F$Time/F$ID/F$Exit).
- **2026-07-04 — Phase 4.5 (Signale: F$Send + F$Icpt + F$RTE)** ✅: schließt Phase 4 (Prozesse)
  ab. **syscall.h**: `F_ICPT` ($09) und `F_RTE` ($1E) neu, `F_SEND` ($08) war schon reserviert.
  **proc.h/.c**: vierter Weckgrund `Q9_WAIT_SIGNAL` (bereits seit 4.4 als Enum-Wert vorhanden,
  jetzt erstmals genutzt) + drei neue `q9_pd_t`-Felder: `icpt_handler` (`q9_proc_step_fn`,
  `NULL` = kein Handler installiert), `pending_signal` (letztes zugestelltes Signal, lesbar über
  `q9_proc_current()->pending_signal` — kein neuer Getter nötig, das Feld ist schon öffentlich),
  `in_intercept` (1 = Scheduler ruft `icpt_handler()` statt der normalen Step-Funktion).
  `q9_proc_icpt(pid, handler)` installiert/deinstalliert. `q9_proc_send(pid, signal)`: bricht
  `WAITING`/`SLEEPING` UNABHÄNGIG vom Weckgrund sofort ab (Zustand → `ACTIVE`); ist zusätzlich
  ein Handler installiert, wird `pending_signal` gesetzt und `in_intercept` aktiviert — ohne
  Handler bleibt es beim reinen Aufwecken. `q9_proc_rte(pid)` beendet den Intercept-Modus wieder
  (`E$IPrcID`, falls gerade gar keiner läuft — F$RTE ins Leere ergibt keinen Sinn).
  **`q9_proc_schedule()`**: pro Tick jetzt `if (in_intercept && icpt_handler) icpt_handler();
  else if (step) step();` statt immer nur `step()` — minimal-invasive Umschaltung, der Rest des
  Scheduler-Kerns (Round-Robin, Weckgrund-Checks für WAITING/SLEEPING) bleibt unverändert.
  **syscall.c**: `F_SEND`/`F_ICPT`/`F_RTE` im Dispatcher — F$Icpt/F$RTE wirken (anders als
  F$SSpd/F$SPrior) bewusst NUR auf den aufrufenden Prozess selbst (wie in echtem OS-9), `a0` trägt
  bei F$Icpt den rohen Funktionszeiger (erster Syscall, der das tut — bisher liefen
  Step-Funktionszeiger nur intern über F$Fork/F$Chain + Q9_MOD_NATIVE, s. Entscheidung E9).
  **Selbsttests** (kernel.c): `icpt_test_step`/`icpt_handler_step` beweisen die volle Kette —
  Prozess installiert beim ersten eigenen Tick seinen Handler, F$Send lenkt den nächsten
  Scheduler-Aufruf auf ihn um (Signal-Nummer korrekt über `pending_signal` lesbar), F$RTE schaltet
  zurück, der übernächste Tick läuft wieder normal. Zweiter Test: F$Send auf einen WAITING-Prozess
  OHNE installierten Handler weckt ihn nur (kein Umweg über einen Handler). Fehlerpfad-Checks
  (unbekannte PID, F$Icpt/F$RTE außerhalb eines Prozesses) → `E$IPrcID`.
  docs/SYSCALLS.md (neuer $08/$09/$1E-Abschnitt), docs/SYSCALL_ROADMAP.md (Status-Spalten),
  docs/HANDBUCH.md (Abschnitt 5.6 komplettiert, Abschnitt 6 Phase 4 auf „fertig" gesetzt)
  aktualisiert. `make clean && make native && make test` PASS, warnungsfrei; zusätzlich mit
  AddressSanitizer/UBSan gegenverifiziert (sauber). `make wasm` baut warnungsfrei (reine
  Kernel-Logik, kein HAL-Bezug). **Damit ist Phase 4 (Prozesse) vollständig abgearbeitet** — kein
  weiterer 🟢-Ready-Schritt mit Wer=Claudia offen; nächste Schritte (Phase 5, Shell) müssen erst
  mit Andreas besprochen und freigegeben werden.
- **2026-07-04 — Phase 4.4 (F$SSpd + F$SPrior)** ✅: zwei unabhängige, kleine Ergänzungen zum
  Prozessmodell. **syscall.h**: neue Funktionsnummern `F_SSPD` ($0B) und `F_SPRIOR` ($0D).
  **proc.h/.c**: vierter Weckgrund `Q9_WAIT_SIGNAL` (neuer Enum-Wert in `q9_wait_reason_t`) +
  neues `priority`-Feld (`uint8_t`) in `q9_pd_t`. `q9_proc_suspend(pid)` versetzt eine beliebige
  bekannte PID nach `Q9_PS_WAITING`/`Q9_WAIT_SIGNAL` — bewusst OHNE eigenen Weckcheck in
  `q9_proc_schedule()` (der `default: return 0;`-Zweig in `wait_condition_met()` deckt das
  automatisch ab, ganz ohne Sonderfall): ein so suspendierter Prozess bleibt dauerhaft stehen,
  bis `F$Send` (4.5) `WAITING`/`SLEEPING` unabhängig vom Weckgrund gewaltsam abbricht.
  `q9_proc_set_priority(pid, new, &old)` setzt/liefert das reine Datenfeld — der Scheduler
  bleibt Round-Robin, Priorisierung/Aging lohnt sich erst bei echter Konkurrenz um Rechenzeit.
  **syscall.c**: `F_SSPD`/`F_SPRIOR` im Dispatcher, beide mit `d0.w` = Ziel-PID (`0` = aufrufender
  Prozess — `E$IPrcID`, falls das außerhalb eines Scheduler-Aufrufs verwendet wird, analog zu
  F$Chain); unbekannte PID liefert ebenfalls `E$IPrcID`.
  **Selbsttests** (kernel.c): `sspd_test_step` (zählt nur mit, wie oft der Scheduler ihn steppt)
  beweist, dass F$SSpd den Zähler dauerhaft einfriert (kein Aufwachen ohne 4.5); F$SPrior-Check
  setzt zweimal hintereinander die Priorität von PID 1 und prüft den jeweils zurückgelieferten
  alten Wert, plus ein Fehlerpfad-Check je Syscall (unbekannte PID → `E$IPrcID`). Fork über
  `q9_proc_fork()` direkt (wie schon bei 4.3), kein Modul-Directory-Slot belegt.
  docs/SYSCALLS.md (neue $0B/$0D-Abschnitte), docs/SYSCALL_ROADMAP.md (Status-Spalten),
  docs/HANDBUCH.md (Abschnitt 5.6 Prozessmodell) aktualisiert. `make clean && make native &&
  make test` PASS, warnungsfrei; zusätzlich mit AddressSanitizer/UBSan gegenverifiziert (sauber).
  `make wasm` baut warnungsfrei (reine Kernel-Logik, kein HAL-Bezug). **Nächster Ready-Schritt:
  4.5** (Signale: F$Send/F$Icpt/F$RTE).
- **2026-07-04 — Phase 4.3 (echtes Blockieren: Waiting/Sleeping + Weckgrund, F$Sleep)** ✅: ersetzt
  das E$NotRdy-Poll-Provisorium (I$Read/I$ReadLn seit 1.2, F$Wait seit 4.2) durch echte
  Scheduler-Zustandswechsel — kein eingefrorener Stack (E8 gilt weiter), aber der Scheduler
  steppt blockierte Prozesse gar nicht erst, statt sie bei jedem Tick sinnlos erneut aufzurufen.
  **proc.h/.c**: neuer Enum `q9_wait_reason_t` (`Q9_WAIT_NONE/DEVICE/CHILD/TIMER`) + drei neue
  `q9_pd_t`-Felder (`wait_reason`, `wait_dev`, `wake_tick`); neue Funktionen
  `q9_proc_wait_device(pid, dev)` (WAITING/DEVICE), `q9_proc_wait_child(pid)` (WAITING/CHILD),
  `q9_proc_sleep(pid, ticks)` (SLEEPING/TIMER, `wake_tick` = aktueller Tick-Zähler + ticks,
  `ticks == 0` → +1 = "einmal yielden"). Neuer statischer Tick-Zähler `q9_tick` (proc.c, zählt
  abgeschlossene Scheduler-Durchläufe). `q9_proc_schedule()` prüft je Slot VOR dem Stepp-Aufruf,
  ob ein WAITING/SLEEPING-Weckgrund erfüllt ist (`dev_ready()` fragt `SS.Ready` ab — kein
  Gerät/keine getstat-Op zählt sofort als bereit, sonst würde ein Prozess ohne Weckmöglichkeit
  für immer hängen; `zombie_child_exists()` sucht ein Zombie-Kind), weckt dann auf ACTIVE und
  steppt im selben Tick weiter.
  **syscall.c**: `sc_read` (I$Read/I$ReadLn) versetzt bei `E$NotRdy` auf dem Treiberpfad (kein
  File-Manager — FAT16-`E$NotRdy` bleibt ein echter I/O-Fehler, kein Weckgrund) den aufrufenden
  Prozess per `q9_proc_wait_device` in WAITING; der Rückgabewert an den Aufrufer bleibt
  `E$NotRdy` (kein eingefrorener Stack möglich). `F$Wait` ruft bei `E$NotRdy` zusätzlich
  `q9_proc_wait_child` auf. Neuer Dispatcher-Case **F$Sleep** ($0A, Register schon seit 1.0 in
  syscall.h reserviert): `d1.l` Ticks, ruft `q9_proc_sleep`; außerhalb eines Prozesses (kein
  `q9_proc_current()`) → `E$IPrcID`, analog zu F$Chain.
  **dev_term.c**: nur Kommentare präzisiert (Treiber selbst bleibt unverändert
  nicht-blockierend — das Blockieren sitzt eine Ebene höher in `syscall.c`).
  **Selbsttests** (kernel.c): drei neue Prozess-Step-Funktionen — `readln_block_step` (ruft
  I$ReadLn auf stdin; im Testharness nie Eingabe, stdin ist nicht-blockierend ohne echte
  Tastatureingabe, s. `hal_posix.c` — bleibt deshalb deterministisch WAITING) beweist, dass der
  Scheduler einen blockierten Prozess NICHT erneut steppt (Zähler bleibt nach dem ersten Tick
  stehen); `wait_parent_step` (forkt einen Enkel, der sich sofort beendet, ruft F$Wait) beweist
  WAITING/Q9_WAIT_CHILD + automatisches Aufwachen bei Zombie — die Wartschleife im Test ist
  bewusst NICHT auf einen festen Tick-Wert fixiert, weil die Reihenfolge von Enkel/Parent im
  Round-Robin von ihrer Tabellenposition abhängt; `sleep_test_step` (F$Sleep(2), beendet sich
  erst beim zweiten Aufruf) beweist SLEEPING/Q9_WAIT_TIMER über genau die erwartete Anzahl
  Ticks. Alle drei forken direkt über `q9_proc_fork()` (proc.c-API) statt über F$Fork/
  Modul-Directory — kein registriertes Modul nötig, belegt also keinen der nur 8
  Modul-Directory-Slots dauerhaft (ein erster Anlauf über F$Fork + `q9_mod_register()` hatte
  genau das getan und dadurch spätere modulverzeichnis-abhängige Selbsttests zum Scheitern
  gebracht, weil das Directory dann schon voll war, bevor sie liefen).
  **Bugfix im Zuge dessen**: das feste `checks[64]`-Array im Selbsttest (kernel.c) war mit den
  drei neuen Checks auf 66 Einträge gewachsen → Stack-Buffer-Overflow (per AddressSanitizer
  verifiziert, `checks[96]` behebt es mit Luft nach oben).
  docs/SYSCALLS.md ($8B/$89 I$Read/I$ReadLn-Blockier-Hinweis, F$Wait-Abschnitt, neuer
  F$Sleep-Abschnitt, „Bewusste Abweichungen"-Punkt 1 aktualisiert), docs/SYSCALL_ROADMAP.md
  (F$Sleep ✅, F$Wait-Notiz aktualisiert), docs/HANDBUCH.md (Abschnitt 5.6 Prozessmodell)
  aktualisiert. `make clean && make native && make test` PASS, warnungsfrei; zusätzlich mit
  AddressSanitizer/UBSan gegenverifiziert (sauber). `make wasm` baut warnungsfrei (reine
  Kernel-Logik, kein HAL-Bezug — Browser-Verifikation nicht nötig, da keine wasm-HAL-Datei
  berührt wurde). **Nächster Ready-Schritt: 4.4** (F$SSpd + F$SPrior).
- **2026-07-04 — Phase 4.2 (F$Fork + F$Exit + F$Wait + F$Chain)** ✅: echte Mehrprozess-Semantik
  auf dem 4.1-Fundament. **Design-Entscheidung E9** (PROJECT.md, neu): Q9 hat vor Phase 6 keine
  68k/WASM-Ausführungs-Engine, die aus einem geladenen Modul heraus echten Byte-Code starten
  könnte — deshalb neues Language-Byte `Q9_MOD_NATIVE` (module.h, Wert 4): ein solches Modul
  trägt direkt hinter dem Header (Offset `execoff`) einen rohen `q9_proc_step_fn`-Funktionszeiger
  statt Byte-Code (gültig nur innerhalb desselben laufenden Host-Prozesses — NICHT Teil eines
  portablen Moduldateiformats, reine Übergangslösung). `q9_proc_native_entry()` (proc.c) liest
  diesen Zeiger, `E$NEMod` ($EA, MWOS-verifiziert) bei jeder anderen Sprache.
  **F$Fork** (proc.c: `q9_proc_fork`): sucht das Modul über `q9_mod_link` (wie F$Link, erhöht den
  Link-Count), validiert `Q9_MOD_NATIVE`, alloziert einen neuen Tabellenslot (Parent = aufrufende
  PID, Std-Pfade vom Parent geerbt, Zustand ACTIVE). `E$MNF` (Modul nicht gefunden), `E$NEMod`
  (nicht nativ), `E$PrcFul` (Tabelle voll, $E5, MWOS-verifiziert).
  **F$Exit** (proc.c: `q9_proc_exit`) hat jetzt echte Semantik statt des 1.2-Stubs: Exit-Code
  merken, verlinktes Modul entlinken; hat der Prozess einen Parent, wird er **Zombie**
  (`Q9_PS_ZOMBIE`, neuer Enum-Wert — bewusst kein E8-Scheduler-Zustand, hält nur Exit-Code/PID
  bis zum Reap vor); ohne Parent (z.B. PID 1) wird der Slot sofort frei, weil niemand reapen
  kann. Der alte `q9_proc_halted()`-Notbehelf (samt der globalen `proc_halted`-Variable in
  syscall.c) ist komplett entfernt — der Scheduler ruft nicht-ACTIVE-Prozesse ohnehin nie mehr
  als Step-Funktion auf, das reicht als Guard.
  **F$Wait** (proc.c: `q9_proc_wait`): sucht ein Zombie-Kind der aufrufenden PID und reapt es
  (Tabellenslot frei, Exit-Code/PID zurückgegeben). Noch laufendes, aber kein beendetes Kind:
  `E$NotRdy` (Provisorium — Aufrufer pollt, wie bei I$Read vor 4.3; echtes Blockieren kommt mit
  4.3). Nie ein Kind gehabt: `E$NoChld` ($E2 — bestätigt eine Altnotiz aus syscall.h 1.40, die
  genau diesen Wert schon 2026-07-03 für spätere Verwendung reserviert hatte).
  **F$Chain**: ersetzt das Modul des aufrufenden Prozesses (PID/Parent/Std-Pfade bleiben,
  Exit-Code auf 0 zurückgesetzt); altes Modul wird entlinkt, bevor das neue verlinkt wird
  (`E$NEMod` lässt das alte Modul unangetastet, falls das neue nicht nativ ist).
  **Selbsttests** (kernel.c): `build_native_module()` (Analogon zu `build_module()`, baut ein
  echtes `Q9_MOD_NATIVE`-Modul samt Funktionszeiger) + `child_step` (beendet sich sofort mit
  Exit-Code 42) beweisen Fork→ein Tick läuft→Exit→Wait reapt; zusätzlich `child_before_chain_step`/
  `child_after_chain_step` beweisen F$Chain (Kind verkettet sich nach dem ersten Tick auf ein
  anderes Modul, der ZWEITE Tick führt schon die neue Step-Funktion aus, Exit-Code 77 bestätigt
  den Wechsel). Je ein Fehlerpfad-Check für F$Fork (unbekanntes Modul, nicht-natives Modul) und
  F$Wait (nie ein Kind gehabt). docs/SYSCALLS.md ($03/$04/$05/$06-Abschnitte + Registerformate),
  docs/SYSCALL_ROADMAP.md (Status-Spalten), docs/HANDBUCH.md (Abschnitt 5.6 Prozessmodell,
  Abschnitt 8 Glossar: `Q9_MOD_NATIVE`/Zombie-Prozess), PROJECT.md (Entscheidung E9) aktualisiert.
  `make clean && make test` PASS, warnungsfrei. wasm nicht angefasst (reine Kernel-Logik).
  **Bewusst nicht Teil von 4.2** (Ideenspeicher): Reparenting verwaister Kind-Prozesse auf PID 1.
  **Nächster Ready-Schritt: 4.3** (echtes Blockieren).
- **2026-07-04 — Phase 4.1 (Prozess-Descriptor-Tabelle + Round-Robin-Scheduler)** ✅: neue Datei
  `src/kernel/proc.c/.h` — statische Tabelle `q9_pd_t proctab[Q9_NPROCS]` (Q9_NPROCS=8, kein
  malloc, analog zur Gerätetabelle in device.c): `pid`/`parent`/`module` (Zeiger auf
  `q9_modhdr_t`, `NULL` = interner/nativer Prozess ohne geladenes Modul)/`state`
  (`Q9_PS_FREE`/`ACTIVE`/`WAITING`/`SLEEPING` — Waiting/Sleeping als Enum-Werte für 4.3 schon
  vorgesehen, aber von 4.1 noch nicht erzeugt)/`exitcode`/`stdpath[3]` (eigene Std-Pfade 0/1/2 —
  Indizes in die weiterhin globale Pfadtabelle, device.h)/`step` (Funktionszeiger `void(*)(void)`
  — die Step-Funktion, Entscheidung E8). `q9_proc_schedule()`: Round-Robin über die komplette
  Tabelle, ruft jeden `ACTIVE`-Prozess einmal pro Aufruf; `q9_proc_current()` liefert während des
  Aufrufs den gerade gestepten Prozess (für Syscalls wie F$ID). `q9_proc_init(step)` legt PID 1
  an (Parent 0 = Kernel, kein Modul, Std-Pfade 0/1/2, Zustand ACTIVE).
  **Verdrahtung** (kernel.c): der bisherige Körper von `q9_kernel_step()` (I$ReadLn-Poll, "exit"
  → F$Exit, Prompt) wandert unverändert in eine neue Funktion `repl_step()`; `q9_kernel_init()`
  registriert sie über `q9_proc_init(repl_step)` als PID 1; `q9_kernel_step()` selbst wird zu
  `q9_proc_schedule()`. **F$ID** (syscall.c) liest die PID jetzt über `q9_proc_current()` aus der
  Tabelle statt sie fest zu verdrahten (Fallback PID 1, wenn außerhalb eines Scheduler-Aufrufs
  aufgerufen — z.B. der Selbsttest-Syscall direkt nach dem Boot, bevor der erste Tick lief).
  **Bewusst nicht Teil von 4.1** (kommt mit 4.2): F$Fork/F$Exit/F$Wait/F$Chain — F$Exit bleibt der
  bisherige Stub (setzt nur `proc_halted`, räumt den Tabellen-Slot noch nicht ab); es gibt bis
  4.2 ohnehin nur den einen Prozess. 2 neue Selbsttest-Checks (kernel.c: PID 1 hat
  Parent 0/kein Modul/Zustand ACTIVE/Std-Pfade 0/1/2 aus der echten Tabelle; unbekannte PID
  liefert NULL). Makefile (`KSRC`/`HDRS`) um proc.c/.h ergänzt. docs/SYSCALLS.md (F$ID-Abschnitt)
  und docs/HANDBUCH.md (Abschnitt 3 Quellcode-Layout, Abschnitt 5.1 Schichtenmodell, Abschnitt 5.6
  Prozessmodell) aktualisiert. `make test` PASS, warnungsfrei. wasm nicht angefasst (reine
  Kernel-Logik, kein HAL-Bezug) — Build/Verifikation folgt spätestens, sobald ein Schritt die
  wasm-HAL berührt. **Nächster Ready-Schritt: 4.2** (F$Fork/F$Exit/F$Wait/F$Chain).
- **2026-07-04 — Phase 3.7 (FAT16-Directory-Einträge: echtes Datum/Uhrzeit)** ✅: neue
  Hilfsfunktion `fat_pack_datetime()` (fat16.c) liest `q9_hal_time()` (existiert bereits seit
  1.9) und packt sie ins FAT16-Datum/Zeit-Format (Datum: Bit15-9 Jahr seit 1980, Bit8-5 Monat,
  Bit4-0 Tag; Zeit: Bit15-11 Stunde, Bit10-5 Minute, Bit4-0 Sekunde/2 — FAT16 löst Sekunden nur
  in 2er-Schritten auf). Ohne Zeitquelle (`q9_hal_time` liefert `-1`) oder bei Jahr < 1980
  (vor der FAT16-Epoche) bleiben beide Felder wie bisher 0 — reiner Fallback, kein Fehlerfall.
  `fat16_create` und `fat16_makdir` befüllen damit `crtdate`/`crttime`/`wrtdate`/`wrttime`/
  `lstaccdate` neu angelegter Directory-Einträge — bei `fat16_makdir` zusätzlich die `.`/`..`-
  Einträge im neuen Verzeichnis-Cluster selbst (vorher: Nullfelder überall, macOS zeigte
  entsprechend „1.1.1970" als Anlegedatum, wie in der Live-Demo 2026-07-04 aufgefallen).
  `test/06_test_fat16.py` (`post_validate()`) um zwei neue, komplett unabhängige
  Python-Nachvalidierungs-Checks erweitert: `crtdate`/`wrtdate` von `NEU.TXT` (I$Create, bleibt
  nach I$Delete im Rest-Dirent stehen — nur `name[0]` wird auf `DIRENT_FREE` gesetzt) und von
  `NEUDIR` samt seinem eigenen `.`-Eintrag (I$MakDir) sind ungleich 0. docs/DEVICES.md
  (FAT16-Abschnitt) um den Datum/Uhrzeit-Absatz ergänzt. `make test` PASS, warnungsfrei
  (6 Testskripte, jetzt mit 2 zusätzlichen Checks). wasm nicht betroffen (reine Kernel-Logik,
  keine HAL-Änderung). **Damit ist Phase 3 wieder vollständig abgearbeitet** — offene
  🟢-Ready-Schritte mit Wer=Claudia sind jetzt nur noch 4.1–4.5 (Phase 4, Prozesse).
- **2026-07-04 — Phase 3.6 (wasm-HAL: OPFS-Blockgerät + Upload/Download)** ✅: `q9_hal_blk_read`/
  `q9_hal_blk_write` (`src/hal/wasm/hal_wasm.c`) sind jetzt über `globalThis.q9blk` an einen
  echten Blockspeicher gebunden statt an den bisherigen `-1`-Stub. **Design-Entscheidung**:
  Browser-API `FileSystemSyncAccessHandle` (synchroner OPFS-Zugriff, zwingend für einen
  synchronen C-Syscall wie `q9_hal_blk_read`) existiert nur innerhalb eines Dedicated Workers —
  deshalb wandert der komplette Kernel (WASM-Instanz + `q9_kernel_step()`-Loop) in einen neuen
  `web/worker.js`; `web/index.html` bedient nur noch xterm.js und tauscht mit dem Worker per
  `postMessage` aus (Tastendrücke rein, UTF-8-Konsolen-Fragmente raus — Nachrichtenformat siehe
  docs/DEVICES.md, Abschnitt „wasm-HAL-Backend für /d0 (OPFS)"). Kein rAF im Worker (Worker haben
  keinen `requestAnimationFrame`) — `setInterval(pump, 16)` plus Input-getriebenes Pumpen reicht.
  `q9blk.read`/`q9blk.write` kopieren zwischen `HEAPU8` und einem `Uint8Array(512)` (analog zum
  bestehenden `q9host.putc/getc`-Muster); Lesen über das Image-Ende hinaus liefert Nullblöcke
  statt Fehler. Ein frisches `q9disk.img` (OPFS, `navigator.storage.getDirectory()`) wird beim
  ersten Worker-Start auf 2880 Blöcke (1,44 MB) gebracht (letzten Block einmal schreiben).
  **Image-Upload/-Download** im Frontend: Toolbar mit zwei Buttons — Upload liest die gewählte
  Datei per `File.arrayBuffer()` und schickt sie als transferable `ArrayBuffer` (`load-image`) an
  den Worker, der `q9disk.img` per `truncate`+`write` komplett ersetzt; Download fragt das ganze
  Image beim Worker an (`get-image`) und löst über `Blob`+`<a download>` einen Browser-Download
  aus. Makefile (`wasm`-Target) kopiert `web/worker.js` jetzt mit nach `build/wasm/` (wie bisher
  schon `index.html`). docs/DEVICES.md um Abschnitt „wasm-HAL-Backend für /d0 (OPFS)" ergänzt.
  **wasm-Build und Browser-Test dieses Schritts sind ungetestet** — emsdk fehlt sowohl auf dem
  Desktop-PC als auch auf diesem Mac Mini (siehe „Geparkt"); `make test` (nativ, unverändert von
  diesem Schritt betroffen) bleibt PASS, warnungsfrei. JS-Syntax von `worker.js` und dem
  Inline-Script in `index.html` wurde per `node --check` verifiziert, ersetzt aber keinen echten
  Browser-Test (OPFS-API-Verhalten, Worker-Timing, `importScripts`-Reihenfolge relativ zu
  `Module`-Hooks). **Damit ist Phase 3 vollständig abgearbeitet** — kein weiterer 🟢-Ready-Schritt
  mit Wer=Claudia offen (Phase 4 steht komplett auf 💡 Vorschlag, wartet auf Freigabe durch
  Andreas).
- **2026-07-04 — Phase 3.5 (F$Load)** ✅: `q9_mod_load` (module.c/.h) — Modul aus einer echten
  Datei laden (statt nur ROM-Image), ueber die VFS-Schicht (`q9_vfs_open`, braucht einen File-
  Manager hinter dem Geraet, z.B. FAT16 an `/d0`). Erste Speicherverwaltung im Kernel: statischer
  Load-Puffer-Pool (`Q9_MOD_LOADBUF_COUNT` = 4 Slots à `Q9_MOD_LOADBUF_SIZE` = 4096 Byte, kein
  malloc). **Design-Entscheidung** (Umfang war laut Notiz zu klaeren): die ganze Datei WIRD das
  Modul — anders als beim ROM-Image gibt es keine Sync-Suche, der Header MUSS bei Byte 0 stehen
  (deshalb bekam `q9_mod_validate` zusaetzlich eine explizite Sync-Pruefung, die vorher implizit
  durch den vorgeschalteten ROM-Scan abgedeckt war). Q9 nimmt bewusst den vollen Dateipfad
  entgegen statt (wie echtes OS-9) eine Execution-Search-List aus Verzeichnissen zu durchsuchen —
  Q9 hat noch keine Suchliste (kommt fruehestens mit Prozessen/Shell in Phase 4). `mod_register_ex`
  (gemeinsamer Unterbau von `q9_mod_register` und `q9_mod_load`) merkt sich in der Modul-Directory
  zusaetzlich die Puffer-Herkunft (ROM/eingebaut vs. Load-Puffer-Index) — **Lebensdauer-
  Unterschied**: ein ROM-Modul bleibt bei Link-Count 0 registriert (kostet ja keinen Speicher),
  ein per F$Load geladenes Modul wird bei Link-Count 0 SOFORT aus der Directory entfernt und sein
  Puffer freigegeben (`q9_mod_unlink` erweitert) — sonst waere der kleine Puffer-Pool nach wenigen
  Load/Unlink-Zyklen erschoepft. Kein Ghost/Sticky-Attribut ausgewertet (Ideenspeicher). Neuer
  Fehlercode `E$NoRAM` ($ED, MWOS-verifiziert) fuer vollen Puffer-Pool/zu grosse Datei. Dispatcher
  (syscall.c): neuer Case F_LOAD, liefert wie F$Link a1=Header/a2=Einsprung/d0.b=Revision.
  Selbsttest (kernel.c) schreibt sich ein Testmodul selbst per I$Create/I$Write auf `/d0/LOADMOD.BIN`
  (Nagelprobe Phase 2+3 zusammen), laedt es per F$Load, prueft F$Link auf denselben Header, baut
  beide Referenzen per F$UnLink ab, sowie Fehlerfaelle (fehlende Datei -> E$PNNF, kaputte Sync-
  Bytes -> E$BMHP). Testdateien werden am Ende selbst wieder geloescht (`test/06_test_fat16.py`
  hat nur 16 Root-Slots/8 Cluster; die Python-Nachvalidierung dort prueft feste Cluster-Nummern
  fuer NEU.TXT/NEUDIR und wuerde sonst durch liegengebliebene Load-Testdateien verfaelscht — die
  F$Load-Checks laufen deshalb bewusst VOR dem NEU.TXT-Delete, s. Kommentar in kernel.c). docs/
  SYSCALLS.md (neuer F$Load-Abschnitt), docs/SYSCALL_ROADMAP.md, docs/MODULES.md (Load-Puffer-
  Pool-Abschnitt unter F$Load), docs/DEVICES.md aktualisiert. `make test` PASS, warnungsfrei
  (6 Testskripte, mehrfach wiederholt zur Determinismus-Pruefung). wasm ungetestet (emsdk fehlt
  lokal weiterhin, siehe Geparkt). **Naechster Ready-Schritt: 3.6** (wasm-HAL OPFS).
- **2026-07-04 — Phase 3.4 (FAT16 schreibend)** ✅: `fat16.c/.h` um I$Create/I$MakDir/I$Delete
  und einen echten `fat16_write` erweitert (vorher Gerüste seit 3.2). Neue Helfer: `fat_alloc`
  (freien Cluster linear ab 2 suchen, als EOC markieren), `fat_set`/`fat_free_chain` (FAT-Eintrag
  setzen bzw. komplette Kette freigeben — **immer in beide FAT-Kopien**, wichtig für Interop mit
  macOS/Windows, die im Zweifel die zweite Kopie lesen), `dir_alloc_slot` (freien/gelöschten
  Directory-Slot suchen, Unterverzeichnis-Kette bei Bedarf um einen genullten Cluster verlängern
  — Root-Directory ist fester Bereich und wächst NICHT), `resolve_parent` (Pfad bis zum
  Elternverzeichnis auflösen + letztes Namenselement liefern), `make_83name` (validiert reinen
  8.3-Namen, `E$BPNam` bei LFN-pflichtigem Namen — **LFN-Schreiben bleibt Ideenspeicher**).
  `fat16_create`: legt Dirent mit Cluster=0/Größe=0 an (erster Cluster kommt lazy mit dem ersten
  I$Write), `E$BPNam` bei bereits vergebenem Namen (kein Truncate-Flag im mode-Byte — ein I$Open
  im Update-Modus reicht zum Überschreiben). `fat16_makdir`: alloziert sofort einen Cluster mit
  `.`/`..`-Standardeinträgen (`..` im Root-Unterverzeichnis zeigt auf Cluster 0 — FAT16-Konvention).
  `fat16_remove`: gibt Cluster-Kette frei, markiert Dirent als gelöscht (`DIRENT_FREE`/$E5) —
  bewusst KEINE Prüfung auf "Verzeichnis nicht leer" (Ideenspeicher, falls später nötig).
  `fat16_write`: Read-Modify-Write pro Sektor, alloziert neue Cluster ans Kettenende bei Bedarf,
  schreibt am Ende Größe (und ggf. Start-Cluster) ins Directory zurück (`fmctx.dir_start`/
  `dir_index` dafür in 3.4 neu, seit I$Open/I$Create mitgeführt). `fat16_ctx_t` dafür um
  `dir_start`/`dir_index` erweitert (16 → 24 Byte, `Q9_FMCTX_SIZE` in device.h angepasst).
  Dispatcher (syscall.c): I$Create/I$MakDir/I$Delete routen jetzt echt auf `fm->create/makdir/
  remove()` (statt `E$UnkSvc`); I$Write routet auf `fm->write()`, wenn ein File-Manager mit
  write-Op hinter dem Pfad steht (sonst weiter an die Treiber-Op, rückwärtskompatibel).
  Makefile: `test`-Target löscht `q9disk.img` vor dem Lauf (sonst kann ein FAT16-Image aus einem
  früheren Lauf — z.B. mit dem 06-Selbsttest-`NEUDIR`— die Tests 01-05 verwirren, bevor 06 es neu
  aufbaut). `test/06_test_fat16.py` um I$Create/I$Write/I$MakDir/I$Delete-Checks samt einer
  komplett unabhängigen Python-`post_validate()`-Nachvalidierung erweitert (beide FAT-Kopien
  byteidentisch, `NEU.TXT` im Root als `DIRENT_FREE` markiert, `NEUDIR` als echtes Verzeichnis mit
  korrekten `.`/`..`-Einträgen im zugehörigen Cluster, Ex-`NEU.TXT`-Cluster nach dem Löschen in
  beiden FAT-Kopien wieder frei) — das ist der praktikable, reproduzierbare Ersatz für "am Mac
  mounten", weil ein komplett unabhängiger Parser dieselbe Interop-Aussage belegt.
  docs/SYSCALLS.md/SYSCALL_ROADMAP.md/DEVICES.md aktualisiert. `make test` PASS, warnungsfrei
  (6 Testskripte, alle Checks grün). wasm ungetestet (emsdk fehlt lokal weiterhin, siehe Geparkt).
  **Gegentest zusätzlich mit echtem Mounten durchgeführt**: `hdiutil attach -imagekey
  diskimage-class=CRawDiskImage` auf das von Q9 geschriebene Image, `diskutil mount` — macOS
  erkennt das Volume (`Q9TESTVOL`), zeigt `HELLO.TXT`/die LFN-Datei/`NEUDIR` korrekt an,
  `NEUDIR` funktioniert als echtes Verzeichnis (`.`/`..` werden von macOS akzeptiert), `NEU.TXT`
  ist wie erwartet weg; danach sauber mit `diskutil eject` wieder ausgehängt. **Nächster Ready-
  Schritt: 3.5** (F$Load: Modul aus Datei laden) oder 3.6 (wasm-HAL OPFS).
- **2026-07-04 — Phase 3.3 (FAT16 lesend)** ✅: `src/kernel/fat16.c/.h` neu — erster echter
  File-Manager an `/d0`. Boot-Sektor (BPB) plausibilisieren (BytesPerSector `==Q9_BLK_SIZE`,
  SectorsPerCluster Zweierpotenz, FATSize16/RootEntryCount/NumFATs `!=0`, Boot-Signatur
  `$55AA`), Root-Directory (fester Bereich) und Unterverzeichnisse (Cluster-Kette) Slot für
  Slot durchsuchen, dabei 8.3- UND LFN-Namen zusammensetzen und case-insensitiv vergleichen
  (LFN-Einträge liegen laut Spezifikation in absteigender Sequenz VOR dem 8.3-Eintrag — die
  Teile werden rückwärts zu einem vollständigen Namen zusammengesetzt). FAT16-Cluster-Ketten
  über die erste FAT-Kopie folgen (`$FFF8`-`$FFFF` = Ende). Datei-Kontext `fat16_ctx_t` (16
  Byte: start_cluster/cur_cluster/pos/size) passt exakt in `Q9_FMCTX_SIZE`.
  **Design-Entscheidung**: `q9_fm_t` (vfs.h, aus 3.2) um `read`/`seek`-Funktionszeiger
  erweitert — I$Read/I$Seek brauchen den Datei-Kontext im Pfad-Deskriptor, den nur der
  File-Manager interpretieren kann, nicht der Block-Treiber dahinter. Dispatcher (syscall.c):
  I$Read routet auf `fm->read()`, wenn ein File-Manager mit read-Op hinter dem Pfad steht
  (sonst weiterhin an die Treiber-Op, rückwärtskompatibel); I$Seek ist komplett neu
  (`E$UnkSvc` ohne passenden File-Manager). Verdrahtung: `q9_dev_init()` (device.c) versucht
  `q9_fat16_mount()` direkt nach der `/d0`-Registrierung und setzt den File-Manager NUR bei
  erkanntem FAT16-Superfloppy — ohne erkanntes Image bleibt `/d0` wie vor 3.3.
  **Zwei Nebenbei-Bugfixes im 3.1/3.2-Selbsttest gefunden**: (1) der 3.1-Roundtrip-Test auf
  LBA 1 überschrieb dauerhaft Testdaten — bei einem echten FAT16-Image liegt dort
  typischerweise die erste FAT-Kopie, das hätte ein gemountetes Dateisystem im Selbsttest
  zerstört; der Test sichert/stellt LBA 1 jetzt wieder her. (2) die 3.2-VFS-Selbsttests
  (Test-File-Manager) setzten `/d0`s File-Manager hart auf NULL zurück statt den
  ursprünglichen zu merken — das hätte den produktiven FAT16-Manager aus dem Selbsttest
  gerissen; jetzt wird der vorherige `fm`-Wert gesichert und wiederhergestellt.
  5 neue Selbsttest-Checks (nur aktiv, wenn `/d0` beim Boot als FAT16 erkannt wurde — sonst
  stiller Skip statt Fehler) + neues `test/06_test_fat16.py` (baut ein FAT16-Superfloppy-Image
  komplett per Python-Stdlib von Hand: Boot-Sektor/BPB, 2 FAT-Kopien, Root-Directory mit einer
  8.3-Datei, einer LFN-Datei mit langem Namen und einem Unterverzeichnis samt verschachtelter
  Datei — kein externes Tool wie hdiutil/newfs_msdos nötig). docs/SYSCALLS.md (I$Read-Routing,
  neuer I$Seek-Abschnitt, Funktionsnummern-Tabelle), docs/SYSCALL_ROADMAP.md, docs/DEVICES.md
  (q9_fm_t-Erweiterung, fmctx-FAT16-Layout-Tabelle, neuer FAT16-File-Manager-Abschnitt)
  aktualisiert. `make test` PASS, warnungsfrei (6 Testskripte). wasm ungetestet (emsdk fehlt
  weiterhin lokal, siehe Geparkt). **Nächster Schritt: 3.4** (FAT16 schreibend).
- **2026-07-04 — Phase 3.2 (VFS-Schicht)** ✅: `src/kernel/vfs.c/.h` neu — Pfad-Routing für
  `/d0/pfad/datei`: `q9_vfs_open` löst relative Pfade (kein führendes `/`) zuerst gegen ein
  globales Arbeitsverzeichnis auf (`cwd`, statischer String, `Q9_CWD_MAXLEN`=63 — pro-Prozess-
  Variante erst Phase 4, Entscheidung E8), trennt dann per F$PrsNam Gerätename/Rest-Pfad.
  Neue File-Manager-Schnittstelle `q9_fm_t` (vfs.h) — analog zu `q9_drv_t` (device.h), aber
  eine Stufe höher (Pfade/Dateien statt Blöcke/Zeichen): `open`/`create`/`makdir`/`remove`
  nehmen einen Rest-Pfad-**String** entgegen (bewusst kein OS-9-Pfaddeskriptor-Struct) — Schnitt
  mit dem Dibble-Buch ("OS-9 Insights") abgeglichen und so geschnitten, dass später ein
  68k-Manager-Adapter (Ideenspeicher) dieselbe C-Schnittstelle hinter einer Trap-Bridge
  bedienen könnte, ohne den Kernel-Teil anzufassen. `q9_dev_t` bekommt ein optionales
  `fm`-Feld (NULL = kein Dateisystem, Grundzustand aller Geräte inkl. `/d0` vor 3.3);
  `q9_path_t` bekommt `fmctx[16]` (Datei-Kontext pro Pfad, File-Manager-eigenes Byte-Array,
  kein malloc — ab 3.3 z.B. FAT16-Cluster/Position). Geräte OHNE File-Manager (`/term`, `/nil`)
  verhalten sich unverändert: Rest-Pfad muss leer sein, sonst neuer Fehlercode `E$PNNF`
  ($D8, "Path Name Not Found", MWOS-verifiziert) statt E$MNF/E$BPNam — sauberer als vorher,
  weil es explizit "Pfad im Dateisystem nicht gefunden" von "Gerät unbekannt" unterscheidet.
  `device.c` bekommt `q9_path_open_dev` (Pfad auf bereits aufgelöstes Gerät öffnen, ohne
  erneuten Namens-Lookup) als gemeinsamen Unterbau für `q9_path_open` und `q9_vfs_open`.
  Dispatcher (syscall.c): `I$Open`/`I$ChgDir` echt implementiert; `I$Create`/`I$MakDir`/
  `I$Delete` bewusst nur Gerüst (`E$UnkSvc`) — echte Semantik erst mit FAT16 schreibend (3.4).
  5 neue Selbsttest-Checks (u.a. ein Test-File-Manager NUR im Selbsttest, der beweist, dass
  das Routing bis zum Datei-Kontext im Pfad funktioniert, ohne dass FAT16 existieren muss) +
  neues `test/05_test_vfs.py`. docs/SYSCALLS.md (I$Open/I$ChgDir/I$Create/I$MakDir/I$Delete +
  E$PNNF), docs/SYSCALL_ROADMAP.md, docs/DEVICES.md (VFS-Abschnitt + Schichtendiagramm)
  aktualisiert. `make test` PASS, warnungsfrei (40 Selbsttest-Checks). wasm ungetestet (emsdk
  fehlt lokal weiterhin, siehe Geparkt). **Kein FAT16 in diesem Schritt** — das ist 3.3/3.4,
  bewusst separat. **Nächster Schritt: 3.3** (FAT16 lesend).
- **2026-07-03 — Phase 3.1 (Block-Device /d0)** ✅: `src/kernel/dev_d0.c` neu — reiner
  Blockzugriff über I$GetStt/I$SetStt: SS.BlkRd($14)/SS.BlkWr($15) (Codes aus MWOS
  `sg_codes.h`, RBF-Vorbild), d2.l = LBA, a0 = Puffer (Q9_BLK_SIZE Byte), delegiert an
  `q9_hal_blk_read`/`q9_hal_blk_write` (HAL existierte schon aus Phase 0). Normales
  I$Read/I$Write/I$ReadLn/I$WritLn ergibt ohne Dateisystem keinen Sinn — bewusst
  `E$UnkSvc`, kommt erst mit der VFS-Schicht (3.2). Fehlerfälle: kein Puffer → `E$Param`,
  HAL-Fehler (Image fehlt/I/O) → `E$NotRdy`. `/d0` in `q9_dev_init()` registriert (3.
  Treiber neben /term, /nil). 2 neue Selbsttest-Checks (Roundtrip Schreiben/Lesen auf
  LBA 1, Fehlerfälle) — 36 insgesamt. Neues `test/04_test_blkdev.py`, Makefile-Target
  `test` erweitert. `q9disk.img` (HAL-seitig lazy erzeugtes Testimage) neu in
  `.gitignore`. docs/DEVICES.md (neuer /d0-Abschnitt) + docs/SYSCALLS.md (SS.BlkRd/
  SS.BlkWr-Zeilen) aktualisiert. `make test` PASS, warnungsfrei. **Nächster Schritt:
  3.2** (VFS-Schicht: Pfad-Routing `/d0/pfad/datei`).
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

**Letzte Aktualisierung**: 2026-07-04 abends — **Phase 4 komplett (4.1–4.5),
Anschluss 4.6–4.8 freigegeben.** Andreas' Einwand: `F$Fork` führt bislang
keinen echten geladenen Code aus (E9-Stopgap, nur Funktionszeiger im selben
Host-Prozess). 4.6–4.8 lösen das für WASM (Primärformat, kein
Relozierungsproblem, aber Instanziierung + Syscall-Bridge + Pointer-Marshaling
über die Modulgrenze fehlen — siehe PROJECT.md O5/O6). Bevor Phase 5 (Shell)
sinnvoll ist, muss das stehen, sonst wäre die Shell nur derselbe
Etikettenschwindel wie der jetzige Kernel-REPL.

Davor: 2026-07-04 abends — **Phase U (Userland-Werkzeuge,
Codex-Baustelle) eingetragen und U.1–U.4 fertig.** `libq9` + sieben Datei-
Werkzeuge (cat/copy/dir/mkdir/rm/touch/stat), separater Branch
`codex-userland`, isoliert von `main`, alle Runden von Claudia unabhängig
verifiziert. Merge nach `main` steht noch aus (wartet auf Andreas' Review).
PROJECT.md O6 neu: offene Frage, wie Userland-Code später zu ladbaren
Q9-Modulen wird (WASM-Import-ABI vs. 68k-PIC).

Davor: 2026-07-04 nachmittags — **emsdk auf dem Mac Mini installiert**
(`~/emsdk`, portabel, Version 6.0.2 via `./emsdk install/activate latest`; Aktivierung pro
Shell: `source ~/emsdk/emsdk_env.sh`, siehe docs/TOOLCHAIN.md). `make wasm` baut jetzt sauber,
warnungsfrei. Browser-Boot verifiziert (Preview-Tool, Server auf build/wasm): Banner zeigt
`target: wasm-browser`, alle Geräte (/term, /nil, /d0) da, REPL antwortet und echot über die
Worker-Konsolen-Pipe (`worker.js`), keine Konsolen-/Netzwerkfehler. `q9disk.img` liegt nach
dem Boot nachweislich in OPFS (`navigator.storage.getDirectory()` zeigt die Datei) — der seit
3.6 ungetestete OPFS-Block-Pfad funktioniert also. Der wasm-Blindfleck des Autonomie-Betriebs
ist damit geschlossen; „⛔ Geparkt"-Punkt entsprechend verengt (betrifft nur noch den
Desktop-PC). **Kein Kernel-Code geändert** — reine Verifikation des von der Routine
mitgeschriebenen wasm-Zweigs (1.9, 3.6 rückwirkend bestätigt).

Davor: 2026-07-04 mittags — **Phase 3 live verifiziert (Andreas):**
FAT16-Image mit macOS/`newfs_msdos` formatiert, LFN-Datei + Unterverzeichnis-Datei am Mac
angelegt, von Q9 über I$Open/I$Read komplett gelesen; Q9 hat per I$Create/I$Write eine neue
Datei geschrieben, macOS mountet das Image danach anstandslos und liest sie — Interop-Beweis
mit echtem macOS-Tooling (nicht nur Python-Simulation wie im Selbsttest). Einziger Fund: neue
Dateien zeigen "1.1.1970" als Datum → **3.7** (FAT16-Zeitstempel) neu als 🟢 eingetragen.
**Phase 4 (4.1–4.5) auf 🟢 Ready gestellt** — Konzept stand schon, jetzt freigegeben.

Davor: **Phase 3.5 (F$Load) abgeschlossen**:
`q9_mod_load` (module.c/.h) lädt ein Modul aus einer echten Datei über die VFS-Schicht (statt nur
ROM-Image) — erste Speicherverwaltung im Kernel (statischer Load-Puffer-Pool, 4×4096 Byte, kein
malloc), automatische Freigabe bei Link-Count 0 (anders als ROM-Module). Dispatcher (syscall.c)
neuer Case F$Load, liefert wie F$Link a1=Header/a2=Einsprung/d0.b=Revision. `q9_mod_validate`
bekam eine explizite Sync-Prüfung (F$Load hat keine vorgeschaltete ROM-Scan-Suche). `make test`
PASS, warnungsfrei (6 Testskripte). wasm ungetestet (emsdk fehlt lokal). Nächster Ready-Schritt:
**3.6** (wasm-HAL OPFS).
Davor: **Phase 3.4 (FAT16 schreibend) abgeschlossen**: `src/kernel/fat16.c/.h` um I$Create/
I$MakDir/I$Delete + echtes I$Write erweitert (FAT-Ketten allozieren/freigeben in beiden
FAT-Kopien, Directory-Slots suchen/anlegen/löschen, nur 8.3-Namen). Dispatcher (syscall.c) routet
die drei Syscalls jetzt echt auf den File-Manager statt `E$UnkSvc`. Gegentest zusätzlich mit
`hdiutil attach -imagekey diskimage-class=CRawDiskImage` + `diskutil mount` durchgeführt: macOS
erkennt und mountet das von Q9 geschriebene Image, `NEUDIR` erscheint als echtes Verzeichnis,
sauber wieder ausgehängt (`diskutil eject`).
Davor: **Phase 3.3 (FAT16 lesend) abgeschlossen**: `src/kernel/fat16.c/.h` (Boot-Sektor/Root-Dir/
Cluster-Ketten, 8.3+LFN-Namen, File-Manager an `/d0` nur bei erkanntem FAT16-Superfloppy),
`q9_fm_t` um `read`/`seek` erweitert, `I$Read` routet auf den File-Manager, `I$Seek` neu im
Dispatcher. Nebenbei zwei Selbsttest-Bugfixes (LBA-1-Restore im 3.1-Test, fm-Restore im
3.2-Test). `make test` PASS, warnungsfrei (6 Testskripte, 45 Selbsttest-Checks inkl. FAT16).
Davor: **Phase 3.2 (VFS-Schicht) abgeschlossen**: `src/kernel/vfs.c/.h` (Pfad-Routing,
File-Manager-Interface `q9_fm_t`, Datei-Kontext pro Pfad, globales Arbeitsverzeichnis für
I$ChgDir), `I$Open`/`I$ChgDir` implementiert, `I$Create`/`I$MakDir`/`I$Delete` als Gerüst,
neuer Fehlercode `E$PNNF`. `make test` PASS, warnungsfrei (40 Selbsttest-Checks).
Davor: **Phase 3.1 (Block-Device /d0) abgeschlossen**, `make test` PASS, warnungsfrei
(36 Selbsttest-Checks). Phase 3 freigegeben, 3.1–3.6 auf 🟢 Ready (Wer=Claudia) gestellt;
Phase-3-Vorbesprechung mit Andreas: Schritte 3.1–3.6 als 💡 eingetragen (O1 = FAT16
entschieden, LFN lesen ja / schreiben Ideenspeicher, Superfloppy zuerst, Dibble-Buch als
Design-Referenz), drei neue Ideenspeicher-Einträge (LFN-Schreiben, MBR-Partitionen,
Original-RBF/PCF via Musashi).
Davor: **Phase 2.3 (Modul-Directory) komplett abgeschlossen** — 2.3b
(q9_mod_validate), 2.3c (q9_mod_register/find, Directory), 2.3d (F$Link/F$UnLink),
`make test` PASS, warnungsfrei (34 Selbsttest-Checks). 2.2/2.4 bleiben 💤 bis
freigegeben.
