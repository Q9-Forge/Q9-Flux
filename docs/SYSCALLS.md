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
| $01 | F$Load   | geplant (Phase 2/3) |
| $02 | F$UnLink | ✅ implementiert (Phase 2.3d) |
| $03 | F$Fork   | geplant (Phase 4, Prozesse) |
| $04 | F$Wait   | geplant (Phase 4) |
| $06 | F$Exit   | ✅ implementiert (Phase-1-Semantik: hält Proto-Prozess an) |
| $0C | F$ID     | ✅ implementiert (liefert Proto-Prozess-ID 1) |
| $10 | F$PrsNam | ✅ implementiert (Phase 1.5) |
| $11 | F$CmpNam | ✅ implementiert (Phase 1.5) |
| $15 | F$Time   | ✅ implementiert (Phase 1.9: echte Uhrzeit via HAL) |
| $16 | F$STime  | ✅ implementiert (Phase 1.9) |
| $80 | I$Attach | ✅ implementiert (Phase 1.6) |
| $81 | I$Detach | ✅ implementiert (Phase 1.6) |
| $89 | I$Read   | ✅ implementiert |
| $8A | I$Write  | ✅ implementiert |
| $8B | I$ReadLn | ✅ implementiert |
| $8C | I$WritLn | ✅ implementiert |
| $82 | I$Dup    | ✅ implementiert (Phase 1.4) |
| $84 | I$Open   | ✅ implementiert (Phase 3.2: VFS-Pfad-Routing; seit 3.3 FAT16 an /d0) |
| $83 | I$Create | ✅ Grundgerüst (Phase 3.2: `E$UnkSvc`, echte Semantik erst 3.4) |
| $85 | I$MakDir | ✅ Grundgerüst (Phase 3.2: `E$UnkSvc`, echte Semantik erst 3.4) |
| $86 | I$ChgDir | ✅ implementiert (Phase 3.2: globales Arbeitsverzeichnis) |
| $87 | I$Delete | ✅ Grundgerüst (Phase 3.2: `E$UnkSvc`, echte Semantik erst 3.4) |
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

### I$Write ($8A) / I$WritLn ($8C)

| Register | Input                    | Output               |
|----------|--------------------------|----------------------|
| d0.w     | Pfadnummer               | —                    |
| d1.l     | max. Byte-Anzahl         | tatsächliche Anzahl  |
| a0       | Pufferadresse            | —                    |

- I$WritLn schreibt bis einschließlich CR (oder LF) oder bis d1 erreicht ist;
  CR wird auf der Konsole als CR+LF ausgegeben.

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

### I$Create ($83) / I$MakDir ($85) / I$Delete ($87) — Gerüst seit Phase 3.2

Liefern aktuell `E$UnkSvc` — echte Semantik kommt mit FAT16 schreibend (Phase 3.4), wenn ein
File-Manager auch `create`/`makdir`/`remove` sinnvoll implementieren kann (VFS-seitig sind die
q9_fm_t-Funktionszeiger dafür schon vorgesehen, siehe docs/DEVICES.md).

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

### F$Exit ($06)

| Register | Input       |
|----------|-------------|
| d1.w     | Status-Code |

### F$ID ($0C)

| Register | Output                  |
|----------|-------------------------|
| d0.w     | Prozess-ID              |
| d1.l     | User-ID (Phase 1: 0)    |

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

---

## Bewusste Abweichungen von OS-9 (Phase-1-Stand)

1. **Kein Blockieren**: Bis der Scheduler existiert (Phase 4), liefern I$Read/I$ReadLn
   `E$NotRdy`, wenn keine (vollständige) Eingabe ansteht — der Aufrufer pollt.
   Ab Phase 4 blockiert der aufrufende Prozess, wie es sich gehört.
2. ~~F$Time liefert Uptime~~ — seit Phase 1.9 echte Uhrzeit über `q9_hal_time()`.
3. ~~Pfade 0/1/2 fest verdrahtet~~ — seit Phase 1.3 laufen alle Pfade über das
   Device-Modell (Pfadtabelle → Treiber-Modul, siehe docs/DEVICES.md). Die
   Standardpfade 0/1/2 öffnet der Kernel beim Boot auf /term (Update-Modus).
   ~~User-seitiges I$Open/I$Close mit Pfadnamen kommt in Phase 3~~ — seit
   Phase 3.2 implementiert (I$Open über die VFS-Schicht, s.o.).
4. ~~Kein Dateisystem hinter I$Open vor Phase 3.3~~ — seit Phase 3.3 hat `/d0` produktiv
   einen File-Manager (FAT16 lesend), sofern das gemountete Image erkannt wurde. Ohne
   erkanntes FAT16-Image bzw. bei anderen Geräten (`/term`, `/nil`) bleibt I$Open auf leeren
   Rest-Pfad beschränkt (wie bisher). I$Create/I$MakDir/I$Delete sind weiterhin bis 3.4
   (FAT16 schreibend) reine Gerüste (`E$UnkSvc`).

**Erstellt**: 2026-07-03
**Letzte Aktualisierung**: 2026-07-04 (Phase 3.3: FAT16 lesend über I$Open/I$Read/I$Seek)
