#!/usr/bin/env python3
"""Inspect ELF32/i386 objects for the planned OS-9000/x86 packer.

The first milestone is intentionally read-only.  It validates the input and
prints the information a packer must consume; unsupported ELF variants or
relocations fail loudly instead of producing a plausible but invalid module.
"""
from __future__ import annotations

import argparse
import struct
from dataclasses import dataclass
from pathlib import Path


ELF32_EHDR = struct.Struct("<16sHHIIIIIHHHHHH")
ELF32_SHDR = struct.Struct("<IIIIIIIIII")
ELF32_SYM = struct.Struct("<IIIBBH")
ELF32_REL = struct.Struct("<II")
ET_REL = 1
ET_EXEC = 2
EM_386 = 3
SHT_SYMTAB = 2
SHT_STRTAB = 3
SHT_RELA = 4
SHT_REL = 9
MH_SIZE = 80
OS9K_SYNC = b"\xfc\x4a"


@dataclass(frozen=True)
class Section:
    index: int
    name: str
    sh_type: int
    flags: int
    offset: int
    size: int
    link: int
    info: int
    align: int
    entsize: int


def cstring(blob: bytes, offset: int) -> str:
    if offset < 0 or offset >= len(blob):
        return "<bad-name>"
    return blob[offset:].split(b"\0", 1)[0].decode("ascii", "replace")


def parse(path: Path) -> tuple[bytes, list[Section], list[dict], list[dict]]:
    data = path.read_bytes()
    if len(data) < ELF32_EHDR.size or data[:4] != b"\x7fELF":
        raise ValueError("not an ELF file")
    ident, typ, machine, version, entry, phoff, shoff, flags, ehsize, phentsize, phnum, shentsize, shnum, shstrndx = ELF32_EHDR.unpack_from(data)
    if ident[4] != 1 or ident[5] != 1:
        raise ValueError("only little-endian ELF32 is supported")
    if typ not in (ET_REL, ET_EXEC) or machine != EM_386:
        raise ValueError(f"expected ELF32/i386 ET_REL or ET_EXEC (type={typ}, machine={machine})")
    if shentsize != ELF32_SHDR.size or shoff + shnum * shentsize > len(data):
        raise ValueError("invalid section header table")
    raw = [ELF32_SHDR.unpack_from(data, shoff + i * shentsize) for i in range(shnum)]
    if shstrndx >= shnum:
        raise ValueError("invalid section-name string table")
    shstr = raw[shstrndx]
    names = data[shstr[4]:shstr[4] + shstr[5]]
    sections = [Section(i, cstring(names, r[0]), r[1], r[2], r[4], r[5], r[6], r[7], r[8], r[9]) for i, r in enumerate(raw)]
    strings: dict[int, bytes] = {}
    for sec in sections:
        if sec.sh_type == SHT_STRTAB:
            strings[sec.index] = data[sec.offset:sec.offset + sec.size]
    symbols: list[dict] = []
    for sec in sections:
        if sec.sh_type != SHT_SYMTAB:
            continue
        if sec.entsize != ELF32_SYM.size or sec.link not in strings:
            raise ValueError(f"invalid symbol table {sec.name}")
        strtab = strings[sec.link]
        for off in range(sec.offset, sec.offset + sec.size, sec.entsize):
            name, value, size, info, other, shndx = ELF32_SYM.unpack_from(data, off)
            symbols.append({"index": len(symbols), "name": cstring(strtab, name), "value": value, "size": size, "bind": info >> 4, "type": info & 0xf, "section": shndx})
    relocs: list[dict] = []
    for sec in sections:
        if sec.sh_type == SHT_RELA:
            raise ValueError(f"unsupported RELA relocation section {sec.name}; i386 OS-9000 milestone uses SHT_REL")
        if sec.sh_type != SHT_REL:
            continue
        if sec.entsize != ELF32_REL.size:
            raise ValueError(f"invalid relocation table {sec.name}")
        for off in range(sec.offset, sec.offset + sec.size, sec.entsize):
            r_offset, r_info = ELF32_REL.unpack_from(data, off)
            relocs.append({"section": sec.name, "target_section": sec.info, "offset": r_offset, "symbol": r_info >> 8, "type": r_info & 0xff})
    return data, sections, symbols, relocs


def crc24(data: bytes) -> int:
    value = 0xFFFFFF
    for byte in data:
        value ^= byte << 16
        for _ in range(8):
            value = ((value << 1) ^ 0x800063) & 0xFFFFFF if value & 0x800000 else (value << 1) & 0xFFFFFF
    return value


def crc_bytes(body: bytes) -> bytes:
    """Return the three OS-9000 CRC bytes (full-module residue 0x800fe3)."""
    base = crc24(body + b"\0\0\0")
    wanted = base ^ 0x800FE3
    columns = [crc24(body + (1 << (23 - bit)).to_bytes(3, "big")) ^ base for bit in range(24)]
    rows = []
    for equation in range(24):
        row = sum(1 << (23 - variable) for variable, column in enumerate(columns)
                  if column & (1 << (23 - equation)))
        rows.append(row | (((wanted >> (23 - equation)) & 1) << 24))
    for pivot in range(24):
        candidate = next(i for i in range(pivot, 24) if rows[i] & (1 << (23 - pivot)))
        rows[pivot], rows[candidate] = rows[candidate], rows[pivot]
        for i in range(24):
            if i != pivot and rows[i] & (1 << (23 - pivot)):
                rows[i] ^= rows[pivot]
    value = sum(1 << (23 - i) for i, row in enumerate(rows) if row & (1 << 24))
    return value.to_bytes(3, "big")


def header_parity(header: bytearray) -> None:
    """Set the native x86 module-header XOR parity (44 little-endian words)."""
    value = 0
    for offset in range(0, 0x58, 2):
        if offset != 0x56:
            value ^= struct.unpack_from("<H", header, offset)[0]
    struct.pack_into("<H", header, 0x56, 0xFFFF ^ value)


def pack_minimal(data: bytes, sections: list[Section], symbols: list[dict], relocs: list[dict], name: str) -> bytes:
    """Emit the deliberately small, experimental no-relocation module form."""
    if relocs:
        raise ValueError("minimal writer only accepts objects without relocations")
    text_sections = [s for s in sections if s.name in (".text", "text")]
    if len(text_sections) != 1:
        raise ValueError("minimal writer requires exactly one .text section")
    text = data[text_sections[0].offset:text_sections[0].offset + text_sections[0].size]
    entry = next((s for s in symbols if s["name"] in ("__start", "_start") and s["section"] == text_sections[0].index), None)
    if entry is None:
        raise ValueError("minimal writer requires a __start or _start symbol in .text")
    if entry["value"] >= len(text):
        raise ValueError("entry symbol is outside .text")
    module_name = name.encode("ascii") + b"\0"
    # OS-9000/x86 user modules reserve an 8-byte post-header area and begin
    # executable code at 0x60 (as observed in the resident echo module).
    code_offset = 0x60
    name_offset = code_offset + len(text)
    module_size = name_offset + len(module_name) + 3
    # The common x86 header fields occupy 0x50 bytes; the loader's parity
    # word lives in the extended 0x58-byte header boundary.
    header = bytearray(MH_SIZE)
    header.extend(b"\0" * (0x58 - len(header)))
    header[0:2] = OS9K_SYNC
    struct.pack_into("<H", header, 2, 2)  # OS-9000 system revision
    struct.pack_into("<I", header, 4, module_size)
    struct.pack_into("<I", header, 12, name_offset)
    struct.pack_into("<H", header, 16, 0x0555)  # owner/group/world read+execute
    header[18] = 1  # x86 object code
    header[19] = 1  # program module
    struct.pack_into("<I", header, 36, code_offset + entry["value"])  # m_exec
    struct.pack_into("<I", header, 48, 0x1000)  # conservative process stack
    struct.pack_into("<H", header, 68, 0x0080)  # x86 module ident marker
    header_parity(header)
    body = bytes(header) + b"\0" * (code_offset - len(header)) + text + module_name
    return body + crc_bytes(body)


def pack_with_template(data: bytes, sections: list[Section], symbols: list[dict], relocs: list[dict], template: Path, preserve_init: bool = False) -> bytes:
    """Patch a relocation-free text body into a known-good OS-9000 module."""
    if relocs:
        raise ValueError("template writer only accepts objects without relocations")
    image = bytearray(template.read_bytes())
    if image[:2] != OS9K_SYNC or len(image) < MH_SIZE + 3:
        raise ValueError("template is not an OS-9000/x86 module")
    size = struct.unpack_from("<I", image, 4)[0]
    if size != len(image):
        raise ValueError("template module size does not match file length")
    text_sections = [s for s in sections if s.name in (".text", "text")]
    if len(text_sections) != 1:
        raise ValueError("template writer requires exactly one .text section")
    text = data[text_sections[0].offset:text_sections[0].offset + text_sections[0].size]
    entry = next((s for s in symbols if s["name"] in ("__start", "_start") and s["section"] == text_sections[0].index), None)
    if entry is None or entry["value"] + len(text) > 0x1000:
        raise ValueError("invalid entry or oversized template text")
    exec_offset = struct.unpack_from("<I", image, 0x24)[0]
    # Resident x86 user modules enter through a short trampoline.  In the
    # known-good echo module the final near JMP in that trampoline targets the
    # C-style body at 0x11a; replacing m_exec itself destroys the ABI setup.
    code_offset = exec_offset
    for pos in range(exec_offset, min(exec_offset + 32, len(image) - 5)):
        if image[pos] == 0xE9:
            code_offset = pos + 5 + struct.unpack_from("<i", image, pos + 1)[0]
            break
    if preserve_init:
        marker = bytes.fromhex("8d45f4")  # first body-local stack temporary in echo
        body = image.find(marker, code_offset + 16)
        if body < 0:
            raise ValueError("could not locate template initialization/body boundary")
        code_offset = body
    idata_offset = struct.unpack_from("<I", image, 0x34)[0]
    limit = idata_offset if idata_offset else len(image) - 3
    if code_offset + entry["value"] + len(text) > limit:
        raise ValueError("ELF text does not fit before template initialized data")
    image[code_offset + entry["value"]:code_offset + entry["value"] + len(text)] = text
    image[-3:] = crc_bytes(bytes(image[:-3]))
    return bytes(image)


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("object", type=Path)
    ap.add_argument("--pack", type=Path, metavar="OUTPUT", help="write an experimental no-relocation OS-9000/x86 module")
    ap.add_argument("--name", default=None, help="module name for --pack (defaults to object filename)")
    ap.add_argument("--template", type=Path, help="known-good OS-9000/x86 module to preserve header/data layout")
    ap.add_argument("--preserve-init", action="store_true", help="preserve the template's entry initialization prefix")
    args = ap.parse_args()
    data, sections, symbols, relocs = parse(args.object)
    print(f"ELF32/i386 ET_REL: {args.object} ({len(data)} bytes)")
    print("Sections:")
    for sec in sections:
        print(f"  [{sec.index:2}] {sec.name or '<unnamed>':16} type={sec.sh_type:2} off=0x{sec.offset:x} size=0x{sec.size:x} flags=0x{sec.flags:x}")
    print("Symbols:")
    for sym in symbols:
        print(f"  [{sym['index']:2}] {sym['name'] or '<unnamed>':24} value=0x{sym['value']:x} size={sym['size']} section={sym['section']}")
    print("Relocations:")
    for rel in relocs:
        print(f"  {rel['section']}: offset=0x{rel['offset']:x} symbol={rel['symbol']} type={rel['type']}")
    if args.pack:
        name = args.name or args.object.stem
        output = (pack_with_template(data, sections, symbols, relocs, args.template, args.preserve_init)
                  if args.template else pack_minimal(data, sections, symbols, relocs, name))
        args.pack.write_bytes(output)
        print(f"wrote experimental OS-9000/x86 module {args.pack} ({len(output)} bytes)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
