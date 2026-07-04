//════════════════════════════════════════════════════════════════════════════════════════════════
// File:   cb030.h                                                                         Ver. 1.00
// Owner:  AF
// Desc.:  CB030-Board-Emulation (Schritt 5.2, docs/CB030.md) — Bootstrap/Validierungs-Zwischenschritt
//         fuer die Musashi-Integration (5.1) mit dem originalen, proprietaeren Microware-OS-9-Boot-
//         ROM. Schritt 5.2a: NUR die RAM/ROM/Remap-Speicherlogik (Adress-Dispatch als if/else-Kette,
//         RAM zuerst geprueft, s. docs/CB030.md "Emulations-Architektur") + der REMAP-Zustand als
//         eigener Merker (unabhaengig von Musashis CPU-Zustand). Peripherie (UART/CF/Timer, 5.2b-d)
//         ist hier bewusst noch NICHT angebunden (I/O-Bereich liefert 0 bzw. verwirft Schreibzugriffe
//         kommentarlos) und die Anbindung an Musashis m68k_read/write_memory_*-Hooks (m68krt.c) ist
//         ebenfalls noch offen — dieser Schritt liefert nur den reinen Adress-Dekoder + Selbsttest.
//
// Call:   q9_cb030_t b; q9_cb030_init(&b, rom, rom_len, ram, ram_len);
//         v = q9_cb030_read8(&b, addr); q9_cb030_write8(&b, addr, v); q9_cb030_reset(&b);
//
// Edition History
//─────────┬──────┬────────────────────────────────────────────────────────────────────────┬──────
// Date    │ Ver. │ Description                                                            │ By
//─────────┼──────┼────────────────────────────────────────────────────────────────────────┼──────
// 26-07-04│ 1.00 │ 5.2a: Erster Grundbaustein — RAM/ROM/Remap-Adress-Dispatch              │ CF
//═════════╧══════╧════════════════════════════════════════════════════════════════════════╧══════
#ifndef Q9_CB030_H
#define Q9_CB030_H

#include <stdint.h>

#define Q9_CB030_OK          0
#define Q9_CB030_ERR_RAM    -1                       /* RAM fehlt */

/* Adress-Konstanten aus docs/CB030.md ("Speicherkarte"/"Emulations-Architektur"). */
#define Q9_CB030_ROM_MIRROR_LIMIT  0x08000000u        /* Reset-Zustand: ROM gespiegelt bis hier */
#define Q9_CB030_ROM_REMAP_BASE    0xFE000000u        /* Remap-Zustand: ROM liegt einmal hier   */
#define Q9_CB030_ROM_REMAP_TOP     0xFE07FFFFu
#define Q9_CB030_REMAP_REG_BASE    0xFFFF8000u        /* REMAP-Register: reiner Adress-Trigger  */
#define Q9_CB030_REMAP_REG_TOP     0xFFFF8FFFu

typedef struct q9_cb030 {
    const uint8_t *rom;                               /* Boot-ROM-Inhalt, nur lesend            */
    uint32_t       rom_len;
    uint8_t       *ram;                                /* Emuliertes RAM (Groesse = SIM-Bestueckung) */
    uint32_t       ram_len;
    int            remapped;                           /* 0 = Reset-Zustand, 1 = nach REMAP-Trigger */
} q9_cb030_t;

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_cb030_init
// Desc.:    Bindet ROM- und RAM-Puffer an ein Board-Handle, Ausgangszustand = Reset (nicht remapped).
//           rom darf NULL/0 sein (z.B. fuer reine RAM-Tests ohne Boot-ROM). ram darf nicht NULL sein.
// Call:     err = q9_cb030_init(&b, rom, rom_len, ram, ram_len)
//════════════════════════════════════════════════════════════════════════════════════════════════
int q9_cb030_init(q9_cb030_t *b, const uint8_t *rom, uint32_t rom_len, uint8_t *ram, uint32_t ram_len);

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_cb030_reset
// Desc.:    Setzt den REMAP-Zustand auf "nicht remapped" zurueck (Reset-Zustand: ROM bei Adresse 0).
// Call:     q9_cb030_reset(&b)
//════════════════════════════════════════════════════════════════════════════════════════════════
void q9_cb030_reset(q9_cb030_t *b);

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_cb030_read8/16/32
// Desc.:    Liest ein Byte/Word/Long-Word aus der Board-Adresse 'addr'. Dispatch je nach REMAP-
//           Zustand (s. docs/CB030.md): Reset = ROM gespiegelt bis Q9_CB030_ROM_MIRROR_LIMIT;
//           remapped = RAM ab 0, ROM einmalig bei Q9_CB030_ROM_REMAP_BASE. Ein Zugriff auf den
//           REMAP-Registerbereich schaltet IMMER (unabhaengig vom bisherigen Zustand) auf remapped
//           um, bevor der eigentliche Lesewert (0) ermittelt wird. Peripherie-Bereiche (UART/CF/
//           Timer) liefern hier noch 0 (5.2b-d). 16/32-Bit sind big-endian (68k-Byteorder), wie
//           m68krt.c.
// Call:     v = q9_cb030_read8(&b, addr)
//════════════════════════════════════════════════════════════════════════════════════════════════
uint8_t  q9_cb030_read8(q9_cb030_t *b, uint32_t addr);
uint16_t q9_cb030_read16(q9_cb030_t *b, uint32_t addr);
uint32_t q9_cb030_read32(q9_cb030_t *b, uint32_t addr);

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_cb030_write8/16/32
// Desc.:    Schreibt ein Byte/Word/Long-Word auf die Board-Adresse 'addr'. Schreibzugriffe auf ROM
//           (in beiden REMAP-Zustaenden) werden verworfen. Ein Zugriff auf den REMAP-Registerbereich
//           schaltet ebenfalls um (Wert wird verworfen, reiner Adress-Trigger, s. docs/CB030.md).
//           Peripherie-Bereiche verwerfen den Wert kommentarlos (5.2b-d).
// Call:     q9_cb030_write8(&b, addr, val)
//════════════════════════════════════════════════════════════════════════════════════════════════
void q9_cb030_write8(q9_cb030_t *b, uint32_t addr, uint8_t val);
void q9_cb030_write16(q9_cb030_t *b, uint32_t addr, uint16_t val);
void q9_cb030_write32(q9_cb030_t *b, uint32_t addr, uint32_t val);

#endif // Q9_CB030_H

//────────────────────────────────────────────────────────────────────────────────────────────────
// EOF cb030.h                                                                             Ver. 1.00
//────────────────────────────────────────────────────────────────────────────────────────────────
