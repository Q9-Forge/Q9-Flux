//════════════════════════════════════════════════════════════════════════════════════════════════
// File:   07_hal_stub.c                                                                  Ver. 1.00
// Owner:  AF
// Desc.:  Minimal-HAL nur fuer 07_test_cf_sector512.c: cb030.c referenziert q9_hal_* (UART/RTC-
//         Codepfade), die der reine CF-Sektortest nie erreicht -- diese Stubs loesen nur das
//         Linken, ohne die volle native HAL (hal_posix.c, mit eigener main()/Terminal-Rohmodus)
//         einzubinden.
//
// Edition History
//─────────┬──────┬────────────────────────────────────────────────────────────────────────┬──────
// Date    │ Ver. │ Description                                                            │ By
//─────────┼──────┼────────────────────────────────────────────────────────────────────────┼──────
// 26-07-21│ 1.00 │ Initiale Version, s. 07_test_cf_sector512.c                             │ CF
//═════════╧══════╧════════════════════════════════════════════════════════════════════════╧══════
#include "../src/hal/q9_hal.h"

void q9_hal_con_put(char c)      { (void)c; }
int  q9_hal_con_get(void)        { return -1; }
int  q9_hal_con_tx_ready(void)   { return 1; }
int  q9_hal_con_tx_empty(void)   { return 1; }
int  q9_hal_time(q9_datetime_t *dt)
{
    if (dt) { dt->year = 2026; dt->month = 1; dt->day = 1; dt->hour = 0; dt->min = 0; dt->sec = 0; }
    return 0;
}

//────────────────────────────────────────────────────────────────────────────────────────────────
// EOF 07_hal_stub.c                                                                       Ver. 1.00
//────────────────────────────────────────────────────────────────────────────────────────────────
