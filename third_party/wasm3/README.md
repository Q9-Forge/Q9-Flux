# wasm3 (vendored)

Eingebettete WASM-Interpreter-Bibliothek für den nativen Q9-Build (Entscheidung
E10, PROJECT.md; Arbeitsschritt 4.6, ARBEITSPLAN.md). Löst Architekturfrage O5
(PROJECT.md).

- Quelle: https://github.com/wasm3/wasm3
- Commit: d77cd814aa0bc68cb1df917580a6304d34cfb30b (2026-06-26)
- Lizenz: MIT (siehe LICENSE in diesem Verzeichnis) — Code darf unverändert
  übernommen und weitergegeben werden.

## Umfang

Nur der Kern-Interpreter ist vendored (`m3_bind`, `m3_code`, `m3_compile`,
`m3_core`, `m3_env`, `m3_exec`, `m3_function`, `m3_info`, `m3_module`,
`m3_parse` + zugehörige Header). **Nicht** übernommen: `m3_api_wasi.c`,
`m3_api_libc.c`, `m3_api_uvwasi.c`, `m3_api_tracer.c` (WASI-/libc-Import-
Bindings, Tracing) — Q9 braucht keine WASI-Umgebung, die künftige
Syscall-Bridge (Schritt 4.7) verdrahtet eigene Q9-Importe direkt über
`m3_LinkRawFunction`.

Dateien sind **unverändert** aus dem Original übernommen (keine lokalen
Patches), damit ein späteres Update einfach ein Diff/Ersetzen ist.

## Bewusste Ausnahme von der Kernel-Regel "kein malloc"

wasm3 nutzt intern `malloc`/`realloc`/`free` (Environment/Runtime/Module-
Allokation, Compile-Page-Allokator) — das ist für einen allgemeinen
WASM-Interpreter mit dynamisch großen Modulen praktisch unvermeidbar und
architektonisch etwas anderes als Q9s eigene Kernel-Tabellen (Geräte, Pfade,
Prozesse, Module), die bewusst statisch sind. Diese Ausnahme gilt **nur**
für den vendorten wasm3-Code selbst, nicht für den Q9-seitigen Wrapper
(`src/kernel/wasmrt.c`) oder den restlichen Kernel. Siehe Entscheidung E10
in PROJECT.md.

## Build-Hinweis

wasm3 wird nur in den **nativen** Build eingebunden (Makefile-Ziel `native`),
nicht in `make wasm` — im Browser läuft Q9 selbst schon als WASM, ein
eingebetteter WASM-Interpreter im WASM-Build ist für 4.6/4.7 nicht nötig
(dort übernimmt `WebAssembly.instantiate` die Rolle, siehe O6 in
PROJECT.md). Der vendorte Code wird bewusst NICHT mit `-Wall -Wextra`
übersetzt (fremder Code, den wir nicht pflegen) — nur Q9s eigener Code
(inkl. `wasmrt.c`) muss warnungsfrei bleiben.
