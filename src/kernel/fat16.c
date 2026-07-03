//════════════════════════════════════════════════════════════════════════════════════════════════
// File:   fat16.c                                                                        Ver. 1.10
// Owner:  AF
// Desc.:  Q9 FAT16-File-Manager, lesend + schreibend (Phase 3.3/3.4). Boot-Sektor (BPB) parsen,
//         Root-Directory und Unterverzeichnisse durchsuchen (8.3- und LFN-Namen lesen, NUR 8.3
//         beim Anlegen — LFN-Schreiben ist Ideenspeicher, ARBEITSPLAN.md), Cluster-Ketten der FAT
//         folgen/allozieren/freigeben, Datei-Inhalt lesen/schreiben. Nur Superfloppy (kein MBR,
//         Boot-Sektor bei LBA 0 des Block-Device). Zugriff auf den Datentraeger AUSSCHLIESSLICH
//         ueber q9_hal_blk_read/write (Q9_BLK_SIZE-Byte-Bloecke) — kein direkter Treiberzugriff,
//         kein malloc (ein statischer 512-Byte-Sektor-Puffer fuer alle Zugriffe, da Q9 nicht
//         nebenlaeufig ist).
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
// 26-07-04│ 1.10 │ 3.4: I$Create/I$MakDir/I$Delete + I$Write echt implementiert. FAT-      │ CF
//         │      │ Ketten allozieren/freigeben (freie Cluster linear ab 2 gesucht, beide   │
//         │      │ FAT-Kopien synchron gehalten). Directory-Eintrag im Elternverzeichnis   │
//         │      │ (Root oder Unterverzeichnis) finden/anlegen/loeschen. Nur 8.3-Namen bei │
//         │      │ neuen Dateien/Verzeichnissen (LFN-Schreiben -> Ideenspeicher)           │
//═════════╧══════╧════════════════════════════════════════════════════════════════════════╧══════

#include "../hal/q9_hal.h"
#include "device.h"
#include "fat16.h"
#include "name.h"
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
//║ FILE CONTEXT (q9_path_t.fmctx, 24 Byte — device.h Q9_FMCTX_SIZE)                              ║
//╚══════════════════════════════════════════════════════════════════════════════════════════════╝

#pragma pack(push, 1)
typedef struct fat16_ctx {
    uint32_t start_cluster;                             /* 0 = Root-Directory-Pseudo-Datei         */
    uint32_t cur_cluster;                               /* Cluster, in dem "pos" gerade liegt      */
    uint32_t pos;                                        /* aktuelle Byte-Position in der Datei     */
    uint32_t size;                                       /* Dateigroesse (Root-Dir: Bytes gesamt)   */
    uint32_t dir_start;                                 /* 3.4: Elternverzeichnis (0 = Root) —     */
                                                        /*   noetig, um nach I$Write den Directory- */
                                                        /*   Eintrag (Groesse/Start-Cluster)        */
                                                        /*   zurueckzuschreiben. 0xFFFFFFFF = kein  */
                                                        /*   Dirent zum Zurueckschreiben (Root-Dir- */
                                                        /*   Pseudo-Datei selbst, nur lesend)       */
    uint32_t dir_index;                                 /* 3.4: Slot-Index im Elternverzeichnis     */
} fat16_ctx_t;                                          /* 24 Byte gesamt                          */
#pragma pack(pop)

//╔══════════════════════════════════════════════════════════════════════════════════════════════╗
//║ MOUNT STATE (ein Datentraeger reicht fuer Q9 — statisch, kein malloc)                        ║
//╚══════════════════════════════════════════════════════════════════════════════════════════════╝

static int      mounted        = 0;
static uint32_t fat_start_lba;                          /* erste FAT-Kopie                        */
static uint32_t fatsz;                                  /* Sektoren pro FAT-Kopie                  */
static uint32_t numfats;                                /* Anzahl FAT-Kopien (3.4: alle werden     */
                                                        /*   synchron gehalten, Standard = 2)      */
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
// Function: fat_set
// Desc.:    Schreibt einen FAT16-Eintrag ("clus" -> "val", z.B. Kettenglied oder FAT16_FREE/EOC)
//           in BEIDE FAT-Kopien (Standard-Konvention, wichtig fuer Interop mit macOS/newfs_msdos:
//           ein Treiber, der nur die erste Kopie liest, sieht sonst inkonsistente Daten, sobald
//           er irgendwann die zweite Kopie zur Reparatur heranzieht). 0 = ok, E$NotRdy bei I/O-
//           Fehler.
//────────────────────────────────────────────────────────────────────────────────────────────────
static int fat_set(uint32_t clus, uint16_t val)
{
    uint32_t byteoff = clus * 2u;
    uint32_t off     = byteoff % Q9_BLK_SIZE;

    for (uint32_t f = 0; f < numfats; f++) {
        uint32_t lba = fat_start_lba + f * fatsz + byteoff / Q9_BLK_SIZE;
        if (q9_hal_blk_read(lba, secbuf) != 0) {
            return E_NOTRDY;
        }
        secbuf[off]     = (uint8_t)(val & 0xffu);
        secbuf[off + 1] = (uint8_t)(val >> 8);
        if (q9_hal_blk_write(lba, secbuf) != 0) {
            return E_NOTRDY;
        }
    }
    return 0;
}

//────────────────────────────────────────────────────────────────────────────────────────────────
// Function: fat_alloc
// Desc.:    Sucht linear ab Cluster 2 den ersten freien Cluster (FAT-Eintrag == FAT16_FREE),
//           markiert ihn als Kettenende ($FFFF — im gueltigen EOC-Bereich $FFF8-$FFFF, s.o.) in
//           beiden FAT-Kopien und liefert seine Nummer. 0 = Datentraeger voll/Fehler.
//────────────────────────────────────────────────────────────────────────────────────────────────
static uint32_t fat_alloc(void)
{
    /* Obere Grenze: wie viele Cluster passen rein rechnerisch in "fatsz" FAT-Sektoren
       (256 16-Bit-Eintraege je Sektor) — die eigentliche Datentraegergroesse (totsec/BPB) wird
       beim Mount nicht gemerkt, diese Grenze ist konservativ genug (nie kleiner als die echte
       Cluster-Anzahl, da die FAT immer mindestens so viele Eintraege hat wie es Cluster gibt). */
    uint32_t lastclus = fatsz * (Q9_BLK_SIZE / 2u);

    for (uint32_t c = 2; c < lastclus; c++) {
        uint32_t v = fat_next(c);
        if (v == 0xFFFFFFFFu) {
            return 0;                                   /* I/O-Fehler                              */
        }
        if (v == FAT16_FREE) {
            if (fat_set(c, 0xFFFFu) != 0) {              /* explizites EOC, s. Aufgabenstellung     */
                return 0;
            }
            return c;
        }
    }
    return 0;                                           /* Datentraeger voll                        */
}

//────────────────────────────────────────────────────────────────────────────────────────────────
// Function: fat_free_chain
// Desc.:    Gibt die komplette Cluster-Kette ab "start" frei (alle Glieder auf FAT16_FREE, beide
//           FAT-Kopien). Bricht bei Kettenende/Fehler sauber ab.
//────────────────────────────────────────────────────────────────────────────────────────────────
static void fat_free_chain(uint32_t start)
{
    uint32_t c = start;

    while (c >= 2u && c < FAT16_EOC_MIN) {
        uint32_t next = fat_next(c);
        fat_set(c, FAT16_FREE);
        if (next == 0xFFFFFFFFu) {
            break;
        }
        c = next;
    }
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

static int dir_find_idx(uint32_t dirstart, const char *name, uint32_t len, fat16_dirent_t *out,
                         uint32_t *outidx);
static int dir_find(uint32_t dirstart, const char *name, uint32_t len, fat16_dirent_t *out);
static void split_first(const char *path, uint32_t len, const char **elem, uint32_t *elemlen,
                         const char **restp, uint32_t *restlen);

//────────────────────────────────────────────────────────────────────────────────────────────────
// Function: write_dir_slot
// Desc.:    Schreibt "de" (32 Byte) in den "index"-ten Slot eines Directory ("dirstart", 0 =
//           Root). Gegenstueck zu read_dir_slot — anders als beim Lesen wird bei einem
//           Unterverzeichnis NICHT automatisch die Kette verlaengert, wenn "index" ausserhalb
//           der bisherigen Kette liegt (das macht dir_alloc_slot() vorher explizit). 0 = ok,
//           E$NotRdy bei I/O-Fehler, E$PARAM wenn der Slot ausserhalb des Directory liegt.
//────────────────────────────────────────────────────────────────────────────────────────────────
static int write_dir_slot(uint32_t dirstart, uint32_t index, const fat16_dirent_t *de)
{
    uint32_t entsperclus  = clus_bytes() / 32u;
    uint32_t entspersec   = Q9_BLK_SIZE / 32u;
    uint32_t lba;
    uint32_t slot_in_sec;

    if (dirstart == 0) {
        uint32_t entsperroot = root_bytes / 32u;
        if (index >= entsperroot) {
            return E_PARAM;
        }
        lba = root_start_lba + (index * 32u) / Q9_BLK_SIZE;
        slot_in_sec = index % entspersec;
    } else {
        uint32_t clusidx = index / entsperclus;
        uint32_t inclus  = index % entsperclus;
        uint32_t clus    = cluster_at_offset(dirstart, clusidx);
        if (clus == 0) {
            return E_PARAM;
        }
        lba = clus_to_lba(clus) + (inclus * 32u) / Q9_BLK_SIZE;
        slot_in_sec = inclus % entspersec;
    }
    if (q9_hal_blk_read(lba, secbuf) != 0) {
        return E_NOTRDY;
    }
    for (uint32_t i = 0; i < 32; i++) {
        secbuf[slot_in_sec * 32u + i] = ((const uint8_t *)de)[i];
    }
    if (q9_hal_blk_write(lba, secbuf) != 0) {
        return E_NOTRDY;
    }
    return 0;
}

//────────────────────────────────────────────────────────────────────────────────────────────────
// Function: dir_alloc_slot
// Desc.:    Sucht im Directory "dirstart" (0 = Root) den ersten freien Slot (DIRENT_FREE oder
//           DIRENT_END) und liefert dessen Index in "*outidx". Bei einem Unterverzeichnis wird
//           die Cluster-Kette bei Bedarf um einen neuen (genullten) Cluster verlaengert, wenn
//           kein freier Slot mehr in der bisherigen Kette liegt (Root-Directory ist ein fester
//           Bereich fester Groesse und kann NICHT wachsen — Standard-FAT16-Einschraenkung).
//           0 = ok, E$NotRdy bei Datentraeger voll/I-O-Fehler.
//────────────────────────────────────────────────────────────────────────────────────────────────
static int dir_alloc_slot(uint32_t dirstart, uint32_t *outidx)
{
    fat16_dirent_t de;
    uint32_t       idx;

    for (idx = 0; ; idx++) {
        int used = read_dir_slot(dirstart, idx, &de);
        if (!used) {
            /* read_dir_slot liefert 0 sowohl bei einem echten DIRENT_END-Slot (der Slot IST      */
            /* lesbar, nur "leer") als auch, wenn die Kette/der Root-Bereich zu Ende ist. Wir      */
            /* muessen unterscheiden: liegt idx noch innerhalb des Bereichs, ist es ein freier      */
            /* Slot; sonst muss (bei einem Unterverzeichnis) ein neuer Cluster angehaengt werden.   */
            uint32_t entsperclus = clus_bytes() / 32u;
            if (dirstart == 0) {
                uint32_t entsperroot = root_bytes / 32u;
                if (idx < entsperroot) {
                    *outidx = idx;                       /* echter DIRENT_END-Slot im Root-Bereich  */
                    return 0;
                }
                return E_NOTRDY;                         /* Root-Directory voll, kann nicht wachsen */
            } else {
                uint32_t clusidx = idx / entsperclus;
                uint32_t clus    = cluster_at_offset(dirstart, clusidx);
                if (clus != 0) {
                    *outidx = idx;                       /* echter DIRENT_END-Slot in der Kette      */
                    return 0;
                }
                /* Kette zu kurz -> letzten Cluster suchen und einen neuen anhaengen */
                {
                    uint32_t last = dirstart;
                    uint32_t n;
                    for (;;) {
                        n = fat_next(last);
                        if (n == 0xFFFFFFFFu) {
                            return E_NOTRDY;
                        }
                        if (n >= FAT16_EOC_MIN) {
                            break;
                        }
                        last = n;
                    }
                    {
                        uint32_t newc = fat_alloc();
                        if (newc == 0) {
                            return E_NOTRDY;              /* Datentraeger voll                       */
                        }
                        if (fat_set(last, (uint16_t)newc) != 0) {
                            return E_NOTRDY;
                        }
                        /* neuen Cluster mit Nullbytes initialisieren (DIRENT_END ueberall, $00) */
                        for (uint32_t i = 0; i < Q9_BLK_SIZE; i++) {
                            secbuf[i] = 0;
                        }
                        for (uint32_t s = 0; s < sec_per_clus; s++) {
                            if (q9_hal_blk_write(clus_to_lba(newc) + s, secbuf) != 0) {
                                return E_NOTRDY;
                            }
                        }
                        *outidx = idx;                    /* erster Slot im frisch angehaengten Cluster */
                        return 0;
                    }
                }
            }
        }
        if ((uint8_t)de.name[0] == DIRENT_FREE) {
            *outidx = idx;                                /* wiederverwendbarer geloeschter Slot      */
            return 0;
        }
    }
}

//────────────────────────────────────────────────────────────────────────────────────────────────
// Function: make_83name
// Desc.:    Validiert "name" (Laenge "len") als reinen 8.3-Namen und baut das 11-Byte-Directory-
//           Feld ("NAME    EXT", grossgeschrieben, space-gepolstert) in "out11". Erlaubt sind die
//           OS-9-Namenszeichen minus Punkt/Slash als Sonderzeichen (Basis <=8, Erweiterung <=3,
//           genau ein Punkt als Trenner erlaubt). E$BPNam bei ungueltigem/zu langem Namen (z.B.
//           LFN-pflichtige Namen — LFN-SCHREIBEN ist bewusst nicht implementiert, Ideenspeicher).
//────────────────────────────────────────────────────────────────────────────────────────────────
static int make_83name(const char *name, uint32_t len, uint8_t *out11)
{
    uint32_t baselen = 0, extlen = 0;
    uint32_t dot = len;
    int      i;

    if (len == 0 || len > 12) {                          /* "12345678.123" max. */
        return E_BPNAM;
    }
    for (i = 0; i < (int)len; i++) {
        if (name[i] == '.') {
            if (dot != len) {                             /* zweiter Punkt -> kein gueltiger 8.3-Name */
                return E_BPNAM;
            }
            dot = (uint32_t)i;
        }
    }
    baselen = (dot == len) ? len : dot;
    extlen  = (dot == len) ? 0 : (len - dot - 1u);
    if (baselen == 0 || baselen > 8 || extlen > 3) {
        return E_BPNAM;
    }
    for (uint32_t k = 0; k < 11; k++) {
        out11[k] = ' ';
    }
    for (uint32_t k = 0; k < baselen; k++) {
        char c = name[k];
        if (c >= 'a' && c <= 'z') {
            c = (char)(c - 'a' + 'A');
        }
        if (!q9_name_is_char(c) || c == '.') {
            return E_BPNAM;
        }
        out11[k] = (uint8_t)c;
    }
    for (uint32_t k = 0; k < extlen; k++) {
        char c = name[dot + 1u + k];
        if (c >= 'a' && c <= 'z') {
            c = (char)(c - 'a' + 'A');
        }
        if (!q9_name_is_char(c) || c == '.') {
            return E_BPNAM;
        }
        out11[8 + k] = (uint8_t)c;
    }
    return 0;
}

//────────────────────────────────────────────────────────────────────────────────────────────────
// Function: resolve_parent
// Desc.:    Loest "restpath" bis zum VORLETZTEN Pfadelement auf (das Elternverzeichnis) und
//           liefert dessen Start-Cluster (0 = Root) in "*parent_dirstart" sowie Start/Laenge des
//           letzten Elements (des anzulegenden/zu loeschenden Namens) in "*lastname"/"*lastlen".
//           E$PNNF, wenn ein Zwischenelement fehlt oder keine Datei ist.
//────────────────────────────────────────────────────────────────────────────────────────────────
static int resolve_parent(const char *restpath, uint32_t len, uint32_t *parent_dirstart,
                           const char **lastname, uint32_t *lastlen)
{
    uint32_t    dirstart = 0;
    const char *cur = restpath;
    uint32_t    curlen = len;

    if (len == 0) {
        return E_BPNAM;                                  /* kein Name angegeben (Wurzel selbst)     */
    }
    for (;;) {
        const char     *elem;
        uint32_t        elemlen;
        const char     *rest;
        uint32_t        restlen;
        fat16_dirent_t  de;
        int             err;

        split_first(cur, curlen, &elem, &elemlen, &rest, &restlen);
        if (elemlen == 0) {
            return E_PNNF;
        }
        if (!rest) {                                     /* letztes Element: das ist der Name       */
            *parent_dirstart = dirstart;
            *lastname        = elem;
            *lastlen         = elemlen;
            return 0;
        }
        err = dir_find(dirstart, elem, elemlen, &de);
        if (err != 0) {
            return err;
        }
        if (!(de.attr & ATTR_DIRECTORY)) {
            return E_PNNF;
        }
        dirstart = de.fstcluslo;
        cur      = rest;
        curlen   = restlen;
    }
}

//────────────────────────────────────────────────────────────────────────────────────────────────
// Function: dir_find_idx
// Desc.:    Wie dir_find(), liefert zusaetzlich den Slot-Index des gefundenen 8.3-/LFN-Alias-
//           Eintrags in "*outidx" (3.4, noetig fuer I$Write, um den Directory-Eintrag spaeter
//           gezielt zu ueberschreiben — LFN-Vorlaufeintraege zaehlen NICHT als Fundstelle, das
//           Alias-Slot ist massgeblich, denn NUR dort stehen Cluster/Groesse). "outidx" darf NULL
//           sein (dann identisch zu dir_find()).
//────────────────────────────────────────────────────────────────────────────────────────────────
static int dir_find_idx(uint32_t dirstart, const char *name, uint32_t len, fat16_dirent_t *out,
                         uint32_t *outidx)
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
            if (outidx) {
                *outidx = idx;
            }
            return 0;
        }
        {
            char name83[13];
            uint32_t n83 = dirent_name83(&de, name83);
            if (n83 == len && ci_eq(name, len, name83)) {
                *out = de;
                if (outidx) {
                    *outidx = idx;
                }
                return 0;
            }
        }
        have_lfn = 0;
    }
    return E_PNNF;
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
    return dir_find_idx(dirstart, name, len, out, 0);
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
//           is_root zeigt den Root-Sonderfall an), sonst E$PNNF. Liefert zusaetzlich (3.4) das
//           Elternverzeichnis ("*out_dirstart") und den Slot-Index des gefundenen Eintrags
//           ("*out_diridx") — ungenutzt bei is_root (Root-Directory hat kein Eltern-Slot).
//────────────────────────────────────────────────────────────────────────────────────────────────
static int resolve_path(const char *restpath, uint32_t len, fat16_dirent_t *out, int *is_root,
                         uint32_t *out_dirstart, uint32_t *out_diridx)
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
        uint32_t    idx;
        int         err;

        split_first(cur, curlen, &elem, &elemlen, &rest, &restlen);
        if (elemlen == 0) {
            return E_PNNF;
        }
        err = dir_find_idx(dirstart, elem, elemlen, out, &idx);
        if (err != 0) {
            return err;
        }
        if (!rest) {                                     /* letztes Element gefunden                */
            *out_dirstart = dirstart;
            *out_diridx   = idx;
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
//           Modus ist nicht gefordert). E$PNNF, wenn der Pfad nicht existiert. Seit 3.4 auch im
//           Schreib-/Update-Modus erlaubt (eine bereits mit I$Create angelegte oder bestehende
//           Datei zum Weiterschreiben/Ueberschreiben oeffnen) — I$Write beginnt dann an Position 0
//           und ueberschreibt, Anhaengen braucht vorher ein I$Seek ans Dateiende (OS-9-Semantik).
//────────────────────────────────────────────────────────────────────────────────────────────────
static int fat16_open(q9_dev_t *dev, q9_path_t *p, const char *restpath, uint32_t len, uint8_t mode)
{
    fat16_dirent_t de;
    fat16_ctx_t    ctx;
    int            is_root;
    int            err;
    uint32_t       pdirstart = 0, pdiridx = 0;

    (void)dev; (void)mode;
    if (!mounted) {
        return E_NOTRDY;
    }
    err = resolve_path(restpath, len, &de, &is_root, &pdirstart, &pdiridx);
    if (err != 0) {
        return err;
    }
    if (is_root) {
        ctx.start_cluster = 0;
        ctx.size          = root_bytes;
        ctx.dir_start      = 0xFFFFFFFFu;                 /* Root-Dir-Pseudo-Datei: kein Dirent      */
        ctx.dir_index      = 0;
    } else {
        ctx.start_cluster = de.fstcluslo;
        ctx.size          = de.filesize;
        if (de.attr & ATTR_DIRECTORY) {                   /* Directory-Groesse ist im Eintrag 0 —    */
            ctx.size = 0xFFFFFFFFu;                       /* Kette bis EOC folgen (I$Read-Grenzwert) */
        }
        ctx.dir_start = pdirstart;
        ctx.dir_index = pdiridx;
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
    } else if (ctx.dir_start != 0xFFFFFFFFu && pos != 0) {
        return E_PARAM;                                  /* frisch angelegte, noch leere Datei (3.4) */
    }                                                     /*   ohne Cluster -> nur Position 0 gueltig */
    for (uint32_t i = 0; i < sizeof(ctx); i++) {
        p->fmctx[i] = ((const uint8_t *)&ctx)[i];
    }
    return 0;
}

//────────────────────────────────────────────────────────────────────────────────────────────────
// Function: fat16_create
// Desc.:    I$Create-Unterbau (3.4): legt eine neue, leere Datei im Elternverzeichnis von
//           "restpath" an. Nur 8.3-Namen (make_83name) — kein LFN-Schreiben. Existiert der Name
//           bereits, wird das als Fehler behandelt (E$BPNam — Q9 hat kein Truncate-Flag im
//           mode-Byte, ein I$Open im Update-Modus reicht zum Ueberschreiben). Legt den
//           Directory-Eintrag mit Cluster=0/Groesse=0 an (der erste I$Write alloziert den ersten
//           Cluster lazy) und befuellt p->fmctx wie fat16_open.
//────────────────────────────────────────────────────────────────────────────────────────────────
static int fat16_create(q9_dev_t *dev, q9_path_t *p, const char *restpath, uint32_t len, uint8_t mode)
{
    uint32_t       parent;
    const char    *lastname;
    uint32_t       lastlen;
    uint8_t        name11[11];
    fat16_dirent_t existing;
    fat16_dirent_t de;
    fat16_ctx_t    ctx;
    uint32_t       slot;
    int            err;

    (void)mode;
    (void)dev;
    if (!mounted) {
        return E_NOTRDY;
    }
    err = resolve_parent(restpath, len, &parent, &lastname, &lastlen);
    if (err != 0) {
        return err;
    }
    err = make_83name(lastname, lastlen, name11);
    if (err != 0) {
        return err;
    }
    if (dir_find(parent, lastname, lastlen, &existing) == 0) {
        return E_BPNAM;                                  /* Name bereits vergeben (kein Truncate-Flag*/
    }                                                     /*   im mode-Byte, s. Funktionskopf oben)   */
    err = dir_alloc_slot(parent, &slot);
    if (err != 0) {
        return err;
    }
    for (uint32_t i = 0; i < sizeof(de); i++) {
        ((uint8_t *)&de)[i] = 0;
    }
    for (uint32_t i = 0; i < 11; i++) {
        ((uint8_t *)&de)[i] = name11[i];
    }
    de.attr = ATTR_ARCHIVE;
    err = write_dir_slot(parent, slot, &de);
    if (err != 0) {
        return err;
    }

    ctx.start_cluster = 0;
    ctx.cur_cluster    = 0;
    ctx.pos            = 0;
    ctx.size           = 0;
    ctx.dir_start      = parent;
    ctx.dir_index      = slot;
    for (uint32_t i = 0; i < sizeof(ctx); i++) {
        p->fmctx[i] = ((const uint8_t *)&ctx)[i];
    }
    return 0;
}

//────────────────────────────────────────────────────────────────────────────────────────────────
// Function: fat16_makdir
// Desc.:    I$MakDir-Unterbau (3.4): legt ein neues Unterverzeichnis an — alloziert einen
//           Cluster, initialisiert ihn mit "." (zeigt auf sich selbst) und ".." (zeigt auf das
//           Elternverzeichnis, 0 = Root — FAT16-Konvention: ".." im Root-Unterverzeichnis hat
//           Cluster 0, obwohl das Root-Directory selbst gar keinen Cluster besitzt), und legt den
//           Directory-Eintrag (ATTR_DIRECTORY) im Elternverzeichnis an. Nur 8.3-Namen.
//────────────────────────────────────────────────────────────────────────────────────────────────
static int fat16_makdir(q9_dev_t *dev, const char *restpath, uint32_t len)
{
    uint32_t       parent;
    const char    *lastname;
    uint32_t       lastlen;
    uint8_t        name11[11];
    fat16_dirent_t existing;
    fat16_dirent_t de;
    uint32_t       slot;
    uint32_t       newclus;
    int            err;

    (void)dev;
    if (!mounted) {
        return E_NOTRDY;
    }
    err = resolve_parent(restpath, len, &parent, &lastname, &lastlen);
    if (err != 0) {
        return err;
    }
    err = make_83name(lastname, lastlen, name11);
    if (err != 0) {
        return err;
    }
    if (dir_find(parent, lastname, lastlen, &existing) == 0) {
        return E_BPNAM;                                  /* Name bereits vergeben (kein Truncate-Flag*/
    }                                                     /*   im mode-Byte, s. Funktionskopf oben)   */
    newclus = fat_alloc();
    if (newclus == 0) {
        return E_NOTRDY;                                 /* Datentraeger voll                        */
    }

    /* Neuen Cluster nullen, dann "." und ".." als erste zwei Eintraege schreiben (Standard-FAT-
       Konvention, wichtig fuer Interop mit macOS/anderen FAT-Treibern). */
    for (uint32_t i = 0; i < Q9_BLK_SIZE; i++) {
        secbuf[i] = 0;
    }
    for (uint32_t s = 0; s < sec_per_clus; s++) {
        if (q9_hal_blk_write(clus_to_lba(newclus) + s, secbuf) != 0) {
            fat_free_chain(newclus);
            return E_NOTRDY;
        }
    }
    for (uint32_t i = 0; i < sizeof(de); i++) {
        ((uint8_t *)&de)[i] = 0;
    }
    de.name[0] = '.'; de.name[1] = ' '; de.name[2] = ' '; de.name[3] = ' ';
    de.name[4] = ' '; de.name[5] = ' '; de.name[6] = ' '; de.name[7] = ' ';
    de.ext[0]  = ' '; de.ext[1]  = ' '; de.ext[2]  = ' ';
    de.attr      = ATTR_DIRECTORY;
    de.fstcluslo = (uint16_t)newclus;
    if (write_dir_slot(newclus, 0, &de) != 0) {
        fat_free_chain(newclus);
        return E_NOTRDY;
    }
    de.name[1] = '.';
    de.fstcluslo = (uint16_t)parent;                      /* ".." zeigt aufs Elternverzeichnis        */
    if (write_dir_slot(newclus, 1, &de) != 0) {           /* (Root -> 0, FAT16-Konvention)            */
        fat_free_chain(newclus);
        return E_NOTRDY;
    }

    err = dir_alloc_slot(parent, &slot);
    if (err != 0) {
        fat_free_chain(newclus);
        return err;
    }
    for (uint32_t i = 0; i < sizeof(de); i++) {
        ((uint8_t *)&de)[i] = 0;
    }
    for (uint32_t i = 0; i < 11; i++) {
        ((uint8_t *)&de)[i] = name11[i];
    }
    de.attr      = ATTR_DIRECTORY;
    de.fstcluslo = (uint16_t)newclus;
    de.filesize  = 0;                                     /* Verzeichnisse haben Groesse 0 im Eintrag */
    err = write_dir_slot(parent, slot, &de);
    if (err != 0) {
        fat_free_chain(newclus);
        return err;
    }
    return 0;
}

//────────────────────────────────────────────────────────────────────────────────────────────────
// Function: fat16_remove
// Desc.:    I$Delete-Unterbau (3.4): loescht Datei ODER Unterverzeichnis unter "restpath". Gibt
//           die komplette Cluster-Kette frei (beide FAT-Kopien, fat_free_chain) und markiert den
//           Directory-Eintrag als geloescht (erstes Namensbyte = DIRENT_FREE). Bewusst KEINE
//           Pruefung auf "Verzeichnis nicht leer" (Q9 hat noch kein rekursives Loeschen/keine
//           Schutzsemantik dafuer definiert — Ideenspeicher, falls das mal noetig wird).
//────────────────────────────────────────────────────────────────────────────────────────────────
static int fat16_remove(q9_dev_t *dev, const char *restpath, uint32_t len)
{
    uint32_t       parent;
    const char    *lastname;
    uint32_t       lastlen;
    fat16_dirent_t de;
    uint32_t       slot;
    int            err;

    (void)dev;
    if (!mounted) {
        return E_NOTRDY;
    }
    err = resolve_parent(restpath, len, &parent, &lastname, &lastlen);
    if (err != 0) {
        return err;
    }
    err = dir_find_idx(parent, lastname, lastlen, &de, &slot);
    if (err != 0) {
        return err;
    }
    if (de.fstcluslo != 0) {
        fat_free_chain(de.fstcluslo);
    }
    de.name[0] = DIRENT_FREE;
    return write_dir_slot(parent, slot, &de);
}

//────────────────────────────────────────────────────────────────────────────────────────────────
// Function: fat16_write
// Desc.:    I$Write-Unterbau (3.4): schreibt ab der aktuellen Position (fmctx) bis zu *n Bytes.
//           Alloziert bei Bedarf neue Cluster ans Kettenende (ueber fat_alloc/fat_set, BEIDE
//           FAT-Kopien), aktualisiert danach Groesse (und bei der allerersten Allozierung den
//           Start-Cluster) im Directory-Eintrag. *n = tatsaechlich geschriebene Bytes.
//────────────────────────────────────────────────────────────────────────────────────────────────
static int fat16_write(q9_dev_t *dev, q9_path_t *p, const uint8_t *buf, uint32_t *n)
{
    fat16_ctx_t ctx;
    uint32_t    want = *n;
    uint32_t    got  = 0;
    int         grew = 0;                                /* Groesse/Cluster im Dirent aktualisieren? */

    (void)dev;
    if (!mounted) {
        return E_NOTRDY;
    }
    for (uint32_t i = 0; i < sizeof(ctx); i++) {
        ((uint8_t *)&ctx)[i] = p->fmctx[i];
    }
    if (ctx.dir_start == 0xFFFFFFFFu) {                   /* Root-Directory-Pseudo-Datei: kein Ziel   */
        return E_UNKSVC;                                  /*   fuer I$Write (kein normaler Dateiinhalt)*/
    }
    while (got < want) {
        uint32_t off_in_clus = ctx.pos % clus_bytes();
        uint32_t lba;
        uint32_t off_in_sec;
        uint32_t avail;

        if (ctx.cur_cluster == 0) {                       /* Datei hat noch KEINEN Cluster (Create)   */
            uint32_t c = fat_alloc();
            if (c == 0) {
                *n = got;
                return got > 0 ? 0 : E_NOTRDY;            /* Datentraeger voll                        */
            }
            ctx.start_cluster = c;
            ctx.cur_cluster   = c;
            grew = 1;
        } else if (off_in_clus == 0 && ctx.pos != 0) {    /* Cluster-Grenze -> naechster/neuer Cluster */
            uint32_t nextc = fat_next(ctx.cur_cluster);
            if (nextc == 0xFFFFFFFFu) {
                *n = got;
                return got > 0 ? 0 : E_NOTRDY;
            }
            if (nextc >= FAT16_EOC_MIN || nextc < 2u) {   /* Kettenende -> neuen Cluster anhaengen     */
                uint32_t newc = fat_alloc();
                if (newc == 0) {
                    *n = got;
                    return got > 0 ? 0 : E_NOTRDY;
                }
                if (fat_set(ctx.cur_cluster, (uint16_t)newc) != 0) {
                    *n = got;
                    return got > 0 ? 0 : E_NOTRDY;
                }
                nextc = newc;
            }
            ctx.cur_cluster = nextc;
        }

        lba = clus_to_lba(ctx.cur_cluster) + off_in_clus / Q9_BLK_SIZE;
        off_in_sec = off_in_clus % Q9_BLK_SIZE;
        avail = Q9_BLK_SIZE - off_in_sec;
        if (clus_bytes() - off_in_clus < avail) {
            avail = clus_bytes() - off_in_clus;
        }
        if (avail > want - got) {
            avail = want - got;
        }
        /* Sektor lesen (Read-Modify-Write, falls nur ein Teil des Sektors geschrieben wird),
           dann die neuen Bytes einmischen und zurueckschreiben. */
        if (q9_hal_blk_read(lba, secbuf) != 0) {
            *n = got;
            return got > 0 ? 0 : E_NOTRDY;
        }
        for (uint32_t i = 0; i < avail; i++) {
            secbuf[off_in_sec + i] = buf[got + i];
        }
        if (q9_hal_blk_write(lba, secbuf) != 0) {
            *n = got;
            return got > 0 ? 0 : E_NOTRDY;
        }
        got     += avail;
        ctx.pos += avail;
    }
    if (ctx.pos > ctx.size) {
        ctx.size = ctx.pos;
        grew = 1;
    }
    for (uint32_t i = 0; i < sizeof(ctx); i++) {
        p->fmctx[i] = ((const uint8_t *)&ctx)[i];
    }
    *n = got;
    if (grew) {
        /* Directory-Eintrag (Start-Cluster/Groesse) im Elternverzeichnis nachziehen — die
           Fundstelle (dir_start/dir_index) wurde beim I$Open/I$Create in fmctx abgelegt, genau
           fuer diesen Zweck (device.h/vfs.h-Kommentar zum erweiterten Q9_FMCTX_SIZE). */
        fat16_dirent_t de;
        if (read_dir_slot(ctx.dir_start, ctx.dir_index, &de)) {
            de.fstcluslo = (uint16_t)ctx.start_cluster;
            de.filesize  = ctx.size;
            write_dir_slot(ctx.dir_start, ctx.dir_index, &de); /* Fehler hier wuerde die Datei     */
        }                                                      /* inhaltlich trotzdem korrekt lassen,*/
    }                                                          /* nur die Metadaten waeren veraltet   */
    return 0;
}

const q9_fm_t q9_fat16_fm = {
    "FAT16",
    fat16_open,
    fat16_create,
    fat16_makdir,
    fat16_remove,
    fat16_read,
    fat16_write,
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
    numfats        = bpb.numfats;
    root_start_lba = fat_start_lba + (uint32_t)bpb.numfats * fatsz;
    root_sectors   = ((uint32_t)bpb.rootentcnt * 32u + Q9_BLK_SIZE - 1u) / Q9_BLK_SIZE;
    root_bytes     = (uint32_t)bpb.rootentcnt * 32u;
    data_start_lba = root_start_lba + root_sectors;
    sec_per_clus   = bpb.secperclus;

    mounted = 1;
    return 0;
}

//────────────────────────────────────────────────────────────────────────────────────────────────
// EOF fat16.c                                                                            Ver. 1.10
//────────────────────────────────────────────────────────────────────────────────────────────────
