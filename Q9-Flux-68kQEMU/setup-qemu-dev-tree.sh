#!/bin/bash
# setup-qemu-dev-tree.sh -- richtet den lokalen QEMU-Checkout unter
# third_party/qemu fuer die Q9-board-Entwicklung ein: initialisiert das
# Submodul, kopiert unsere eigenen Geraete-/Maschinen-Dateien gemaess
# qemu-mapping.conf hinein und wendet unsere Patches auf bestehende
# QEMU-Dateien an (overlay/patches/).
#
# Idempotent -- kann nach jedem "git submodule update" (das den Checkout
# auf den sauberen Pin zuruecksetzt und unsere Aenderungen entfernt)
# einfach erneut ausgefuehrt werden.
set -euo pipefail

HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
QEMU_DIR="$HERE/third_party/qemu"
MAPPING="$HERE/qemu-mapping.conf"

echo "==> Submodul initialisieren/aktualisieren"
git -C "$HERE/.." submodule update --init "Q9-Flux-68kQEMU/third_party/qemu"

echo "==> Eigene Geraete-/Maschinen-Dateien gemaess qemu-mapping.conf kopieren"
while read -r src dst; do
    [ -z "$src" ] && continue
    case "$src" in \#*) continue ;; esac
    mkdir -p "$QEMU_DIR/$(dirname "$dst")"
    cp "$HERE/$src" "$QEMU_DIR/$dst"
    echo "    $src -> $dst"
done < "$MAPPING"

echo "==> Patches auf bestehende QEMU-Dateien anwenden"
for patch in "$HERE"/overlay/patches/*.patch; do
    [ -e "$patch" ] || continue
    echo "    $(basename "$patch")"
    ( cd "$QEMU_DIR" && git apply --check "$patch" 2>/dev/null ) \
        && ( cd "$QEMU_DIR" && git apply "$patch" ) \
        || echo "    (bereits angewendet oder Konflikt -- pruefen: cd $QEMU_DIR && git apply --check $patch)"
done

echo "==> Fertig. Bauen mit:"
echo "    cd $QEMU_DIR && mkdir -p build-m68k && cd build-m68k"
echo "    ../configure --target-list=m68k-softmmu && ninja"
