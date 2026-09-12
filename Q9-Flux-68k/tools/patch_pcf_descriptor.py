#!/usr/bin/env python3
"""Adapt a licensed OS-9 PCF descriptor template to the Q9 CF interface.

The Q9 port ships an RBF ``f0`` descriptor.  A PCF/FAT image needs a PCF
descriptor instead.  This tool keeps the PCF option table from a supplied
template, changing only the Q9 port, driver name and logical device name,
then repairs OS-9 header parity and CRC.
"""

from __future__ import annotations

import argparse
import struct
from pathlib import Path

from patch_pks_descriptor import module_name, os9_crc, stored_crc


def verify(module: bytes) -> None:
    if struct.unpack_from(">H", module, 0)[0] != 0x4AFC:
        raise ValueError("not an OS-9 module")
    if os9_crc(module) != (0x80, 0x0F, 0xE3):
        raise ValueError("module CRC invalid")
    parity = 0
    for index in range(0, 48, 2):
        parity ^= (module[index] << 8) | module[index + 1]
    if parity != 0xFFFF:
        raise ValueError(f"module header parity invalid: 0x{parity:04x}")


def c_string(data: bytearray, offset: int) -> str:
    end = data.index(0, offset)
    return bytes(data[offset:end]).decode("ascii")


def image_geometry(path: Path) -> tuple[int, int, int]:
    """Derive classic PCF CHS geometry from a 512-byte image file."""
    size = path.stat().st_size
    if size == 0 or size % 512:
        raise ValueError("image size must be a non-zero multiple of 512 bytes")
    sectors = size // 512
    heads, sectors_track = 255, 63
    cylinders = (sectors + heads * sectors_track - 1) // (heads * sectors_track)
    if cylinders > 0xFFFF:
        raise ValueError("image is too large for the descriptor cylinder field")
    return cylinders, heads, sectors_track


def patch(path: Path, output_name: str, base: int, lsn: int, format_enabled: bool,
          hard_autosize: bool, driver_name: str, geometry: tuple[int, int, int] | None) -> None:
    data = bytearray(path.read_bytes())
    size = struct.unpack_from(">I", data, 4)[0]
    if size != len(data):
        raise ValueError("template size does not match module header")
    verify(data)
    if module_name(data, 0) not in {"pcd0", "d0"}:
        raise ValueError("expected a PCF pcd0/d0 template")
    fm_off = struct.unpack_from(">H", data, 0x38)[0]
    drv_off = struct.unpack_from(">H", data, 0x3A)[0]
    con_off = struct.unpack_from(">H", data, 0x3C)[0]
    name_off = struct.unpack_from(">I", data, 0x0C)[0]
    if c_string(data, fm_off) != "pcf":
        raise ValueError("template is not a PCF descriptor")
    if len(driver_name) > len(c_string(data, drv_off)):
        raise ValueError("driver name does not fit template")
    data[0x30:0x34] = base.to_bytes(4, "big")
    data[0x68:0x6C] = lsn.to_bytes(4, "big")
    if hard_autosize:
        # The SDK pcd0 template is a 1.44-MB floppy (pcdos380).  Q9 uses
        # PCF only for FAT12/16 CF media, so advertise a hard, autosized
        # medium; PCF then obtains the actual geometry from the BPB.
        data[0x4B] = 0x80       # PD_TYP: Hard
        data[0x4C] = 0x00       # PD_DNS: single density (not floppy MFM)
        data[0x4E:0x50] = b"\0\0"  # cylinders (autosize)
        data[0x50] = 0           # heads (autosize)
        data[0x52:0x56] = b"\0\0\0\0"  # sectors/track (autosize)
        # Preserve format-disabled and multi-sector support, add AutoEnabl.
        control = struct.unpack_from(">H", data, 0x5E)[0]
        struct.pack_into(">H", data, 0x5E, control | 0x0008)
    if geometry:
        cylinders, heads, sectors_track = geometry
        data[0x4E:0x50] = cylinders.to_bytes(2, "big")
        data[0x50] = heads
        data[0x52:0x54] = sectors_track.to_bytes(2, "big")
        data[0x54:0x56] = sectors_track.to_bytes(2, "big")
        data[0x6C:0x6E] = cylinders.to_bytes(2, "big")
        # A fixed geometry descriptor must not ask the driver to autosize.
        control = struct.unpack_from(">H", data, 0x5E)[0]
        struct.pack_into(">H", data, 0x5E, control & ~0x0008)
    old_drv_end = data.index(0, drv_off)
    data[drv_off : old_drv_end + 1] = driver_name.encode("ascii") + b"\0" * (old_drv_end + 1 - drv_off - len(driver_name))
    # MVME pcd0 templates carry a board-specific SCSI DevCon string
    # (scsi147/scsi167/...).  Q9's CF driver has no such constants module;
    # keep the field valid while replacing the stale board name.
    devcon = c_string(data, con_off)
    if devcon.startswith("scsi"):
        if len(driver_name) > len(devcon):
            raise ValueError("driver name does not fit DevCon field")
        data[con_off : con_off + len(devcon) + 1] = (
            driver_name.encode("ascii") + b"\0" * (len(devcon) + 1 - len(driver_name))
        )
    old_end = data.index(0, name_off)
    if len(output_name) <= old_end - name_off:
        data[name_off : old_end + 1] = output_name.encode("ascii") + b"\0" * (old_end - name_off + 1 - len(output_name))
    else:
        body = data[:-3]
        body = body[:name_off] + output_name.encode("ascii") + b"\0" + body[old_end + 1:]
        data = bytearray(body + b"\0\0\0")
        struct.pack_into(">I", data, 4, len(data))
    if format_enabled:
        # PCFDesc.Control: clear FmtDsabl, preserving the other capability bits.
        control = struct.unpack_from(">H", data, 0x5E)[0]
        struct.pack_into(">H", data, 0x5E, control & ~1)
    parity = 0
    for index in range(0, 0x2E, 2):
        parity ^= (data[index] << 8) | data[index + 1]
    data[0x2E:0x30] = (0xFFFF ^ parity).to_bytes(2, "big")
    data[-3:] = stored_crc(data[:-3])
    verify(data)
    path.write_bytes(data)
    print(f"PCF descriptor: pcd0 -> {output_name}, driver {driver_name}, port ${base:08X}, {len(data)} bytes")


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("input", type=Path)
    parser.add_argument("output", type=Path)
    parser.add_argument("--name", default="f0")
    parser.add_argument("--base", type=lambda x: int(x, 0), default=0xFFFFC010)
    parser.add_argument("--lsn", type=lambda x: int(x, 0), default=0)
    parser.add_argument("--format", action="store_true",
                        help="Formatierung erlauben (FmtDsabl loeschen)")
    parser.add_argument("--hard-autosize", action="store_true",
                        help="Q9-CF: Hard-Disk-Typ und Geometrie per FAT-BPB")
    parser.add_argument("--image", type=Path,
                        help="Image-Datei: Geometrie aus ihrer 512-Byte-Sektorzahl ableiten")
    parser.add_argument("--driver", default="cfide",
                        help="Treibername im Descriptor (Default: cfide)")
    parser.add_argument("--geometry", metavar="CYL,HEADS,SPT",
                        help="feste PCF-Geometrie, z.B. 261,255,63")
    args = parser.parse_args()
    args.output.write_bytes(args.input.read_bytes())
    geometry = None
    if args.image and args.geometry:
        raise SystemExit("--image und --geometry schliessen sich aus")
    if args.image and args.hard_autosize:
        raise SystemExit("--image und --hard-autosize schliessen sich aus")
    if args.image:
        try:
            geometry = image_geometry(args.image)
        except (OSError, ValueError) as exc:
            raise SystemExit(f"--image: {exc}") from exc
    if args.geometry:
        try:
            geometry = tuple(int(part, 0) for part in args.geometry.split(","))
            if len(geometry) != 3 or not (0 < geometry[0] <= 0xFFFF and
                                          0 < geometry[1] <= 0xFF and
                                          0 < geometry[2] <= 0xFFFF):
                raise ValueError
        except ValueError as exc:
            raise SystemExit("--geometry erwartet CYL,HEADS,SPT") from exc
    patch(args.output, args.name, args.base, args.lsn, args.format, args.hard_autosize,
          args.driver, geometry)


if __name__ == "__main__":
    main()
