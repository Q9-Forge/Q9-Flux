/* dhf_host_fs.c - Host Filesystem implementation with path confinement
 */

#include "dhf_host_fs.h"
#include "dhf_proto.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <limits.h>
#include <time.h>
#include <sys/time.h>
#include <sys/statvfs.h>

/* OS-9-Attributbyte (FD_ATT-Bitlage: Bit0-2 Besitzer r/w/e, Bit3-5 oeffentlich r/w/e)
   -> Unix-Rechte. Einzige Stelle fuer diese Abbildung (Create, MakDir, SS_Attr);
   Gegenrichtung in dhf_host_fs_getfd_at(). 2026-09-26: vorher gaben Create/MakDir das
   OS-9-Byte UNGEWANDELT als Unix-Modus weiter ($3F -> 077: Besitzer ohne jedes Recht,
   jedes Create im frisch angelegten Verzeichnis scheiterte mit E$FNA -- gefunden vom
   Regressionstest test/dhfregr_68k.a). Verzeichnisse: ein Host-Verzeichnis ohne x ist
   nicht betretbar, OS-9 durchsucht Verzeichnisse aber per Lesen -- deshalb folgt dort
   das x-Recht dem r-Recht. 0 = keine Angabe -> Voreinstellung (Datei rw-r--r--,
   Verzeichnis rwxr-xr-x). */
static mode_t os9_attr_to_mode(unsigned attr, int is_dir) {
    mode_t m = 0;
    attr &= 0x3F;
    if (attr == 0) return is_dir ? 0755 : 0644;
    if (attr & 0x01) m |= S_IRUSR;
    if (attr & 0x02) m |= S_IWUSR;
    if (attr & 0x04) m |= S_IXUSR;
    if (attr & 0x08) m |= S_IROTH;
    if (attr & 0x10) m |= S_IWOTH;
    if (attr & 0x20) m |= S_IXOTH;
    if (is_dir) {
        if (m & S_IRUSR) m |= S_IXUSR;
        if (m & S_IROTH) m |= S_IXOTH;
    }
    return m;
}

/* Host-errno -> OS-9-Fehlercode (s. dhf_proto.h: der Wert geht unveraendert an den
   I$-Aufrufer). 2026-09-26 um die Faelle erweitert, die vorher im Sammelcode landeten. */
static uint8_t errno_to_dhf(int err) {
    switch (err) {
        case 0:            return DHF_ERR_OK;
        case ENOENT:       return DHF_ERR_NOT_FOUND;
        case EACCES:
        case EPERM:        return DHF_ERR_NO_PERMISSION;
        case EEXIST:       return DHF_ERR_FILE_EXISTS;
        case EBADF:        return DHF_ERR_BAD_PATH;
        case ENOSPC:
        case EDQUOT:
        case EFBIG:        return DHF_ERR_DISK_FULL;
        case EISDIR:       return DHF_ERR_IS_DIR;
        case ENOTDIR:      return DHF_ERR_NOT_DIR;
        case ENOTEMPTY:    return DHF_ERR_DIR_NOT_EMPTY;
        case EROFS:        return DHF_ERR_WRITE_PROT;
        case EMFILE:
        case ENFILE:       return DHF_ERR_PATH_FULL;
        case ENAMETOOLONG:
        case ELOOP:        return DHF_ERR_BAD_NAME;
        case EBUSY:
        case ETXTBSY:      return DHF_ERR_SHARING;
        default:           return DHF_ERR_ERROR;
    }
}

/* Resolve path relative to cwd and basepath; enforce basepath confinement */
static int resolve_confined_path(dhf_host_fs_t *fs, const char *rel_or_abs, char *out_buf, size_t out_len) {
    char combined[DHF_PATH_MAX * 2];

    /* 2026-09-26: Pfad zuerst selbst normalisieren (Komponente fuer Komponente), dann erst
       realpath(): "." entfaellt, ".." geht eine Ebene hoch ("..." zwei usw.), aber NIE ueber die Wurzel des
       Laufwerks hinaus -- wie RBF, wo "/d0/.." wieder "/d0" ist. Vorher fuehrte "./.." in der
       Wurzel (so fragt bashs getwd nach dem Elternverzeichnis) aus dem Basispfad heraus.
       Absolute OS-9-Pfade "/<geraet>/rest": der Geraetename gehoert nicht zum Dateipfad
       (Basispfad = Wurzel). Relative Pfade gelten ab cwd. */
    char rel[DHF_PATH_MAX * 2];
    if (!rel_or_abs || !rel_or_abs[0]) {
        snprintf(rel, sizeof(rel), "%s", fs->cwd);
    } else if (rel_or_abs[0] == '/') {
        const char *rest = strchr(rel_or_abs + 1, '/');
        snprintf(rel, sizeof(rel), "%s", rest ? rest + 1 : "");
    } else {
        snprintf(rel, sizeof(rel), "%s%s%s", fs->cwd, fs->cwd[0] ? "/" : "", rel_or_abs);
    }
    char norm[DHF_PATH_MAX * 2];
    size_t nlen = 0;
    norm[0] = '\0';
    for (char *tok = strtok(rel, "/"); tok; tok = strtok(NULL, "/")) {
        if (strcmp(tok, ".") == 0) continue;
        /* OS-9: ".." eine Ebene hoch, "..." zwei, "...." drei usw. (n Punkte = n-1 Ebenen) --
           bashs getwd bildet genau so "./..", "./...", "./....". Nie ueber die Wurzel. */
        size_t dots = strspn(tok, ".");
        if (dots >= 2 && tok[dots] == '\0') {
            for (size_t up = 1; up < dots; up++) {
                char *sl = strrchr(norm, '/');
                if (sl) { *sl = '\0'; nlen = (size_t)(sl - norm); }
                else    { norm[0] = '\0'; nlen = 0; break; }
            }
            continue;
        }
        size_t tl = strlen(tok);
        if (nlen + tl + 2 >= sizeof(norm)) return -1;
        if (nlen) norm[nlen++] = '/';
        memcpy(norm + nlen, tok, tl + 1);
        nlen += tl;
    }
    if (nlen) snprintf(combined, sizeof(combined), "%s/%s", fs->basepath, norm);
    else      snprintf(combined, sizeof(combined), "%s", fs->basepath);

    /* Clean path / remove multiple slashes and /./ */
    char resolved[PATH_MAX];
    /* For non-existing targets (create), realpath might fail on full path,
       so resolve directory component */
    char *last_slash = strrchr(combined, '/');
    if (last_slash && last_slash != combined) {
        char dir_part[DHF_PATH_MAX];
        size_t dlen = (size_t)(last_slash - combined);
        if (dlen >= sizeof(dir_part)) dlen = sizeof(dir_part) - 1;
        strncpy(dir_part, combined, dlen);
        dir_part[dlen] = '\0';

        if (realpath(dir_part, resolved) != NULL) {
            snprintf(out_buf, out_len, "%s/%s", resolved, last_slash + 1);
        } else {
            strncpy(out_buf, combined, out_len - 1);
            out_buf[out_len - 1] = '\0';
        }
    } else {
        strncpy(out_buf, combined, out_len - 1);
        out_buf[out_len - 1] = '\0';
    }

    /* Confinement check: out_buf must start with fs->basepath */
    size_t base_len = strlen(fs->basepath);
    if (strncmp(out_buf, fs->basepath, base_len) != 0) {
        return -1; /* Escape detected */
    }
    /* 2026-09-26: reiner Praefix-Vergleich reicht nicht -- "/d1/../OS9SYS.hda" ergibt bei
       Basis ".../cf_images/OS9SYS" den Pfad ".../cf_images/OS9SYS.hda", der textuell mit der
       Basis BEGINNT, aber daneben liegt (dort liegen die Boot-Images!). Hinter der Basis muss
       deshalb ein '/' folgen oder der Pfad dort enden. */
    if (out_buf[base_len] != '\0' && out_buf[base_len] != '/') {
        return -1; /* Escape detected (Geschwister mit gleichem Namensanfang) */
    }
    return 0;
}

/* 2026-09-26: statt eines vom Simulator selbst vergebenen Handles kann der Aufrufer (der
 * Q9-DHF-Manager) einen FESTEN Index vorgeben -- die OS-9-Pfadnummer selbst, die IOMan
 * ohnehin schon fuer die Lebensdauer des Pfades verwaltet. Damit muss der Manager kein
 * eigenes Handle mehr ueber Aufrufe hinweg merken (s. Q9-OS/Q9-DHF-68k/STATUS.md); nur
 * OPEN/CREATE nutzen das, alloc_handle() (scannend) bleibt fuer OPENDIR unveraendert. Ein
 * bereits belegter Slot an diesem Index wird als verwaist behandelt (z.B. Pfad ohne
 * ordnungsgemaesses CLOSE wiederverwendet) und sauber geschlossen, statt einen Fehler zu
 * liefern -- robuster fuer Tests/Entwicklung als ein hartes E$-Fehlschlagen. */
static int dir_real_next(DIR *d, char *name, size_t max);

static void dir_snapshot_free(dhf_host_fs_t *fs, int h) {
    for (uint32_t i = 0; i < fs->handles[h].dir_count; i++) free(fs->handles[h].dir_names[i]);
    free(fs->handles[h].dir_names);
    fs->handles[h].dir_names = NULL;
    fs->handles[h].dir_count = 0;
}

/* Platztabelle des Verzeichnisses path holen/anlegen und mit dem Host abgleichen: nicht mehr
   vorhandene Namen werden freie Plaetze, neue Host-Namen fuellen den ersten freien Platz oder
   werden angehaengt. So bleiben Positionen ueber mehrere Opens stabil -- "deldir" oeffnet ein
   Verzeichnis nach dem Abarbeiten eines Unterverzeichnisses NEU und seekt auf die alte
   Position; ruecken die Eintraege dabei vor, uebersprang es jeden folgenden. */
static int dircache_sync(dhf_host_fs_t *fs, const char *path) {
    int slot = -1;
    for (int i = 0; i < DHF_DIRCACHE_SLOTS; i++) {
        if (fs->dircache[i].path[0] && strcmp(fs->dircache[i].path, path) == 0) { slot = i; break; }
    }
    if (slot < 0) {                            /* aeltesten Platz neu belegen */
        slot = 0;
        for (int i = 1; i < DHF_DIRCACHE_SLOTS; i++)
            if (fs->dircache[i].used < fs->dircache[slot].used) slot = i;
        for (uint32_t k = 0; k < fs->dircache[slot].count; k++) free(fs->dircache[slot].names[k]);
        free(fs->dircache[slot].names);
        fs->dircache[slot].names = NULL;
        fs->dircache[slot].count = 0;
        strncpy(fs->dircache[slot].path, path, DHF_PATH_MAX - 1);
        fs->dircache[slot].path[DHF_PATH_MAX - 1] = '\0';
    }
    fs->dircache[slot].used = ++fs->dircache_clock;

    /* aktuelle Host-Namen einlesen */
    char **host = NULL; uint32_t nh = 0, cap = 0;
    DIR *d = opendir(path);
    if (d) {
        char name[256];
        while (dir_real_next(d, name, sizeof(name))) {
            if (nh == cap) {
                cap = cap ? cap * 2 : 32;
                char **n = realloc(host, cap * sizeof(char *));
                if (!n) break;
                host = n;
            }
            host[nh++] = strdup(name);
        }
        closedir(d);
    }
    /* verschwundene Namen -> freie Plaetze */
    for (uint32_t k = 0; k < fs->dircache[slot].count; k++) {
        char *nm = fs->dircache[slot].names[k];
        if (!nm) continue;
        int found = 0;
        for (uint32_t j = 0; j < nh; j++) if (host[j] && strcmp(host[j], nm) == 0) { found = 1; free(host[j]); host[j] = NULL; break; }
        if (!found) { free(nm); fs->dircache[slot].names[k] = NULL; }
    }
    /* neue Namen -> erster freier Platz, sonst anhaengen */
    for (uint32_t j = 0; j < nh; j++) {
        if (!host[j]) continue;
        uint32_t k;
        for (k = 0; k < fs->dircache[slot].count; k++) if (!fs->dircache[slot].names[k]) break;
        if (k == fs->dircache[slot].count) {
            char **n = realloc(fs->dircache[slot].names, (k + 1) * sizeof(char *));
            if (!n) { free(host[j]); continue; }
            fs->dircache[slot].names = n;
            fs->dircache[slot].count++;
        }
        fs->dircache[slot].names[k] = host[j];
    }
    free(host);
    return slot;
}

/* Momentaufnahme der Platztabelle beim Open (s. dircache_sync/dir_entry_name) */
static void dir_snapshot(dhf_host_fs_t *fs, int h) {
    dir_snapshot_free(fs, h);
    int c = dircache_sync(fs, fs->handles[h].path);
    uint32_t n = fs->dircache[c].count;
    if (!n) return;
    fs->handles[h].dir_names = calloc(n, sizeof(char *));
    if (!fs->handles[h].dir_names) return;
    for (uint32_t k = 0; k < n; k++)
        fs->handles[h].dir_names[k] = fs->dircache[c].names[k] ? strdup(fs->dircache[c].names[k]) : NULL;
    fs->handles[h].dir_count = n;
}

/* Groesse der virtuellen Verzeichnisdatei eines offenen Handles (Momentaufnahme) */
static uint32_t dir_handle_size(dhf_host_fs_t *fs, int h) {
    return (fs->handles[h].dir_count + 2) * 32u;
}

static int alloc_handle_at(dhf_host_fs_t *fs, int idx, int is_dir, const char *path) {
    if (idx < 0 || idx >= DHF_MAX_HANDLES) {
        return -1;
    }
    if (fs->handles[idx].in_use) {
        dir_snapshot_free(fs, idx);
        if (fs->handles[idx].is_dir && fs->handles[idx].dir) {
            closedir(fs->handles[idx].dir);
        } else if (!fs->handles[idx].is_dir && fs->handles[idx].fd >= 0) {
            close(fs->handles[idx].fd);
        }
    }
    fs->handles[idx].in_use = 1;
    fs->handles[idx].is_dir = is_dir;
    fs->handles[idx].fd = -1;
    fs->handles[idx].dir = NULL;
    fs->handles[idx].dir_pos = 0;
    fs->handles[idx].is_raw = 0;
    fs->handles[idx].dir_names = NULL;
    fs->handles[idx].dir_count = 0;
    strncpy(fs->handles[idx].path, path ? path : "", sizeof(fs->handles[idx].path) - 1);
    return idx;
}

static int alloc_handle(dhf_host_fs_t *fs, int is_dir, const char *path) {
    for (int i = 0; i < DHF_MAX_HANDLES; i++) {
        if (!fs->handles[i].in_use) {
            fs->handles[i].in_use = 1;
            fs->handles[i].is_dir = is_dir;
            fs->handles[i].fd = -1;
            fs->handles[i].dir = NULL;
            strncpy(fs->handles[i].path, path ? path : "", sizeof(fs->handles[i].path) - 1);
            return i;
        }
    }
    return -1;
}

int dhf_host_fs_init(dhf_host_fs_t *fs, const char *basepath) {
    if (!fs) return -1;
    for (uint32_t i = 0; i < fs->lsn_cnt; i++) free(fs->lsn_tab[i]);   /* erneutes iniz */
    free(fs->lsn_tab);
    free(fs->lsn_hash);
    memset(fs, 0, sizeof(*fs));

    char real_base[PATH_MAX];
    if (basepath && basepath[0]) {
        if (realpath(basepath, real_base) != NULL) {
            strncpy(fs->basepath, real_base, sizeof(fs->basepath) - 1);
        } else {
            strncpy(fs->basepath, basepath, sizeof(fs->basepath) - 1);
        }
    } else {
        getcwd(fs->basepath, sizeof(fs->basepath) - 1);
    }
    fs->cwd[0] = '\0'; /* root inside basepath */
    return 0;
}

void dhf_host_fs_cleanup(dhf_host_fs_t *fs) {
    if (!fs) return;
    for (int i = 0; i < DHF_MAX_HANDLES; i++) {
        if (fs->handles[i].in_use) {
            dir_snapshot_free(fs, i);
            if (fs->handles[i].is_dir && fs->handles[i].dir) {
                closedir(fs->handles[i].dir);
            } else if (!fs->handles[i].is_dir && fs->handles[i].fd >= 0) {
                close(fs->handles[i].fd);
            }
            fs->handles[i].in_use = 0;
        }
    }
}

int dhf_host_fs_open(dhf_host_fs_t *fs, const char *path, int flags, uint8_t *status) {
    char target[DHF_PATH_MAX];
    if (resolve_confined_path(fs, path, target, sizeof(target)) != 0) {
        if (status) *status = DHF_ERR_BAD_NAME;
        return -1;
    }

    int oflags = 0;
    if ((flags & (DHF_MODE_READ | DHF_MODE_WRITE)) == (DHF_MODE_READ | DHF_MODE_WRITE)) {
        oflags = O_RDWR;
    } else if (flags & DHF_MODE_WRITE) {
        oflags = O_WRONLY;
    } else {
        oflags = O_RDONLY;
    }

    if (flags & 0x10) oflags |= O_APPEND;   /* Append_: Schreiben immer ans Dateiende */
    int fd = open(target, oflags);
    if (fd < 0) {
        if (status) *status = errno_to_dhf(errno);
        return -1;
    }

    int h = alloc_handle(fs, 0, target);
    if (h < 0) {
        close(fd);
        if (status) *status = DHF_ERR_PATH_FULL;
        return -1;
    }

    fs->handles[h].fd = fd;
    if (status) *status = DHF_ERR_OK;
    return h;
}

int dhf_host_fs_create(dhf_host_fs_t *fs, const char *path, int flags, int mode, uint8_t *status) {
    if (fs && fs->readonly) {                      /* Laufwerk nur lesbar */
        if (status) *status = DHF_ERR_WRITE_PROT;
        return -1;
    }
    char target[DHF_PATH_MAX];
    if (resolve_confined_path(fs, path, target, sizeof(target)) != 0) {
        if (status) *status = DHF_ERR_BAD_NAME;
        return -1;
    }

    /* 2026-09-26: O_EXCL statt O_TRUNC -- OS-9 I$Create auf eine bestehende Datei ist
       E$CEF, sie darf NICHT still geleert werden (Regressionstest dhfregr Fall 55). */
    int oflags = O_CREAT | O_EXCL;
    if ((flags & (DHF_MODE_READ | DHF_MODE_WRITE)) == (DHF_MODE_READ | DHF_MODE_WRITE)) {
        oflags |= O_RDWR;
    } else if (flags & DHF_MODE_WRITE) {
        oflags |= O_WRONLY;
    } else {
        oflags |= O_RDWR;
    }

    mode_t pmode = os9_attr_to_mode((unsigned)mode, 0);
    if (flags & 0x10) oflags |= O_APPEND;   /* Append_: Schreiben immer ans Dateiende */
    int fd = open(target, oflags, pmode);
    if (fd < 0) {
        if (status) *status = errno_to_dhf(errno);
        return -1;
    }
    fchmod(fd, pmode);          /* exakt die OS-9-Attribute, unabhaengig von der Host-umask */

    int h = alloc_handle(fs, 0, target);
    if (h < 0) {
        close(fd);
        if (status) *status = DHF_ERR_PATH_FULL;
        return -1;
    }

    fs->handles[h].fd = fd;
    if (status) *status = DHF_ERR_OK;
    return h;
}

/* 2026-09-26: wie dhf_host_fs_open, aber mit vom Aufrufer vorgegebenem Index (s.
 * alloc_handle_at-Kommentar) statt automatischer Vergabe -- fuer den Q9-DHF-Manager, der
 * die OS-9-Pfadnummer direkt als Index nutzt und dadurch selbst kein Handle mehr merken
 * muss. */
/* 2026-09-26: virtuelles Rohgeraet "/<geraet>@" (RBF: das ganze Medium ab LSN 0). DHF hat
   keine Sektoren -- "free" braucht aber genau das: LSN 0 (Identifikationssektor, rbf.h
   struct sector0) und die Belegungs-Bitmap ab dd_maplsn. Beides wird aus statvfs() des
   Basispfads berechnet: Sektorgroesse (dd_lsnsize, 256..32768) und Clustergroesse (dd_bit) so
   gewaehlt, dass Gesamtsektoren in 24 Bit und die Bitmap in 65535 Byte passen (Kapazitaet
   notfalls gekappt, freier Platz exakt bis 256 GB); die ersten
   Cluster gelten als belegt (Host-Belegung), der Rest als frei. Nur lesbar (E$WP). */
typedef struct { uint32_t lsnsize, tot, bit, map, clusters, used; } dhf_raw_geo_t;

static void raw_geometry(dhf_host_fs_t *fs, dhf_raw_geo_t *g) {
    struct statvfs sv;
    uint64_t total = 0, freeb = 0;
    if (statvfs(fs->basepath, &sv) == 0) {
        total = (uint64_t)sv.f_blocks * sv.f_frsize;
        freeb = (uint64_t)sv.f_bavail * sv.f_frsize;
    }
    /* hoechstens 16384: "free" liest dd_lsnsize vorzeichenbehaftet ($8000 = negativ -> faellt
       auf 256 zurueck). 16384 x 24 Bit = 256 GB; eine groessere Host-Platte wird in der
       Kapazitaet gekappt, der FREIE Platz bleibt bis 256 GB exakt. */
    uint32_t ls = 256;
    while (ls < 16384 && total / ls > 0xFFFFFFu) ls <<= 1;
    uint64_t tot = total / ls;
    if (tot > 0xFFFFFFu) tot = 0xFFFFFFu;
    if (tot < 16) tot = 16;
    uint64_t fr = freeb / ls;
    if (fr > tot) fr = tot;
    uint32_t bit = 1;
    while (bit < 32768 && (tot + bit - 1) / bit > 65535u * 8u) bit <<= 1;
    g->lsnsize = ls;
    g->tot = (uint32_t)tot;
    g->bit = bit;
    g->clusters = (uint32_t)((tot + bit - 1) / bit);
    g->map = (g->clusters + 7) / 8;
    uint64_t usedsec = tot - fr;
    g->used = (uint32_t)((usedsec + bit - 1) / bit);
    if (g->used < 1) g->used = 1;              /* LSN 0 + Bitmap sind immer belegt */
}

/* Byte an Position pos des virtuellen Rohgeraets */
static unsigned char raw_byte(dhf_host_fs_t *fs, const dhf_raw_geo_t *g, const unsigned char *lsn0, uint64_t pos) {
    if (pos < 256) return lsn0[pos];
    uint64_t mstart = (uint64_t)g->lsnsize;     /* dd_maplsn = 1 */
    if (pos >= mstart && pos < mstart + g->map) {
        uint64_t k = pos - mstart;
        unsigned char b = 0;
        for (int i = 0; i < 8; i++) {
            uint64_t c = k * 8 + (uint64_t)i;   /* Bit 7 = erster Cluster des Bytes */
            if (c < g->used || c >= g->clusters) b |= (unsigned char)(0x80 >> i);
        }
        return b;
    }
    (void)fs;
    return 0;
}

static void raw_lsn0(dhf_host_fs_t *fs, const dhf_raw_geo_t *g, unsigned char *p) {
    memset(p, 0, 256);
    p[0] = (unsigned char)(g->tot >> 16); p[1] = (unsigned char)(g->tot >> 8); p[2] = (unsigned char)g->tot;
    p[4] = (unsigned char)(g->map >> 8);  p[5] = (unsigned char)g->map;           /* dd_map */
    p[6] = (unsigned char)(g->bit >> 8);  p[7] = (unsigned char)g->bit;           /* dd_bit */
    uint32_t dir = 1 + (g->map + g->lsnsize - 1) / g->lsnsize;                      /* dd_dir */
    p[8] = (unsigned char)(dir >> 16); p[9] = (unsigned char)(dir >> 8); p[10] = (unsigned char)dir;
    p[13] = 0xFF;                                                                   /* dd_att */
    p[14] = 0x44; p[15] = 0x48;                                                     /* dd_dsk "DH" */
    p[16] = 0x06;                                                                   /* dd_fmt */
    struct stat st;
    if (stat(fs->basepath, &st) == 0) {                                             /* dd_date */
        struct tm tmv; localtime_r(&st.st_ctime, &tmv);
        p[26] = (unsigned char)tmv.tm_year; p[27] = (unsigned char)(tmv.tm_mon + 1);
        p[28] = (unsigned char)tmv.tm_mday; p[29] = (unsigned char)tmv.tm_hour;
        p[30] = (unsigned char)tmv.tm_min;
    }
    const char *nm = strrchr(fs->basepath, '/');                                    /* dd_name */
    nm = nm ? nm + 1 : fs->basepath;
    size_t n = strlen(nm); if (n > 31) n = 31;
    memcpy(p + 31, nm, n); if (n) p[31 + n - 1] |= 0x80;
    p[63] = 1;                                                                      /* dd_opt: DT_RBF */
    memcpy(p + 96, "Cruz", 4);                                                      /* dd_sync */
    p[103] = 1;                                                                     /* dd_maplsn = 1 */
    p[104] = (unsigned char)(g->lsnsize >> 8); p[105] = (unsigned char)g->lsnsize;  /* dd_lsnsize */
    p[107] = 1;                                                                     /* dd_versid */
}

static ssize_t raw_read(dhf_host_fs_t *fs, int h, void *buf, size_t count, uint8_t *status) {
    dhf_raw_geo_t g; raw_geometry(fs, &g);
    unsigned char lsn0[256]; raw_lsn0(fs, &g, lsn0);
    uint64_t size = (uint64_t)g.tot * g.lsnsize;
    uint64_t pos = fs->handles[h].dir_pos;
    if (pos >= size || count == 0) { if (status) *status = DHF_ERR_EOF; return 0; }
    if (pos + count > size) count = (size_t)(size - pos);
    unsigned char *out = (unsigned char *)buf;
    for (size_t i = 0; i < count; i++) out[i] = raw_byte(fs, &g, lsn0, pos + i);
    fs->handles[h].dir_pos = (uint32_t)(pos + count);
    if (status) *status = DHF_ERR_OK;
    return (ssize_t)count;
}

/* "/<geraet>@..." -> Rohgeraet? (erste Pfadkomponente endet auf '@') */
static int path_is_raw(const char *path) {
    if (!path || path[0] != '/') return 0;
    const char *e = strchr(path + 1, '/');
    size_t n = e ? (size_t)(e - path - 1) : strlen(path + 1);
    return n > 0 && path[n] == '@';
}

/* Wurde bei diesem Verzeichnis das Dir-Bit per SS_Attr entfernt? (s. dhf_host_fs_unlink) */
static int deldir_marked(dhf_host_fs_t *fs, const char *target, int consume) {
    char real[PATH_MAX];
    const char *cmp = realpath(target, real) ? real : target;
    for (uint32_t i = 0; i < DHF_DELDIR_SLOTS; i++) {
        if (fs->deldir_ok[i][0] && strcmp(fs->deldir_ok[i], cmp) == 0) {
            if (consume) fs->deldir_ok[i][0] = '\0';
            return 1;
        }
    }
    return 0;
}

int dhf_host_fs_open_at(dhf_host_fs_t *fs, int idx, const char *path, int flags, uint8_t *status) {
    if (fs && fs->readonly && (flags & DHF_MODE_WRITE)) {   /* nur lesbares Laufwerk */
        if (status) *status = DHF_ERR_WRITE_PROT;
        return -1;
    }
    if (path_is_raw(path)) {
        if (flags & DHF_MODE_WRITE) {                 /* Rohgeraet nur lesbar */
            if (status) *status = DHF_ERR_WRITE_PROT;
            return -1;
        }
        int h = alloc_handle_at(fs, idx, 0, fs->basepath);
        if (h < 0) { if (status) *status = DHF_ERR_BAD_PATH; return -1; }
        fs->handles[h].is_raw = 1;
        if (status) *status = DHF_ERR_OK;
        return h;
    }
    char target[DHF_PATH_MAX];
    if (resolve_confined_path(fs, path, target, sizeof(target)) != 0) {
        if (status) *status = DHF_ERR_BAD_NAME;
        return -1;
    }

    /* 2026-09-26: OS-9 kennt kein eigenes I$OpenDir -- ein Verzeichnis wird per ganz
       normalem I$Open geoeffnet (wie jede andere Datei) und per I$Read gelesen ("OS-9
       Technical Manual" Kap. 7, "Directory File Format"). Ein Host-Verzeichnis kann nicht
       mit open()+read() gelesen werden (read() auf einem Directory-fd liefert EISDIR) --
       deshalb hier per stat() erkennen und stattdessen opendir() benutzen; dhf_host_fs_read()
       liefert fuer is_dir-Handles die RBF-Verzeichniseintraege (s. dort). */
    struct stat pst;
    if (stat(target, &pst) == 0 && S_ISDIR(pst.st_mode)) {
        /* 2026-09-26: wie RBF -- ein Verzeichnis laesst sich nur MIT Verzeichnis-Bit ($80) im
           Zugriffsmodus oeffnen, sonst E$FNA. Die echte "dir"-Utility verlaesst sich darauf:
           sie oeffnet ihr Argument erst OHNE das Bit und erkennt ein Verzeichnis gerade am
           E$FNA; gelang der Open (wie bisher hier), hielt sie "/d0" fuer eine Datei und
           druckte nur den Namen. */
        /* Ausnahme wie RBF: ist das Dir-Bit per SS_Attr schon entfernt, gilt der Eintrag
           nicht mehr als Verzeichnis und darf ohne $80 geoeffnet werden ("deldir"). */
        if (!(flags & DHF_MODE_DIR) && !deldir_marked(fs, target, 0)) {
            if (status) *status = DHF_ERR_IS_DIR;
            return -1;
        }
        DIR *d = opendir(target);
        if (!d) {
            if (status) *status = errno_to_dhf(errno);
            return -1;
        }
        /* aufgeloester Pfad als Schluessel: "/d0/dd" und "." (cwd dd) sind dasselbe
           Verzeichnis und muessen dieselbe Platztabelle teilen (s. dircache_sync) */
        char dreal[PATH_MAX];
        int h = alloc_handle_at(fs, idx, 1, realpath(target, dreal) ? dreal : target);
        if (h < 0) {
            closedir(d);
            if (status) *status = DHF_ERR_BAD_PATH;
            return -1;
        }
        fs->handles[h].dir = d;
        dir_snapshot(fs, h);
        if (status) *status = DHF_ERR_OK;
        return h;
    }

    /* 2026-09-26: wie RBF -- eine Datei MIT Verzeichnis-Bit ($80) oeffnen ist E$FNA. "copy"
       prueft so, ob die Quelle ein Verzeichnis ist; gelang der Open, hielt es jede DHF-Datei
       fuer ein Verzeichnis ("you must specify -z or files to copy"). */
    if (flags & DHF_MODE_DIR) {
        if (status) *status = DHF_ERR_IS_DIR;           /* E$FNA */
        return -1;
    }
    int oflags = 0;
    if ((flags & (DHF_MODE_READ | DHF_MODE_WRITE)) == (DHF_MODE_READ | DHF_MODE_WRITE)) {
        oflags = O_RDWR;
    } else if (flags & DHF_MODE_WRITE) {
        oflags = O_WRONLY;
    } else {
        oflags = O_RDONLY;
    }

    if (flags & 0x10) oflags |= O_APPEND;   /* Append_: Schreiben immer ans Dateiende */
    int fd = open(target, oflags);
    if (fd < 0) {
        if (status) *status = errno_to_dhf(errno);
        return -1;
    }

    int h = alloc_handle_at(fs, idx, 0, target);
    if (h < 0) {
        close(fd);
        if (status) *status = DHF_ERR_BAD_PATH;
        return -1;
    }

    fs->handles[h].fd = fd;
    if (status) *status = DHF_ERR_OK;
    return h;
}

/* 2026-09-26: wie dhf_host_fs_create, aber mit vom Aufrufer vorgegebenem Index, s.o. */
int dhf_host_fs_create_at(dhf_host_fs_t *fs, int idx, const char *path, int flags, int mode, uint8_t *status) {
    if (fs && fs->readonly) {                      /* Laufwerk nur lesbar */
        if (status) *status = DHF_ERR_WRITE_PROT;
        return -1;
    }
    char target[DHF_PATH_MAX];
    if (resolve_confined_path(fs, path, target, sizeof(target)) != 0) {
        if (status) *status = DHF_ERR_BAD_NAME;
        return -1;
    }

    /* 2026-09-26: O_EXCL statt O_TRUNC -- OS-9 I$Create auf eine bestehende Datei ist
       E$CEF, sie darf NICHT still geleert werden (Regressionstest dhfregr Fall 55). */
    int oflags = O_CREAT | O_EXCL;
    if ((flags & (DHF_MODE_READ | DHF_MODE_WRITE)) == (DHF_MODE_READ | DHF_MODE_WRITE)) {
        oflags |= O_RDWR;
    } else if (flags & DHF_MODE_WRITE) {
        oflags |= O_WRONLY;
    } else {
        oflags |= O_RDWR;
    }

    mode_t pmode = os9_attr_to_mode((unsigned)mode, 0);
    if (flags & 0x10) oflags |= O_APPEND;   /* Append_: Schreiben immer ans Dateiende */
    int fd = open(target, oflags, pmode);
    if (fd < 0) {
        if (status) *status = errno_to_dhf(errno);
        return -1;
    }
    fchmod(fd, pmode);          /* exakt die OS-9-Attribute, unabhaengig von der Host-umask */

    int h = alloc_handle_at(fs, idx, 0, target);
    if (h < 0) {
        close(fd);
        if (status) *status = DHF_ERR_BAD_PATH;
        return -1;
    }

    fs->handles[h].fd = fd;
    if (status) *status = DHF_ERR_OK;
    return h;
}

int dhf_host_fs_close(dhf_host_fs_t *fs, int handle, uint8_t *status) {
    if (!fs || handle < 0 || handle >= DHF_MAX_HANDLES || !fs->handles[handle].in_use) {
        if (status) *status = DHF_ERR_BAD_PATH;
        return -1;
    }

    if (fs->handles[handle].is_dir) {
        dir_snapshot_free(fs, handle);
        if (fs->handles[handle].dir) closedir(fs->handles[handle].dir);
    } else {
        if (fs->handles[handle].fd >= 0) close(fs->handles[handle].fd);
    }
    fs->handles[handle].in_use = 0;
    if (status) *status = DHF_ERR_OK;
    return 0;
}

/* 2026-09-26: RBF-Verzeichnisformat ("OS-9 Technical Manual" Kap. 7, "Directory File
   Format", lokal unter /Volumes/SSD1TB/Documents/OS-9 v2.4 Technical Reference Manual):
   32-Byte-Eintraege je Datei -- Byte 0-27 Dateiname (High-Bit auf dem LETZTEN Zeichen
   gesetzt, erstes Byte 0 = geloescht/unbenutzt), Byte 28 unbenutzt/muss 0 sein, Byte 29-31
   3-Byte "FD"-Zeiger (LSN des File-Descriptor-Sektors auf echten RBF-Medien -- DHF hat kein
   Medium mit LSNs, bleibt hier bewusst 0; "dir" braucht fuer eine einfache Namensliste nur
   den Namen). */
#define DHF_DIRENT_SIZE 32

/* 2026-09-26: Ein Host-Verzeichnis erscheint dem Gast als virtuelle RBF-Verzeichnisdatei aus
   32-Byte-Eintraegen ("OS-9 Technical Manual" Kap. 7) mit eigener Byteposition dir_pos --
   frei positionierbar per I$Seek und in beliebigen Stuecken lesbar, wie auf RBF. Eintrag 0
   ist immer "..", Eintrag 1 immer "." (RBF-Reihenfolge), danach die echten Host-Eintraege (readdir()-
   Reihenfolge, "." und ".." dort uebersprungen). Vorher konnte ein Verzeichnis nur
   sequenziell in ganzen Eintraegen gelesen werden und I$Seek/SS_Pos/SS_EOF lehnten
   Verzeichnis-Handles ab -- die echte "dir"-Utility seekt nach dem Lesen von "."/".." auf
   Position 64 und brach mit 'can't seek past "." and ".."' ab. */
/* 2026-09-26: Host-Namen, die OS-9 nicht darstellen kann, erscheinen nicht im Verzeichnis:
   macOS-Metadaten (.DS_Store, AppleDouble "._*"), Namen ueber 28 Zeichen (RBF-Grenze; ein
   abgeschnittener Name waere nicht mehr oeffnbar) und Namen mit Bytes >= $80 (UTF-8, z.B.
   Umlaute -- Bit 7 markiert im RBF-Eintrag das Namensende) oder Steuerzeichen. */
static int os9_name_ok(const char *n) {
    size_t len = strlen(n);
    if (len == 0 || len > 28) return 0;
    if (strcmp(n, ".DS_Store") == 0 || strncmp(n, "._", 2) == 0) return 0;
    for (size_t i = 0; i < len; i++) {
        unsigned char c = (unsigned char)n[i];
        if (c < 0x20 || c >= 0x7F) return 0;
    }
    return 1;
}

static int dir_real_next(DIR *d, char *name, size_t max) {
    struct dirent *de;
    while ((de = readdir(d)) != NULL) {
        if (strcmp(de->d_name, ".") == 0 || strcmp(de->d_name, "..") == 0) continue;
        if (!os9_name_ok(de->d_name)) continue;
        strncpy(name, de->d_name, max - 1);
        name[max - 1] = '\0';
        return 1;
    }
    return 0;
}

/* Name des Eintrags idx holen (0=".." 1="." ab 2 Host): 1 = Eintrag, 2 = freier Platz
   (inzwischen geloescht), 0 = hinter dem letzten Eintrag. 2026-09-26: aus der Momentaufnahme
   beim Open statt frisch per readdir() -- sonst rueckten nach einem Delete alle folgenden
   Eintraege eine Position vor und ein Aufrufer, der beim Lesen loescht ("deldir"),
   uebersprang jeden zweiten. Auf RBF bleibt ein geloeschter Eintrag als freier Platz stehen. */
static int dir_entry_name(dhf_host_fs_t *fs, int handle, uint32_t idx, char *name, size_t max) {
    /* RBF-Reihenfolge: Eintrag 0 = "..", Eintrag 1 = "." (2026-09-26 korrigiert -- dsave
       vergleicht die Sektornummern von Eintrag 0 und 1, um die Wurzel zu erkennen, und sucht
       danach Eintrag 1 im Elternverzeichnis; mit vertauschter Reihenfolge suchte es dort das
       Elternverzeichnis selbst und brach mit 'can't read directory ".."' ab). */
    if (idx == 0) { strncpy(name, "..", max); return 1; }
    if (idx == 1) { strncpy(name, ".", max); return 1; }
    uint32_t i = idx - 2;
    if (i >= fs->handles[handle].dir_count) return 0;
    if (!fs->handles[handle].dir_names[i]) return 2;       /* freier Platz */
    strncpy(name, fs->handles[handle].dir_names[i], max - 1);
    name[max - 1] = '\0';
    char full[DHF_PATH_MAX * 2];
    struct stat st;
    snprintf(full, sizeof(full), "%s/%s", fs->handles[handle].path, name);
    return (lstat(full, &st) == 0) ? 1 : 2;
}

/* Groesse der virtuellen Verzeichnisdatei in Byte (Eintraege inkl. "."/".." mal 32) */
static uint32_t dir_virtual_size(const char *path) {
    uint32_t n = 2;
    DIR *d = opendir(path);
    if (d) {
        char name[256];
        while (dir_real_next(d, name, sizeof(name))) n++;
        closedir(d);
    }
    return n * DHF_DIRENT_SIZE;
}

static uint32_t lsn_strhash(const char *p) {
    uint32_t h = 2166136261u;
    while (*p) { h ^= (unsigned char)*p++; h *= 16777619u; }
    return h;
}

/* Pseudo-Sektornummer fuer einen Host-Pfad (1..0xFFFFFF, nie 0); derselbe Pfad behaelt seine
   Nummer fuer die ganze Laufzeit (bis zum naechsten iniz). */
static uint32_t lsn_for_path(dhf_host_fs_t *fs, const char *path) {
    if (fs->lsn_hcap == 0 || (fs->lsn_cnt + 1) * 2 > fs->lsn_hcap) {        /* Hash vergroessern */
        uint32_t nc = fs->lsn_hcap ? fs->lsn_hcap * 2 : 4096;
        uint32_t *nh = calloc(nc, sizeof(uint32_t));
        if (!nh) return 0;
        for (uint32_t i = 0; i < fs->lsn_cnt; i++) {
            uint32_t k = lsn_strhash(fs->lsn_tab[i]) & (nc - 1);
            while (nh[k]) k = (k + 1) & (nc - 1);
            nh[k] = i + 1;
        }
        free(fs->lsn_hash);
        fs->lsn_hash = nh;
        fs->lsn_hcap = nc;
    }
    uint32_t k = lsn_strhash(path) & (fs->lsn_hcap - 1);
    while (fs->lsn_hash[k]) {
        if (strcmp(fs->lsn_tab[fs->lsn_hash[k] - 1], path) == 0) return fs->lsn_hash[k];
        k = (k + 1) & (fs->lsn_hcap - 1);
    }
    if (fs->lsn_cnt >= 0xFFFFFEu) return 0;                                  /* 24 Bit voll */
    if (fs->lsn_cnt == fs->lsn_cap) {
        uint32_t nc = fs->lsn_cap ? fs->lsn_cap * 2 : 1024;
        char **nt = realloc(fs->lsn_tab, nc * sizeof(char *));
        if (!nt) return 0;
        fs->lsn_tab = nt;
        fs->lsn_cap = nc;
    }
    fs->lsn_tab[fs->lsn_cnt] = strdup(path);
    fs->lsn_hash[k] = ++fs->lsn_cnt;
    return fs->lsn_cnt;
}

static const char *lsn_to_path(dhf_host_fs_t *fs, uint32_t lsn) {
    return (lsn >= 1 && lsn <= fs->lsn_cnt) ? fs->lsn_tab[lsn - 1] : NULL;
}

/* 2026-09-26: aktuelles Verzeichnis des AUFRUFENDEN Prozesses setzen (vor jeder Anfrage aus
   SH_SEQ, das der Manager aus P$DIO des Prozesses fuellt). Vorher gab es EIN cwd je Laufwerk
   fuer alle Prozesse -- "dsave ... | mshell" verstellte sich gegenseitig das Verzeichnis
   ("dsave: can't chd to b"). 0/unbekannt -> Wurzel. */
void dhf_host_fs_set_cwd_lsn(dhf_host_fs_t *fs, uint32_t lsn) {
    const char *p = lsn_to_path(fs, lsn);
    size_t bl = strlen(fs->basepath);
    if (p && strncmp(p, fs->basepath, bl) == 0 && (p[bl] == '/' || p[bl] == '\0')) {
        const char *rel = p + bl;
        if (*rel == '/') rel++;
        snprintf(fs->cwd, sizeof(fs->cwd), "%s", rel);
    } else {
        fs->cwd[0] = '\0';
    }
}

/* Host-Pfad des Verzeichniseintrags "name" im Verzeichnis dirpath ("." / ".." / Datei);
   ".." bleibt an der Basispfad-Grenze stehen (wie "/d0/.." auf RBF) */
static void dir_entry_hostpath(dhf_host_fs_t *fs, const char *dirpath, const char *name,
                               char *out, size_t max) {
    if (strcmp(name, ".") == 0) {
        snprintf(out, max, "%s", dirpath);
    } else if (strcmp(name, "..") == 0) {
        const char *sl = strrchr(dirpath, '/');
        if (strcmp(dirpath, fs->basepath) == 0 || !sl || (size_t)(sl - dirpath) < strlen(fs->basepath)) {
            snprintf(out, max, "%s", fs->basepath);
        } else {
            snprintf(out, max, "%.*s", (int)(sl - dirpath), dirpath);
        }
    } else {
        snprintf(out, max, "%s/%s", dirpath, name);
    }
}

/* 2026-09-26: Pseudo-Sektornummern eines offenen Handles und seines Elternverzeichnisses --
   der Manager traegt sie nach Open/Create als pd_fd/pd_dfd in den RBF-Optionsteil des
   Pfaddeskriptors ein (rbf.h struct rbf_opt, $B6/$BA). bashs getwd vergleicht pd_fd von "."
   mit den Sektornummern (Byte 29-31) der Eintraege in "..". Dieselbe Pfadform wie in
   dir_entry_hostpath(), damit die Nummern uebereinstimmen. */
/* 2026-09-26: pd_dfd wie RBF = Sektornummer des Verzeichnisses, in dem der LETZTE
   Bestandteil des uebergebenen Pfads gefunden wurde (nicht das Elternverzeichnis des Ziels):
   "."  -> aktuelles Verzeichnis, "./.." -> ebenfalls das aktuelle (pd_fd dagegen dessen
   Eltern), "/d0/a/b" -> /d0/a. Einzelner relativer Name -> cwd; Laufwerkswurzel -> sie selbst. */
uint32_t dhf_host_fs_container_lsn(dhf_host_fs_t *fs, const char *path) {
    char cont[DHF_PATH_MAX], target[DHF_PATH_MAX];
    if (!fs) return 0;
    const char *sl = path ? strrchr(path, '/') : NULL;
    if (!path || !sl) {
        cont[0] = '\0';                               /* relativer Einzelname: cwd */
    } else if (sl == path) {
        snprintf(cont, sizeof(cont), "%s", path);      /* "/d0": Wurzel selbst */
    } else {
        snprintf(cont, sizeof(cont), "%.*s", (int)(sl - path), path);
        if (cont[0] == '/' && !strchr(cont + 1, '/')) {
            /* "/d0/x" -> Behaelter "/d0" = Laufwerkswurzel */
        }
    }
    if (resolve_confined_path(fs, cont, target, sizeof(target)) != 0) return 0;
    char real[PATH_MAX];
    return lsn_for_path(fs, realpath(target, real) ? real : target);
}

int dhf_host_fs_handle_lsns(dhf_host_fs_t *fs, int handle, uint32_t *self, uint32_t *parent) {
    if (!fs || handle < 0 || handle >= DHF_MAX_HANDLES || !fs->handles[handle].in_use) return -1;
    const char *p = fs->handles[handle].path;
    char par[DHF_PATH_MAX];
    size_t bl = strlen(fs->basepath);
    const char *sl = strrchr(p, '/');
    if (strcmp(p, fs->basepath) == 0 || !sl || (size_t)(sl - p) < bl)
        snprintf(par, sizeof(par), "%s", fs->basepath);
    else
        snprintf(par, sizeof(par), "%.*s", (int)(sl - p), p);
    if (self) *self = lsn_for_path(fs, p);
    if (parent) *parent = lsn_for_path(fs, par);
    return 0;
}

static ssize_t dhf_read_dir_entries(dhf_host_fs_t *fs, int handle, void *buf, size_t count, uint8_t *status) {
    unsigned char *out = (unsigned char*)buf;
    size_t produced = 0;
    uint32_t pos = fs->handles[handle].dir_pos;
    while (produced < count) {
        uint32_t idx = pos / DHF_DIRENT_SIZE, off = pos % DHF_DIRENT_SIZE;
        char name[256];
        int kind = dir_entry_name(fs, handle, idx, name, sizeof(name));
        if (!kind) break;
        unsigned char rec[DHF_DIRENT_SIZE];
        memset(rec, 0, sizeof(rec));
        if (kind == 2) {                  /* geloeschter Eintrag: freier Platz, alles 0 */
            size_t n0 = DHF_DIRENT_SIZE - off;
            if (n0 > count - produced) n0 = count - produced;
            memcpy(out + produced, rec + off, n0);
            produced += n0;
            pos += (uint32_t)n0;
            continue;
        }
        size_t nlen = strlen(name);
        if (nlen > 28) nlen = 28;
        memcpy(rec, name, nlen);
        if (nlen > 0) rec[nlen - 1] |= 0x80;
        /* Byte 29-31: FD-Sektornummer -- hier eine Pseudo-Nummer, die SS_FDInf aufloest */
        char hp[DHF_PATH_MAX];
        dir_entry_hostpath(fs, fs->handles[handle].path, name, hp, sizeof(hp));
        uint32_t lsn = lsn_for_path(fs, hp);
        rec[29] = (unsigned char)(lsn >> 16);
        rec[30] = (unsigned char)(lsn >> 8);
        rec[31] = (unsigned char)lsn;
        size_t n = DHF_DIRENT_SIZE - off;
        if (n > count - produced) n = count - produced;
        memcpy(out + produced, rec + off, n);
        produced += n;
        pos += (uint32_t)n;
    }
    fs->handles[handle].dir_pos = pos;
    if (produced == 0) {
        /* echtes Verzeichnisende -- wie bei Dateien ein Fehler, kein stiller 0-Byte-Erfolg
           (s. Kommentar unten bei der Datei-Variante). */
        if (status) *status = DHF_ERR_EOF;
        return 0;
    }
    if (status) *status = DHF_ERR_OK;
    return (ssize_t)produced;
}

ssize_t dhf_host_fs_read(dhf_host_fs_t *fs, int handle, void *buf, size_t count, uint8_t *status) {
    if (!fs || handle < 0 || handle >= DHF_MAX_HANDLES || !fs->handles[handle].in_use) {
        if (status) *status = DHF_ERR_BAD_PATH;
        return -1;
    }

    if (fs->handles[handle].is_dir) {
        return dhf_read_dir_entries(fs, handle, buf, count, status);
    }
    if (fs->handles[handle].is_raw) {
        return raw_read(fs, handle, buf, count, status);
    }

    ssize_t res = read(fs->handles[handle].fd, buf, count);
    if (res < 0) {
        if (status) *status = errno_to_dhf(errno);
        return -1;
    }
    /* 2026-09-26: echtes I$Read MUSS am Dateiende einen Fehler (E$EOF) melden statt
       Erfolg mit 0 Bytes -- "OS-9 System Calls" Kap. 2: "If there is no data available,
       an EOF error is returned." Ohne das ruft ein Aufrufer wie das echte "list"-Kommando
       I$Read endlos weiter (0 Bytes/kein Fehler ist aus seiner Sicht kein Abbruchgrund) --
       gefunden, als "list /d0/hello.txt" nach korrekt ausgegebenem Inhalt haengenblieb. */
    if (res == 0 && count > 0) {
        if (status) *status = DHF_ERR_EOF;
        return 0;
    }
    if (status) *status = DHF_ERR_OK;
    return res;
}

ssize_t dhf_host_fs_write(dhf_host_fs_t *fs, int handle, const void *buf, size_t count, uint8_t *status) {
    if (fs && fs->readonly) {                      /* Laufwerk nur lesbar */
        if (status) *status = DHF_ERR_WRITE_PROT;
        return -1;
    }
    if (!fs || handle < 0 || handle >= DHF_MAX_HANDLES || !fs->handles[handle].in_use || fs->handles[handle].is_dir) {
        if (status) *status = DHF_ERR_BAD_PATH;
        return -1;
    }
    if (fs->handles[handle].is_raw) {                 /* Rohgeraet: kein Schreiben (format!) */
        if (status) *status = DHF_ERR_WRITE_PROT;
        return -1;
    }

    ssize_t res = write(fs->handles[handle].fd, buf, count);
    if (res < 0) {
        if (status) *status = errno_to_dhf(errno);
        return -1;
    }
    if (status) *status = DHF_ERR_OK;
    return res;
}

off_t dhf_host_fs_seek(dhf_host_fs_t *fs, int handle, off_t offset, int whence, uint8_t *status) {
    if (!fs || handle < 0 || handle >= DHF_MAX_HANDLES || !fs->handles[handle].in_use) {
        if (status) *status = DHF_ERR_BAD_PATH;
        return -1;
    }
    if (fs->handles[handle].is_raw) {
        off_t np = (whence == DHF_SEEK_CUR ? (off_t)fs->handles[handle].dir_pos : 0) + offset;
        if (np < 0 || np > 0xFFFFFFFFLL) { if (status) *status = DHF_ERR_ERROR; return -1; }
        fs->handles[handle].dir_pos = (uint32_t)np;
        if (status) *status = DHF_ERR_OK;
        return np;
    }
    if (fs->handles[handle].is_dir) {
        off_t base = 0;
        if (whence == DHF_SEEK_CUR) base = fs->handles[handle].dir_pos;
        else if (whence == DHF_SEEK_END) base = dir_handle_size(fs, handle);
        off_t np = base + offset;
        if (np < 0 || np > 0xFFFFFFFFLL) {
            if (status) *status = DHF_ERR_ERROR;
            return -1;
        }
        fs->handles[handle].dir_pos = (uint32_t)np;
        if (status) *status = DHF_ERR_OK;
        return np;
    }

    int pwhence = SEEK_SET;
    if (whence == DHF_SEEK_CUR) pwhence = SEEK_CUR;
    else if (whence == DHF_SEEK_END) pwhence = SEEK_END;

    off_t res = lseek(fs->handles[handle].fd, offset, pwhence);
    if (res == (off_t)-1) {
        if (status) *status = errno_to_dhf(errno);
        return -1;
    }
    if (status) *status = DHF_ERR_OK;
    return res;
}

int dhf_host_fs_readln(dhf_host_fs_t *fs, int handle, char *buf, size_t maxlen, uint8_t *status) {
    if (!fs || handle < 0 || handle >= DHF_MAX_HANDLES || !fs->handles[handle].in_use || fs->handles[handle].is_dir || maxlen == 0) {
        if (status) *status = DHF_ERR_BAD_PATH;
        return -1;
    }

    /* 2026-09-26: bis zu maxlen Byte (OS-9: "d1.l = maximum number of bytes to read"),
       nicht maxlen-1 -- der Puffer ist ein Gast-Puffer ohne NUL-Konvention. */
    size_t i = 0;
    while (i < maxlen) {
        char ch;
        ssize_t r = read(fs->handles[handle].fd, &ch, 1);
        if (r <= 0) {
            if (r < 0) {
                if (status) *status = errno_to_dhf(errno);
                return -1;
            }
            break; /* EOF */
        }
        buf[i++] = ch;
        if (ch == '\n' || ch == '\r') {
            break;
        }
    }
    /* s. dhf_host_fs_read: 0 Bytes UND wirkliches Dateiende (keine Zeile, auch nicht
       unvollstaendig, gelesen) muss E$EOF melden, sonst endlose I$ReadLn-Wiederholung. */
    if (i == 0) {
        if (status) *status = DHF_ERR_EOF;
        return 0;
    }
    if (status) *status = DHF_ERR_OK;
    return (int)i;
}

/* I$WritLn: schreibt bis EINSCHLIESSLICH des ersten CR ($0D), hoechstens len Byte
   ("OS-9 System Calls": "writes ... until a carriage return is encountered or d1.l bytes
   have been written"), und haengt nichts an. 2026-09-26: vorher wurde immer der ganze
   Puffer geschrieben und ein Unix-'\n' angehaengt (Regressionstest dhfregr Fall 07). */
int dhf_host_fs_writeln(dhf_host_fs_t *fs, int handle, const char *buf, size_t len, uint8_t *status) {
    size_t n = 0;
    while (n < len && buf[n] != '\r') n++;
    if (n < len) n++;           /* CR selbst mitschreiben */
    ssize_t w = dhf_host_fs_write(fs, handle, buf, n, status);
    return (w < 0) ? -1 : (int)w;
}

int dhf_host_fs_getstat(dhf_host_fs_t *fs, const char *path, void *statbuf, size_t *out_size, uint8_t *status) {
    char target[DHF_PATH_MAX];
    if (resolve_confined_path(fs, path, target, sizeof(target)) != 0) {
        if (status) *status = DHF_ERR_BAD_NAME;
        return -1;
    }

    struct stat st;
    if (stat(target, &st) != 0) {
        if (status) *status = errno_to_dhf(errno);
        return -1;
    }

    /* Serialize compact stat into statbuf:
       uint32_t size, uint32_t mode, uint32_t mtime, uint32_t atime */
    uint32_t *fields = (uint32_t*)statbuf;
    fields[0] = htonl((uint32_t)st.st_size);
    fields[1] = htonl((uint32_t)st.st_mode);
    fields[2] = htonl((uint32_t)st.st_mtime);
    fields[3] = htonl((uint32_t)st.st_atime);

    if (out_size) *out_size = 16;
    if (status) *status = DHF_ERR_OK;
    return 0;
}

/* 2026-09-26: I$GetStt (Q9-DHF-68k manager) has no pathname at hand for an already-open
   path -- like Read/Write/Close, it only carries the OS-9 path number (here as "handle",
   the array index dhf_host_fs_open_at() already assigned). fstat() on the stored fd avoids
   re-resolving/re-confining a path string the caller doesn't have. */
int dhf_host_fs_getstat_at(dhf_host_fs_t *fs, int handle, void *statbuf, size_t *out_size, uint8_t *status) {
    if (!fs || handle < 0 || handle >= DHF_MAX_HANDLES || !fs->handles[handle].in_use) {
        if (status) *status = DHF_ERR_BAD_PATH;
        return -1;
    }

    struct stat st;
    int rc = fs->handles[handle].is_dir
             ? stat(fs->handles[handle].path, &st)
             : fstat(fs->handles[handle].fd, &st);
    if (rc != 0) {
        if (status) *status = errno_to_dhf(errno);
        return -1;
    }

    uint32_t *fields = (uint32_t*)statbuf;
    /* Verzeichnis: Groesse der virtuellen RBF-Verzeichnisdatei, nicht die Host-Groesse */
    fields[0] = htonl(fs->handles[handle].is_dir ? dir_handle_size(fs, handle)
                                                 : (uint32_t)st.st_size);
    fields[1] = htonl((uint32_t)st.st_mode);
    fields[2] = htonl((uint32_t)st.st_mtime);
    fields[3] = htonl((uint32_t)st.st_atime);

    if (out_size) *out_size = 16;
    if (status) *status = DHF_ERR_OK;
    return 0;
}

int dhf_host_fs_setstat(dhf_host_fs_t *fs, const char *path, const void *statbuf, uint8_t *status) {
    char target[DHF_PATH_MAX];
    if (resolve_confined_path(fs, path, target, sizeof(target)) != 0) {
        if (status) *status = DHF_ERR_BAD_NAME;
        return -1;
    }
    (void)statbuf;
    if (status) *status = DHF_ERR_OK;
    return 0;
}

/* 2026-09-26: I$SetStt (Q9-DHF-68k manager), wie schon I$GetStt, hat fuer einen bereits
   offenen Pfad nur die OS-9-Pfadnummer (hier als "handle"), keinen Pfadnamen -- ftruncate()
   auf dem gespeicherten fd braucht keine erneute Pfadaufloesung/-Confinement-Pruefung.
   Bislang nur SS_Size (Dateigroesse setzen) verdrahtet -- die einzige SetStt-Funktion, die
   gegen ein reines Host-Passthrough-Dateisystem ueberhaupt sinnvoll 1:1 abbildbar ist
   (SS_Attr/SS_Reset/SS_RFM/... haben auf einem Host-Verzeichnis kein Aequivalent). */
int dhf_host_fs_setsize_at(dhf_host_fs_t *fs, int handle, uint32_t new_size, uint8_t *status) {
    if (fs && fs->readonly) {                      /* Laufwerk nur lesbar */
        if (status) *status = DHF_ERR_WRITE_PROT;
        return -1;
    }
    if (!fs || handle < 0 || handle >= DHF_MAX_HANDLES || !fs->handles[handle].in_use ||
        fs->handles[handle].is_dir) {
        if (status) *status = DHF_ERR_BAD_PATH;
        return -1;
    }
    if (ftruncate(fs->handles[handle].fd, (off_t)new_size) != 0) {
        if (status) *status = errno_to_dhf(errno);
        return -1;
    }
    if (status) *status = DHF_ERR_OK;
    return 0;
}

/* 2026-09-26: I$GetStt SS_FD ("Read File Descriptor Sector", "OS-9 Technical Manual" Kap. 7
   Figure 7-2 + "OS-9 System Calls" Kap. 2): liefert ein FD-Sektor-Abbild, das reale
   RBF-Utilities wie "attr" ueber I$GetStt(SS_FD) anfordern -- KEIN physischer Sektor-
   Zugriff (das waere bei einem Host-Passthrough-Dateisystem ohnehin nicht abbildbar),
   sondern ein ganz normaler GetStt-Aufruf mit dokumentiertem Format:
     Offset $00 (1)   FD_ATT    Dateiattribute (Bit7=Verzeichnis, Bit6=exklusiv,
                                Bit5/4/3=oeffentlich x/w/r, Bit2/1/0=Besitzer x/w/r)
     Offset $01 (2)   FD_OWN    Besitzer-User-ID (hier immer 0, DHF kennt keine echten
                                OS-9-Benutzer)
     Offset $03 (5)   FD_DAT    Letzte Aenderung: Jahr/Monat/Tag/Stunde/Minute
     Offset $08 (1)   FD_LNK    Link-Zaehler (immer 1)
     Offset $09 (4)   FD_SIZ    Dateigroesse
     Offset $0D (3)   FD_CREAT  Erstellungsdatum: Jahr/Monat/Tag
     Offset $10 (240) FD_SEG    Segmentliste -- bleibt genullt (kein echtes Medium mit
                                LSNs; "Unused segments must be zero" laut Handbuch)
   Gibt bis zu want_len Bytes (vom Aufrufer vorgegeben, <= Sektorgroesse) zurueck. */
#define DHF_FD_SECTOR_SIZE 256

/* FD-Sektor-Abbild ("OS-9 Technical Manual" Kap. 7, Figure 7-2) aus stat() -- gemeinsam fuer
   SS_FD (per Handle) und SS_FDInf (per Pseudo-Sektornummer). 2026-09-26: Zeiten in ORTSZEIT
   (vorher UTC -- "dir -e" zeigte alles 2 h zu frueh; die OS-9-Uhr laeuft in Ortszeit). */
static size_t build_fd(const struct stat *st, const char *path, void *buf, size_t want_len) {
    unsigned char fd[DHF_FD_SECTOR_SIZE];
    memset(fd, 0, sizeof(fd));

    unsigned char att = 0;
    if (st->st_mode & S_IRUSR) att |= 0x01;
    if (st->st_mode & S_IWUSR) att |= 0x02;
    if (st->st_mode & S_IXUSR) att |= 0x04;
    if (st->st_mode & S_IROTH) att |= 0x08;
    if (st->st_mode & S_IWOTH) att |= 0x10;
    if (st->st_mode & S_IXOTH) att |= 0x20;
    if (S_ISDIR(st->st_mode))  att |= 0x80;
    fd[0x00] = att;                         /* FD_ATT */
    fd[0x01] = 0; fd[0x02] = 0;             /* FD_OWN */

    struct tm tmv;
    localtime_r(&st->st_mtime, &tmv);
    fd[0x03] = (unsigned char)tmv.tm_year;  /* FD_DAT: Jahr (seit 1900, wie tm_year) */
    fd[0x04] = (unsigned char)(tmv.tm_mon + 1);
    fd[0x05] = (unsigned char)tmv.tm_mday;
    fd[0x06] = (unsigned char)tmv.tm_hour;
    fd[0x07] = (unsigned char)tmv.tm_min;

    fd[0x08] = 1;                           /* FD_LNK */

    uint32_t size_be = htonl(S_ISDIR(st->st_mode) ? dir_virtual_size(path) : (uint32_t)st->st_size);
    memcpy(&fd[0x09], &size_be, 4);         /* FD_SIZ */

    localtime_r(&st->st_ctime, &tmv);
    fd[0x0D] = (unsigned char)tmv.tm_year;  /* FD_CREAT: Jahr/Monat/Tag */
    fd[0x0E] = (unsigned char)(tmv.tm_mon + 1);
    fd[0x0F] = (unsigned char)tmv.tm_mday;
    /* FD_SEG (Offset $10, 240 Byte) bleibt genullt */

    size_t n = want_len;
    if (n > sizeof(fd)) n = sizeof(fd);
    memcpy(buf, fd, n);
    return n;
}

int dhf_host_fs_getfd_at(dhf_host_fs_t *fs, int handle, void *buf, size_t want_len, size_t *out_len, uint8_t *status) {
    if (!fs || handle < 0 || handle >= DHF_MAX_HANDLES || !fs->handles[handle].in_use) {
        if (status) *status = DHF_ERR_BAD_PATH;
        return -1;
    }

    struct stat st;
    int rc = fs->handles[handle].is_dir
             ? stat(fs->handles[handle].path, &st)
             : fstat(fs->handles[handle].fd, &st);
    if (rc != 0) {
        if (status) *status = errno_to_dhf(errno);
        return -1;
    }
    size_t n = build_fd(&st, fs->handles[handle].path, buf, want_len);
    if (out_len) *out_len = n;
    if (status) *status = DHF_ERR_OK;
    return 0;
}

/* 2026-09-26: I$SetStt SS_FD ("Write File Descriptor Sector", 68k_tech.pdf S. 587): nur
   FD_OWN, FD_DAT und FD_Creat werden uebernommen -- auf dem Host davon FD_DAT (Offset 3,
   Jahr-1900/Monat/Tag/Stunde/Minute, Ortszeit wie in build_fd) als Aenderungszeit. "copy" setzt
   so das Datum der Kopie ("can't put file descriptor" ohne). */
int dhf_host_fs_setfd_at(dhf_host_fs_t *fs, int handle, const unsigned char *fdimg, uint8_t *status) {
    if (fs && fs->readonly) {                      /* Laufwerk nur lesbar */
        if (status) *status = DHF_ERR_WRITE_PROT;
        return -1;
    }
    if (!fs || handle < 0 || handle >= DHF_MAX_HANDLES || !fs->handles[handle].in_use || !fdimg) {
        if (status) *status = DHF_ERR_BAD_PATH;
        return -1;
    }
    struct tm tmv;
    memset(&tmv, 0, sizeof(tmv));
    tmv.tm_year = fdimg[3];
    tmv.tm_mon  = fdimg[4] ? fdimg[4] - 1 : 0;
    tmv.tm_mday = fdimg[5] ? fdimg[5] : 1;
    tmv.tm_hour = fdimg[6];
    tmv.tm_min  = fdimg[7];
    tmv.tm_isdst = -1;                  /* Sommerzeit selbst bestimmen lassen */
    time_t t = mktime(&tmv);
    struct timeval tv[2];
    tv[0].tv_sec = time(NULL); tv[0].tv_usec = 0;   /* Zugriffszeit: jetzt */
    tv[1].tv_sec = t;          tv[1].tv_usec = 0;   /* Aenderungszeit: FD_DAT */
    int rc = (fs->handles[handle].is_dir || fs->handles[handle].fd < 0)
             ? utimes(fs->handles[handle].path, tv)
             : futimes(fs->handles[handle].fd, tv);
    if (rc != 0) {
        if (status) *status = errno_to_dhf(errno);
        return -1;
    }
    if (status) *status = DHF_ERR_OK;
    return 0;
}

/* 2026-09-26: I$GetStt SS_FDInf -- FD-Abbild zur Pseudo-Sektornummer aus einem frueher
   gelesenen Verzeichniseintrag (s. lsn_for_path). Unbekannte Nummer -> E$PNNF. */
int dhf_host_fs_getfd_lsn(dhf_host_fs_t *fs, uint32_t lsn, void *buf, size_t want_len, size_t *out_len, uint8_t *status) {
    const char *lp = fs ? lsn_to_path(fs, lsn) : NULL;
    if (!lp) {
        if (status) *status = DHF_ERR_NOT_FOUND;
        return -1;
    }
    struct stat st;
    if (stat(lp, &st) != 0) {
        if (status) *status = errno_to_dhf(errno);
        return -1;
    }
    size_t n = build_fd(&st, lp, buf, want_len);
    if (out_len) *out_len = n;
    if (status) *status = DHF_ERR_OK;
    return 0;
}

/* 2026-09-26: I$SetStt SS_Attr -- Gegenstueck zu SS_FD's Lesen (dhf_host_fs_getfd_at).
   Setzt Unix-Rechte-Bits aus dem FD_ATT-Byte (gleiche Bitlage wie dort: Bit0-2=Besitzer
   r/w/x, Bit3-5=oeffentlich r/w/x). Bit6 (exklusiv) und Bit7 (Verzeichnis) werden bewusst
   ignoriert -- Verzeichnis-Status folgt bei uns aus dem tatsaechlichen Host-Dateityp, nicht
   aus einem Flag ("OS-9 System Calls": "not permitted to set the dir bit of a non-
   directory file"), und "exklusiv" hat fuer ein Host-Passthrough-Dateisystem ohnehin keine
   Entsprechung. */
int dhf_host_fs_setattr_at(dhf_host_fs_t *fs, int handle, uint8_t attr, uint8_t *status) {
    if (fs && fs->readonly) {                      /* Laufwerk nur lesbar */
        if (status) *status = DHF_ERR_WRITE_PROT;
        return -1;
    }
    if (!fs || handle < 0 || handle >= DHF_MAX_HANDLES || !fs->handles[handle].in_use) {
        if (status) *status = DHF_ERR_BAD_PATH;
        return -1;
    }
    /* attr==0 heisst hier wirklich "alle Rechte weg", nicht "Voreinstellung" */
    mode_t mode = (attr & 0x3F) ? os9_attr_to_mode(attr, fs->handles[handle].is_dir) : 0;
    if (fs->handles[handle].is_dir && !(attr & 0x80)) {
        /* Dir-Bit entfernt: ab jetzt darf I$Delete dieses (leere) Verzeichnis loeschen */
        uint32_t slot = fs->deldir_next++ % DHF_DELDIR_SLOTS;
        char real[PATH_MAX];
        const char *src = realpath(fs->handles[handle].path, real) ? real : fs->handles[handle].path;
        strncpy(fs->deldir_ok[slot], src, DHF_PATH_MAX - 1);
        fs->deldir_ok[slot][DHF_PATH_MAX - 1] = '\0';
    }

    int rc = fs->handles[handle].is_dir
             ? chmod(fs->handles[handle].path, mode)
             : fchmod(fs->handles[handle].fd, mode);
    if (rc != 0) {
        if (status) *status = errno_to_dhf(errno);
        return -1;
    }
    if (status) *status = DHF_ERR_OK;
    return 0;
}

/* I$GetStt SS_Pos -- aktuelle Dateiposition (Handle-basiert wie alle anderen `_at`-
   Funktionen). */
int dhf_host_fs_getpos_at(dhf_host_fs_t *fs, int handle, uint32_t *out_pos, uint8_t *status) {
    if (!fs || handle < 0 || handle >= DHF_MAX_HANDLES || !fs->handles[handle].in_use) {
        if (status) *status = DHF_ERR_BAD_PATH;
        return -1;
    }
    if (fs->handles[handle].is_dir) {
        if (out_pos) *out_pos = fs->handles[handle].dir_pos;
        if (status) *status = DHF_ERR_OK;
        return 0;
    }
    off_t pos = lseek(fs->handles[handle].fd, 0, SEEK_CUR);
    if (pos == (off_t)-1) {
        if (status) *status = errno_to_dhf(errno);
        return -1;
    }
    if (out_pos) *out_pos = (uint32_t)pos;
    if (status) *status = DHF_ERR_OK;
    return 0;
}

/* I$GetStt SS_EOF -- Dateiende-Test: aktuelle Position mit Dateigroesse vergleichen.
   status=DHF_ERR_EOF (== echtes E$EOF, keine Umrechnung noetig) wenn am Ende, sonst
   DHF_ERR_OK. */
int dhf_host_fs_iseof_at(dhf_host_fs_t *fs, int handle, uint8_t *status) {
    if (!fs || handle < 0 || handle >= DHF_MAX_HANDLES || !fs->handles[handle].in_use) {
        if (status) *status = DHF_ERR_BAD_PATH;
        return -1;
    }
    if (fs->handles[handle].is_dir) {
        uint32_t size = dir_handle_size(fs, handle);
        if (status) *status = (fs->handles[handle].dir_pos >= size) ? DHF_ERR_EOF : DHF_ERR_OK;
        return 0;
    }
    off_t pos = lseek(fs->handles[handle].fd, 0, SEEK_CUR);
    struct stat st;
    if (pos == (off_t)-1 || fstat(fs->handles[handle].fd, &st) != 0) {
        if (status) *status = errno_to_dhf(errno);
        return -1;
    }
    if (status) *status = (pos >= st.st_size) ? DHF_ERR_EOF : DHF_ERR_OK;
    return 0;
}

/* I$ChgDir: Verzeichnis pruefen und seine Pseudo-Sektornummer liefern -- der Manager legt sie
   in P$DIO des Prozesses ab (wie RBF die Verzeichnisadresse); kein Zustand hier. */
int dhf_host_fs_chdir(dhf_host_fs_t *fs, const char *path, uint32_t *out_lsn, uint8_t *status) {
    char target[DHF_PATH_MAX];
    if (resolve_confined_path(fs, path, target, sizeof(target)) != 0) {
        if (status) *status = DHF_ERR_BAD_NAME;
        return -1;
    }

    struct stat st;
    if (stat(target, &st) != 0 || !S_ISDIR(st.st_mode)) {
        if (status) *status = DHF_ERR_NOT_DIR;
        return -1;
    }

    /* 2026-09-26: das Ziel vollstaendig aufloesen, bevor es als cwd gespeichert wird -- sonst
       blieb nach "chd unter" + "chd .." der Text "dd/unter/.." stehen, der ins Leere zeigt,
       sobald "unter" geloescht ist ("deldir": danach scheiterte jedes Open auf "."). */
    char real[PATH_MAX];
    if (realpath(target, real) != NULL) {
        strncpy(target, real, sizeof(target) - 1);
        target[sizeof(target) - 1] = '\0';
    }

    if (out_lsn) *out_lsn = lsn_for_path(fs, target);
    if (status) *status = DHF_ERR_OK;
    return 0;
}

int dhf_host_fs_mkdir(dhf_host_fs_t *fs, const char *path, int mode, uint8_t *status) {
    if (fs && fs->readonly) {                      /* Laufwerk nur lesbar */
        if (status) *status = DHF_ERR_WRITE_PROT;
        return -1;
    }
    char target[DHF_PATH_MAX];
    if (resolve_confined_path(fs, path, target, sizeof(target)) != 0) {
        if (status) *status = DHF_ERR_BAD_NAME;
        return -1;
    }

    mode_t pmode = os9_attr_to_mode((unsigned)mode, 1);
    if (mkdir(target, pmode) != 0) {
        if (status) *status = errno_to_dhf(errno);
        return -1;
    }
    chmod(target, pmode);       /* exakt die OS-9-Attribute, unabhaengig von der Host-umask */
    if (status) *status = DHF_ERR_OK;
    return 0;
}

int dhf_host_fs_rmdir(dhf_host_fs_t *fs, const char *path, uint8_t *status) {
    if (fs && fs->readonly) {                      /* Laufwerk nur lesbar */
        if (status) *status = DHF_ERR_WRITE_PROT;
        return -1;
    }
    char target[DHF_PATH_MAX];
    if (resolve_confined_path(fs, path, target, sizeof(target)) != 0) {
        if (status) *status = DHF_ERR_BAD_NAME;
        return -1;
    }

    if (rmdir(target) != 0) {
        if (status) *status = errno_to_dhf(errno);
        return -1;
    }
    if (status) *status = DHF_ERR_OK;
    return 0;
}

/* I$Delete. 2026-09-26: Verzeichnisse wie RBF -- nur loeschbar, nachdem ihr Dir-Bit per
   SS_Attr entfernt wurde ("deldir" macht genau das), und nur wenn leer (sonst E$DNE); ein
   blosses "del" auf ein Verzeichnis bleibt E$FNA. Vorher scheiterte jedes Verzeichnis an
   unlink() -> "deldir -q" blieb auf dem leeren Verzeichnis stehen. */
int dhf_host_fs_unlink(dhf_host_fs_t *fs, const char *path, uint8_t *status) {
    if (fs && fs->readonly) {                      /* Laufwerk nur lesbar */
        if (status) *status = DHF_ERR_WRITE_PROT;
        return -1;
    }
    char target[DHF_PATH_MAX];
    if (resolve_confined_path(fs, path, target, sizeof(target)) != 0) {
        if (status) *status = DHF_ERR_BAD_NAME;
        return -1;
    }

    struct stat st;
    if (stat(target, &st) == 0 && S_ISDIR(st.st_mode)) {
        if (!deldir_marked(fs, target, 1)) {
            if (status) *status = DHF_ERR_IS_DIR;      /* E$FNA wie RBF */
            return -1;
        }
        if (rmdir(target) != 0) {
            if (status) *status = errno_to_dhf(errno);  /* nicht leer -> E$DNE */
            return -1;
        }
        if (status) *status = DHF_ERR_OK;
        return 0;
    }

    if (unlink(target) != 0) {
        if (status) *status = errno_to_dhf(errno);
        return -1;
    }
    if (status) *status = DHF_ERR_OK;
    return 0;
}

int dhf_host_fs_rename(dhf_host_fs_t *fs, const char *oldp, const char *newp, uint8_t *status) {
    if (fs && fs->readonly) {                      /* Laufwerk nur lesbar */
        if (status) *status = DHF_ERR_WRITE_PROT;
        return -1;
    }
    char target_old[DHF_PATH_MAX];
    char target_new[DHF_PATH_MAX];

    if (resolve_confined_path(fs, oldp, target_old, sizeof(target_old)) != 0 ||
        resolve_confined_path(fs, newp, target_new, sizeof(target_new)) != 0) {
        if (status) *status = DHF_ERR_BAD_NAME;
        return -1;
    }

    if (rename(target_old, target_new) != 0) {
        if (status) *status = errno_to_dhf(errno);
        return -1;
    }
    if (status) *status = DHF_ERR_OK;
    return 0;
}

/* Ein einzelner OS-9-Dateiname (Eintrag in einem Verzeichnis) in name kopieren; Hochbit am
   letzten Zeichen (RBF-Verzeichnisformat) wird entfernt. 0 = ungueltig ('/', "."/"..",
   leer, >28 Zeichen). */
static int os9_simple_name(const char *in, char *name, size_t max) {
    if (!in) return 0;
    size_t n = 0;
    while (in[n] && n + 1 < max) {
        name[n] = (char)(in[n] & 0x7F);
        if ((unsigned char)in[n] & 0x80) { n++; break; }
        n++;
    }
    name[n] = '\0';
    if (n == 0 || n > 28 || strchr(name, '/') || strcmp(name, ".") == 0 || strcmp(name, "..") == 0)
        return 0;
    return 1;
}

/* 2026-09-26: I$SetStt SS_Rename in der RBF-Konvention (ermittelt per Ablaufverfolgung der
   echten /CMDS/rename): handle ist ein offenes VERZEICHNIS, oldname/newname sind Eintraege
   darin. Der Eintrag behaelt seinen Platz in der Verzeichnis-Platztabelle (wie RBF, das nur
   den Namen im Verzeichniseintrag ueberschreibt). Vorher: handle = die Datei selbst, ein
   Pfad als neuer Name -- geraten, von keinem echten Programm so benutzt. */
int dhf_host_fs_rename_at(dhf_host_fs_t *fs, int handle, const char *oldname, const char *newname, uint8_t *status) {
    if (fs && fs->readonly) {                      /* Laufwerk nur lesbar */
        if (status) *status = DHF_ERR_WRITE_PROT;
        return -1;
    }
    if (!fs || handle < 0 || handle >= DHF_MAX_HANDLES || !fs->handles[handle].in_use) {
        if (status) *status = DHF_ERR_BAD_PATH;
        return -1;
    }
    if (!fs->handles[handle].is_dir) {
        if (status) *status = DHF_ERR_NOT_DIR;          /* E$FNA */
        return -1;
    }
    char on[64], nn[64];
    if (!os9_simple_name(oldname, on, sizeof(on)) || !os9_simple_name(newname, nn, sizeof(nn))) {
        if (status) *status = DHF_ERR_BAD_NAME;         /* E$BPNam */
        return -1;
    }
    char from[DHF_PATH_MAX * 2], to[DHF_PATH_MAX * 2];
    snprintf(from, sizeof(from), "%s/%s", fs->handles[handle].path, on);
    snprintf(to, sizeof(to), "%s/%s", fs->handles[handle].path, nn);
    struct stat st;
    if (lstat(from, &st) != 0) {
        if (status) *status = DHF_ERR_NOT_FOUND;        /* E$PNNF */
        return -1;
    }
    if (lstat(to, &st) == 0) {
        if (status) *status = DHF_ERR_FILE_EXISTS;      /* E$CEF */
        return -1;
    }
    if (rename(from, to) != 0) {
        if (status) *status = errno_to_dhf(errno);
        return -1;
    }
    /* Platz in der Platztabelle behalten: alten Namen dort durch den neuen ersetzen */
    for (int c = 0; c < DHF_DIRCACHE_SLOTS; c++) {
        if (!fs->dircache[c].path[0] || strcmp(fs->dircache[c].path, fs->handles[handle].path) != 0) continue;
        for (uint32_t k = 0; k < fs->dircache[c].count; k++) {
            if (fs->dircache[c].names[k] && strcmp(fs->dircache[c].names[k], on) == 0) {
                free(fs->dircache[c].names[k]);
                fs->dircache[c].names[k] = strdup(nn);
                break;
            }
        }
    }
    if (status) *status = DHF_ERR_OK;
    return 0;
}

/* I$GetStt SS_Free -- freier Speicherplatz im Basisverzeichnis. Braucht kein Handle (wirkt
   auf das gesamte DHF-"Geraet", nicht auf eine einzelne Datei) -- der Aufrufer liefert per
   Spezifikation trotzdem eine Pfadnummer mit, die hier aber ungenutzt bleibt (DHF hat nur
   einen einzigen Basispfad je Deskriptor, keine Mehrfach-Volumes). Ergebnis auf 32 Bit
   gekappt (OS-9s d0.l ist 32 Bit; ein Host-Dateisystem kann theoretisch mehr freien Platz
   melden als das darstellbar ist). */
/* 2026-09-26: I$GetStt SS_VolStore -- was "free" zuerst fragt. Gelingt es, benutzt es die
   Werte direkt (kein LSN0/Bitmap noetig) und zeigt die echte Groesse des Host-Dateisystems;
   512-Byte-Sektoren in 32 Bit reichen bis 2 TB. */
int dhf_host_fs_volstore(dhf_host_fs_t *fs, uint32_t out[4], uint8_t *status) {
    struct statvfs sv;
    if (!fs || statvfs(fs->basepath, &sv) != 0) {
        if (status) *status = fs ? errno_to_dhf(errno) : DHF_ERR_BAD_PATH;
        return -1;
    }
    uint64_t tot = (uint64_t)sv.f_blocks * sv.f_frsize / 512;
    uint64_t fr  = (uint64_t)sv.f_bavail * sv.f_frsize / 512;
    if (tot > 0xFFFFFFFFULL) tot = 0xFFFFFFFFULL;
    if (fr > tot) fr = tot;
    out[0] = 512;
    out[1] = (uint32_t)tot;
    out[2] = (uint32_t)fr;
    out[3] = (uint32_t)fr;          /* Host-Dateisystem: freier Platz ist "ein Block" */
    if (status) *status = DHF_ERR_OK;
    return 0;
}

int dhf_host_fs_getfree(dhf_host_fs_t *fs, uint32_t *out_free, uint8_t *status) {
    if (!fs) {
        if (status) *status = DHF_ERR_BAD_PATH;
        return -1;
    }
    struct statvfs sv;
    if (statvfs(fs->basepath, &sv) != 0) {
        if (status) *status = errno_to_dhf(errno);
        return -1;
    }
    uint64_t free_bytes = (uint64_t)sv.f_bavail * (uint64_t)sv.f_frsize;
    if (free_bytes > 0xFFFFFFFFULL) free_bytes = 0xFFFFFFFFULL;
    if (out_free) *out_free = (uint32_t)free_bytes;
    if (status) *status = DHF_ERR_OK;
    return 0;
}

int dhf_host_fs_opendir(dhf_host_fs_t *fs, const char *path, uint8_t *status) {
    char target[DHF_PATH_MAX];
    if (resolve_confined_path(fs, path, target, sizeof(target)) != 0) {
        if (status) *status = DHF_ERR_BAD_NAME;
        return -1;
    }

    DIR *d = opendir(target);
    if (!d) {
        if (status) *status = errno_to_dhf(errno);
        return -1;
    }

    int h = alloc_handle(fs, 1, target);
    if (h < 0) {
        closedir(d);
        if (status) *status = DHF_ERR_PATH_FULL;
        return -1;
    }

    fs->handles[h].dir = d;
    if (status) *status = DHF_ERR_OK;
    return h;
}

int dhf_host_fs_readdir(dhf_host_fs_t *fs, int handle, char *name_out, size_t max_name, uint32_t *size_out, uint32_t *mode_out, uint8_t *status) {
    if (!fs || handle < 0 || handle >= DHF_MAX_HANDLES || !fs->handles[handle].in_use || !fs->handles[handle].is_dir) {
        if (status) *status = DHF_ERR_BAD_PATH;
        return -1;
    }

    struct dirent *de = readdir(fs->handles[handle].dir);
    if (!de) {
        if (status) *status = DHF_ERR_OK;
        return 0; /* EOF */
    }

    if (name_out && max_name > 0) {
        strncpy(name_out, de->d_name, max_name - 1);
        name_out[max_name - 1] = '\0';
    }

    /* Stat the file to get size and mode */
    char fullpath[DHF_PATH_MAX * 2];
    snprintf(fullpath, sizeof(fullpath), "%s/%s", fs->handles[handle].path, de->d_name);
    struct stat st;
    if (stat(fullpath, &st) == 0) {
        if (size_out) *size_out = (uint32_t)st.st_size;
        if (mode_out) *mode_out = (uint32_t)st.st_mode;
    } else {
        if (size_out) *size_out = 0;
        if (mode_out) *mode_out = 0;
    }

    if (status) *status = DHF_ERR_OK;
    return 1;
}
