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

## Ausblick

- **Phase 1.x**: I$GetStt/I$SetStt, Geräte per Name über I$Attach/I$Detach
- **Phase 2**: Treiber als echte Q9-Module (Typ 2) statt einkompiliert
- **Phase 3**: I$Open/I$Close mit Pfadnamen (`/term`), RBF-artige Block-Devices über
  die HAL-Block-API

**Erstellt**: 2026-07-03
