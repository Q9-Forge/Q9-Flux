//════════════════════════════════════════════════════════════════════════════════════════════════
// File:   hal_wasm.c                                                                      Ver. 1.00
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

const char *q9_hal_target(void)
{
    return "wasm-browser";
}

//────────────────────────────────────────────────────────────────────────────────────────────────
// EOF hal_wasm.c                                                                          Ver. 1.00
//────────────────────────────────────────────────────────────────────────────────────────────────
