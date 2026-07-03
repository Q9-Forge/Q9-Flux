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

- **Gerätetabelle** (`Q9_NDEVS` = 4): Name, Treiber-Ops (`q9_drv_t`), Static Storage
  (vom Treiber-`init` angehängt), Link-Count.
- **Pfadtabelle** (`Q9_NPATHS` = 8): Gerät + Zugriffsmodus. `q9_path_open` vergibt die
  niedrigste freie Pfadnummer (wie OS-9).
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

## Ausblick

- **Phase 3.2**: VFS-Schicht (Pfad-Routing `/d0/pfad/datei`) über `/d0` als erstes Gerät
  mit echtem Dateisystem dahinter (FAT16, 3.3/3.4)
- **Phase 6+**: Treiber als echte Q9-Module (Typ 2) statt einkompiliert

**Erstellt**: 2026-07-03
**Zuletzt aktualisiert**: 2026-07-03 (Phase 3.1: /d0 ergänzt)
