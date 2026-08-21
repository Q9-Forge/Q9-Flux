//════════════════════════════════════════════════════════════════════════════════════════════════
// File:   timer_irq.c                                                                     Ver. 1.00
// Owner:  CF
// Desc.:  Implementierung, siehe timer_irq.h. Reine Verschiebung aus src/kernel/q9board.c
//         (2026-08-21, Hardware-Vereinheitlichung) -- Logik UNVERAENDERT. Neu ist nur
//         q9_devdesc_timer_irq am Dateiende.
//
// Edition History
//─────────┬──────┬────────────────────────────────────────────────────────────────────────┬──────
// Date    │ Ver. │ Description                                                            │ By
//─────────┼──────┼────────────────────────────────────────────────────────────────────────┬──────
// 26-07-04│ 1.xx │ 5.2d/5.6/5.17: urspruenglich Teil von q9board.c, s. dortige Historie     │ CF
// 26-08-21│ 1.00 │ Hardware-Vereinheitlichung: aus q9board.c hierher verschoben, neu         │ Cld
//         │      │ q9_devdesc_timer_irq; q9_board_poll_timer wurde `static`                  │
//═════════╧══════╧════════════════════════════════════════════════════════════════════════╧══════
#include "timer_irq.h"

/* 2026-08-21: `static` -- kein externer Aufrufer mehr ausser der eigenen Vtable weiter unten
   (verifiziert: weder q9boardrun.c/m68krt.c noch test/tools/ riefen diese Funktion je direkt auf,
   sie war seit 5.17 nur noch ueber timer_dev_poll erreicht). War zuvor q9_board_poll_timer
   (extern, q9board.h) -- Name unveraendert, nur die Sichtbarkeit eingeschraenkt. */
static int q9_board_poll_timer(q9_board_t *b, uint32_t now_ms)
{
    if (!b->timer_active) {
        return 0;
    }
    if (!b->timer_synced) {
        /* Erster Poll nach TI_IRQ_ON: sofort ausloesen und die Tick-Epoche starten
           (Verhalten wie bisher, s. Selbsttest 5.2d). */
        b->timer_synced  = 1;
        b->timer_last_ms = now_ms;
        return 1;
    }
    if (now_ms - b->timer_last_ms >= Q9_BOARD_TIMER_PERIOD_MS) {
        /* 5.6: Tick-Schulden nachholen statt verwerfen — vorher wurde timer_last_ms auf
           "jetzt" gesetzt, d.h. pro Poll hoechstens EIN Tick, egal wie viel Echtzeit
           vergangen war. Im Idle-Betrieb (STOP + Host-Schlafdrossel, 5.9) verlor die
           OS-9-Uhr dadurch fast alle Ticks und blieb praktisch stehen ('date' fror ein).
           Jetzt rueckt timer_last_ms nur um EINE Periode vor, so dass aufeinanderfolgende
           Polls die aufgelaufenen Ticks einzeln nachliefern (OS-9 zaehlt pro Interrupt
           genau einen Tick). Deckel bei 30 s Rueckstand, damit ein stundenlang
           schlafender Host keinen minutenlangen Tick-Sturm ausloest — den absoluten
           Abgleich liefert dann ohnehin die RTC (setime -s). */
        if (now_ms - b->timer_last_ms > 30000u) {
            b->timer_last_ms = now_ms - 30000u;
        }
        b->timer_last_ms += Q9_BOARD_TIMER_PERIOD_MS;
        return 1;
    }
    return 0;
}

//────────────────────────────────────────────────────────────────────────────────────────────────
// Function: timer_dev_* / q9_devtype_timer_irq
// Desc.:    5.17: Vtable-Adapter fuer die Geraete-Registry (devreg.h). TI_IRQ_ON/OFF sind reine
//           Adress-Trigger (kein Datenwert, Lesen wie Schreiben loesen dieselbe Wirkung aus, s.
//           q9board.h) -- read8/write8 fassen deshalb beide Fenster ($FFFF9000-$FFFF97FF OFF,
//           $FFFF9800-$FFFF9FFF ON) in EINEM Geraet zusammen und unterscheiden per Adresse.
//           poll()/irq_pending() bilden den bisherigen q9boardrun.c-Aufruf ab (q9_board_poll_timer
//           liefert 1 GENAU IN DER RUNDE, in der ein Tick faellig ist): poll() ruft ihn auf und
//           merkt sich das Ergebnis transient in b->timer_irq_pending; irq_pending() liest nur
//           diesen Merker (kein erneuter Seiteneffekt). level_held=0 (s. devreg.h) haelt den
//           Timer bewusst aus der IACK-/Reassert-Pruefschleife in m68krt.c heraus -- exakt wie
//           vor 5.17 (Level 6 faellt beim IACK immer auf den Autovektor, reassert_pending_irq
//           griff nie fuer den Timer).
//────────────────────────────────────────────────────────────────────────────────────────────────
static uint8_t timer_dev_read8(q9_device_t *dev, uint32_t addr)
{
    q9_board_t *b = (q9_board_t *)dev->state;
    if (addr >= Q9_BOARD_TIRQ_ON_BASE && addr <= Q9_BOARD_TIRQ_ON_TOP) {
        b->timer_active = 1;
        b->timer_synced = 0;                          /* 5.6: Tick-Epoche neu starten            */
    } else {
        b->timer_active = 0;
    }
    return 0;
}

static void timer_dev_write8(q9_device_t *dev, uint32_t addr, uint8_t val)
{
    (void)val;
    (void)timer_dev_read8(dev, addr);
}

static void timer_dev_poll(q9_device_t *dev, uint32_t now_ms)
{
    q9_board_t *b = (q9_board_t *)dev->state;
    b->timer_irq_pending = q9_board_poll_timer(b, now_ms);
}

static int timer_dev_irq_pending(q9_device_t *dev)
{
    return ((q9_board_t *)dev->state)->timer_irq_pending;
}

const q9_device_vtable_t q9_devtype_timer_irq = {
    .read8         = timer_dev_read8,
    .write8        = timer_dev_write8,
    .read16        = NULL,
    .write16       = NULL,
    .read32        = NULL,
    .write32       = NULL,
    .poll          = timer_dev_poll,
    .irq_pending   = timer_dev_irq_pending,
    .reset         = NULL,
    .irq_vector_fn = NULL,                            /* Level 6 faellt immer zum Autovektor     */
};

/* 2026-08-21 (Hardware-Vereinheitlichung, Folgeschritt nach dem "cf"-Piloten): q9_devdesc_
   timer_irq -- noch OHNE extra_fields (kein Config-Schema, immer hartkodiert instanziiert, s.
   m68krt.c q9_m68krt_attach_quicc -- eigener, spaeterer Schritt). */
const q9_devdesc_t q9_devdesc_timer_irq = {
    .type              = "timer_irq",
    .desc              = "Timer/IRQ3-Adress-Trigger (100 Hz, kooperativ)",
    .vt                = &q9_devtype_timer_irq,
    .use_table_default = 1,                            /* liegt im Fast-Table-Cluster              */
    .extra_fields      = NULL,
    .extra_field_count = 0,
};

//────────────────────────────────────────────────────────────────────────────────────────────────
// EOF timer_irq.c                                                                         Ver. 1.00
//────────────────────────────────────────────────────────────────────────────────────────────────
