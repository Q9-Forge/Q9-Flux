//════════════════════════════════════════════════════════════════════════════════════════════════
// File:   q9_hal.h                                                                        Ver. 1.10
// Owner:  AF
// Desc.:  Q9 Hardware Abstraction Layer — schmale Schnittstelle zwischen Kernel und Target.
//         Jedes Target (wasm, native, m68k) liefert genau eine Implementierung dieser Funktionen.
//
// Call:   #include "hal/q9_hal.h"
//
// Edition History
//─────────┬──────┬────────────────────────────────────────────────────────────────────────┬──────
// Date    │ Ver. │ Description                                                            │ By
//─────────┼──────┼────────────────────────────────────────────────────────────────────────┼──────
// 26-07-02│ 1.00 │ Initiale Version (Konsole, Timer, Block-Device, Target-Info)           │ CF
// 26-07-03│ 1.10 │ 1.9: q9_hal_time (Echtzeit-Quelle für F$Time)                          │ CF
// 26-07-10│ 1.20 │ 5.7: TX-Puffer-Auskunft (q9_hal_con_flush/tx_ready/tx_empty), damit    │ CF
//         │      │ die CB030-DUART-Emulation ehrliche TxRDY/TxEMT-Bits liefern kann        │
// 26-07-10│ 1.30 │ 5.9: q9_hal_sleep_ms — oeffentliche Schlaf-API fuer die Idle-Drossel    │ CF
//         │      │ des CB030-Runners (vorher nur internes usleep in hal_posix.c)           │
//═════════╧══════╧════════════════════════════════════════════════════════════════════════╧══════
#ifndef Q9_HAL_H
#define Q9_HAL_H

#include <stdint.h>

#define Q9_BLK_SIZE 512u

typedef struct q9_datetime {
    uint16_t year;                                     /* z.B. 2026                              */
    uint8_t  month;                                    /* 1..12                                  */
    uint8_t  day;                                      /* 1..31                                  */
    uint8_t  hour;                                     /* 0..23                                  */
    uint8_t  min;                                      /* 0..59                                  */
    uint8_t  sec;                                      /* 0..59                                  */
} q9_datetime_t;

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_hal_init
// Desc.:    Initialisiert die Target-Hardware (Konsole, Timer). Erster Aufruf vor allem anderen.
// Call:     q9_hal_init()
//════════════════════════════════════════════════════════════════════════════════════════════════
void q9_hal_init(void);

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_hal_con_put / q9_hal_con_get
// Desc.:    Konsolen-I/O. con_get ist nicht blockierend und liefert -1, wenn kein Zeichen ansteht.
// Call:     q9_hal_con_put('x');   c = q9_hal_con_get();
//════════════════════════════════════════════════════════════════════════════════════════════════
void q9_hal_con_put(char c);
int  q9_hal_con_get(void);

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_hal_con_flush / q9_hal_con_tx_ready / q9_hal_con_tx_empty
// Desc.:    5.7: con_put darf den Aufrufer nie blockieren — Targets mit einem echten TX-Puffer
//           (POSIX: Ringpuffer + nicht-blockierendes write()) versuchen bei jedem Aufruf so viel
//           wie moeglich auszuliefern; q9_hal_con_flush() gibt dem Host-Loop die Moeglichkeit,
//           das auch OHNE neue Ausgabe pro Hauptschleifen-Durchlauf zu wiederholen (Rest bleibt
//           sonst bis zum naechsten con_put liegen). tx_ready = Platz fuer mind. ein weiteres
//           Byte (TxRDY-Aequivalent); tx_empty = Puffer vollstaendig geleert (TxEMT-Aequivalent).
//           Targets ohne echten Puffer (Windows/wasm) liefern immer 1 bzw. tun nichts.
// Call:     q9_hal_con_flush();   if (q9_hal_con_tx_ready()) ...   if (q9_hal_con_tx_empty()) ...
//════════════════════════════════════════════════════════════════════════════════════════════════
void q9_hal_con_flush(void);
int  q9_hal_con_tx_ready(void);
int  q9_hal_con_tx_empty(void);

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_hal_sleep_ms
// Desc.:    5.9: Legt den aufrufenden Thread fuer ungefaehr 'ms' Millisekunden schlafen (POSIX:
//           usleep/nanosleep, Windows: Sleep). Fuer Idle-Drosseln im Host-Loop gedacht (z.B.
//           CB030-Runner, wenn die emulierte CPU per STOP angehalten ist) — kein Echtzeit-Timer,
//           kann laenger als angefordert dauern (Scheduler-Jitter), aber nie kuerzer im Normalfall.
// Call:     q9_hal_sleep_ms(1)
//════════════════════════════════════════════════════════════════════════════════════════════════
void q9_hal_sleep_ms(uint32_t ms);

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_hal_ticks_ms
// Desc.:    Monotoner Millisekunden-Zähler seit Start (wrappt nach ~49 Tagen, uint32).
// Call:     t = q9_hal_ticks_ms()
//════════════════════════════════════════════════════════════════════════════════════════════════
uint32_t q9_hal_ticks_ms(void);

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_hal_blk_read / q9_hal_blk_write
// Desc.:    Block-Device-Zugriff, Blockgröße Q9_BLK_SIZE. Rückgabe 0 = ok, -1 = Fehler.
//           Phase 0: native = Image-Datei, wasm = Stub (kommt mit Phase 3).
// Call:     q9_hal_blk_read(lba, buf);   q9_hal_blk_write(lba, buf);
//════════════════════════════════════════════════════════════════════════════════════════════════
int q9_hal_blk_read(uint32_t lba, void *buf);
int q9_hal_blk_write(uint32_t lba, const void *buf);

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_hal_time
// Desc.:    Liefert die lokale Uhrzeit des Hosts (native: localtime, wasm: Date, m68k: RTC).
//           Rückgabe 0 = ok, -1 = keine Zeitquelle vorhanden.
// Call:     q9_datetime_t dt; if (q9_hal_time(&dt) == 0) ...
//════════════════════════════════════════════════════════════════════════════════════════════════
int q9_hal_time(q9_datetime_t *dt);

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_hal_target
// Desc.:    Liefert einen kurzen Target-Namen fürs Boot-Banner, z.B. "native-win64".
// Call:     name = q9_hal_target()
//════════════════════════════════════════════════════════════════════════════════════════════════
const char *q9_hal_target(void);

#endif // Q9_HAL_H

//────────────────────────────────────────────────────────────────────────────────────────────────
// EOF q9_hal.h                                                                            Ver. 1.10
//────────────────────────────────────────────────────────────────────────────────────────────────
