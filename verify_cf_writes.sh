#!/bin/bash
# Verify that CF writes work correctly

CF_IMG="local_images/Q9-cb030-work-test.hda"
OS9="/Volumes/SSD1TB/projects/MWOS/tools/macos/bin/os9"

echo "=== CF Write Verification ==="
echo ""
echo "Checking: $CF_IMG"
echo ""

if [ ! -f "$CF_IMG" ]; then
    echo "ERROR: $CF_IMG not found"
    echo "Run ./test_save_modules.sh first to create test image"
    exit 1
fi

echo "1. Disk structure check..."
$OS9 dcheck "$CF_IMG" && echo "   ✓ Disk structure OK" || echo "   ✗ Disk corrupted!"

echo ""
echo "2. Checking RAMTEST directory..."
if $OS9 dir "$CF_IMG,CMDS/BOOTOBJS/RAMTEST" > /dev/null 2>&1; then
    echo "   ✓ Directory exists"
    echo ""
    echo "3. Files in RAMTEST:"
    $OS9 dir -e "$CF_IMG,CMDS/BOOTOBJS/RAMTEST"
    
    echo ""
    echo "4. Verifying module integrity..."
    for file in $($OS9 dir "$CF_IMG,CMDS/BOOTOBJS/RAMTEST" 2>/dev/null | awk '{print $NF}' | grep -v '^$'); do
        if [ "$file" != "Name" ]; then
            result=$($OS9 ident "$CF_IMG,CMDS/BOOTOBJS/RAMTEST/$file" 2>&1 | grep -c "Good CRC")
            if [ "$result" -gt 0 ]; then
                echo "   ✓ $file - Good CRC"
            else
                echo "   ✗ $file - BAD CRC or corrupted!"
            fi
        fi
    done
else
    echo "   ✗ Directory not found"
    echo "   You need to run the emulator test first:"
    echo "   ./save_ram_modules.exp <path-to-rom-image>"
fi

echo ""
echo "=== Comparison with backup ==="
if [ -f "local_images/Q9-cb030-work-max.prev.hda" ]; then
    echo "Checking if writes modified the image..."
    if ! cmp -s "local_images/Q9-cb030-work-max.hda" "local_images/Q9-cb030-work-max.prev.hda" 2>/dev/null; then
        echo "✓ Image was modified (writes happened)"
    else
        echo "✗ Image unchanged (no writes or write failed)"
    fi
fi

echo ""
echo "Done!"
