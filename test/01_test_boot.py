#═════════════════════════════════════════════════════════════════════════════════════════════════
# File:   01_test_boot.py                                                                 Ver. 1.00
# Owner:  AF
# Desc.:  Boot-Test: startet den nativen Q9-Build im Selftest-Modus und prüft Banner + Exit-Code.
#
# Call:   python test/01_test_boot.py   (aus dem Projekt-Root, nach "make native")
#
# Edition History
#─────────┬──────┬─────────────────────────────────────────────────────────────────────────┬──────
# Date    │ Ver. │ Description                                                             │ By
#─────────┼──────┼─────────────────────────────────────────────────────────────────────────┼──────
# 26-07-02│ 1.00 │ Initiale Version                                                        │ CF
# 26-07-03│ 1.01 │ 1.10: Target-Check generalisiert ("native-" statt "native-win64")       │ CF
#═════════╧══════╧═════════════════════════════════════════════════════════════════════════╧══════
import os
import subprocess
import sys

EXE = os.path.join("build", "native", "q9.exe")


def main() -> int:
    if not os.path.exists(EXE):
        print("01_test_boot: FAIL (build/native/q9.exe fehlt - erst 'make native')")
        return 1

    result = subprocess.run([EXE, "--selftest"], capture_output=True, text=True,
                            encoding="utf-8", errors="replace", timeout=15)

    checks = {
        "Exit-Code 0":        result.returncode == 0,
        "Banner 'Q9 v'":      "Q9 v" in result.stdout,
        "Target-Zeile":       "native-" in result.stdout,
        "SELFTEST PASS":      "SELFTEST PASS" in result.stdout,
    }

    for name, ok in checks.items():
        print(f"  [{'ok' if ok else 'FEHLER'}] {name}")

    if all(checks.values()):
        print("01_test_boot: PASS")
        return 0
    print("01_test_boot: FAIL")
    return 1


if __name__ == "__main__":
    sys.exit(main())

#─────────────────────────────────────────────────────────────────────────────────────────────────
# EOF 01_test_boot.py                                                                     Ver. 1.00
#─────────────────────────────────────────────────────────────────────────────────────────────────
