#!/bin/bash
# Test-Script: Speichert alle RAM-Module in BOOTOBJS auf CF-Image (testet Write-Fix)

set -e

ROM="local_images/HD5-skip2-work.hda"
CF_IMG="local_images/Q9-cb030-work-test.hda"
CF_ORIG="local_images/Q9-cb030-work-max.hda"
OS9="/Volumes/SSD1TB/projects/MWOS/tools/macos/bin/os9"
EMU="./build/native/q9.exe"

echo "=== Q9 CF Write Test: Save Modules ==="
echo ""

# Backup original and create test copy
if [ ! -f "$CF_ORIG" ]; then
    echo "ERROR: $CF_ORIG not found!"
    exit 1
fi

echo "Creating test CF image from $CF_ORIG..."
cp "$CF_ORIG" "$CF_IMG"

# Create BOOTOBJS/RAMTEST directory on CF image
echo "Creating CMDS/BOOTOBJS/RAMTEST directory..."
$OS9 makdir "$CF_IMG,CMDS/BOOTOBJS/RAMTEST" 2>/dev/null || true

echo ""
echo "Test CF image ready: $CF_IMG"
echo ""
echo "Now you need to manually:"
echo "1. Start emulator: export Q9_CB030_CF_TRACE=1 && $EMU --cb030 $ROM --cf $CF_IMG"
echo "2. Wait for boot to shell prompt"
echo "3. Run: mdir"
echo "4. Run: makdir /d0/CMDS/BOOTOBJS/RAMTEST"
echo "5. For each module shown by mdir, run: save <modulename> /d0/CMDS/BOOTOBJS/RAMTEST/<modulename>"
echo "6. Exit emulator (Ctrl-C)"
echo "7. Check results with: $OS9 dir $CF_IMG,CMDS/BOOTOBJS/RAMTEST"
echo ""
echo "Or use the automated expect script (if available)..."
