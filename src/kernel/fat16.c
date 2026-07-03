//════════════════════════════════════════════════════════════════════════════════════════════════
// File:   fat16.c                                                                        Ver. 1.00
// Owner:  AF
// Desc.:  Q9 FAT16-File-Manager, lesend (Phase 3.3). Boot-Sektor (BPB) parsen, Root-Directory
//         und Unterverzeichnisse durchsuchen (8.3- und LFN-Namen), Cluster-Ketten der FAT folgen,
//         Datei-Inhalt lesen. Nur Superfloppy (kein MBR, Boot-Sektor bei LBA 0 des Block-Device).
//         Zugriff auf den Datentraeger AUSSCHLIESSLICH ueber q9_hal_blk_read (Q9_BLK_SIZE-Byte-
//         Bloecke) — kein direkter Treiberzugriff, kein malloc (ein statischer 512-Byte-Sektor-
//         Puffer fuer alle Lesevorgaenge, da Q9 nicht nebenlaeufig ist).
//
//         Datei-Kontext pro Pfad (q9_path_t.fmctx, 16 Byte, device.h) — Layout (siehe fat16_ctx_t
//         unten): start_cluster (Cluster der Datei; 0 = Root-Directory-Pseudo-Eintrag),
//         cur_cluster (Cluster, in dem "pos" gerade liegt), pos (aktuelle Byte-Position in der
//         Datei), size (Dateigroesse aus dem Directory-Eintrag). Root-Dir hat keine Cluster-Kette
//         (fester Bereich vor der Datenregion) — start_cluster == 0 markiert diesen Sonderfall,
//         weil Cluster 0/1 in FAT16 nie echte Datencluster sind (Datencluster beginnen bei 2).
//
// Call:   siehe fat16.h
//
// Edition History
//─────────┬──────┬────────────────────────────────────────────────────────────────────────┬──────
// Date    │ Ver. │ Description                                                            │ By
//─────────┼──────┼────────────────────────────────────────────────────────────────────────┼──────
// 26-07-04│ 1.00 │ 3.3: Initiale Version                                                  │ CF
//═════════╧══════╧════════════════════════════════════════════════════════════════════════╧══════

#include "../hal/q9_hal.h"
#include "device.h"
#include "fat16.h"
#include "syscall.h"

//╔══════════════════════════════════════════════════════════════════════════════════════════════╗
//║ ON-DISK STRUCTURES (oeffentlich dokumentiertes FAT16-Layout, siehe fat16.h)                   ║
//╚══════════════════════════════════════════════════════════════════════════════════════════════╝

#pragma pack(push, 1)
typedef struct fat16_bpb {
    uint8_t  jmp[3];                                    /* $00: Sprungbefehl (ignoriert)          */
    uint8_t  oemname[8];                                /* $03                                    */
    uint16_t bytespersec;                               /* $0B                                    */
    uint8_t  secperclus;                                /* $0D                                    */
    uint16_t reservedsecs;                               /* $0E                                    */
    uint8_t  numfats;                                    /* $10                                    */
    uint16_t rootentcnt;                                 /* $11                                    */
    uint16_t totsec16;                                   /* $13                                    */
    uint8_t  media;                                      /* $15                                    */
    uint16_t fatsz16;                                    /* $16                                    */
    uint16_t secpertrk;                                  /* $18                                    */
    uint16_t numheads;                                   /* $1A                                    */
    uint32_t hiddensecs;                                 /* $1C                                    */
    uint32_t totsec32;                                   /* $20                                    */
    uint8_t  drvnum;                                     /* $24                                    */
    uint8_t  reserved1;                                  /* $25                                    */
    uint8_t  bootsig;                                    /* $26                                    */
    uint32_t volid;                                      /* $27                                    */
    uint8_t  vollabel[11];                               /* $2B                                    */
    uint8_t  fstype[8];                                  /* $36                                    */
    uint8_t  pad[448];                                   /* $3E: Bootcode, ungenutzt               */
    uint16_t sig55aa;                                    /* $1FE: Boot-Signatur $55AA (LE: AA 55)  */
} fat16_bpb_t;

typedef struct fat16_dirent {
    uint8_t  name[8];                                    /* $00: 8.3-Name (padded mit Space)       */
    uint8_t  ext[3];                                      /* $08: 8.3-Erweiterung                   */
    uint8_t  attr;                                        /* $0B: Attribut-Bits                     */
    uint8_t  ntres;                                       /* $0C                                    */
    uint8_t  crttimetenth;                                /* $0D                                    */
    uint16_t crttime;                                     /* $0E                                    */
    uint16_t crtdate;                                     /* $10                                    */
    uint16_t lstaccdate;                                  /* $12                                    */
    uint16_t fstclushi;                                   /* $14: FAT16 immer 0                     */
    uint16_t wrttime;                                     /* $16                                    */
    uint16_t wrtdate;                                     /* $18                                    */
    uint16_t fstcluslo;                                   /* $1A: Start-Cluster (FAT16: das ganze   */
                                                          /*      Cluster-Feld)                     */
    uint32_t filesize;                                   /* $1C                                    */
} fat16_dirent_t;                                        /* 32 Byte gesamt                          */

typedef struct fat16_lfnent {
    uint8_t  seq;                                         /* $00: Sequenznummer, Bit6 = letzter Teil*/
    uint16_t name1[5];                                    /* $01: UTF-16-Zeichen 1..5               */
    uint8_t  attr;                                         /* $0B: immer $0F (ATTR_LFN)              */
    uint8_t  type;                                         /* $0C: immer 0                           */
    uint8_t  chksum;                                       /* $0D: Pruefsumme des 8.3-Alias           */
    uint16_t name2[6];                                     /* $0E: UTF-16-Zeichen 6..11              */
    uint16_t fstcluslo;                                    /* $1A: immer 0                           */
    uint16_t name3[2];                                     /* $1C: UTF-16-Zeichen 12..13              */
} fat16_lfnent_t;                                          /* 32 Byte gesamt                          */
#pragma pack(pop)

#define ATTR_READONLY  0x01
#define ATTR_HIDDEN    0x02
#define ATTR_SYSTEM    0x04
#define ATTR_VOLUMEID  0x08
#define ATTR_DIRECTORY 0x10
#define ATTR_ARCHIVE   0x20
#define ATTR_LFN       0x0F                                /* (READONLY|HIDDEN|SYSTEM|VOLUMEID)      */

#define FAT16_EOC_MIN  0xFFF8u                              /* Cluster-Kette Ende: 0xFFF8..0xFFFF     */
#define FAT16_BAD      0xFFF7u
#define FAT16_FREE     0x0000u
#define DIRENT_FREE    0xE5                                 /* geloeschter Eintrag (erstes Namensbyte)*/
#define DIRENT_END     0x00                                 /* Ende des Directory                     */

//╔══════════════════════════════════════════════════════════════════════════════════════════════╗
//║ FILE CONTEXT (q9_path_t.fmctx, 16 Byte — device.h Q9_FMCTX_SIZE)                              ║
//╚══════════════════════════════════════════════════════════════════════════════════════════════╝

#pragma pack(push, 1)
typedef struct fat16_ctx {
    uint32_t start_cluster;                             /* 0 = Root-Directory-Pseudo-Datei         */
    uint32_t cur_cluster;                               /* Cluster, in dem "pos" gerade liegt      */
    uint32_t pos;                                        /* aktuelle Byte-Position in der Datei     */
    uint32_t size;                                       /* Dateigroesse (Root-Dir: Bytes gesamt)   */
} fat16_ctx_t;
#pragma pack(pop)

//╔══════════════════════════════════════════════════════════════════════════════════════════════╗
//║ MOUNT STATE (ein Datentraeger reicht fuer Q9 — statisch, kein malloc)                        ║
//╚══════════════════════════════════════════════════════════════════════════════════════════════╝

static int      mounted        = 0;
static uint32_t fat_start_lba;                          /* erste FAT-Kopie                        */
static uint32_t fatsz;                                  /* Sektoren pro FAT-Kopie                  */
static uint32_t root_start_lba;
static uint32_t root_sectors;                           /* Root-Directory-Region in Sektoren       */
static uint32_t data_start_lba;                         /* Cluster 2 beginnt hier                  */
static uint32_t sec_per_clus;
static uint32_t root_bytes;                             /* root_sectors * Q9_BLK_SIZE              */

static uint8_t  secbuf[Q9_BLK_SIZE];                    /* einziger Sektor-Arbeitspuffer           */

//╔══════════════════════════════════════════════════════════════════════════════════════════════╗
//║ INTERNAL HELPERS                                                                             ║
//╚══════════════════════════════════════════════════════════════════════════════════════════════╝

static uint32_t clus_bytes(void)
{
    return sec_per_clus * Q9_BLK_SIZE;
}

//────────────────────────────────────────────────────────────────────────────────────────────────
// Function: clus_to_lba
// Desc.:    Erster Sektor eines Datencluster (Cluster >= 2, FAT16-Konvention).
//────────────────────────────────────────────────────────────────────────────────────────────────
static uint32_t clus_to_lba(uint32_t clus)
{
    return data_start_lba + (clus - 2u) * sec_per_clus;
}

//────────────────────────────────────────────────────────────────────────────────────────────────
// Function: fat_next
// Desc.:    Liefert den naechsten Cluster in der Kette (FAT16: 16-Bit-Eintraege, 256 pro Sektor).
//           Rueckgabe >= 0xFFF8 = Kettenende, sonst naechster Cluster. 0xFFFFFFFF bei I/O-Fehler.
//────────────────────────────────────────────────────────────────────────────────────────────────
static uint32_t fat_next(uint32_t clus)
{
    uint32_t byteoff = clus * 2u;
    uint32_t lba     = fat_start_lba + byteoff / Q9_BLK_SIZE;
    uint32_t off     = byteoff % Q9_BLK_SIZE;

    if (q9_hal_blk_read(lba, secbuf) != 0) {
        return 0xFFFFFFFFu;
    }
    return (uint32_t)secbuf[off] | ((uint32_t)secbuf[off + 1] << 8);
}

//────────────────────────────────────────────────────────────────────────────────────────────────
// Function: cluster_at_offset
// Desc.:    Folgt der Cluster-Kette ab "start" um "nskip" Cluster weiter (0 = start selbst).
//           0 bei Kettenende/Fehler (Cluster 0/1 sind nie gueltige Datencluster).
//────────────────────────────────────────────────────────────────────────────────────────────────
static uint32_t cluster_at_offset(uint32_t start, uint32_t nskip)
{
    uint32_t c = start;

    while (nskip-- > 0) {
        c = fat_next(c);
        if (c == 0xFFFFFFFFu || c >= FAT16_EOC_MIN || c < 2u) {
            return 0;
        }
    }
    return c;
}

//────────────────────────────────────────────────────────────────────────────────────────────────
// Function: dirent_name83
// Desc.:    Setzt "out" (13 Byte reichen: 8+1+3+1) auf den 8.3-Namen in kanonischer Form
//           ("NAME.EXT", ohne Padding, Kleinbuchstaben — FAT ist case-insensitiv gespeichert in
//           Grossbuchstaben, wir vergleichen spaeter case-insensitiv). Rueckgabe: Laenge.
//────────────────────────────────────────────────────────────────────────────────────────────────
static uint32_t dirent_name83(const fat16_dirent_t *de, char *out)
{
    uint32_t n = 0;
    for (int i = 0; i < 8 && de->name[i] != ' '; i++) {
        out[n++] = (char)de->name[i];
    }
    if (de->ext[0] != ' ') {
        out[n++] = '.';
        for (int i = 0; i < 3 && de->ext[i] != ' '; i++) {
            out[n++] = (char)de->ext[i];
        }
    }
    out[n] = 0;
    return n;
}

//────────────────────────────────────────────────────────────────────────────────────────────────
// Function: ci_eq
// Desc.:    Case-insensitiver Vergleich zweier Namen fester Laenge (a) gegen nullterminiertes b.
//────────────────────────────────────────────────────────────────────────────────────────────────
static int ci_eq(const char *a, uint32_t alen, const char *b)
{
    uint32_t i;
    for (i = 0; i < alen; i++) {
        char ca = a[i];
        char cb = b[i];
        if (ca >= 'a' && ca <= 'z') ca = (char)(ca - 'a' + 'A');
        if (cb >= 'a' && cb <= 'z') cb = (char)(cb - 'a' + 'A');
        if (cb == 0 || ca != cb) {
            return 0;
        }
    }
    return b[i] == 0;
}

//────────────────────────────────────────────────────────────────────────────────────────────────
// Function: lfn_char
// Desc.:    Liefert das i-te UTF-16-Zeichen eines LFN-Eintrags (0..12), 0xFFFF wenn ausserhalb.
//           Nur der Low-Byte wird verwendet (BMP-Zeichen < 0x100 reichen fuer Q9-Namenszeichen;
//           hoehere Codepoints werden auf '?' abgebildet — Q9-Pfadnamen sind ohnehin auf das
//           OS-9-Namensalphabet plus die Zeichen begrenzt, die 8.3/LFN hergeben).
//────────────────────────────────────────────────────────────────────────────────────────────────
static uint16_t lfn_char(const fat16_lfnent_t *le, int i)
{
    if (i < 5)  return le->name1[i];
    if (i < 11) return le->name2[i - 5];
    if (i < 13) return le->name3[i - 11];
    return 0xFFFFu;
}

//────────────────────────────────────────────────────────────────────────────────────────────────
// Function: read_dir_slot
// Desc.:    Liest den "index"-ten 32-Byte-Slot eines Directory (Root ODER Unterverzeichnis) in
//           "de" (als Dirent) bzw. "le" (als LFN-Eintrag, gleicher Speicher, unterschiedliche
//           Interpretation je nach de->attr). "dirstart" = 0 -> Root-Directory, sonst Start-
//           Cluster eines Unterverzeichnisses. Rueckgabe: 1 = gelesen, 0 = Ende des Directory
//           erreicht (freier Slot mit DIRENT_END) oder Fehler/aus der Kette gelaufen.
//────────────────────────────────────────────────────────────────────────────────────────────────
static int read_dir_slot(uint32_t dirstart, uint32_t index, fat16_dirent_t *out)
{
    uint32_t entsperclus  = clus_bytes() / 32u;
    uint32_t entspersec   = Q9_BLK_SIZE / 32u;
    uint32_t lba;
    uint32_t slot_in_sec;

    if (dirstart == 0) {                                /* Root-Directory: fester Bereich          */
        uint32_t entsperroot = root_bytes / 32u;
        if (index >= entsperroot) {
            return 0;
        }
        lba = root_start_lba + (index * 32u) / Q9_BLK_SIZE;
        slot_in_sec = index % entspersec;
    } else {                                            /* Unterverzeichnis: Cluster-Kette         */
        uint32_t clusidx = index / entsperclus;
        uint32_t inclus  = index % entsperclus;
        uint32_t clus    = cluster_at_offset(dirstart, clusidx);
        if (clus == 0) {
            return 0;
        }
        lba = clus_to_lba(clus) + (inclus * 32u) / Q9_BLK_SIZE;
        slot_in_sec = inclus % entspersec;
    }
    if (q9_hal_blk_read(lba, secbuf) != 0) {
        return 0;
    }
    for (uint32_t i = 0; i < 32; i++) {
        ((uint8_t *)out)[i] = secbuf[slot_in_sec * 32u + i];
    }
    return (out->name[0] != DIRENT_END);
}

//────────────────────────────────────────────────────────────────────────────────────────────────
// Function: dir_find
// Desc.:    Sucht im Directory "dirstart" (0 = Root) nach "name" (Laenge "len", ein einzelnes
//           Pfadelement ohne '/'). Vergleicht sowohl den zusammengesetzten LFN-Namen (falls LFN-
//           Eintraege vorausgehen) als auch den 8.3-Namen. Liefert den gefundenen Dirent in "out".
//           0 = gefunden, E$PNNF = nicht gefunden/Lesefehler.
//────────────────────────────────────────────────────────────────────────────────────────────────
static int dir_find(uint32_t dirstart, const char *name, uint32_t len, fat16_dirent_t *out)
{
    fat16_dirent_t de;
    char           lfnbuf[261];                          /* max. 20 LFN-Eintraege * 13 Zeichen      */
    uint32_t       lfnlen = 0;
    int            have_lfn = 0;

    for (uint32_t idx = 0; read_dir_slot(dirstart, idx, &de); idx++) {
        if ((uint8_t)de.name[0] == DIRENT_FREE) {
            have_lfn = 0;                               /* geloeschter Eintrag reisst LFN-Kette ab */
            continue;
        }
        if (de.attr == ATTR_LFN) {
            const fat16_lfnent_t *le = (const fat16_lfnent_t *)&de;
            uint32_t seqnum  = le->seq & 0x1Fu;
            uint32_t partlen = 0;
            char     part[13];
            for (int i = 0; i < 13; i++) {
                uint16_t ch = lfn_char(le, i);
                if (ch == 0 || ch == 0xFFFFu) {
                    break;
                }
                part[partlen++] = (char)(ch < 0x80 ? ch : '?');
            }
            /* LFN-Eintraege liegen in ABSTEIGENDER Sequenz vor dem 8.3-Eintrag (letzter Teil        */
            /* zuerst, Bit6 im seq-Byte). Wir bauen den Namen daher von hinten nach vorne zusammen:  */
            /* beim ERSTEN (= letzten Teil, hoechste Sequenznummer) Eintrag verwerfen wir was vorher */
            /* dastand und schieben den neuen Teil an den ANFANG. */
            if (le->seq & 0x40u) {
                lfnlen = 0;
            }
            if (lfnlen + partlen < sizeof(lfnbuf) - 1) {
                for (uint32_t i = lfnlen; i > 0; i--) {    /* rueckwaerts kopieren (ueberlappender    */
                    lfnbuf[partlen + i - 1] = lfnbuf[i - 1]; /* Bereich, da wir nach RECHTS schieben)  */
                }
                for (uint32_t i = 0; i < partlen; i++) {
                    lfnbuf[i] = part[i];
                }
                lfnlen += partlen;
                lfnbuf[lfnlen] = 0;
            }
            have_lfn = (seqnum != 0);
            continue;
        }
        if (de.attr & ATTR_VOLUMEID) {                   /* Volume-Label — kein Datei-/Dir-Eintrag  */
            have_lfn = 0;
            continue;
        }
        if (have_lfn && lfnlen == len && ci_eq(name, len, lfnbuf)) {
            *out = de;
            return 0;
        }
        {
            char name83[13];
            uint32_t n83 = dirent_name83(&de, name83);
            if (n83 == len && ci_eq(name, len, name83)) {
                *out = de;
                return 0;
            }
        }
        have_lfn = 0;
    }
    return E_PNNF;
}

//────────────────────────────────────────────────────────────────────────────────────────────────
// Function: split_first
// Desc.:    Zerlegt "path" (ohne fuehrenden '/') in das erste Element (bis '/' oder Ende) und den
//           Rest (hinter dem '/', oder NULL wenn kein weiteres Element folgt).
//────────────────────────────────────────────────────────────────────────────────────────────────
static void split_first(const char *path, uint32_t len, const char **elem, uint32_t *elemlen,
                         const char **restp, uint32_t *restlen)
{
    uint32_t i = 0;
    while (i < len && path[i] != '/') {
        i++;
    }
    *elem    = path;
    *elemlen = i;
    if (i < len) {
        *restp   = path + i + 1;
        *restlen = len - i - 1;
    } else {
        *restp   = 0;
        *restlen = 0;
    }
}

//────────────────────────────────────────────────────────────────────────────────────────────────
// Function: resolve_path
// Desc.:    Loest "restpath" (Laenge "len", KEIN fuehrender '/', s. vfs.h) ab dem Root-Directory
//           Element fuer Element auf. Leerer restpath -> Root-Directory selbst (Pseudo-Dirent
//           mit attr=ATTR_DIRECTORY, fstcluslo=0, filesize=root_bytes). 0 = gefunden (out gesetzt,
//           is_root zeigt den Root-Sonderfall an), sonst E$PNNF.
//────────────────────────────────────────────────────────────────────────────────────────────────
static int resolve_path(const char *restpath, uint32_t len, fat16_dirent_t *out, int *is_root)
{
    uint32_t dirstart = 0;                               /* 0 = Root-Directory                     */
    const char *cur = restpath;
    uint32_t    curlen = len;

    *is_root = 0;
    if (len == 0) {
        *is_root = 1;
        return 0;
    }
    for (;;) {
        const char *elem;
        uint32_t    elemlen;
        const char *rest;
        uint32_t    restlen;
        int         err;

        split_first(cur, curlen, &elem, &elemlen, &rest, &restlen);
        if (elemlen == 0) {
            return E_PNNF;
        }
        err = dir_find(dirstart, elem, elemlen, out);
        if (err != 0) {
            return err;
        }
        if (!rest) {                                     /* letztes Element gefunden                */
            return 0;
        }
        if (!(out->attr & ATTR_DIRECTORY)) {              /* Zwischenelement ist keine Datei-Datei   */
            return E_PNNF;
        }
        dirstart = out->fstcluslo;
        cur      = rest;
        curlen   = restlen;
    }
}

//╔══════════════════════════════════════════════════════════════════════════════════════════════╗
//║ FILE-MANAGER OPERATIONS (q9_fm_t, vfs.h)                                                     ║
//╚══════════════════════════════════════════════════════════════════════════════════════════════╝

//────────────────────────────────────────────────────────────────────────────────────────────────
// Function: fat16_open
// Desc.:    I$Open-Unterbau: loest restpath auf, befuellt p->fmctx (fat16_ctx_t). Verzeichnisse
//           duerfen geoeffnet werden (Position/Groesse wie eine Datei — I$Read liefert dann die
//           rohen 32-Byte-Directory-Eintraege; ausreichend fuer 3.3, ein eigener I$Read-Dirent-
//           Modus ist nicht gefordert). E$PNNF, wenn der Pfad nicht existiert.
//────────────────────────────────────────────────────────────────────────────────────────────────
static int fat16_open(q9_dev_t *dev, q9_path_t *p, const char *restpath, uint32_t len, uint8_t mode)
{
    fat16_dirent_t de;
    fat16_ctx_t    ctx;
    int            is_root;
    int            err;

    (void)dev;
    if (!mounted) {
        return E_NOTRDY;
    }
    if (mode & Q9_MODE_WRITE) {                          /* 3.3 = nur lesend                       */
        return E_UNKSVC;
    }
    err = resolve_path(restpath, len, &de, &is_root);
    if (err != 0) {
        return err;
    }
    if (is_root) {
        ctx.start_cluster = 0;
        ctx.size          = root_bytes;
    } else {
        ctx.start_cluster = de.fstcluslo;
        ctx.size          = de.filesize;
        if (de.attr & ATTR_DIRECTORY) {                   /* Directory-Groesse ist im Eintrag 0 —    */
            ctx.size = 0xFFFFFFFFu;                       /* Kette bis EOC folgen (I$Read-Grenzwert) */
        }
    }
    ctx.cur_cluster = ctx.start_cluster;
    ctx.pos         = 0;
    for (uint32_t i = 0; i < sizeof(ctx); i++) {
        p->fmctx[i] = ((const uint8_t *)&ctx)[i];
    }
    return 0;
}

//────────────────────────────────────────────────────────────────────────────────────────────────
// Function: fat16_read
// Desc.:    I$Read-Unterbau: liest ab der aktuellen Position (fmctx) bis zu *n Bytes, folgt dabei
//           der Cluster-Kette (bzw. liest sequenziell aus dem festen Root-Bereich). *n = tatsaech-
//           lich gelesene Bytes. E$EOF, wenn die Position bereits am Dateiende steht.
//────────────────────────────────────────────────────────────────────────────────────────────────
static int fat16_read(q9_dev_t *dev, q9_path_t *p, uint8_t *buf, uint32_t *n)
{
    fat16_ctx_t ctx;
    uint32_t    want = *n;
    uint32_t    got  = 0;

    (void)dev;
    if (!mounted) {
        return E_NOTRDY;
    }
    for (uint32_t i = 0; i < sizeof(ctx); i++) {
        ((uint8_t *)&ctx)[i] = p->fmctx[i];
    }
    if (ctx.size != 0xFFFFFFFFu && ctx.pos >= ctx.size) {
        return E_EOF;
    }
    while (got < want) {
        uint32_t lba;
        uint32_t off_in_region;                          /* Byte-Offset innerhalb Cluster/Root      */
        uint32_t off_in_sec;
        uint32_t avail;

        if (ctx.size != 0xFFFFFFFFu && ctx.pos >= ctx.size) {
            break;                                       /* Dateiende erreicht                      */
        }
        if (ctx.start_cluster == 0) {                     /* Root-Directory: fester Bereich          */
            if (ctx.pos >= root_bytes) {
                break;
            }
            lba = root_start_lba + ctx.pos / Q9_BLK_SIZE;
            off_in_sec = ctx.pos % Q9_BLK_SIZE;
            avail = Q9_BLK_SIZE - off_in_sec;
            if (root_bytes - ctx.pos < avail) {
                avail = root_bytes - ctx.pos;
            }
        } else {
            off_in_region = ctx.pos % clus_bytes();
            if (off_in_region == 0 && ctx.pos != 0) {      /* Cluster-Grenze ueberschritten           */
                uint32_t nextc = fat_next(ctx.cur_cluster);
                if (nextc == 0xFFFFFFFFu || nextc >= FAT16_EOC_MIN || nextc < 2u) {
                    break;                                /* Kettenende (kuerzer als "size" behauptet)*/
                }
                ctx.cur_cluster = nextc;
            }
            lba = clus_to_lba(ctx.cur_cluster) + off_in_region / Q9_BLK_SIZE;
            off_in_sec = off_in_region % Q9_BLK_SIZE;
            avail = Q9_BLK_SIZE - off_in_sec;
            if (clus_bytes() - off_in_region < avail) {
                avail = clus_bytes() - off_in_region;
            }
        }
        if (avail > want - got) {
            avail = want - got;
        }
        if (ctx.size != 0xFFFFFFFFu && avail > ctx.size - ctx.pos) {
            avail = ctx.size - ctx.pos;
        }
        if (avail == 0) {
            break;
        }
        if (q9_hal_blk_read(lba, secbuf) != 0) {
            return E_NOTRDY;
        }
        for (uint32_t i = 0; i < avail; i++) {
            buf[got + i] = secbuf[off_in_sec + i];
        }
        got     += avail;
        ctx.pos += avail;
    }
    for (uint32_t i = 0; i < sizeof(ctx); i++) {
        p->fmctx[i] = ((const uint8_t *)&ctx)[i];
    }
    *n = got;
    if (got == 0 && want > 0) {
        return E_EOF;
    }
    return 0;
}

//────────────────────────────────────────────────────────────────────────────────────────────────
// Function: fat16_seek
// Desc.:    I$Seek-Unterbau: setzt die Position absolut. E$Param, wenn "pos" hinter dem
//           Dateiende liegt (Root-Directory/normale Dateien; Unterverzeichnisse mit unbekannter
//           Groesse — size==0xFFFFFFFF — erlauben jede Position, Grenzpruefung passiert beim
//           naechsten I$Read ueber die Kettenlaenge).
//────────────────────────────────────────────────────────────────────────────────────────────────
static int fat16_seek(q9_dev_t *dev, q9_path_t *p, uint32_t pos)
{
    fat16_ctx_t ctx;
    uint32_t    clusidx;

    (void)dev;
    if (!mounted) {
        return E_NOTRDY;
    }
    for (uint32_t i = 0; i < sizeof(ctx); i++) {
        ((uint8_t *)&ctx)[i] = p->fmctx[i];
    }
    if (ctx.size != 0xFFFFFFFFu && pos > ctx.size) {
        return E_PARAM;
    }
    ctx.pos = pos;
    if (ctx.start_cluster != 0) {                         /* Root-Dir braucht keinen Cluster-Zeiger  */
        clusidx = pos / clus_bytes();
        ctx.cur_cluster = cluster_at_offset(ctx.start_cluster, clusidx);
        if (ctx.cur_cluster == 0 && pos < ctx.size) {
            return E_PARAM;                              /* Kette kuerzer als erwartet               */
        }
    }
    for (uint32_t i = 0; i < sizeof(ctx); i++) {
        p->fmctx[i] = ((const uint8_t *)&ctx)[i];
    }
    return 0;
}

static int fat16_create(q9_dev_t *dev, q9_path_t *p, const char *restpath, uint32_t len, uint8_t mode)
{
    (void)dev; (void)p; (void)restpath; (void)len; (void)mode;
    return E_UNKSVC;                                     /* 3.4: FAT16 schreibend                   */
}

static int fat16_makdir(q9_dev_t *dev, const char *restpath, uint32_t len)
{
    (void)dev; (void)restpath; (void)len;
    return E_UNKSVC;                                     /* 3.4                                     */
}

static int fat16_remove(q9_dev_t *dev, const char *restpath, uint32_t len)
{
    (void)dev; (void)restpath; (void)len;
    return E_UNKSVC;                                     /* 3.4                                     */
}

const q9_fm_t q9_fat16_fm = {
    "FAT16",
    fat16_open,
    fat16_create,
    fat16_makdir,
    fat16_remove,
    fat16_read,
    fat16_seek,
};

//╔══════════════════════════════════════════════════════════════════════════════════════════════╗
//║ MOUNT                                                                                        ║
//╚══════════════════════════════════════════════════════════════════════════════════════════════╝

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_fat16_mount
// Desc.:    Boot-Sektor lesen + plausibilisieren, Geometrie ableiten. Siehe fat16.h.
// Call:     err = q9_fat16_mount()
//════════════════════════════════════════════════════════════════════════════════════════════════
int q9_fat16_mount(void)
{
    fat16_bpb_t bpb;

    mounted = 0;
    if (q9_hal_blk_read(0, secbuf) != 0) {
        return E_NOTRDY;
    }
    for (uint32_t i = 0; i < sizeof(bpb) && i < Q9_BLK_SIZE; i++) {
        ((uint8_t *)&bpb)[i] = secbuf[i];
    }
    if (bpb.sig55aa != 0xAA55u) {                         /* Little-Endian: Byte AA, dann 55         */
        return E_NOTRDY;
    }
    if (bpb.bytespersec != Q9_BLK_SIZE) {
        return E_NOTRDY;
    }
    if (bpb.secperclus == 0 || (bpb.secperclus & (bpb.secperclus - 1)) != 0) {
        return E_NOTRDY;                                 /* muss Zweierpotenz sein                  */
    }
    if (bpb.numfats == 0 || bpb.fatsz16 == 0 || bpb.rootentcnt == 0) {
        return E_NOTRDY;
    }

    fat_start_lba  = bpb.reservedsecs;
    fatsz          = bpb.fatsz16;
    root_start_lba = fat_start_lba + (uint32_t)bpb.numfats * fatsz;
    root_sectors   = ((uint32_t)bpb.rootentcnt * 32u + Q9_BLK_SIZE - 1u) / Q9_BLK_SIZE;
    root_bytes     = (uint32_t)bpb.rootentcnt * 32u;
    data_start_lba = root_start_lba + root_sectors;
    sec_per_clus   = bpb.secperclus;

    mounted = 1;
    return 0;
}

//────────────────────────────────────────────────────────────────────────────────────────────────
// EOF fat16.c                                                                            Ver. 1.00
//────────────────────────────────────────────────────────────────────────────────────────────────
