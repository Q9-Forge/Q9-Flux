//════════════════════════════════════════════════════════════════════════════════════════════════
// File:   remap.h                                                                         Ver. 1.00
// Owner:  AF
// Desc.:  REMAP-Register — reiner Adress-Trigger (kein Datenwert, Lesen wie Schreiben loesen
//         dieselbe Wirkung aus): ein Zugriff irgendwo im Fenster $FFFF8000-$FFFF8FFF schaltet das
//         Board unwiderruflich vom Reset-Zustand (ROM ab Adresse 0 gespiegelt) in den Remap-
//         Zustand (RAM ab Adresse 0, ROM nur noch bei $FE000000-$FE07FFFF) -- exakt wie beim
//         originalen CB030-Vorbild. Das Boot-ROM loest den Trigger genau einmal aus, sobald es
//         RAM-Setup abgeschlossen hat.
//
//         2026-08-21 (Hardware-Vereinheitlichung, Andreas' Idee: "das müssten wir vielleicht auch
//         als simuliertes Gerät einbauen"): aus src/kernel/q9board.c/.h HIERHER verschoben --
//         reiner Adress-Trigger, genau wie TI_IRQ_ON/OFF (s. timer_irq.h), damit ein eigenes
//         devreg-Geraet statt eines Sonderfalls im Board-Fallback. BEWUSST NICHT verschoben: die
//         eigentliche RAM/ROM-Interpretation (board_read_byte/write_byte in q9board.c, plus der
//         RAM-Fast-Path in m68krt.c) -- das ist keine Fenster-Peripherie, sondern die
//         Adressraum-Topologie des ganzen Boards, mit eigenem performance-kritischem Fast-Path,
//         der devreg bewusst umgeht. Der Zustand (q9_board_t.remapped) bleibt EIN gemeinsames
//         Feld -- dev->state zeigt wie bei duart68681/rtc72421/timer_irq einfach auf das ganze
//         q9_board_t, kein neuer Weitergabe-Mechanismus noetig.
//
// Edition History
//─────────┬──────┬────────────────────────────────────────────────────────────────────────┬──────
// Date    │ Ver. │ Description                                                            │ By
//─────────┼──────┼────────────────────────────────────────────────────────────────────────┬──────
// 25/26-xx│ 1.xx │ 5.2a/5.3: urspruenglich Teil von q9board.h/.c, s. dortige Historie       │ CF
// 26-08-21│ 1.00 │ Hardware-Vereinheitlichung: aus q9board.c/.h hierher verschoben, neu      │ Cld
//         │      │ q9_devdesc_remap (noch ohne extra_fields, kein Config-Schema)             │
//═════════╧══════╧════════════════════════════════════════════════════════════════════════╧══════
#ifndef Q9_REMAP_H
#define Q9_REMAP_H

#include "../../kernel/q9board.h"                          /* q9_board_t -- s. Kopfkommentar     */
#include "../../kernel/devdesc.h"                           /* q9_devdesc_t                        */

#define Q9_BOARD_REMAP_REG_BASE    0xFFFF8000u        /* REMAP-Register: reiner Adress-Trigger  */
#define Q9_BOARD_REMAP_REG_TOP     0xFFFF8FFFu

extern const q9_device_vtable_t q9_devtype_remap;
extern const q9_devdesc_t       q9_devdesc_remap;           /* 2026-08-21: Vtable+Schema vereint   */

#endif /* Q9_REMAP_H */
//────────────────────────────────────────────────────────────────────────────────────────────────
// EOF remap.h                                                                            Ver. 1.00
//────────────────────────────────────────────────────────────────────────────────────────────────
