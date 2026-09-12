#!/usr/bin/env python3
"""Build an OS-9000/x86 PCF descriptor from a QEMU RAM snapshot.

This deliberately only emits a module.  Installing it into the proprietary
RBF volume is a separate, validated step.
"""
from __future__ import annotations

import argparse
import struct
from pathlib import Path

SYNC = b"\xfc\x4a"
HEADER_SIZE = 0x58
CRC_GOOD = 0x800FE3
CRC_POLY = 0x800063
DD_LU_NUM = 0x5C
DD_TYPE = 0x60
RB765_UNIT_MIRROR = 0x1EA
# The supplied x86 RB765 descriptor pair proves these PCF-specific values.
# They deliberately take precedence over the generic/68k DPIO definitions.
DT_PCF_X86 = 10
PCF_FLAG_OFFSET = 0xB0
PCF_RESERVED_OFFSET = 0xE8


def crc24(data: bytes) -> int:
    value = 0xFFFFFF
    for byte in data:
        value ^= byte << 16
        for _ in range(8):
            value = ((value << 1) ^ CRC_POLY) & 0xFFFFFF if value & 0x800000 else (value << 1) & 0xFFFFFF
    return value


def solve_crc(body: bytes) -> bytes:
    base = crc24(body + b"\0\0\0")
    wanted = base ^ CRC_GOOD
    columns = [crc24(body + (1 << (23 - bit)).to_bytes(3, "big")) ^ base for bit in range(24)]
    rows = []
    for equation in range(24):
        row = sum((1 << (23 - variable)) for variable, column in enumerate(columns)
                  if column & (1 << (23 - equation)))
        rows.append(row | ((wanted >> (23 - equation) & 1) << 24))
    for pivot in range(24):
        candidate = next(index for index in range(pivot, 24) if rows[index] & (1 << (23 - pivot)))
        rows[pivot], rows[candidate] = rows[candidate], rows[pivot]
        for index in range(24):
            if index != pivot and rows[index] & (1 << (23 - pivot)):
                rows[index] ^= rows[pivot]
    value = sum((1 << (23 - index)) for index, row in enumerate(rows) if row & (1 << 24))
    result = value.to_bytes(3, "big")
    assert crc24(body + result) == CRC_GOOD
    return result


def find_d0_in_region(memory: bytes, base_address: int) -> tuple[int, bytearray] | None:
    offset = 0
    while (offset := memory.find(SYNC, offset)) >= 0:
        if offset + HEADER_SIZE <= len(memory):
            size = struct.unpack_from("<I", memory, offset + 4)[0]
            name_offset = struct.unpack_from("<I", memory, offset + 0x0C)[0]
            if 0 < size < 4096 and name_offset < size and memory[offset + name_offset:offset + name_offset + 3] == b"d0\0":
                return base_address + offset, bytearray(memory[offset:offset + size])
        offset += 2
    return None


def find_d0(snapshot: bytes) -> tuple[int, bytearray]:
    """Locate d0 in either a raw RAM dump or QEMU's ELF64 core-dump format."""
    if snapshot.startswith(b"\x7fELF") and snapshot[4:6] == b"\x02\x01":
        program_offset = struct.unpack_from("<Q", snapshot, 0x20)[0]
        entry_size = struct.unpack_from("<H", snapshot, 0x36)[0]
        entries = struct.unpack_from("<H", snapshot, 0x38)[0]
        for index in range(entries):
            entry = program_offset + index * entry_size
            entry_type, _, file_offset, _, physical, file_size, _, _ = struct.unpack_from(
                "<IIQQQQQQ", snapshot, entry)
            if entry_type != 1:  # PT_LOAD
                continue
            found = find_d0_in_region(snapshot[file_offset:file_offset + file_size], physical)
            if found:
                return found
    else:
        found = find_d0_in_region(snapshot, 0)
        if found:
            return found
    raise ValueError("live d0 descriptor not found")


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("snapshot", type=Path)
    parser.add_argument("output", type=Path)
    parser.add_argument("--name", default="p0", help="two-character device name (default: p0)")
    parser.add_argument("--unit", type=int, default=0, help="floppy logical unit (default: 0)")
    args = parser.parse_args()
    if len(args.name) != 2 or not args.name.isascii() or "\0" in args.name:
        parser.error("--name must be exactly two ASCII characters")
    if not 0 <= args.unit <= 0xFFFF:
        parser.error("--unit must be an unsigned 16-bit value")
    address, module = find_d0(args.snapshot.read_bytes())
    if struct.unpack_from("<H", module, 0x12)[0] != 0x0F01:
        raise ValueError("candidate is not a device descriptor")
    fmgr_offset = struct.unpack_from("<I", module, 0x64)[0]
    name_offset = struct.unpack_from("<I", module, 0x0C)[0]
    if module[fmgr_offset:fmgr_offset + 4] != b"rbf\0" or module[name_offset:name_offset + 3] != b"d0\0":
        raise ValueError("candidate does not have the expected XiBase d0 layout")
    module[fmgr_offset:fmgr_offset + 4] = b"pcf\0"
    module[name_offset:name_offset + 3] = args.name.encode("ascii") + b"\0"
    # The native x86 RB765 d1_3.d1 -> md1_3.md1 pair is the exact PCF
    # conversion reference.  It changes dd_type to 10, clears the PCF flag
    # at 0xB0 and the four-byte reserved word at 0xE8.  It also mirrors the
    # floppy unit in RB765's private tail at 0x1EA.
    struct.pack_into("<H", module, DD_LU_NUM, args.unit)
    module[RB765_UNIT_MIRROR] = args.unit
    struct.pack_into("<H", module, DD_TYPE, DT_PCF_X86)
    module[PCF_FLAG_OFFSET] = 0
    module[PCF_RESERVED_OFFSET:PCF_RESERVED_OFFSET + 4] = b"\0" * 4
    module[-3:] = solve_crc(bytes(module[:-3]))
    args.output.write_bytes(module)
    print(f"wrote {args.output} ({len(module)} bytes; source physical offset {address:#x}; "
          f"device {args.name}, unit {args.unit})")


if __name__ == "__main__":
    main()
