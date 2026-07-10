//════════════════════════════════════════════════════════════════════════════════════════════════
// File:   hal_wasm.c                                                                      Ver. 1.21
// Owner:  AF
// Desc.:  HAL-Implementierung für das WASM/Browser-Target (Emscripten).
//         Konsole läuft über globalThis.q9host (definiert in web/worker.js), Block-Device über
//         globalThis.q9blk (OPFS-Sync-Access-Handle, ebenfalls worker.js). Kein main(): der
//         JS-Loader ruft _q9_kernel_init/_q9_kernel_step direkt auf (jetzt im Worker, seit 3.6).
//
// Call:   emcc ... src/hal/wasm/hal_wasm.c (siehe Makefile, Target "wasm")
//
// Edition History
//─────────┬──────┬────────────────────────────────────────────────────────────────────────┬──────
// Date    │ Ver. │ Description                                                            │ By
//─────────┼──────┼────────────────────────────────────────────────────────────────────────┬──────
// 26-07-02│ 1.00 │ Initiale Version: Konsole via q9host, Timer, Disk-Stubs                │ CF
// 26-07-03│ 1.10 │ 1.9: q9_hal_time via Date (ungetestet, emsdk fehlt auf AF-PC)          │ CF
// 26-07-04│ 1.20 │ 3.6: q9_hal_blk_read/write via globalThis.q9blk (OPFS-SyncAccessHandle │ CF
//         │      │ im Worker, web/worker.js); Modul läuft jetzt komplett im Worker         │
//         │      │ (ungetestet, emsdk fehlt lokal weiterhin)                              │
// 26-07-10│ 1.21 │ 5.7/5.9: HAL-Interface-Erfuellung (con_flush/tx_ready/tx_empty trivial, │ CF
//         │      │ q9_hal_sleep_ms no-op) -- cb030.c wird im wasm-Build nicht mitgebaut    │
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

EM_JS(int, js_blk_read, (uint32_t lba, uint8_t *buf), {
    var data = globalThis.q9blk.read(lba);             /* Uint8Array(512) oder null bei Fehler   */
    if (!data) return -1;
    HEAPU8.set(data, buf);
    return 0;
});

EM_JS(int, js_blk_write, (uint32_t lba, const uint8_t *buf), {
    var data = HEAPU8.slice(buf, buf + 512);
    return globalThis.q9blk.write(lba, data) ? 0 : -1;
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

/* 5.7: kein Software-TX-Puffer auf diesem Target -- js_con_put liefert synchron an den Worker,
   also ist der Puffer immer sofort leer. Reine Interface-Erfuellung (s. q9_hal.h); cb030.c (der
   einzige Aufrufer der neuen Funktionen) wird im wasm-Build ohnehin nicht mitkompiliert. */
void q9_hal_con_flush(void) { }
int  q9_hal_con_tx_ready(void) { return 1; }
int  q9_hal_con_tx_empty(void) { return 1; }

uint32_t q9_hal_ticks_ms(void)
{
    return (uint32_t)emscripten_get_now();
}

/* 5.9: kein echtes Schlafen im synchronen Worker-Kontext -- q9_kernel_step() wird ohnehin
   getaktet vom Browser aufgerufen (Muster wie q9_hal_con_flush oben), und cb030.c (der einzige
   Aufrufer der Idle-Drossel) wird im wasm-Build ohnehin nicht mitkompiliert. Reine Interface-
   Erfuellung (s. q9_hal.h). */
void q9_hal_sleep_ms(uint32_t ms)
{
    (void)ms;
}

int q9_hal_blk_read(uint32_t lba, void *buf)
{
    return js_blk_read(lba, (uint8_t *)buf);
}

int q9_hal_blk_write(uint32_t lba, const void *buf)
{
    return js_blk_write(lba, (const uint8_t *)buf);
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
// EOF hal_wasm.c                                                                          Ver. 1.20
//────────────────────────────────────────────────────────────────────────────────────────────────
