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
} q9_fm_t;
```

- `restpath` ist der Teil hinter dem Gerätenamen, **ohne** führenden `/` (leer = Gerätewurzel).
- Ein Gerät bekommt seinen File-Manager über `q9_dev_set_fm(dev, &fm)` (`q9_dev_t.fm`,
  `NULL` = kein Dateisystem — Grundzustand aller Geräte, auch `/d0` vor Phase 3.3).
- **Schnitt bewusst schmal gehalten** (Rest-Pfad-String statt OS-9-Pfaddeskriptor-Internas),
  damit später auch ein 68k-Manager-Adapter (Ideenspeicher, ARBEITSPLAN.md) dieselbe
  C-Schnittstelle hinter einer Trap-Bridge bedienen kann — mit dem Dibble-Buch
  ("OS-9 Insights") als Vorbild abgeglichen, aber eigene C99-Implementierung (kein Codeklau).
- `create`/`makdir`/`remove` sind für Phase 3.2 nur Teil der Schnittstelle — echte
  FAT16-Implementierung folgt in 3.4 (schreibend). Der Kernel-Syscall-Dispatcher (syscall.c)
  liefert `I$Create`/`I$MakDir`/`I$Delete` bis dahin direkt `E$UnkSvc`.

### Datei-Kontext pro Pfad (`q9_path_t.fmctx`, device.h)

Jeder offene Pfad hat ein festes Byte-Array (`Q9_FMCTX_SIZE` = 16 Byte) für File-Manager-
eigenen Zustand (z.B. aktuelle Position/Cluster ab 3.3) — **kein malloc**, der Kontext liegt
direkt in der (statischen) Pfadtabelle. Der Kernel fasst das Feld nie an, nur der File-Manager
hinter dem jeweiligen Gerät interpretiert es. Für Geräte ohne File-Manager unbenutzt (bleibt 0).

### Globales Arbeitsverzeichnis (I$ChgDir)

Ein einziger globaler Pfad-String (`Q9_CWD_MAXLEN` = 63 Zeichen, Start: `/`) — **kein**
Arbeitsverzeichnis pro Prozess (das kommt erst mit echten Prozessen in Phase 4, siehe
ARBEITSPLAN.md Entscheidung E8). Relative Pfade (kein führendes `/`) werden bei jedem I$Open
gegen dieses Verzeichnis aufgelöst (`q9_vfs_cwd()`, vfs.c).

## Ausblick

- **Phase 3.3**: erster echter File-Manager (FAT16 lesend) an `/d0` — nutzt die VFS-Schicht
  aus 3.2, `q9_fm_t.open` bekommt echten Inhalt (Boot-Sektor/Root-Dir/Cluster-Ketten)
- **Phase 6+**: Treiber als echte Q9-Module (Typ 2) statt einkompiliert

**Erstellt**: 2026-07-03
**Zuletzt aktualisiert**: 2026-07-04 (Phase 3.2: VFS-Schicht/File-Manager-Interface ergänzt)
