#!/usr/bin/env python3
"""Patch the OS-9000/x86 Init module's SPF database startup command.

The input is an extracted module (for example from a QEMU RAM dump).  The
module size and layout remain unchanged; only the SYS_PARAMS string and its
module CRC are updated.
"""
from __future__ import annotations

import argparse
from pathlib import Path

from build_pcf_descriptor import crc24


SYNC = b"\xfcJ"
OLD_PREFIX = b"setenv SHELL"
STARTUP_MARKER = b"dbglog_p2"
CORRECTED_PARAMS = (
    b"setenv SHELL mshell ; alias /dd /hc1 ; chd /dd ; chx /dd/CMDS ; "
    b"mbinstall ; ndbmod create inetdb3 11 400 0 160 0 0 0 100 0 400 65 256 ; "
    b"ipstart ; /dd/SYS/startup & "
)


def patch_module(data: bytes) -> bytes:
    if data[:2] != SYNC:
        raise ValueError("not an OS-9000 module")
    size = int.from_bytes(data[4:8], "little")
    name_offset = int.from_bytes(data[12:16], "little")
    if size != len(data):
        raise ValueError(f"module size {size:#x} does not match input length {len(data):#x}")
    if data[name_offset:name_offset + 4] != b"init":
        raise ValueError("input module is not named init")
    start = data.find(OLD_PREFIX)
    end = data.find(STARTUP_MARKER, start)
    if start < 0 or end < 0:
        raise ValueError("Init SYS_PARAMS string not found")
    old = data[start:end]
    if len(CORRECTED_PARAMS) > len(old):
        raise ValueError("corrected SYS_PARAMS does not fit reserved string area")
    result = bytearray(data)
    result[start:end] = CORRECTED_PARAMS.ljust(len(old), b"\0")
    result[-3:] = b"\0\0\0"
    result[-3:] = solve_crc(bytes(result[:-3]))
    if crc24(bytes(result)) != 0x800FE3:
        raise AssertionError("module CRC verification failed")
    return bytes(result)


def solve_crc(body: bytes) -> bytes:
    base = crc24(body + b"\0\0\0")
    wanted = base ^ 0x800FE3
    columns = [crc24(body + (1 << (23 - bit)).to_bytes(3, "big")) ^ base for bit in range(24)]
    rows = []
    for equation in range(24):
        row = sum((1 << (23 - variable)) for variable, column in enumerate(columns)
                  if column & (1 << (23 - equation)))
        rows.append(row | (((wanted >> (23 - equation)) & 1) << 24))
    for pivot in range(24):
        candidate = next(index for index in range(pivot, 24) if rows[index] & (1 << (23 - pivot)))
        rows[pivot], rows[candidate] = rows[candidate], rows[pivot]
        for index in range(24):
            if index != pivot and rows[index] & (1 << (23 - pivot)):
                rows[index] ^= rows[pivot]
    value = sum((1 << (23 - index)) for index, row in enumerate(rows) if row & (1 << 24))
    return value.to_bytes(3, "big")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("input", type=Path)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    if args.input.resolve() == args.output.resolve():
        parser.error("output must differ from input")
    patched = patch_module(args.input.read_bytes())
    args.output.write_bytes(patched)
    print(f"wrote {args.output}: {len(patched)} bytes, CRC {patched[-3:].hex()}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
