#!/usr/bin/env python3
# ═════════════════════════════════════════════════════════════════════════════════════════════════
# File:   make_fat_image.py                                                                Ver. 1.00
# Owner:  AF
# Desc.:  5.19: Erzeugt ein FAT12- oder FAT16-Superfloppy-Image (PCF-Typ) fuer den Q9-Emulator.
#         512 Byte/Sektor (die Buffergroesse, die aktuell funktioniert, = Q9_CB030_CF_SECTOR_SIZE).
#         FAT12 vs. FAT16 ergibt sich automatisch aus der Cluster-Anzahl (< 4085 -> FAT12).
#         Legt optional ein paar Testdateien + ein Unterverzeichnis an, damit dir/read im
#         Emulator etwas zu sehen haben.
#
#         Kein Fremdmodul — reine Standardbibliothek, Layout nach der oeffentlich dokumentierten
#         FAT-Spezifikation (Microsoft "FAT: General Overview of On-Disk Format"), analog zu
#         test/06_test_fat16.py.
#
# Call:   tools/make_fat_image.py <out.img> [--mb N] [--spc N] [--label NAME]
#
# Edition History
# ─────────┬──────┬─────────────────────────────────────────────────────────────────────────┬──────
# 26-07-16│ 1.00 │ Initiale Version (FAT12/16-Superfloppy, 512 B/Sektor)                    │ CF
# ═════════╧══════╧═════════════════════════════════════════════════════════════════════════╧══════
import argparse
import struct
import sys

SECTOR = 512                             # Buffergroesse, die aktuell funktioniert (CF-Sektor)
RESERVED_SECS = 1                        # nur der Boot-Sektor
NUM_FATS = 2
ROOT_ENT_CNT = 512                       # 512 * 32 = 16 KiB = 32 Sektoren Root-Directory


def make_83_name(name):
    if "." in name:
        base, ext = name.split(".", 1)
    else:
        base, ext = name, ""
    base = base.upper().ljust(8)[:8]
    ext = ext.upper().ljust(3)[:3]
    return (base + ext).encode("ascii")


def make_dirent(name, attr, cluster, size):
    e = bytearray(32)
    e[0:11] = make_83_name(name)
    e[0x0B] = attr
    struct.pack_into("<H", e, 0x14, (cluster >> 16) & 0xFFFF)   # FstClusHI (FAT16: 0)
    struct.pack_into("<H", e, 0x1A, cluster & 0xFFFF)           # FstClusLO
    struct.pack_into("<I", e, 0x1C, size)                       # FileSize
    # simples festes Anlegedatum (16.07.2026 12:00), damit kein Nullfeld
    struct.pack_into("<H", e, 0x18, ((2026 - 1980) << 9) | (7 << 5) | 16)
    struct.pack_into("<H", e, 0x16, (12 << 11))
    return bytes(e)


def make_boot_sector(total_sectors, sec_per_clus, fat_sectors, is_fat16, label):
    b = bytearray(SECTOR)
    b[0:3] = b"\xEB\x3C\x90"                        # JMP + NOP
    b[3:11] = b"Q9MKFAT0"                           # OEM-Name
    struct.pack_into("<H", b, 0x0B, SECTOR)         # BytesPerSector
    b[0x0D] = sec_per_clus                          # SectorsPerCluster
    struct.pack_into("<H", b, 0x0E, RESERVED_SECS)  # ReservedSectors
    b[0x10] = NUM_FATS                              # NumFATs
    struct.pack_into("<H", b, 0x11, ROOT_ENT_CNT)   # RootEntryCount
    if total_sectors < 0x10000:
        struct.pack_into("<H", b, 0x13, total_sectors)   # TotalSectors16
        struct.pack_into("<I", b, 0x20, 0)
    else:
        struct.pack_into("<H", b, 0x13, 0)
        struct.pack_into("<I", b, 0x20, total_sectors)   # TotalSectors32
    b[0x15] = 0xF8                                  # Media (fixed disk)
    struct.pack_into("<H", b, 0x16, fat_sectors)    # FATSize16
    struct.pack_into("<H", b, 0x18, 63)             # SectorsPerTrack (informativ)
    struct.pack_into("<H", b, 0x1A, 255)            # NumHeads (informativ)
    struct.pack_into("<I", b, 0x1C, 0)              # HiddenSectors
    b[0x24] = 0x80                                  # DriveNumber (Festplatte)
    b[0x26] = 0x29                                  # BootSig (erweiterte BPB)
    struct.pack_into("<I", b, 0x27, 0x51394642)     # VolumeID
    b[0x2B:0x2B + 11] = label.upper().ljust(11)[:11].encode("ascii")
    b[0x36:0x36 + 8] = (b"FAT16   " if is_fat16 else b"FAT12   ")
    struct.pack_into("<H", b, 0x1FE, 0xAA55)        # Boot-Signatur
    return bytes(b)


def fat_set(fat, is_fat16, idx, val):
    """Schreibt einen FAT12- oder FAT16-Eintrag."""
    if is_fat16:
        struct.pack_into("<H", fat, idx * 2, val & 0xFFFF)
    else:
        off = idx + (idx >> 1)                      # idx * 1.5
        if idx & 1:
            cur = fat[off] | (fat[off + 1] << 8)
            cur = (cur & 0x000F) | ((val & 0xFFF) << 4)
        else:
            cur = fat[off] | (fat[off + 1] << 8)
            cur = (cur & 0xF000) | (val & 0xFFF)
        fat[off] = cur & 0xFF
        fat[off + 1] = (cur >> 8) & 0xFF


def build(out_path, size_mb, sec_per_clus, label, start_sector=0):
    payload_sectors = (size_mb * 1024 * 1024) // SECTOR
    total_sectors = payload_sectors
    root_sectors = ROOT_ENT_CNT * 32 // SECTOR

    # FAT-Groesse iterativ bestimmen: Datenbereich haengt von FAT-Groesse ab (Henne/Ei).
    fat_sectors = 1
    for _ in range(64):
        data_sectors = total_sectors - RESERVED_SECS - NUM_FATS * fat_sectors - root_sectors
        clusters = data_sectors // sec_per_clus
        is_fat16 = clusters >= 4085
        bits = 16 if is_fat16 else 12
        need_bytes = (clusters + 2) * bits // 8 + 1
        need_sectors = (need_bytes + SECTOR - 1) // SECTOR
        if need_sectors <= fat_sectors:
            break
        fat_sectors = need_sectors

    data_sectors = total_sectors - RESERVED_SECS - NUM_FATS * fat_sectors - root_sectors
    clusters = data_sectors // sec_per_clus
    is_fat16 = clusters >= 4085

    fat = bytearray(fat_sectors * SECTOR)
    fat_set(fat, is_fat16, 0, 0xFF8)                # Media-Byte-Echo
    fat_set(fat, is_fat16, 1, 0xFFF if not is_fat16 else 0xFFFF)

    # Testinhalt: README.TXT (Cluster 2), Q9DIR/ (Cluster 3), Q9DIR/INFO.TXT (Cluster 4)
    readme = ("Q9 FAT%d-Testimage\r\n"
              "512 Bytes/Sektor, %d Sektoren/Cluster.\r\n"
              "Erzeugt von tools/make_fat_image.py.\r\n" % (16 if is_fat16 else 12, sec_per_clus))
    readme_b = readme.encode("ascii")
    info_b = b"Datei im Unterverzeichnis Q9DIR.\r\n"

    fat_set(fat, is_fat16, 2, 0xFFF if not is_fat16 else 0xFFFF)   # README.TXT
    fat_set(fat, is_fat16, 3, 0xFFF if not is_fat16 else 0xFFFF)   # Q9DIR
    fat_set(fat, is_fat16, 4, 0xFFF if not is_fat16 else 0xFFFF)   # INFO.TXT

    root = bytearray(root_sectors * SECTOR)
    root[0:32] = make_dirent("README.TXT", 0x20, 2, len(readme_b))
    root[32:64] = make_dirent("Q9DIR", 0x10, 3, 0)

    clus_bytes = sec_per_clus * SECTOR

    def clusbuf(data):
        buf = bytearray(clus_bytes)
        buf[0:len(data)] = data
        return bytes(buf)

    # Q9DIR-Cluster: "." / ".." + INFO.TXT
    subdir = bytearray(clus_bytes)
    subdir[0:32] = make_dirent(".", 0x10, 3, 0)
    subdir[32:64] = make_dirent("..", 0x10, 0, 0)
    subdir[64:96] = make_dirent("INFO.TXT", 0x20, 4, len(info_b))

    # FAT16-Obergrenze (65524 Cluster) pruefen — sonst waere das Image kein gueltiges FAT16.
    if is_fat16 and clusters > 65524:
        raise SystemExit("FAT16-Grenze ueberschritten (%d > 65524 Cluster) — --spc erhoehen "
                         "oder --mb verkleinern" % clusters)

    with open(out_path, "wb") as f:
        if start_sector:
            f.seek(start_sector * SECTOR)
        f.write(make_boot_sector(total_sectors, sec_per_clus, fat_sectors, is_fat16, label))
        for _ in range(NUM_FATS):
            f.write(bytes(fat))
        f.write(bytes(root))
        f.write(clusbuf(readme_b))                  # Cluster 2
        f.write(clusbuf(subdir))                    # Cluster 3
        f.write(clusbuf(info_b))                    # Cluster 4
        # Rest bis zur vollen Groesse SPARSE anlegen (truncate -> Loch, keine echten Nullbytes;
        # wichtig bei GByte-Images, sonst 2 GB Nullen im RAM/auf der Platte).
        f.truncate((start_sector + total_sectors) * SECTOR)

    print("%s: FAT%d, %d MB, %d Sektoren, %d Sektoren/Cluster, %d Cluster, FAT=%d Sektoren"
          % (out_path, 16 if is_fat16 else 12, size_mb, total_sectors, sec_per_clus,
             clusters, fat_sectors))


def main():
    ap = argparse.ArgumentParser(description="FAT12/16-Superfloppy-Image fuer den Q9-Emulator")
    ap.add_argument("out", help="Ausgabedatei (.img)")
    ap.add_argument("--mb", type=int, default=16, help="Groesse in MByte (Default 16)")
    ap.add_argument("--spc", type=int, default=4, help="Sektoren/Cluster (Default 4)")
    ap.add_argument("--label", default="Q9DATA", help="Datentraeger-Label (Default Q9DATA)")
    ap.add_argument("--start-sector", type=int, default=0,
                     help="Host-Startsektor fuer den FAT-Partitionanfang (Default 0)")
    args = ap.parse_args()
    build(args.out, args.mb, args.spc, args.label, args.start_sector)
    return 0


if __name__ == "__main__":
    sys.exit(main())
