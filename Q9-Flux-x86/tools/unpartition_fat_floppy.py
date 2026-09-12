#!/usr/bin/env python3
"""Turn a one-partition FAT floppy image into a raw FAT "superfloppy".

OS-9000's PCF floppy path reads the FAT boot record at physical sector zero.
Some host tools create an MBR with the FAT filesystem beginning at sector one;
that layout works for a hard disk but not for the PCF floppy driver.
"""

from __future__ import annotations

import argparse
import struct
from pathlib import Path


SECTOR_SIZE = 512
FLOPPY_SECTORS = 2880


def fail(message: str) -> None:
    raise SystemExit(f"error: {message}")


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("source", type=Path, help="MBR-partitioned FAT image")
    parser.add_argument("output", type=Path, help="raw FAT floppy output")
    args = parser.parse_args()

    source = args.source.read_bytes()
    if len(source) < 2 * SECTOR_SIZE or len(source) % SECTOR_SIZE:
        fail("source is not sector aligned")
    mbr = source[:SECTOR_SIZE]
    if mbr[510:512] != b"\x55\xaa":
        fail("source does not have an MBR signature")
    part_type = mbr[0x1C2]
    start, sectors = struct.unpack_from("<II", mbr, 0x1C6)
    if part_type not in {0x01, 0x04, 0x06, 0x0B, 0x0C, 0x0E}:
        fail(f"partition 1 has unsupported type 0x{part_type:02x}")
    if start == 0 or sectors == 0 or (start + sectors) * SECTOR_SIZE > len(source):
        fail("partition 1 is outside the source image")

    volume = bytearray(source[start * SECTOR_SIZE : (start + sectors) * SECTOR_SIZE])
    if volume[0] not in {0xEB, 0xE9} or volume[510:512] != b"\x55\xaa":
        fail("partition 1 does not start with a FAT boot sector")
    bytes_per_sector = struct.unpack_from("<H", volume, 0x0B)[0]
    if bytes_per_sector != SECTOR_SIZE:
        fail(f"unsupported FAT sector size {bytes_per_sector}")
    if sectors > FLOPPY_SECTORS:
        fail(f"partition has {sectors} sectors; exceeds a 1.44-MB floppy")

    # Make it a standard 1.44-MB superfloppy.  The appended sector stays free;
    # changing these BPB fields removes the hard-disk geometry assumption.
    volume.extend(b"\0" * ((FLOPPY_SECTORS - sectors) * SECTOR_SIZE))
    struct.pack_into("<H", volume, 0x13, FLOPPY_SECTORS)
    volume[0x15] = 0xF0  # floppy media descriptor (rather than hard-disk F8)
    struct.pack_into("<H", volume, 0x18, 18)  # sectors per track
    struct.pack_into("<H", volume, 0x1A, 2)  # heads
    struct.pack_into("<I", volume, 0x1C, 0)  # hidden sectors
    reserved = struct.unpack_from("<H", volume, 0x0E)[0]
    fat_count = volume[0x10]
    sectors_per_fat = struct.unpack_from("<H", volume, 0x16)[0]
    for fat_index in range(fat_count):
        fat_start = (reserved + fat_index * sectors_per_fat) * SECTOR_SIZE
        # FAT[0]/FAT[1] encode the same media type as the BPB.
        volume[fat_start : fat_start + 3] = b"\xF0\xFF\xFF"
    args.output.write_bytes(volume)
    print(
        f"wrote {args.output}: FAT volume moved from LBA {start}, "
        f"{sectors} -> {FLOPPY_SECTORS} sectors"
    )


if __name__ == "__main__":
    main()
