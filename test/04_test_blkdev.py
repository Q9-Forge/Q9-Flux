#═════════════════════════════════════════════════════════════════════════════════════════════════
# File:   04_test_blkdev.py                                                               Ver. 1.00
# Owner:  AF
# Desc.:  Block-Device-Test (Phase 3.1): prüft, dass /d0 über SS.BlkWr/SS.BlkRd Roundtrips
#         gegen die HAL (local_images/q9disk.img) macht und Fehlerfälle (NULL-Puffer, unbekannter SS-Code)
#         korrekt meldet. local_images/q9disk.img wird von jedem --selftest-Lauf (mit-)erzeugt und ist
#         .gitignore't — kein eigenes Aufräumen hier noetig.
#
# Call:   python test/04_test_blkdev.py   (aus dem Projekt-Root, nach "make native")
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
        print("04_test_blkdev: FAIL (build/native/q9.exe fehlt - erst 'make native')")
        return 1

    result = subprocess.run([EXE, "--selftest"], capture_output=True, text=True,
                            encoding="utf-8", errors="replace", timeout=15)

    checks = {
        "Exit-Code 0":                   result.returncode == 0,
        "Banner nennt /d0":              "/nil, /d0" in result.stdout,
        "SS.BlkWr/SS.BlkRd Roundtrip":   "[ok] /d0: SS.BlkWr/SS.BlkRd Roundtrip (LBA 1)" in result.stdout,
        "NULL-Puffer/unbek. SS-Code":    "[ok] /d0: NULL-Puffer -> E$Param, unbek. SS-Code -> E$UnkSvc" in result.stdout,
    }

    for name, ok in checks.items():
        print(f"  [{'ok' if ok else 'FEHLER'}] {name}")

    if all(checks.values()):
        print("04_test_blkdev: PASS")
        return 0
    print("04_test_blkdev: FAIL")
    print(result.stdout)
    return 1


if __name__ == "__main__":
    sys.exit(main())

#─────────────────────────────────────────────────────────────────────────────────────────────────
# EOF 04_test_blkdev.py                                                                   Ver. 1.00
#─────────────────────────────────────────────────────────────────────────────────────────────────
