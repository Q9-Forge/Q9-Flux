#!/usr/bin/env python3
import os
import re
import shutil
import subprocess
from collections import defaultdict
from pathlib import Path

ROOT = Path("/Volumes/SSD1TB/projects/Q9")
OS9 = Path("/Volumes/SSD1TB/projects/MWOS/tools/macos/bin/os9")
OUT = ROOT / "local_images" / "Q9-cb030-work-max.hda"
LOG = ROOT / "local_images" / "Q9-cb030-work-max-build.log"
ROMRAM_SRC = ROOT / "local_images" / "Q9-cb030-work.hda"

MWOS = Path("/Volumes/SSD1TB/projects/MWOS/OS9")
PRO = Path("/Volumes/SSD1TB/OS-9_Professional_3.0")

MAX_LSN = "16777215"
CHUNK_SIZE = 24
OPTIONAL_MAX = 32768

ESSENTIAL_CMDS = [
    "shell", "mshell", "dir", "pd", "iniz", "devs", "free", "procs",
    "mdir", "mfree", "paths", "ident", "dump", "list", "attr", "copy",
    "del", "deldir", "makdir", "rename", "cmp", "date",
    "save", "fixmod",
]

COMPILER_CMDS = ["cc", "c68", "cpp", "o68", "r68", "l68", "make", "touch"]

DIRECT_DEFS = {
    "const.h", "ctype.h", "dir.h", "direct.h", "errno.h",
    "float.h", "funcs.h", "io.h", "limits.h",
    "math.h", "memory.h", "module.h", "modes.h", "path.h", "process.h",
    "procid.h", "rbf.h", "sbf.h", "scf.h", "setjmp.h",
    "sg_codes.h", "sgstat.h", "signal.h", "stdio.h", "strings.h",
    "time.h", "types.h",
}

DIRECT_LIB = {
    "clib.l", "cstart.r", "ansi_cstart.r", "sys.l", "sys_clib.l",
    "sys_csl.l", "csl.l", "os_lib.l", "os_csl.l", "math.l",
    "fpl.l", "fpio.l", "sclib.l", "termlib.l", "sbf.l",
    "curses.l", "gfx.l", "gfx2.l",
}

SKIP_NAMES = {".DS_Store", ".updated"}
SKIP_SUFFIXES = (".map", ".bak")


def run(args, check=True):
    p = subprocess.run([str(a) for a in args], text=True, capture_output=True)
    if check and p.returncode != 0:
        raise RuntimeError(f"command failed: {' '.join(map(str, args))}\n{p.stdout}{p.stderr}")
    return p


def os9_path(path):
    return f"{OUT},{path}" if path else f"{OUT},"


def ensure_dir(path, made):
    parts = [p for p in path.split("/") if p]
    cur = ""
    for part in parts:
        cur = part if not cur else f"{cur}/{part}"
        if cur not in made:
            run([OS9, "makdir", os9_path(cur)], check=False)
            made.add(cur)


def ident(path):
    p = run([OS9, "ident", path], check=False)
    text = p.stdout + p.stderr
    name = None
    edition = -1
    good_crc = "Good CRC" in text
    m = re.search(r"Header for :\s+([^\n\r]*)", text)
    if m:
        name = m.group(1).strip() or None
    m = re.search(r"Edition:\s+\$[0-9A-Fa-f]+\s+#(\d+)", text)
    if m:
        edition = int(m.group(1))
    return name, edition, good_crc, text


def add_candidate(cands, file, source):
    if file.name in SKIP_NAMES or file.name.endswith(SKIP_SUFFIXES):
        return
    if not file.is_file():
        return
    name, edition, good_crc, _ = ident(str(file))
    key = name or file.name
    cands[key].append({
        "file": file,
        "source": source,
        "edition": edition,
        "good_crc": good_crc,
        "basename": file.name,
    })


def choose_best(items):
    return sorted(
        items,
        key=lambda x: (x["edition"], x["good_crc"], source_rank(x["source"]), str(x["file"])),
        reverse=True,
    )[0]


def source_rank(source):
    if source.startswith("PRO30"):
        return 30
    if source.startswith("MWOS-CB030"):
        return 25
    if source.startswith("MWOS"):
        return 20
    return 0


def copy_file(src, dst_dir, made, executable=False, log=None):
    ensure_dir(dst_dir, made)
    target = os9_path(dst_dir)
    run([OS9, "copy", "-r", src, target])
    dst = f"{dst_dir}/{Path(str(src).split(',')[-1]).name}"
    if executable:
        run([OS9, "attr", "-q", os9_path(dst), "-e", "-r", "-w", "-pe", "-pr"], check=False)
    if log is not None:
        log.append(f"COPY {src} -> {dst}")


def copy_host_file(file, dst_dir, made, executable=False, log=None):
    copy_file(str(file), dst_dir, made, executable, log)


def copy_tree_flat(files, dst_base, made, executable=False, log=None, max_size=None):
    selected = []
    for file in sorted(files, key=lambda p: p.name.lower()):
        if max_size is not None and file.stat().st_size > max_size:
            if log is not None:
                log.append(f"SKIP optional-large {file} size={file.stat().st_size}")
            continue
        selected.append(file)
    for idx, file in enumerate(selected):
        chunk = idx // CHUNK_SIZE
        copy_host_file(file, f"{dst_base}/P{chunk:02d}", made, executable, log)


def list_image_dir(image, path):
    p = run([OS9, "dir", "-e", f"{image},{path}"], check=True)
    names = []
    for line in p.stdout.splitlines():
        fields = line.split()
        if len(fields) >= 7 and not fields[0].startswith("-") and fields[-1] != "Name":
            names.append(fields[-1])
    return names


def main():
    OUT.parent.mkdir(parents=True, exist_ok=True)
    if OUT.exists():
        backup = OUT.with_suffix(".prev.hda")
        if backup.exists():
            backup.unlink()
        OUT.rename(backup)

    log = []
    made = set()

    run([OS9, "format", "-q", "-k", "-nQ9WORK", "-l" + MAX_LSN, "-c64", OUT])
    log.append(f"FORMAT {OUT} sectors={MAX_LSN} bytes_per_sector=256 cluster=64")

    for d in ["CMDS", "CMDS/C", "CMDS/MORE", "CMDS/BOOTOBJS", "C", "DEFS", "LIB", "IO", "SYS", "DIST"]:
        ensure_dir(d, made)

    cmd_candidates = defaultdict(list)
    cmd_roots = [
        (MWOS / "68000" / "CMDS", "MWOS-68000-CMDS"),
        (MWOS / "68000" / "CMDS" / "NOCSL", "MWOS-68000-NOCSL"),
        (MWOS / "68020" / "CMDS", "MWOS-68020-CMDS"),
        (MWOS / "68030" / "PORTS" / "CB030" / "CMDS", "MWOS-CB030-CMDS"),
    ]
    for root in sorted(PRO.glob("#*/extracted/CMDS")) + sorted(PRO.glob("#*/extracted/CMDS_NEW")):
        cmd_roots.append((root, "PRO30-" + root.parts[-3].split("_")[0].strip("#")))
    for root, source in cmd_roots:
        if root.is_dir():
            for file in root.iterdir():
                add_candidate(cmd_candidates, file, source)

    chosen_cmds = {name: choose_best(items) for name, items in cmd_candidates.items()}
    for name in sorted(chosen_cmds):
        best = chosen_cmds[name]
        alts = sorted(best["edition"] for best in cmd_candidates[name])
        log.append(
            f"CMD {name}: ed={best['edition']} source={best['source']} file={best['file']} all_editions={alts}"
        )

    used_cmds = set()
    for name in ESSENTIAL_CMDS:
        if name in chosen_cmds:
            copy_host_file(chosen_cmds[name]["file"], "CMDS", made, True, log)
            used_cmds.add(name)
    for name in COMPILER_CMDS:
        if name in chosen_cmds:
            copy_host_file(chosen_cmds[name]["file"], "CMDS/C", made, True, log)
            used_cmds.add(name)

    more_cmds = [v["file"] for k, v in chosen_cmds.items() if k not in used_cmds]
    copy_tree_flat(more_cmds, "CMDS/MORE", made, True, log, OPTIONAL_MAX)

    cb030_boot = MWOS / "68030" / "PORTS" / "CB030" / "CMDS" / "BOOTOBJS"
    for file in sorted(cb030_boot.iterdir(), key=lambda p: p.name.lower()):
        if file.is_file() and file.name not in SKIP_NAMES and not file.name.endswith(SKIP_SUFFIXES):
            copy_host_file(file, "CMDS/BOOTOBJS", made, False, log)
    if (cb030_boot / "BOOTFILES").is_dir():
        for file in sorted((cb030_boot / "BOOTFILES").iterdir(), key=lambda p: p.name.lower()):
            if file.is_file() and file.name not in SKIP_NAMES and file.stat().st_size <= OPTIONAL_MAX:
                copy_host_file(file, "CMDS/BOOTOBJS/BOOTFILES", made, False, log)

    rom_names = list_image_dir(ROMRAM_SRC, "CMDS/BOOTOBJS/ROMRAM")
    for idx, name in enumerate(rom_names):
        chunk = idx // CHUNK_SIZE
        src = f"{ROMRAM_SRC},CMDS/BOOTOBJS/ROMRAM/{name}"
        copy_file(src, f"CMDS/BOOTOBJS/ROMRAM/R{chunk:02d}", made, False, log)

    boot_candidates = []
    for root in [MWOS / "68000" / "CMDS" / "BOOTOBJS", MWOS / "68020" / "CMDS" / "BOOTOBJS"]:
        if root.is_dir():
            boot_candidates.extend([p for p in root.iterdir() if p.is_file() and p.name not in SKIP_NAMES])
    for root in sorted(PRO.glob("#*/extracted/CMDS/BOOTOBJS")):
        boot_candidates.extend([p for p in root.iterdir() if p.is_file() and p.name not in SKIP_NAMES])
    copy_tree_flat(boot_candidates, "CMDS/BOOTOBJS/MORE", made, False, log, OPTIONAL_MAX)

    c_root = PRO / "#123_OS9_Professional_V3.0_3of6" / "extracted" / "C"
    if c_root.is_dir():
        for file in sorted(c_root.rglob("*"), key=lambda p: str(p).lower()):
            if file.is_file():
                rel = file.relative_to(c_root).parent
                copy_host_file(file, "C" if str(rel) == "." else "C/" + str(rel), made, False, log)

    def copy_selected(files, direct_names, direct_dir, more_dir):
        more = []
        selected = {}
        for file in files:
            if file.name in SKIP_NAMES:
                continue
            prev = selected.get(file.name)
            if prev is None or source_rank(str(file)) > source_rank(str(prev)):
                selected[file.name] = file
        for name, file in sorted(selected.items()):
            if name in direct_names:
                copy_host_file(file, direct_dir, made, False, log)
            else:
                more.append(file)
        copy_tree_flat(more, more_dir, made, False, log, OPTIONAL_MAX)

    defs_files = []
    for root in [MWOS / "68000" / "DEFS", PRO / "#124_OS9_Professional_V3.0_4of6" / "extracted" / "DEFS"]:
        if root.is_dir():
            defs_files.extend([p for p in root.iterdir() if p.is_file()])
    copy_selected(defs_files, DIRECT_DEFS, "DEFS", "DEFS/MORE")

    lib_files = []
    for root in [MWOS / "68000" / "LIB", PRO / "#125_OS9_Professional_V3.0_5of6" / "extracted" / "LIB"]:
        if root.is_dir():
            lib_files.extend([p for p in root.iterdir() if p.is_file()])
    copy_selected(lib_files, DIRECT_LIB, "LIB", "LIB/MORE")

    for disk in sorted(PRO.glob("#*/extracted/SYS")):
        if disk.is_dir():
            for file in sorted(disk.iterdir(), key=lambda p: p.name.lower()):
                if file.is_file():
                    copy_host_file(file, "SYS", made, False, log)

    io_root = PRO / "#125_OS9_Professional_V3.0_5of6" / "extracted" / "IO"
    if io_root.is_dir():
        for file in sorted(io_root.rglob("*"), key=lambda p: str(p).lower()):
            if file.is_file():
                rel = file.relative_to(io_root).parent
                copy_host_file(file, "IO" if str(rel) == "." else "IO/" + str(rel), made, False, log)

    run([OS9, "dcheck", OUT])
    LOG.write_text("\n".join(log) + "\n")
    print(f"built {OUT}")
    print(f"log {LOG}")


if __name__ == "__main__":
    main()
