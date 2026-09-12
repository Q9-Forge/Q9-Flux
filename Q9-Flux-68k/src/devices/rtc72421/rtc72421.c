//════════════════════════════════════════════════════════════════════════════════════════════════
// File:   rtc72421.c                                                                      Ver. 1.00
// Owner:  CF
// Desc.:  Implementierung, siehe rtc72421.h. Reine Verschiebung aus src/kernel/q9board.c
//         (2026-08-21, Hardware-Vereinheitlichung) -- Registerlogik UNVERAENDERT. Neu ist nur
//         q9_devdesc_rtc72421 am Dateiende.
//
// Edition History
//─────────┬──────┬────────────────────────────────────────────────────────────────────────┬──────
// Date    │ Ver. │ Description                                                            │ By
//─────────┼──────┼────────────────────────────────────────────────────────────────────────┬──────
// 26-07-14│ 1.xx │ 5.6/5.17: urspruenglich Teil von q9board.c, s. dortige Historie          │ CF
// 26-08-21│ 1.00 │ Hardware-Vereinheitlichung: aus q9board.c hierher verschoben, neu         │ Cld
//         │      │ q9_devdesc_rtc72421                                                       │
//═════════╧══════╧════════════════════════════════════════════════════════════════════════╧══════
#include "rtc72421.h"
#include "../../hal/q9_hal.h"
#include <string.h>

//────────────────────────────────────────────────────────────────────────────────────────────────
// 5.6: RTC72421 — Epson-Echtzeituhr am Bus ($FFFFD000, 16 Nibble-Register).
// Lesen = Host-Uhr (q9_hal_time), Schreiben wird ignoriert (s. q9board.h). Register:
//   0 S1  1 S10  2 MI1  3 MI10  4 H1  5 H10  6 D1  7 D10  8 MO1  9 MO10  A Y1  B Y10  C W
//   D Control D (HOLD/BUSY/IRQ — bei uns immer 0, nie busy)   E Control E (0)
//   F Control F (Bit2 = 24h-Modus, fest gesetzt)
// Ein Lesezugriff auf Register 0 frischt den Latch auf; die uebrigen Register lesen aus dem
// Latch, damit ein Treiber-Lesedurchlauf S1..W einen konsistenten Zeitstempel sieht.
//────────────────────────────────────────────────────────────────────────────────────────────────
static void board_rtc_refresh(q9_board_t *b)
{
    q9_datetime_t dt;
    if (q9_hal_time(&dt) != 0) {
        memset(b->rtc_regs, 0, sizeof(b->rtc_regs));
        b->rtc_latch_valid = 1;
        return;
    }
    /* Wochentag nach Sakamoto, 0 = Sonntag (uebliche 72421-Konvention) */
    static const int wt[] = {0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4};
    int y = dt.year, m = dt.month, d = dt.day;
    if (m < 3) {
        y -= 1;
    }
    int w = (y + y / 4 - y / 100 + y / 400 + wt[m - 1] + d) % 7;

    b->rtc_regs[0]  = (uint8_t)(dt.sec % 10);          /* S1   */
    b->rtc_regs[1]  = (uint8_t)(dt.sec / 10);          /* S10  */
    b->rtc_regs[2]  = (uint8_t)(dt.min % 10);          /* MI1  */
    b->rtc_regs[3]  = (uint8_t)(dt.min / 10);          /* MI10 */
    b->rtc_regs[4]  = (uint8_t)(dt.hour % 10);         /* H1   */
    b->rtc_regs[5]  = (uint8_t)(dt.hour / 10);         /* H10 (24h-Modus: 0..2, kein PM-Bit)     */
    b->rtc_regs[6]  = (uint8_t)(dt.day % 10);          /* D1   */
    b->rtc_regs[7]  = (uint8_t)(dt.day / 10);          /* D10  */
    b->rtc_regs[8]  = (uint8_t)(dt.month % 10);        /* MO1  */
    b->rtc_regs[9]  = (uint8_t)(dt.month / 10);        /* MO10 */
    b->rtc_regs[10] = (uint8_t)((dt.year % 100) % 10); /* Y1 (Basis 2000, wie rtclock-Treiber)   */
    b->rtc_regs[11] = (uint8_t)((dt.year % 100) / 10); /* Y10  */
    b->rtc_regs[12] = (uint8_t)w;                      /* W    */
    b->rtc_latch_valid = 1;
}

static uint8_t board_rtc_read(q9_board_t *b, uint32_t off)
{
    if (off == 0 || !b->rtc_latch_valid) {
        board_rtc_refresh(b);
    }
    if (off <= 12) {
        return b->rtc_regs[off];
    }
    if (off == 15) {
        return 0x04;                                   /* Control F: Bit2 = 24h-Modus            */
    }
    return 0x00;                                       /* Control D/E: nie HOLD/BUSY/IRQ         */
}

//────────────────────────────────────────────────────────────────────────────────────────────────
// Function: rtc_dev_* / q9_devtype_rtc72421
// Desc.:    5.17: Vtable-Adapter fuer die Geraete-Registry (devreg.h). board_rtc_read bleibt
//           unveraendert; Schreiben wird weiterhin komplett ignoriert (Host-Uhr ist die Wahrheit,
//           s. q9board.h). Kein IRQ.
//────────────────────────────────────────────────────────────────────────────────────────────────
static uint8_t rtc_dev_read8(q9_device_t *dev, uint32_t addr)
{
    return board_rtc_read((q9_board_t *)dev->state, addr - Q9_BOARD_RTC_BASE);
}

static void rtc_dev_write8(q9_device_t *dev, uint32_t addr, uint8_t val)
{
    (void)dev; (void)addr; (void)val;                 /* 5.6: Schreiben ignoriert (wie bisher)  */
}

const q9_device_vtable_t q9_devtype_rtc72421 = {
    .read8         = rtc_dev_read8,
    .write8        = rtc_dev_write8,
    .read16        = NULL,
    .write16       = NULL,
    .read32        = NULL,
    .write32       = NULL,
    .poll          = NULL,
    .irq_pending   = NULL,
    .reset         = NULL,
    .irq_vector_fn = NULL,
};

/* 2026-08-21 (Hardware-Vereinheitlichung, Folgeschritt nach dem "cf"-Piloten): q9_devdesc_
   rtc72421 -- noch OHNE extra_fields (kein Config-Schema, immer hartkodiert instanziiert, s.
   m68krt.c q9_m68krt_attach_board -- eigener, spaeterer Schritt). */
const q9_devdesc_t q9_devdesc_rtc72421 = {
    .type              = "rtc72421",
    .desc              = "RTC72421-Echtzeituhr (Host-Uhr, nur lesend)",
    .vt                = &q9_devtype_rtc72421,
    .use_table_default = 1,                            /* liegt im Fast-Table-Cluster              */
    .extra_fields      = NULL,
    .extra_field_count = 0,
};

//────────────────────────────────────────────────────────────────────────────────────────────────
// EOF rtc72421.c                                                                          Ver. 1.00
//────────────────────────────────────────────────────────────────────────────────────────────────
