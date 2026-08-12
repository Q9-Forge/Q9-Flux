#!/bin/sh
#═════════════════════════════════════════════════════════════════════════════════════════════════
# File:   fetch-isa-tests.sh                                                              Ver. 1.00
# Owner:  Claudia
# Desc.:  Holt und baut die offizielle RISC-V-ISA-Testsuite (riscv-tests) fuer den Prueflauf des
#         vendorierten CPU-Kerns. Die gebauten ELF-Dateien landen unter build/riscv-tests/rv<n>/
#         und werden NICHT eingecheckt -- es sind Fremdmaterial-Artefakte.
#
# Call:   test/riscv/fetch-isa-tests.sh [32|64]        (Vorgabe: 32)
#         danach: make test-riscv
#
# Voraussetzung: riscv64-elf-gcc + riscv64-elf-binutils (Homebrew, oder eine andere Toolchain per
#         RISCV_PREFIX). ACHTUNG: die Suite erwartet standardmaessig das Praefix
#         "riscv64-unknown-elf-", Homebrew liefert aber "riscv64-elf-" -- deshalb setzen wir es
#         hier ausdruecklich.
#
# Gebaut werden nur die "p"-Varianten (physische Adressierung, ohne MMU). Die "v"-Varianten
# brauchen eine libc (string.h/stdint.h) und die MMU; die Homebrew-Formel bringt KEINE newlib mit,
# und fuer den reinen CPU-Nachweis sind sie auch nicht noetig.
#═════════════════════════════════════════════════════════════════════════════════════════════════
set -e

XLEN="${1:-32}"
PREFIX="${RISCV_PREFIX:-riscv64-elf-}"
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
SRC="$ROOT/build/riscv-tests/src"
OUT="$ROOT/build/riscv-tests/rv$XLEN"

if ! command -v "${PREFIX}gcc" >/dev/null 2>&1; then
    echo "fetch-isa-tests: ${PREFIX}gcc nicht gefunden." >&2
    echo "  macOS:  brew install riscv64-elf-gcc riscv64-elf-binutils" >&2
    echo "  sonst:  RISCV_PREFIX=<dein-praefix->  $0 $XLEN" >&2
    exit 1
fi

mkdir -p "$SRC" "$OUT"

if [ ! -d "$SRC/.git" ]; then
    echo "-> hole riscv-tests"
    git clone -q --depth 1 --recursive \
        https://github.com/riscv-software-src/riscv-tests "$SRC"
fi

echo "-> baue rv$XLEN p-Tests mit ${PREFIX}gcc"
# -k: die "v"-Varianten scheitern mangels libc; das soll die "p"-Varianten nicht aufhalten.
( cd "$OUT" && make -k -f "$SRC/isa/Makefile" src_dir="$SRC/isa" \
      XLEN="$XLEN" RISCV_PREFIX="$PREFIX" >/dev/null 2>&1 ) || true

n=$(ls "$OUT" 2>/dev/null | grep -v '\.dump$' | grep -c "^rv${XLEN}" || true)
if [ "$n" -eq 0 ]; then
    echo "fetch-isa-tests: keine Tests gebaut -- Toolchain pruefen." >&2
    exit 1
fi
echo "-> $n Tests in $OUT"
