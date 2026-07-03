//════════════════════════════════════════════════════════════════════════════════════════════════
// File:   module.c                                                                        Ver. 1.10
// Owner:  AF
// Desc.:  CRC32-Routine + ROM-Image-Suche fuer Q9-Module (Header-Struct steht in module.h).
//         Validieren/Bekanntmachen des ROM-Image-Blobs folgt in 2.3b/c.
//
// Call:   crc = q9_crc32(data, len); hdr = q9_mod_scan_first(rom, romlen)
//
// Edition History
//─────────┬──────┬────────────────────────────────────────────────────────────────────────┬──────
// Date    │ Ver. │ Description                                                            │ By
//─────────┼──────┼────────────────────────────────────────────────────────────────────────┼──────
// 26-07-03│ 1.00 │ 2.1: q9_crc32 (bitweise, CRC-32/ISO-HDLC)                              │ CF
// 26-07-03│ 1.10 │ 2.3a: q9_mod_scan_first/next — Sync-Suche, Sprung um ModuleSize        │ CF
//═════════╧══════╧════════════════════════════════════════════════════════════════════════╧══════

#include "module.h"

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_crc32
// Desc.:    Siehe module.h. Bitweise CRC-32/ISO-HDLC ueber data[0..len).
// Call:     crc = q9_crc32(data, len)
//════════════════════════════════════════════════════════════════════════════════════════════════
uint32_t q9_crc32(const uint8_t *data, uint32_t len)
{
    uint32_t crc = 0xFFFFFFFFu;

    for (uint32_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (int bit = 0; bit < 8; bit++) {
            uint32_t mask = -(crc & 1u);
            crc = (crc >> 1) ^ (0xEDB88320u & mask);
        }
    }
    return crc ^ 0xFFFFFFFFu;
}

//────────────────────────────────────────────────────────────────────────────────────────────────
// Function: sync_ok
// Desc.:    Prueft, ob bei rom[off] ein voller Header Platz hat und dort die Sync-Bytes stehen.
//────────────────────────────────────────────────────────────────────────────────────────────────
static int sync_ok(const uint8_t *rom, uint32_t romlen, uint32_t off)
{
    if (off > romlen - Q9_MOD_HDRSIZE) {               /* romlen >= Q9_MOD_HDRSIZE vorausgesetzt  */
        return 0;
    }
    return rom[off] == Q9_MOD_SYNC0 && rom[off + 1] == Q9_MOD_SYNC1;
}

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_mod_scan_first
// Desc.:    Siehe module.h. Byteweise Suche nach den Sync-Bytes, Groesse/CRC folgt in 2.3b.
// Call:     hdr = q9_mod_scan_first(rom, romlen)
//════════════════════════════════════════════════════════════════════════════════════════════════
const q9_modhdr_t *q9_mod_scan_first(const uint8_t *rom, uint32_t romlen)
{
    if (romlen < Q9_MOD_HDRSIZE) {
        return 0;
    }
    for (uint32_t off = 0; off <= romlen - Q9_MOD_HDRSIZE; off++) {
        if (sync_ok(rom, romlen, off)) {
            return (const q9_modhdr_t *)(rom + off);
        }
    }
    return 0;
}

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_mod_scan_next
// Desc.:    Siehe module.h. Springt exakt um ModuleSize weiter (kein erneutes Durchsuchen) —
//           OS-9-Vorbild: Module liegen im ROM-Image lueckenlos hintereinander.
// Call:     next = q9_mod_scan_next(rom, romlen, hdr)
//════════════════════════════════════════════════════════════════════════════════════════════════
const q9_modhdr_t *q9_mod_scan_next(const uint8_t *rom, uint32_t romlen, const q9_modhdr_t *cur)
{
    uint32_t curoff  = (uint32_t)((const uint8_t *)cur - rom);
    uint32_t nextoff = curoff + cur->modsize;

    if (cur->modsize < Q9_MOD_HDRSIZE || nextoff < curoff) {  /* Groessen-Plausibilitaet: 2.3b;   */
        return 0;                                             /* hier nur Schutz vor Endlosschleife/Overflow */
    }
    if (romlen < Q9_MOD_HDRSIZE || nextoff > romlen - Q9_MOD_HDRSIZE) {
        return 0;
    }
    return sync_ok(rom, romlen, nextoff) ? (const q9_modhdr_t *)(rom + nextoff) : 0;
}

//────────────────────────────────────────────────────────────────────────────────────────────────
// EOF module.c                                                                            Ver. 1.10
//────────────────────────────────────────────────────────────────────────────────────────────────
