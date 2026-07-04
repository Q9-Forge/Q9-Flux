//════════════════════════════════════════════════════════════════════════════════════════════════
// File:   cb030run.c                                                                      Ver. 1.00
// Owner:  AF
// Desc.:  Implementierung des CB030-Boot-Runners, siehe cb030run.h.
//
// Edition History
//─────────┬──────┬────────────────────────────────────────────────────────────────────────┬──────
// Date    │ Ver. │ Description                                                            │ By
//─────────┼──────┼────────────────────────────────────────────────────────────────────────┬──────
// 26-07-05│ 1.00 │ 5.3: Erster Boot-Runner                                                 │ CF
//═════════╧══════╧════════════════════════════════════════════════════════════════════════╧══════
#include "cb030run.h"
#include "cb030.h"
#include "m68krt.h"
#include "../hal/q9_hal.h"
#include <stdio.h>

#define CB030_RAM_BYTES   (16u * 1024u * 1024u)       /* 16 MByte SIM-Bestueckung (docs/CB030.md) */
#define CB030_ROM_MAX     (512u * 1024u)              /* 29F040-Flash: 512 KByte                  */
#define CB030_CF_IMAGE    "cb030_cf.img"              /* Backing-Datei, lazy angelegt (5.2c)      */
#define CB030_SLICE_CYCLES 20000                       /* CPU-Takte je Runde zwischen Timer-Polls  */

/* Statisch statt Host-malloc (Q9-Grundsatz, vgl. Fixed-Heap-Entscheidung 4.9) — native-only,
   im BSS kostet das nichts, solange es unberuehrt bleibt. */
static uint8_t cb030_ram[CB030_RAM_BYTES];
static uint8_t cb030_rom[CB030_ROM_MAX];

int q9_cb030_boot(const char *rom_path)
{
    static q9_cb030_t board;                           /* eine Instanz, wie Musashi selbst (5.1) */
    q9_m68krt_t       rt;
    uint32_t          rom_len = 0;

    if (q9_cb030_rom_load(rom_path, cb030_rom, sizeof(cb030_rom), &rom_len) != Q9_CB030_OK) {
        fprintf(stderr, "cb030: ROM-Datei '%s' nicht lesbar (fehlt, leer oder > %u KByte)\n",
                rom_path, CB030_ROM_MAX / 1024u);
        return 1;
    }

    printf("cb030: ROM '%s' geladen (%u Byte), %u MByte RAM, CF -> %s — Reset.\n",
           rom_path, rom_len, CB030_RAM_BYTES / (1024u * 1024u), CB030_CF_IMAGE);
    fflush(stdout);                                    /* Banner raus, bevor der CPU-Loop beginnt */

    q9_cb030_init(&board, cb030_rom, rom_len, cb030_ram, sizeof(cb030_ram));
    q9_cb030_cf_attach(&board, CB030_CF_IMAGE);

    q9_m68krt_init(&rt, cb030_ram, sizeof(cb030_ram));
    q9_m68krt_attach_board(&board);                    /* ab jetzt laeuft ALLES ueber das Board  */
    q9_m68krt_reset(&rt);                              /* Reset-Vektoren kommen aus dem ROM      */

    for (;;) {
        q9_m68krt_execute(&rt, CB030_SLICE_CYCLES);
        if (q9_cb030_poll_timer(&board, q9_hal_ticks_ms())) {
            q9_m68krt_set_irq(3);                      /* 100Hz-Tick, kooperativ (5.2d)          */
        }
    }
}

//────────────────────────────────────────────────────────────────────────────────────────────────
// EOF cb030run.c                                                                          Ver. 1.00
//────────────────────────────────────────────────────────────────────────────────────────────────
