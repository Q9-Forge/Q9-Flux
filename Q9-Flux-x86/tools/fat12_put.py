#!/usr/bin/env python3
"""Copy one 8.3 file into a standard FAT12 floppy image."""
from __future__ import annotations

import argparse
import struct
from pathlib import Path


def fat12_get(fat: bytearray, cluster: int) -> int:
    offset = cluster + cluster // 2
    word = fat[offset] | (fat[offset + 1] << 8)
    return (word >> 4) if cluster & 1 else (word & 0xFFF)


def fat12_set(fat: bytearray, cluster: int, value: int) -> None:
    offset = cluster + cluster // 2
    if cluster & 1:
        fat[offset] = (fat[offset] & 0x0F) | ((value << 4) & 0xF0)
        fat[offset + 1] = (value >> 4) & 0xFF
    else:
        fat[offset] = value & 0xFF
        fat[offset + 1] = (fat[offset + 1] & 0xF0) | ((value >> 8) & 0x0F)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("source_image", type=Path)
    parser.add_argument("output_image", type=Path)
    parser.add_argument("source_file", type=Path)
    parser.add_argument("name", help="destination 8.3 name, e.g. INIT")
    args = parser.parse_args()
    if args.source_image.resolve() == args.output_image.resolve():
        parser.error("output_image must differ from source_image")
    name = args.name.upper().encode("ascii")
    if not name or len(name) > 11 or b"." in name:
        parser.error("name must be an 8.3 name without a dot, padded by this tool")
    if len(name) <= 8:
        directory_name = name.ljust(8, b" ") + b"   "
    else:
        stem, suffix = name.split(b".", 1) if b"." in name else (name[:8], name[8:])
        directory_name = stem.ljust(8, b" ") + suffix.ljust(3, b" ")
    data = args.source_file.read_bytes()
    image = bytearray(args.source_image.read_bytes())
    bps = struct.unpack_from("<H", image, 0x0B)[0]
    reserved = struct.unpack_from("<H", image, 0x0E)[0]
    fats = image[0x10]
    root_entries = struct.unpack_from("<H", image, 0x11)[0]
    sectors_per_fat = struct.unpack_from("<H", image, 0x16)[0]
    sectors_per_cluster = image[0x0D]
    if (bps, sectors_per_cluster, fats, sectors_per_fat) != (512, 1, 2, 9):
        raise ValueError("only standard 512-byte FAT12 floppy images are supported")
    root_start = (reserved + fats * sectors_per_fat) * bps
    root_size = root_entries * 32
    data_start = root_start + root_size
    fat_size = sectors_per_fat * bps
    root = memoryview(image)[root_start:root_start + root_size]
    existing = None
    free = None
    for offset in range(0, root_size, 32):
        entry = root[offset:offset + 32]
        entry_name = bytes(entry[:11])
        if entry_name == directory_name:
            existing = offset
            break
        if free is None and entry[0] in (0x00, 0xE5):
            free = offset
    if existing is not None:
        raise ValueError(f"destination {args.name!r} already exists")
    if free is None:
        raise ValueError("FAT12 root directory is full")
    clusters = (len(data) + bps - 1) // bps
    if clusters == 0:
        clusters = 1
    fat = bytearray(image[reserved * bps:reserved * bps + fat_size])
    free_clusters = []
    for cluster in range(2, 2 + (len(image) - data_start) // bps):
        if fat12_get(fat, cluster) == 0:
            free_clusters.append(cluster)
            if len(free_clusters) == clusters:
                break
    if len(free_clusters) != clusters:
        raise ValueError("not enough free FAT12 clusters")
    for index, cluster in enumerate(free_clusters):
        fat12_set(fat, cluster, 0xFFF if index + 1 == clusters else free_clusters[index + 1])
        start = data_start + (cluster - 2) * bps
        image[start:start + bps] = data[index * bps:(index + 1) * bps].ljust(bps, b"\0")
    for copy in range(fats):
        begin = (reserved + copy * sectors_per_fat) * bps
        image[begin:begin + fat_size] = fat
    entry = root[free:free + 32]
    entry[:11] = directory_name
    entry[11] = 0x20
    struct.pack_into("<H", entry, 26, free_clusters[0])
    struct.pack_into("<I", entry, 28, len(data))
    args.output_image.write_bytes(image)
    print(f"wrote {args.output_image}: {args.name} ({len(data)} bytes, {clusters} clusters)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
