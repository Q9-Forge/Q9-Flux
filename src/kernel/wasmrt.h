//════════════════════════════════════════════════════════════════════════════════════════════════
// File:   wasmrt.h                                                                        Ver. 1.00
// Owner:  AF
// Desc.:  Schmaler Q9-Wrapper um die eingebettete wasm3-Bibliothek (third_party/wasm3, Entscheidung
//         E10 in PROJECT.md, loest Architekturfrage O5). Native-Build-only — im Browser laeuft Q9
//         selbst schon als WASM, dort uebernimmt WebAssembly.instantiate diese Rolle (O6).
//         Schritt 4.6 (ARBEITSPLAN.md): reiner Grundbaustein OHNE Syscall-Bridge — laedt ein
//         beliebiges .wasm-Modul und ruft eine exportierte Funktion mit zwei i32-Parametern auf.
//         Die Syscall-Bridge (Import-Tabelle Richtung Q9-Kernel) kommt erst mit Schritt 4.7.
//
// Call:   q9_wasmrt_t rt; q9_wasmrt_init(&rt);
//         q9_wasmrt_load(&rt, bytes, len);
//         q9_wasmrt_call_i32(&rt, "add", 2, 3, &result);
//         q9_wasmrt_free(&rt);
//
// Edition History
//─────────┬──────┬────────────────────────────────────────────────────────────────────────┬──────
// Date    │ Ver. │ Description                                                            │ By
//─────────┼──────┼────────────────────────────────────────────────────────────────────────┼──────
// 26-07-04│ 1.00 │ 4.6: Erster Grundbaustein — Laden + Ausfuehren eines add(a,b)-Testmoduls │ CF
//═════════╧══════╧════════════════════════════════════════════════════════════════════════╧══════
#ifndef Q9_WASMRT_H
#define Q9_WASMRT_H

#include <stdint.h>

/* Fehlercodes — bewusst eigenstaendig (noch keine Q9-Syscall-Fehlercodes, s. syscall.h), weil
   diese API vor der Syscall-Bridge (4.7) noch nicht von aussen ueber F$Fork/F$Chain erreichbar
   ist. Die Zuordnung zu E$-Codes (falls ueberhaupt noetig) folgt mit 4.7. */
#define Q9_WASMRT_OK          0
#define Q9_WASMRT_ERR_ENV    -1                               /* Environment/Runtime-Anlage fehlgeschlagen */
#define Q9_WASMRT_ERR_PARSE  -2                               /* Modul-Bytes sind kein gueltiges .wasm     */
#define Q9_WASMRT_ERR_LOAD   -3                               /* Laden ins Runtime fehlgeschlagen          */
#define Q9_WASMRT_ERR_FIND   -4                               /* Export-Funktion nicht gefunden            */
#define Q9_WASMRT_ERR_CALL   -5                               /* Aufruf/Ergebnis-Abholung fehlgeschlagen   */

/* Opaque Handle: haelt Environment+Runtime+Modul von wasm3. Die wasm3-Typen selbst bleiben in
   wasmrt.c gekapselt (void*), damit kein Aufrufer wasm3.h einbinden muss. */
typedef struct q9_wasmrt {
    void *env;                                                 /* IM3Environment                      */
    void *runtime;                                              /* IM3Runtime                           */
    void *module;                                                /* IM3Module (NULL bis q9_wasmrt_load) */
} q9_wasmrt_t;

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_wasmrt_init
// Desc.:    Legt Environment + Runtime an (fester Stack, s. wasmrt.c). Muss vor q9_wasmrt_load
//           aufgerufen werden. Nutzt wasm3s eigene Heap-Allokation (malloc) — bewusste Ausnahme
//           von Q9s "kein malloc im Kernel"-Regel, siehe third_party/wasm3/README.md + PROJECT.md
//           Entscheidung E10; betrifft NUR den vendorten wasm3-Code, nicht diesen Wrapper.
// Call:     err = q9_wasmrt_init(&rt)
//════════════════════════════════════════════════════════════════════════════════════════════════
int q9_wasmrt_init(q9_wasmrt_t *rt);

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_wasmrt_load
// Desc.:    Parst und laedt ein .wasm-Modul aus einem Speicherblock in die Runtime. Ersetzt ein
//           zuvor geladenes Modul nicht — pro q9_wasmrt_t genau ein Modul (reicht fuer 4.6; die
//           Syscall-Bridge in 4.7 legt pro Prozess eine eigene q9_wasmrt_t an).
// Call:     err = q9_wasmrt_load(&rt, bytes, len)
//════════════════════════════════════════════════════════════════════════════════════════════════
int q9_wasmrt_load(q9_wasmrt_t *rt, const uint8_t *bytes, uint32_t len);

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_wasmrt_call_i32
// Desc.:    Sucht eine exportierte Funktion per Name und ruft sie mit genau zwei i32-Parametern
//           auf (Beweis-Fall fuer 4.6: add(a,b)). Liefert das i32-Ergebnis ueber *out. Allgemeinere
//           Signaturen (variable Parameterzahl/-typen, Zeiger-Marshaling) kommen erst mit 4.7/4.8.
// Call:     err = q9_wasmrt_call_i32(&rt, "add", 2, 3, &result)
//════════════════════════════════════════════════════════════════════════════════════════════════
int q9_wasmrt_call_i32(q9_wasmrt_t *rt, const char *funcname, int32_t a, int32_t b, int32_t *out);

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_wasmrt_free
// Desc.:    Gibt Runtime + Environment wieder frei (wasm3-eigene Buchhaltung, s.o.). Nach dem
//           Aufruf ist *rt nicht mehr benutzbar, ausser durch erneutes q9_wasmrt_init.
// Call:     q9_wasmrt_free(&rt)
//════════════════════════════════════════════════════════════════════════════════════════════════
void q9_wasmrt_free(q9_wasmrt_t *rt);

#endif // Q9_WASMRT_H

//────────────────────────────────────────────────────────────────────────────────────────────────
// EOF wasmrt.h                                                                            Ver. 1.00
//────────────────────────────────────────────────────────────────────────────────────────────────
