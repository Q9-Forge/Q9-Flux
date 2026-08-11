#!/bin/bash
# deploy_gdp_5_29.sh -- baut mc6845.a/crtc0.a (SCF-Treiber, 5.28/5.29) frisch
# per Wine-Toolchain (r68/l68) und spielt sie zusammen mit vramtest per
# ToolShed auf ein Testimage.
#
# WICHTIG (2026-08-10, s. ARBEITSPLAN.md "Nachtrag ... Checkpoint-8-
# Regression aufgeklaert"): `os9 copy -r` setzt beim Kopieren einer Datei
# auf ein OS-9-Image das RBF-Owner-Execute-Bit NICHT. Fuer normale Dateien
# faellt das nicht auf, aber ein per F$Load zu ladendes Treiber-/System-
# State-Modul wird von OS-9 dann mit einer voellig unauffaelligen, fehler-
# textlosen Meldung abgelehnt ("load: can't load "<pfad>" -", ohne
# Fehlernummer/-text danach) -- CRC/Header/`ident`/`dir -e` bleiben dabei
# alle "gut", sieht wie ein Treiberbug aus, ist aber ein reines Rechte-
# Problem. Deshalb setzt dieses Skript nach JEDEM Copy zusaetzlich
# `os9 attr -e` -- das ist der eigentliche Kern dieses Skripts, nicht nur
# Bequemlichkeit. Wer den Deploy-Schritt von Hand macht (os9 copy direkt),
# MUSS `os9 attr -e "<image>,<pfad>"` fuer jedes Treiber-/Programm-Modul
# manuell nachziehen, sonst schlaegt "load"/Programmstart fehl.
#
# Nutzung:
#   test/expect/deploy_gdp_5_29.sh [<image-pfad>]
# Ohne Argument wird lokal_images/OS9SYS.gdp-test-20260803.hda verwendet
# (das vom Testharness/den .exp-Skripten erwartete Standard-Testimage).
set -euo pipefail

Q9FLUX_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
MWOS="/Volumes/SSD1TB/projects/MWOS"
Q9PORT="$MWOS/OS9/68030/PORTS/Q9"
IMG="${1:-$Q9FLUX_ROOT/local_images/OS9SYS.gdp-test-20260803.hda}"

WINE="$HOME/.local/wine-stable/Wine Stable.app/Contents/Resources/wine/bin/wine"
export WINEPREFIX="$HOME/.wine"
export WINEDEBUG=-all

SCFDIR='M:\OS9\68030\PORTS\Q9\SCF'
DESCSDIR='M:\OS9\SRC\IO\SCF\DESC'
OSDEFS='M:\OS9\SRC\DEFS'
MACDIR='M:\OS9\SRC\MACROS'
SYSRELS='M:\OS9\68000\LIB'
BOOTOBJS='M:\OS9\68030\PORTS\Q9\CMDS\BOOTOBJS'

echo "=== mc6845.a bauen (SCF/scf_mc6845.make-Rezept) ==="
"$WINE" cmd /c "cd /d $SCFDIR && M:\DOS\BIN\r68.exe -qb -u=. -u=$OSDEFS -u=$MACDIR mc6845.a -O=RELS\\mc6845.r"
# Achtung, os9make-Eigenheit (Make-Variable SLIB ist ein einziger, space-
# getrennter String -> nur die ERSTE Bibliothek bekommt "-l="): exakt so
# nachgebaut, damit das Ergebnis 1:1 dem offiziellen os9make-Build
# entspricht (per `os9make -e ... GOAL=build SCF` verifiziert, 2026-08-10).
"$WINE" cmd /c "cd /d $SCFDIR && M:\DOS\BIN\l68.exe -l=$SYSRELS\\sys.l  $SYSRELS\\scfstat.l -gu=0.0 RELS\\mc6845.r -O=$BOOTOBJS\\mc6845"

echo "=== crtc0.a bauen (SCF/scf_descriptors.make-Rezept) ==="
"$WINE" cmd /c "cd /d $SCFDIR && M:\DOS\BIN\r68.exe -q -u=. -u=$OSDEFS -u=$DESCSDIR crtc0.a -O=RELS\\crtc0.r"
"$WINE" cmd /c "cd /d $SCFDIR && M:\DOS\BIN\l68.exe -l=$SYSRELS\\sys.l -gu=0.0 -p=577 RELS\\crtc0.r -O=$BOOTOBJS\\crtc0"

echo "=== Deploy auf $IMG ==="
os9 makdir "$IMG,CMDS/GDPTEST" 2>/dev/null || true
os9 copy -r "$Q9PORT/CMDS/BOOTOBJS/mc6845" "$IMG,CMDS/BOOTOBJS/mc6845"
os9 copy -r "$Q9PORT/CMDS/BOOTOBJS/crtc0"  "$IMG,CMDS/BOOTOBJS/crtc0"

echo "=== Execute-Bit nachziehen (s. Kommentar oben -- der eigentliche Zweck dieses Skripts) ==="
os9 attr -e "$IMG,CMDS/BOOTOBJS/mc6845"
os9 attr -e "$IMG,CMDS/BOOTOBJS/crtc0"

if [ -n "${VRAMTEST_BIN:-}" ] && [ -f "$VRAMTEST_BIN" ]; then
    echo "=== vramtest deployen ($VRAMTEST_BIN) ==="
    os9 copy -r "$VRAMTEST_BIN" "$IMG,CMDS/GDPTEST/vramtest"
    os9 attr -e "$IMG,CMDS/GDPTEST/vramtest"
fi

echo "=== fertig -- Attribute zur Kontrolle ==="
os9 attr "$IMG,CMDS/BOOTOBJS/mc6845"
os9 attr "$IMG,CMDS/BOOTOBJS/crtc0"
