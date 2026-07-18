//════════════════════════════════════════════════════════════════════════════════════════════════
// File:   m68krt.h                                                                        Ver. 1.20
// Owner:  AF
// Desc.:  Schmaler Q9-Wrapper um die eingebettete Musashi-68000-Emulation (third_party/musashi,
//         Entscheidung E12 in PROJECT.md). Native-Build-only — Grundbaustein fuer Phase 5 (Prozesse
//         mit echtem 68k-Maschinencode, vgl. den wasm3-Zweig aus Phase 4/wasmrt.h). Schritt 5.1:
//         nur Laden + Ausfuehren in emuliertem RAM + Register lesen, KEINE Scheduler-/
//         Syscall-Bridge-Entscheidungen (die kommen erst mit der Detailplanung von Phase 5).
//
//         Musashi haelt seinen kompletten CPU-Zustand in eigenen globalen Variablen (kein Kontext-
//         Zeiger in m68k_read/write_memory_*, anders als wasm3s IM3Runtime-Handles) — es kann daher
//         je Prozess immer nur EINE Musashi-Instanz aktiv sein. q9_m68krt_t ist deshalb bewusst kein
//         eigenstaendiges, mehrfach instanzierbares Handle wie q9_wasmrt_t, sondern nur eine duenne
//         Buchhaltungs-Struktur (RAM-Zeiger+Groesse) neben den Musashi-eigenen Globals.
//
// Call:   q9_m68krt_t rt; q9_m68krt_init(&rt, ram, sizeof(ram));
//         q9_m68krt_reset(&rt); q9_m68krt_execute(&rt, 100);
//         d0 = q9_m68krt_get_d(&rt, 0); q9_m68krt_free(&rt);
//
// Edition History
//─────────┬──────┬────────────────────────────────────────────────────────────────────────┬──────
// Date    │ Ver. │ Description                                                            │ By
//─────────┼──────┼────────────────────────────────────────────────────────────────────────┼──────
// 26-07-04│ 1.00 │ 5.1: Erster Grundbaustein — RAM anbinden, Reset+Execute, D0-D7 lesen    │ CF
// 26-07-04│ 1.10 │ 5.2d: q9_m68krt_set_irq — duenner Wrapper um m68k_set_irq() fuer         │ CF
//         │      │ cb030.c's Timer/IRQ3-Polling                                            │
// 26-07-05│ 1.20 │ 5.3: q9_m68krt_attach_board — Speicherzugriffe wahlweise ueber den       │ CF
//         │      │ CB030-Adress-Dispatch (cb030.h) statt nacktem RAM-Block                 │
// 26-07-10│ 1.30 │ 5.9: q9_m68krt_is_stopped — Wrapper um Musashis m68k_is_stopped()        │ CF
//         │      │ (Vendor-Patch) fuer die CB030-Idle-Drossel                              │
//═════════╧══════╧════════════════════════════════════════════════════════════════════════╧══════
#ifndef Q9_M68KRT_H
#define Q9_M68KRT_H

#include <stdint.h>
#include "cb030.h"

#define Q9_M68KRT_OK        0
#define Q9_M68KRT_ERR_RAM  -1                       /* RAM fehlt oder zu klein fuer Reset-Vektoren */

/* Buchhaltung neben Musashis eigenen Globals (s.o.) — kein Handle im Sinne von q9_wasmrt_t. */
typedef struct q9_m68krt {
    uint8_t  *ram;                                   /* Emuliertes RAM, big-endian (68k-Byteorder) */
    uint32_t  ram_len;
} q9_m68krt_t;

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_m68krt_init
// Desc.:    Bindet einen RAM-Block als Speicher der Musashi-Instanz an (m68k_read/write_memory_*
//           in m68krt.c greifen darauf zu) und legt den CPU-Typ fest (68030, Entscheidung E12).
//           ram_len muss mindestens 8 Byte sein (Reset-Vektoren: SP bei Adresse 0, PC bei Adresse
//           4, je 4 Byte big-endian) — der Aufrufer traegt die Vektoren + das Programm selbst ein,
//           BEVOR q9_m68krt_reset() aufgerufen wird.
// Call:     err = q9_m68krt_init(&rt, ram, sizeof(ram))
//════════════════════════════════════════════════════════════════════════════════════════════════
int q9_m68krt_init(q9_m68krt_t *rt, uint8_t *ram, uint32_t ram_len);

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_m68krt_reset
// Desc.:    Liest die Reset-Vektoren aus dem RAM (Adresse 0 = initialer SP, Adresse 4 = initialer
//           PC) und setzt die CPU auf diesen Zustand zurueck (m68k_pulse_reset()).
// Call:     q9_m68krt_reset(&rt)
//════════════════════════════════════════════════════════════════════════════════════════════════
void q9_m68krt_reset(q9_m68krt_t *rt);

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_m68krt_execute
// Desc.:    Fuehrt bis zu 'cycles' CPU-Takte aus (m68k_execute()). Liefert die tatsaechlich
//           verbrauchten Takte zurueck (kann wegen unvollstaendiger letzter Instruktion leicht
//           abweichen).
// Call:     spent = q9_m68krt_execute(&rt, 100)
//════════════════════════════════════════════════════════════════════════════════════════════════
int q9_m68krt_execute(q9_m68krt_t *rt, int cycles);

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_m68krt_get_d
// Desc.:    Liefert den Inhalt von Datenregister Dn (n = 0..7) — fuer den Rauchtest (5.1) reicht
//           das; PC/SR/Ax folgen erst, wenn die Syscall-Bridge sie tatsaechlich braucht.
// Call:     v = q9_m68krt_get_d(&rt, 0)
//════════════════════════════════════════════════════════════════════════════════════════════════
uint32_t q9_m68krt_get_d(q9_m68krt_t *rt, int n);

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_m68krt_free
// Desc.:    Loest die RAM-Bindung wieder (Musashis eigene Globals bleiben bestehen, s.o. — ein
//           erneutes q9_m68krt_init() mit neuem RAM ist trotzdem sauber moeglich, weil jeder
//           Aufruf m68k_init()+m68k_set_cpu_type() neu ausfuehrt).
// Call:     q9_m68krt_free(&rt)
//════════════════════════════════════════════════════════════════════════════════════════════════
void q9_m68krt_free(q9_m68krt_t *rt);

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_m68krt_set_irq
// Desc.:    5.2d: Duenner Wrapper um Musashis m68k_set_irq(level) — die eigentliche Interrupt-
//           Mechanik (PC+SR auf den Supervisor-Stack, Vektor holen, springen) macht Musashi
//           vollstaendig selbst; dieser Wrapper existiert nur, damit cb030.c (das Musashi bewusst
//           nicht kennt, s. cb030.h) nicht direkt gegen third_party/musashi linken muss. level = 0
//           loescht die Interrupt-Anforderung wieder (Musashi-Konvention).
// Call:     q9_m68krt_set_irq(3)
//════════════════════════════════════════════════════════════════════════════════════════════════
void q9_m68krt_set_irq(int level);

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_m68krt_attach_board
// Desc.:    5.3: Schaltet die sechs Musashi-Speicher-Hooks auf den CB030-Adress-Dispatch um —
//           ALLE CPU-Zugriffe (auch das Holen der Reset-Vektoren durch q9_m68krt_reset) laufen
//           dann ueber q9_cb030_read/write8/16/32 (RAM/ROM/Remap/UART/CF/Timer) statt ueber den
//           nackten RAM-Block aus q9_m68krt_init. Setzt ausserdem den Interrupt-Acknowledge-
//           Callback auf Autovector-Betrieb mit Puls-Verhalten (IRQ-Leitung wird beim Annehmen
//           losgelassen, s. m68krt.c). board = NULL schaltet zurueck in den RAM-Modus (5.1);
//           q9_m68krt_init/free setzen ebenfalls auf RAM-Modus zurueck. Aufruf NACH
//           q9_m68krt_init und VOR q9_m68krt_reset.
// Call:     q9_m68krt_attach_board(&board);  ...  q9_m68krt_attach_board(0);
//════════════════════════════════════════════════════════════════════════════════════════════════
void q9_m68krt_attach_board(q9_cb030_t *board);

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_m68krt_attach_quicc
// Desc.:    5.11: Haengt die QUICC-Ethernet-Emulation (quicc.h) in den Adress-Dispatch ein —
//           Zugriffe auf das Fenster $FFFF2000-$FFFF3FFF gehen dann an q9_quicc_read/write*,
//           und der Interrupt-Acknowledge liefert fuer Level 5 den QUICC-Vektor (254), solange
//           der QUICC einen Interrupt anfordert. quicc = NULL haengt das Fenster wieder aus;
//           q9_m68krt_free setzt ebenfalls zurueck. Aufruf NACH q9_m68krt_attach_board.
// Call:     q9_m68krt_attach_quicc(&quicc);  ...  q9_m68krt_attach_quicc(0);
//════════════════════════════════════════════════════════════════════════════════════════════════
struct q9_quicc;
void q9_m68krt_attach_quicc(struct q9_quicc *quicc);

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_m68krt_attach_cf2
// Desc.:    5.19a: Registriert ein ZWEITES Compact-Flash-Interface (RC2014-SC145-Kartenleser bei
//           Q9_CB030_CF2_BASE, Descriptoren e0/f0 im MWOS-Q9-Port) in der Geraete-Registry. Nutzt
//           dieselbe q9_devtype_cf-Vtable wie die Onboard-CF, nur mit eigener q9_cf_t-Instanz und
//           eigener Basisadresse. Kein IRQ (wie die Onboard-CF). Nur aufrufen, wenn die Board-
//           Config dort Images anhaengt — ohne Aufruf existiert das Fenster nicht (Board wie 5.17).
//           cf2 muss die gesamte Lauf­zeit ueberleben (wird nur als Zeiger gehalten).
// Call:     q9_m68krt_attach_cf2(&cf2);
//════════════════════════════════════════════════════════════════════════════════════════════════
void q9_m68krt_attach_cf2(q9_cf_t *cf2);
void q9_m68krt_attach_cf_at(q9_cf_t *cf, uint32_t base, const char *name);

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_m68krt_debug_state
// Desc.:    Diagnose (5.4): aktueller PC + SR der emulierten CPU und Anzahl der bisher
//           durchlaufenen Interrupt-Acknowledge-Zyklen — fuer die Boot-Fehlersuche im Runner
//           (wo haengt die CPU, kommt der Interrupt ueberhaupt an, wie steht die IRQ-Maske).
// Call:     q9_m68krt_debug_state(&pc, &sr, &acks)
//════════════════════════════════════════════════════════════════════════════════════════════════
void q9_m68krt_debug_state(uint32_t *pc, uint32_t *sr, uint32_t *acks);

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_m68krt_quicc_acks
// Desc.:    5.15-Diagnose (TCP-Haenger): Anzahl der bisher tatsaechlich an die CPU zugestellten
//           QUICC-Interrupts (Level 5). Waechst dieser Zaehler waehrend eines Haengers NICHT
//           weiter, obwohl die QUICC-RXF-Zaehler (quicc.c diag_rxf) steigen, wird der Hardware-
//           Interrupt nicht zugestellt -> Emulator-Bug. Waechst er weiter -> ISR laeuft, der
//           Stillstand sitzt im (geschlossenen) Gast-Treiber.
//════════════════════════════════════════════════════════════════════════════════════════════════
uint32_t q9_m68krt_quicc_acks(void);

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_m68krt_is_stopped
// Desc.:    5.9: Duenner Wrapper um Musashis m68k_is_stopped() (Vendor-Patch, s.
//           third_party/musashi/Q9_VENDOR.md) — 1 wenn die CPU per STOP-Instruktion angehalten
//           ist (z.B. OS-9s Idle-Loop), sonst 0. Grundlage fuer die Host-Idle-Drossel im
//           CB030-Runner (cb030run.c): wenn gestoppt UND kein IRQ anliegt, kann der Host
//           kurz schlafen statt den Slice sofort wieder "leer" zu verbrennen.
// Call:     if (q9_m68krt_is_stopped()) ...
//════════════════════════════════════════════════════════════════════════════════════════════════
int q9_m68krt_is_stopped(void);

#endif // Q9_M68KRT_H

//────────────────────────────────────────────────────────────────────────────────────────────────
// EOF m68krt.h                                                                            Ver. 1.20
//────────────────────────────────────────────────────────────────────────────────────────────────
