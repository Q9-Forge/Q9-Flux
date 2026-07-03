//════════════════════════════════════════════════════════════════════════════════════════════════
// File:   hal_wasm.c                                                                      Ver. 1.10
// Owner:  AF
// Desc.:  HAL-Implementierung für das WASM/Browser-Target (Emscripten).
//         Konsole läuft über globalThis.q9host (definiert in web/index.html, xterm.js).
//         Kein main(): der JS-Loader ruft _q9_kernel_init/_q9_kernel_step direkt auf.
//
// Call:   emcc ... src/hal/wasm/hal_wasm.c (siehe Makefile, Target "wasm")
//
// Edition History
//─────────┬──────┬────────────────────────────────────────────────────────────────────────┬──────
// Date    │ Ver. │ Description                                                            │ By
//─────────┼──────┼────────────────────────────────────────────────────────────────────────┬──────
// 26-07-02│ 1.00 │ Initiale Version: Konsole via q9host, Timer, Disk-Stubs                │ CF
// 26-07-03│ 1.10 │ 1.9: q9_hal_time via Date (ungetestet, emsdk fehlt auf AF-PC)          │ CF
//═════════╧══════╧════════════════════════════════════════════════════════════════════════╧══════

#include <emscripten.h>

#include "../q9_hal.h"

//╔══════════════════════════════════════════════════════════════════════════════════════════════╗
//║ JS GLUE                                                                                      ║
//╚══════════════════════════════════════════════════════════════════════════════════════════════╝

EM_JS(void, js_con_put, (int c), {
    globalThis.q9host.putc(c);
});

EM_JS(int, js_con_get, (), {
    return globalThis.q9host.getc();
});

EM_JS(int, js_date_ymd, (), {                          /* (year<<9) | (month<<5) | day           */
    const d = new Date();
    return (d.getFullYear() << 9) | ((d.getMonth() + 1) << 5) | d.getDate();
});

EM_JS(int, js_time_hms, (), {                          /* (hour<<12) | (min<<6) | sec            */
    const d = new Date();
    return (d.getHours() << 12) | (d.getMinutes() << 6) | d.getSeconds();
});

//╔══════════════════════════════════════════════════════════════════════════════════════════════╗
//║ HAL IMPLEMENTATION                                                                           ║
//╚══════════════════════════════════════════════════════════════════════════════════════════════╝

void q9_hal_init(void)
{
    /* browser host is already up when we get here */
}

void q9_hal_con_put(char c)
{
    js_con_put((int)(unsigned char)c);
}

int q9_hal_con_get(void)
{
    return js_con_get();
}

uint32_t q9_hal_ticks_ms(void)
{
    return (uint32_t)emscripten_get_now();
}

int q9_hal_blk_read(uint32_t lba, void *buf)
{
    (void)lba; (void)buf;
    return -1;                                         /* disk arrives with phase 3 (OPFS)       */
}

int q9_hal_blk_write(uint32_t lba, const void *buf)
{
    (void)lba; (void)buf;
    return -1;                                         /* disk arrives with phase 3 (OPFS)       */
}

int q9_hal_time(q9_datetime_t *dt)
{
    int ymd = js_date_ymd();
    int hms = js_time_hms();

    dt->year  = (uint16_t)(ymd >> 9);
    dt->month = (uint8_t)((ymd >> 5) & 0x0f);
    dt->day   = (uint8_t)(ymd & 0x1f);
    dt->hour  = (uint8_t)(hms >> 12);
    dt->min   = (uint8_t)((hms >> 6) & 0x3f);
    dt->sec   = (uint8_t)(hms & 0x3f);
    return 0;
}

const char *q9_hal_target(void)
{
    return "wasm-browser";
}

//────────────────────────────────────────────────────────────────────────────────────────────────
// EOF hal_wasm.c                                                                          Ver. 1.10
//────────────────────────────────────────────────────────────────────────────────────────────────
