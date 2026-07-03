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
File-Manager (q9_fm_t, ab 3.3: FAT16 an /d0)
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
- `create`/`makdir`/`remove` bleiben Teil der Schnittstelle, aber weiterhin nur Gerüst — echte
  FAT16-Implementierung folgt in 3.4 (schreibend). Der Kernel-Syscall-Dispatcher (syscall.c)
  liefert `I$Create`/`I$MakDir`/`I$Delete` bis dahin direkt `E$UnkSvc`.

### Datei-Kontext pro Pfad (`q9_path_t.fmctx`, device.h)

Jeder offene Pfad hat ein festes Byte-Array (`Q9_FMCTX_SIZE` = 16 Byte) für File-Manager-
eigenen Zustand — **kein malloc**, der Kontext liegt direkt in der (statischen) Pfadtabelle.
Der Kernel fasst das Feld nie an, nur der File-Manager hinter dem jeweiligen Gerät
interpretiert es. Für Geräte ohne File-Manager unbenutzt (bleibt 0).

**FAT16-Belegung (seit 3.3, `fat16_ctx_t` in fat16.c)** — passt exakt in die 16 Byte:

| Offset | Feld            | Bedeutung |
|--------|-----------------|-----------|
| 0      | `start_cluster` | Start-Cluster der Datei; `0` = Root-Directory-Pseudo-Datei (fester Bereich, keine Kette) |
| 4      | `cur_cluster`   | Cluster, in dem die aktuelle Position liegt (Cache, um nicht bei jedem Read die Kette neu abzulaufen) |
| 8      | `pos`           | aktuelle Byte-Position in der Datei |
| 12     | `size`          | Dateigröße (`0xFFFFFFFF` bei Unterverzeichnissen — Ende zeigt sich erst über die Cluster-Kette) |

### FAT16-File-Manager (fat16.c/.h) — seit Phase 3.3

Erster echter File-Manager, **nur lesend**, **nur Superfloppy** (Boot-Sektor bei LBA 0 des
Block-Device, keine Partitionstabelle — MBR-Partitionen sind Ideenspeicher). Layout nach
öffentlich dokumentierter FAT16-Spezifikation (Boot-Sektor/BPB, Root-Directory-Region fester
Größe, 16-Bit-FAT-Einträge, 8.3-Verzeichniseinträge, LFN-Einträge mit Attribut `$0F`) —
eigene C99-Implementierung, kein Copyright-Code übernommen. Design-Referenz für den
File-Manager-Schnitt: "OS-9 Insights" (Dibble), nur konzeptionell gelesen.

- **Mount**: `q9_fat16_mount()` liest den Boot-Sektor über `q9_hal_blk_read(0, ...)` und
  prüft ihn auf Plausibilität (BytesPerSector `== Q9_BLK_SIZE`, SectorsPerCluster
  Zweierpotenz, FATSize16/RootEntryCount/NumFATs `!= 0`, Boot-Signatur `$55AA`). Bei Erfolg
  wird die Geometrie (FAT-Start/Root-Dir-Start/Datenregion-Start/Cluster-Größe) in
  statischen Variablen gemerkt — kein malloc, ein Q9 kennt genau einen Datenträger.
- **Verdrahtung ans Gerät**: `q9_dev_init()` (device.c) ruft `q9_fat16_mount()` direkt nach
  der `/d0`-Registrierung; nur bei Erfolg wird `q9_dev_set_fm(d0, &q9_fat16_fm)` gesetzt —
  ist das Image kein FAT16-Superfloppy (z.B. leer, oder ein generisches Testimage wie in
  Test 04), bleibt `/d0` ohne File-Manager wie vor 3.3 (Rest-Pfad muss dann leer sein).
- **Verzeichnisse lesen**: `dir_find()` durchsucht Root- oder Unterverzeichnisse Slot für
  Slot; LFN-Einträge (Attribut `$0F`) liegen laut Spezifikation in ABsteigender
  Sequenznummer VOR dem zugehörigen 8.3-Eintrag — die Namensteile werden dabei von hinten
  nach vorne zu einem vollständigen Namen zusammengesetzt und sowohl gegen den LFN-Namen als
  auch den 8.3-Namen verglichen (case-insensitiv).
- **Cluster-Ketten**: `fat_next()` liest den nächsten Cluster aus der ersten FAT-Kopie
  (16-Bit-Little-Endian-Einträge); Kettenende ist `$FFF8`–`$FFFF`. Ein einziger statischer
  512-Byte-Sektor-Puffer (`secbuf`) reicht für alle Lesevorgänge, da Q9 nicht nebenläufig ist.
- **I$Read/I$Seek**: routen über die neuen `q9_fm_t`-Felder `read`/`seek` (s.o.). `fat16_read`
  folgt bei Cluster-Grenzen automatisch der Kette weiter; `fat16_seek` setzt Position und
  passenden Cluster-Zeiger neu.
- **Nur lesend**: `create`/`makdir`/`remove` liefern weiterhin `E$UnkSvc` — Schreiben kommt
  in Phase 3.4.

**Wichtiger Seiteneffekt-Fix (3.3)**: der ältere `/d0`-Selbsttest "SS.BlkWr/SS.BlkRd
Roundtrip" (Phase 3.1) schrieb testweise auf LBA 1 und ließ das Testmuster stehen — bei einem
echten FAT16-Image liegt dort typischerweise die erste FAT-Kopie. Der Selbsttest sichert LBA 1
seit 3.3 vor dem Test und stellt ihn danach wieder her, damit er ein zuvor gemountetes FAT16
läuft nicht mehr zerstört.

### Globales Arbeitsverzeichnis (I$ChgDir)

Ein einziger globaler Pfad-String (`Q9_CWD_MAXLEN` = 63 Zeichen, Start: `/`) — **kein**
Arbeitsverzeichnis pro Prozess (das kommt erst mit echten Prozessen in Phase 4, siehe
ARBEITSPLAN.md Entscheidung E8). Relative Pfade (kein führendes `/`) werden bei jedem I$Open
gegen dieses Verzeichnis aufgelöst (`q9_vfs_cwd()`, vfs.c).

## Ausblick

- **Phase 3.4**: FAT16 schreibend — `I$Create`/`I$MakDir`/`I$Delete` bekommen echte Semantik
  (FAT-Ketten allozieren/freigeben, neue 8.3-Einträge; LFN-Schreiben bleibt Ideenspeicher)
- **Phase 6+**: Treiber als echte Q9-Module (Typ 2) statt einkompiliert

**Erstellt**: 2026-07-03
**Zuletzt aktualisiert**: 2026-07-04 (Phase 3.3: FAT16-File-Manager lesend an /d0, q9_fm_t um
read/seek erweitert)
