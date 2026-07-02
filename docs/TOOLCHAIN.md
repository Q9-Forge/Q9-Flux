# TOOLCHAIN — Q9 Build-Umgebung

Installiert am 2026-07-02 (durch Claudia, Phase 0.1).

## Komponenten

| Werkzeug | Version | Ort | Zweck |
|----------|---------|-----|-------|
| w64devkit (gcc, make, busybox-sh) | 2.8.0 / gcc 16.1.0 | `C:\Users\foell\w64devkit` | nativer PC-Build |
| Emscripten SDK (emcc) | latest (emsdk) | `C:\Users\foell\emsdk` | WASM/Browser-Build |
| Python | 3.14 | `C:\Python314` | Tests, spätere Tools |
| Node.js | vorhanden | `C:\Programme\nodejs` | wird von emsdk genutzt |

Beides portable Installationen ohne Admin-Rechte — Deinstallation = Ordner löschen.

## Umgebung einrichten (pro Shell-Session)

**PowerShell:**
```powershell
$env:PATH = "C:\Users\foell\w64devkit\bin;$env:PATH"   # gcc + make
C:\Users\foell\emsdk\emsdk_env.ps1                      # emcc (nur für wasm-Builds nötig)
```

**cmd:**
```bat
set PATH=C:\Users\foell\w64devkit\bin;%PATH%
call C:\Users\foell\emsdk\emsdk_env.bat
```

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
