#!/usr/bin/env python3
"""Disable paging pause in the OS-9 ``pks`` descriptor.

This is deliberately a narrow binary patch.  It targets the ``pks`` SCF
descriptor inside a merged OS-9 module file (normally ``/dd/netmods``), not
the ``pkdvr`` driver, ``pkman`` file manager, Q9's host ``nettty`` code, or
any source-tree configuration.

The option byte is module-relative offset 0x4f (PD_PAU, page-pause enable):
    0x01 -> 0x00

The three-byte OS-9 CRC at the end of the pks module is regenerated.  The
script accepts a standalone pks module too, which makes the byte-level change
and its verification easy to inspect before deploying it into an image.
"""

from __future__ import annotations

import argparse
import struct
from pathlib import Path


SYNC = 0x4AFC
MODULE_HEADER_SIZE = 48
MODULE_CRC_SIZE = 3
PKS_NAME = "pks"
PD_PAU_OFFSET = 0x4F


def os9_crc(data: bytes | bytearray) -> tuple[int, int, int]:
    """Return the OS-9 24-bit CRC state for *data*."""
    crc = [0xFF, 0xFF, 0xFF]
    for byte in data:
        a = byte ^ crc[0]
        crc[0] = crc[1]
        crc[1] = crc[2]
        crc[1] ^= (a >> 7) & 0xFF
        crc[2] = (a << 1) & 0xFF
        crc[1] ^= (a >> 2) & 0xFF
        crc[2] ^= (a << 6) & 0xFF
        a ^= (a << 1) & 0xFF
        a ^= (a << 2) & 0xFF
        a ^= (a << 4) & 0xFF
        a &= 0xFF
        if a & 0x80:
            crc[0] ^= 0x80
            crc[2] ^= 0x21
    return tuple(crc)


def stored_crc(body: bytes | bytearray) -> bytes:
    """Encode the CRC bytes stored at the end of an OS-9 module."""
    crc = os9_crc(body)
    return bytes(value ^ 0xFF for value in crc)


def module_name(data: bytes | bytearray, start: int) -> str:
    name_offset = struct.unpack_from(">I", data, start + 12)[0]
    if not name_offset or start + name_offset >= len(data):
        return ""
    pos = start + name_offset
    chars = bytearray()
    while pos < len(data) and len(chars) < 64:
        value = data[pos]
        pos += 1
        if (value & 0x7F) == 0:
            break
        chars.append(value & 0x7F)
        if value & 0x80:
            break
    return chars.decode("ascii", errors="replace")


def module_size(data: bytes | bytearray, start: int) -> int:
    return struct.unpack_from(">I", data, start + 4)[0]


def find_pks(data: bytes | bytearray) -> tuple[int, int]:
    """Find pks and return (start, size), accepting standalone or merge."""
    pos = 0
    while pos + MODULE_HEADER_SIZE <= len(data):
        if struct.unpack_from(">H", data, pos)[0] != SYNC:
            pos += 2
            continue
        size = module_size(data, pos)
        if (
            size < MODULE_HEADER_SIZE + MODULE_CRC_SIZE
            or pos + size > len(data)
            or pos + 12 >= len(data)
        ):
            pos += 2
            continue
        if module_name(data, pos) == PKS_NAME:
            return pos, size
        pos += size
    raise ValueError("OS-9 module 'pks' not found")


def verify_module(data: bytes | bytearray, start: int, size: int) -> None:
    module = data[start : start + size]
    if struct.unpack_from(">H", module, 0)[0] != SYNC:
        raise ValueError("pks has invalid OS-9 sync word")
    if module_size(module, 0) != size:
        raise ValueError("pks header size does not match module size")
    if len(module) < MODULE_HEADER_SIZE + MODULE_CRC_SIZE:
        raise ValueError("pks module is too small")
    if os9_crc(module) != (0x80, 0x0F, 0xE3):
        raise ValueError("pks CRC is invalid")
    parity = 0
    for index in range(0, MODULE_HEADER_SIZE, 2):
        parity ^= (module[index] << 8) | module[index + 1]
    if parity != 0xFFFF:
        raise ValueError(f"pks header parity is invalid: 0x{parity:04x}")


def patch(path: Path) -> tuple[int, int, int, int]:
    data = bytearray(path.read_bytes())
    start, size = find_pks(data)
    verify_module(data, start, size)
    absolute = start + PD_PAU_OFFSET
    old = data[absolute]
    if old != 0x01:
        raise ValueError(
            f"unexpected pks PD_PAU at module offset 0x{PD_PAU_OFFSET:x}: "
            f"0x{old:02x} (expected 0x01)"
        )
    data[absolute] = 0x00
    crc_start = start + size - MODULE_CRC_SIZE
    data[crc_start : start + size] = stored_crc(data[start:crc_start])
    verify_module(data, start, size)
    path.write_bytes(data)
    return start, size, old, data[absolute]


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("input", type=Path, help="merged netmods or standalone pks")
    parser.add_argument("output", type=Path, help="patched output file")
    args = parser.parse_args()

    if args.input.resolve() == args.output.resolve():
        raise SystemExit("input and output must differ (original is preserved)")
    args.output.write_bytes(args.input.read_bytes())
    start, size, old, new = patch(args.output)
    print(f"target module: pks")
    print(f"merge offset: 0x{start:x}")
    print(f"module size: {size} bytes")
    print(f"module offset 0x{PD_PAU_OFFSET:x} (PD_PAU): 0x{old:02x} -> 0x{new:02x}")
    print(f"patched: {args.output}")


if __name__ == "__main__":
    main()
