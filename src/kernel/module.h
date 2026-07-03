//════════════════════════════════════════════════════════════════════════════════════════════════
// File:   module.h                                                                        Ver. 1.00
// Owner:  AF
// Desc.:  Q9-Modul-Header (Phase 2, Entwurf aus PROJECT.md/docs/MODULES.md) + CRC32-Routine.
//         Konzepttreu zu OS-9, aber NICHT binärkompatibel (Entscheidung E2). Reines Datenformat
//         hier — Suchen/Validieren/Bekanntmachen (2.3a/b/c) folgt in eigenen Modulen.
//
// Call:   crc = q9_crc32(data, len)
//
// Edition History
//─────────┬──────┬────────────────────────────────────────────────────────────────────────┬──────
// Date    │ Ver. │ Description                                                            │ By
//─────────┼──────┼────────────────────────────────────────────────────────────────────────┼──────
// 26-07-03│ 1.00 │ 2.1: Modul-Header-Struct + q9_crc32                                    │ CF
//═════════╧══════╧════════════════════════════════════════════════════════════════════════╧══════
#ifndef Q9_MODULE_H
#define Q9_MODULE_H

#include <stdint.h>

#define Q9_MOD_SYNC0   0x51                                /* 'Q' — Modul-Sync-Byte 0             */
#define Q9_MOD_SYNC1   0x39                                /* '9' — Modul-Sync-Byte 1             */
#define Q9_MOD_HDRSIZE 0x1C                                /* Header-Groesse in Bytes             */

/* Type ($0C) — was das Modul ist (siehe PROJECT.md) */
#define Q9_MOD_PRGRM   1                                   /* Programm-Modul                      */
#define Q9_MOD_DRIVR   2                                   /* Treiber                             */
#define Q9_MOD_FILEMGR 3                                   /* File-Manager                        */
#define Q9_MOD_DATA    4                                   /* Datenmodul                          */
#define Q9_MOD_RUNTIME 5                                   /* Runtime (z.B. 68k-Emulator)          */

/* Language ($0D) — womit es ausfuehrbar ist */
#define Q9_MOD_WASM    1
#define Q9_MOD_M68K    2
#define Q9_MOD_MC6809  3                                   /* reserviert                          */

/* Attribute ($0E) */
#define Q9_MOD_REENT   0x01                                /* reentrant                            */

//╔══════════════════════════════════════════════════════════════════════════════════════════════╗
//║ MODUL-HEADER (feste Byte-Offsets, siehe PROJECT.md — "Modul-Header (Entwurf)")                ║
//╚══════════════════════════════════════════════════════════════════════════════════════════════╝
// #pragma pack erzwingt die exakten Offsets ohne Alignment-Luecken — auf allen drei Q9-Toolchains
// (gcc/w64devkit, clang/macOS, emcc) unterstuetzt. Damit ist der Struct direkt aufs ROM-Image-Blob
// abbildbar (2.3a), ohne die Felder einzeln byteweise zusammenzusetzen.
#pragma pack(push, 1)
typedef struct q9_modhdr {
    uint8_t  sync[2];                                      /* $00: Sync $51 $39 ("Q9")            */
    uint16_t hdrsize;                                       /* $02: Header-Groesse                 */
    uint32_t modsize;                                       /* $04: Modulgroesse gesamt (inkl. CRC) */
    uint32_t nameoff;                                       /* $08: Offset zum Modulnamen          */
    uint8_t  type;                                          /* $0C: Q9_MOD_...                     */
    uint8_t  lang;                                          /* $0D: Q9_MOD_...                     */
    uint8_t  attr;                                          /* $0E: Q9_MOD_REENT ...                */
    uint8_t  rev;                                            /* $0F: Revision                       */
    uint32_t execoff;                                        /* $10: Einsprung                      */
    uint32_t datasize;                                       /* $14: statischer Datenbedarf          */
    uint32_t crc32;                                          /* $18: CRC ueber gesamtes Modul,       */
                                                              /*      Feld selbst = 0 gerechnet      */
} q9_modhdr_t;
#pragma pack(pop)

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_crc32
// Desc.:    Standard-CRC-32 (CRC-32/ISO-HDLC, Poly $EDB88320 reflektiert, Init/Final $FFFFFFFF —
//           wie in ZIP/Ethernet). Bitweise Implementierung, keine Tabelle (kein malloc im Kernel,
//           Q9-Module sind klein genug, dass die Tabelle sich nicht lohnt).
// Call:     crc = q9_crc32(data, len)
//════════════════════════════════════════════════════════════════════════════════════════════════
uint32_t q9_crc32(const uint8_t *data, uint32_t len);

#endif // Q9_MODULE_H

//────────────────────────────────────────────────────────────────────────────────────────────────
// EOF module.h                                                                            Ver. 1.00
//────────────────────────────────────────────────────────────────────────────────────────────────
