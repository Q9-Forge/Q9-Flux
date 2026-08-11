# Ethernet-Recherche für das Q9-Board (und Vinculum)

Stand: 2026-07-07 (aus dem CF-Debugging-Arbeitsstand gerettet, 2026-07-09).

Ziel: Wenn möglich eine reale, historisch nachbaubare Ethernet-Hardware
verwenden, statt nur einen frei erfundenen virtuellen Adapter. Emulator
zuerst, später ggf. echte Hardware.

## Variante 1: LANCE / Am7990 — bester Kandidat für das Q9-Board

Im MWOS-SDK ist ein echter OS-9/SPF-Treiber vorhanden; das ist aktuell der
beste Kandidat für Q9-Board-Ethernet.

Gefundene Module/Quellen (alle lokal, MWOS ist proprietär):

- Treiberquelle: `/Volumes/SSD1TB/projects/MWOS/SRC/DPIO/SPF/DRVR/SPLANCE`
- Wichtige Header: `am7990.h`, `defs.h`
- Fertige MVME147-Module: `sp147`, `sple0`, `sple1`
  unter `/Volumes/SSD1TB/projects/MWOS/OS9/68030/PORTS/MVME147/...`

Descriptor-/Hardwaredaten aus `sple0`/`sple1`:

| Feld | Wert |
|------|------|
| Treibername | `sp147` |
| Basisadresse | `0xFFFE1800` |
| IRQ-Level | 5 |
| Vektor | 68 |
| RX/TX-Ringe | `sple0`: 16/16, `sple1`: 4/4 |
| Geschwindigkeit | 10 MBit |

Hardware-Einschätzung (Andreas, 2026-07-07):

- AM7990PC im DIL48 auf eBay gut verfügbar, ab ca. 2,50 EUR.
- AM7992 (SIA/ENDEC) ebenfalls verfügbar, teurer als der Netzwerkchip, aber
  bezahlbar. AM7990 braucht typischerweise AM7992 plus Transceiver/AUI/
  10BASE-T-Teil.
- Ceramic-DIL-Angebote sind eher teuer und für uns nicht nötig.

Warum interessant: Treiber vorhanden, Chip klassisch und nachbaubar, LANCE
ist gut dokumentiert und auch in anderen Emulatoren/BSD/Linux-Treibern
bekannt.

Offene Emulator-Aufgaben:

- LANCE-Registermodell an passender Board-Adresse emulieren.
- CSR0-3, RAP/RDP, Init-Block, RX/TX-Descriptor-Ringe implementieren.
- IRQ-Level/Vektor für LANCE ergänzen.
- Host-Anbindung zuerst einfach halten: Frame-Logger, UDP-Tunnel, später
  ggf. TAP/BPF/raw.

## Variante 2: MC68360 / QUICC — Testpfad für Vinculum

Für 68360/QUICC ist ebenfalls ein kompletter SPF-Ethernet-Treiber vorhanden.
Für Q9-Board-Ethernet nicht der direkte Weg (der Treiber ist für Systeme
gedacht, bei denen der 68360 selbst Systemprozessor/On-Chip-Peripheriezentrum
ist) — für Vinculum aber sehr wertvoll als vorhandener Ethernet-Testpfad.

Gefundene Module/Quellen:

- Treiberquelle: `/Volumes/SSD1TB/projects/MWOS/SRC/DPIO/SPF/DRVR/SPQUICC`
  (`main.c`, `init.c`, `isr.c`, `entry.c`, `term.c`, `pins.c`, `defs.h`,
  `qedvr.h`, `quicc.h`, `regs360.h`, `pram360.h`, `enet360.h`)
- Port: `/Volumes/SSD1TB/projects/MWOS/OS9/CPU32/PORTS/QUADS/SPF/SPQUICC`
- Fertige Module: `sp360`, `spqe0`

Descriptordaten für `spqe0`:

| Feld | Wert |
|------|------|
| Treibername | `sp360` |
| Basisadresse | `0x22C00` |
| Vektor | 254 |
| IRQ-Level | 5 |
| RX/TX-Ringe | 4/4 |
| Geschwindigkeit | 10 MBit |

Zugehörige 68360-Systemmodule/Definitionen: `sim360.d` (SIM/MBAR/SCC-Basen/
CPM-Interruptregister), `ser360.d` (seriell), `timm360.d` (Timer),
`mc68360defs` (zieht die drei zusammen), `sc68360` (serieller Treiber),
`tk68360` (Clock/Timer).

## Weitere Chips / Treiberlage

- **Intel i82596**: Treiber vorhanden für MVME162/167/172/177 (`sp162`,
  `sp167`, `sp172`, `sp177`, Descriptor `spie0`), 10 MBit — zu komplex für
  unser erstes Ethernet-Ziel.
- **RTL8019AS / NE2000**: Hardware bei Andreas vorhanden, hardwareseitig
  attraktiv — aber im lokalen SDK kein echter `spne2000`/`spne0`-Treiber
  gefunden (nur Hinweis in `SRC/SYS/loadspf`).
- **CS8900 / LAN911x / LAN91Cxxx**: lokal keine brauchbaren Treiberquellen
  oder fertigen Module gefunden; `loadspf` nennt nur generisch SMC 91C94.

## Empfehlung

- Für das Q9-Board: zuerst LANCE/AM7990 emulieren und mit `sp147` + `sple0` testen.
- Für Vinculum: 68360/QUICC-Pfad im Auge behalten (`sp360`/`spqe0` plus
  Systemmodule vorhanden).
- NE2000/RTL8019AS bleibt hardwareseitig interessant, aber ohne
  OS-9-Treiber erstmal zweite Priorität.
