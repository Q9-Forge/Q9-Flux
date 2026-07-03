# OS-9-Modulsystem — Referenz für Q9 Phase 2

Zusammenfassung des echten OS-9/68K-Modulsystems (Header-Felder + Verwaltung),
als Entscheidungsgrundlage für Q9s eigenes Modulsystem (Phase 2). Q9 bleibt
dabei **konzepttreu, aber nicht binärkompatibel** (Entscheidung E2 in
PROJECT.md) — hier steht, was OS-9 macht, nicht was Q9 übernehmen muss.

**Quellen:**
- `M:\MWOS\PACKAGES\OS9_Professional_V3.0\DEFS\module.h` (C-Structs)
- `M:\MWOS\PACKAGES\OS9_Professional_V3.0\DEFS\oskdefs.d` (Assembler-Konstanten, vollständigere Language-Liste)
- `M:\MWOS\DOC\MANUALS\MW 0000-0000 OS-9 for 68K Technical Reference Manual.pdf` (Kapitel „System Overview" + „System Calls", F$Link/F$UnLink/F$VModul/F$FModul/F$Load)

---

## 1. Der gemeinsame Header (`modhcom`)

Jedes OS-9-Modul beginnt mit diesem gemeinsamen Kopf (Feldreihenfolge aus
`module.h`; Offsets **abgeleitet** aus der Feldreihenfolge/-größe, nicht
direkt aus einer Offset-Tabelle zitiert):

| Offset (abgeleitet) | Größe | Feld | Bedeutung |
|----|---|------|-----------|
| $00 | 2 | `_msync` | Sync-Bytes `$4AFC` |
| $02 | 2 | `_msysrev` | System-Revision-Prüfwert (Kompatibilität) |
| $04 | 4 | `_msize` | Modulgröße gesamt |
| $08 | 4 | `_mowner` | Owner-User-ID (selten genutzt) |
| $0C | 4 | `_mname` | Offset zum Modulnamen (nullterminiert) |
| $10 | 2 | `_maccess` | Zugriffsrechte (wie Datei-Permissions: Owner/Group/World × R/W/X) |
| $12 | 2 | `_mtylan` | **Type/Language-Wort**: `(Type<<8)\|Language` |
| $14 | 2 | `_mattrev` | **Attribut/Revision-Wort**: `(Attr<<8)\|Revision` |
| $16 | 2 | `_medit` | Edition |
| $18 | 4 | `_musage` | Offset zu einem Kommentar-String (optional) |
| $1C | 4 | `_msymbol` | Offset zur Symboltabelle (optional, Debug) |
| $20 | 2 | `_mident` | „Ident code" — Zweck unklar, evtl. Tool-intern |
| $22 | 6 | `_mspare[6]` | reserviert |
| $28 | 4 | `_mhdext` | Offset zu einer **Header-Extension** (erweiterbar, ohne alte Tools zu brechen) |
| $2C | 2 | `_mhdextsz` | Größe der Header-Extension |
| $2E | 2 | `_mparity` | **Header-Parity** — schneller Check NUR des Headers (separat von der CRC über das gesamte Modul) |

Danach folgen je nach Modultyp weitere Felder — siehe Abschnitt 3.

## 2. Type- und Language-Werte

Aus `oskdefs.d` (Assembler-Referenz, vollständiger als der C-Header):

**Type** (`_mtylan` oberes Byte — was das Modul *ist*):

| Wert | Name | Bedeutung |
|------|------|-----------|
| 1 | Prgrm | Programm-Modul |
| 2 | Sbrtn | Unterprogramm-Modul (Trap-Library) |
| 3 | Multi | Multi-Modul |
| 4 | **Data** | **Datenmodul** |
| 5 | CSDData | Configuration Status Descriptor |
| 11 | TrapLib | Trap-Handler-Library |
| 12 | Systm | System |
| 13 | FlMgr | File Manager |
| 14 | Drivr | Device Driver |
| 15 | Devic | Device Descriptor |

**Language** (`_mtylan` unteres Byte — womit es ausführbar ist):

| Wert | Name | Bedeutung |
|------|------|-----------|
| 1 | Objct | Objekt-Code (natives 68k-Maschinencode — OS-9/68K kennt nur EINE CPU-Familie, kein Slot für x86/MIPS/ColdFire!) |
| 2 | ICode | Basic09-I-Code |
| 3 | PCode | Pascal-P-Code |
| 4 | CCode | C-I-Code |
| 5 | CblCode | Cobol-I-Code |
| 6 | FrtnCode | Fortran-I-Code |

→ **Für Q9 relevant:** Type+Language als zwei getrennte Bytes ist strukturell
identisch übernommen (siehe `PROJECT.md`). Die *Werte* sind zwangsläufig
eigene, da OS-9/68K für "mehrere Zielarchitekturen nebeneinander" (Q9s
WASM+68k-Doppelziel) gar kein Konzept hatte — Multi-Architektur kam laut
Recherche erst mit OS-9000 (nicht in `M:\MWOS` vorhanden, nicht verifizierbar).

**Attribute** (`_mattrev` oberes Byte):

| Bit | Name | Bedeutung |
|-----|------|-----------|
| 7 ($80) | ReEnt | reentrant (mehrere Prozesse können gleichzeitig linken) |
| 6 ($40) | Ghost / „sticky" | bleibt im Speicher, auch wenn Link-Count 0 erreicht (siehe unten) — **Achtung**: `oskdefs.d` nennt das Bit „Ghost", das Technical Reference Manual nennt dasselbe Bit im Fließtext „sticky module" — zwei Namen, ein Bit, keine zwei Konzepte |
| 5 ($20) | SupStat | muss im Supervisor-State laufen |

## 3. Typ-spezifische Erweiterungen (nach dem gemeinsamen Header)

| Modultyp | Zusätzliche Felder |
|----------|---------------------|
| **Programm/Data/TrapLib** (`mod_exec`) | Exec-Offset, Exception-Offset, Datengröße, Stackgröße, Init-Data-Offset, Data-Ref-Offset |
| **Device Driver** (`mod_driver`) | Exec-, Exception-Offset, Datengröße, dazu **sechs** Routine-Offsets: `Init/Read/Write/GetStat/SetStat/Term/Error` |
| **Device Descriptor** (`mod_dev`) | Port-Adresse, IRQ-Vektor/Level/Priorität, Mode-Fähigkeiten, **Name des File Managers**, **Name des Treibers**, DevCon-Offset (treiberspezifische Konfig, z.B. Baudrate — riesige Tabelle in `moded.fields`), Device-Type-Code |
| **Config/Init-Modul** (`mod_config`) | Boot-Parameter: Tabellengrößen (Prozesse/Pfade/Module), Name des ersten auszuführenden Moduls, Default-Device, Konsolenname, Clock-Modul, CPU-Typ, OS-Level ... |

**Wichtige Architektur-Erkenntnis:** OS-9 trennt bei Geräten **drei** Rollen,
die bei Q9 aktuell in einem einzigen `q9_dev_t` zusammengefasst sind:
1. **File Manager** (generische I/O-Semantik, z.B. SCF=Terminal, RBF=Disk)
2. **Device Driver** (hardwarenahe Low-Level-Routinen)
3. **Device Descriptor** (benannte Konfig-Instanz, die File-Manager + Treiber
   + Parameter wie Baudrate zusammenbindet — mehrere Descriptors können
   denselben Treiber mit unterschiedlicher Konfiguration nutzen)

Q9s `dev_term.c`/`dev_nil.c` sind aktuell Treiber+Konfig in einem — passt für
Phase 1/2, aber falls später mehrere Instanzen desselben Treibers mit
unterschiedlichen Parametern gebraucht werden (z.B. zwei serielle Ports),
lohnt sich die Dreiteilung. **Offene Design-Frage für heute Abend.**

Auch interessant: OS-9-Treiber haben **Term** und **Error**-Routinen, die
Q9s `q9_drv_t` (aktuell: init/read/write/readln/writln/getstat/setstat) noch
fehlen — relevant, sobald Module aus- und wieder eingehängt werden (Phase 2.4).

## 4. Die Modul-Directory (Kernel-interne Verwaltungstabelle)

Das ist die "Modulliste, die auf die Module verweist" — im Kernel als
`mod_dir`-Struct:

```c
typedef struct mod_dir {
    struct modhcom *md_mptr;    /* Zeiger aufs Modul                    */
    struct modhcom *md_group;   /* Zeiger auf die Modul-Gruppe          */
    unsigned long   md_static;  /* Speichergröße der Modul-Gruppe       */
    unsigned short  md_link;    /* Link-Count                           */
    unsigned short  md_mchk;    /* Header-Checksumme                    */
} mod_dir;
```

Genau das Pendant zu Q9s künftigem Modul-Directory (Phase 2.3, ARBEITSPLAN).

### Link-Counting & Lebenszyklus

- **F$Link**: sucht die Directory nach Name **+ Type + Language** (nicht nur
  Name!). Gefunden → Link-Count +1, liefert Header-Zeiger (a2) und
  Exec-Einsprung (a1). Fehlt Leserecht oder ist der Header beschädigt → Fehler.
- **F$UnLink** / **F$UnLoad**: Link-Count −1. Bei 0 → Modul wird aus Directory
  entfernt und Speicher freigegeben — **außer** das Modul ist „sticky".
- **Modul-Gruppen**: Werden mehrere Module zusammen geladen (eine Datei mit
  mehreren Modulen, `F$Load`), bilden sie eine Gruppe (`md_group`). Sie werden
  **erst entfernt, wenn ALLE Module der Gruppe Link-Count 0 haben** —
  Einzelmodule können nicht separat aus einer Gruppe verschwinden.
- **Sticky/Ghost-Module** (Attribut-Bit 6): bleiben auch bei Link-Count 0 im
  Speicher, bis der Count auf **−1** fällt oder der Speicher anderweitig
  gebraucht wird. Praktisch für häufig genutzte Module (Shell, Bibliotheken).
- **F$FModul**: reine Suche ohne Linken (kein Link-Count-Increment) — liefert
  nur den Directory-Eintrag. Nützlich für Inspektion/Tools.
- **F$Load**: liest **eine oder mehrere** Module aus einer Datei, bis EOF/Fehler,
  linkt automatisch das erste. → **Direktes Vorbild für Q9s „ROM-Image"-Idee**
  (Phase 2.3: „Module aus eingebautem ROM-Image").

### Namenskollision & Revision-Regel

Beim Eintragen eines neuen Moduls (`F$VModul`) wird die Directory zuerst nach
einem Modul **gleichen Namens und Typs** durchsucht:
- Gibt es eins → das mit der **höheren Revision** bleibt.
- Bei Gleichstand gewinnt das **bereits etablierte** Modul (Bestandsschutz).

→ Relevant für Q9s F$Link-Implementierung (Phase 2.3): diese Tie-Break-Regel
sauber übernehmen, sonst ist "Modul neu laden/updaten" unvorhersehbar.

### Boot-Zeit-Scan (ROM-Module)

Beim Systemstart durchsucht der Kernel den ROM-Bereich nach dem Sync-Code
(`$4AFC`). Gefunden → **zweistufige Prüfung**: erst die schnelle
Header-Parity (`_mparity`, nur der Header), dann — falls die besteht — die
vollständige 24-Bit-CRC über das ganze Modul. Erst danach kommt es in die
Directory. **Zwei getrennte Prüfstufen** (billig vor teuer) ist ein Muster,
das Q9 fürs Laden aus dem ROM-Image übernehmen könnte, aber nicht muss —
bei Q9s Modulgrößen (WASM, klein) ist der Performance-Vorteil fraglich.

---

## Zusammenfassung: was für Q9 wichtig ist

| Konzept | Für Q9 relevant? |
|---------|-------------------|
| Type/Language als getrennte Bytes | ✅ schon übernommen (PROJECT.md) |
| Modul-Directory mit Link-Count | ✅ Phase 2.3, direkt vorgesehen |
| Namenskollision → höhere Revision gewinnt | ✅ sollte übernommen werden (Phase 2.3) |
| `F$Load` liest mehrere Module aus einer Datei | ✅ passt zur „ROM-Image"-Idee (Phase 2.3) |
| Sticky/Ghost-Module (Bit 6) | 💤 nice-to-have, keine Eile |
| Modul-Gruppen (gemeinsames Unlink) | ❓ Design-Frage — braucht Q9 das? |
| 3-Wege-Split File-Manager/Treiber/Descriptor | ❓ Design-Frage — heute Abend besprechen |
| Header-Parity als schneller Vorab-Check | 🚫 wahrscheinlich unnötig (Q9-Module sind klein) |
| `_musage`/`_msymbol` (Kommentar/Symboltabelle) | 🚫 optional, niedrige Priorität |
| Header-Extension-Mechanismus (`_mhdext`) | 💤 evtl. als Zukunftssicherung übernehmen |
| Access-Permissions (Owner/Group/World) | 🚫 Q9 ist aktuell Single-User |

**Erstellt**: 2026-07-03
