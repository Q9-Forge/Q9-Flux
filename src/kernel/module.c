//════════════════════════════════════════════════════════════════════════════════════════════════
// File:   module.c                                                                        Ver. 1.00
// Owner:  AF
// Desc.:  CRC32-Routine fuer Q9-Module (Header-Struct steht in module.h). Suchen/Validieren/
//         Bekanntmachen des ROM-Image-Blobs folgt in 2.3a/b/c.
//
// Call:   crc = q9_crc32(data, len)
//
// Edition History
//─────────┬──────┬────────────────────────────────────────────────────────────────────────┬──────
// Date    │ Ver. │ Description                                                            │ By
//─────────┼──────┼────────────────────────────────────────────────────────────────────────┼──────
// 26-07-03│ 1.00 │ 2.1: q9_crc32 (bitweise, CRC-32/ISO-HDLC)                              │ CF
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
// EOF module.c                                                                            Ver. 1.00
//────────────────────────────────────────────────────────────────────────────────────────────────
