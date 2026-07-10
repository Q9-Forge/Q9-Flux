//════════════════════════════════════════════════════════════════════════════════════════════════
// File:   cb030run.c                                                                      Ver. 1.10
// Owner:  AF
// Desc.:  Implementierung des CB030-Boot-Runners, siehe cb030run.h.
//
// Edition History
//─────────┬──────┬────────────────────────────────────────────────────────────────────────┬──────
// Date    │ Ver. │ Description                                                            │ By
//─────────┼──────┼────────────────────────────────────────────────────────────────────────┬──────
// 26-07-05│ 1.00 │ 5.3: Erster Boot-Runner                                                 │ CF
// 26-07-05│ 1.10 │ 5.5a: cf_path-Parameter (NULL = Default CB030_CF_IMAGE)                 │ CF
// 26-07-10│ 1.20 │ 5.7: q9_hal_con_flush() pro Runde -- TX-Ringpuffer-Rest ausliefern,     │ CF
//         │      │ auch ohne neues THRA-Byte im selben Durchlauf                           │
// 26-07-10│ 1.30 │ 5.9: Idle-Drossel -- q9_hal_sleep_ms(1) statt Busy-Loop, wenn die CPU    │ CF
//         │      │ per STOP angehalten ist UND kein IRQ ansteht (OS-9-Leerlauf)             │
//═════════╧══════╧════════════════════════════════════════════════════════════════════════╧══════
#include "cb030run.h"
#include "cb030.h"
#include "m68krt.h"
#include "../hal/q9_hal.h"
#include <stdio.h>
#include <stdlib.h>

#define CB030_RAM_BYTES   (16u * 1024u * 1024u)       /* 16 MByte SIM-Bestueckung (docs/CB030.md) */
#define CB030_ROM_MAX     (512u * 1024u)              /* 29F040-Flash: 512 KByte                  */
#define CB030_CF_IMAGE    "cb030_cf.img"              /* Backing-Datei, lazy angelegt (5.2c)      */
#define CB030_SLICE_CYCLES 20000                       /* CPU-Takte je Runde zwischen Timer-Polls  */

/* Statisch statt Host-malloc (Q9-Grundsatz, vgl. Fixed-Heap-Entscheidung 4.9) — native-only,
   im BSS kostet das nichts, solange es unberuehrt bleibt. */
static uint8_t cb030_ram[CB030_RAM_BYTES];
static uint8_t cb030_rom[CB030_ROM_MAX];

int q9_cb030_boot(const char *rom_path, const char *cf_path)
{
    static q9_cb030_t board;                           /* eine Instanz, wie Musashi selbst (5.1) */
    q9_m68krt_t       rt;
    uint32_t          rom_len = 0;

    if (cf_path == NULL || cf_path[0] == '\0') {
        cf_path = CB030_CF_IMAGE;                       /* Andreas' fertige Images bleiben unangetastet */
    }

    if (q9_cb030_rom_load(rom_path, cb030_rom, sizeof(cb030_rom), &rom_len) != Q9_CB030_OK) {
        fprintf(stderr, "cb030: ROM-Datei '%s' nicht lesbar (fehlt, leer oder > %u KByte)\n",
                rom_path, CB030_ROM_MAX / 1024u);
        return 1;
    }

    printf("cb030: ROM '%s' geladen (%u Byte), %u MByte RAM, CF -> %s — Reset.\n",
           rom_path, rom_len, CB030_RAM_BYTES / (1024u * 1024u), cf_path);
    fflush(stdout);                                    /* Banner raus, bevor der CPU-Loop beginnt */

    q9_cb030_init(&board, cb030_rom, rom_len, cb030_ram, sizeof(cb030_ram));
    q9_cb030_cf_attach(&board, cf_path);

    q9_m68krt_init(&rt, cb030_ram, sizeof(cb030_ram));
    q9_m68krt_attach_board(&board);                    /* ab jetzt laeuft ALLES ueber das Board  */
    q9_m68krt_reset(&rt);                              /* Reset-Vektoren kommen aus dem ROM      */

    {
        /* Q9_CB030_DEBUG=1 in der Umgebung: alle ~3s CPU-Zustand auf stderr (PC/SR/IACK-Zaehler
           + Board-Zustand) — das Werkzeug, mit dem der erste OS-9-Boot durchdebuggt wurde. */
        int      dbg         = getenv("Q9_CB030_DEBUG") != 0;
        uint32_t last_dbg_ms = q9_hal_ticks_ms();

        for (;;) {
            int      irq;
            uint32_t now_ms;

            q9_m68krt_execute(&rt, CB030_SLICE_CYCLES);
            q9_hal_con_flush();                             /* 5.7: TX-Rest aus vorherigen Runden   */
            now_ms = q9_hal_ticks_ms();

            /* Timer (100Hz, Autovektor 27) und DUART (vektorisiert, IVR) teilen sich IRQ3 —
               die Vektor-Auswahl macht der IACK-Callback (m68krt.c). Timer-Poll immer
               ausfuehren (haelt last_ms aktuell). */
            irq  = q9_cb030_poll_timer(&board, now_ms);
            irq |= q9_cb030_uart_irq_pending(&board);
            if (irq) {
                q9_m68krt_set_irq(3);
            }

            /* 5.9: OS-9 idlet per STOP -- m68k_execute() "verbrennt" dann sofort alle
               angeforderten Takte, ohne etwas zu tun (busy loop, 100% Host-CPU). Ohne anstehenden
               IRQ kann in dieser Zeit nichts passieren, bevor der naechste 10ms-Timer-Tick (oder
               ein DUART-Interrupt) die CPU sowieso weckt -- also kurz schlafen statt sofort
               weiterzudrehen. q9_hal_ticks_ms() bleibt Wanduhr-basiert, die OS-9-Uhr geht also
               nicht falsch. */
            if (!irq && q9_m68krt_is_stopped()) {
                q9_hal_sleep_ms(1);
            }

            if (dbg && now_ms - last_dbg_ms >= 3000u) {
                uint32_t pc, sr, acks;
                q9_m68krt_debug_state(&pc, &sr, &acks);
                fprintf(stderr, "\n[dbg pc=%08x sr=%04x acks=%u imr=%02x rxfifo=%u rxovf=%u timer=%d]\n",
                        pc, sr, acks, board.uart_imr, board.uart_rx_count,
                        board.uart_rx_overflow, board.timer_active);
                last_dbg_ms = now_ms;
            }
        }
    }
}

//────────────────────────────────────────────────────────────────────────────────────────────────
// EOF cb030run.c                                                                          Ver. 1.00
//────────────────────────────────────────────────────────────────────────────────────────────────
