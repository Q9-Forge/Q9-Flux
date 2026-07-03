//════════════════════════════════════════════════════════════════════════════════════════════════
// File:   module.h                                                                        Ver. 1.30
// Owner:  AF
// Desc.:  Q9-Modul-Header (Phase 2, Entwurf aus PROJECT.md/docs/MODULES.md) + CRC32-Routine +
//         ROM-Image-Suche/Validierung + Modul-Directory (Bekanntmachen) + F$Link/F$UnLink-
//         Unterbau. Konzepttreu zu OS-9, aber NICHT binärkompatibel (Entscheidung E2).
//         Boot-Pipeline (docs/MODULES.md): Suchen (2.3a) -> Validieren (2.3b) ->
//         Bekanntmachen (2.3c) -> Link/Unlink (2.3d).
//
// Call:   crc = q9_crc32(data, len); hdr = q9_mod_scan_first(rom, romlen);
//         err = q9_mod_validate(rom, romlen, hdr); err = q9_mod_register(hdr);
//         err = q9_mod_link("name", type, lang, &hdr); err = q9_mod_unlink(hdr)
//
// Edition History
//─────────┬──────┬────────────────────────────────────────────────────────────────────────┬──────
// Date    │ Ver. │ Description                                                            │ By
//─────────┼──────┼────────────────────────────────────────────────────────────────────────┼──────
// 26-07-03│ 1.00 │ 2.1: Modul-Header-Struct + q9_crc32                                    │ CF
// 26-07-03│ 1.10 │ 2.3a: q9_mod_scan_first/next (ROM-Image-Suche nach Sync-Bytes)         │ CF
// 26-07-03│ 1.20 │ 2.3b: q9_mod_validate (Groesse/Nameoffset/CRC32)                       │ CF
// 26-07-03│ 1.30 │ 2.3c+d: Modul-Directory (q9_mod_register/find) + F$Link/UnLink-Unterbau│ CF
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

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_mod_scan_first
// Desc.:    Durchsucht ein ROM-Image-Blob byteweise nach den Sync-Bytes ($51 $39). Prüft nur die
//           Sync-Bytes (Größe/CRC folgt in 2.3b) — liefert einen Zeiger auf den (mutmaßlichen)
//           Modul-Header, oder NULL, wenn im Blob kein Sync mehr Platz für einen vollen Header hat.
// Call:     hdr = q9_mod_scan_first(rom, romlen)
//════════════════════════════════════════════════════════════════════════════════════════════════
const q9_modhdr_t *q9_mod_scan_first(const uint8_t *rom, uint32_t romlen);

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_mod_scan_next
// Desc.:    Springt von einem gefundenen Modul um dessen ModuleSize weiter und prüft dort erneut
//           die Sync-Bytes (OS-9-Vorbild: Module liegen im ROM-Image lückenlos hintereinander).
//           NULL, wenn dort kein Sync steht oder der Sprung aus dem Blob heraus führen würde.
// Call:     next = q9_mod_scan_next(rom, romlen, hdr)
//════════════════════════════════════════════════════════════════════════════════════════════════
const q9_modhdr_t *q9_mod_scan_next(const uint8_t *rom, uint32_t romlen, const q9_modhdr_t *cur);

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_mod_validate
// Desc.:    Validiert einen von q9_mod_scan_first/next gefundenen Header: HeaderSize, ModuleSize
//           (muss vollständig ins Blob passen), NameOffset (muss innerhalb des Moduls liegen),
//           danach CRC32 über das gesamte Modul (CRC-Feld selbst wird dabei als 0 gerechnet, wie
//           im Header dokumentiert). Struktur-Checks bewusst VOR der CRC (billig vor teuer) —
//           die von OS-9 bekannte zusätzliche Header-Parity-Stufe wird NICHT nachgebildet
//           (Q9-Module sind klein genug, siehe ARBEITSPLAN 2.3b).
// Call:     err = q9_mod_validate(rom, romlen, hdr)   // 0 = ok, sonst E$BMHP/E$BMCRC
//════════════════════════════════════════════════════════════════════════════════════════════════
int q9_mod_validate(const uint8_t *rom, uint32_t romlen, const q9_modhdr_t *hdr);

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_mod_name
// Desc.:    Liefert den (nullterminierten) Modulnamen über NameOffset. Aufrufer muss vorher
//           q9_mod_validate() erfolgreich durchlaufen haben (NameOffset sonst nicht geprüft).
// Call:     name = q9_mod_name(hdr)
//════════════════════════════════════════════════════════════════════════════════════════════════
const char *q9_mod_name(const q9_modhdr_t *hdr);

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_mod_register
// Desc.:    Trägt einen bereits validierten Header in die Modul-Directory ein ("Bekanntmachen",
//           2.3c). Namenskollisions-Regel wie OS-9 (docs/MODULES.md): gleicher Name+Type ->
//           höhere Revision gewinnt, bei Gleichstand bleibt das etablierte Modul (kein Fehler,
//           stiller No-Op). E$DirFul, wenn für ein neues Modul kein Slot mehr frei ist.
// Call:     err = q9_mod_register(hdr)
//════════════════════════════════════════════════════════════════════════════════════════════════
int q9_mod_register(const q9_modhdr_t *hdr);

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_mod_find
// Desc.:    Sucht die Directory nach Name (+ Type/Language, 0 = "beliebig") — reine Suche ohne
//           Link-Count-Änderung (OS-9-Vorbild: F$FModul). NULL = nicht gefunden.
// Call:     hdr = q9_mod_find("term", Q9_MOD_DRIVR, 0)
//════════════════════════════════════════════════════════════════════════════════════════════════
const q9_modhdr_t *q9_mod_find(const char *name, uint8_t type, uint8_t lang);

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_mod_link
// Desc.:    F$Link-Unterbau: sucht per q9_mod_find, erhöht bei Erfolg den Link-Count und liefert
//           den Header über *out. E$MNF, wenn kein passendes Modul in der Directory steht.
// Call:     err = q9_mod_link("term", Q9_MOD_DRIVR, 0, &hdr)
//════════════════════════════════════════════════════════════════════════════════════════════════
int q9_mod_link(const char *name, uint8_t type, uint8_t lang, const q9_modhdr_t **out);

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_mod_unlink
// Desc.:    F$UnLink-Unterbau: senkt den Link-Count eines per q9_mod_link geholten Moduls
//           (Boden bei 0, Speicherfreigabe bei Link-Count 0 entfällt — ROM-Image, siehe 2.3-Intro
//           im ARBEITSPLAN). E$MNF, wenn hdr kein Directory-Eintrag ist.
// Call:     err = q9_mod_unlink(hdr)
//════════════════════════════════════════════════════════════════════════════════════════════════
int q9_mod_unlink(const q9_modhdr_t *hdr);

#endif // Q9_MODULE_H

//────────────────────────────────────────────────────────────────────────────────────────────────
// EOF module.h                                                                            Ver. 1.30
//────────────────────────────────────────────────────────────────────────────────────────────────
