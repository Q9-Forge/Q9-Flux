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
| $15 | F$Time   | ✅ provisorisch (siehe Abweichungen) |
| $89 | I$Read   | ✅ implementiert |
| $8A | I$Write  | ✅ implementiert |
| $8B | I$ReadLn | ✅ implementiert |
| $8C | I$WritLn | ✅ implementiert |
| $84 | I$Open   | geplant (Phase 3, VFS) |
| $8F | I$Close  | geplant (Phase 3) |

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
3. Pfade 0/1/2 (stdin/stdout/stderr) sind fest auf die Konsole verdrahtet, bis das
   Device-Modell (Phase 1.3) und I$Open/I$Close (Phase 3) existieren.

**Erstellt**: 2026-07-03
