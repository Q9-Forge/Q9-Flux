//════════════════════════════════════════════════════════════════════════════════════════════════
// File:   07_hal_stub.c                                                                  Ver. 1.10
// Owner:  AF
// Desc.:  Minimal-HAL fuer Testziele, die devreg.c (dessen Typ-Registry g_device_types[]
//         unbedingt q9_devtype_duart68681 referenziert, s. dortiger Kommentar) oder direkt
//         src/devices/duart68681/duart68681.c bzw. src/devices/rtc72421/rtc72421.c linken, ohne
//         selbst UART/RTC-Codepfade zu erreichen -- diese Stubs loesen nur das Linken, ohne die
//         volle native HAL (hal_posix.c, mit eigener main()/Terminal-Rohmodus) einzubinden.
//
// Edition History
//─────────┬──────┬────────────────────────────────────────────────────────────────────────┬──────
// Date    │ Ver. │ Description                                                            │ By
//─────────┼──────┼────────────────────────────────────────────────────────────────────────┼──────
// 26-07-21│ 1.00 │ Initiale Version, s. 07_test_cf_sector512.c                             │ CF
// 26-08-21│ 1.10 │ Hardware-Vereinheitlichung: DUART/RTC nach eigene Dateien umgezogen,      │ Cld
//         │      │ dieser Stub jetzt von mehreren Testzielen + dem Editor-Makefile genutzt   │
//         │      │ (ueberall dort, wo devreg.c bzw. DEVDESC_SRC verlinkt wird)                │
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
