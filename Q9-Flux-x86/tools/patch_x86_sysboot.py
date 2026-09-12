#!/usr/bin/env python3
"""Patch an OS-9000/x86 XiBase sysboot container offline.

The x86 BIOS bootstrap expects the XiBase ``OS9Z`` container, not the raw
module stream produced by a guest-side ``bootgen`` run.  This tool replaces a
module in the decompressed stream, rebuilds the zlib container, and writes a
new disposable image.  It deliberately refuses to grow the existing file
allocation; image allocation changes need a full x86-RBF writer.
"""
from __future__ import annotations

import argparse
import struct
import zlib
from pathlib import Path

SECTOR = 512
PARTITION = 17 * SECTOR
ROOT_DIR = PARTITION + 0x800
ENTRY_SIZE = 64
FD_SYNC = b"\xfd\xb0\xb0\xfd"
OS9Z = b"OS9Z"


def u32(data: bytes | bytearray | memoryview, offset: int) -> int:
    return struct.unpack_from("<I", data, offset)[0]


def xor_words(block: bytes | bytearray | memoryview) -> int:
    value = 0
    for offset in range(0, SECTOR, 4):
        value ^= u32(block, offset)
    return value


def module_at(stream: bytes, wanted: str) -> tuple[int, int]:
    for offset in range(0, len(stream) - 16):
        if stream[offset:offset + 2] != b"\xfcJ":
            continue
        size = u32(stream, offset + 4)
        name_offset = u32(stream, offset + 12)
        if size < 64 or offset + size > len(stream) or name_offset >= size:
            continue
        name = stream[offset + name_offset:offset + size].split(b"\0", 1)[0]
        if name.decode("ascii", "ignore") == wanted:
            return offset, size
    raise ValueError(f"module {wanted!r} not found in OS9Z payload")


def read_sysboot(image: bytearray) -> tuple[int, int, memoryview, list[tuple[int, int]], bytes]:
    entry_offset = None
    for offset in range(ROOT_DIR, ROOT_DIR + 4096, ENTRY_SIZE):
        entry = image[offset:offset + ENTRY_SIZE]
        name = bytes(entry[:44]).split(b"\0", 1)[0]
        if name == b"sysboot":
            entry_offset = offset
            break
    if entry_offset is None:
        raise ValueError("XiBase root entry 'sysboot' not found")

    logical_fd = u32(image, entry_offset + 60)
    fd_offset = PARTITION + (logical_fd + 1) * SECTOR
    fd = memoryview(image)[fd_offset:fd_offset + SECTOR]
    if bytes(fd[:4]) != FD_SYNC:
        raise ValueError(f"sysboot descriptor at LSN {logical_fd:#x} is invalid")

    segments: list[tuple[int, int]] = []
    for offset in range(0x30, SECTOR, 8):
        start, count = struct.unpack_from("<II", fd, offset)
        if not start and not count:
            break
        if not count:
            raise ValueError("sysboot descriptor contains a zero-length segment")
        segments.append((start, count))

    size = u32(fd, 0x14)
    payload = bytearray()
    for start, count in segments:
        begin = PARTITION + (start + 1) * SECTOR
        payload.extend(image[begin:begin + count * SECTOR])
    return logical_fd, fd_offset, fd, segments, bytes(payload[:size])


def descriptor(image: bytearray, logical_fd: int) -> tuple[int, memoryview, list[tuple[int, int]], int]:
    """Return an x86-RBF descriptor and its allocated data segments."""
    fd_offset = PARTITION + (logical_fd + 1) * SECTOR
    fd = memoryview(image)[fd_offset:fd_offset + SECTOR]
    if bytes(fd[:4]) != FD_SYNC:
        raise ValueError(f"descriptor at LSN {logical_fd:#x} is invalid")
    segments: list[tuple[int, int]] = []
    for offset in range(0x30, SECTOR, 8):
        start, count = struct.unpack_from("<II", fd, offset)
        if not start and not count:
            break
        if not count:
            raise ValueError("descriptor contains a zero-length segment")
        segments.append((start, count))
    return fd_offset, fd, segments, u32(fd, 0x14)


def file_data(image: bytearray, logical_fd: int) -> bytes:
    _, _, segments, size = descriptor(image, logical_fd)
    payload = bytearray()
    for start, count in segments:
        begin = PARTITION + (start + 1) * SECTOR
        payload.extend(image[begin:begin + count * SECTOR])
    return bytes(payload[:size])


def find_child(image: bytearray, parent: str, child: str) -> int:
    """Find a file descriptor LSN in a named root directory."""
    parent_fd = None
    for offset in range(ROOT_DIR, ROOT_DIR + 4096, ENTRY_SIZE):
        entry = image[offset:offset + ENTRY_SIZE]
        if bytes(entry[:56]).split(b"\0", 1)[0] == parent.encode("ascii"):
            parent_fd = u32(entry, 60)
            break
    if parent_fd is None:
        raise ValueError(f"root directory {parent!r} not found")
    directory = file_data(image, parent_fd)
    for offset in range(0, len(directory), ENTRY_SIZE):
        entry = directory[offset:offset + ENTRY_SIZE]
        if bytes(entry[:56]).split(b"\0", 1)[0] == child.encode("ascii"):
            return u32(entry, 60)
    raise ValueError(f"file /{parent}/{child} not found")


def replace_file(image: bytearray, logical_fd: int, data: bytes) -> None:
    fd_offset, fd, segments, _ = descriptor(image, logical_fd)
    capacity = sum(count for _, count in segments) * SECTOR
    if len(data) > capacity:
        raise ValueError(f"replacement file needs {len(data)} bytes, allocation holds {capacity}")
    for start, count in segments:
        begin = PARTITION + (start + 1) * SECTOR
        image[begin:begin + count * SECTOR] = b"\0" * (count * SECTOR)
    written = 0
    for start, count in segments:
        begin = PARTITION + (start + 1) * SECTOR
        amount = min(count * SECTOR, len(data) - written)
        image[begin:begin + amount] = data[written:written + amount]
        written += amount
        if written == len(data):
            break
    struct.pack_into("<I", fd, 0x14, len(data))
    struct.pack_into("<I", fd, 4, 0)
    struct.pack_into("<I", fd, 4, xor_words(fd) ^ 0xFFFFFFFF)


NETWORK_LINES = (
    b"* Configure Q9-Flux NE2000 networking",
    b"ndbmod interface del enet0",
    b"ndbmod interface add enet0 binding /spne0/enet",
    b"dhcp enet0 -override -v -timeout 3 -tries 1 &",
)


def add_network_startup(data: bytes) -> bytes:
    """Insert the persistent NE2000 rebind/DHCP sequence once."""
    if b"ndbmod interface add enet0 binding /spne0/enet" in data:
        return data
    newline = b"\r\n" if b"\r\n" in data else b"\r" if b"\r" in data else b"\n"
    block = newline.join(NETWORK_LINES) + newline
    marker = b"iniz r0 h0 d0 t1 term"
    position = data.find(marker)
    if position >= 0:
        end = data.find(newline, position)
        end = len(data) if end < 0 else end + len(newline)
        return data[:end] + block + data[end:]
    return block + data


def add_module_startup(data: bytes, module_name: str) -> bytes:
    """Run an inserted module once during startup, before the interactive shell."""
    command = module_name.encode("ascii")
    if command + b"\r" in data or command + b"\n" in data:
        return data
    newline = b"\r\n" if b"\r\n" in data else b"\r" if b"\r" in data else b"\n"
    block = b"* Q9-x86 test module\n" + command + newline
    marker = b"chd /dd"
    position = data.find(marker)
    if position >= 0:
        return data[:position] + block + data[position:]
    return data + (b"" if data.endswith(newline) else newline) + block


def build_container(old: bytes, module: Path, module_name: str, add_module: bool = False) -> bytes:
    if old[:4] != OS9Z or u32(old, 4) != 0x12345678 or u32(old, 8) != 0x20:
        raise ValueError("sysboot is not an OS9Z container")
    compressed_size = u32(old, 16)
    if 32 + compressed_size > len(old):
        raise ValueError("OS9Z compressed payload is truncated")
    compressed = old[32:32 + compressed_size]
    stream = zlib.decompress(compressed)
    replacement = module.read_bytes()
    if not replacement:
        raise ValueError("replacement module is empty")
    try:
        start, size = module_at(stream, module_name)
    except ValueError:
        if not add_module:
            raise
        stream = stream + replacement
    else:
        stream = stream[:start] + replacement + stream[start + size:]

    compressed = zlib.compress(stream, 9)
    opaque = u32(old, 20)
    header1 = struct.pack(
        "<4s7I", OS9Z, 0x12345678, 0x20, len(stream), len(compressed),
        opaque, zlib.crc32(compressed) & 0xFFFFFFFF, 0,
    )
    header2 = struct.pack(
        "<4s7I", OS9Z, 0x87654321, 0x20, len(stream), len(compressed),
        opaque, zlib.crc32(compressed) & 0xFFFFFFFF, 0,
    )
    return header1 + compressed + header2


def patch_image(source: Path, output: Path, module: Path, module_name: str,
                startup: Path | None, network_startup: bool, add_module: bool,
                run_module: bool) -> None:
    if source.resolve() == output.resolve():
        raise ValueError("output image must differ from source image")
    image = bytearray(source.read_bytes())
    sysboot_fd, fd_offset, fd, segments, old = read_sysboot(image)
    container = build_container(old, module, module_name, add_module)
    capacity = sum(count for _, count in segments) * SECTOR
    if len(container) > capacity:
        raise ValueError(f"new sysboot needs {len(container)} bytes, allocation holds {capacity}")

    replace_file(image, sysboot_fd, container)
    startup_data = startup.read_bytes() if startup is not None else None
    if network_startup:
        startup_fd = find_child(image, "SYS", "startup")
        startup_data = add_network_startup(file_data(image, startup_fd))
    if run_module:
        startup_fd = find_child(image, "SYS", "startup")
        startup_data = add_module_startup(startup_data if startup_data is not None else file_data(image, startup_fd), module_name)
    if startup_data is not None:
        startup_fd = find_child(image, "SYS", "startup")
        replace_file(image, startup_fd, startup_data)
    output.write_bytes(image)
    detail = ", network startup enabled" if network_startup else (
        f", startup {startup}" if startup is not None else "")
    print(f"patched {output}: {module_name} {len(old)} -> {len(container)} bytes{detail}")


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("source_image", type=Path)
    parser.add_argument("output_image", type=Path)
    parser.add_argument("module", type=Path)
    parser.add_argument("--module-name", default="init")
    parser.add_argument("--add-module", action="store_true", help="append module when it is not already in OS9Z")
    parser.add_argument("--run-module", action="store_true", help="add module name to /SYS/startup")
    parser.add_argument("--startup", type=Path,
                        help="replace /SYS/startup with this x86-RBF file")
    parser.add_argument("--network-startup", action="store_true",
                        help="insert the NE2000 rebind and DHCP commands into /SYS/startup")
    args = parser.parse_args()
    patch_image(args.source_image, args.output_image, args.module, args.module_name,
                args.startup, args.network_startup, args.add_module, args.run_module)


if __name__ == "__main__":
    main()
