#!/usr/bin/env sh
#═════════════════════════════════════════════════════════════════════════════════════════════════
# File:   build.sh                                                                        Ver. 1.00
# Owner:  AF
# Desc.:  Isolierter nativer Build fuer userland/libq9 + Beispiel-Tools + Testharness. Bindet
#         bewusst nicht das Root-Makefile ein und schreibt nur unter userland/build/.
#
# Call:   userland/build.sh
#
# Edition History
#─────────┬──────┬─────────────────────────────────────────────────────────────────────────┬──────
# Date    │ Ver. │ Description                                                             │ By
#─────────┼──────┼─────────────────────────────────────────────────────────────────────────┼──────
# 26-07-04│ 1.00 │ Initiale Version                                                        │ CX
# 26-07-04│ 1.01 │ q9dir in Build und Test aufgenommen                                     │ CX
# 26-07-04│ 1.02 │ q9mkdir/q9rm in Build und Test aufgenommen                              │ CX
# 26-07-04│ 1.03 │ q9touch/q9stat in Build und Test aufgenommen                            │ CX
#═════════╧══════╧═════════════════════════════════════════════════════════════════════════╧══════

set -eu

ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
BUILD="$ROOT/userland/build"
CC=${CC:-cc}
CFLAGS=${CFLAGS:-"-std=c99 -Wall -Wextra -O2"}
COMMON_FLAGS="-D_POSIX_C_SOURCE=200809L"

cd "$ROOT"
mkdir -p "$BUILD/obj"

KSRC="src/kernel/kernel.c src/kernel/syscall.c src/kernel/device.c src/kernel/dev_term.c src/kernel/dev_nil.c src/kernel/dev_d0.c src/kernel/name.c src/kernel/module.c src/kernel/vfs.c src/kernel/fat16.c src/kernel/proc.c"
USRC="userland/lib/libq9.c userland/tools/q9cat.c userland/tools/q9copy.c userland/tools/q9dir.c userland/tools/q9mkdir.c userland/tools/q9rm.c userland/tools/q9touch.c userland/tools/q9stat.c userland/test/harness.c"

for src in $KSRC $USRC; do
    obj="$BUILD/obj/$(printf '%s' "$src" | tr '/.' '__').o"
    "$CC" $CFLAGS $COMMON_FLAGS -c "$src" -o "$obj"
done

HAL_OBJ="$BUILD/obj/hal_posix.o"
"$CC" $CFLAGS $COMMON_FLAGS -Dmain=q9_hal_host_main -c src/hal/posix/hal_posix.c -o "$HAL_OBJ"

"$CC" "$BUILD"/obj/*.o -o "$BUILD/userland_test"
cd "$BUILD"
./userland_test

#─────────────────────────────────────────────────────────────────────────────────────────────────
# EOF build.sh                                                                            Ver. 1.00
#─────────────────────────────────────────────────────────────────────────────────────────────────
