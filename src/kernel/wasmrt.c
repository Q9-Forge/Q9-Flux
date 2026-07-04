//════════════════════════════════════════════════════════════════════════════════════════════════
// File:   wasmrt.c                                                                        Ver. 1.00
// Owner:  AF
// Desc.:  Implementierung des wasm3-Wrappers, siehe wasmrt.h. Kapselt alle wasm3-Typen als void*
//         nach aussen, damit kein Aufrufer third_party/wasm3/wasm3.h einbinden muss.
//
// Edition History
//─────────┬──────┬────────────────────────────────────────────────────────────────────────┬──────
// Date    │ Ver. │ Description                                                            │ By
//─────────┼──────┼────────────────────────────────────────────────────────────────────────┼──────
// 26-07-04│ 1.00 │ 4.6: Erster Grundbaustein                                               │ CF
//═════════╧══════╧════════════════════════════════════════════════════════════════════════╧══════
#include "wasmrt.h"
#include "wasm3.h"
#include <string.h>

/* Fester Stack fuer die wasm3-Ausfuehrung selbst (nicht zu verwechseln mit Q9s eigenen, statisch
   allozierten Kernel-Tabellen) — 4096 Byte reichen fuer die trivialen Testmodule aus 4.6 bei
   weitem; Bedarf wird mit 4.7/4.8 (echte Programme) neu bewertet. */
#define Q9_WASMRT_STACK_BYTES 4096u

int q9_wasmrt_init(q9_wasmrt_t *rt)
{
    memset(rt, 0, sizeof(*rt));

    IM3Environment env = m3_NewEnvironment();
    if (!env) {
        return Q9_WASMRT_ERR_ENV;
    }

    IM3Runtime runtime = m3_NewRuntime(env, Q9_WASMRT_STACK_BYTES, NULL);
    if (!runtime) {
        m3_FreeEnvironment(env);
        return Q9_WASMRT_ERR_ENV;
    }

    rt->env = env;
    rt->runtime = runtime;
    rt->module = NULL;
    return Q9_WASMRT_OK;
}

int q9_wasmrt_load(q9_wasmrt_t *rt, const uint8_t *bytes, uint32_t len)
{
    IM3Module module;
    M3Result result = m3_ParseModule((IM3Environment)rt->env, &module, bytes, len);
    if (result) {
        return Q9_WASMRT_ERR_PARSE;
    }

    result = m3_LoadModule((IM3Runtime)rt->runtime, module);
    if (result) {
        return Q9_WASMRT_ERR_LOAD;
    }

    rt->module = module;
    return Q9_WASMRT_OK;
}

int q9_wasmrt_call_i32(q9_wasmrt_t *rt, const char *funcname, int32_t a, int32_t b, int32_t *out)
{
    IM3Function func;
    M3Result result = m3_FindFunction(&func, (IM3Runtime)rt->runtime, funcname);
    if (result) {
        return Q9_WASMRT_ERR_FIND;
    }

    result = m3_CallV(func, a, b);
    if (result) {
        return Q9_WASMRT_ERR_CALL;
    }

    int32_t r = 0;
    result = m3_GetResultsV(func, &r);
    if (result) {
        return Q9_WASMRT_ERR_CALL;
    }

    *out = r;
    return Q9_WASMRT_OK;
}

void q9_wasmrt_free(q9_wasmrt_t *rt)
{
    if (rt->runtime) {
        m3_FreeRuntime((IM3Runtime)rt->runtime);
    }
    if (rt->env) {
        m3_FreeEnvironment((IM3Environment)rt->env);
    }
    memset(rt, 0, sizeof(*rt));
}

//────────────────────────────────────────────────────────────────────────────────────────────────
// EOF wasmrt.c                                                                            Ver. 1.00
//────────────────────────────────────────────────────────────────────────────────────────────────
