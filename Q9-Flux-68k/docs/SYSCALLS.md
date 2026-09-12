# Q9 Syscall-ABI

**Entscheidung E7 (2026-07-03)**: Q9 übernimmt die **OS-9-Funktionsnummern und
Registerkonventionen**. Quelle der Nummern: MWOS `DEFS/funcs.h` (OS-9 Professional V3.0),
Fehlercodes aus MWOS `DEFS/errno.h`. Ziel: OS-9-Software (68k wie 6809) lässt sich
mechanisch auf Q9 abbilden — die 68k-/6809-Runtimes werden dünne Register-Mapper.

---

## Der virtuelle Registersatz

Alle Syscalls übergeben Parameter in einem virtuellen Registersatz, der dem
**68k-Layout** entspricht (das ist die kanonische Form):

```c
typedef struct q9_regs {
    uint32_t d[8];      /* d0..d7 — Datenregister                     */
    void    *a[8];      /* a0..a7 — Adressregister (Kernel-Sicht)     */
} q9_regs_t;

int q9_syscall(uint16_t func, q9_regs_t *regs);
```

- **Rückgabewert**: `0` = Erfolg, sonst OS-9-Fehlercode.
  (Entspricht OS-9: Carry clear/set + Fehlercode in d1.w bzw. 6809-B.)
- **Breiten-Konvention wie OS-9**: `d0.w` heißt "nur unteres Wort relevant" —
  dokumentiert pro Call, technisch liegt alles in `uint32_t`.

### Runtime-Mapping

| Q9 virtuell | 68k-Runtime (Musashi) | 6809-Runtime (Vision, Richtlinie*) |
|-------------|-----------------------|-------------------------------------|
| d0          | d0                    | A bzw. D (Byte-/Wort-Parameter 1)   |
| d1          | d1                    | Y (Zähler/Größen) bzw. B (Fehler)   |
| d2          | d2                    | B (zweiter Byte-Parameter)          |
| a0          | a0                    | X (primärer Zeiger)                 |
| a1          | a1                    | U (sekundärer Zeiger)               |

*Richtlinie abgeleitet aus den dokumentierten Paaren (I$Read: A/Y/X ↔ d0/d1/a0;
F$Fork: A/X/U/Y ↔ d0/a0/a1/d1). **Verbindlich ist immer die Tabelle pro Call.**

---

## Funktionsnummern (Auszug; identisch zu MWOS funcs.h)

| Nummer | Name     | Status Phase 1 |
|--------|----------|----------------|
| $00 | F$Link   | ✅ implementiert (Phase 2.3d: sucht Modul-Directory nach Name+Type+Language) |
| $01 | F$Load   | ✅ implementiert (Phase 3.5: Modul aus Datei, siehe unten) |
| $02 | F$UnLink | ✅ implementiert (Phase 2.3d; seit 3.5 gibt sie F$Load-Puffer bei Link-Count 0 frei) |
| $03 | F$Fork   | ✅ implementiert (Phase 4.2: startet ein Modul aus dem Modul-Directory als neuen Prozess, s.u.) |
| $04 | F$Wait   | ✅ implementiert (Phase 4.2: sammelt einen beendeten Kind-Prozess ein, s.u.) |
| $05 | F$Chain  | ✅ implementiert (Phase 4.2: ersetzt das eigene Modul, PID/Parent/Std-Pfade bleiben, s.u.) |
| $06 | F$Exit   | ✅ implementiert (Phase 4.2: echte Semantik — Zombie/Reap oder sofortiges Freigeben, s.u.) |
| $0C | F$ID     | ✅ implementiert (liefert die echte PID aus der Prozesstabelle, s.u.) |
| $10 | F$PrsNam | ✅ implementiert (Phase 1.5) |
| $11 | F$CmpNam | ✅ implementiert (Phase 1.5) |
| $15 | F$Time   | ✅ implementiert (Phase 1.9: echte Uhrzeit via HAL) |
| $16 | F$STime  | ✅ implementiert (Phase 1.9) |
| $80 | I$Attach | ✅ implementiert (Phase 1.6) |
| $81 | I$Detach | ✅ implementiert (Phase 1.6) |
| $89 | I$Read   | ✅ implementiert (seit 3.3: routet über `fm->read()`, wenn ein File-Manager hinter dem Pfad hängt) |
| $8A | I$Write  | ✅ implementiert (seit 3.4: routet über `fm->write()`, wenn ein File-Manager hinter dem Pfad hängt) |
| $8B | I$ReadLn | ✅ implementiert |
| $8C | I$WritLn | ✅ implementiert |
| $82 | I$Dup    | ✅ implementiert (Phase 1.4) |
| $84 | I$Open   | ✅ implementiert (Phase 3.2: VFS-Pfad-Routing; seit 3.3 FAT16 an /d0) |
| $83 | I$Create | ✅ implementiert (Phase 3.4: FAT16 — legt neuen 8.3-Dirent an, `E$UnkSvc` ohne File-Manager) |
| $85 | I$MakDir | ✅ implementiert (Phase 3.4: FAT16 — neuer Cluster mit `.`/`..`, `E$UnkSvc` ohne File-Manager) |
| $86 | I$ChgDir | ✅ implementiert (Phase 3.2: globales Arbeitsverzeichnis) |
| $87 | I$Delete | ✅ implementiert (Phase 3.4: FAT16 — Cluster-Kette freigeben + Dirent als gelöscht markieren) |
| $88 | I$Seek   | ✅ implementiert (Phase 3.3: über `fm->seek()`, sonst `E$UnkSvc`) |
| $8D | I$GetStt | ✅ Grundgerüst (Phase 1.8: SS.Ready, SS.EOF; 3.1: SS.BlkRd auf /d0) |
| $8E | I$SetStt | ✅ Grundgerüst (Phase 3.1: SS.BlkWr auf /d0) |
| $8F | I$Close  | ✅ implementiert (Phase 1.4) |

Alle nicht implementierten Nummern liefern `E$UnkSvc` ($D0).

## Fehlercodes (Auszug; identisch zu MWOS errno.h)

| Code | Name      | Bedeutung |
|------|-----------|-----------|
| $C8  | E$PthFul  | Pfadtabelle voll |
| $C9  | E$BPNum   | ungültige Pfadnummer |
| $CB  | E$BMode   | falscher Zugriffsmodus |
| $CD  | E$BMID    | ungültiger Modul-Header (2.3b: HeaderSize/ModuleSize/NameOffset) |
| $CE  | E$DirFul  | Modul-Directory voll (2.3c) |
| $D0  | E$UnkSvc  | unbekannter Service-Request |
| $D1  | E$ModBsy  | Modul noch gelinkt (reserviert, MWOS-verifiziert) |
| $D2  | E$BPAddr  | ungültige Parameter-Adresse |
| $D3  | E$EOF     | Dateiende |
| $E1  | E$Param   | ungültiger Parameter |
| $E8  | E$BMCRC   | Modul-CRC stimmt nicht (2.3b) |
| $EC  | E$BMHP    | Modul-Header strukturell defekt (2.3b; Q9 macht KEINE separate Parity-Vorabprüfung wie OS-9, der Code wird für alle Strukturchecks vor der CRC verwendet) |
| $F6  | E$NotRdy  | Gerät nicht bereit |
| $D8  | E$PNNF    | Pfad nicht gefunden (3.2: VFS-Routing — Rest-Pfad auf Gerät ohne File-Manager, unbekannter Pfad im File-Manager, I$ChgDir zu lang/leer) |

---

## Implementierte Calls (Phase 1)

### I$Read ($89) / I$ReadLn ($8B)

| Register | Input                    | Output               |
|----------|--------------------------|----------------------|
| d0.w     | Pfadnummer               | —                    |
| d1.l     | max. Byte-Anzahl         | tatsächliche Anzahl  |
| a0       | Pufferadresse            | —                    |

- I$ReadLn liest zeilenweise **inklusive CR**, macht Echo und Zeilen-Editierung
  (Backspace) — SCF-Verhalten. I$Read liest rohe Zeichen ohne Echo.
- Zeilenende ist CR ($0D) wie bei OS-9; LF von Host-Terminals wird als CR akzeptiert.
- **Seit Phase 3.3**: hat das Gerät hinter dem Pfad einen File-Manager MIT `read`-Op
  (`q9_dev_t.fm`, z.B. FAT16 an `/d0`), geht I$Read an `fm->read()` statt an die Treiber-Op —
  liest aus der über I$Open an `q9_path_t.fmctx` gebundenen Datei, folgt dabei selbstständig
  der Cluster-Kette. `E$EOF`, wenn die Position bereits am Dateiende steht. I$ReadLn bleibt
  auf dem Treiber-Pfad (Zeilenmodus mit Echo/Editierung ergibt für Dateien keinen Sinn).
- **Seit Phase 4.3**: liefert der Treiber-Pfad (kein File-Manager — dort ist `E$NotRdy` ein
  echter I/O-Fehler, kein Weckgrund) `E$NotRdy`, versetzt `syscall.c` den aufrufenden Prozess
  per `q9_proc_wait_device` in den Zustand `Q9_PS_WAITING` (Weckgrund `Q9_WAIT_DEVICE`) — der
  Rückgabewert an den Aufrufer bleibt `E$NotRdy` (kein eingefrorener Stack, Entscheidung E8),
  aber der Scheduler steppt den Prozess erst wieder, wenn `SS.Ready` des Geräts anspricht
  (proc.c: `dev_ready()`). Damit fühlt sich I$Read/I$ReadLn für den aufrufenden Prozess
  blockierend an, ohne dass der Scheduler ihn bis dahin sinnlos weiter pollt.

### I$Write ($8A) / I$WritLn ($8C)

| Register | Input                    | Output               |
|----------|--------------------------|----------------------|
| d0.w     | Pfadnummer               | —                    |
| d1.l     | max. Byte-Anzahl         | tatsächliche Anzahl  |
| a0       | Pufferadresse            | —                    |

- I$WritLn schreibt bis einschließlich CR (oder LF) oder bis d1 erreicht ist;
  CR wird auf der Konsole als CR+LF ausgegeben.
- **Seit Phase 3.4**: hat das Gerät hinter dem Pfad einen File-Manager MIT `write`-Op
  (`q9_dev_t.fm`, z.B. FAT16 an `/d0`), geht I$Write (nicht I$WritLn — Zeilenmodus ergibt für
  Dateien keinen Sinn, analog zu I$Read/I$ReadLn) an `fm->write()` statt an die Treiber-Op —
  schreibt ab der über I$Open/I$Create an `q9_path_t.fmctx` gebundenen Position, alloziert bei
  Bedarf neue Cluster ans Kettenende und aktualisiert Größe/Start-Cluster im Directory-Eintrag.

### I$Attach ($80) / I$Detach ($81) — seit Phase 1.6

| Register | I$Attach Input             | I$Attach Output | I$Detach Input |
|----------|----------------------------|-----------------|----------------|
| d0.b     | Zugriffsmodus (noch ignoriert) | —           | —              |
| a0       | Gerätename ("/term")       | —               | —              |
| a2       | —                          | Gerätetabellen-Eintrag | Gerätetabellen-Eintrag |

- Name wird per F$PrsNam-Logik geparst (führender `/` optional, case-insensitiv).
- Unbekanntes Gerät → `E$MNF` (wie OS-9: kein Descriptor-Modul gefunden);
  ungültiger Name → `E$BPNam`; ungültiger a2 bei Detach → `E$Param`.
- Attach/Detach zählen den Link-Count des Geräts hoch/runter. Da Treiber noch
  einkompiliert sind (bis Phase 2), wird bei Link-Count 0 noch nichts entladen.

### I$Dup ($82) / I$Close ($8F) — seit Phase 1.4

| Register | Input      | Output (nur I$Dup)     |
|----------|------------|------------------------|
| d0.w     | Pfadnummer | neue Pfadnummer        |

- I$Dup dupliziert einen offenen Pfad (gleiches Gerät, gleicher Modus) auf die
  **niedrigste freie** Pfadnummer — OS-9-Semantik, wichtig für spätere I/O-Umlenkung.
- I$Close gibt den Pfadeintrag frei; geschlossene/ungültige Pfade → `E$BPNum`,
  volle Pfadtabelle bei Dup → `E$PthFul`.

### F$PrsNam ($10) / F$CmpNam ($11) — seit Phase 1.5

F$PrsNam parst das nächste Pathlist-Element (führender `/` wird übersprungen;
gültige Namenszeichen: `A-Z a-z 0-9 _ . $`):

| Register | Input          | Output                              |
|----------|----------------|-------------------------------------|
| d0       | —              | d0.b = Trennzeichen nach dem Namen  |
| d1       | —              | d1.w = Namenslänge                  |
| a0       | Pathlist-Ptr   | Start des Namens (nach `/`)         |
| a1       | —              | erstes Zeichen NACH dem Namen       |

- Kein gültiger Name an der Position → `E$BPNam`. Ketten-Parsing: nächster
  Aufruf mit a0 = a1 des vorherigen.

F$CmpNam vergleicht zwei Namen fester Länge, **case-insensitiv**:

| Register | Input                    |
|----------|--------------------------|
| d1.w     | Länge                    |
| a0       | Name 1                   |
| a1       | Name 2                   |

- Gleich → 0; verschieden → `E$Differ` ($A5, MWOS-verifiziert). Wildcards: noch keine.

### I$GetStt ($8D) / I$SetStt ($8E) — seit Phase 1.8

| Register | Input                       | Output (je nach Code)   |
|----------|-----------------------------|-------------------------|
| d0.w     | Pfadnummer                  | —                       |
| d1.w     | Status-Code (SS.*)          | SS.Ready: Zeichen im Eingabepuffer |

Implementierte Codes (**SS-Nummern beim MWOS-Abgleich prüfen**):

| Code | Name     | Gerät | Verhalten |
|------|----------|-------|-----------|
| $01  | SS.Ready | /term | pollt Eingabe; d1.l = gesammelte Zeichen, sonst `E$NotRdy` — der saubere Weg zu prüfen, ob Eingabe ansteht |
| $06  | SS.EOF   | /term | nie am Dateiende → 0 |
| $06  | SS.EOF   | /nil  | immer am Dateiende → `E$EOF` |
| $14  | SS.BlkRd | /d0   | d2.l = LBA, a0 = Puffer (Q9_BLK_SIZE Byte) → liest Block über die HAL (seit 3.1) |
| $15  | SS.BlkWr | /d0   | d2.l = LBA, a0 = Puffer (Q9_BLK_SIZE Byte) → schreibt Block über die HAL (seit 3.1) |

- Unbekannte Codes bzw. Treiber ohne getstat/setstat-Op → `E$UnkSvc`.
- I$SetStt hat außer SS.BlkWr (nur /d0) noch keine weiteren Codes (SS.Opt für
  Pfadoptionen kommt später).
- SS.BlkRd/SS.BlkWr: kein Puffer (`a0` = 0) → `E$Param`, HAL-Fehler → `E$NotRdy`.
  Details: docs/DEVICES.md.

### I$Open ($84) / I$ChgDir ($86) — seit Phase 3.2 (VFS-Schicht)

| Register | I$Open Input                | I$Open Output    | I$ChgDir Input |
|----------|------------------------------|-------------------|-----------------|
| d0.b     | Zugriffsmodus (Q9_MODE_...) | neue Pfadnummer (d0.w) | — |
| a0       | Pathlist ("/d0/pfad/datei") | —                 | Pathlist        |

- **Pfad-Routing** (docs/DEVICES.md, docs/MODULES.md — OS-9-Vorbild IOMan/RBF-Trennung):
  `q9_vfs_open` löst relative Pfade (kein führendes `/`) zuerst gegen das globale
  Arbeitsverzeichnis auf (s.u.), zerlegt dann per F$PrsNam in Gerätename + Rest-Pfad.
- **Gerät ohne File-Manager** (z.B. `/term`, `/nil`): Rest-Pfad **muss leer** sein — sonst
  `E$PNNF`. Rückwärtskompatibel zu Phase 1/2 (entspricht dem bisherigen `q9_path_open`).
- **Gerät mit File-Manager** (`q9_dev_t.fm`): Rest-Pfad wird an `fm->open()` weitergereicht,
  das einen Datei-Kontext im Pfad-Deskriptor ablegt (`q9_path_t.fmctx`, device.h —
  File-Manager-eigenes Byte-Array, kein malloc). **Seit Phase 3.3** hat `/d0` produktiv einen
  File-Manager (FAT16, siehe docs/DEVICES.md), sofern das gemountete Image beim Boot als
  FAT16-Superfloppy erkannt wurde — sonst bleibt `fm` weiterhin `NULL` wie in Phase 1/2/3.2.
- I$ChgDir setzt **ein einziges globales** Arbeitsverzeichnis (kein Pfad pro Prozess — das
  kommt erst mit echten Prozessen in Phase 4). Direkt nach dem Boot: `/` (nur absolute Pfade
  funktionieren, bis ein I$ChgDir gesetzt wurde). Zu langer/leerer Pfad → `E$PNNF`.

### I$Create ($83) / I$MakDir ($85) / I$Delete ($87) — seit Phase 3.4 (FAT16 schreibend)

| Register | I$Create Input               | I$Create Output       | I$MakDir/I$Delete Input |
|----------|-------------------------------|------------------------|--------------------------|
| d0.b     | Zugriffsmodus (Q9_MODE_...)  | —                      | —                        |
| a0       | Pathlist ("/d0/pfad/NEU.TXT") | —                      | Pathlist                 |
| d0.w     | —                              | neue Pfadnummer        | —                        |

- Ohne File-Manager hinter dem Geräte (z.B. `/term`, `/nil`) → `E$UnkSvc` (unverändert seit 3.2 —
  diese Geräte kennen kein Dateisystem).
- **FAT16 (`/d0`, seit 3.3 erkannt)**:
  - **I$Create**: legt einen neuen 8.3-Directory-Eintrag an (Größe 0, noch ohne Cluster — der
    erste Cluster kommt erst mit dem ersten I$Write dazu) und öffnet ihn direkt (liefert eine
    Pfadnummer wie I$Open). **Nur 8.3-Namen** — ein Name, der LFN-Einträge bräuchte (>8+3
    Zeichen, mehr als ein Punkt, ungültige Zeichen), liefert `E$BPNam` (LFN-**Schreiben** ist
    bewusst nicht implementiert, siehe ARBEITSPLAN.md/Ideenspeicher). Existiert der Name bereits
    → ebenfalls `E$BPNam` (OS-9-Vorbild: I$Create auf einen vorhandenen Namen ist ein Fehler,
    anders als I$Open).
  - **I$MakDir**: wie I$Create, alloziert aber sofort einen Datencluster mit den Standard-
    Einträgen `.` (zeigt auf sich selbst) und `..` (zeigt auf das Elternverzeichnis, `0` = Root)
    — wichtig für Interop mit macOS/Windows, die diese Einträge beim Navigieren erwarten.
  - **I$Delete**: sucht den Directory-Eintrag, gibt seine komplette Cluster-Kette frei (beide
    FAT-Kopien) und markiert den Eintrag als gelöscht (erstes Namensbyte `DIRENT_FREE`, $E5).
    Verzeichnisse werden **nicht** auf Leerheit geprüft (bewusst einfach gehalten).
  - **FAT-Updates gehen immer in BEIDE FAT-Kopien** (freie Cluster werden linear ab Cluster 2
    gesucht) — wichtig für Interop, macOS/Windows lesen im Zweifel die zweite Kopie.
  - Details/Interop-Nachweis: docs/DEVICES.md, ARBEITSPLAN.md (Schritt 3.4).

### I$Seek ($88) — seit Phase 3.3

| Register | Input                          |
|----------|--------------------------------|
| d0.w     | Pfadnummer                     |
| d1.l     | neue Position (absolut, Byte)  |

- Hat das Gerät hinter dem Pfad **keinen** File-Manager oder keine `seek`-Op → `E$UnkSvc`
  (vor 3.3 gab es I$Seek noch gar nicht im Dispatcher — Rohdatenträger/Zeichengeräte kennen
  keine Position).
- FAT16 (`fm->seek`): setzt die Position absolut (kein `SEEK_CUR`/`SEEK_END` wie bei Unix,
  OS-9-Vorbild). `E$Param`, wenn die Position hinter dem Dateiende liegt (Verzeichnisse mit
  unbekannter Größe ausgenommen — deren Grenze zeigt sich erst beim nächsten I$Read über die
  Kettenlänge).

### F$Fork ($03) — seit Phase 4.2

| Register | Input                                    | Output                       |
|----------|-------------------------------------------|-------------------------------|
| a0       | Modulname (C-String, wie F$Link)          | —                             |
| d1.b     | Modul-Type (0 = beliebig)                  | —                             |
| d2.b     | Modul-Language (0 = beliebig)               | —                             |
| d0.w     | —                                           | PID des neuen Kind-Prozesses  |

Sucht das Modul über das Modul-Directory (`q9_mod_link` — wie F$Link, erhöht dessen Link-Count),
liest daraus über `q9_proc_native_entry` einen `q9_proc_step_fn`-Funktionszeiger (nur
**`Q9_MOD_NATIVE`**-Module sind ausführbar — Entscheidung E9, PROJECT.md, solange es keine
68k/WASM-Runtime gibt) und trägt einen neuen Prozess in die Tabelle ein: Parent = aufrufende PID,
Std-Pfade vom Parent geerbt, Zustand `ACTIVE`. Fehler: `E$MNF` (Modul nicht gefunden), `E$NEMod`
(Modul existiert, ist aber nicht `Q9_MOD_NATIVE` — noch nicht ausführbar), `E$PrcFul`
(Prozesstabelle voll, `Q9_NPROCS` = 8).

### F$Wait ($04) — seit Phase 4.2

| Register | Output                                   |
|----------|--------------------------------------------|
| d0.w     | PID des eingesammelten Kind-Prozesses       |
| d1.w     | dessen Exit-Code                            |

Sucht ein beendetes (Zombie-)Kind der aufrufenden PID und reapt es (Tabellenslot wird frei). Kein
Zombie, aber mindestens ein noch laufendes Kind: **`E$NotRdy`** — seit Phase 4.3 versetzt
`syscall.c` den Aufrufer dabei zusätzlich per `q9_proc_wait_child` in den Zustand
`Q9_PS_WAITING` (Weckgrund `Q9_WAIT_CHILD`): der Scheduler steppt ihn erst wieder, sobald
mindestens ein Kind Zombie geworden ist (proc.c: `zombie_child_exists()`). Der Rückgabewert
bleibt `E$NotRdy` (kein eingefrorener Stack). Nie ein Kind gehabt (auch keins mehr übrig):
`E$NoChld`.

### F$Chain ($05) — seit Phase 4.2

| Register | Input                             |
|----------|--------------------------------------|
| a0       | Modulname (C-String, wie F$Fork)     |
| d1.b     | Modul-Type (0 = beliebig)             |
| d2.b     | Modul-Language (0 = beliebig)         |

Ersetzt das Modul des AUFRUFENDEN Prozesses (PID/Parent/Std-Pfade bleiben, Exit-Code wird auf 0
zurückgesetzt) — wie F$Fork nur für `Q9_MOD_NATIVE`-Module (sonst `E$NEMod`, altes Modul bleibt
unangetastet). Das alte Modul wird entlinkt (`q9_mod_unlink`), bevor das neue verlinkt wird.

### F$Exit ($06) — echte Semantik seit Phase 4.2

| Register | Input       |
|----------|-------------|
| d1.w     | Status-Code |

Beendet den AUFRUFENDEN Prozess: Exit-Code merken, ein evtl. verlinktes Modul entlinken. Hat der
Prozess einen Parent (`parent != 0`), wird er **Zombie** (wartet auf F$Wait des Parents); hat er
keinen (Parent 0 — z.B. PID 1, die REPL), wird der Tabellenslot sofort freigegeben, weil niemand
reapen kann. Der Scheduler ruft einen nicht-`ACTIVE`-Prozess nie wieder als Step-Funktion auf —
das ersetzt den alten `q9_proc_halted()`-Notbehelf aus Phase 1..4.1 (entfernt).

**Bewusst nicht Teil von 4.2** (Ideenspeicher): Reparenting verwaister Kind-Prozesse auf PID 1,
falls deren Parent selbst beendet wird, bevor er sie reapen konnte.

### F$ID ($0C)

| Register | Output                  |
|----------|-------------------------|
| d0.w     | Prozess-ID              |
| d1.l     | User-ID (Phase 1: 0)    |

**Seit Phase 4.1**: `d0` kommt aus der echten Prozess-Descriptor-Tabelle (`proc.c`,
`q9_proc_current()` — der Prozess, dessen Step-Funktion der Scheduler gerade ausführt), nicht
mehr fest verdrahtet. Außerhalb eines Scheduler-Aufrufs (z.B. ein Selbsttest-Syscall vor dem
ersten `q9_kernel_step()`-Tick) liefert `q9_proc_current()` NULL — F$ID fällt dann auf PID 1
zurück (ebenso F$Fork/F$Wait/F$Chain/F$Exit, wenn außerhalb der Schedulers aufgerufen).

### F$Sleep ($0A) — seit Phase 4.3

| Register | Input                                |
|----------|----------------------------------------|
| d1.l     | Ticks (0 = einmal yielden)             |

Versetzt den AUFRUFENDEN Prozess in den Zustand `Q9_PS_SLEEPING` (Weckgrund `Q9_WAIT_TIMER`,
proc.c: `q9_proc_sleep`): `wake_tick` = aktueller Scheduler-Tick-Zähler + `d1.l` (0 Ticks → +1,
"einmal yielden" — der Prozess pausiert fuer genau einen Scheduler-Durchlauf). Der Syscall selbst
kehrt sofort mit 0 zurück (kein eingefrorener Stack, Entscheidung E8) — die eigentliche
Step-Funktion muss danach selbst zurückkehren, der Scheduler ruft sie erst wieder auf, wenn der
Tick-Zähler den Zielwert erreicht hat. Außerhalb eines Prozesses (kein `q9_proc_current()`):
`E$IPrcID`, analog zu F$Chain.

### F$SSpd ($0B) — seit Phase 4.4

| Register | Input                                        |
|----------|-------------------------------------------------|
| d0.w     | PID (0 = aufrufender Prozess)                   |

Versetzt den ZIELPROZESS (nicht zwingend der Aufrufer — wie in echtem OS-9 darf jede bekannte PID
suspendiert werden) in den Zustand `Q9_PS_WAITING` mit Weckgrund `Q9_WAIT_SIGNAL` (proc.c:
`q9_proc_suspend`). Bewusst **ohne** eigenen Weckmechanismus: der Scheduler steppt einen so
suspendierten Prozess nie wieder von selbst — erst `F$Send` (Phase 4.5) wird `WAITING`/`SLEEPING`
unabhängig vom Weckgrund gewaltsam abbrechen können. `d0.w == 0` außerhalb eines Prozesses (kein
`q9_proc_current()`): `E$IPrcID`. Unbekannte PID: `E$IPrcID`.

### F$SPrior ($0D) — seit Phase 4.4

| Register | Input                          | Output              |
|----------|----------------------------------|----------------------|
| d0.w     | PID (0 = aufrufender Prozess)   | —                    |
| d1.b     | neue Priorität                  | alte Priorität       |

Setzt das `priority`-Feld der Ziel-PID (proc.c: `q9_proc_set_priority`) und liefert den alten Wert
zurück. **Reines Datenfeld** — der Scheduler bleibt Round-Robin, Priorisierung/Aging lohnt sich
erst bei echter Konkurrenz um Rechenzeit (ARBEITSPLAN.md, Schritt 4.4). `d0.w == 0` außerhalb
eines Prozesses: `E$IPrcID`. Unbekannte PID: `E$IPrcID`.

### F$Send ($08) / F$Icpt ($09) / F$RTE ($1E) — seit Phase 4.5

| Register | F$Send Input                    | F$Icpt Input                          |
|----------|-------------------------------------|--------------------------------------|
| d0.w     | Ziel-PID                            | —                                      |
| d1.l     | Signal-Nummer                       | —                                      |
| a0       | —                                    | Intercept-Handler (`q9_proc_step_fn`, `NULL` = deinstalliert) |

**F$Icpt** installiert `a0` als Intercept-Handler des AUFRUFENDEN Prozesses (proc.c:
`q9_proc_icpt`) — anders als F$SSpd/F$SPrior wirkt F$Icpt bewusst nur auf sich selbst (wie in
echtem OS-9), außerhalb eines Prozesses (kein `q9_proc_current()`): `E$IPrcID`.

**F$Send** stellt der Ziel-PID ein Signal zu (proc.c: `q9_proc_send`): steht der Zielprozess in
`WAITING`/`SLEEPING`, wird er UNABHÄNGIG vom Weckgrund sofort `ACTIVE` ("Signal bricht
Waiting/Sleeping ab"). Ist zusätzlich ein Intercept-Handler installiert, ruft der Scheduler ab
dem nächsten Tick diesen Handler statt der normalen Step-Funktion auf (`q9_pd_t.in_intercept`) —
der Handler liest die zugestellte Signal-Nummer über `q9_proc_current()->pending_signal`. Ohne
installierten Handler bleibt es beim reinen Aufwecken, kein Umweg über einen Handler. Unbekannte
Ziel-PID: `E$IPrcID`.

**F$RTE** beendet den Intercept-Modus der AUFRUFENDEN PID (proc.c: `q9_proc_rte`) — ab dem
nächsten Tick ruft der Scheduler wieder die normale Step-Funktion. Außerhalb eines Prozesses ODER
wenn gerade gar kein Intercept läuft (kein vorheriges F$Send mit installiertem Handler):
`E$IPrcID`.

Bewusste Vereinfachung ggü. echtem OS-9: Q9 kennt genau EINEN Intercept-Handler pro Prozess (kein
Signalmaskenkonzept, `F$SigMask`/`F$SigReset` bleiben 💤, s. docs/SYSCALL_ROADMAP.md) und keine
vordefinierten Signalnummern — die Bedeutung von `d1.l` liegt beim aufrufenden Code.

### F$Time ($15) / F$STime ($16) — seit Phase 1.9 echte Uhrzeit, Register 1.9.1 MWOS-korrigiert

| Register | F$Time Output                          | F$STime Input     |
|----------|----------------------------------------|-------------------|
| d0.l     | Zeit: (Std<<16) \| (Min<<8) \| Sek     | Zeit (gleich)     |
| d1.l     | Datum: (Jahr<<16) \| (Monat<<8) \| Tag | Datum (gleich)    |
| d2.w     | Wochentag (0 = Sonntag)                | —                 |
| d3.l     | Millisekunden-Ticks seit Boot          | —                 |

- Registerbelegung MWOS-verifiziert (OS-9 for 68K Technical Reference Manual,
  Tabelle „Gregorian vs. Julian Time"): **d0 = Zeit, d1 = Datum** — bewusst
  gegenläufig zur intuitiven Reihenfolge, aber exakt wie im echten OS-9/68K.
  Die Feld-Packung selbst (Jahr/Monat/Tag bzw. Std/Min/Sek) war von Anfang an
  korrekt, nur d0/d1 waren zunächst vertauscht (Fix: 2026-07-03).
- Zeitquelle: `q9_hal_time()` (native: localtime, wasm: `Date`), einmalig beim
  ersten F$Time geholt und über den ms-Ticker fortgeschrieben. Ohne Zeitquelle
  startet die Uhr bei 2000-01-01. F$STime stellt die Kernel-Uhr (nicht die
  Host-Uhr); ungültige Werte → `E$Param` (echtes OS-9 prüft laut Handbuch gar
  nicht — bewusst strengere Q9-Variante). Kalenderbereich: 2000–2136 (uint32).
- Der `Format`-Eingabeparameter von echtem F$Time (d0.w: 0=Gregorianisch,
  1=Julianisch, 2/3=mit Tick-Rate) wird von Q9 noch **nicht** ausgewertet —
  offener Punkt im Ideenspeicher, kein Bug.

### F$Link ($00) / F$UnLink ($02) — seit Phase 2.3d

| Register | F$Link Input                          | F$Link Output                  | F$UnLink Input |
|----------|----------------------------------------|--------------------------------|-----------------|
| a0       | Modulname (nullterminiert)             | —                               | —               |
| d1.b     | Type (0 = beliebig)                    | —                               | —               |
| d2.b     | Language (0 = beliebig)                | —                               | —               |
| a1       | —                                       | Modul-Header-Zeiger             | Modul-Header-Zeiger (von F$Link) |
| a2       | —                                       | Einsprung (Header + ExecOffset) | —               |
| d0.b     | —                                       | Revision                        | —               |

- Sucht in der Modul-Directory (2.3c: `q9_mod_register`) nach Name (+ Type/
  Language, falls angegeben) — kein Datei-/ROM-Zugriff hier, das Modul muss
  vorher per `q9_mod_register` bekannt gemacht worden sein (bei Q9 aktuell
  nur über den Selbsttest möglich; `F$Load`/Boot-Scan folgt mit 2.4/2.2).
  Nicht gefunden → `E$MNF`.
- F$UnLink senkt den Link-Count (Boden bei 0), Directory-Eintrag/Speicher
  bleiben bestehen — Q9 hält Module aktuell als In-Place-Referenz aufs
  ROM-Image (kein Entladen nötig, siehe Phase-2-Intro in ARBEITSPLAN.md).
  Unbekannter Header-Zeiger → `E$MNF`.
- **Nicht binärkompatibel zu OS-9** (Entscheidung E2): echtes F$Link liefert
  A2 = Header, A1 = Einsprung; Q9 vertauscht das bewusst zugunsten der
  eigenen a1/a2-Konvention (a1 = "das gefundene Ding", a2 = "wo man
  reinspringt", konsistent mit I$Attach a2 = Geräte-Handle).

### F$Load ($01) — seit Phase 3.5

| Register | Input                                   | Output                          |
|----------|------------------------------------------|----------------------------------|
| a0       | Pathlist zur Moduldatei ("/d0/HELLO.MOD") | —                               |
| a1       | —                                          | Modul-Header-Zeiger              |
| a2       | —                                          | Einsprung (Header + ExecOffset)  |
| d0.b     | —                                          | Revision                         |

- Oeffnet die Datei ueber die VFS-Schicht (`q9_vfs_open`, braucht einen File-
  Manager hinter dem Geraet, z.B. FAT16 an `/d0` seit 3.3), liest sie
  **komplett** in einen von `Q9_MOD_LOADBUF_COUNT` (4) statischen Load-
  Puffern (`Q9_MOD_LOADBUF_SIZE` = 4096 Byte, kein malloc im Kernel — erste
  Speicherverwaltung des Kernels, ARBEITSPLAN.md Schritt 3.5), validiert sie
  als Q9-Modul (`q9_mod_validate` — die ganze Datei IST das Modul, anders als
  beim ROM-Image gibt es **keine Sync-Suche**: der Header MUSS bei Byte 0
  stehen) und traegt sie in die Modul-Directory ein (`q9_mod_register`),
  danach wie F$Link ein Link-Count-Anstieg.
- **Abweichung von OS-9** (zusaetzlich zu Entscheidung E2): echtes OS-9
  durchsucht eine Execution-Search-List aus Verzeichnissen anhand des reinen
  Modulnamens; Q9 hat noch keine Suchliste (kommt fruehestens mit Prozessen/
  Shell in Phase 4) und nimmt deshalb bewusst den vollen Dateipfad entgegen.
- Fehlercodes: `E$PNNF`/sonstige VFS-Fehler beim Oeffnen (Datei fehlt),
  `E$BMHP` bei ungueltigem Header (auch falsche Sync-Bytes — `q9_mod_validate`
  prueft die seit 3.5 explizit, nicht nur der ROM-Scan-Pfad), `E$BMCRC` bei
  CRC-Mismatch, `E$NoRAM` ($ED), wenn kein Load-Puffer mehr frei ist ODER die
  Datei nicht in einen Puffer passt (4096 Byte Obergrenze).
- **Lebensdauer**: anders als ein ROM-Modul (das bei Link-Count 0 registriert
  bleibt, es kostet ja keinen Speicher) wird ein per F$Load geladenes Modul
  bei Link-Count 0 **sofort** wieder aus der Directory entfernt und sein
  Load-Puffer freigegeben (`q9_mod_unlink`) — der kleine Puffer-Pool waere
  sonst nach wenigen Load/Unlink-Zyklen erschoepft. Kein Ghost/Sticky-
  Attribut ($40) ausgewertet (Ideenspeicher, falls ein Modul trotz
  Link-Count 0 resident bleiben soll).
- Namenskollision mit einem bereits registrierten Modul gleicher oder
  hoeherer Revision: wie `q9_mod_register` gewinnt das etablierte Modul,
  `a1` zeigt dann auf DESSEN Header statt auf das frisch geladene; der
  eigene Load-Puffer wird sofort wieder freigegeben.

---

## Bewusste Abweichungen von OS-9 (Phase-1-Stand)

1. ~~Kein Blockieren~~ — seit Phase 4.3 versetzt `syscall.c` den aufrufenden Prozess bei
   `E$NotRdy` (I$Read/I$ReadLn auf dem Treiberpfad, F$Wait ohne Zombie-Kind) in einen echten
   Scheduler-Zustand (`Q9_PS_WAITING`/`SLEEPING` + Weckgrund, proc.h/.c) — der Rückgabewert
   bleibt zwar `E$NotRdy` (kein eingefrorener Stack möglich, Entscheidung E8), aber der
   Scheduler steppt den Prozess erst wieder, wenn der Weckgrund erfüllt ist, statt ihn wie
   davor bei jedem Tick sinnlos erneut aufzurufen.
2. ~~F$Time liefert Uptime~~ — seit Phase 1.9 echte Uhrzeit über `q9_hal_time()`.
3. ~~Pfade 0/1/2 fest verdrahtet~~ — seit Phase 1.3 laufen alle Pfade über das
   Device-Modell (Pfadtabelle → Treiber-Modul, siehe docs/DEVICES.md). Die
   Standardpfade 0/1/2 öffnet der Kernel beim Boot auf /term (Update-Modus).
   ~~User-seitiges I$Open/I$Close mit Pfadnamen kommt in Phase 3~~ — seit
   Phase 3.2 implementiert (I$Open über die VFS-Schicht, s.o.).
4. ~~Kein Dateisystem hinter I$Open vor Phase 3.3~~ — seit Phase 3.3 hat `/d0` produktiv
   einen File-Manager (FAT16 lesend), sofern das gemountete Image erkannt wurde. Ohne
   erkanntes FAT16-Image bzw. bei anderen Geräten (`/term`, `/nil`) bleibt I$Open auf leeren
   Rest-Pfad beschränkt (wie bisher). ~~I$Create/I$MakDir/I$Delete sind reine Gerüste~~ — seit
   Phase 3.4 implementiert FAT16 auch das Schreiben (s.o.); ohne File-Manager weiterhin
   `E$UnkSvc`. **LFN-Schreiben bleibt bewusst außen vor** (nur 8.3-Namen beim Anlegen).

**Erstellt**: 2026-07-03
**Letzte Aktualisierung**: 2026-07-04 (Phase 3.5: F$Load über q9_mod_load — Modul aus Datei
statt nur ROM-Image, statischer Load-Puffer-Pool, siehe eigener Abschnitt oben)
