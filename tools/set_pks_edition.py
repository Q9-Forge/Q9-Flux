#!/usr/bin/env python3
"""Set the OS-9 edition number of the pks module in a merged module file."""

from __future__ import annotations

import argparse
import struct
from pathlib import Path

from patch_pks_descriptor import find_pks, stored_crc, verify_module


EDITION_OFFSET = 0x16
PARITY_OFFSET = 0x2E
HEADER_SIZE = 48
CRC_SIZE = 3


def fix_header_parity(module: bytearray) -> None:
    parity = 0
    for index in range(0, PARITY_OFFSET, 2):
        parity ^= (module[index] << 8) | module[index + 1]
    value = 0xFFFF ^ parity
    module[PARITY_OFFSET : PARITY_OFFSET + 2] = value.to_bytes(2, "big")


def update(path: Path, edition: int) -> tuple[int, int, int]:
    data = bytearray(path.read_bytes())
    start, size = find_pks(data)
    verify_module(data, start, size)
    module = bytearray(data[start : start + size])
    old = int.from_bytes(module[EDITION_OFFSET : EDITION_OFFSET + 2], "big")
    module[EDITION_OFFSET : EDITION_OFFSET + 2] = edition.to_bytes(2, "big")
    fix_header_parity(module)
    module[-CRC_SIZE:] = stored_crc(module[:-CRC_SIZE])
    verify_module(module, 0, size)
    data[start : start + size] = module
    path.write_bytes(data)
    return start, old, edition


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("input", type=Path)
    parser.add_argument("output", type=Path)
    parser.add_argument("--edition", type=int, required=True)
    args = parser.parse_args()
    if not 0 <= args.edition <= 0xFFFF:
        raise SystemExit("edition must fit in an OS-9 header word")
    args.output.write_bytes(args.input.read_bytes())
    start, old, new = update(args.output, args.edition)
    print(f"target module: pks (merge offset 0x{start:x})")
    print(f"edition: {old} -> {new}")
    print(f"patched: {args.output}")


if __name__ == "__main__":
    main()
