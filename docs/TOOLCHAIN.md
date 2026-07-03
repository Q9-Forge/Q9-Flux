# TOOLCHAIN — Q9 Build-Umgebung

Es wird auf zwei Rechnern gearbeitet — beide mit portablen Installationen
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
