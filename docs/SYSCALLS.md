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
| $00 | F$Link   | geplant (Phase 2, Modulsystem) |
| $01 | F$Load   | geplant (Phase 2/3) |
| $02 | F$UnLink | geplant (Phase 2) |
| $03 | F$Fork   | geplant (Phase 4, Prozesse) |
| $04 | F$Wait   | geplant (Phase 4) |
| $06 | F$Exit   | ✅ implementiert (Phase-1-Semantik: hält Proto-Prozess an) |
| $0C | F$ID     | ✅ implementiert (liefert Proto-Prozess-ID 1) |
| $10 | F$PrsNam | ✅ implementiert (Phase 1.5) |
| $11 | F$CmpNam | ✅ implementiert (Phase 1.5, E$Diff-Nummer vorläufig) |
| $15 | F$Time   | ✅ provisorisch (siehe Abweichungen) |
| $80 | I$Attach | ✅ implementiert (Phase 1.6) |
| $81 | I$Detach | ✅ implementiert (Phase 1.6) |
| $89 | I$Read   | ✅ implementiert |
| $8A | I$Write  | ✅ implementiert |
| $8B | I$ReadLn | ✅ implementiert |
| $8C | I$WritLn | ✅ implementiert |
| $82 | I$Dup    | ✅ implementiert (Phase 1.4) |
| $84 | I$Open   | geplant (Phase 3, VFS) |
| $8D | I$GetStt | ✅ Grundgerüst (Phase 1.8: SS.Ready, SS.EOF) |
| $8E | I$SetStt | ✅ Grundgerüst (Phase 1.8: noch keine SS-Codes) |
| $8F | I$Close  | ✅ implementiert (Phase 1.4; Pfadnamen-Open kommt in Phase 3) |

Alle nicht implementierten Nummern liefern `E$UnkSvc` ($D0).

## Fehlercodes (Auszug; identisch zu MWOS errno.h)

| Code | Name      | Bedeutung |
|------|-----------|-----------|
| $C8  | E$PthFul  | Pfadtabelle voll |
| $C9  | E$BPNum   | ungültige Pfadnummer |
| $CB  | E$BMode   | falscher Zugriffsmodus |
| $D0  | E$UnkSvc  | unbekannter Service-Request |
| $D2  | E$BPAddr  | ungültige Parameter-Adresse |
| $D3  | E$EOF     | Dateiende |
| $E1  | E$Param   | ungültiger Parameter |
| $F6  | E$NotRdy  | Gerät nicht bereit |

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

- Gleich → 0; verschieden → `E$Diff` ($E2, **vorläufige Nummer** — OS-9 setzt
  nur Carry; beim MWOS-Abgleich prüfen). Wildcards: noch keine.

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

- Unbekannte Codes bzw. Treiber ohne getstat/setstat-Op → `E$UnkSvc`.
- I$SetStt hat noch keine Codes (SS.Opt für Pfadoptionen kommt später).

### F$Exit ($06)

| Register | Input       |
|----------|-------------|
| d1.w     | Status-Code |

### F$ID ($0C)

| Register | Output                  |
|----------|-------------------------|
| d0.w     | Prozess-ID              |
| d1.l     | User-ID (Phase 1: 0)    |

### F$Time ($15) — provisorisch

| Register | Output                              |
|----------|-------------------------------------|
| d0.l     | Sekunden seit Boot (bis RTC kommt)  |
| d3.l     | Millisekunden-Ticks seit Boot       |

---

## Bewusste Abweichungen von OS-9 (Phase-1-Stand)

1. **Kein Blockieren**: Bis der Scheduler existiert (Phase 4), liefern I$Read/I$ReadLn
   `E$NotRdy`, wenn keine (vollständige) Eingabe ansteht — der Aufrufer pollt.
   Ab Phase 4 blockiert der aufrufende Prozess, wie es sich gehört.
2. **F$Time** liefert Uptime statt Uhrzeit, bis eine RTC-Quelle da ist (HAL-Erweiterung).
3. ~~Pfade 0/1/2 fest verdrahtet~~ — seit Phase 1.3 laufen alle Pfade über das
   Device-Modell (Pfadtabelle → Treiber-Modul, siehe docs/DEVICES.md). Die
   Standardpfade 0/1/2 öffnet der Kernel beim Boot auf /term (Update-Modus).
   User-seitiges I$Open/I$Close mit Pfadnamen kommt in Phase 3; falscher
   Zugriffsmodus liefert bereits `E$BMode`.

**Erstellt**: 2026-07-03
**Letzte Aktualisierung**: 2026-07-03 (Phase 1.3: Device-Modell)
