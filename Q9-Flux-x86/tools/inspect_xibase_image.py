#!/usr/bin/env python3
"""Read-only structural inspector for the OS-9000 XiBase volume."""
from __future__ import annotations
import argparse
from pathlib import Path

PARTITION_OFFSET = 17 * 512
ROOT_OFFSET = PARTITION_OFFSET + 0x800
ENTRY_SIZE = 64


def descriptor_segments(data: bytes, lsn: int) -> tuple[int, list[tuple[int, int]]] | None:
    offset = PARTITION_OFFSET + lsn * 512
    fd = data[offset:offset + 512]
    if len(fd) != 512 or fd[:4] != b"\xfd\xb0\xb0\xfd":
        return None
    size = int.from_bytes(fd[0x14:0x18], "little")
    segments = []
    for index in range(0x30, 512, 8):
        start = int.from_bytes(fd[index:index + 4], "little")
        count = int.from_bytes(fd[index + 4:index + 8], "little")
        if start == 0 and count == 0:
            break
        segments.append((start, count))
    return size, segments


def show_descriptor(data: bytes, lsn: int, label: str) -> tuple[int, list[tuple[int, int]]] | None:
    result = descriptor_segments(data, lsn)
    if result is None:
        print(f"  {label}: descriptor at LSN={lsn:#x} uses an unrecognised format")
        return None
    size, segments = result
    print(f"  {label}: fd LSN={lsn:#x}, size={size}, segments=" + ", ".join(f"{start:#x}+{count}" for start, count in segments))
    return result


def list_directory(data: bytes, segments: list[tuple[int, int]], label: str) -> None:
    print(f"  {label} directory entries:")
    for start, count in segments:
        begin = PARTITION_OFFSET + start * 512
        for offset in range(begin, begin + count * 512, ENTRY_SIZE):
            entry = data[offset:offset + ENTRY_SIZE]
            name = entry[:44].split(b"\0", 1)[0].decode("ascii", "replace")
            fd = int.from_bytes(entry[60:64], "little")
            if name:
                print(f"    {offset:#010x}  {name:<40} fd={fd:#x}")
            elif entry == bytes(ENTRY_SIZE):
                print(f"    {offset:#010x}  <free>")

def main() -> None:
    p = argparse.ArgumentParser()
    p.add_argument("image", type=Path)
    a = p.parse_args()
    data = a.image.read_bytes()
    boot = data[PARTITION_OFFSET:PARTITION_OFFSET + 512]
    if boot[:8] != b"\xeb\x0aXD00BT":
        raise SystemExit("not the expected XiBase OS-9000 boot wrapper")
    print(f"XiBase wrapper: partition offset {PARTITION_OFFSET:#x}; root directory {ROOT_OFFSET:#x}")
    for off in range(ROOT_OFFSET, ROOT_OFFSET + 4096, ENTRY_SIZE):
        entry = data[off:off + ENTRY_SIZE]
        name = entry[:44].split(b"\0", 1)[0].decode("ascii", "replace")
        fd = int.from_bytes(entry[60:64], "little")
        if name:
            print(f"{off:#010x}  {name:<40} fd={fd:#x}")
            if name in {"CMDS", "SYS"}:
                result = show_descriptor(data, fd, name)
                if name == "CMDS" and result:
                    print("  CMDS directory block scaling is not yet validated; no entries decoded")

if __name__ == "__main__":
    main()
