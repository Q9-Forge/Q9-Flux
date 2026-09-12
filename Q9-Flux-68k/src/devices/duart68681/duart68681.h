//════════════════════════════════════════════════════════════════════════════════════════════════
// File:   duart68681.h                                                                    Ver. 1.00
// Owner:  CF
// Desc.:  68681-DUART (docs/BOARD.md, Abschnitt "68681 DUART"). Kanal A ist die Konsole (THRA ->
//         q9_hal_con_put, RHRA <- Host-Terminal via RX-FIFO); Kanal B ist unverbunden (sendet ins
//         Leere, empfaengt nie). Nur SRA+THRA/RHRA wirklich aktiv, der Rest des Registersatzes
//         wird sauber angenommen (liest 0, Schreiben verworfen).
//
//         2026-08-21 (Hardware-Vereinheitlichung, Folgeschritt nach dem "cf"-Piloten): aus
//         src/kernel/q9board.c/.h HIERHER verschoben -- Andreas' Vorgabe, pro Hardware-Typ EIN
//         eigenes Sourcefile zu haben. ANDERS als bei "cf": der Registerzustand (uart_rx_fifo/
//         uart_mr_*/uart_ivr/uart_imr) steckt weiterhin IN q9_board_t (kein eigenes q9_duart_t --
//         die DUART ist heute nicht mehrfach instanziierbar, anders als CF mit seinen zwei
//         Interfaces). Reines Verschieben der FUNKTIONEN (dev->state bleibt q9_board_t*), KEINE
//         Verhaltensaenderung. q9_board_uart_irq_pending() wurde dabei `static` (kein externer
//         Aufrufer ausser der eigenen Vtable, s. Commit-Kommentar).
//
// Edition History
//─────────┬──────┬────────────────────────────────────────────────────────────────────────┬──────
// Date    │ Ver. │ Description                                                            │ By
//─────────┼──────┼────────────────────────────────────────────────────────────────────────┬──────
// 26-07-04│ 1.xx │ 5.2b/5.4/5.7: urspruenglich Teil von q9board.h/.c, s. dortige Historie   │ CF
//         │      │ bis Ver. 2.30/2.10 fuer die volle Entwicklungsgeschichte                 │
// 26-08-21│ 1.00 │ Hardware-Vereinheitlichung: aus q9board.c/.h hierher verschoben,          │ Cld
//         │      │ neu q9_devdesc_duart68681 (noch ohne extra_fields, kein Config-Schema)   │
//═════════╧══════╧════════════════════════════════════════════════════════════════════════╧══════
#ifndef Q9_DUART68681_H
#define Q9_DUART68681_H

#include "../../kernel/q9board.h"                          /* q9_board_t -- s. Kopfkommentar     */
#include "../../kernel/devdesc.h"                           /* q9_devdesc_t                        */

extern const q9_device_vtable_t q9_devtype_duart68681;
extern const q9_devdesc_t       q9_devdesc_duart68681;      /* 2026-08-21: Vtable+Schema vereint   */

/*───────────────────────────────────────────────────────────────────────────
  Diagnose (2026-09-03): THRA-Mitschrift.

  Haelt fest, was TATSAECHLICH auf dem Sendedatenregister landet -- Buswert,
  dazu d0 und PC der CPU beim selben Zugriff. Entscheidender Vorteil
  gegenueber einer Instrumentierung im Gast: der Gastcode bleibt
  unveraendert, ein timing-abhaengiges Symptom verschwindet also nicht unter
  der Messung.

  Damit wurde 2026-09-03 eine verstuemmelte Konsolenausgabe zerlegt: die
  Mitschrift zeigte, dass die Bytes bereits verstuemmelt AM BUS ankamen --
  womit der gesamte Emulator-Ausgabepfad als Ursache ausschied und die Suche
  im Gast weiterging (es war dort ein als Code ausgefuehrter Funktionszeiger,
  s. Q9-OS q9kernel_entry.a, Q9K_IOManOutVtable).

  Kostet einen Store pro ausgegebenem Zeichen -- bewusst ohne fprintf, damit
  das Timing unangetastet bleibt. Ausgabe im Ctrl-^-Dump (q9boardrun.c).
  ───────────────────────────────────────────────────────────────────────────*/
#define Q9_DBG_THRA_LOG_SIZE 512u
extern uint8_t  q9_dbg_thra_log[Q9_DBG_THRA_LOG_SIZE];      /* Buswert                              */
extern uint8_t  q9_dbg_thra_d0[Q9_DBG_THRA_LOG_SIZE];       /* d0.b der CPU beim selben Zugriff     */
extern uint32_t q9_dbg_thra_pc[Q9_DBG_THRA_LOG_SIZE];       /* PC beim selben Zugriff               */
extern uint32_t q9_dbg_thra_count;                          /* Gesamtzahl, auch ueber die Puffergroesse hinaus */

#endif /* Q9_DUART68681_H */
//────────────────────────────────────────────────────────────────────────────────────────────────
// EOF duart68681.h                                                                        Ver. 1.00
//────────────────────────────────────────────────────────────────────────────────────────────────
