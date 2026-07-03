#═════════════════════════════════════════════════════════════════════════════════════════════════
# File:   06_test_fat16.py                                                                Ver. 1.20
# Owner:  AF
# Desc.:  FAT16-Test (Phase 3.3/3.4/3.5): baut von Hand ein minimales FAT16-Superfloppy-Image
#         (Boot-Sektor/BPB + 2 FAT-Kopien + Root-Directory + Datenregion) nach oeffentlich
#         dokumentiertem Layout, schreibt es als q9disk.img und laesst Q9 im Selbsttest daraus
#         lesen: eine 8.3-Datei, eine Datei mit langem Namen (LFN-Eintraege), eine Datei in einem
#         Unterverzeichnis, sowie I$Seek mitten in eine Datei. Reines Python-Stdlib, kein
#         externes Tool (kein hdiutil/newfs_msdos noetig, damit der Test ueberall laeuft) —
#         das Andreas-Interop-Szenario (am Mac befuelltes Image) wird durch das Layout simuliert,
#         nicht durch echtes Mounten.
#
#         Seit 3.4 (FAT16 schreibend) laesst der Selbsttest Q9 zusaetzlich selbst schreiben
#         (I$Create/I$Write/I$MakDir/I$Delete auf /d0) und post_validate() liest das resultierende
#         Image DANACH nochmal komplett unabhaengig vom Q9-Code mit einem eigenen kleinen Python-
#         Parser (FAT-Kopien-Vergleich, Directory-Eintraege, Cluster-Freigabe) — der praktikable
#         Ersatz fuer "am Mac mounten" (ARBEITSPLAN.md 3.4-Notiz), weil er dieselbe Interop-Aussage
#         belegt: ein voellig unabhaengiger Leser versteht das von Q9 geschriebene Rohformat.
#
#         Seit 3.5 (F$Load) prueft derselbe Selbsttest-Lauf zusaetzlich, dass Q9 ein Modul aus
#         einer echten Datei auf /d0 laden kann (Nagelprobe Phase 2 + 3 zusammen): der Kernel-
#         Selbsttest (kernel.c) schreibt sich das Testmodul selbst per I$Create/I$Write, ruft
#         F$Load auf und raeumt seine Testdateien danach selbst wieder per I$Delete auf — bewusst
#         NICHT als zweiter `--selftest`-Prozesslauf in einem eigenen Testskript, weil ein erneuter
#         Lauf gegen dasselbe (schon veraenderte) Image nicht idempotent waere (z.B. I$MakDir
#         /d0/NEUDIR ein zweites Mal wuerde einen doppelten Directory-Eintrag anlegen) — alle
#         F$Load-Checks laufen deshalb in DIESEM EINEN Selbsttest-Aufruf mit.
#
# Call:   python test/06_test_fat16.py   (aus dem Projekt-Root, nach "make native")
#
# Edition History
#─────────┬──────┬─────────────────────────────────────────────────────────────────────────┬──────
# Date    │ Ver. │ Description                                                             │ By
#─────────┼──────┼─────────────────────────────────────────────────────────────────────────┼──────
# 26-07-04│ 1.00 │ Initiale Version                                                        │ CF
# 26-07-04│ 1.10 │ 3.4: Checks fuer I$Create/I$Write/I$MakDir/I$Delete + post_validate()   │ CF
#         │      │ (unabhaengige Python-Nachvalidierung des von Q9 geschriebenen Images)   │ CF
# 26-07-04│ 1.20 │ 3.5: Checks fuer F$Load (Modul aus Datei laden/validieren/registrieren, │ CF
#         │      │ F$Link danach ueber den Namen, F$UnLink, Fehlerfaelle E$PNNF/E$BMHP)    │ CF
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
        "FAT16: I$Create + I$Write":      "[ok] FAT16: I$Create + I$Write /d0/NEU.TXT" in result.stdout,
        "FAT16: geschriebene Datei zurueckgelesen":
            "[ok] FAT16: neu geschriebene Datei zurueckgelesen" in result.stdout,
        "FAT16: I$Create Duplikat -> E$BPNam":
            "[ok] FAT16: I$Create auf existierenden Namen -> E$BPNam" in result.stdout,
        "FAT16: I$Create LFN-Name -> E$BPNam":
            "[ok] FAT16: I$Create mit LFN-pflichtigem Namen -> E$BPNam" in result.stdout,
        "FAT16: I$MakDir + I$Open":       "[ok] FAT16: I$MakDir /d0/NEUDIR + I$Open darauf" in result.stdout,
        "F$Load: Testmodul geschrieben":
            "[ok] F$Load: Testmodul nach /d0/LOADMOD.BIN geschrieben" in result.stdout,
        "F$Load: geladen/validiert/registriert":
            "[ok] F$Load: /d0/LOADMOD.BIN geladen, validiert, registriert" in result.stdout,
        "F$Load + F$Link: ueber den Namen erreichbar":
            "[ok] F$Load + F$Link: dasselbe Modul ueber den Namen erreichbar" in result.stdout,
        "F$UnLink: beide Referenzen (F$Load+F$Link) abgebaut":
            "[ok] F$UnLink: beide Referenzen auf loadmod sauber abgebaut" in result.stdout,
        "F$Load: fehlende Datei -> E$PNNF":
            "[ok] F$Load: fehlende Datei -> E$PNNF" in result.stdout,
        "F$Load: kaputter Sync -> E$BMHP":
            "[ok] F$Load: kaputter Sync -> E$BMHP, kein Directory-Eintrag" in result.stdout,
        "FAT16: I$Delete + E$PNNF":       "[ok] FAT16: I$Delete /d0/NEU.TXT, danach E$PNNF" in result.stdout,
        "SYSCALL TEST PASS":               "SYSCALL TEST PASS" in result.stdout,
    }

    for name, ok in checks.items():
        print(f"  [{'ok' if ok else 'FEHLER'}] {name}")

    # --- Unabhaengige Python-Nachvalidierung des von Q9 geschriebenen Rohformats ------------------
    # Praktikabler Ersatz fuer "am Mac mounten" (ARBEITSPLAN.md 3.4): eine komplett unabhaengige
    # Neuimplementierung liest das Rohformat nach dem Q9-Selbsttest erneut und prueft FAT-Konsistenz
    # (beide Kopien identisch), Directory-Eintraege (NEUDIR als Verzeichnis mit "."/".." vorhanden,
    # NEU.TXT als geloescht markiert -DIRENT_FREE) und dass die vormals von NEU.TXT belegten Cluster
    # wieder frei sind (FAT16_FREE in BEIDEN Kopien) — dieselbe Interop-Aussage wie ein echtes Mounten,
    # weil sie das Byte-Layout unabhaengig vom Q9-eigenen Code nachrechnet.
    postcheck_ok, postcheck_msgs = post_validate(IMG)
    for msg in postcheck_msgs:
        print(f"  {msg}")
    checks["Python-Nachvalidierung des Rohformats"] = postcheck_ok

    if all(checks.values()):
        print("06_test_fat16: PASS")
        return 0
    print("06_test_fat16: FAIL")
    print(result.stdout)
    return 1


def post_validate(img_path: str):
    """Liest das Q9-geschriebene Image komplett unabhaengig vom Q9-Code neu ein (eigener Python-
    Parser, kein Aufruf von Q9-Funktionen) und prueft: (1) beide FAT-Kopien sind byteidentisch,
    (2) NEUDIR existiert im Root als Verzeichnis mit einem Cluster, dessen erste zwei Eintraege
    "." (-> sich selbst) und ".." (-> Root, Cluster 0) sind, (3) NEU.TXT ist im Root als geloescht
    markiert (erstes Namensbyte DIRENT_FREE=0xE5), (4) alle Cluster, die NEU.TXT vor dem Loeschen
    belegt haben koennte, sind in BEIDEN FAT-Kopien wieder FAT16_FREE (kein Leck). Rueckgabe:
    (ok: bool, messages: list[str])."""
    msgs = []
    ok = True
    with open(img_path, "rb") as f:
        data = f.read()

    fat_start = RESERVED_SECS
    fat1 = data[fat_start * SECTOR: (fat_start + FAT_SECTORS) * SECTOR]
    fat2 = data[(fat_start + FAT_SECTORS) * SECTOR: (fat_start + 2 * FAT_SECTORS) * SECTOR]
    if fat1 == fat2:
        msgs.append("[ok] Python-Nachvalidierung: beide FAT-Kopien identisch")
    else:
        msgs.append("[FEHLER] Python-Nachvalidierung: FAT-Kopien weichen voneinander ab")
        ok = False

    root_start = fat_start + NUM_FATS * FAT_SECTORS
    root_sectors = ROOT_ENT_CNT * 32 // SECTOR
    root = data[root_start * SECTOR: (root_start + root_sectors) * SECTOR]

    def parse_dirent(raw: bytes) -> dict:
        name = raw[0:8]
        ext = raw[8:11]
        attr = raw[0x0B]
        fstcluslo = struct.unpack_from("<H", raw, 0x1A)[0]
        filesize = struct.unpack_from("<I", raw, 0x1C)[0]
        return {"name": name, "ext": ext, "attr": attr, "clus": fstcluslo, "size": filesize}

    root_entries = [parse_dirent(root[i:i + 32]) for i in range(0, len(root), 32)]

    neu_txt = None
    neudir = None
    for e in root_entries:
        # 8.3-Name "NEU     " wird beim Loeschen NUR im ersten Byte durch DIRENT_FREE (0xE5)
        # ersetzt (FAT16-Konvention) — der Rest des Namensfeldes ("EU" + Padding) bleibt stehen.
        if e["name"][0:1] == b"\xe5" and e["name"][1:8] == b"EU     " and e["ext"] == b"TXT":
            neu_txt = e
        if e["name"] == b"NEUDIR  " and e["ext"] == b"   " and (e["attr"] & 0x10):
            neudir = e

    if neu_txt is not None:
        msgs.append("[ok] Python-Nachvalidierung: NEU.TXT im Root als geloescht (DIRENT_FREE) markiert")
    else:
        msgs.append("[FEHLER] Python-Nachvalidierung: kein geloeschter NEU.TXT-Eintrag im Root gefunden")
        ok = False

    # 3.5: F$Load-Testdateien (/d0/LOADMOD.BIN, /d0/BADMOD.BIN) raeumt der Kernel-Selbsttest selbst
    # per I$Delete wieder auf (kernel.c) — hier unabhaengig bestaetigt, dass davon kein AKTIVER
    # (nicht geloeschter) Directory-Eintrag mehr im Root steht, sonst wuerden sie bei einem
    # naechsten Testlauf Directory-Slots/Cluster dauerhaft binden.
    leftover = [e for e in root_entries
                if e["name"][0:1] != b"\xe5" and e["name"][0:1] != b"\x00" and
                ((e["name"] == b"LOADMOD " and e["ext"] == b"BIN") or
                 (e["name"] == b"BADMOD  " and e["ext"] == b"BIN"))]
    if not leftover:
        msgs.append("[ok] Python-Nachvalidierung: F$Load-Testdateien (LOADMOD.BIN/BADMOD.BIN) sauber aufgeraeumt")
    else:
        msgs.append(f"[FEHLER] Python-Nachvalidierung: F$Load-Testdateien nicht aufgeraeumt: {leftover}")
        ok = False

    if neudir is not None:
        msgs.append(f"[ok] Python-Nachvalidierung: NEUDIR im Root als Verzeichnis gefunden (Cluster {neudir['clus']})")
        data_start = root_start + root_sectors
        clus_lba = data_start + (neudir["clus"] - 2) * SEC_PER_CLUS
        clusdata = data[clus_lba * SECTOR: clus_lba * SECTOR + 64]
        dot = parse_dirent(clusdata[0:32])
        dotdot = parse_dirent(clusdata[32:64])
        if dot["name"] == b".       " and dot["clus"] == neudir["clus"] and \
           dotdot["name"] == b"..      " and dotdot["clus"] == 0:
            msgs.append("[ok] Python-Nachvalidierung: NEUDIR-Cluster enthaelt korrekte '.'/'..' -Eintraege")
        else:
            msgs.append("[FEHLER] Python-Nachvalidierung: '.'/'..' im NEUDIR-Cluster fehlerhaft")
            ok = False
    else:
        msgs.append("[FEHLER] Python-Nachvalidierung: kein NEUDIR-Verzeichniseintrag im Root gefunden")
        ok = False

    # NEU.TXT wurde per I$Create/I$Write angelegt (erster freier Cluster ab 2, linear gesucht ->
    # Cluster 6, da 2/3/4/5 schon von HELLO.TXT/Langname-Datei/SUBDIR/NESTED.TXT belegt sind) und
    # per I$Delete wieder geloescht — sein Cluster muss danach in BEIDEN FAT-Kopien wieder
    # FAT16_FREE (0x0000) sein. NEUDIR (Cluster 7, s.o.) bleibt dagegen bewusst belegt (nicht
    # geloescht) und darf hier NICHT als frei erwartet werden.
    neu_clus = 6
    v1 = struct.unpack_from("<H", fat1, neu_clus * 2)[0]
    v2 = struct.unpack_from("<H", fat2, neu_clus * 2)[0]
    if v1 == 0x0000 and v2 == 0x0000:
        msgs.append(f"[ok] Python-Nachvalidierung: Cluster {neu_clus} (ex-NEU.TXT) nach I$Delete in beiden FAT-Kopien frei")
    else:
        msgs.append(f"[FEHLER] Python-Nachvalidierung: Cluster-Leck nach I$Delete: Cluster {neu_clus} = ({v1:#06x}, {v2:#06x})")
        ok = False

    return ok, msgs


if __name__ == "__main__":
    sys.exit(main())

#─────────────────────────────────────────────────────────────────────────────────────────────────
# EOF 06_test_fat16.py                                                                    Ver. 1.20
#─────────────────────────────────────────────────────────────────────────────────────────────────
