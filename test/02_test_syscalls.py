#═════════════════════════════════════════════════════════════════════════════════════════════════
# File:   02_test_syscalls.py                                                             Ver. 1.00
# Owner:  AF
# Desc.:  Syscall-Test: prüft, dass der Kernel-Selbsttest der OS-9-ABI durchläuft
#         (I$Read/Write/ReadLn/WritLn, F$Exit/ID/Time, Fehlercodes E$BPNum/E$UnkSvc/E$NotRdy).
#
# Call:   python test/02_test_syscalls.py   (aus dem Projekt-Root, nach "make native")
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
        print("02_test_syscalls: FAIL (build/native/q9.exe fehlt - erst 'make native')")
        return 1

    result = subprocess.run([EXE, "--selftest"], capture_output=True, text=True,
                            encoding="utf-8", errors="replace", timeout=15)

    checks = {
        "Exit-Code 0":            result.returncode == 0,
        "SYSCALL TEST PASS":      "SYSCALL TEST PASS" in result.stdout,
        "kein [FEHLER]":          "[FEHLER]" not in result.stdout,
    }

    for name, ok in checks.items():
        print(f"  [{'ok' if ok else 'FEHLER'}] {name}")

    if all(checks.values()):
        print("02_test_syscalls: PASS")
        return 0
    print("02_test_syscalls: FAIL")
    print(result.stdout)
    return 1


if __name__ == "__main__":
    sys.exit(main())

#─────────────────────────────────────────────────────────────────────────────────────────────────
# EOF 02_test_syscalls.py                                                                 Ver. 1.00
#─────────────────────────────────────────────────────────────────────────────────────────────────
