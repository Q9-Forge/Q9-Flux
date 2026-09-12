#!/usr/bin/env python3
"""Create an empty conventional 1.44-MB FAT12 floppy image."""

from __future__ import annotations

import argparse
import struct
from pathlib import Path


SECTOR = 512
SECTORS = 2880
FAT_SECTORS = 9


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()

    image = bytearray(SECTOR * SECTORS)
    boot = memoryview(image)[:SECTOR]
    boot[:3] = b"\xeb\x3c\x90"
    boot[3:11] = b"Q9FLUX  "
    struct.pack_into("<H", boot, 0x0B, SECTOR)
    boot[0x0D] = 1
    struct.pack_into("<H", boot, 0x0E, 1)
    boot[0x10] = 2
    struct.pack_into("<H", boot, 0x11, 224)
    struct.pack_into("<H", boot, 0x13, SECTORS)
    boot[0x15] = 0xF0
    struct.pack_into("<H", boot, 0x16, FAT_SECTORS)
    struct.pack_into("<H", boot, 0x18, 18)
    struct.pack_into("<H", boot, 0x1A, 2)
    boot[0x24] = 0
    boot[0x26] = 0x29
    struct.pack_into("<I", boot, 0x27, 0x51394639)
    boot[0x2B:0x36] = b"Q9FAT12    "
    boot[0x36:0x3E] = b"FAT12   "
    boot[510:512] = b"\x55\xaa"
    for fat_sector in (1, 1 + FAT_SECTORS):
        offset = fat_sector * SECTOR
        image[offset:offset + 3] = b"\xf0\xff\xff"
    args.output.write_bytes(image)
    print(f"wrote {args.output}: empty 1.44-MB FAT12 superfloppy")


if __name__ == "__main__":
    main()
