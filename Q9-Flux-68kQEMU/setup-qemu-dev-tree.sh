#!/bin/bash
# setup-qemu-dev-tree.sh -- richtet den lokalen QEMU-Checkout unter
# third_party/qemu fuer die Q9-board-Entwicklung ein: initialisiert das
# Submodul, kopiert unsere neuen Dateien rein (overlay/new-files/) und
# wendet unsere Patches auf bestehende QEMU-Dateien an (overlay/patches/).
#
# Idempotent -- kann nach jedem "git submodule update" (das den Checkout
# auf den sauberen Pin zuruecksetzt und unsere Aenderungen entfernt)
# einfach erneut ausgefuehrt werden.
set -euo pipefail

HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
QEMU_DIR="$HERE/third_party/qemu"
OVERLAY="$HERE/overlay"

echo "==> Submodul initialisieren/aktualisieren"
git -C "$HERE/.." submodule update --init "Q9-Flux-68kQEMU/third_party/qemu"

echo "==> Eigene neue Dateien kopieren"
( cd "$OVERLAY/new-files" && find . -type f ) | while read -r rel; do
    mkdir -p "$QEMU_DIR/$(dirname "$rel")"
    cp "$OVERLAY/new-files/$rel" "$QEMU_DIR/$rel"
    echo "    $rel"
done

echo "==> Patches auf bestehende QEMU-Dateien anwenden"
for patch in "$OVERLAY"/patches/*.patch; do
    [ -e "$patch" ] || continue
    echo "    $(basename "$patch")"
    ( cd "$QEMU_DIR" && git apply --check "$patch" 2>/dev/null ) \
        && ( cd "$QEMU_DIR" && git apply "$patch" ) \
        || echo "    (bereits angewendet oder Konflikt -- pruefen: cd $QEMU_DIR && git apply --check $patch)"
done

echo "==> Fertig. Bauen mit:"
echo "    cd $QEMU_DIR && mkdir -p build-m68k && cd build-m68k"
echo "    ../configure --target-list=m68k-softmmu && ninja"
