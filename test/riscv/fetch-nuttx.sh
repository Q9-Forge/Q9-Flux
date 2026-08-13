#!/bin/sh
#═════════════════════════════════════════════════════════════════════════════════════════════════
# File:   fetch-nuttx.sh                                                                  Ver. 1.00
# Owner:  Claudia
# Desc.:  Holt und baut NuttX (rv-virt:nsh) fuer den Stufe-3-Prueflauf (make test-rvnuttx). Siehe
#         docs/RISCV.md fuer die vollstaendige Begruendung -- hier nur der Ablauf.
#
# Call:   test/riscv/fetch-nuttx.sh
#         danach: make test-rvnuttx
#
# VORAUSSETZUNGEN (deutlich mehr als bei fetch-isa-tests.sh, siehe docs/RISCV.md "Werkzeuge"):
#   - xPack riscv-none-elf-gcc im PATH -- NICHT die Homebrew-Formel riscv64-elf-gcc: die erzeugte
#     beim Binden ABI-/Multilib-Konflikte, die exakt die von NuttX selbst empfohlene xPack-
#     Toolchain vermeidet. https://github.com/xpack-dev-tools/riscv-none-elf-gcc-xpack/releases,
#     "...darwin-arm64.tar.gz" entpacken, bin/ in den PATH.
#   - kconfig-tweak im PATH -- Homebrews eigenes Formula (Tap shizacat/kconfig-frontends) hat eine
#     tote Quell-URL; gebaut wird stattdessen aus dem gepflegten NuttX-Tools-Repo:
#       git clone --depth 1 https://github.com/patacongo/tools /tmp/nuttx-tools
#       cd /tmp/nuttx-tools/kconfig-frontends && patch -p1 < ../kconfig-macos.diff
#       ./configure --disable-mconf --disable-shared --enable-static \
#                    --disable-gconf --disable-qconf --disable-nconf --prefix=/opt/homebrew
#       make && make install
#     (--disable-mconf umgeht die ncurses-Verlinkung auf macOS -- wir brauchen nur kconfig-tweak,
#     keine interaktive Menue-Oberflaeche.)
#   - genromfs im PATH -- selbes Repo, utils/genromfs-<version>.tar.gz entpacken, "make" (einfaches
#     C-Programm, kein Autotools-Umbau noetig).
#   - flock im PATH -- "brew install flock" (util-linux' flock fehlt auf macOS von Haus aus, NuttX'
#     App-Build braucht es fuer paralleles .a-Archivieren).
#   - FALLE: ist in dieser Shell "$MAKE" auf einen fremden Cross-Toolchain-"make" gesetzt (z.B. aus
#     einer anderen Projekt-Bauumgebung), schreibt NuttX' eigenes "./configure" diesen kaputten
#     Wert FEST in die generierte Makefile (sichtbar als "MAKE=<anderes-make> -e" darin) -- ein
#     blosses "unset MAKE" NACH dem Konfigurieren reicht dann nicht mehr, es muss VOR jedem
#     configure/make-Aufruf leer sein. Dieses Skript ruft "unset MAKE" deshalb selbst auf.
#═════════════════════════════════════════════════════════════════════════════════════════════════
set -e
unset MAKE

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
SRC="$ROOT/build/riscv-tests/nuttx-src"

for t in riscv-none-elf-gcc kconfig-tweak genromfs flock; do
    if ! command -v "$t" >/dev/null 2>&1; then
        echo "fetch-nuttx: '$t' fehlt -- s. Kopf dieses Skripts fuer die Beschaffung." >&2
        exit 1
    fi
done

mkdir -p "$SRC"
if [ ! -d "$SRC/nuttx/.git" ]; then
    echo "-> hole nuttx"
    git clone -q --depth 1 https://github.com/apache/nuttx.git "$SRC/nuttx"
fi
if [ ! -d "$SRC/apps/.git" ]; then
    echo "-> hole nuttx-apps"
    git clone -q --depth 1 https://github.com/apache/nuttx-apps.git "$SRC/apps"
fi

cd "$SRC/nuttx"
echo "-> konfiguriere rv-virt:nsh"
rm -f .config .config.old
./tools/configure.sh rv-virt:nsh >/dev/null

echo "-> baue (CROSSDEV=riscv-none-elf-)"
make CROSSDEV=riscv-none-elf- -j4 >/tmp/q9-nuttx-build.log 2>&1 || {
    echo "fetch-nuttx: Bau fehlgeschlagen, Log: /tmp/q9-nuttx-build.log" >&2
    exit 1
}

OUT="$ROOT/build/riscv-tests/nuttx_nsh.elf"
cp nuttx "$OUT"
echo "-> $OUT ($(wc -c < "$OUT" | tr -d ' ') Byte)"
