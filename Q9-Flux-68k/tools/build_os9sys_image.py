#!/usr/bin/env python3
"""Build the Q9 CB030 OS9SYS work image.

The toolshed rewrite path has shown allocation-map leaks when existing files
inside an RBF image are replaced. This builder therefore selects the winning
source for every path first, writes a fresh image, and copies every file once.
"""

from __future__ import annotations

import os
import re
import subprocess
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
OS9 = Path("/Volumes/SSD1TB/projects/MWOS/tools/macos/bin/os9")
IMG = ROOT / "local_images" / "OS9SYS.hda"
ROM = ROOT / "local_images" / "OS9SYS.before-rom-resave.hda"
PROF_ROOT = Path("/Volumes/SSD1TB/OS-9_Professional_3.0")
SDK_CMDS = Path("/Volumes/SSD1TB/projects/MWOS/OS9/68000/CMDS")
SDK_BOOT = Path("/Volumes/SSD1TB/projects/MWOS/OS9/68030/PORTS/CB030/CMDS/BOOTOBJS")
SDK_BOOT_68000 = Path("/Volumes/SSD1TB/projects/MWOS/OS9/68000/CMDS/BOOTOBJS")
SDK_BOOT_68020 = Path("/Volumes/SSD1TB/projects/MWOS/OS9/68020/CMDS/BOOTOBJS")
SDK_CMDS_68020 = Path("/Volumes/SSD1TB/projects/MWOS/OS9/68020/CMDS")
TMP = Path("/private/tmp/q9_os9sys_build")
LOG = Path("/private/tmp/q9_os9sys_build.log")

IMAGE_SECTORS = "8388607"
IMAGE_BYTES = "4294966784"

BOOT_ENTRIES = [
    "CMDS/BOOTOBJS/dker030s",
    "CMDS/BOOTOBJS/ioman_DEV",
    "CMDS/BOOTOBJS/init_disk",
    "CMDS/BOOTOBJS/cache030",
    "CMDS/BOOTOBJS/ssm851",
    "CMDS/BOOTOBJS/fpu",
    "CMDS/BOOTOBJS/tkcb030",
    "CMDS/BOOTOBJS/rtccb030",
    "CMDS/BOOTOBJS/scf",
    "CMDS/BOOTOBJS/null",
    "CMDS/BOOTOBJS/nil",
    "CMDS/BOOTOBJS/pipeman",
    "CMDS/BOOTOBJS/pipe",
    "CMDS/BOOTOBJS/sc68681",
    "CMDS/BOOTOBJS/term",
    "CMDS/BOOTOBJS/t1",
    "CMDS/BOOTOBJS/rbf",
    "CMDS/BOOTOBJS/cfide",
    "CMDS/BOOTOBJS/dd",
    "CMDS/BOOTOBJS/c0",
    "CMDS/BOOTOBJS/c0_fmt",
    "CMDS/BOOTOBJS/sysgo_smart",
    "CMDS/mshell",
    "CMDS/csl",
    "CMDS/cio",
    "CMDS/attr",
    "CMDS/dcheck",
    "CMDS/format",
    "CMDS/kermit",
    "CMDS/makdir",
    "CMDS/os9gen",
    "CMDS/pd",
    "CMDS/tmode",
]

BOOT_MODULES = [
    "dker030s",
    "ioman_DEV",
    "init_disk",
    "cache030",
    "ssm851",
    "fpu",
    "tkcb030",
    "rtccb030",
    "scf",
    "null",
    "nil",
    "pipeman",
    "pipe",
    "sc68681",
    "term",
    "t1",
    "rbf",
    "cfide",
    "dd",
    "c0",
    "c0_fmt",
    "sysgo_smart",
    "mshell",
    "csl",
    "cio",
    "attr",
    "dcheck",
    "format",
    "kermit",
    "makdir",
    "os9gen",
    "pd",
    "tmode",
]


def run(args: list[str | Path], check: bool = False) -> tuple[str, int]:
    proc = subprocess.run(
        [str(a) for a in args],
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
    )
    if check and proc.returncode != 0:
        raise RuntimeError(f"command failed {proc.returncode}: {args}\n{proc.stdout}")
    return proc.stdout, proc.returncode


def os9spec(image: Path | str, rel: str = "") -> str:
    return f"{image}," if not rel else f"{image},{rel}"


def fstat(spec: str) -> str:
    return run([OS9, "fstat", spec])[0]


def exists(spec: str) -> bool:
    return fstat(spec).startswith("File Information for ")


def is_dir(spec: str) -> bool:
    return re.search(r"Attributes\s*: d", fstat(spec)) is not None


def list_dir(spec: str) -> list[str]:
    out, _ = run([OS9, "dir", "-a", spec])
    names: list[str] = []
    for line in out.splitlines()[2:]:
        for name in line.split():
            if name not in (".", ".."):
                names.append(name)
    return names


def edition(spec: str) -> int | None:
    out, rc = run([OS9, "ident", spec])
    if rc != 0 or "Good CRC" not in out or "Good parity" not in out:
        return None
    match = re.search(r"Edition:\s+\$[0-9A-Fa-f]+\s+#(\d+)", out)
    return int(match.group(1)) if match else None


def parent(rel: str) -> str:
    return rel.rsplit("/", 1)[0] if "/" in rel else ""


def create_overrides(tmp: Path) -> dict[str, str]:
    tmp.mkdir(parents=True, exist_ok=True)

    def write_os9_text(path: Path, text: str, encoding: str = "ascii") -> None:
        path.write_text(text.replace("\n", "\r"), encoding=encoding)

    startup = tmp / "startup"
    write_os9_text(
        startup,
        """* Q9 CB030 startup file
* CompactFlash image: OS9SYS

setenv TERM q9term

iniz /dd
chd /dd
chx /dd/CMDS

link mshell
link csl cio

* RAM disk is useful, but non-fatal if already initialized.
load -d /dd/CMDS/BOOTOBJS/r0
iniz /r0

echo "===================================================================="
echo " Q9 CB030 OS-9/68K Arbeitsimage"
echo " TERM=q9term  CMDS=/dd/CMDS  Editor=umacs"
echo "===================================================================="
date -m
""",
    )

    login = tmp / ".login"
    write_os9_text(login, "setenv TERM q9term\n")

    bootlist = tmp / "bootlist.cb030"
    bootlist.write_text("* Q9 CB030 diskboot bootlist\n" + "\n".join(BOOT_ENTRIES) + "\n", encoding="ascii")

    termcap_src = tmp / "termcap.src"
    run([OS9, "copy", os9spec(IMG, "SYS/termcap"), termcap_src])
    base_termcap = termcap_src.read_text(encoding="latin-1") if termcap_src.exists() else ""
    q9_entry = r"""#
# Q9 CB030 host terminal (ANSI/xterm-style)
#
q9|q9term|Q9 CB030 host terminal:\
        :co#80:li#24:am:bs:pt:\
        :cl=\E[H\E[2J:cm=\E[%i%d;%dH:ho=\E[H:\
        :ce=\E[K:cd=\E[J:nd=\E[C:up=\E[A:\
        :so=\E[7m:se=\E[m:us=\E[4m:ue=\E[m:me=\E[m:\
        :ku=\E[A:kd=\E[B:kr=\E[C:kl=\E[D:kh=\E[H:\
        :k1=\EOP:k2=\EOQ:k3=\EOR:k4=\EOS:kb=^H:\
        :al=\E[L:dl=\E[M:ic=\E[@:dc=\E[P:sr=\EM:sf=\n:

"""
    termcap = tmp / "termcap"
    if "q9|q9term|" in base_termcap:
        write_os9_text(termcap, base_termcap, encoding="latin-1")
    else:
        write_os9_text(termcap, q9_entry + base_termcap, encoding="latin-1")

    return {
        "startup": str(startup),
        ".login": str(login),
        "SYS/startup": str(startup),
        "SYS/termcap": str(termcap),
        "CMDS/BOOTOBJS/bootlist.cb030": str(bootlist),
    }


def main() -> int:
    TMP.mkdir(parents=True, exist_ok=True)
    LOG.write_text("", encoding="utf-8")

    stats = {
        "prof_files_seen": 0,
        "prof_dirs_seen": 0,
        "rom_selected": 0,
        "sdk_selected": 0,
        "sdk_nonmodules_skipped": 0,
        "replaced_by_rule": 0,
        "skipped_prof_boot": 0,
        "errors": 0,
        "copied": 0,
        "made_dirs": 0,
    }
    candidates: dict[str, dict[str, object]] = {}
    dirs: set[str] = set()

    def log(line: str) -> None:
        with LOG.open("a", encoding="utf-8") as fh:
            print(line, file=fh)

    def add_parent_dirs(rel: str) -> None:
        rel_parent = parent(rel)
        if not rel_parent:
            return
        acc: list[str] = []
        for part in rel_parent.split("/"):
            acc.append(part)
            dirs.add("/".join(acc))

    def select(rel: str, src: str, origin: str, policy: str) -> bool:
        ed = edition(src)
        old = candidates.get(rel)
        if policy == "force":
            if old:
                stats["replaced_by_rule"] += 1
            candidates[rel] = {"src": src, "origin": origin, "ed": ed}
            add_parent_dirs(rel)
            return True
        if policy == "module_newer":
            if ed is None:
                return False
            if not old:
                candidates[rel] = {"src": src, "origin": origin, "ed": ed}
                add_parent_dirs(rel)
                return True
            old_ed = old.get("ed")
            if old_ed is not None and ed > int(old_ed):
                stats["replaced_by_rule"] += 1
                candidates[rel] = {"src": src, "origin": origin, "ed": ed}
                add_parent_dirs(rel)
                return True
            return False
        if policy == "prof":
            if rel == "OS9Boot":
                stats["skipped_prof_boot"] += 1
                return False
            if not old:
                candidates[rel] = {"src": src, "origin": origin, "ed": ed}
                add_parent_dirs(rel)
                return True
            old_ed = old.get("ed")
            if ed is not None and old_ed is not None:
                if ed > int(old_ed):
                    stats["replaced_by_rule"] += 1
                    candidates[rel] = {"src": src, "origin": origin, "ed": ed}
                    add_parent_dirs(rel)
                    return True
                return False
            stats["replaced_by_rule"] += 1
            candidates[rel] = {"src": src, "origin": origin, "ed": ed}
            add_parent_dirs(rel)
            return True
        raise ValueError(policy)

    def walk_prof(image: Path, dirrel: str = "") -> None:
        for name in list_dir(os9spec(image, dirrel)):
            rel = name if not dirrel else f"{dirrel}/{name}"
            src = os9spec(image, rel)
            if is_dir(src):
                dirs.add(rel)
                stats["prof_dirs_seen"] += 1
                walk_prof(image, rel)
            else:
                stats["prof_files_seen"] += 1
                select(rel, src, "prof", "prof")

    prof_images = sorted(
        p for p in PROF_ROOT.glob("*/images/*.img") if re.search(r"#12[1-5]_.*\.img$", str(p))
    )
    for image in prof_images:
        log(f"SCAN PROF {image}")
        walk_prof(image)

    for name in list_dir(os9spec(ROM, "CMDS")):
        if name != "BOOTOBJS" and select(f"CMDS/{name}", os9spec(ROM, f"CMDS/{name}"), "rom-cmd", "module_newer"):
            stats["rom_selected"] += 1

    for name in list_dir(os9spec(ROM, "CMDS/BOOTOBJS")):
        select(f"CMDS/BOOTOBJS/{name}", os9spec(ROM, f"CMDS/BOOTOBJS/{name}"), "rom-boot", "force")
        stats["rom_selected"] += 1

    for src in sorted(SDK_BOOT.iterdir()):
        if not src.is_file():
            continue
        name = src.name
        if (
            name in (".DS_Store", ".updated")
            or name.endswith((".bak", ".map", ".BIN"))
            or name.startswith("romimage")
            or name == "romboot"
        ):
            continue
        policy = "force" if name == "cfide" else "module_newer"
        if select(f"CMDS/BOOTOBJS/{name}", str(src), "sdk-boot", policy):
            stats["sdk_selected"] += 1

    for src in sorted(SDK_BOOT_68020.iterdir()):
        if not src.is_file() or src.name == ".DS_Store":
            continue
        if select(f"CMDS/BOOTOBJS/{src.name}", str(src), "sdk-68020-boot", "module_newer"):
            stats["sdk_selected"] += 1

    for src in sorted(SDK_BOOT_68000.iterdir()):
        if not src.is_file() or src.name == ".DS_Store":
            continue
        policy = "force" if src.name in {"sysgo", "ioman_DEV"} else "module_newer"
        if select(f"CMDS/BOOTOBJS/{src.name}", str(src), "sdk-68000-boot", policy):
            stats["sdk_selected"] += 1

    for src in sorted(SDK_CMDS.iterdir()):
        if not src.is_file() or src.name == ".DS_Store":
            continue
        if edition(str(src)) is None:
            stats["sdk_nonmodules_skipped"] += 1
            continue
        if select(f"CMDS/{src.name}", str(src), "sdk-cmd", "module_newer"):
            stats["sdk_selected"] += 1

    for name in ("csl", "cio"):
        src = SDK_CMDS_68020 / name
        if src.is_file() and select(f"CMDS/{name}", str(src), "sdk-68020-cmd", "module_newer"):
            stats["sdk_selected"] += 1

    for rel, src in create_overrides(TMP).items():
        select(rel, src, "q9-override", "force")

    IMG.unlink(missing_ok=True)
    run([OS9, "format", "-q", "-k", "-nOS9SYS", "-bs512", f"-l{IMAGE_SECTORS}", "-c32", IMG], check=True)
    run(["truncate", "-s", IMAGE_BYTES, IMG], check=True)

    for rel in sorted(dirs, key=lambda d: (d.count("/"), d)):
        out, rc = run([OS9, "makdir", os9spec(IMG, rel)])
        if rc == 0 or exists(os9spec(IMG, rel)):
            stats["made_dirs"] += 1
        else:
            stats["errors"] += 1
            log(f"ERROR MAKDIR {rel}: {out.strip()}")

    for rel in sorted(candidates):
        src = str(candidates[rel]["src"])
        out, rc = run([OS9, "copy", src, os9spec(IMG, parent(rel))])
        if rc == 0:
            stats["copied"] += 1
        else:
            stats["errors"] += 1
            log(f"ERROR COPY {rel}: {src} -> {os9spec(IMG, parent(rel))}: {out.strip()}")

    boot_data = bytearray()
    for idx, rel in enumerate(BOOT_ENTRIES, 1):
        tmp_mod = TMP / f"mod_{idx:02d}_{rel.replace('/', '_')}"
        out, rc = run([OS9, "copy", os9spec(IMG, rel), tmp_mod])
        if rc != 0:
            stats["errors"] += 1
            log(f"ERROR EXTRACT BOOT MODULE {rel}: {out.strip()}")
            continue
        boot_data.extend(tmp_mod.read_bytes())
        tmp_mod.unlink(missing_ok=True)

    bootfile = TMP / "OS9Boot"
    bootfile.unlink(missing_ok=True)
    bootfile.write_bytes(boot_data)
    gen_out, gen_rc = run([OS9, "gen", f"-b={bootfile}", IMG])
    log("GEN OUTPUT:\n" + gen_out)
    if gen_rc != 0:
        stats["errors"] += 1

    run([OS9, "attr", os9spec(IMG, "startup"), "-e", "-pe"])

    cfide_out, _ = run([OS9, "ident", os9spec(IMG, "CMDS/BOOTOBJS/cfide")])
    boot_out, _ = run([OS9, "ident", os9spec(IMG, "OS9Boot")])
    dcheck_out, _ = run([OS9, "dcheck", IMG])
    stat_out, _ = run(["stat", "-f", "size=%z bytes blocks=%b blocksize=%k", IMG])

    summary = {
        **stats,
        "selected_files": len(candidates),
        "dirs": len(dirs),
        "bootfile_bytes": len(boot_data),
        "gen_rc": gen_rc,
        "log": str(LOG),
    }
    print(" ".join(f"{key}={value}" for key, value in summary.items()))
    print(
        "cfide:",
        " ".join(
            line.strip()
            for line in cfide_out.splitlines()
            if "Module CRC" in line or "Header Parity" in line or "Edition:" in line
        ),
    )
    print(
        "bootfile:",
        " ".join(
            line.strip()
            for line in boot_out.splitlines()[:24]
            if "Header for" in line or "Good CRC" in line or "Bad CRC" in line or "Edition:" in line
        ),
    )
    print("\n".join(dcheck_out.splitlines()[-10:]))
    print(stat_out.strip())
    return 1 if stats["errors"] or gen_rc != 0 else 0


if __name__ == "__main__":
    raise SystemExit(main())
