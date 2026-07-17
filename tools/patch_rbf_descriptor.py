#!/usr/bin/env python3
"""Patch a CB030 RBF descriptor name and partition LSN offset."""
import argparse
import struct
from pathlib import Path

from patch_pks_descriptor import stored_crc, os9_crc

def patch(inp: Path, out: Path, name: str, lsn: int, part_size: int, base: int,
          driver: str | None, media_type: int | None) -> None:
    data = bytearray(inp.read_bytes())
    size = struct.unpack_from(">I", data, 4)[0]
    if size != len(data) or struct.unpack_from(">H", data, 0)[0] != 0x4AFC:
        raise ValueError("not a standalone OS-9 module")
    name_off = struct.unpack_from(">I", data, 12)[0]
    # Older generated copies incorrectly advanced the name pointer when the
    # driver string grew by one byte. Repair that harmless stale offset.
    if name_off >= len(data):
        name_off -= 1
        struct.pack_into(">I", data, 12, name_off)
    old_end = data.index(0, name_off)
    if len(name) > old_end - name_off:
        raise ValueError("descriptor name does not fit template")
    data[name_off:old_end + 1] = name.encode("ascii") + b"\0" * (old_end - name_off + 1 - len(name))
    struct.pack_into(">I", data, 0x30, base)
    # PD_LSNOffs in the CB030 e0 descriptor (big-endian long).
    struct.pack_into(">I", data, 0x68, lsn)
    # RBFDesc.PD_PartSiz (explicit partition length in logical sectors).
    struct.pack_into(">I", data, 0x74, part_size)
    if media_type is not None:
        data[0x5F] = media_type & 0xFF
    if driver:
        drv_off = struct.unpack_from(">H", data, 0x3A)[0]
        old_end = data.index(0, drv_off)
        old_len = old_end - drv_off
        if len(driver) > old_len:
            extra = len(driver) - old_len
            data[old_end:old_end] = b"\0" * extra
            struct.pack_into(">I", data, 4, len(data))
            old_end += extra
        data[drv_off:old_end + 1] = driver.encode("ascii") + b"\0" * (old_end + 1 - drv_off - len(driver))
    parity = 0
    for i in range(0, 0x2e, 2):
        parity ^= (data[i] << 8) | data[i + 1]
    struct.pack_into(">H", data, 0x2e, 0xffff ^ parity)
    data[-3:] = stored_crc(data[:-3])
    if os9_crc(data) != (0x80, 0x0f, 0xe3):
        raise ValueError("CRC repair failed")
    out.write_bytes(data)

if __name__ == "__main__":
    ap = argparse.ArgumentParser()
    ap.add_argument("input", type=Path)
    ap.add_argument("output", type=Path)
    ap.add_argument("--name", required=True)
    ap.add_argument("--lsn", type=lambda x: int(x, 0), required=True)
    ap.add_argument("--part-size", type=lambda x: int(x, 0), required=True)
    ap.add_argument("--base", type=lambda x: int(x, 0), required=True)
    ap.add_argument("--driver")
    ap.add_argument("--media-type", type=lambda x: int(x, 0))
    a = ap.parse_args()
    patch(a.input, a.output, a.name, a.lsn, a.part_size, a.base, a.driver, a.media_type)
    print(f"patched {a.output}: name={a.name} base=${a.base:08X} PD_LSNOffs={a.lsn} PD_PartSiz={a.part_size}")
