//════════════════════════════════════════════════════════════════════════════════════════════════
// File:   rtc72421.h                                                                      Ver. 1.00
// Owner:  CF
// Desc.:  RTC72421 — Epson-Echtzeituhr am Bus ($FFFFD000, 16 Nibble-Register). LESEN liefert die
//         Host-Uhr (Register 0..C als BCD-Nibbles, D/E/F Control), SCHREIBEN wird komplett
//         ignoriert (die Host-Uhr ist die Wahrheit). Ein Lesezugriff auf Register 0 frischt den
//         internen Latch aus der Host-Uhr auf, alle weiteren Register lesen aus dem Latch — wer
//         S1 zuerst liest (wie der rtclock-Treiber im REF-Q9-Port), bekommt einen in sich
//         konsistenten Zeitstempel ohne Rollover-Risiko.
//
//         2026-08-21 (Hardware-Vereinheitlichung, Folgeschritt nach dem "cf"-Piloten): aus
//         src/kernel/q9board.c/.h HIERHER verschoben. Wie bei duart68681: der Registerzustand
//         (rtc_regs/rtc_latch_valid) steckt weiterhin IN q9_board_t (kein eigenes q9_rtc_t --
//         nicht mehrfach instanziierbar). Reines Verschieben, KEINE Verhaltensaenderung.
//
// Edition History
//─────────┬──────┬────────────────────────────────────────────────────────────────────────┬──────
// Date    │ Ver. │ Description                                                            │ By
//─────────┼──────┼────────────────────────────────────────────────────────────────────────┬──────
// 26-07-14│ 1.xx │ 5.6/5.17: urspruenglich Teil von q9board.h/.c, s. dortige Historie       │ CF
// 26-08-21│ 1.00 │ Hardware-Vereinheitlichung: aus q9board.c/.h hierher verschoben, neu      │ Cld
//         │      │ q9_devdesc_rtc72421 (noch ohne extra_fields, kein Config-Schema)          │
//═════════╧══════╧════════════════════════════════════════════════════════════════════════╧══════
#ifndef Q9_RTC72421_H
#define Q9_RTC72421_H

#include "../../kernel/q9board.h"                          /* q9_board_t -- s. Kopfkommentar     */
#include "../../kernel/devdesc.h"                           /* q9_devdesc_t                        */

extern const q9_device_vtable_t q9_devtype_rtc72421;
extern const q9_devdesc_t       q9_devdesc_rtc72421;        /* 2026-08-21: Vtable+Schema vereint   */

#endif /* Q9_RTC72421_H */
//────────────────────────────────────────────────────────────────────────────────────────────────
// EOF rtc72421.h                                                                          Ver. 1.00
//────────────────────────────────────────────────────────────────────────────────────────────────
