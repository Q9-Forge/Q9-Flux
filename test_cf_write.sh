#!/bin/bash
set -e

# Create a small test image
TEST_IMG="local_images/test-cf-write-256.hda"
TEST_BACKUP="local_images/test-cf-write-256.backup.hda"
OS9="/Volumes/SSD1TB/projects/MWOS/tools/macos/bin/os9"

echo "=== CF Write Test for 256-byte RBF Images ==="

# Create fresh test image (256-byte sectors)
echo "Creating test image..."
$OS9 format -q -k -nTEST256 -l10000 -c4 "$TEST_IMG"

# Backup
cp "$TEST_IMG" "$TEST_BACKUP"

echo ""
echo "Creating test file..."
echo "Hello World from Q9!" > /tmp/test.txt
$OS9 copy /tmp/test.txt "$TEST_IMG,testfile"

echo ""
echo "Reading back..."
$OS9 dump -b "$TEST_IMG,testfile"

echo ""
echo "=== Test image ready: $TEST_IMG ==="
echo "Now run the emulator with this image to test writes"
echo ""
echo "To test:"
echo "  ./build/native/q9.exe --cb030 local_images/HD5-skip2-work.hda --cf local_images/test-cf-write-256.hda"
