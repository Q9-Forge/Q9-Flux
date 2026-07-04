# TOOLCHAIN — Q9 Build-Umgebung

Es wird auf mehreren Rechnern gearbeitet — alle mit portablen Installationen
ohne Admin-Rechte (Deinstallation = Ordner löschen):

## Laptop (User `foell`, eingerichtet 2026-07-02, Phase 0.1)

| Werkzeug | Version | Ort | Zweck |
|----------|---------|-----|-------|
| w64devkit (gcc, make, busybox-sh) | 2.8.0 / gcc 16.1.0 | `C:\Users\foell\w64devkit` | nativer PC-Build |
| Emscripten SDK (emcc) | latest (emsdk) | `C:\Users\foell\emsdk` | WASM/Browser-Build |
| Python | 3.14 | `C:\Python314` | Tests, spätere Tools |
| Node.js | vorhanden | `C:\Programme\nodejs` | wird von emsdk genutzt |

## Desktop AF-PC (User `AF`, eingerichtet 2026-07-03)

| Werkzeug | Version | Ort | Zweck |
|----------|---------|-----|-------|
| w64devkit (gcc, make, busybox-sh) | 2.8.0 / gcc 16.1.0 | `C:\Users\AF\w64devkit` | nativer PC-Build |
| Emscripten SDK | **noch nicht installiert** | — | wasm-Build hier noch nicht möglich |
| Python | 3.14 | `C:\Users\AF\AppData\Local\Programs\Python\Python314` | Tests |

## Mac Mini (User `afoe`, eingerichtet 2026-07-03/04 — Autonomie-Rechner)

| Werkzeug | Version | Ort | Zweck |
|----------|---------|-----|-------|
| Xcode Command Line Tools (clang, make) | vorhanden | System | nativer Build (`hal_posix.c`, seit 1.10) |
| Python | 3 (Homebrew) | `/opt/homebrew/bin/python3` | Tests |
| Emscripten SDK (emcc) | 6.0.2 (latest, 2026-07-04) | `~/emsdk` | WASM/Browser-Build |
| Node.js | 22.16.0 (von emsdk mitinstalliert) | `~/emsdk/node/22.16.0_64bit` | wird von emsdk genutzt |

## Umgebung einrichten (pro Shell-Session)

**PowerShell** (`foell` durch `AF` ersetzen je nach Rechner):
```powershell
$env:PATH = "C:\Users\foell\w64devkit\bin;$env:PATH"   # gcc + make
C:\Users\foell\emsdk\emsdk_env.ps1                      # emcc (nur für wasm-Builds nötig)
```

**cmd:**
```bat
set PATH=C:\Users\foell\w64devkit\bin;%PATH%
call C:\Users\foell\emsdk\emsdk_env.bat
```

**macOS (Mac Mini), bash/zsh:**
```bash
source ~/emsdk/emsdk_env.sh   # emcc (nur für wasm-Builds nötig); make/clang sind ohnehin im PATH
```
Muss pro Shell-Session neu ausgeführt werden (setzt PATH/Env nur für die aktuelle Shell) —
`make native`/`make test` brauchen das nicht, nur `make wasm`.

## Bauen

```
make native   # -> build/native/q9.exe
make wasm     # -> build/wasm/q9.js + q9.wasm + index.html
make test     # nativer Selftest (test/01_test_boot.py)
make clean
```

Browser-Version lokal ausprobieren:
```
python -m http.server 8000 -d build/wasm
# dann http://localhost:8000 öffnen
```

## Noch nicht installiert (kommt später)

- **vbcc** (M68k-Backend) — ab Phase 7 für das Vinculum-Target
