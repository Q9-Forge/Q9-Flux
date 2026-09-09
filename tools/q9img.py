#!/usr/bin/env python3
"""q9img -- Testabbilder je Sitzung trennen, Master bewusst aktuell halten.

HINTERGRUND (2026-09-09): mehrere Claude-Sitzungen arbeiteten parallel am
SELBEN `local_images/OS9SYS.hda`. Wer den Emulator direkt mit dem Master
als `--cf` startet, schreibt hinein -- das Abbild wandert unter den
anderen Sitzungen weg, `os9 gen -b=` bricht danach mit "is fragmented"
ab, und im schlimmsten Fall zeigt der Bootzeiger ins Leere (real
passiert: DD_BT auf eine LSN ohne Modul, DD_BSZ = 0 -> Master nicht mehr
bootfaehig, ohne dass es jemand merkt).

REGELN, die dieses Skript durchsetzt:
  * Der Master wird NIE direkt gebootet. Jede Sitzung bekommt ihr eigenes
    Arbeitsabbild unter local_images/work/<tag>/OS9SYS.hda (APFS-CoW-Klon,
    kostet anfangs praktisch keinen Platz).
  * Aus einem KAPUTTEN Master wird nicht geklont ("new" prueft ihn).
  * Zurueck zum Master geht es nur bewusst ueber "promote" -- mit
    Bootketten-Pruefung und automatischer, datierter Sicherung.

Befehle:
  q9img.py new <tag>        Arbeitsabbild fuer diese Sitzung anlegen
  q9img.py path <tag>       Pfad ausgeben (fuer --cf / Skripte)
  q9img.py list             vorhandene Arbeitsabbilder zeigen
  q9img.py check [<ziel>]   Bootkette pruefen (ohne Argument: Master)
  q9img.py promote <tag>    geprueftes Arbeitsabbild zum neuen Master machen
  q9img.py lock | unlock    Master schreibgeschuetzt setzen / freigeben

<ziel> ist ein Tag oder ein Pfad. Alle Pfade relativ zum Repo-Wurzel.
"""
import os
import shutil
import stat
import subprocess
import sys
import time

REPO = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
IMAGES = os.path.join(REPO, "local_images")
# Q9IMG_MASTER erlaubt ein anderes Goldstueck (z.B. hd1/hd2 oder zum
# Testen des Skripts selbst), ohne den Standardweg zu veraendern.
MASTER = os.environ.get("Q9IMG_MASTER") or os.path.join(IMAGES, "OS9SYS.hda")
WORK = os.path.join(IMAGES, "work")
BACKUPS = os.path.join(IMAGES, "archive_promote")


def die(msg):
    sys.exit("q9img: " + msg)


def clone(src, dst):
    """APFS-Copy-on-Write-Klon (kostet anfangs keinen Platz), sonst normale Kopie."""
    os.makedirs(os.path.dirname(dst), exist_ok=True)
    if subprocess.call(["cp", "-c", src, dst]) != 0:
        shutil.copy2(src, dst)


def resolve(target):
    """Tag oder Pfad -> Pfad."""
    if target is None:
        return MASTER
    if os.path.sep in target or target.endswith(".hda"):
        return target if os.path.isabs(target) else os.path.join(os.getcwd(), target)
    return os.path.join(WORK, target, "OS9SYS.hda")


def read_boot_info(img):
    """DD_BT/DD_BSZ aus dem Identification-Sektor lesen (s. Projektnotiz
    'Q9 Testimage-Bootkette': 0x15-0x17 = 3-Byte-LSN, 0x18-0x19 = Laenge,
    Sektorgroesse 512) und die Bootkette anlesen."""
    with open(img, "rb") as f:
        head = f.read(0x20)
        lsn = int.from_bytes(head[0x15:0x18], "big")
        size = int.from_bytes(head[0x18:0x1A], "big")
        f.seek(lsn * 512)
        first = f.read(16)
    return lsn, size, first


def check(img, quiet=False):
    """True, wenn die Bootkette plausibel ist. Prueft genau die zwei Dinge,
    die real kaputtgegangen sind: zeigt DD_BT auf einen Modulkopf, und ist
    DD_BSZ ungleich 0."""
    if not os.path.exists(img):
        if not quiet:
            print("FEHLT:  %s" % img)
        return False
    lsn, size, first = read_boot_info(img)
    ok_sync = first[:2] == b"\x4a\xfc"
    ok_size = size > 0
    if not quiet:
        print("Abbild:    %s" % img)
        print("  Boot-LSN %d (Offset 0x%X)" % (lsn, lsn * 512))
        print("  Laenge   %d  %s" % (size, "OK" if ok_size else "<-- 0, Bootloader laedt nichts!"))
        print("  Sync     %s  %s" % (first[:4].hex(), "OK (Modulkopf)" if ok_sync
                                     else "<-- kein 4afc, dort liegt kein Modul!"))
        print("  Ergebnis: %s" % ("bootfaehig" if (ok_sync and ok_size) else "NICHT bootfaehig"))
    return ok_sync and ok_size


def cmd_new(tag):
    dst = os.path.join(WORK, tag, "OS9SYS.hda")
    if os.path.exists(dst):
        die("existiert schon: %s\n  (bewusst kein Ueberschreiben -- erst loeschen oder anderen Tag waehlen)" % dst)
    if not check(MASTER, quiet=True):
        check(MASTER)
        die("Master ist nicht bootfaehig -- daraus wird nicht geklont.\n"
            "  Erst mit einem funktionierenden Abbild 'promote' machen.")
    clone(MASTER, dst)
    print("Angelegt: %s" % dst)
    print("Emulator:  ./build/macos/q9.exe --rom local_images/roms/romimage.dev.running.BIN \\\n"
          "             --cf %s" % os.path.relpath(dst, REPO))


def cmd_list():
    if not os.path.isdir(WORK):
        print("(noch keine Arbeitsabbilder)")
        return
    rows = []
    for tag in sorted(os.listdir(WORK)):
        img = os.path.join(WORK, tag, "OS9SYS.hda")
        if not os.path.exists(img):
            continue
        st = os.stat(img)
        real = subprocess.run(["du", "-sh", img], capture_output=True, text=True).stdout.split("\t")[0]
        rows.append((tag, time.strftime("%d.%m. %H:%M", time.localtime(st.st_mtime)), real.strip(),
                     "ok" if check(img, quiet=True) else "DEFEKT"))
    if not rows:
        print("(noch keine Arbeitsabbilder)")
        return
    print("%-24s %-14s %8s  %s" % ("Tag", "geaendert", "belegt", "Bootkette"))
    for r in rows:
        print("%-24s %-14s %8s  %s" % r)


def cmd_promote(tag):
    src = resolve(tag)
    if not check(src, quiet=True):
        check(src)
        die("Abbild ist nicht bootfaehig -- wird nicht zum Master gemacht.")
    os.makedirs(BACKUPS, exist_ok=True)
    stamp = time.strftime("%Y%m%d-%H%M%S")
    backup = os.path.join(BACKUPS, "OS9SYS.vor-promote-%s.hda" % stamp)
    if os.path.exists(MASTER):
        clone(MASTER, backup)
        print("Master gesichert nach: %s" % os.path.relpath(backup, REPO))
    was_locked = os.path.exists(MASTER) and not (os.stat(MASTER).st_mode & stat.S_IWUSR)
    if was_locked:
        os.chmod(MASTER, 0o644)
    if os.path.exists(MASTER):
        os.remove(MASTER)
    clone(src, MASTER)
    if was_locked:
        os.chmod(MASTER, 0o444)
    print("Neuer Master aus: %s" % os.path.relpath(src, REPO))
    check(MASTER)


def cmd_lock(on):
    if not os.path.exists(MASTER):
        die("Master fehlt: %s" % MASTER)
    os.chmod(MASTER, 0o444 if on else 0o644)
    print("Master ist jetzt %s." % ("schreibgeschuetzt (versehentliches --cf schlaegt fehl)"
                                    if on else "beschreibbar"))


def main():
    if len(sys.argv) < 2:
        sys.exit(__doc__)
    cmd, args = sys.argv[1], sys.argv[2:]
    if cmd == "new" and args:
        cmd_new(args[0])
    elif cmd == "path" and args:
        print(resolve(args[0]))
    elif cmd == "list":
        cmd_list()
    elif cmd == "check":
        ok = check(resolve(args[0] if args else None))
        sys.exit(0 if ok else 1)
    elif cmd == "promote" and args:
        cmd_promote(args[0])
    elif cmd == "lock":
        cmd_lock(True)
    elif cmd == "unlock":
        cmd_lock(False)
    else:
        sys.exit(__doc__)


if __name__ == "__main__":
    main()
