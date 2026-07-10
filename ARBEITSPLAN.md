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

**Archiv:** Volltext-Details zu allen ✅-Schritten stehen in `ARBEITSPLAN_ARCHIV.md` (wird von der automatisierten Routine nicht gelesen).

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
| 0.1 | Toolchain installieren: w64devkit 2.8.0 (gcc 16.1.0 + make) + emsdk latest | ✅ | Claudia | portabel, ohne Admin; Doku: docs/TOOLCHAIN.md. → Details: ARBEITSPLAN_ARCHIV.md |
| 0.2 | `src/hal/q9_hal.h` ausformulieren (Konsole, Block-Device, Timer, Target-Info) | ✅ | Claudia | yield gestrichen: Host treibt q9_kernel_step(). → Details: ARBEITSPLAN_ARCHIV.md |
| 0.3 | Kernel-Minimalgerüst: `src/kernel/kernel.c`, Banner, Echo-Loop über HAL | ✅ | Claudia | nicht-blockierendes Step-Design. → Details: ARBEITSPLAN_ARCHIV.md |
| 0.4 | HAL `native/`: PC-Build (conio, Disk-Image-Stub), Makefile-Target `native` | ✅ | Claudia | Windows-only (conio); POSIX-Variante später. → Details: ARBEITSPLAN_ARCHIV.md |
| 0.5 | HAL `wasm/`: Emscripten-Build, `web/index.html` mit xterm.js, Makefile-Target `wasm` | ✅ | Claudia | q9.wasm = 1,3 KB 😄. → Details: ARBEITSPLAN_ARCHIV.md |
| 0.6 | Erster Test: `test/01_test_boot.py` (nativ) + Browser-Boot verifiziert | ✅ | Claudia | PASS; Bugfix: Konsole ist UTF-8-Bytestrom (C1-Falle). → Details: ARBEITSPLAN_ARCHIV.md |
| 0.7 | Git-Repo initialisieren + erster Commit + GitHub | ✅ | Claudia | github.com/foellmy51/Q9 (privat). → Details: ARBEITSPLAN_ARCHIV.md |

### Phase 1 — Kernel-Basis

| # | Schritt | Status | Wer | Notizen |
|---|---------|--------|-----|---------|
| 1.1 | Syscall-Design entschieden (E7): OS-9-Nummern + Registerkonventionen, ABI-Spez in docs/SYSCALLS.md | ✅ | Claudia | Nummern/Fehlercodes aus Andreas' MWOS-SDK (M:\MWOS) verifiziert. → Details: ARBEITSPLAN_ARCHIV.md |
| 1.2 | Dispatcher + erste Calls: I$Read/Write/ReadLn/WritLn, F$Exit/ID/Time; Kernel-REPL nutzt eigene Syscalls; Selbsttest + Test 02 | ✅ | Claudia | E$NotRdy statt Blockieren bis Phase 4 (dokumentiert). → Details: ARBEITSPLAN_ARCHIV.md |
| 1.3 | Device-Modell + Konsolen-Treiber als internes Modul (löst fest verdrahtete Pfade 0/1/2 ab) | ✅ | Claudia | device.c/dev_term.c, Mode-Check E$BMode, Test 03; Doku: docs/DEVICES.md. → Details: ARBEITSPLAN_ARCHIV.md |
| 1.4 | I$Dup + I$Close als Syscalls (Pfadtabelle nach außen nutzbar machen, OS-9-Semantik: Dup liefert niedrigste freie Nummer) | ✅ | Claudia | q9_path_dup, 2 neue Selbsttest-Checks, SYSCALLS.md. → Details: ARBEITSPLAN_ARCHIV.md |
| 1.5 | F$PrsNam + F$CmpNam (Pfadnamen-Parsing nach OS-9-Regeln) | ✅ | Claudia | name.c/h; E$Diff-Nummer ($E2) vorläufig → MWOS-Abgleich heute Abend. → Details: ARBEITSPLAN_ARCHIV.md |
| 1.6 | I$Attach + I$Detach: Geräte per Name ("/term") an-/abmelden, nutzt F$PrsNam | ✅ | Claudia | q9_dev_attach/detach, E$MNF neu; Link-Count-Verwaltung. → Details: ARBEITSPLAN_ARCHIV.md |
| 1.7 | Zweites Gerät /nil (Null-Device) als Mini-Treiber | ✅ | Claudia | dev_nil.c; q9_path_open jetzt namensbasiert ("/nil" mit Slash ok). → Details: ARBEITSPLAN_ARCHIV.md |
| 1.8 | I$GetStt/I$SetStt Grundgerüst: SS-Codes für /term (z.B. SS.Ready = Eingabe wartet?) | ✅ | Claudia | getstat/setstat-Ops im Treiber-IF; SS.Ready+SS.EOF; SS-Nummern beim MWOS-Abgleich prüfen. → Details: ARBEITSPLAN_ARCHIV.md |
| 1.9 | HAL-Erweiterung Echtzeit (q9_hal_time): native = localtime, wasm = Date.now → F$Time liefert echte Uhrzeit + F$STime | ✅ | Claudia | Kalenderlogik 2000–2136, Wochentag in d2; wasm-Teil ungetestet (emsdk fehlt hier). → Details: ARBEITSPLAN_ARCHIV.md |
| 1.10 | POSIX-HAL (`src/hal/posix/`, termios statt conio) + Makefile-Target, damit Q9 auf macOS/Linux baut | ✅ | Claudia | src/hal/posix/hal_posix.c (termios raw+nonblocking, clock_gettime, localtime); Makefile waehlt HAL per `$(OS)` (Windows_NT = conio, sonst POSIX)… → Details: ARBEITSPLAN_ARCHIV.md |

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
| 2.1 | Modul-Header + CRC32-Routine im Kernel | ✅ | Claudia | src/kernel/module.h/.c neu: q9_modhdr_t (28 Byte, #pragma pack, Offsets exakt wie PROJECT.md), q9_crc32 (bitweise CRC-32/ISO-HDLC, kein Table, kein… → Details: ARBEITSPLAN_ARCHIV.md |
| 2.3a | Suchfunktion: ROM-Image-Blob nach Sync-Bytes durchsuchen, nach Fund um ModuleSize zum nächsten Modul springen | ✅ | Claudia | `q9_mod_scan_first`/`q9_mod_scan_next` in module.h/.c; reine Sync-Suche, Größe/CRC-Plausibilisierung bewusst noch nicht hier (kommt in 2.3b) — nur… → Details: ARBEITSPLAN_ARCHIV.md |
| 2.3b | Validierung: Sync/Größe plausibilisieren, CRC32 nachrechnen | ✅ | Claudia | q9_mod_validate in module.h/.c: HeaderSize/ModuleSize/NameOffset-Strukturchecks vor der vollen CRC32 (billig vor teuer); Zwei-Stufen-Check… → Details: ARBEITSPLAN_ARCHIV.md |
| 2.3c | Bekanntmachen: Directory-Eintrag anlegen, Namenskollisions-/Revision-Regel (höhere Revision gewinnt, bei Gleichstand bleibt das etablierte Modul) | ✅ | Claudia | q9_mod_register/q9_mod_find in module.c: Directory als statisches Array (Q9_MOD_MAXDIR=8, kein malloc, wie devtab/pathtab). → Details: ARBEITSPLAN_ARCHIV.md |
| 2.3d | F$Link + F$UnLink als Syscalls: Suche nach Name+Type+Language, Link-Count rauf/runter | ✅ | Claudia | q9_mod_link/q9_mod_unlink in module.c + Dispatcher-Cases F_LINK/F_UNLINK in syscall.c (a0=Name,d1.b=Type,d2.b=Lang ->… → Details: ARBEITSPLAN_ARCHIV.md |
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
| 3.1 | Block-Device `/d0` als Q9-Gerät (nutzt q9_hal_blk_read/write), Roh-Blockzugriff über GetStt/SetStt-SS-Codes; Test-Image per Python in test/ | ✅ | Claudia | dev_d0.c neu: SS.BlkRd($14)/SS.BlkWr($15) aus MWOS sg_codes.h übernommen (RBF-Vorbild), reine Ops read/write/readln/writln bewusst E$UnkSvc (kein… → Details: ARBEITSPLAN_ARCHIV.md |
| 3.2 | VFS-Schicht: Pfad-Routing `/d0/pfad/datei` (F$PrsNam trennt Gerät/Rest), File-Manager als austauschbare Einheit hinter schmaler Schnittstelle, Datei-Kontext pro Pfad, globales Arbeitsverzeichnis für I$ChgDir (pro-Prozess erst Phase 4) | ✅ | Claudia | `src/kernel/vfs.c/.h` neu: `q9_fm_t` (open/create/makdir/remove, restpath-String statt OS-9-Pfaddeskriptor-Internas — schmal genug für einen… → Details: ARBEITSPLAN_ARCHIV.md |
| 3.3 | FAT16 lesend: Boot-Sektor/Root-Dir/Cluster-Ketten, I$Open + I$Read + I$Seek, Verzeichnis lesen; 8.3 **und** LFN-Namen lesen | ✅ | Claudia | `src/kernel/fat16.c/.h` neu — Boot-Sektor (BPB) plausibilisieren, Root-Dir + Unterverzeichnisse durchsuchen (8.3 UND LFN, Namensteile rückwärts… → Details: ARBEITSPLAN_ARCHIV.md |
| 3.4 | FAT16 schreibend: I$Create, I$Delete, I$MakDir, FAT-Ketten allozieren/freigeben; neue Namen nur 8.3 (LFN-Schreiben → Ideenspeicher) | ✅ | Claudia | fat16.c/.h: I$Create/I$MakDir/I$Delete/I$Write echt implementiert, FAT-Ketten allozieren/freigeben (beide FAT-Kopien synchron), nur 8.3-Namen… → Details: ARBEITSPLAN_ARCHIV.md |
| 3.5 | F$Load komplettieren: Modul aus Datei laden (statt nur ROM-Image), validieren, registrieren | ✅ | Claudia | q9_mod_load (module.c) — statischer Load-Puffer-Pool (4x4096 Byte, kein malloc), Directory-Eintraege merken Puffer-Herkunft, automatische Freigabe… → Details: ARBEITSPLAN_ARCHIV.md |
| 3.6 | wasm-HAL: Block-Backend via OPFS (FileSystemSyncAccessHandle im Worker) + Image-Upload/-Download im Frontend | ✅ | Claudia | Kernel läuft jetzt im Worker (Pflicht für Sync-Access-Handle); Details siehe „Erledigt". → Details: ARBEITSPLAN_ARCHIV.md |
| 3.7 | FAT16-Directory-Einträge bekommen echtes Datum/Uhrzeit (q9_hal_time) statt Nullfeldern bei I$Create/I$MakDir | ✅ | Claudia | `fat_pack_datetime()` (fat16.c) — Details siehe „Erledigt" unten. → Details: ARBEITSPLAN_ARCHIV.md |

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
| 4.1 | Prozess-Descriptor-Tabelle (statisch, wie devtab): PID, Parent, Modul, Zustand, Exit-Code, eigene Std-Pfade 0/1/2; Scheduler als Round-Robin über Active in q9_kernel_step() | ✅ | Claudia | `src/kernel/proc.c/.h` neu — Details siehe „Erledigt" unten. → Details: ARBEITSPLAN_ARCHIV.md |
| 4.2 | F$Fork + F$Exit + F$Wait + F$Chain: Prozess aus Modul starten (via Modul-Directory), beenden, auf Kind warten (E$NoChld $E2), verketten | ✅ | Claudia | Entscheidung E9 (Q9_MOD_NATIVE) — Details siehe „Erledigt" unten. → Details: ARBEITSPLAN_ARCHIV.md |
| 4.3 | Echtes Blockieren: E$NotRdy-Provisorium (1.2) ersetzen — Waiting-Zustand + Weckgrund, /term weckt bei Eingabe (SS.Ready-Mechanik), F$Sleep (Ticks, 0 = yield) | ✅ | Claudia | Details siehe „Erledigt" unten. → Details: ARBEITSPLAN_ARCHIV.md |
| 4.4 | F$SSpd (suspendieren) + F$SPrior (Prioritätsfeld setzen) | ✅ | Claudia | Details siehe „Erledigt" unten. → Details: ARBEITSPLAN_ARCHIV.md |
| 4.5 | Signale: F$Send, F$Icpt, F$RTE (Signal bricht Waiting/Sleeping ab, Intercept-Handler als Step-Aufruf) | ✅ | Claudia | Details siehe „Erledigt" unten — damit ist Phase 4 vollständig. → Details: ARBEITSPLAN_ARCHIV.md |

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
| 4.6 | Eingebettete WASM-Runtime für den nativen Build (löst O5): eine Bibliothek (Kandidat wasm3 — klein, embeddable, keine harte malloc-Pflicht) einbinden, die ein beliebiges `.wasm`-Modul zur Laufzeit instanziieren und eine exportierte Funktion aufrufen kann. Noch OHNE Syscall-Bridge — reiner Grundbaustein | ✅ | Claudia | wasm3 (MIT, Commit `d77cd814`) vendored unter `third_party/wasm3/` (nur Kern-Interpreter, kein WASI/libc); `src/kernel/wasmrt.c/.h` als schmaler… → Details: ARBEITSPLAN_ARCHIV.md |
| 4.7 | `Q9_MOD_WASM`-Ausführung: F$Fork/F$Chain erkennen das Language-Byte `Q9_MOD_WASM`, instanziieren das Modul über die Runtime aus 4.6 (bzw. `WebAssembly.instantiate` im Browser) mit einem Import/Host-Funktion als Syscall-Bridge. Erstmal NUR Syscalls ohne Zeiger-Parameter (F$Time, F$ID, F$Exit) | ✅ | Claudia | Native Seite fertig: `src/kernel/wasmproc.c/.h` neu (Import-Bridge `q9.f_id`/`q9.f_time`/`q9.f_exit`), `entry_step_for()` in syscall.c erkennt… → Details: ARBEITSPLAN_ARCHIV.md |
| 4.8 | Speicher-/Pointer-Marshaling über die Modulgrenze: a0–a7-Register als Offsets in die modul-eigene lineare Speicherinstanz interpretieren (statt rohe Host-Zeiger), mit Bounds-Check. Damit werden auch I$Read/I$Write/I$Open (Puffer-/Pfadnamen-Zeiger) für WASM-Module nutzbar | ✅ | Claudia | Vier neue Importe q9.i_open/i_close/i_read/i_write in wasmproc.c. → Details: ARBEITSPLAN_ARCHIV.md |
| 4.9 | `wasm3` auf Fixed-Heap umstellen (kein Host-`malloc`/`calloc`/`realloc` mehr im WASM-Pfad) + erster Baustein einer Q9-Systemkonfiguration | ✅ | Claudia | `src/kernel/config.h` neu (`Q9_SYSTEM_MEM_BYTES` = 256 KiB), Makefile leitet `d_m3FixedHeap` per Compiler-Define daraus ab. → Details: ARBEITSPLAN_ARCHIV.md |

---

### Phase 5 — 68k-Runtime (Musashi) — **umnummeriert 2026-07-04 abends, siehe Entscheidung E11**

**Noch nicht feingranular geplant** — bewusst zurückgestellt für eine eigene,
gründliche Planungsrunde (wie bei Phase 3/4), nicht nebenbei. Diese Phase war
bis 2026-07-04 abends als "Phase 6" nummeriert; die alte "Phase 5" (Shell)
ist jetzt **Phase 6** (siehe dort) — Begründung: Entscheidung E11.

**Grober Umriss aus der heutigen Diskussion** (Ausgangspunkt für die
Detailplanung, keine fertigen Schritte):
- Musashi als 68k-CPU-Kern einbinden — nativ als gewöhnliche C-Bibliothek
  (keine WASM-Ebene nötig), im Browser zu WASM kompiliert (läuft dort effizient
  über die browsereigene WASM-Engine, nicht "doppelt emuliert" im
  Performance-Sinn).
- `Q9_MOD_M68K`: echte PIC-68k-Programme laden und ausführen (vbcc-Toolchain
  noch zu beschaffen, siehe docs/TOOLCHAIN.md „Noch nicht installiert").
- TRAP→Syscall-Bridge (Musashi-Trap ruft `q9_syscall`), analog zur
  `wasmproc.c`-Bridge aus 4.7, aber auf Ebene der CPU-Emulation statt WASM-Import.
  **PIC-Pflicht für 68k-Module** (wie echtes OS-9) — keine Relozierungstabelle
  nötig, wenn vbcc konsequent PC-relativen Code erzeugt.
- Echter Scheduler auf Musashi-Ebene: CPU-Zustand (Register + Stackpointer im
  emulierten RAM) ist reine Datenstruktur, an jeder Instruktion sicherbar/
  wiederherstellbar — das eigentliche Gegenstück zum Step-Modell aus E8, nur
  eine Ebene tiefer, mit echtem Anhalten/Fortsetzen statt Kooperations-Zwang.
- Das ist die Voraussetzung für Phase 6 (Shell): Eine Shell, die vor dieser
  Phase gebaut würde, wäre zwangsläufig nur ein weiterer `Q9_MOD_NATIVE`- oder
  kurzlebiger-WASM-Behelf (E9-Falle), kein echter langlebiger Prozess.

**Ziel-CPU: 68030** (Entscheidung E12, 2026-07-04) — MMU bleibt ungenutzt statt
abgeschaltet. Wichtige Nebenbedingung für später: unser 68k-Code (vbcc) sollte
sich auf einen Befehlssatz beschränken, den auch die reale CPU32-Zielhardware
kennt — der Emulator darf "voller" sein als das, was wir tatsächlich benutzen.

| # | Schritt | Status | Wer | Notizen |
|---|---------|--------|-----|---------|
| 5.2 | **Vorschlag (Andreas, 2026-07-04 abends):** CB030-Board emulieren als Bootstrap/Validierung statt gleich eigene QUICC-Peripherie. Die CB030 hat ein fertiges Boot-ROM mit Microware-OS-9-Modulen — damit kann Musashi an echtem, produktivem 68k-Code getestet werden statt nur an unseren handassemblierten Testprogrammen aus 5.1. Plan: zuerst mit den originalen Microware-Modulen starten (nur lokal, NICHT ins Repo — proprietär, gleiche Regel wie MWOS-SDK), dann nach und nach durch eigene Q9-Module ersetzen, sobald `Q9_MOD_M68K` + Syscall-Bridge stehen. Ändert nichts an der eigentlichen Zielhardware (MC68EN360/QUICC bleibt Ziel für Phase 7) — reiner Zwischenschritt zur Validierung/zum schnelleren Start. Lizenzlage der Microware-Module im Detail und wie weit "graduelle Ersetzung" technisch sauber geht (Modul für Modul austauschbar über F$Link, s. Abschnitt 5.4 HANDBUCH.md?) noch offen | 💡 | — | Aufgeworfen 2026-07-04 abends; PROJECT.md O4 verweist hierher. Speicherkarte + Peripherie-Register vollständig dokumentiert: [`docs/CB030.md`](docs/CB030.md). In Einzelgeräte aufgebrochen (5.2a–d, s.u.) — bleiben 💡 bis zur Phase-5-Detailplanung, da die Anbindung an Musashis Speicher-Hooks/Interrupt-Mechanismus noch nicht architektonisch entschieden ist |

**Einzelgeräte für 5.2** (Andreas, 2026-07-04 abends — Aufschlüsselung nach `docs/CB030.md`):

| # | Gerät | Status | Wer | Notizen |
|---|-------|--------|-----|---------|
| 5.2a | RAM/ROM/Remap-Speicherlogik | ✅ | Claudia | `src/kernel/cb030.c/.h` neu (native-only, wie m68krt.c): `q9_cb030_t` (rom/rom_len, ram/ram_len, remapped-Merker, unabhängig von Musashis… → Details: ARBEITSPLAN_ARCHIV.md |
| 5.2b | 68681-DUART (seriell) | ✅ | Claudia | Minimalansatz umgesetzt: `cb030.c` `cb030_uart_read/write` — nur SRA (`UART_BASE+0x02`, TxRDY=0x04 immer gesetzt, RxRDY=0x01 wenn… → Details: ARBEITSPLAN_ARCHIV.md |
| 5.2c | Compact-Flash-Interface | ✅ | Claudia | ATA-PIO-Minimalprotokoll umgesetzt: `cb030.c` `cb030_cf_read/write`, Register `CF_BASE+0` (Data, 1 Byte/Zugriff) / `+2` (Sectcount) / `+3..+5`… → Details: ARBEITSPLAN_ARCHIV.md |
| 5.2d | Timer/IRQ3 | ✅ | Claudia | Kooperative Umsetzung wie geplant: `q9_cb030_poll_timer(board, now_ms)` in `cb030.c` — `TI_IRQ_ON`/`TI_IRQ_OFF` (reine Adress-Trigger… → Details: ARBEITSPLAN_ARCHIV.md |
| 5.2e | HAL-Konsole: transparenter VT100-Durchreichbetrieb | 💤 | Claudia | Voraussetzung für den ersten Boot-ROM-Lauf mit Eingabe (nicht früher nötig, darum 💤). Der Host darf die serielle Leitung nicht verfälschen — Echo/CR-LF/Backspace regelt OS-9 selbst per `tmode`/`xmode` (SCF), der Host muss nur eine dumme Leitung sein. **POSIX (`hal_posix.c`): schon erledigt** — `q9_hal_init()` macht seit 1.10 termios raw (ICANON+ECHO aus, Wiederherstellung per atexit). **Offen ist nur Windows** (`hal_native.c`): `SetConsoleMode` mit `ENABLE_VIRTUAL_TERMINAL_INPUT` (stdin, sonst verschluckt die Konsole Pfeiltasten — von Andreas real beobachtet) + `ENABLE_VIRTUAL_TERMINAL_PROCESSING` (stdout), `ENABLE_LINE_INPUT`/`ENABLE_ECHO_INPUT` aus; Fehlschlag (Windows < 10) abfangen und ignorieren. Anschauungs-/Diagnosetool dazu existiert schon: `tools/vtmode.c` (zeigt empfangene Tastendrücke als Hex — Pfeiltaste muss als `ESC [ A` = `1B 5B 41` ankommen). Fürs echte CB030-Board ist NICHTS zu tun (PuTTY/TeraTerm bringen die VT100-Emulation mit) |
| 5.4 | **Erster echter OS-9-Boot** — Microware-ROM bootet bis zur interaktiven Shell 🎉 | ✅ | Claudia + Andreas | Das unveränderte `romimage.dev.running.BIN` (ROMBUG, 512K, lokal — nie im Repo) bootet in `q9.exe --cb030 <rom>` bis zum mshell-Prompt; `mdir` läuft… → Details: ARBEITSPLAN_ARCHIV.md |
| 5.5a | CF-Vorbereitung: Multi-Sektor-Transfers + `--cf <pfad>`-Option | ✅ | Claudia | Beide Teile umgesetzt: (1) **Multi-Sektor**: `cb030_cf_read/write` (cb030.c) zaehlen `cf_sectcnt` jetzt echt durch (neues Feld `cf_remaining` in… → Details: ARBEITSPLAN_ARCHIV.md |
| 5.5b | CF-Image bespielen — OS-9 mit echtem Dateisystem (und perspektivisch CF-Boot) | 💤 | Andreas | Wartet auf Andreas (2026-07-06): fertiges CF-Image (evtl. sogar mit C-Compiler!) einbauen — entweder nach `cb030_cf.img` kopieren oder nach 5.5a per `--cf <pfad>` direkt verwenden. Dann testen: liest OS-9 das Dateisystem (`dir /dd`, `pd`)? Fernziel: `os9gen` aufs CF → Boot von CF statt ROM |
| 5.6 | RTC-Emulation — busadressierte Uhr statt I2C | 💡 | Claudia + Andreas | Die echte CB030-RTC hängt an I2C (Bit-Banging zu emulieren wäre Fleißarbeit ohne Erkenntnisgewinn — Andreas' Entscheidung 2026-07-05: lieber anderen Chip). Plan: In MWOS schauen, für welche PARALLEL angebundenen RTC-Chips fertige OS-9-Treiber existieren (Kandidaten: Epson RTC-72423, MK48T02 "Timekeeper" — simple BCD-Register am Bus), Chip auf eine freie CB030-I/O-Adresse legen (z.B. `0xFFFF_D000`-Bereich, ist frei), in `cb030.c` die Register aus der Host-Uhr (`q9_hal_time`) als BCD servieren, im Bootfile `rtccb030` gegen den passenden Treiber tauschen (Andreas baut die ROM-Images selbst). Dann stimmen `date`/`setime` und der `Module Directory at 00:00:00`-Zeitstempel |
| 5.7 | **TX-Ringpuffer für die DUART-Konsole — Gegenstück zum RX-FIFO.** Der RX-FIFO (4 MByte, s. `cb030_uart_poll_rx`) ist bereits umgesetzt und verhindert Zeichenverlust auf der Empfangsseite. Die Ausgabeseite hat aber noch das alte Muster: `q9_hal_con_put()` (`hal_posix.c`) macht pro einzelnem von OS-9 echoten Zeichen ein blockierendes `fputc()`+`fflush(stdout)`. Hält der Leser auf der anderen Seite der Pty (Terminal.app/iTerm, oder `expect` in Testskripten) nicht mit, blockiert der naechste `write()`, bis der Leser wieder Platz macht — und weil CPU-Emulation und Host-I/O in `cb030run.c` in EINEM Thread laufen, friert in diesem Moment die GESAMTE Emulation ein (auch das RX-Polling, das ja im selben Loop haengt). Reproduziert mit `test_paste_burst.exp` (400-Zeilen-Paste): mit dem RX-FIFO allein kommt zwar kein Zeichen mehr dauerhaft abhanden, die Übertragung dauert aber unerwartet lange (>60s statt der rechnerisch erwarteten ~14s) und ein direkt danach gesendeter `list`-Befehl lief in `Read I/O error - Error #000:003` (E$Read, vermutlich Race: `list` traf auf noch aktives `build`). **Aufgabe:** `q9_hal_con_put()` darf den Haupt-Loop nie mehr blockieren koennen — analog zum RX-FIFO ein kleiner Software-TX-Ringpuffer in `hal_posix.c`, der pro Hauptschleifen-Durchlauf so viel wie moeglich nicht-blockierend per `write()` (STDOUT per `fcntl(..., O_NONBLOCK)`) rausschreibt; bei `EAGAIN`/`EWOULDBLOCK` bleibt der Rest im Puffer und wird beim naechsten Durchlauf weiterversucht. TxRDY/TxEMT in `cb030_uart_read`/`cb030.c` muessen dann ehrlich den Fuellstand dieses TX-Puffers widerspiegeln statt wie bisher immer "sofort bereit" zu melden — sonst glaubt OS-9, der Sender sei immer fertig, obwohl noch Daten warten. **Akzeptanzkriterien:** `test_paste_burst.exp` läuft ohne Race/Fehlermeldung durch, `list paste_burst.txt` zeigt den kompletten Originalinhalt; `Q9_CB030_DEBUG=1`-Heartbeat setzt zu keinem Zeitpunkt laenger als ~3-4s aus; Gesamtdauer der 400-Zeilen-Übertragung nahe an der theoretischen Sendezeit (~14s bei 3ms/Zeichen), kein unerklaerter Timeout mehr; `test_paste_paced.exp` (Kontrolltest) weiterhin fehlerfrei; `make test` PASS, `make native` warnungsfrei | ✅ | Claudia | Umgesetzt: TX-Ringpuffer (256 KiB) in `hal_posix.c`, `q9_hal_con_put()` schreibt nicht-blockierend per `write()` (STDOUT `O_NONBLOCK`), Rest bleibt im Puffer und wird bei jedem weiteren `con_put` UND per neuer `q9_hal_con_flush()` (aufgerufen aus `cb030run.c`- und `hal_posix.c`-Hauptschleife) nachgeliefert. Neue HAL-Funktionen `q9_hal_con_flush/tx_ready/tx_empty` (q9_hal.h, alle drei Targets implementiert — native/wasm nur trivial, da dort synchron). `cb030.c` SRA liefert TxRDY/TxEMT jetzt ehrlich aus dem Pufferzustand statt immer "bereit", ebenso die TxRDYA-IRQ-Bedingung. Verifiziert: `test_paste_burst.exp` UND `test_paste_paced.exp` laufen beide sauber durch, kein Freeze, `list paste_burst.txt` zeigt alle 400 Zeilen vollstaendig und unveraendert. Das abschliessende "Read I/O error - Error #000:003" nach `list` tritt IDENTISCH auch im Paced-Kontrolltest auf (also vor UND unabhaengig vom TX-Puffer) — vorbestehendes, von 5.7 unabhaengiges OS-9/`list`-Verhalten, kein Regressionseffekt dieses Schritts; nicht weiter verfolgt (Timebox), bei Bedarf eigener Folge-Schritt. `make test` PASS, `make native` warnungsfrei |
| 5.8 | **Ctrl-]-Host-Escape + DEL→BS-Mapping in `q9_hal_con_get()` verifizieren.** Aus einem frueheren, nicht committeten Arbeitsstand liegt noch ein kleiner Patch fuer `hal_posix.c` vor (Ctrl-], 0x1D, beendet den Emulator sauber; DEL/0x7F wird als BS/0x08 an OS-9 durchgereicht, macOS-Backspace-Taste). Beide Aenderungen sind unabhaengig vom RX-FIFO (5.7 sequenziert) und vom sonstigen Terminal-Raw-Mode (der ist bereits ueber Andreas' Commit `3fd6f15` in `origin/main`). **Aufgabe:** Beide Verhaltensweisen gezielt verifizieren, BEVOR committet wird (bisher kein Testskript/Log dazu vorhanden) — (1) Ctrl-] im laufenden Emulator senden, erwartet: sofortiges sauberes Beenden mit der Meldung "Host-Escape Ctrl-] — Emulator beendet."; (2) DEL-Byte (0x7F) an die Konsole senden, erwartet: OS-9 empfaengt/verarbeitet es wie ein echtes Backspace (0x08), sichtbar z.B. beim Editieren einer Kommandozeile. Danach committen (oder verwerfen, falls sich Nebenwirkungen zeigen) | ✅ | Claudia | Patch aus dem Stash uebernommen (nur die `q9_hal_con_get()`-Ergaenzung — die Raw-Mode-Anteile des Stashes waren bereits ueber Andreas' Commit `3fd6f15` im Hauptbaum). Beide Verhaltensweisen gezielt mit eigenen expect-Testskripten verifiziert: (1) Ctrl-] (0x1D) an den laufenden Emulator gesendet → sofortiges sauberes Beenden mit "Host-Escape Ctrl-] — Emulator beendet.", Termios wird ueber den bestehenden `atexit`-Handler korrekt wiederhergestellt. (2) Eingabe "abc" + DEL (0x7F) + Enter → Kernel echot sichtbar Backspace-Space-Backspace und liefert "echo: ab" zurueck (das "c" wurde durch OS-9s Zeileneditor entfernt) — DEL kommt also als echtes BS (0x08) an, nicht als rohes 0x7F. `make test` PASS, `make native` warnungsfrei |
| 5.9 | **Idle-Drossel für den CB030-Runner** — Host-CPU im OS-9-Leerlauf von 100 % auf wenige Prozent senken. **Befund (Claudia, 2026-07-09, gemessen mit `Q9_CB030_DEBUG=1` am Login-Prompt):** OS-9 idlet per `STOP #$3000` (PC konstant `0xA2E8`, SR `0x3000`, nur die 100-Hz-Timer-IACKs laufen weiter); die 100 % entstehen allein, weil die Schleife in `cb030run.c` nach `q9_m68krt_execute()` sofort die nächste Runde dreht und Musashi eine gestoppte CPU die angeforderten Takte nur "verbrennen" lässt. **Aufgabe in drei Teilen:** (1) Musashi-Vendor-Patch: schmalen Accessor `m68k_is_stopped(void)` ergänzen (gibt zurück, ob das interne `CPU_STOPPED`-Flag gesetzt ist), Änderung in `third_party/musashi/Q9_VENDOR.md` dokumentieren. (2) Neue HAL-Funktion `q9_hal_sleep_ms(ms)` in `q9_hal.h` + beide HALs (POSIX: `usleep`/`nanosleep`; Windows: `Sleep`) — bisher gibt es nur ein internes `usleep(1000)` in `hal_posix.c`, keine öffentliche API. (3) Hauptschleife `cb030run.c`: wenn CPU gestoppt UND kein IRQ anliegt → `q9_hal_sleep_ms(1)` statt sofort weiterzudrehen; der Timer-Poll rechnet mit `q9_hal_ticks_ms()` (Wanduhr), die OS-9-Uhr geht also nicht falsch — der nächste 10-ms-Tick setzt IRQ3 und weckt die STOP-CPU. **Akzeptanzkriterien:** Idle am Login-Prompt < 5 % Host-CPU (`ps`-Stichprobe); Boot bis Login, Login als `super`, `dir /dd` und ein kompletter `cc hello.c`-Durchlauf funktionieren unverändert; `make test` PASS, `make native` warnungsfrei. **Testweg:** Clone-Image `local_images/OS9SYS.claudia-test.hda` + expect-Automatisierung — Muster (CR-Anklopfen nach "devices online", gebremstes Senden ~50 ms/Zeichen), Login `super`/`Al35uUbC` (s. `/dd/SYS/password` im Image, wie in `test_paste_burst.exp` verwendet). Hinweis: Andreas' dauerhaft laufender Emulator profitiert erst nach Neustart mit dem neuen Binary | 🟢 | Claudia | Idee Andreas 2026-07-09 ("kann man erkennen, wenn das OS in der Idle-Schleife hängt, und dann im Host-OS ein give-up-timeslice machen?") — ja, per STOP-Flag, s. Befund. Bewusst NICHT Teil dieses Schritts: Echtzeit-Drossel auf ~25 MHz (nur nötig, falls ein OS-9-Userprozess selbst busy-waitet — bei Bedarf Ideenspeicher) |
| 5.10 | **OS-9-SCF-Treiber für den Q9-Netzwerk-Terminal-Server (`/t1`–`/t4` über Telnet, Port 2000).** Der Host-Teil ist fertig und getestet (`m68krt.c`: `init_network_terminals`/`update_network_terminals`, 4 Kanäle, Verbindungsabbruch wird sauber erkannt) — Telnet verbindet sich bereits erfolgreich auf Port 2000, aber es passiert nichts, weil OS-9 die simulierte Hardware noch nicht kennt: kein Treiber, kein Descriptor, kein `/t1`. **Dieser Schritt gehört ins MWOS-SDK** (`/Volumes/SSD1TB/projects/MWOS`), NICHT in den Q9-Kernel — Q9 emuliert nur die Register, OS-9 braucht dafür einen ganz normalen Gerätetreiber wie für echte Hardware. **Eigener Port `Q9` (2026-07-10 angelegt):** Statt den bestehenden `CB030`-Port (Ziel: echte Hardware) mit emulator-spezifischen Erweiterungen zu verwässern, gibt es jetzt `OS9/68030/PORTS/Q9/` als 1:1-Kopie von `OS9/68030/PORTS/CB030/` (Andreas' Entscheidung 2026-07-10) — ALLE Änderungen aus diesem Schritt (Descriptoren, Treiber, Bootfile-Mergeliste) gehören in den `Q9`-Port, `CB030` bleibt unangetastet. **Vor der eigentlichen Treiber-Arbeit einmal zur Kontrolle:** den frisch kopierten `Q9`-Port unverändert bauen (Build-Weg (a) oder (b), s. unten) und verifizieren, dass das Ergebnis sich identisch zum bisherigen `CB030`-Build verhält (bootet im Q9-Emulator genauso durch bis zur Shell) — erst danach den Netzwerk-Treiber ergänzen. **Register-Layout** (Quelle der Wahrheit: `Q9/src/kernel/cb030.h`, Konstanten `Q9_CB030_NET_T1_BASE`…`_T4_BASE`, `Q9/src/kernel/m68krt.c` Funktionen `network_read8`/`network_write8`/`update_network_terminals`) — je Kanal 3 Byte-Register ab seiner Basisadresse `B`: `B+0` Status lesen (Bit0=RX Ready, Bit1=TX Empty, praktisch immer 1 — kein Backpressure-Modell), `B+2` RX-Data lesen (liest das empfangene Byte UND löscht Bit0 im selben Zugriff, wie bei einer einfachen UART ohne FIFO — ein zweites Byte, das ankommt bevor dieses abgeholt wurde, wird auf Host-Seite verworfen, s. `update_network_terminals`), `B+4` TX-Data schreiben (Byte geht synchron in den Telnet-Socket, Treiber muss NICHT auf ein Busy-Bit warten). **Vier Kanal-Adressen (fest, aus `cb030.h`):** `/t1` Port `$FFFF1010`, `/t2` Port `$FFFF1020`, `/t3` Port `$FFFF1030`, `/t4` Port `$FFFF1040` — alle IRQ-Level 4, Autovektoren `/t1`=70=`$46`, `/t2`=71=`$47`, `/t3`=72=`$48`, `/t4`=73=`$49` (jeder Kanal hat seinen EIGENEN Vektor, der Emulator liefert im IACK-Zyklus genau den Vektor des Kanals mit gesetztem RX-Ready-Bit, s. `m68krt_board_int_ack` — ein Treiber-Handler pro Vektor muss also nur seinen eigenen Kanal prüfen, kein Scannen aller vier nötig). **Aufgabe in drei Teilen:** (1) **Vier Descriptor-Module** `t1`/`t2`/`t3`/`t4` (Typ `Devic`) im `Q9`-Port anlegen, unter `OS9/68030/PORTS/Q9/SCF/netterm/` — Vorlage ist das Makro `SCFDesc` aus `OS9/SRC/IO/SCF/DESC/scfdesc.a` (bereits im SDK vorhanden, NICHT neu schreiben): erstes Feld im generierten Descriptor ist exakt `dc.l Port` — das ist die 4-Byte-Speicheradresse, die hier gebraucht wird. Aufruf pro Datei z.B. `SCFDesc $FFFF1010,$46,4,0,NoParity,NoBaud,nettty` (Port, Vektor, IRQ-Level, Priorität, Parity/Baud sind bei dieser Hardware bedeutungslos — auf neutrale Werte setzen, der eingebaute Filemanager `Scf` wird unverändert mitbenutzt, KEIN eigener Filemanager nötig). Vorlage für den Aufbau einer Descriptor-Quelldatei mit patchbaren Feldern: `OS9/68030/PORTS/common/RBF/cfide/c_tmpl.a` (dort `Port equ CF_Base` — exakt dasselbe Prinzip). (2) **Ein neuer Treiber-Modul** (Typ `Drivr`), eigener Name z.B. `nettty` — strukturell orientiert an `OS9/SRC/IO/SCF/DRVR/sc68681.a` (Init/Read/Write/GetStat/SetStat/Interrupt-Handler-Muster), aber NICHT von dort kopieren (Microware-Copyright, Kopfzeile "proprietary confidential" — genau wie schon bei FAT16 entschieden: als Vorlage lesen, eigene Implementierung schreiben) und drastisch einfacher, weil keine Baudrate/Parität/Handshake-Hardware existiert: **Init** liest `Port` aus dem Descriptor (Offset 0), trägt den Interrupt-Handler per `F$IRQ` auf `IRQLevel`+`Vector` aus dem Descriptor ein. **Read** prüft Status-Bit0 (`Port+0`); gesetzt → Byte von `Port+2` lesen und zurückgeben; sonst Prozess schlafen legen bis der Interrupt-Handler ihn aufweckt (Standard-SCF-Blockiermuster, kein Ringpuffer nötig — Q9-Seite puffert ebenfalls nur 1 Byte). **Write** schreibt das Byte nach `Port+4`, kein Warten auf ein Busy-Bit nötig. **GetStat** `SS.Ready` aus Status-Bit0 ableiten, alles andere darf `E$UnkSvc` liefern. **Interrupt-Handler**: Byte von `Port+2` holen, wartenden Prozess aufwecken — dank Pro-Kanal-Vektor ohne Status-Scan über alle vier Kanäle. (3) Beide Deskriptoren + Treiber in die Bootfile-Mergeliste des `Q9`-Ports aufnehmen (`OS9/68030/PORTS/Q9/CMDS/BOOTOBJS/BOOTFILES/`, wie bei den bestehenden Geräten `/term`/`/t1` — Namenskollision mit dem kopierten alten `t1.r` beachten, ggf. umbenennen — `CB030`s eigene Bootfile-Mergeliste bleibt unangetastet). **Zwei grundsätzlich verschiedene Build-Wege stehen zur Verfügung:** (a) **Cross-Build am Mac über Wine** — die eigene `build_os9.bat` (s. Memory "MWOS Build-System" — `M:\build_os9.bat`, `os9make -e MWOS=M: GOAL=build`) treibt `os9make.exe`/`r68.exe`/`l68.exe` (alles PE32-Windows-Binaries, es gibt im SDK keinen wine-freien Cross-Compiler) über Wine an, bekannt funktionierend, baut aber den GANZEN CB030-Port neu. (b) **Selbstgehosteter nativer Build INNERHALB des laufenden OS-9** (Andreas' bevorzugter Weg, 2026-07-10): OS-9 im Q9-Emulator bringt seinen eigenen C-Compiler/Assembler/Linker mit und baut sich damit selbst (funktioniert bereits nachweislich, s. Akzeptanzkriterium in 5.9: "ein kompletter `cc hello.c`-Durchlauf"). Dafür müssen die neuen Quelldateien (4× Descriptor-`.a`, 1× Treiber-`.a`) erst in die laufende OS-9-Instanz hinein transferiert werden — Weg dafür existiert schon (`build`/Paste-Mechanismus über die DUART-Konsole, s. `test_paste_burst.exp`/`test_paste_paced.exp`, oder Kermit s. Ideenspeicher-Eintrag "Kermit als robuster Dateitransfer-Weg") — dort dann mit OS-9-Bordmitteln assemblieren (`asm`) und linken/mergen (`os9merge`), kein Wine/Mac-Werkzeug beteiligt. Sauberer Weg, um ihn zu testen: `local_images/OS9SYS.claudia-test.hda` klonen (wie schon in 5.9 verwendet), damit das produktive Image unangetastet bleibt. **Bekannte Emulator-Einschränkung, die dabei auffallen wird (separat klären, ggf. eigener Folge-Schritt):** `m68krt_board_int_ack` löscht die IRQ-Leitung beim Quittieren GLOBAL (`m68k_set_irq(0)`), unabhängig davon, ob noch ein ANDERER Netzwerk-Kanal ein unabgeholtes Byte hat — bei zwei gleichzeitig aktiven Telnet-Verbindungen kann ein Kanal seinen Interrupt verlieren und bekommt erst beim nächsten Byte wieder einen. Vor produktivem 4-Kanal-Betrieb gegentesten, ggf. Nachbesserung im Emulator (pro Kanal erneut prüfen und IRQ ggf. sofort wieder anheben, statt pauschal auf 0 zu setzen). **Akzeptanzkriterien:** `telnet localhost 2000` verbindet sich, OS-9-Login-Prompt erscheint auf `/t1` (Login-Prozess muss auf `/t1` gestartet werden, analog zu `/term` — Startup-Datei/`login`-Mechanismus prüfen); Zeicheneingabe und Echo funktionieren bidirektional; zweite gleichzeitige Telnet-Verbindung bekommt `/t2` zugewiesen (Host-Meldung "Gast dynamisch an /t2 uebergeben" in der Q9-Konsole); Verbindung trennen (Ctrl-], `exit`) gibt den Kanal sauber frei, ein erneuter Connect landet wieder auf `/t1`; `make test` in Q9 bleibt unverändert PASS (dieser Schritt ändert nichts am Q9-Kernel, nur am MWOS-SDK) | 💡 | — | Vorschlag von Andreas 2026-07-10, im Anschluss an den CLOSE_WAIT-Bugfix (s. `Q9_CURRENT_STATUS.md`/Commit-Historie 2026-07-10). Quelltext-Autoring (Descriptoren + Treiber) braucht MWOS-SDK-Zugriff und OS-9-Kenntnis der bestehenden Konventionen (scfdesc.a-Makro, sc68681.a als Referenz) — das kann Claudia mit MWOS-Zugriff übernehmen. Der eigentliche Build läuft aber laut Andreas bevorzugt Weg (b) SELBSTGEHOSTET im laufenden Q9-Emulator (Datei-Transfer + `asm`/`os9merge` von Hand in der Konsole) — das liegt technisch INNERHALB des Q9-Repo-Scopes und könnte die stündliche Q9-Arbeitsplan-Routine perspektivisch mit übernehmen, ist aber deutlich interaktiver als bisherige Ready-Schritte (Datei-Transfer in eine laufende Session); "Wer" bewusst offen gelassen, bis Andreas entscheidet, ob Claudia das end-to-end probieren soll oder er den Build-Teil selbst macht |
| 5.3 | Musashi ↔ CB030 verdrahten + Boot-ROM laden + Boot-Runner | ✅ | Claudia | Damit ist der Weg frei für den ersten Boot-Versuch mit dem echten Microware-ROM. → Details: ARBEITSPLAN_ARCHIV.md |
| 5.1 | Musashi als CPU-Kern einbinden — Grundbaustein + Rauchtest, KEINE Scheduler-/Syscall-Bridge-Entscheidungen (die kommen erst mit der Detailplanung). Analog zu 4.6 (wasm3): Makefile-Integration (Musashis Zweistufen-Build — `m68kmake` generiert `m68kops.c/.h` aus `m68k_in.c` zur Bauzeit, siehe `third_party/musashi/Q9_VENDOR.md`), schmaler Wrapper `src/kernel/m68krt.c/.h` (analog `wasmrt.c/.h`), CPU-Typ `M68K_CPU_TYPE_68030`. Rauchtest: ein von Hand geschriebenes/assembliertes 68k-Testprogramm (z.B. zwei Zahlen addieren) in emuliertes RAM legen, `m68k_pulse_reset()` + `m68k_execute()` aufrufen, Ergebnis über die emulierten Register prüfen | ✅ | Claudia | Makefile-Integration steht: `m68kmake` wird als Host-Tool gebaut, generiert `m68kops.c/.h` zur Bauzeit nach `build/native/musashi_gen/` (nicht… → Details: ARBEITSPLAN_ARCHIV.md |

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
| U.1 | `libq9`: Wrapper um die fertigen Syscalls + `q9cat`/`q9copy` + nativer Testharness | ✅ | Codex | Testharness bootet den echten Kernel nativ (POSIX-HAL) gegen ein selbst gebautes FAT16-Image, statt zu mocken. → Details: ARBEITSPLAN_ARCHIV.md |
| U.2 | `q9dir`: Verzeichnis auflisten | ✅ | Codex | Liest rohe 32-Byte-FAT16-Directory-Einträge über `I$Read` auf einen Verzeichnispfad (Kernel-Unterstützung existierte schon seit 3.3); Parsing in… → Details: ARBEITSPLAN_ARCHIV.md |
| U.3 | `q9mkdir` + `q9rm` | ✅ | Codex | Rundet das ursprünglich gewünschte "dir/list/copy"-Set ab; kein rekursives Löschen. → Details: ARBEITSPLAN_ARCHIV.md |
| U.4 | `q9touch` + `q9stat` + Testumgebung konsolidiert | ✅ | Codex | Sieben Tools insgesamt (cat/copy/dir/mkdir/rm/touch/stat); README vollständig; Test-Redundanzen aufgeräumt. → Details: ARBEITSPLAN_ARCHIV.md |
| U.5 | Funktions-Kommentare nachtragen (Andreas' Review: Code gut, aber unkommentiert) | ✅ | Codex | 88 Funktionen in 16 Dateien im Q9-Standardstil (Function/Desc/Call-Header) kommentiert; reine Ergänzung, +520/-0 Zeilen, kein Verhalten geändert (per… → Details: ARBEITSPLAN_ARCHIV.md |
| U.6 | argc/argv-`main()`-Einsprungpunkte für alle sieben Tools (Vorbereitung auf spätere 68k-Kompilierung) | ✅ | Codex | Eigener Branch `codex-m68k-entrypoints`, `q9<name>_main(argc, argv)` neben unveränderten `_run()`-Funktionen, `-h`/`--help` überall, `-v`/`--verbose`… → Details: ARBEITSPLAN_ARCHIV.md |
| U.7 (Idee) | Lange Dateinamen (LFN) auch in `q9dir`/`q9stat` anzeigen | 💭 | — | LFN-Parse-Logik steckt privat in `fat16.c` — Duplizieren oder Kernel müsste sie exportieren |
| U.8 (Idee) | Tools als echte ladbare Q9-Module, sobald Phase 5/6 (Modul-Ausführung) und O6 (Verpackungsformat) stehen | 💭 | — | heute nur Design-Vorgriff |
| U.10 | Bestehende Userland-Tools auf echtem OS-9 testen (im CB030-Emulator) | 💡 | Codex + Andreas | Andreas' Idee (2026-07-05): Solange Q9s eigene 68k-Runtime noch nicht steht, das emulierte echte OS-9 als Prüfstand für die U.1–U.6-Tools nutzen — validiert die Tool-Logik auf einem echten OS und beweist nebenbei, dass Q9s API-Design OS-9-artig genug ist. Drei Voraussetzungen: (1) **Compiler-Pfad** — MWOS-Cross-Compiler (Windows) oder besser der native OS-9-C-Compiler auf dem CF-Image (→ nach 5.5b). (2) **Portabilitätsschicht** — dünnes OS-9-Backend für die Tools gegen die OS-9-C-Library (Datei-I/O fast Unix-artig; Verzeichnis-Lesen + Fehlercodes anders) — gute Codex-Aufgabe, klar umrissen. (3) **Transportweg** für die Binaries: ins CF-Image einbauen ODER per `kermit` über die emulierte serielle Leitung (steckt im Bootfile — wäre gleich ein Härtetest für die DUART-Emulation). Einschränkung bewusst akzeptiert: OS-9-spezifischer Portierungsanteil wandert nicht 1:1 zu Q9 mit. Wird 🟢, sobald 5.5b (CF-Image) steht und der Compiler-Weg geklärt ist |
| U.9 | OS-9-Klassiker-Tools nachbauen: `q9mdir`, `q9mfree`, `q9procs`, `q9free`, `q9devs`, `q9ident` | 💡 | Codex | Andreas' Idee (2026-07-05, nach dem OS-9-Boot-Meilenstein): die vertrauten OS-9-Systemkommandos als Q9-Userland-Tools, gegen Q9s Syscall-API — `q9mdir` (Modul-Directory listen, kurz + `-e` erweitert wie das Original), `q9mfree` (freier Speicher), `q9procs` (Prozesstabelle), `q9free` (Block-Device-Belegung), `q9devs` (Gerätetabelle), `q9ident` (Modul-Header/CRC einer Datei anzeigen). Gleiches Muster wie U.1–U.6: `q9<name>_main(argc, argv)`-Einsprungpunkte, Testharness, isolierter Branch. VORAUSSETZUNG klären, bevor es 🟢 wird: welche der nötigen Kernel-Auskunfts-Syscalls (Modul-Directory iterieren, Prozesstabelle lesen, Freispeicher abfragen) existieren schon bzw. müssen vorher von Claudia ergänzt werden — sonst baut Codex gegen Luft |

**Stand 2026-07-04**: U.1–U.5 fertig, jede Runde von Claudia unabhängig
nachgebaut (`userland/build.sh`, PASS, `-Wall -Wextra` warnungsfrei), Branch
`codex-userland` gepusht (Commits `bf5d494`, `a94ca93`). Nebenbefund bei der
Kommentar-Durchsicht: auch der Hauptkernel wurde stichprobenartig geprüft
(fast lückenlos kommentiert, eine echte Lücke `is_leap()` in `syscall.c`
gefunden und behoben, Commit `1caf2b3`). **Nach Andreas' Durchsicht per Pull
Request #1 in `main` gemerged (2026-07-04 abends, Merge-Commit `ebdde4b`)** —
Worktree und Branch aufgeräumt, `userland/` ist damit fester Bestandteil des
Hauptbaums. Anbindung an O6 (Modul-Verpackung) bleibt offen für später.

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
| **Kermit als robuster Dateitransfer-Weg** statt/ergänzend zum rohen Paste in `build`: `CMDS/kermit` liegt schon im Image, sc68681-Emulation existiert. Kermit überträgt in kleinen Paketen (~96 Byte) mit ACK je Paket und Prüfsumme — die Bremse steckt schon im Protokoll (Sender wartet auf Antwort, bevor er weiterschickt), kein künstliches Pacing nötig, und da Kermit binär ohne Zeichen-Echo läuft, umgeht es auch den 5.7-Bug (blockierendes THRA-Echo) weitgehend. Bei Übertragungsfehlern automatischer Retransmit statt stillem Datenverlust wie beim rohen Paste. **Ausdrücklich unabhängig von 5.7** — 5.7 (blockierendes `write()` beim Konsolen-Echo) ist die eigentliche Fehlerursache und muss so oder so behoben werden, egal ob Kermit je zum Einsatz kommt | Andreas 2026-07-09, im Nachgang zur Paste-Korruptions-Analyse diskutiert | Noch kein 💡-Vorschlag, weil U.10 (Voraussetzung: Compiler-Pfad + Portabilitätsschicht) Kermit als Transportweg für die Userland-Tools ohnehin schon nennt — erst dort einordnen, wenn der Bedarf konkret wird |

---

## ⛔ Geparkt / mit Andreas zu besprechen

- **emsdk fehlt auf dem Desktop AF-PC** (noch unverändert): wasm-Build/Browser-Test dort
  weiterhin nicht möglich. Auf dem Mac Mini seit 2026-07-04 behoben (siehe unten) — betrifft
  jetzt nur noch den Desktop-Rechner, nicht mehr den Autonomie-Betrieb.
- **4.7 Browser-Seite (WebAssembly.instantiate im Worker) bewusst nicht umgesetzt** (2026-07-04):
  ARBEITSPLAN nennt "bzw. WebAssembly.instantiate im Browser" als Alternative zur nativen
  wasm3-Runtime. Umgesetzt wurde nur die native Seite (wasmproc.c) — ein Q9_MOD_WASM-Gastprogramm
  aus dem Worker heraus zu instanziieren (verschachteltes WASM: der Q9-Kernel selbst laeuft im
  Worker schon als WASM, ein Gastmodul muesste per JS `WebAssembly.instantiate` zur Laufzeit
  dazugeladen und dessen Importe per `Module.ccall`-Bruecke zurueck in den Kernel verdrahtet
  werden) ist eine eigene, nicht triviale Design-/Implementierungsaufgabe (worker.js-Aenderungen,
  JS<->WASM-Grenze). Mit Andreas zu besprechen, ob/wann das eigenstaendig eingeplant wird — bis
  dahin liefert `F$Fork`/`F$Chain` auf `Q9_MOD_WASM` im wasm-Build (ohne `-DQ9_HAVE_WASM3`)
  weiterhin `E$NEMod`, exakt wie vor 4.7.

---

## Erledigt

Vollständiger Session-Verlauf (alle abgeschlossenen Schritte im Detail, chronologischer Arbeitslog): siehe `ARBEITSPLAN_ARCHIV.md`.
