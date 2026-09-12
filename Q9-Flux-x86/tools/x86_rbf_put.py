#!/usr/bin/env python3
"""Install one host file in a guest-formatted OS-9000/x86 RBF floppy.

This deliberately narrow first version replaces an existing root-directory
placeholder.  It is sufficient for the ``probe -> p1`` staging workflow and
avoids making unverified assumptions about directory growth.
"""
from __future__ import annotations

import argparse
import struct
from pathlib import Path


BLOCK_SIZE = 512
FD_SYNC = b"\xfd\xb0\xb0\xfd"
ID_SYNC = b"\xad\xb0\xb0\xad"
ENTRY_SIZE = 64
ENTRY_NAME_SIZE = 56
ENTRY_ADDRESS = 60
FD_PARITY = 4
FD_SIZE = 20
FD_SEGMENTS = 48


def xor32(block: bytes) -> int:
    value = 0
    for offset in range(0, len(block), 4):
        value ^= struct.unpack_from("<I", block, offset)[0]
    return value


def logical_to_physical(logical_block: int) -> int:
    # The guest format reports block offset 1: logical RBF block 0 maps to
    # physical block 1 in the raw floppy image.
    return logical_block + 1


def block_offset(block: int) -> int:
    return block * BLOCK_SIZE


def block_view(image: bytearray, block: int) -> memoryview:
    offset = block_offset(block)
    return memoryview(image)[offset:offset + BLOCK_SIZE]


def segment_pairs(fd: bytes) -> list[tuple[int, int]]:
    pairs = []
    for offset in range(FD_SEGMENTS, BLOCK_SIZE, 8):
        logical, count = struct.unpack_from("<II", fd, offset)
        if not logical or not count:
            break
        pairs.append((logical, count))
    return pairs


def root_descriptor(image: bytearray) -> tuple[int, memoryview]:
    """Find the descriptor whose directory data contains . and ..."""
    for physical in range(1, len(image) // BLOCK_SIZE):
        fd = block_view(image, physical)
        if fd[:4] != FD_SYNC:
            continue
        logical_self = physical - 1
        for logical, count in segment_pairs(fd):
            for item in range(count):
                directory = block_view(image, logical_to_physical(logical + item))
                names = [bytes(directory[offset:offset + ENTRY_NAME_SIZE]).split(b"\0", 1)[0]
                         for offset in range(0, BLOCK_SIZE, ENTRY_SIZE)]
                addresses = [struct.unpack_from("<I", directory, offset + ENTRY_ADDRESS)[0]
                             for offset in range(0, BLOCK_SIZE, ENTRY_SIZE)]
                if b"." in names and b".." in names and logical_self in addresses:
                    return physical, fd
    raise ValueError("OS-9000/x86 RBF root descriptor not found")


def root_entries(image: bytearray, root: memoryview):
    for logical, count in segment_pairs(root):
        for item in range(count):
            directory = block_view(image, logical_to_physical(logical + item))
            for offset in range(0, BLOCK_SIZE, ENTRY_SIZE):
                name = bytes(directory[offset:offset + ENTRY_NAME_SIZE]).split(b"\0", 1)[0]
                if name:
                    yield name.decode("ascii"), directory, offset


def bitmap_block(image: bytearray) -> memoryview:
    # The guest formatter places a bitmap in a dedicated block.  It is the
    # only block with a long 0xff run followed by allocation bits on a freshly
    # formatted 1.44-MiB image.
    for physical in range(1, len(image) // BLOCK_SIZE):
        candidate = block_view(image, physical)
        if bytes(candidate[:12]) == b"\xff" * 12 and candidate[12] not in (0, 0xff):
            return candidate
    # Native x86 RBF format marks a freshly formatted 1.44-MB floppy with a
    # compact bitmap: the first byte is 0xff and the remaining bytes are
    # initially zero.  The older heuristic above was written for a later
    # allocator layout and misses this valid form.
    for physical in range(1, len(image) // BLOCK_SIZE):
        candidate = block_view(image, physical)
        if candidate[0] == 0xff and candidate[1] == 0:
            return candidate
    raise ValueError("allocation bitmap not found")


def used(bitmap: memoryview, logical: int) -> bool:
    return bool(bitmap[logical // 8] & (1 << (7 - logical % 8)))


def mark_used(bitmap: memoryview, logical: int) -> None:
    bitmap[logical // 8] |= 1 << (7 - logical % 8)


def allocate_contiguous(bitmap: memoryview, blocks: int, limit: int) -> int:
    for logical in range(limit - blocks + 1):
        if all(not used(bitmap, candidate) for candidate in range(logical, logical + blocks)):
            for candidate in range(logical, logical + blocks):
                mark_used(bitmap, candidate)
            return logical
    raise ValueError("not enough contiguous RBF blocks")


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("source_image", type=Path, help="guest-formatted x86 RBF floppy")
    parser.add_argument("output_image", type=Path, help="new RBF floppy to write")
    parser.add_argument("host_file", type=Path)
    parser.add_argument("--replace", required=True, help="existing root placeholder, e.g. probe")
    parser.add_argument("--name", required=True, help="new root filename, e.g. p1")
    parser.add_argument("--template", default="pcf", help="existing regular root file to clone")
    parser.add_argument("--reuse-template-segment", action="store_true",
                        help="diagnostic mode: overwrite the template's already-valid data segment")
    parser.add_argument("--grow-template-segment", action="store_true",
                        help="extend the replacement template from its existing start block")
    parser.add_argument("--reuse-next-placeholder",
                        help="use the next placeholder's one-block segment and remove its directory entry")
    parser.add_argument("--preserve-template-descriptor", action="store_true",
                        help="diagnostic mode: keep the template descriptor byte-for-byte unchanged")
    args = parser.parse_args()
    if args.source_image.resolve() == args.output_image.resolve():
        parser.error("output_image must differ from source_image")
    if not args.name.isascii() or not args.name or len(args.name) > ENTRY_NAME_SIZE:
        parser.error("--name must be 1..56 ASCII characters")

    image = bytearray(args.source_image.read_bytes())
    if len(image) % BLOCK_SIZE or image[BLOCK_SIZE:BLOCK_SIZE + 4] != ID_SYNC:
        raise ValueError("not a guest-formatted OS-9000/x86 RBF floppy")
    data = args.host_file.read_bytes()
    if not data:
        raise ValueError("empty files are not supported by this initial injector")

    _, root = root_descriptor(image)
    entries = {name: (directory, offset) for name, directory, offset in root_entries(image, root)}
    if args.replace not in entries:
        raise ValueError(f"root placeholder {args.replace!r} not found")
    if args.template not in entries:
        raise ValueError(f"regular-file template {args.template!r} not found")
    if args.preserve_template_descriptor and args.replace != args.template:
        parser.error("--preserve-template-descriptor requires --replace to equal --template")
    if args.grow_template_segment and args.replace != args.template:
        parser.error("--grow-template-segment requires --replace to equal --template")

    target_directory, target_offset = entries[args.replace]
    target_logical = struct.unpack_from("<I", target_directory, target_offset + ENTRY_ADDRESS)[0]
    target_fd = block_view(image, logical_to_physical(target_logical))
    template_directory, template_offset = entries[args.template]
    template_logical = struct.unpack_from("<I", template_directory, template_offset + ENTRY_ADDRESS)[0]
    template_fd = bytes(block_view(image, logical_to_physical(template_logical)))
    if template_fd[:4] != FD_SYNC:
        raise ValueError("template is not an x86 RBF file descriptor")

    pair_logical = None
    pair_directory = None
    pair_offset = None
    if args.reuse_next_placeholder:
        if not args.reuse_template_segment:
            parser.error("--reuse-next-placeholder requires --reuse-template-segment")
        if args.reuse_next_placeholder not in entries:
            raise ValueError(f"placeholder {args.reuse_next_placeholder!r} not found")
        pair_directory, pair_offset = entries[args.reuse_next_placeholder]
        pair_fd_logical = struct.unpack_from("<I", pair_directory, pair_offset + ENTRY_ADDRESS)[0]
        pair_fd = bytes(block_view(image, logical_to_physical(pair_fd_logical)))
        pair_segments = segment_pairs(pair_fd)
        if len(pair_segments) != 1 or pair_segments[0][1] != 1:
            raise ValueError("next placeholder must provide exactly one data block")
        # The placeholder descriptor block itself is inside the valid RBF
        # volume.  Its advertised data block may be a diagnostic/out-of-range
        # placeholder, so reuse the descriptor block after removing its entry.
        pair_logical = pair_fd_logical

    data_blocks = (len(data) + BLOCK_SIZE - 1) // BLOCK_SIZE
    if data_blocks < 2:
        raise ValueError("initial injector is intentionally for multi-block files")
    if args.grow_template_segment:
        template_segments = segment_pairs(template_fd)
        if len(template_segments) != 1:
            raise ValueError("template must have one data segment to grow")
        data_logical = template_segments[0][0]
        segment_blocks = data_blocks
    elif args.reuse_template_segment:
        template_segments = segment_pairs(template_fd)
        if len(template_segments) != 1 or template_segments[0][1] < data_blocks:
            if pair_logical is None or len(template_segments) != 1 or template_segments[0][1] != 1:
                raise ValueError("template does not provide one sufficiently large data segment")
        data_logical = template_segments[0][0]
        segment_blocks = template_segments[0][1]
    else:
        bitmap = bitmap_block(image)
        data_logical = allocate_contiguous(bitmap, data_blocks, len(image) // BLOCK_SIZE - 1)
        segment_blocks = data_blocks
    data_locations = ([data_logical, pair_logical] if pair_logical is not None
                      else list(range(data_logical, data_logical + data_blocks)))

    if not args.preserve_template_descriptor:
        # Preserve the native descriptor's private tail/check fields.  The
        # x86 RBF driver validates bytes beyond the public segment list; a
        # zeroed tail produces E_Sect even when the data blocks are correct.
        target_fd[:] = template_fd
        struct.pack_into("<I", target_fd, FD_SIZE, len(data))
        struct.pack_into("<II", target_fd, FD_SEGMENTS, data_logical, 1 if pair_logical else segment_blocks)
        if pair_logical is not None:
            struct.pack_into("<II", target_fd, FD_SEGMENTS + 8, pair_logical, 1)
        struct.pack_into("<I", target_fd, FD_PARITY, 0)
        struct.pack_into("<I", target_fd, FD_PARITY, xor32(target_fd) ^ 0xFFFFFFFF)
        assert xor32(target_fd) == 0xFFFFFFFF

    for index in range(data_blocks):
        payload = data[index * BLOCK_SIZE:(index + 1) * BLOCK_SIZE]
        target = block_view(image, logical_to_physical(data_locations[index]))
        target[:] = payload.ljust(BLOCK_SIZE, b"\0")

    target_directory[target_offset:target_offset + ENTRY_NAME_SIZE] = b"\0" * ENTRY_NAME_SIZE
    target_directory[target_offset:target_offset + len(args.name)] = args.name.encode("ascii")
    if pair_directory is not None and pair_offset is not None:
        pair_directory[pair_offset] = 0
    args.output_image.write_bytes(image)
    print(f"wrote {args.output_image}: {args.name} ({len(data)} bytes), "
          f"descriptor logical block {target_logical}, data logical blocks {data_logical}..{data_logical + data_blocks - 1}")


if __name__ == "__main__":
    main()
