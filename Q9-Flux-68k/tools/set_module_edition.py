#!/usr/bin/env python3
"""Set one OS-9 module edition inside a standalone or merged module file."""

from __future__ import annotations

import argparse
import struct
from pathlib import Path

from patch_pks_descriptor import module_name, module_size, os9_crc, stored_crc


HEADER_SIZE = 48
CRC_SIZE = 3
EDITION_OFFSET = 0x16
PARITY_OFFSET = 0x2E


def find_module(data: bytes, wanted: str) -> tuple[int, int]:
    pos = 0
    while pos + HEADER_SIZE <= len(data):
        if struct.unpack_from(">H", data, pos)[0] != 0x4AFC:
            pos += 2
            continue
        size = module_size(data, pos)
        if size < HEADER_SIZE + CRC_SIZE or pos + size > len(data):
            pos += 2
            continue
        if module_name(data, pos) == wanted:
            return pos, size
        pos += size
    raise ValueError(f"OS-9 module {wanted!r} not found")


def verify(module: bytes) -> None:
    if os9_crc(module) != (0x80, 0x0F, 0xE3):
        raise ValueError("module CRC invalid")
    parity = 0
    for index in range(0, PARITY_OFFSET + 2, 2):
        parity ^= (module[index] << 8) | module[index + 1]
    if parity != 0xFFFF:
        raise ValueError(f"module header parity invalid: 0x{parity:04x}")


def update(path: Path, wanted: str, edition: int) -> tuple[int, int, int]:
    data = bytearray(path.read_bytes())
    start, size = find_module(data, wanted)
    module = bytearray(data[start : start + size])
    verify(module)
    old = int.from_bytes(module[EDITION_OFFSET : EDITION_OFFSET + 2], "big")
    module[EDITION_OFFSET : EDITION_OFFSET + 2] = edition.to_bytes(2, "big")
    parity = 0
    for index in range(0, PARITY_OFFSET, 2):
        parity ^= (module[index] << 8) | module[index + 1]
    module[PARITY_OFFSET : PARITY_OFFSET + 2] = (0xFFFF ^ parity).to_bytes(2, "big")
    module[-CRC_SIZE:] = stored_crc(module[:-CRC_SIZE])
    verify(module)
    data[start : start + size] = module
    path.write_bytes(data)
    return start, old, edition


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("input", type=Path)
    parser.add_argument("output", type=Path)
    parser.add_argument("--module", required=True)
    parser.add_argument("--edition", required=True, type=int)
    args = parser.parse_args()
    args.output.write_bytes(args.input.read_bytes())
    start, old, new = update(args.output, args.module, args.edition)
    print(f"module: {args.module}, merge offset: 0x{start:x}")
    print(f"edition: {old} -> {new}")
    print(f"patched: {args.output}")


if __name__ == "__main__":
    main()
