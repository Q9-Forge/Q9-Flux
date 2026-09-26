//════════════════════════════════════════════════════════════════════════════════════════════════
// File:   q9_dhf.h                                                                        Ver. 2.00
// Owner:  Claude
// Desc.:  DHF (Direct Host Filesystem) -- duenner devreg-Adapter um das bereits vorhandene
//         Zero-Copy-Geraet aus Q9-OS/Q9-DHFDRV-68k/emulator/ (dhf_emu_device.c/.h, dhf_host_fs.c/.h,
//         hierher kopiert, s. deren Kopfkommentare). Ersetzt die eigene Byte-Kopier-Fassung von
//         Ver. 1.00 (2026-09-25) -- Andreas' Hinweis: "der Treiber muss ja nicht viel machen, im
//         Prinzip die Daten nur an die Hardwareschnittstelle weiter geben und wieder zurueck",
//         genau das leistet dieses Protokoll bereits (A0/A1 = 68k-Zeiger auf Pfad/Puffer im
//         Gast-RAM, D0-D2 = Handle/Laenge/Flags, s. dhf_proto.h/dhf_shared.h) UND es gibt schon
//         einen dazu passenden 68k-Treiber (Q9-OS/Q9-DHFDRV-68k/driver/dhfdrv.c).
//
//         Registerfenster = struct dhf_shared (dhf_shared.h), 28 Byte, big-endian, ueber
//         dhf_emu_device_read8/write8/... byteweise/wortweise ansprechbar (bereits "devreg-
//         Vtable-kompatibel" geschrieben, s. dhf_emu_device.h Kopfkommentar).
//
// Edition History
//─────────┬──────┬────────────────────────────────────────────────────────────────────────┬──────
// Date    │ Ver. │ Description                                                            │ By
//─────────┼──────┼────────────────────────────────────────────────────────────────────────┬──────
// 26-09-25│ 1.00 │ Erster Wurf, eigenes Byte-Kopier-Protokoll                              │ Cld
// 26-09-25│ 2.00 │ Ersetzt durch Adapter um Andreas' bereits vorhandenes Zero-Copy-Geraet   │ Cld
//         │      │ (Q9-DHFDRV-68k) -- kompatibel mit dessen fertigem 68k-Treiber            │
//═════════╧══════╧════════════════════════════════════════════════════════════════════════╧══════
#ifndef Q9_DHF_H
#define Q9_DHF_H

#include "dhf_emu_device.h"
#include "../../kernel/devdesc.h"

/* Freie Luecke zwischen nettty ($FFFF1000-17FF) und QUICC ($FFFF2000-3FFF) einerseits und REMAP
   ($FFFF8000) andererseits: $FFFF4000-7FFF, 16K frei. NICHT DHF_DEFAULT_HW_BASE (dhf_proto.h)
   verwenden -- das kollidiert mit Q9_BOARD_RTC_BASE ($FFFFD000)! Basepath/Base sind laut
   dhf_emu_device_t sowieso Instanz-Felder, kein Compile-Zeit-Zwang. */
#define Q9_BOARD_DHF_BASE          0xFFFF4000u
#define Q9_BOARD_DHF_TOP           (Q9_BOARD_DHF_BASE + (uint32_t)sizeof(struct dhf_shared) - 1u)
/* 2026-09-26: zweite, voellig unabhaengige Instanz (eigener Basispfad, eigene Handle-Tabelle)
   fuer ein zweites DHF-Laufwerk (Deskriptor d1, Q9-OS/Q9-DHF-68k/descriptor/d1_dhf.a) -- wie
   ein zweiter Controller. Eigener 256-Byte-Slot der Fast-Table (m68krt.c Q9_IO_SLOT_SHIFT). */
#define Q9_BOARD_DHF1_BASE         0xFFFF4100u
#define Q9_BOARD_DHF1_TOP          (Q9_BOARD_DHF1_BASE + (uint32_t)sizeof(struct dhf_shared) - 1u)

typedef struct {
    struct dhf_shared shared;      /* das eigentliche Registerfenster, s. dhf_shared.h */
    dhf_emu_device_t  dev;         /* Handle-Tabelle/Basepath/RAM-Zeiger, s. dhf_emu_device.h */
} q9_dhf_t;

extern const q9_device_vtable_t q9_devtype_dhf;
extern const q9_devdesc_t       q9_devdesc_dhf;

/* m68krt.c ruft dies beim Attach auf (vor q9_devreg_add) -- ram/ram_len kommen aus q9_board_t,
   damit dhf_emu_device_process() Gastadressen (A0/A1) direkt indizieren kann (board->ram[a0] usw.,
   dasselbe Prinzip wie board_read_byte in q9board.c). */
void q9_dhf_init(q9_dhf_t *state, const char *basepath, uint8_t *ram, size_t ram_len);

#endif /* Q9_DHF_H */
//────────────────────────────────────────────────────────────────────────────────────────────────
// EOF q9_dhf.h                                                                            Ver. 2.00
//────────────────────────────────────────────────────────────────────────────────────────────────
