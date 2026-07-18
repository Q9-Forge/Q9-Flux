#!/usr/bin/env python3
"""Build a clearly named OS-9 image with only the pks page-pause fix.

The input image is cloned with APFS ``cp -c``.  The input is never modified.
The output receives one change in the root ``netmods`` merge:
``pks`` module, module-relative offset ``0x4f`` (PD_PAU), ``01 -> 00``.
"""

from __future__ import annotations

import argparse
import subprocess
import tempfile
from pathlib import Path

from patch_pks_descriptor import find_pks, patch, verify_module


ROOT = Path(__file__).resolve().parents[1]
DEFAULT_BASE = ROOT / "local_images/OS9SYS.claudia-optionb-patchtest.hda"
DEFAULT_OUTPUT = ROOT / "local_images/OS9SYS.pks-nopause.hda"
DEFAULT_OS9 = Path("/Volumes/SSD1TB/projects/MWOS/tools/macos/bin/os9")


def run(*args: object) -> None:
    command = [str(arg) for arg in args]
    subprocess.run(command, check=True)


def image_path(image: Path, guest_path: str) -> str:
    return f"{image},{guest_path}"


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--base", type=Path, default=DEFAULT_BASE)
    parser.add_argument("--output", type=Path, default=DEFAULT_OUTPUT)
    parser.add_argument("--os9", type=Path, default=DEFAULT_OS9)
    args = parser.parse_args()

    if not args.base.is_file():
        raise SystemExit(f"base image not found: {args.base}")
    if args.output.exists():
        raise SystemExit(
            f"output already exists (refusing to overwrite): {args.output}\n"
            "choose another --output name or remove it explicitly"
        )
    args.output.parent.mkdir(parents=True, exist_ok=True)

    print(f"base (read-only): {args.base}")
    print(f"output clone:      {args.output}")
    print("target:            /dd/netmods -> module pks -> offset 0x4f (PD_PAU)")

    # APFS clone: this is intentionally not a normal destructive copy of the
    # user's source image.  On non-APFS systems cp -c fails loudly.
    run("cp", "-c", args.base, args.output)

    with tempfile.TemporaryDirectory(prefix="q9-pks-nopause-") as temp:
        temp_dir = Path(temp)
        # Keep the host basename "netmods" so ToolShed installs the patched
        # file under the correct guest pathname when copying to the root.
        patched = temp_dir / "netmods"
        run(args.os9, "copy", image_path(args.output, "netmods"), patched)
        start, size, old, new = patch(patched)

        # Replace the root file with a basename of exactly "netmods"; ToolShed
        # otherwise uses the temporary host filename as the guest filename.
        run(args.os9, "del", image_path(args.output, "netmods"))
        run(args.os9, "copy", patched, image_path(args.output, "."))
        run(args.os9, "attr", image_path(args.output, "netmods"), "-e", "-pe")

        verify = temp_dir / "netmods-verify"
        run(args.os9, "copy", image_path(args.output, "netmods"), verify)
        verify_data = verify.read_bytes()
        verify_start, verify_size = find_pks(verify_data)
        verify_module(verify_data, verify_start, verify_size)
        if verify_data[verify_start + 0x4F] != 0:
            raise RuntimeError("deployed pks PD_PAU is not zero")

    print(f"pks merge offset: 0x{start:x}")
    print(f"pks module size: {size} bytes")
    print(f"pks offset 0x4f (PD_PAU): 0x{old:02x} -> 0x{new:02x}")
    print(f"verified image: {args.output}")


if __name__ == "__main__":
    main()
