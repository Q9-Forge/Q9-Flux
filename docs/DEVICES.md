# Q9 Device-Modell

**Phase 1.3 (2026-07-03)**: Kernel-I/O läuft über eine Geräte- und eine Pfadtabelle
(OS-9-Vorbild: IOMan). Treiber sind **interne Module** mit einheitlichen Operationen —
in Phase 2 werden daraus echte Q9-Module (Typ 2 = Treiber im Modul-Header).

---

## Schichtung (OS-9-Analogie)

```
I$-Syscalls (syscall.c)      ← OS-9: IOMan-Einstieg
   │  Pfadnummer → Pfadtabelle, Mode-Check
   ▼
Pfadtabelle / Gerätetabelle (device.c)   ← OS-9: IOMan-Tabellen
   │  dev->drv->op(...)
   ▼
Treiber-Modul (dev_term.c)   ← OS-9: SCF + scf-Treiber (in Q9 vorerst eine Schicht)
   │  Zeilen-Editierung, Echo, CR→CRLF; Zustand im Static Storage
   ▼
HAL (q9_hal_con_get/put)     ← OS-9: Hardware
```

## Tabellen (device.h)

- **Gerätetabelle** (`Q9_NDEVS` = 4): Name, Treiber-Ops (`q9_drv_t`), optionaler File-Manager
  (`q9_dev_t.fm`, seit 3.2 — `NULL` = kein Dateisystem), Static Storage (vom Treiber-`init`
  angehängt), Link-Count.
- **Pfadtabelle** (`Q9_NPATHS` = 8): Gerät + Zugriffsmodus + Datei-Kontext (`fmctx`, seit 3.2).
  `q9_path_open` vergibt die niedrigste freie Pfadnummer (wie OS-9).
- **Modi**: `Q9_MODE_READ` / `WRITE` / `UPDATE`. Verstoß → `E$BMode`,
  ungültige/geschlossene Pfadnummer → `E$BPNum`.

## Treiber-Operationen

```c
typedef struct q9_drv {
    const char *name;
    int (*init)  (q9_dev_t *dev);                              /* Storage anhängen, HW-Init */
    int (*read)  (q9_dev_t *dev, uint8_t *buf, uint32_t *n);   /* roh, ohne Echo            */
    int (*write) (q9_dev_t *dev, const uint8_t *buf, uint32_t *n);
    int (*readln)(q9_dev_t *dev, uint8_t *buf, uint32_t *n);   /* Zeile inkl. CR, editiert  */
    int (*writln)(q9_dev_t *dev, const uint8_t *buf, uint32_t *n); /* stoppt nach CR/LF     */
} q9_drv_t;
```

`*n` ist in/out (max. rein, tatsächlich raus). Rückgabe 0 oder OS-9-Fehlercode.

## Gerät /term (dev_term.c)

Konsolen-Treiber, SCF-artig: `readln` mit Echo/Backspace-Editierung (Zeilenende CR),
`read` roh und nicht blockierend (`E$NotRdy`), Ausgabe gekocht (CR/LF → CR+LF).
Der Zeilenpuffer liegt im Static Storage des **Geräts** (nicht des Pfads) — alle Pfade
auf /term teilen sich die eine physische Konsole.

Beim Boot öffnet `q9_dev_init()` die Standardpfade **0/1/2 auf /term im Update-Modus**
(wie die Standardpfade einer OS-9-Shell).

## Gerät /nil (dev_nil.c) — seit Phase 1.7

Null-Device nach OS-9-Vorbild: Schreiben verwirft die Daten (meldet Erfolg),
Lesen liefert `E$EOF`. Zustandslos. Zweiter Treiber im System — beweist, dass
das Device-Modell trägt (2 Treiber, 1 Schnittstelle).

## Namensbasierter Zugriff — seit Phase 1.5/1.6

- `q9_dev_attach("/term")` / `q9_dev_detach(dev)` (Syscalls I$Attach/I$Detach):
  Gerätesuche per Pathlist-Name (führender `/` optional, **case-insensitiv**,
  Parsing via name.c), Link-Count-Verwaltung. Unbekannt → `E$MNF`.
- `q9_path_open("/nil", mode)` läuft ebenfalls über diese Namenslogik.

## Gerät /d0 (dev_d0.c) — seit Phase 3.1

Roh-Block-Device ohne Dateisystem (Nagelprobe vor der VFS-Schicht, 3.2). I$Read/I$Write/
I$ReadLn/I$WritLn ergeben ohne Dateisystem keinen Sinn → `E$UnkSvc`. Blockzugriff läuft
ausschließlich über I$GetStt/I$SetStt:

| Code | Name      | Register                                  | Verhalten |
|------|-----------|--------------------------------------------|-----------|
| $14  | SS.BlkRd  | d2.l = LBA, a0 = Puffer (Q9_BLK_SIZE Byte) | liest Block über `q9_hal_blk_read` |
| $15  | SS.BlkWr  | d2.l = LBA, a0 = Puffer (Q9_BLK_SIZE Byte) | schreibt Block über `q9_hal_blk_write` |

Kein Puffer (`a0` = 0) → `E$Param`, HAL-Fehler (Image fehlt/I/O-Fehler) → `E$NotRdy`,
andere SS-Codes → `E$UnkSvc`. Zustandslos — die HAL hält das Disk-Image offen (nativ:
`q9disk.img`, lazy erzeugt beim ersten Zugriff, per `.gitignore` ausgeschlossen).

**Seit Phase 3.3** bekommt `/d0` beim Boot zusätzlich einen File-Manager (FAT16, siehe unten),
wenn `q9_fat16_mount()` das Image als FAT16-Superfloppy erkennt — dann laufen I$Open/I$Read/
I$Seek über den File-Manager, SS.BlkRd/SS.BlkWr bleiben unverändert als Rohzugriff daneben
verfügbar (der Treiber selbst kennt kein Dateisystem, das sitzt eine Schicht höher).

### wasm-HAL-Backend für /d0 (OPFS) — seit Phase 3.6

`q9_hal_blk_read`/`q9_hal_blk_write` (HAL-Ebene, nicht Kernel) sind im Browser-Target über
`globalThis.q9blk` an einen **OPFS-`FileSystemSyncAccessHandle`** auf eine Datei `q9disk.img`
im Origin Private File System gebunden (`web/worker.js`). `FileSystemSyncAccessHandle` ist eine
Browser-API, die synchronen Blockzugriff erlaubt, aber **nur innerhalb eines Dedicated Workers**
existiert — deshalb läuft der komplette Q9-Kernel (WASM-Instanz + `q9_kernel_step()`-Loop) seit
diesem Schritt im Worker, nicht mehr im Haupt-Thread. Der Haupt-Thread (`web/index.html`) bedient
nur noch xterm.js und tauscht mit dem Worker per `postMessage` aus:

| Nachricht (Haupt-Thread → Worker) | Bedeutung |
|---|---|
| `{type:'input', code}` | Tastendruck, landet in der Worker-internen Input-Queue (`q9host.getc`) |
| `{type:'load-image', data}` | Upload: ArrayBuffer ersetzt `q9disk.img` komplett (Truncate + Write) |
| `{type:'get-image'}` | Download-Anfrage: Worker liest das ganze Image und schickt es zurück |

| Nachricht (Worker → Haupt-Thread) | Bedeutung |
|---|---|
| `{type:'out', text}` | UTF-8-dekodiertes Konsolen-Fragment (`q9host.putc`), wird in xterm geschrieben |
| `{type:'image-loaded', size}` | Bestätigung nach Upload |
| `{type:'image-data', data}` | Antwort auf Download-Anfrage (ArrayBuffer, transferable), löst Browser-Download aus |

`q9_hal_blk_read` über das Ende des Images hinaus liefert Nullblöcke statt eines Fehlers (analog
einem frisch erzeugten, größtenteils leeren Image); ein neu angelegtes `q9disk.img` wird beim
ersten Worker-Start auf eine feste Default-Größe (2880 Blöcke = 1,44 MB) gebracht, indem der
letzte Block einmal geschrieben wird. **wasm-Build und Browser-Test dieses Schritts sind
ungetestet** — emsdk fehlt sowohl auf dem Desktop-PC als auch auf dem Mac Mini (siehe
ARBEITSPLAN.md, „Geparkt").

## VFS-Schicht (vfs.c/.h) — seit Phase 3.2

Pfad-Routing für Pfade der Form `/d0/pfad/datei`: `q9_vfs_open` (I$Open-Unterbau) trennt per
F$PrsNam (name.c) den Gerätenamen vom Rest-Pfad und reicht Letzteren an einen optionalen
**File-Manager** hinter dem Gerät weiter (OS-9-Vorbild: IOMan/RBF-Trennung, docs/MODULES.md).

```
I$Open (syscall.c)
   │  Pfadname -> Arbeitsverzeichnis-Aufloesung (relativ) -> F$PrsNam
   ▼
q9_vfs_open (vfs.c)
   │  Geraet per Name attachen; dev->fm vorhanden?
   ├─ nein -> altes Verhalten: Rest-Pfad MUSS leer sein (E$PNNF sonst) — /term, /nil
   └─ ja   -> Rest-Pfad an dev->fm->open() weiterreichen
   ▼
File-Manager (q9_fm_t, ab 3.3: FAT16 an /d0, lesend + seit 3.4 schreibend)
   │  fuellt Datei-Kontext im Pfad-Deskriptor (q9_path_t.fmctx)
   ▼
Treiber (q9_drv_t) — liest/schreibt Bloecke fuer den File-Manager
```

### File-Manager-Schnittstelle (`q9_fm_t`, vfs.h)

Analog zum Treiber-Interface `q9_drv_t` (device.h), aber eine Stufe höher: der Treiber spricht
Blöcke/Zeichen, der File-Manager spricht Pfade/Dateien.

```c
typedef struct q9_fm {
    const char *name;
    int (*open)  (q9_dev_t *dev, q9_path_t *p, const char *restpath, uint32_t len, uint8_t mode);
    int (*create)(q9_dev_t *dev, q9_path_t *p, const char *restpath, uint32_t len, uint8_t mode);
    int (*makdir)(q9_dev_t *dev, const char *restpath, uint32_t len);
    int (*remove)(q9_dev_t *dev, const char *restpath, uint32_t len);
    int (*read)  (q9_dev_t *dev, q9_path_t *p, uint8_t *buf, uint32_t *n);     /* seit 3.3 */
    int (*write) (q9_dev_t *dev, q9_path_t *p, const uint8_t *buf, uint32_t *n); /* seit 3.4 */
    int (*seek)  (q9_dev_t *dev, q9_path_t *p, uint32_t pos);                  /* seit 3.3 */
} q9_fm_t;
```

- `restpath` ist der Teil hinter dem Gerätenamen, **ohne** führenden `/` (leer = Gerätewurzel).
- Ein Gerät bekommt seinen File-Manager über `q9_dev_set_fm(dev, &fm)` (`q9_dev_t.fm`,
  `NULL` = kein Dateisystem — Grundzustand aller Geräte).
- **Schnitt bewusst schmal gehalten** (Rest-Pfad-String statt OS-9-Pfaddeskriptor-Internas),
  damit später auch ein 68k-Manager-Adapter (Ideenspeicher, ARBEITSPLAN.md) dieselbe
  C-Schnittstelle hinter einer Trap-Bridge bedienen kann — mit dem Dibble-Buch
  ("OS-9 Insights") als Vorbild abgeglichen, aber eigene C99-Implementierung (kein Codeklau).
- **Design-Entscheidung 3.3**: `q9_fm_t` wurde um `read`/`seek` erweitert (zusätzlich zu
  `open`/`create`/`makdir`/`remove` aus 3.2). Grund: I$Read/I$Seek brauchen für Dateien den
  Datei-Kontext aus `q9_path_t.fmctx` (aktuelle Position/Cluster) — das kann nur der
  File-Manager interpretieren, nicht der Block-Treiber dahinter. Der Kernel-Dispatcher
  (syscall.c) prüft bei I$Read zuerst, ob `p->dev->fm->read` gesetzt ist, und routet dorthin;
  sonst (wie bisher) an die Treiber-Op. I$Seek existiert erst seit 3.3 und liefert `E$UnkSvc`,
  wenn kein File-Manager mit `seek`-Op hinter dem Pfad steht.
- **Design-Entscheidung 3.4**: `q9_fm_t` wurde zusätzlich um `write` erweitert (I$Write
  routet analog zu I$Read auf `fm->write()`, wenn vorhanden). `create`/`makdir`/`remove`
  bekommen jetzt echte Semantik (FAT16 — s.u.), statt wie bis 3.2/3.3 nur `E$UnkSvc` zu liefern.

### Datei-Kontext pro Pfad (`q9_path_t.fmctx`, device.h)

Jeder offene Pfad hat ein festes Byte-Array (`Q9_FMCTX_SIZE`) für File-Manager-eigenen
Zustand — **kein malloc**, der Kontext liegt direkt in der (statischen) Pfadtabelle. Der
Kernel fasst das Feld nie an, nur der File-Manager hinter dem jeweiligen Gerät interpretiert
es. Für Geräte ohne File-Manager unbenutzt (bleibt 0). **Seit 3.4**: `Q9_FMCTX_SIZE` von 16
auf **24 Byte** vergrößert — FAT16 schreibend braucht zusätzlich die Fundstelle des eigenen
Directory-Eintrags (Elternverzeichnis-Cluster + Slot-Index), um nach einem I$Write, das die
Datei wachsen lässt, Größe/Start-Cluster im Directory zurückzuschreiben.

**FAT16-Belegung (`fat16_ctx_t` in fat16.c)** — passt exakt in die 24 Byte:

| Offset | Feld            | Bedeutung |
|--------|-----------------|-----------|
| 0      | `start_cluster` | Start-Cluster der Datei; `0` = Root-Directory-Pseudo-Datei (fester Bereich, keine Kette), oder (seit 3.4) noch kein Cluster alloziert (frisch per I$Create angelegt) |
| 4      | `cur_cluster`   | Cluster, in dem die aktuelle Position liegt (Cache, um nicht bei jedem Read/Write die Kette neu abzulaufen) |
| 8      | `pos`           | aktuelle Byte-Position in der Datei |
| 12     | `size`          | Dateigröße (`0xFFFFFFFF` bei Unterverzeichnissen — Ende zeigt sich erst über die Cluster-Kette) |
| 16     | `dir_start`     | *(seit 3.4)* Elternverzeichnis des eigenen Directory-Eintrags (`0` = Root); `0xFFFFFFFF` bei der Root-Pseudo-Datei selbst (kein Eintrag zum Zurückschreiben) |
| 20     | `dir_index`     | *(seit 3.4)* Slot-Index im Elternverzeichnis — zusammen mit `dir_start` die Fundstelle für das I$Write-Directory-Update |

### FAT16-File-Manager (fat16.c/.h) — seit Phase 3.3, schreibend seit Phase 3.4

Erster echter File-Manager, **nur Superfloppy** (Boot-Sektor bei LBA 0 des Block-Device, keine
Partitionstabelle — MBR-Partitionen sind Ideenspeicher). Layout nach öffentlich dokumentierter
FAT16-Spezifikation (Boot-Sektor/BPB, Root-Directory-Region fester Größe, 16-Bit-FAT-Einträge,
8.3-Verzeichniseinträge, LFN-Einträge mit Attribut `$0F`) — eigene C99-Implementierung, kein
Copyright-Code übernommen. Design-Referenz für den File-Manager-Schnitt: "OS-9 Insights"
(Dibble), nur konzeptionell gelesen.

- **Mount**: `q9_fat16_mount()` liest den Boot-Sektor über `q9_hal_blk_read(0, ...)` und
  prüft ihn auf Plausibilität (BytesPerSector `== Q9_BLK_SIZE`, SectorsPerCluster
  Zweierpotenz, FATSize16/RootEntryCount/NumFATs `!= 0`, Boot-Signatur `$55AA`). Bei Erfolg
  wird die Geometrie (FAT-Start/Root-Dir-Start/Datenregion-Start/Cluster-Größe, Anzahl
  FAT-Kopien) in statischen Variablen gemerkt — kein malloc, ein Q9 kennt genau einen
  Datenträger.
- **Verdrahtung ans Gerät**: `q9_dev_init()` (device.c) ruft `q9_fat16_mount()` direkt nach
  der `/d0`-Registrierung; nur bei Erfolg wird `q9_dev_set_fm(d0, &q9_fat16_fm)` gesetzt —
  ist das Image kein FAT16-Superfloppy (z.B. leer, oder ein generisches Testimage wie in
  Test 04), bleibt `/d0` ohne File-Manager wie vor 3.3 (Rest-Pfad muss dann leer sein).
- **Verzeichnisse lesen**: `dir_find()`/`dir_find_idx()` durchsuchen Root- oder
  Unterverzeichnisse Slot für Slot; LFN-Einträge (Attribut `$0F`) liegen laut Spezifikation in
  ABsteigender Sequenznummer VOR dem zugehörigen 8.3-Eintrag — die Namensteile werden dabei
  von hinten nach vorne zu einem vollständigen Namen zusammengesetzt und sowohl gegen den
  LFN-Namen als auch den 8.3-Namen verglichen (case-insensitiv). `dir_find_idx()` liefert
  zusätzlich den Slot-Index (seit 3.4, für I$Write-Directory-Updates und I$Delete gebraucht).
- **Cluster-Ketten lesen**: `fat_next()` liest den nächsten Cluster aus der ersten FAT-Kopie
  (16-Bit-Little-Endian-Einträge); Kettenende ist `$FFF8`–`$FFFF`. Ein einziger statischer
  512-Byte-Sektor-Puffer (`secbuf`) reicht für alle Zugriffe, da Q9 nicht nebenläufig ist.
- **I$Read/I$Seek**: routen über die `q9_fm_t`-Felder `read`/`seek`. `fat16_read` folgt bei
  Cluster-Grenzen automatisch der Kette weiter; `fat16_seek` setzt Position und passenden
  Cluster-Zeiger neu.
- **Cluster-Ketten allozieren/freigeben (3.4)**: `fat_alloc()` sucht linear ab Cluster 2 den
  ersten freien Cluster (FAT-Eintrag `== FAT16_FREE`, `$0000`) und markiert ihn sofort als
  Kettenende (`$FFFF`) — **in BEIDEN FAT-Kopien** (`fat_set()` schreibt immer alle
  `numfats`-Kopien synchron, wichtig für Interop: ein Reader, der nur die zweite Kopie
  heranzieht, sieht sonst inkonsistente Daten). `fat_free_chain()` läuft eine komplette Kette
  ab und setzt jedes Glied in beiden Kopien auf `FAT16_FREE` zurück.
- **I$Write (`fat16_write`)**: schreibt ab der aktuellen Position, alloziert bei Bedarf neue
  Cluster ans Kettenende (auch den allerersten Cluster einer per I$Create angelegten, noch
  leeren Datei), Read-Modify-Write pro Sektor (falls nur ein Teil geschrieben wird). Wächst die
  Datei, wird danach über `dir_start`/`dir_index` (fmctx) der Directory-Eintrag im
  Elternverzeichnis nachgezogen (Start-Cluster + neue Größe).
- **I$Create (`fat16_create`)**: löst den Pfad bis zum Elternverzeichnis auf
  (`resolve_parent`), validiert das letzte Element als reinen 8.3-Namen (`make_83name` — nur
  `A-Z0-9_$`, Basis ≤8, Erweiterung ≤3, genau ein Punkt; **kein LFN-Schreiben**, Ideenspeicher/
  ARBEITSPLAN.md), lehnt bereits vorhandene Namen ab (`E$BPNam`, OS-9-Vorbild), sucht/alloziert
  einen freien Directory-Slot (`dir_alloc_slot` — verlängert bei Unterverzeichnissen bei Bedarf
  die Cluster-Kette; das Root-Directory ist ein fester Bereich fester Größe und kann NICHT
  wachsen) und schreibt einen neuen Eintrag (Attribut `ARCHIVE`, Cluster 0, Größe 0 — der erste
  Cluster kommt beim ersten I$Write dazu).
- **I$MakDir (`fat16_makdir`)**: wie I$Create, alloziert aber sofort einen Datencluster mit
  Standard-`.`/`..`-Einträgen (`.` zeigt auf sich selbst, `..` auf das Elternverzeichnis,
  `0` = Root) — Standard-FAT-Konvention, wichtig für Interop mit macOS/Windows.
- **I$Delete (`fat16_remove`)**: sucht den Directory-Eintrag (`dir_find_idx`), gibt seine
  komplette Cluster-Kette frei (`fat_free_chain`, beide FAT-Kopien) und markiert den Eintrag
  als gelöscht (erstes Namensbyte `DIRENT_FREE`, `$E5` — Rest des 11-Byte-Namensfelds bleibt
  unverändert stehen, FAT16-Konvention). Verzeichnisse werden nicht auf Leerheit geprüft.
- **Nur der Reserve-/Boot-Sektor-Bereich wird nie angefasst** — Schreibzugriffe beschränken
  sich auf FAT-Kopien, Directory-Bereich und Datencluster (wichtig für Interop, s.u.).

**Wichtiger Seiteneffekt-Fix (3.3)**: der ältere `/d0`-Selbsttest "SS.BlkWr/SS.BlkRd
Roundtrip" (Phase 3.1) schrieb testweise auf LBA 1 und ließ das Testmuster stehen — bei einem
echten FAT16-Image liegt dort typischerweise die erste FAT-Kopie. Der Selbsttest sichert LBA 1
seit 3.3 vor dem Test und stellt ihn danach wieder her, damit er ein zuvor gemountetes FAT16
läuft nicht mehr zerstört.

**Interop-Nachweis (3.4)**: das von Q9 per I$Create/I$Write/I$MakDir/I$Delete geschriebene
Image wurde zusätzlich zur Python-Nachvalidierung (`test/06_test_fat16.py`, `post_validate()`)
testweise mit `hdiutil attach -imagekey diskimage-class=CRawDiskImage` gemountet — macOS
erkennt das Volume (`Q9TESTVOL`), zeigt `HELLO.TXT`/die LFN-Datei korrekt an und liest
`NEUDIR` als echtes Verzeichnis mit funktionierenden `.`/`..`-Einträgen. Kein produktiver
Bestandteil des Testlaufs (`hdiutil` ist nicht auf jeder Entwicklungsmaschine verfügbar) —
die Python-Nachvalidierung bleibt der reproduzierbare, CI-taugliche Interop-Beleg.

### Globales Arbeitsverzeichnis (I$ChgDir)

Ein einziger globaler Pfad-String (`Q9_CWD_MAXLEN` = 63 Zeichen, Start: `/`) — **kein**
Arbeitsverzeichnis pro Prozess (das kommt erst mit echten Prozessen in Phase 4, siehe
ARBEITSPLAN.md Entscheidung E8). Relative Pfade (kein führendes `/`) werden bei jedem I$Open
gegen dieses Verzeichnis aufgelöst (`q9_vfs_cwd()`, vfs.c).

## Ausblick

- **Phase 6+**: Treiber als echte Q9-Module (Typ 2) statt einkompiliert

**Erstellt**: 2026-07-03
**Zuletzt aktualisiert**: 2026-07-04 (Phase 3.5: F$Load über die VFS-Schicht — Modul aus einer
Datei hinter einem beliebigen File-Manager laden, statischer Load-Puffer-Pool in module.c;
Details siehe docs/SYSCALLS.md)
