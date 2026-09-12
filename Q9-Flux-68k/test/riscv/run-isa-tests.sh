#!/bin/sh
#═════════════════════════════════════════════════════════════════════════════════════════════════
# File:   run-isa-tests.sh                                                                Ver. 1.00
# Owner:  Claudia
# Desc.:  Laesst die RISC-V-ISA-Testsuite gegen den vendorierten CPU-Kern laufen und vergleicht
#         das Ergebnis mit dem FESTGEHALTENEN Stand. Gemeldet wird nur, was sich VERAENDERT hat.
#
# Warum nicht einfach "alles muss gruen sein": der Kern (TinyEMU, 2017) setzt bewusst nicht die
# gesamte privilegierte Architektur um, und die Erweiterungen Zba/Zbb/Zbc/Zbs/Zbkb/Zbkx/Zfh/Zicond
# gab es damals noch nicht. Eine Suite, die dauerhaft rot ist, wird ignoriert -- also halten wir
# den bekannten Stand fest und schlagen an, sobald er sich aendert. Wird eine Luecke geschlossen,
# gehoert die Zahl hier hochgesetzt (das meldet das Skript ausdruecklich als Verbesserung).
#
# Call:   test/riscv/run-isa-tests.sh [32|64]
#         Voraussetzung: test/riscv/fetch-isa-tests.sh <xlen>  und  make test-riscv (baut runner)
#═════════════════════════════════════════════════════════════════════════════════════════════════
set -e

XLEN="${1:-32}"
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
DIR="$ROOT/build/riscv-tests/rv$XLEN"

# Runner-Pfad: plattformabhaengig, dieselbe Logik wie im Makefile.
case "$(uname -s)" in
    Darwin*)  PLAT=macos ;;
    Linux*)   PLAT=linux ;;
    *)        PLAT=windows ;;
esac
RUNNER="$ROOT/build/$PLAT/rvtest_runner"

[ -x "$RUNNER" ] || { echo "  $RUNNER fehlt -- zuerst: make test-riscv" >&2; exit 1; }
[ -d "$DIR" ]    || { echo "  $DIR fehlt -- zuerst: test/riscv/fetch-isa-tests.sh $XLEN" >&2; exit 1; }

#───────────────────────────────────────────────────────────────────────────────────────────────
# Festgehaltener Stand, gemessen 2026-08-12 gegen TinyEMU-Commit 56ba49b + Q9-Aenderung.
# Format: <gruppe> <erwartet-bestanden> <gesamt>
# Die bekannten Ausfaelle und ihre Ursache stehen in third_party/tinyemu/Q9_VENDOR.md.
#───────────────────────────────────────────────────────────────────────────────────────────────
baseline_32='
rv32ui 42 42
rv32um 8 8
rv32ua 9 10
rv32uc 1 1
rv32uf 10 11
rv32ud 9 10
rv32mi 11 16
rv32si 5 6
'

eval "expected=\$baseline_$XLEN"
[ -n "$expected" ] || { echo "  kein festgehaltener Stand fuer RV$XLEN" >&2; exit 1; }

changed=0
sum_pass=0
sum_total=0

echo "  RV$XLEN ISA-Tests gegen third_party/tinyemu"
echo ""

echo "$expected" | while read -r grp exp tot; do
    [ -z "$grp" ] && continue
    files=$(ls "$DIR"/${grp}-p-* 2>/dev/null | grep -v '\.dump$' || true)
    if [ -z "$files" ]; then
        printf '  %-9s  ---   nicht gebaut\n' "$grp"
        continue
    fi
    out=$(echo $files | xargs "$RUNNER" --xlen "$XLEN" 2>&1 || true)
    got=$(echo "$out" | sed -n 's/.*ISA-Tests: \([0-9]*\)\/.*/\1/p')
    [ -z "$got" ] && got=0

    if [ "$got" -eq "$exp" ]; then
        printf '  %-9s %3s/%-3s  unveraendert\n' "$grp" "$got" "$tot"
    elif [ "$got" -gt "$exp" ]; then
        printf '  %-9s %3s/%-3s  BESSER als festgehalten (%s) -- Stand im Skript hochsetzen\n' \
               "$grp" "$got" "$tot" "$exp"
        echo "$grp better" >> "$DIR/.changed"
    else
        printf '  %-9s %3s/%-3s  SCHLECHTER als festgehalten (%s) -- Regression\n' \
               "$grp" "$got" "$tot" "$exp"
        echo "$grp worse" >> "$DIR/.changed"
        echo "$out" | grep -E '^(FAIL|HANG|ERR)' | sed 's/^/      /'
    fi
done

# Die Schleife laeuft in einer Subshell (Pipe), deshalb ueber eine Datei kommunizieren.
if [ -f "$DIR/.changed" ]; then
    worse=$(grep -c ' worse$' "$DIR/.changed" || true)
    rm -f "$DIR/.changed"
    echo ""
    [ "$worse" -gt 0 ] && { echo "  => Regression, siehe oben"; exit 1; }
    echo "  => Verbesserung -- festgehaltenen Stand in diesem Skript nachziehen"
    exit 0
fi

echo ""
echo "  => alle Basisgruppen wie festgehalten"
