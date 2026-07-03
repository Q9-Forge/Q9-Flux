#═════════════════════════════════════════════════════════════════════════════════════════════════
# File:   06_test_fat16.py                                                                Ver. 1.00
# Owner:  AF
# Desc.:  FAT16-Test (Phase 3.3): baut von Hand ein minimales FAT16-Superfloppy-Image
#         (Boot-Sektor/BPB + 2 FAT-Kopien + Root-Directory + Datenregion) nach oeffentlich
#         dokumentiertem Layout, schreibt es als q9disk.img und laesst Q9 im Selbsttest daraus
#         lesen: eine 8.3-Datei, eine Datei mit langem Namen (LFN-Eintraege), eine Datei in einem
#         Unterverzeichnis, sowie I$Seek mitten in eine Datei. Reines Python-Stdlib, kein
#         externes Tool (kein hdiutil/newfs_msdos noetig, damit der Test ueberall laeuft) —
#         das Andreas-Interop-Szenario (am Mac befuelltes Image) wird durch das Layout simuliert,
#         nicht durch echtes Mounten.
#
# Call:   python test/06_test_fat16.py   (aus dem Projekt-Root, nach "make native")
#
# Edition History
#─────────┬──────┬─────────────────────────────────────────────────────────────────────────┬──────
# Date    │ Ver. │ Description                                                             │ By
#─────────┼──────┼─────────────────────────────────────────────────────────────────────────┼──────
# 26-07-04│ 1.00 │ Initiale Version                                                        │ CF
#═════════╧══════╧═════════════════════════════════════════════════════════════════════════╧══════
import os
import struct
import subprocess
import sys

EXE = os.path.join("build", "native", "q9.exe")
IMG = "q9disk.img"

SECTOR = 512
SEC_PER_CLUS = 1                       # 1 Sektor/Cluster -> einfache Geometrie fuer den Test
RESERVED_SECS = 1                      # nur der Boot-Sektor (der 3.1-Selbsttest sichert/restauriert
                                        # LBA 1 inzwischen selbst, siehe kernel.c)
NUM_FATS = 2
ROOT_ENT_CNT = 16                      # 16 * 32 = 512 Byte = 1 Sektor Root-Dir
FAT_SECTORS = 1                        # reicht fuer die paar Test-Cluster
TOTAL_CLUSTERS = 8
TOTAL_SECTORS = RESERVED_SECS + NUM_FATS * FAT_SECTORS + \
    (ROOT_ENT_CNT * 32 // SECTOR) + TOTAL_CLUSTERS * SEC_PER_CLUS


def make_boot_sector() -> bytes:
    b = bytearray(SECTOR)
    b[0:3] = b"\xEB\x3C\x90"                       # JMP + NOP (Dummy)
    b[3:11] = b"Q9TEST01"                          # OEM-Name
    struct.pack_into("<H", b, 0x0B, SECTOR)        # BytesPerSector
    b[0x0D] = SEC_PER_CLUS                         # SectorsPerCluster
    struct.pack_into("<H", b, 0x0E, RESERVED_SECS)  # ReservedSectors
    b[0x10] = NUM_FATS                             # NumFATs
    struct.pack_into("<H", b, 0x11, ROOT_ENT_CNT)  # RootEntryCount
    struct.pack_into("<H", b, 0x13, TOTAL_SECTORS)  # TotalSectors16
    b[0x15] = 0xF8                                  # Media (fixed disk)
    struct.pack_into("<H", b, 0x16, FAT_SECTORS)   # FATSize16
    struct.pack_into("<H", b, 0x18, 0)              # SectorsPerTrack (irrelevant, Superfloppy)
    struct.pack_into("<H", b, 0x1A, 0)              # NumHeads
    struct.pack_into("<I", b, 0x1C, 0)              # HiddenSectors (kein MBR -> 0)
    struct.pack_into("<I", b, 0x20, 0)              # TotalSectors32 (0, TotalSectors16 gilt)
    b[0x24] = 0x00                                  # DriveNumber
    b[0x25] = 0x00                                  # Reserved1
    b[0x26] = 0x29                                  # BootSig (erweiterte BPB vorhanden)
    struct.pack_into("<I", b, 0x27, 0x12345678)      # VolumeID
    b[0x2B:0x2B + 11] = b"Q9TESTVOL  "               # VolumeLabel
    b[0x36:0x36 + 8] = b"FAT16   "                   # FSType (informativ)
    struct.pack_into("<H", b, 0x1FE, 0xAA55)         # Boot-Signatur
    return bytes(b)


def make_83_name(name: str) -> bytes:
    """'HELLO.TXT' -> 11 Byte 8.3-Feld (space-padded, Grossbuchstaben)."""
    if "." in name:
        base, ext = name.split(".", 1)
    else:
        base, ext = name, ""
    base = base.upper().ljust(8)[:8]
    ext = ext.upper().ljust(3)[:3]
    return (base + ext).encode("ascii")


def make_dirent(name: str, attr: int, cluster: int, size: int) -> bytes:
    e = bytearray(32)
    e[0:11] = make_83_name(name)
    e[0x0B] = attr
    struct.pack_into("<H", e, 0x14, 0)               # FstClusHI (FAT16: immer 0)
    struct.pack_into("<H", e, 0x1A, cluster)         # FstClusLO
    struct.pack_into("<I", e, 0x1C, size)            # FileSize
    return bytes(e)


def lfn_checksum(name83_11: bytes) -> int:
    chk = 0
    for c in name83_11:
        chk = (((chk & 1) << 7) + (chk >> 1) + c) & 0xFF
    return chk


def make_lfn_entries(longname: str, alias83: bytes) -> list:
    """Baut die LFN-32-Byte-Eintraege fuer 'longname' (VOR dem 8.3-Eintrag, absteigende
    Sequenznummer -> letzter Namensteil zuerst im Directory, wie es echte FAT16-Treiber tun)."""
    chksum = lfn_checksum(alias83)
    chars = list(longname.encode("utf-16-le"))
    # in 13-Zeichen-Teile (26 Byte) aufteilen, letzter Teil mit 0x0000-Terminator + 0xFFFF-Padding
    units = [longname[i:i + 13] for i in range(0, len(longname), 13)] or [""]
    entries = []
    total = len(units)
    for i, part in enumerate(units):
        seq = i + 1
        u16 = [ord(c) for c in part]
        if len(u16) < 13:
            u16.append(0x0000)
        while len(u16) < 13:
            u16.append(0xFFFF)
        e = bytearray(32)
        e[0] = seq | (0x40 if i == total - 1 else 0x00)
        for j in range(5):
            struct.pack_into("<H", e, 1 + j * 2, u16[j])
        e[0x0B] = 0x0F                                  # ATTR_LFN
        e[0x0C] = 0
        e[0x0D] = chksum
        for j in range(6):
            struct.pack_into("<H", e, 0x0E + j * 2, u16[5 + j])
        struct.pack_into("<H", e, 0x1A, 0)
        for j in range(2):
            struct.pack_into("<H", e, 0x1C + j * 2, u16[11 + j])
        entries.append(bytes(e))
    entries.reverse()                                   # Directory-Reihenfolge: hoechste Seq zuerst
    return entries


def main() -> int:
    if not os.path.exists(EXE):
        print("06_test_fat16: FAIL (build/native/q9.exe fehlt - erst 'make native')")
        return 1

    # --- Layout planen -------------------------------------------------------------------------
    fat_start = RESERVED_SECS
    root_start = fat_start + NUM_FATS * FAT_SECTORS
    root_sectors = ROOT_ENT_CNT * 32 // SECTOR
    data_start = root_start + root_sectors            # LBA des Clusters 2

    hello_txt = b"Hallo Q9!!!9"                        # 13 Byte, Cluster 2 (Root)
    longname_txt = b"Langname!"                         # 9 Byte, Cluster 3 (Root, LFN-Datei)
    nested_txt = b"tief verschachtelt"                  # Cluster 5 (in SUBDIR, Cluster 4)

    # --- FAT aufbauen: Cluster 2,3,5 = EOF (einzelner Cluster), Cluster 4 (SUBDIR) = EOF --------
    fat = bytearray(FAT_SECTORS * SECTOR)
    struct.pack_into("<H", fat, 0 * 2, 0xFFF8)          # Cluster 0 (reserviert, Media-Byte-Echo)
    struct.pack_into("<H", fat, 1 * 2, 0xFFFF)          # Cluster 1 (reserviert)
    struct.pack_into("<H", fat, 2 * 2, 0xFFFF)          # Cluster 2 (HELLO.TXT) -> EOF
    struct.pack_into("<H", fat, 3 * 2, 0xFFFF)          # Cluster 3 (Langname-Datei) -> EOF
    struct.pack_into("<H", fat, 4 * 2, 0xFFFF)          # Cluster 4 (SUBDIR) -> EOF
    struct.pack_into("<H", fat, 5 * 2, 0xFFFF)          # Cluster 5 (NESTED.TXT) -> EOF

    # --- Root-Directory: HELLO.TXT (8.3), Langname-Datei (LFN+Alias), SUBDIR --------------------
    root = bytearray(root_sectors * SECTOR)
    pos = 0

    def put(entbytes: bytes):
        nonlocal pos
        root[pos:pos + len(entbytes)] = entbytes
        pos += len(entbytes)

    put(make_dirent("HELLO.TXT", 0x20, 2, len(hello_txt)))       # ATTR_ARCHIVE

    alias = make_83_name("LANGNA~1.TXT")
    for lfn_e in make_lfn_entries("This is a very long filename.txt", alias):
        put(lfn_e)
    alias_ent = bytearray(make_dirent("LANGNA~1.TXT", 0x20, 3, len(longname_txt)))
    put(bytes(alias_ent))

    put(make_dirent("SUBDIR", 0x10, 4, 0))                       # ATTR_DIRECTORY

    # --- SUBDIR-Cluster (Cluster 4): "." / ".." + NESTED.TXT ------------------------------------
    subdir = bytearray(SEC_PER_CLUS * SECTOR)
    spos = 0

    def sput(entbytes: bytes):
        nonlocal spos
        subdir[spos:spos + len(entbytes)] = entbytes
        spos += len(entbytes)

    sput(make_dirent(".", 0x10, 4, 0))
    sput(make_dirent("..", 0x10, 0, 0))
    sput(make_dirent("NESTED.TXT", 0x20, 5, len(nested_txt)))

    # --- Datencluster ----------------------------------------------------------------------------
    def clusbuf(data: bytes) -> bytes:
        buf = bytearray(SEC_PER_CLUS * SECTOR)
        buf[0:len(data)] = data
        return bytes(buf)

    with open(IMG, "wb") as f:
        f.write(make_boot_sector())
        f.write(bytes(fat))                              # FAT-Kopie 1
        f.write(bytes(fat))                              # FAT-Kopie 2
        f.write(bytes(root))                              # Root-Directory
        f.write(clusbuf(hello_txt))                        # Cluster 2
        f.write(clusbuf(longname_txt))                      # Cluster 3
        f.write(bytes(subdir))                              # Cluster 4 (SUBDIR)
        f.write(clusbuf(nested_txt))                        # Cluster 5
        # restliche Cluster (6,7) frei/ungenutzt
        f.write(bytes(SECTOR * (TOTAL_CLUSTERS - 4)))

    assert os.path.getsize(IMG) == TOTAL_SECTORS * SECTOR, \
        f"Image-Groesse {os.path.getsize(IMG)} != {TOTAL_SECTORS * SECTOR}"

    # --- Q9 starten und pruefen ------------------------------------------------------------------
    result = subprocess.run([EXE, "--selftest"], capture_output=True, text=True,
                            encoding="utf-8", errors="replace", timeout=15)

    checks = {
        "Exit-Code 0":                    result.returncode == 0,
        "FAT16: 8.3-Datei HELLO.TXT":     "[ok] FAT16: 8.3-Datei /d0/HELLO.TXT lesen (Inhalt+Groesse)" in result.stdout,
        "FAT16: LFN-Datei (langer Name)": "[ok] FAT16: LFN-Datei (langer Name) lesen" in result.stdout,
        "FAT16: Datei in Unterverzeichnis": "[ok] FAT16: Datei in Unterverzeichnis lesen" in result.stdout,
        "FAT16: I$Seek + I$Read":          "[ok] FAT16: I$Seek + I$Read ab Position 6" in result.stdout,
        "FAT16: unbekannte Datei -> E$PNNF": "[ok] FAT16: unbekannte Datei -> E$PNNF" in result.stdout,
        "SYSCALL TEST PASS":               "SYSCALL TEST PASS" in result.stdout,
    }

    for name, ok in checks.items():
        print(f"  [{'ok' if ok else 'FEHLER'}] {name}")

    if all(checks.values()):
        print("06_test_fat16: PASS")
        return 0
    print("06_test_fat16: FAIL")
    print(result.stdout)
    return 1


if __name__ == "__main__":
    sys.exit(main())

#─────────────────────────────────────────────────────────────────────────────────────────────────
# EOF 06_test_fat16.py                                                                    Ver. 1.00
#─────────────────────────────────────────────────────────────────────────────────────────────────
