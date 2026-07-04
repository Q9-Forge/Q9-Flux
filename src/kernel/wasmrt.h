//════════════════════════════════════════════════════════════════════════════════════════════════
// File:   wasmrt.h                                                                        Ver. 1.10
// Owner:  AF
// Desc.:  Schmaler Q9-Wrapper um die eingebettete wasm3-Bibliothek (third_party/wasm3, Entscheidung
//         E10 in PROJECT.md, loest Architekturfrage O5). Native-Build-only — im Browser laeuft Q9
//         selbst schon als WASM, dort uebernimmt WebAssembly.instantiate diese Rolle (O6).
//         Schritt 4.6: Grundbaustein (Laden + Aufrufen ohne Importe). Schritt 4.7: Parse/Load
//         getrennt, damit ein Aufrufer (wasmproc.c) zwischen beiden Schritten eigene Importe an
//         das Modul linken kann (m3_LinkRawFunction verlangt ein bereits geladenes Modul, s.
//         wasm3-Quelle m3_bind.c/FindAndLinkFunction) — deshalb Reihenfolge Parse -> Load -> Link
//         -> Call, nicht Parse -> Link -> Load.
//
// Call:   q9_wasmrt_t rt; q9_wasmrt_init(&rt);
//         q9_wasmrt_parse(&rt, bytes, len); q9_wasmrt_load(&rt);
//         q9_wasmrt_call_i32(&rt, "add", 2, 3, &result);
//         q9_wasmrt_free(&rt);
//
// Edition History
//─────────┬──────┬────────────────────────────────────────────────────────────────────────┬──────
// Date    │ Ver. │ Description                                                            │ By
//─────────┼──────┼────────────────────────────────────────────────────────────────────────┼──────
// 26-07-04│ 1.00 │ 4.6: Erster Grundbaustein — Laden + Ausfuehren eines add(a,b)-Testmoduls │ CF
// 26-07-04│ 1.10 │ 4.7: q9_wasmrt_load in q9_wasmrt_parse/q9_wasmrt_load aufgeteilt (Importe │ CF
//         │      │ muessen zwischen Parse und Load/Call gelinkt werden koennen);            │
//         │      │ q9_wasmrt_call_raw fuer void-Funktionen + Zugriff auf den rohen M3Result  │
//         │      │ (wasmproc.c braucht das, um den F$Exit-Trap von echten Fehlern zu         │
//         │      │ unterscheiden)                                                            │
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
   wasmrt.c gekapselt (void*); wasmproc.c (4.7) ist eine bewusste Ausnahme und castet rt.module
   selbst zurueck auf IM3Module, weil es zum Linken eigener Importe ohnehin wasm3.h braucht. */
typedef struct q9_wasmrt {
    void *env;                                                 /* IM3Environment                      */
    void *runtime;                                              /* IM3Runtime                           */
    void *module;                                                /* IM3Module (NULL bis q9_wasmrt_parse) */
    int   loaded;                    /* 1 = q9_wasmrt_load war erfolgreich (Modul haengt an der  */
                                      /* Runtime und wird von deren m3_FreeRuntime mitentsorgt);  */
                                      /* 0 = geparst, aber (noch) nicht geladen — q9_wasmrt_free   */
                                      /* muss das Modul dann selbst freigeben, sonst Leck          */
} q9_wasmrt_t;

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_wasmrt_init
// Desc.:    Legt Environment + Runtime an (fester Stack, s. wasmrt.c). Muss vor q9_wasmrt_parse
//           aufgerufen werden. Nutzt wasm3s eigene Heap-Allokation (malloc) — bewusste Ausnahme
//           von Q9s "kein malloc im Kernel"-Regel, siehe third_party/wasm3/README.md + PROJECT.md
//           Entscheidung E10; betrifft NUR den vendorten wasm3-Code, nicht diesen Wrapper.
// Call:     err = q9_wasmrt_init(&rt)
//════════════════════════════════════════════════════════════════════════════════════════════════
int q9_wasmrt_init(q9_wasmrt_t *rt);

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_wasmrt_parse
// Desc.:    Parst ein .wasm-Modul aus einem Speicherblock (nur Struktur-/Typpruefung, noch keine
//           Speicher-/Global-Initialisierung, kein Import-Linking). rt->module ist danach gesetzt.
// Call:     err = q9_wasmrt_parse(&rt, bytes, len)
//════════════════════════════════════════════════════════════════════════════════════════════════
int q9_wasmrt_parse(q9_wasmrt_t *rt, const uint8_t *bytes, uint32_t len);

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_wasmrt_load
// Desc.:    Laedt das zuvor geparste Modul (q9_wasmrt_parse) in die Runtime — Speicher/Globals/
//           Datensegmente werden initialisiert, das Modul bekommt eine Runtime zugewiesen. ERST
//           danach kann ein Aufrufer eigene Importe linken (m3_LinkRawFunction verlangt ein
//           bereits geladenes Modul) und danach Funktionen suchen/aufrufen.
// Call:     err = q9_wasmrt_load(&rt)
//════════════════════════════════════════════════════════════════════════════════════════════════
int q9_wasmrt_load(q9_wasmrt_t *rt);

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_wasmrt_call_i32
// Desc.:    Sucht eine exportierte Funktion per Name und ruft sie mit genau zwei i32-Parametern
//           auf (Beweis-Fall fuer 4.6: add(a,b)). Liefert das i32-Ergebnis ueber *out. Allgemeinere
//           Signaturen (variable Parameterzahl/-typen, Zeiger-Marshaling) kommen erst mit 4.8.
// Call:     err = q9_wasmrt_call_i32(&rt, "add", 2, 3, &result)
//════════════════════════════════════════════════════════════════════════════════════════════════
int q9_wasmrt_call_i32(q9_wasmrt_t *rt, const char *funcname, int32_t a, int32_t b, int32_t *out);

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_wasmrt_call_raw
// Desc.:    Sucht eine exportierte, parameterlose Funktion ohne Rueckgabewert (Q9-Prozess-Entry,
//           4.7: "q9_main") und ruft sie auf. Liefert das rohe wasm3-M3Result zurueck: NULL = die
//           Funktion ist normal zurueckgekehrt, sonst ein Fehler-/Trap-String. Anders als
//           q9_wasmrt_call_i32 wird der String NICHT auf einen Q9_WASMRT_ERR_*-Code abgebildet,
//           weil wasmproc.c (4.7) per Zeiger-Identitaet zwischen "F$Exit-Trap" (gewollter,
//           sauberer Prozessabschluss) und einem echten Laufzeitfehler unterscheiden muss.
// Call:     result = q9_wasmrt_call_raw(&rt, "q9_main")   // NULL = ok
//════════════════════════════════════════════════════════════════════════════════════════════════
const char *q9_wasmrt_call_raw(q9_wasmrt_t *rt, const char *funcname);

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_wasmrt_free
// Desc.:    Gibt Runtime + Environment wieder frei (wasm3-eigene Buchhaltung, s.o.). Nach dem
//           Aufruf ist *rt nicht mehr benutzbar, ausser durch erneutes q9_wasmrt_init.
// Call:     q9_wasmrt_free(&rt)
//════════════════════════════════════════════════════════════════════════════════════════════════
void q9_wasmrt_free(q9_wasmrt_t *rt);

#endif // Q9_WASMRT_H

//────────────────────────────────────────────────────────────────────────────────────────────────
// EOF wasmrt.h                                                                            Ver. 1.10
//────────────────────────────────────────────────────────────────────────────────────────────────
