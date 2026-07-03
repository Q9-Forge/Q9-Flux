#═════════════════════════════════════════════════════════════════════════════════════════════════
# File:   05_test_vfs.py                                                                  Ver. 1.00
# Owner:  AF
# Desc.:  VFS-Test (Phase 3.2): prüft, dass I$Open Geräte ohne File-Manager (Rest-Pfad muss leer
#         sein, Rückwärtskompatibilität), Geräte MIT File-Manager (Rest-Pfad wird durchgereicht,
#         hier ueber einen Test-File-Manager im Selbsttest) und I$ChgDir + relative Pfadaufloesung
#         korrekt behandelt. Kein echtes Dateisystem (FAT16 kommt in 3.3/3.4) — reines Routing.
#
# Call:   python test/05_test_vfs.py   (aus dem Projekt-Root, nach "make native")
#
# Edition History
#─────────┬──────┬─────────────────────────────────────────────────────────────────────────┬──────
# Date    │ Ver. │ Description                                                             │ By
#─────────┼──────┼─────────────────────────────────────────────────────────────────────────┼──────
# 26-07-04│ 1.00 │ Initiale Version                                                        │ CF
#═════════╧══════╧═════════════════════════════════════════════════════════════════════════╧══════
import os
import subprocess
import sys

EXE = os.path.join("build", "native", "q9.exe")


def main() -> int:
    if not os.path.exists(EXE):
        print("05_test_vfs: FAIL (build/native/q9.exe fehlt - erst 'make native')")
        return 1

    result = subprocess.run([EXE, "--selftest"], capture_output=True, text=True,
                            encoding="utf-8", errors="replace", timeout=15)

    checks = {
        "Exit-Code 0":                          result.returncode == 0,
        "I$Open ohne File-Manager (Rest leer)": "[ok] I$Open: /term/xyz -> E$PNNF, /term -> Pfad 3" in result.stdout,
        "I$Open mit File-Manager (Routing)":    "[ok] I$Open: /d0/datei ueber Test-File-Manager, /d0/unbekannt -> E$PNNF" in result.stdout,
        "I$ChgDir + relatives I$Open":          "[ok] I$ChgDir /d0 + relatives I$Open 'datei' -> Pfad 3" in result.stdout,
        "I$Create/I$MakDir/I$Delete-Geruest":   "[ok] I$Create/I$MakDir/I$Delete-Geruest -> E$UnkSvc" in result.stdout,
    }

    for name, ok in checks.items():
        print(f"  [{'ok' if ok else 'FEHLER'}] {name}")

    if all(checks.values()):
        print("05_test_vfs: PASS")
        return 0
    print("05_test_vfs: FAIL")
    print(result.stdout)
    return 1


if __name__ == "__main__":
    sys.exit(main())

#─────────────────────────────────────────────────────────────────────────────────────────────────
# EOF 05_test_vfs.py                                                                      Ver. 1.00
#─────────────────────────────────────────────────────────────────────────────────────────────────
