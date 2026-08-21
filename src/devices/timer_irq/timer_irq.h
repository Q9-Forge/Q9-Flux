//════════════════════════════════════════════════════════════════════════════════════════════════
// File:   timer_irq.h                                                                     Ver. 1.00
// Owner:  CF
// Desc.:  Timer/IRQ3 — reine Adress-Trigger (kein Datenwert, Lesen wie Schreiben loesen dieselbe
//         Wirkung aus): TI_IRQ_OFF ($FFFF9000-$FFFF97FF) schaltet den Timer aus, TI_IRQ_ON
//         ($FFFF9800-$FFFF9FFF) an und startet die Tick-Epoche neu. Kooperative Host-Zeitpruefung
//         (kein echter Host-Interrupt, s. docs/BOARD.md) -- q9_board_poll_timer() zaehlt die
//         100-Hz-Perioden nach, inkl. Nachhol-Logik bei Idle-/Schlafphasen des Hosts.
//
//         2026-08-21 (Hardware-Vereinheitlichung, Folgeschritt nach dem "cf"-Piloten): aus
//         src/kernel/q9board.c/.h HIERHER verschoben. Wie bei duart68681/rtc72421: der Zustand
//         (timer_active/timer_synced/timer_last_ms/timer_irq_pending) steckt weiterhin IN
//         q9_board_t. Reines Verschieben, KEINE Verhaltensaenderung. q9_board_poll_timer() wurde
//         dabei `static` (kein externer Aufrufer ausser der eigenen Vtable, s. Commit-Kommentar).
//
// Edition History
//─────────┬──────┬────────────────────────────────────────────────────────────────────────┬──────
// Date    │ Ver. │ Description                                                            │ By
//─────────┼──────┼────────────────────────────────────────────────────────────────────────┬──────
// 26-07-04│ 1.xx │ 5.2d/5.6/5.17: urspruenglich Teil von q9board.h/.c, s. dortige Historie  │ CF
// 26-08-21│ 1.00 │ Hardware-Vereinheitlichung: aus q9board.c/.h hierher verschoben, neu      │ Cld
//         │      │ q9_devdesc_timer_irq (noch ohne extra_fields, kein Config-Schema)         │
//═════════╧══════╧════════════════════════════════════════════════════════════════════════╧══════
#ifndef Q9_TIMER_IRQ_H
#define Q9_TIMER_IRQ_H

#include "../../kernel/q9board.h"                          /* q9_board_t -- s. Kopfkommentar     */
#include "../../kernel/devdesc.h"                           /* q9_devdesc_t                        */

extern const q9_device_vtable_t q9_devtype_timer_irq;
extern const q9_devdesc_t       q9_devdesc_timer_irq;       /* 2026-08-21: Vtable+Schema vereint   */

#endif /* Q9_TIMER_IRQ_H */
//────────────────────────────────────────────────────────────────────────────────────────────────
// EOF timer_irq.h                                                                         Ver. 1.00
//────────────────────────────────────────────────────────────────────────────────────────────────
