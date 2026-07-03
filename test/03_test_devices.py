#═════════════════════════════════════════════════════════════════════════════════════════════════
# File:   03_test_devices.py                                                              Ver. 1.00
# Owner:  AF
# Desc.:  Device-Modell-Test (Phase 1.3): prüft, dass die Standardpfade 0/1/2 über das
#         /term-Device laufen, Mode-Check (E$BMode) greift und close den Pfad freigibt.
#
# Call:   python test/03_test_devices.py   (aus dem Projekt-Root, nach "make native")
#
# Edition History
#─────────┬──────┬─────────────────────────────────────────────────────────────────────────┬──────
# Date    │ Ver. │ Description                                                             │ By
#─────────┼──────┼─────────────────────────────────────────────────────────────────────────┼──────
# 26-07-03│ 1.00 │ Initiale Version                                                        │ CF
#═════════╧══════╧═════════════════════════════════════════════════════════════════════════╧══════
import os
import subprocess
import sys

EXE = os.path.join("build", "native", "q9.exe")


def main() -> int:
    if not os.path.exists(EXE):
        print("03_test_devices: FAIL (build/native/q9.exe fehlt - erst 'make native')")
        return 1

    result = subprocess.run([EXE, "--selftest"], capture_output=True, text=True,
                            encoding="utf-8", errors="replace", timeout=15)

    checks = {
        "Exit-Code 0":                 result.returncode == 0,
        "Banner nennt /term":          "/term (Pfade 0/1/2)" in result.stdout,
        "Pfade 0/1/2 -> /term":        "[ok] Pfade 0/1/2 -> /term" in result.stdout,
        "Mode-Check E$BMode":          "[ok] I$Write auf Lesepfad -> E$BMode" in result.stdout,
        "close gibt Pfad frei":        "[ok] q9_path_close" in result.stdout,
        "/nil-Device funktioniert":    "[ok] /nil: Write verwirft, Read -> E$EOF" in result.stdout,
    }

    for name, ok in checks.items():
        print(f"  [{'ok' if ok else 'FEHLER'}] {name}")

    if all(checks.values()):
        print("03_test_devices: PASS")
        return 0
    print("03_test_devices: FAIL")
    print(result.stdout)
    return 1


if __name__ == "__main__":
    sys.exit(main())

#─────────────────────────────────────────────────────────────────────────────────────────────────
# EOF 03_test_devices.py                                                                  Ver. 1.00
#─────────────────────────────────────────────────────────────────────────────────────────────────
