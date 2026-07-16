//════════════════════════════════════════════════════════════════════════════════════════════════
// File:   cb030run.c                                                                      Ver. 1.60
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
// 26-07-13│ 1.40 │ 5.12: net_mode-Parameter -> q9_quicc_net_mode (nat|vmnet)               │ CF
// 26-07-14│ 1.50 │ 5.17: Hauptschleifen-Poll fuer DUART/Timer genericisiert (Geraete-        │ CF
//         │      │ Registry statt hartkodierter Bloecke), QUICC bleibt bis zu seinem eigenen │
//         │      │ 5.17-Schritt explizit verdrahtet                                          │
// 26-07-14│ 1.60 │ 5.17: QUICC ebenfalls umgezogen -- Hauptschleifen-Poll ist jetzt EINE      │ CF
//         │      │ einzige Schleife ueber die Geraete-Registry, keine Sonderfaelle mehr       │
//═════════╧══════╧════════════════════════════════════════════════════════════════════════╧══════
#include "cb030run.h"
#include "cb030.h"
#include "m68krt.h"
#include "quicc.h"
#include "devreg.h"
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

int q9_cb030_boot(const char *rom_path, const char *cf_path, const char *net_mode)
{
    static q9_cb030_t board;                           /* eine Instanz, wie Musashi selbst (5.1) */
    static q9_quicc_t quicc;                           /* 5.11: QUICC-Ethernet (SCC1)            */
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
    q9_quicc_init(&quicc, cb030_ram, sizeof(cb030_ram));
    if (q9_quicc_net_mode(&quicc, net_mode) != 0) {    /* 5.12: nat (Default) oder vmnet         */
        return 1;
    }
    q9_m68krt_attach_quicc(&quicc);                    /* 5.11: Ethernet-Fenster $FFFF2000       */
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

            /* 5.17: Hauptschleifen-Poll -- die frueher hier hartkodierten Bloecke (DUART/QUICC/
               Timer je einzeln verdrahtet) sind vollstaendig durch EINE Schleife ueber die
               Geraete-Registry ersetzt: erst poll() (falls vorhanden), danach irq_pending()
               unmittelbar im Anschluss (wichtig fuer den Timer -- s. cb030.c timer_dev_poll/
               timer_dev_irq_pending: der Merker gilt nur fuer GENAU diese Runde).
               WICHTIG fuer die Reihenfolge: q9_m68krt_set_irq() bildet nur EINE kombinierte
               Leitung nach (kein Bus mit unabhaengigen Level-Leitungen, s. m68krt.c-Kommentar bei
               m68krt_reassert_pending_irq) -- der LETZTE Aufruf in dieser Runde gewinnt. Die
               Registrierungsreihenfolge (m68krt.c: DUART 3, Netz-Terminals 4, QUICC 5, Timer 6 --
               s. Kommentare in q9_m68krt_attach_board/attach_quicc) ist deshalb bewusst
               aufsteigend nach IRQ-Level gehalten, damit bei gleichzeitig anstehenden Interrupts
               am Ende dieser Schleife das hoechste Level uebrig bleibt -- genau wie vor 5.17. */
            irq = 0;
            {
                int i, n = q9_devreg_count();
                for (i = 0; i < n; i++) {
                    q9_device_t *d = q9_devreg_get(i);
                    q9_device_poll(d, now_ms);
                    if (q9_device_irq_pending(d)) {
                        q9_m68krt_set_irq((unsigned int)d->irq_level);
                        irq = 1;
                    }
                }
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
                /* 5.15-TCP-Haenger-Diagnose: RXF steigt (ACKs treffen ein), aber qack friert ein
                   -> Level-5-Interrupt wird nicht zugestellt (Emulator). qack steigt weiter, Gast
                   haengt trotzdem -> Bug sitzt im geschlossenen Gast-Treiber. bsy>0 -> RX-Ring lief
                   voll und Frames wurden STILL (ohne sonstiges Log) verworfen. */
                fprintf(stderr, "[quiccdiag rxf=%u bsy=%u txb=%u qack=%u rxfull=%d pending=%d scce=%04x sccm=%04x]\n",
                        quicc.diag_rxf, quicc.diag_bsy, quicc.diag_txb,
                        q9_m68krt_quicc_acks(), q9_quicc_rx_filled(&quicc),
                        q9_quicc_irq_pending(&quicc),
                        q9_quicc_read16(&quicc, 0xFFFF2000u + 0x1610u),
                        q9_quicc_read16(&quicc, 0xFFFF2000u + 0x1614u));
                last_dbg_ms = now_ms;
            }
        }
    }
}

//────────────────────────────────────────────────────────────────────────────────────────────────
// EOF cb030run.c                                                                          Ver. 1.60
//────────────────────────────────────────────────────────────────────────────────────────────────
