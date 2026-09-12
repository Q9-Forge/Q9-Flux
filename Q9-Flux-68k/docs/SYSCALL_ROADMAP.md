# Q9 Syscall-Roadmap

Vollständige Liste aller OS-9-Betriebssystemaufrufe (F$/I$) mit Status in Q9.
**Quelle:** MWOS `PACKAGES/OS9_Professional_V3.0/DEFS/funcs.h` + `errno.h`
(dieselbe Quelle wie Entscheidung E7). 97 Calls insgesamt: 80× F$, 17× I$.

**Status-Legende:**

| Symbol | Bedeutung |
|--------|-----------|
| ✅ | implementiert |
| 🟢 Phase N | für diese Phase fest eingeplant |
| 💤 | später/optional, keine feste Phase |
| 🚫 | ignoriert — nicht Teil der Q9-Vision (Design-Entscheidung) |
| ❓ | Design offen — braucht Entscheidung, bevor geplant werden kann |

---

## 1. I/O & Pfade

Kern des Syscall-Interfaces, größtenteils in Phase 1 fertig geworden.

| # | Name | Beschreibung | Status | Notiz |
|---|------|--------------|--------|-------|
| $89 | I$Read | Rohdaten lesen | ✅ | Phase 1.2; seit 3.3 über `fm->read()` (FAT16), wenn vorhanden |
| $8A | I$Write | Rohdaten schreiben | ✅ | Phase 1.2; seit 3.4 über `fm->write()` (FAT16), wenn vorhanden |
| $8B | I$ReadLn | Zeile lesen | ✅ | Phase 1.2 |
| $8C | I$WritLn | Zeile schreiben | ✅ | Phase 1.2 |
| $82 | I$Dup | Pfad duplizieren | ✅ | Phase 1.4 |
| $8F | I$Close | Pfad schließen | ✅ | Phase 1.4 |
| $80 | I$Attach | Gerät per Name anmelden | ✅ | Phase 1.6 |
| $81 | I$Detach | Gerät abmelden | ✅ | Phase 1.6 |
| $8D | I$GetStt | Pfad-Status abfragen | ✅ | Phase 1.8 (SS.Ready, SS.EOF) |
| $8E | I$SetStt | Pfad-Status setzen | ✅ | Phase 1.8 (Grundgerüst, noch keine SS-Codes) |
| $84 | I$Open | Datei öffnen (Pfadname) | ✅ | Phase 3.2 (VFS-Routing); seit 3.3 FAT16 an /d0 |
| $83 | I$Create | Datei anlegen | ✅ | Phase 3.4: FAT16 — neuer 8.3-Dirent, `E$UnkSvc` ohne File-Manager |
| $85 | I$MakDir | Verzeichnis anlegen | ✅ | Phase 3.4: FAT16 — neuer Cluster + `.`/`..`, `E$UnkSvc` ohne File-Manager |
| $86 | I$ChgDir | Arbeitsverzeichnis wechseln | ✅ | Phase 3.2 (EIN globaler String, pro-Prozess erst Phase 4) |
| $87 | I$Delete | Datei löschen | ✅ | Phase 3.4: FAT16 — Cluster-Kette freigeben + Dirent löschen, `E$UnkSvc` ohne File-Manager |
| $88 | I$Seek | Position ändern | ✅ | Phase 3.3: über `fm->seek()` (FAT16), sonst `E$UnkSvc` |
| $92 | I$SGetSt | GetStt über System-Pfadnummer | 🟢 Phase 3/4 | Sonderfall für System-Pfade |

## 2. Modulsystem

Herzstück von Phase 2 (siehe PROJECT.md).

| # | Name | Beschreibung | Status | Notiz |
|---|------|--------------|--------|-------|
| $00 | F$Link | Modul verknüpfen (aus Directory) | ✅ | Phase 2.3d, Directory aus 2.3c |
| $02 | F$UnLink | Modul lösen | ✅ | Phase 2.3d; seit 3.5 gibt sie F$Load-Puffer bei Link=0 frei |
| $1D | F$UnLoad | Modul per Name lösen | 🟢 Phase 2 | Komfortvariante von F$UnLink |
| $01 | F$Load | Modul aus Datei laden | ✅ | Phase 3.5: q9_mod_load, statischer Load-Puffer-Pool (4x4096 Byte) |
| $17 | F$CRC | CRC erzeugen | 🟢 Phase 2 | für Modul-Header, in `q9mod` mitgedacht (PROJECT.md) |
| $26 | F$SetCRC | Modul-Header + CRC setzen | 🟢 Phase 2 | evtl. nur in `q9mod`-Tool statt Kernel-Syscall |
| $2E | F$VModul | Modul validieren (Header/CRC) | 🟢 Phase 2 | Loader-Baustein |
| $4E | F$FModul | Directory-Eintrag suchen | 🟢 Phase 2 | Loader-Baustein |
| $1A | F$GModDr | Kopie des Modul-Directory | 💤 | nur für Debug-Tools relevant |
| $25 | F$DatMod | Datenmodul erzeugen | 💤 | Typ 4 laut Modul-Header, niedrige Prio |
| $27 | F$SetSys | Systemglobale Variable setzen/lesen | 💤 | optional |
| $12 | F$SchBit | Bitmap durchsuchen | ❓ | Q9-Directory evtl. ohne Bitmap (Liste/Array) — Design erst klären |
| $13 | F$AllBit | Bitmap allozieren | ❓ | s.o. |
| $14 | F$DelBit | Bitmap freigeben | ❓ | s.o. |

## 3. Prozesse & Scheduler

Phase 4, kooperativer Scheduler (Entscheidung E4). Viele OS-9-Allocator-Calls
sind reine Interna der 6809/68k-Referenzimplementierung — Q9 baut vermutlich
ein einfacheres Prozessmodell, ohne sie 1:1 als Syscalls nachzubilden.

| # | Name | Beschreibung | Status | Notiz |
|---|------|--------------|--------|-------|
| $03 | F$Fork | Neuen Prozess starten | ✅ | Phase 4.2: nur `Q9_MOD_NATIVE`-Module (Entscheidung E9), sonst `E$NEMod` |
| $04 | F$Wait | Auf Kind-Prozess warten | ✅ | Phase 4.2: `E$NoChld` ($E2, bestätigt) ohne Kinder; Phase 4.3: `E$NotRdy` ohne Zombie-Kind versetzt den Aufrufer echt in WAITING/Q9_WAIT_CHILD statt zu pollen |
| $05 | F$Chain | Prozess mit neuem Modul verketten | ✅ | Phase 4.2 |
| $08 | F$Send | Signal senden | ✅ | Phase 4.5: bricht WAITING/SLEEPING ab, lenkt bei installiertem Handler auf F$Icpt um |
| $09 | F$Icpt | Signal-Intercept setzen | ✅ | Phase 4.5: nur fuer den Aufrufer selbst, ein Handler pro Prozess |
| $1E | F$RTE | Rückkehr aus Intercept | ✅ | Phase 4.5: Gegenstück zu F$Icpt |
| $0A | F$Sleep | Prozess schlafen legen | ✅ | Phase 4.3: SLEEPING/Q9_WAIT_TIMER, `wake_tick` = Tick-Zaehler + Ticks (0 = einmal yielden) |
| $0B | F$SSpd | Prozess suspendieren | ✅ | Phase 4.4: WAITING/Q9_WAIT_SIGNAL, bewusst ohne Weckmechanismus vor F$Send (4.5) |
| $0D | F$SPrior | Priorität setzen | ✅ | Phase 4.4: reines Datenfeld, Scheduler bleibt Round-Robin |
| $0E | F$STrap | Trap-Intercept setzen | 💤 | eher 6809/68k-Trap-Mechanik, niedrige Prio |
| $57 | F$SigMask | Signalmaske setzen | 💤 Phase 4/5 | |
| $63 | F$SigReset | Signal-Intercept-Kontext zurücksetzen | 💤 Phase 4/5 | |
| $2C | F$AProc | In Active-Process-Queue eintragen | 🚫 | Scheduler-Interna, kein Syscall in Q9 |
| $2D | F$NProc | Nächsten Prozess starten | 🚫 | Scheduler-Interna |
| $2F | F$FindPD | Process/Path-Descriptor finden | 🚫 | Q9 nutzt eigenes, einfacheres Prozessmodell |
| $30 | F$AllPD | Process/Path-Descriptor allozieren | 🚫 | s.o. |
| $31 | F$RetPD | Process/Path-Descriptor freigeben | 🚫 | s.o. |
| $4B | F$AllPrc | Process-Descriptor allozieren | 🚫 | s.o. |
| $4C | F$DelPrc | Process-Descriptor freigeben | 🚫 | s.o. |
| $3F | F$AllTsk | Task-Nummer allozieren | 🚫 | 68k-Task-Register-spezifisch, für WASM irrelevant |
| $40 | F$DelTsk | Task-Nummer freigeben | 🚫 | s.o. |
| $18 | F$GPrDsc | Kopie des Process Descriptor | 💤 | nur Debug/Diagnose |
| $37 | F$GProcP | Pointer auf Process-Struktur | 🚫 | Kernel-intern, kein Syscall |
| $32 | F$SSvc | Service-Request-Table initialisieren | 🚫 | Boot-intern |
| $2B | F$IOQu | In I/O-Queue eintragen | 🚫 | Scheduler/IO-Interna |
| $59 | F$UAcct | User-Accounting-Status melden | 🚫 | Multi-User-Konzept, nicht Teil der Q9-Vision |
| $1C | F$SUser | User-ID setzen | 🚫 | s.o. |

## 4. Zeit & Kalender

| # | Name | Beschreibung | Status | Notiz |
|---|------|--------------|--------|-------|
| $15 | F$Time | Uhrzeit lesen | ✅ | Phase 1.9 |
| $16 | F$STime | Uhrzeit stellen | ✅ | Phase 1.9 |
| $20 | F$Julian | Gregorianisch → Julianisch | 💤 | optionaler Kalenderhelfer |
| $54 | F$Gregor | Julianisch → Gregorianisch | 💤 | optionaler Kalenderhelfer |

## 5. Events, Semaphore & IPC

| # | Name | Beschreibung | Status | Notiz |
|---|------|--------------|--------|-------|
| $53 | F$Event | Named Event erstellen/verknüpfen | 💤 Phase 4/5 | IPC-Erweiterung |
| $62 | F$Sema | Semaphore P/V | 💤 Phase 4/5 | IPC-Erweiterung |

## 6. Speicherverwaltung

WASM hat ein lineares Speichermodell ohne OS-9-Blockverwaltung — für das
Browser-Target größtenteils **irrelevant**. Für Vinculum/68k nativ (Phase 7)
potenziell relevant, aber Q9 wird vermutlich ein eigenes, einfacheres Modell
statt einer 1:1-Nachbildung fahren.

| # | Name | Beschreibung | Status | Notiz |
|---|------|--------------|--------|-------|
| $07 | F$Mem | Speichergröße setzen | 🚫 (WASM) / ❓ (Phase 7) | |
| $28 | F$SRqMem | System-Speicher anfordern | 🚫 (WASM) / ❓ (Phase 7) | |
| $29 | F$SRtMem | System-Speicher zurückgeben | 🚫 (WASM) / ❓ (Phase 7) | |
| $39 | F$AllRAM | RAM-Blöcke allozieren | 🚫 (WASM) / ❓ (Phase 7) | |
| $3A | F$Permit | Image-RAM allozieren | 🚫 (WASM) / ❓ (Phase 7) | (alt: F$AllImg) |
| $3B | F$Protect | Image-RAM freigeben | 🚫 (WASM) / ❓ (Phase 7) | (alt: F$DelImg) |
| $1B | F$CpyMem | Externen Speicher kopieren | 🚫 (WASM) / ❓ (Phase 7) | |
| $38 | F$Move | Daten verschieben | 🚫 (WASM) / ❓ (Phase 7) | |
| $19 | F$GBlkMp | Kopie der System-Blockmap | 🚫 (WASM) / 💤 (Phase 7) | nur Diagnose |
| $5C | F$SRqCMem | „Colored Memory" anfordern | 🚫 | 68k-MMU-spezifisch, kein Q9-Ziel |
| $60 | F$Trans | Adresse intern/extern übersetzen | 🚫 | Hardware-Bus-spezifisch |
| $58 | F$ChkMem | Speicherzugriff prüfen | 🚫 (WASM) / 💤 (Phase 7) | Schutzmechanismus, Single-Address-Space bei WASM |
| $5B | F$GSPUMp | SPU-Map-Info | 🚫 | sehr hardwarespezifisch (vermutl. QUICC-Bezug) |

## 7. Hardware, IRQ & Debug

Echte Interrupts gibt es nur auf dem 68k-Target (Vinculum, Phase 7) —
WASM kennt keine Hardware-IRQs.

| # | Name | Beschreibung | Status | Notiz |
|---|------|--------------|--------|-------|
| $2A | F$IRQ | In IRQ-Polling-Table eintragen | 🚫 (WASM) / 🟢 Phase 7 | |
| $61 | F$FIRQ | Fast-IRQ hinzufügen/entfernen | 🚫 (WASM) / 💤 Phase 7 | |
| $5A | F$CCtl | Cache-Control | 🚫 (WASM) / 💤 Phase 7 | |
| $33 | F$IODel | I/O-Modul (Treiber) löschen | 🟢 Phase 2 | eigentlich Modul-Kontext, nicht Hardware |
| $52 | F$SysDbg | System-Debugger aufrufen | 💤 | Phase 8 „Vision" oder ignoriert |
| $22 | F$DFork | Debug-Fork | 💤 | Phase 8, sehr speziell |
| $23 | F$DExec | Debug-Einzelschritt | 💤 | Phase 8 |
| $24 | F$DExit | Debug-Kill-Child | 💤 | Phase 8 |
| $5E | F$Panic | Panic-Warnung | 💤 | evtl. eigener interner Mechanismus statt Syscall |
| $5D | F$POSK | Service-Request ausführen | ❓ | Zweck aus Doku unklar, sehr Boot-intern |
| $0F | F$PErr | Fehler ausgeben | 💤 | Q9-REPL macht das aktuell direkt im Kernel |
| $55 | F$SysID | System-Identifikation liefern | 💤 | netter „sysinfo"-Call für Phase 5 Shell |

## 8. Netzwerk (Phase 8 „Vision")

| # | Name | Beschreibung | Status | Notiz |
|---|------|--------------|--------|-------|
| $5F | F$MBuf | Memory-Buffer-Manager (Netzwerk) | 💤 Phase 8 | passend zu CH9121/WebSocket-Proxy-Idee |
| $21 | F$TLink | Trap-Subroutine-Paket verknüpfen | 🚫 | 6809/68k-Trap-spezifisch, kein Q9-Ziel |

---

## Zusammenfassung

- **23 / 97** implementiert (Phase 1 komplett: alle I$-Kern-Calls + F$Exit/ID/Time/STime/PrsNam/CmpNam;
  Phase 2.3d: F$Link/F$UnLink über die Modul-Directory; Phase 3.2/3.3: I$Open/I$ChgDir/I$Seek über
  die VFS-Schicht + FAT16 lesend; Phase 3.4: I$Create/I$MakDir/I$Delete/I$Write jetzt mit echter
  FAT16-Semantik, kein Gerüst mehr)
- **~24** für konkrete künftige Phasen eingeplant (🟢, meist Phase 3–4)
- **~10** optional/ohne feste Phase (💤)
- **~30** bewusst ignoriert (🚫) — meist OS-9-Interna, die Q9 nicht 1:1 nachbaut, oder WASM-irrelevant
- **5** mit offenem Design (❓) — Bitmap-Frage (Modul-Directory), Speicherverwaltung Phase 7

---

## ✅ Gefundener und behobener Fehler (2026-07-03)

Beim Erstellen dieser Liste in `errno.h` nachgeschaut: **`E_DIFF` in `syscall.h`
hatte den falschen Wert** (`0xE2`, als vorläufig markiert). Laut offizieller
`errno.h` ist `0xE2` tatsächlich **`E_NOCHLD`** ("No Children") — der
Fehlercode, den `F$Wait` liefert, wenn ein Prozess keine Kinder zum Warten
hat. Das hätte in Phase 4 (F$Wait) kollidiert.

Der richtige Code für „Namen unterschiedlich" ist **`E_DIFFER` = `0xA5`**
("Arguments to F$ChkNam are different"). **Behoben** noch am selben Tag:
`E_DIFF` → `E_DIFFER` (`0xa5`) in `syscall.h`, `name.c`, `kernel.c`-Selbsttest
und `docs/SYSCALLS.md` angepasst.

**Erstellt**: 2026-07-03
**Quelle**: `M:\MWOS\PACKAGES\OS9_Professional_V3.0\DEFS\funcs.h` + `errno.h`
