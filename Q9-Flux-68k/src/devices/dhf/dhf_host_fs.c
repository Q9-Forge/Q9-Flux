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
#include <sys/statvfs.h>

static uint8_t errno_to_dhf(int err) {
    switch (err) {
        case 0:         return DHF_ERR_OK;
        case ENOENT:    return DHF_ERR_NOT_FOUND;
        case EACCES:
        case EPERM:     return DHF_ERR_NO_PERMISSION;
        case EEXIST:    return DHF_ERR_FILE_EXISTS;
        case EBADF:     return DHF_ERR_BAD_PATH;
        case ENOSPC:    return DHF_ERR_DISK_FULL;
        case EISDIR:    return DHF_ERR_IS_DIR;
        case ENOTDIR:   return DHF_ERR_NOT_DIR;
        default:        return DHF_ERR_ERROR;
    }
}

/* Resolve path relative to cwd and basepath; enforce basepath confinement */
static int resolve_confined_path(dhf_host_fs_t *fs, const char *rel_or_abs, char *out_buf, size_t out_len) {
    char combined[DHF_PATH_MAX * 2];

    if (!rel_or_abs || !rel_or_abs[0]) {
        snprintf(combined, sizeof(combined), "%s/%s", fs->basepath, fs->cwd);
    } else if (rel_or_abs[0] == '/') {
        /* Virtual absolute path inside basepath */
        snprintf(combined, sizeof(combined), "%s/%s", fs->basepath, rel_or_abs + 1);
    } else {
        /* Relative path */
        if (fs->cwd[0]) {
            snprintf(combined, sizeof(combined), "%s/%s/%s", fs->basepath, fs->cwd, rel_or_abs);
        } else {
            snprintf(combined, sizeof(combined), "%s/%s", fs->basepath, rel_or_abs);
        }
    }

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
static int alloc_handle_at(dhf_host_fs_t *fs, int idx, int is_dir, const char *path) {
    if (idx < 0 || idx >= DHF_MAX_HANDLES) {
        return -1;
    }
    if (fs->handles[idx].in_use) {
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
        if (status) *status = DHF_ERR_NO_PERMISSION;
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
    char target[DHF_PATH_MAX];
    if (resolve_confined_path(fs, path, target, sizeof(target)) != 0) {
        if (status) *status = DHF_ERR_NO_PERMISSION;
        return -1;
    }

    int oflags = O_CREAT | O_TRUNC;
    if ((flags & (DHF_MODE_READ | DHF_MODE_WRITE)) == (DHF_MODE_READ | DHF_MODE_WRITE)) {
        oflags |= O_RDWR;
    } else if (flags & DHF_MODE_WRITE) {
        oflags |= O_WRONLY;
    } else {
        oflags |= O_RDWR;
    }

    mode_t pmode = (mode == 0) ? 0644 : (mode_t)mode;
    int fd = open(target, oflags, pmode);
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

/* 2026-09-26: wie dhf_host_fs_open, aber mit vom Aufrufer vorgegebenem Index (s.
 * alloc_handle_at-Kommentar) statt automatischer Vergabe -- fuer den Q9-DHF-Manager, der
 * die OS-9-Pfadnummer direkt als Index nutzt und dadurch selbst kein Handle mehr merken
 * muss. */
int dhf_host_fs_open_at(dhf_host_fs_t *fs, int idx, const char *path, int flags, uint8_t *status) {
    char target[DHF_PATH_MAX];
    if (resolve_confined_path(fs, path, target, sizeof(target)) != 0) {
        if (status) *status = DHF_ERR_NO_PERMISSION;
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
        DIR *d = opendir(target);
        if (!d) {
            if (status) *status = errno_to_dhf(errno);
            return -1;
        }
        int h = alloc_handle_at(fs, idx, 1, target);
        if (h < 0) {
            closedir(d);
            if (status) *status = DHF_ERR_BAD_PATH;
            return -1;
        }
        fs->handles[h].dir = d;
        if (status) *status = DHF_ERR_OK;
        return h;
    }

    int oflags = 0;
    if ((flags & (DHF_MODE_READ | DHF_MODE_WRITE)) == (DHF_MODE_READ | DHF_MODE_WRITE)) {
        oflags = O_RDWR;
    } else if (flags & DHF_MODE_WRITE) {
        oflags = O_WRONLY;
    } else {
        oflags = O_RDONLY;
    }

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
    char target[DHF_PATH_MAX];
    if (resolve_confined_path(fs, path, target, sizeof(target)) != 0) {
        if (status) *status = DHF_ERR_NO_PERMISSION;
        return -1;
    }

    int oflags = O_CREAT | O_TRUNC;
    if ((flags & (DHF_MODE_READ | DHF_MODE_WRITE)) == (DHF_MODE_READ | DHF_MODE_WRITE)) {
        oflags |= O_RDWR;
    } else if (flags & DHF_MODE_WRITE) {
        oflags |= O_WRONLY;
    } else {
        oflags |= O_RDWR;
    }

    mode_t pmode = (mode == 0) ? 0644 : (mode_t)mode;
    int fd = open(target, oflags, pmode);
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

int dhf_host_fs_close(dhf_host_fs_t *fs, int handle, uint8_t *status) {
    if (!fs || handle < 0 || handle >= DHF_MAX_HANDLES || !fs->handles[handle].in_use) {
        if (status) *status = DHF_ERR_BAD_PATH;
        return -1;
    }

    if (fs->handles[handle].is_dir) {
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

static ssize_t dhf_read_dir_entries(dhf_host_fs_t *fs, int handle, void *buf, size_t count, uint8_t *status) {
    unsigned char *out = (unsigned char*)buf;
    size_t produced = 0;
    while (produced + DHF_DIRENT_SIZE <= count) {
        struct dirent *de = readdir(fs->handles[handle].dir);
        if (!de) break;
        unsigned char rec[DHF_DIRENT_SIZE];
        memset(rec, 0, sizeof(rec));
        size_t nlen = strlen(de->d_name);
        if (nlen > 28) nlen = 28;
        memcpy(rec, de->d_name, nlen);
        if (nlen > 0) rec[nlen - 1] |= 0x80;
        memcpy(out + produced, rec, DHF_DIRENT_SIZE);
        produced += DHF_DIRENT_SIZE;
    }
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
    if (!fs || handle < 0 || handle >= DHF_MAX_HANDLES || !fs->handles[handle].in_use || fs->handles[handle].is_dir) {
        if (status) *status = DHF_ERR_BAD_PATH;
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
    if (!fs || handle < 0 || handle >= DHF_MAX_HANDLES || !fs->handles[handle].in_use || fs->handles[handle].is_dir) {
        if (status) *status = DHF_ERR_BAD_PATH;
        return -1;
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

    size_t i = 0;
    while (i < maxlen - 1) {
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
    buf[i] = '\0';
    /* s. dhf_host_fs_read: 0 Bytes UND wirkliches Dateiende (keine Zeile, auch nicht
       unvollstaendig, gelesen) muss E$EOF melden, sonst endlose I$ReadLn-Wiederholung. */
    if (i == 0) {
        if (status) *status = DHF_ERR_EOF;
        return 0;
    }
    if (status) *status = DHF_ERR_OK;
    return (int)i;
}

int dhf_host_fs_writeln(dhf_host_fs_t *fs, int handle, const char *buf, size_t len, uint8_t *status) {
    ssize_t w = dhf_host_fs_write(fs, handle, buf, len, status);
    if (w < 0) return -1;
    char nl = '\n';
    if (len == 0 || buf[len - 1] != '\n') {
        write(fs->handles[handle].fd, &nl, 1);
        w++;
    }
    return (int)w;
}

int dhf_host_fs_getstat(dhf_host_fs_t *fs, const char *path, void *statbuf, size_t *out_size, uint8_t *status) {
    char target[DHF_PATH_MAX];
    if (resolve_confined_path(fs, path, target, sizeof(target)) != 0) {
        if (status) *status = DHF_ERR_NO_PERMISSION;
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
    fields[0] = htonl((uint32_t)st.st_size);
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
        if (status) *status = DHF_ERR_NO_PERMISSION;
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

    unsigned char fd[DHF_FD_SECTOR_SIZE];
    memset(fd, 0, sizeof(fd));

    unsigned char att = 0;
    if (st.st_mode & S_IRUSR) att |= 0x01;
    if (st.st_mode & S_IWUSR) att |= 0x02;
    if (st.st_mode & S_IXUSR) att |= 0x04;
    if (st.st_mode & S_IROTH) att |= 0x08;
    if (st.st_mode & S_IWOTH) att |= 0x10;
    if (st.st_mode & S_IXOTH) att |= 0x20;
    if (S_ISDIR(st.st_mode))  att |= 0x80;
    fd[0x00] = att;                         /* FD_ATT */
    fd[0x01] = 0; fd[0x02] = 0;             /* FD_OWN */

    struct tm tmv;
    gmtime_r(&st.st_mtime, &tmv);
    fd[0x03] = (unsigned char)tmv.tm_year;  /* FD_DAT: Jahr (seit 1900, wie tm_year) */
    fd[0x04] = (unsigned char)(tmv.tm_mon + 1);
    fd[0x05] = (unsigned char)tmv.tm_mday;
    fd[0x06] = (unsigned char)tmv.tm_hour;
    fd[0x07] = (unsigned char)tmv.tm_min;

    fd[0x08] = 1;                           /* FD_LNK */

    uint32_t size_be = htonl((uint32_t)st.st_size);
    memcpy(&fd[0x09], &size_be, 4);         /* FD_SIZ */

    gmtime_r(&st.st_ctime, &tmv);
    fd[0x0D] = (unsigned char)tmv.tm_year;  /* FD_CREAT: Jahr/Monat/Tag */
    fd[0x0E] = (unsigned char)(tmv.tm_mon + 1);
    fd[0x0F] = (unsigned char)tmv.tm_mday;
    /* FD_SEG (Offset $10, 240 Byte) bleibt genullt */

    size_t n = want_len;
    if (n > sizeof(fd)) n = sizeof(fd);
    memcpy(buf, fd, n);
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
    if (!fs || handle < 0 || handle >= DHF_MAX_HANDLES || !fs->handles[handle].in_use) {
        if (status) *status = DHF_ERR_BAD_PATH;
        return -1;
    }
    mode_t mode = 0;
    if (attr & 0x01) mode |= S_IRUSR;
    if (attr & 0x02) mode |= S_IWUSR;
    if (attr & 0x04) mode |= S_IXUSR;
    if (attr & 0x08) mode |= S_IROTH;
    if (attr & 0x10) mode |= S_IWOTH;
    if (attr & 0x20) mode |= S_IXOTH;

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
    if (!fs || handle < 0 || handle >= DHF_MAX_HANDLES || !fs->handles[handle].in_use ||
        fs->handles[handle].is_dir) {
        if (status) *status = DHF_ERR_BAD_PATH;
        return -1;
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
    if (!fs || handle < 0 || handle >= DHF_MAX_HANDLES || !fs->handles[handle].in_use ||
        fs->handles[handle].is_dir) {
        if (status) *status = DHF_ERR_BAD_PATH;
        return -1;
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

int dhf_host_fs_chdir(dhf_host_fs_t *fs, const char *path, uint8_t *status) {
    char target[DHF_PATH_MAX];
    if (resolve_confined_path(fs, path, target, sizeof(target)) != 0) {
        if (status) *status = DHF_ERR_NO_PERMISSION;
        return -1;
    }

    struct stat st;
    if (stat(target, &st) != 0 || !S_ISDIR(st.st_mode)) {
        if (status) *status = DHF_ERR_NOT_DIR;
        return -1;
    }

    /* Compute new cwd relative to basepath */
    size_t base_len = strlen(fs->basepath);
    if (strlen(target) > base_len) {
        const char *rel = target + base_len;
        if (*rel == '/') rel++;
        strncpy(fs->cwd, rel, sizeof(fs->cwd) - 1);
    } else {
        fs->cwd[0] = '\0';
    }

    if (status) *status = DHF_ERR_OK;
    return 0;
}

int dhf_host_fs_mkdir(dhf_host_fs_t *fs, const char *path, int mode, uint8_t *status) {
    char target[DHF_PATH_MAX];
    if (resolve_confined_path(fs, path, target, sizeof(target)) != 0) {
        if (status) *status = DHF_ERR_NO_PERMISSION;
        return -1;
    }

    mode_t pmode = (mode == 0) ? 0755 : (mode_t)mode;
    if (mkdir(target, pmode) != 0) {
        if (status) *status = errno_to_dhf(errno);
        return -1;
    }
    if (status) *status = DHF_ERR_OK;
    return 0;
}

int dhf_host_fs_rmdir(dhf_host_fs_t *fs, const char *path, uint8_t *status) {
    char target[DHF_PATH_MAX];
    if (resolve_confined_path(fs, path, target, sizeof(target)) != 0) {
        if (status) *status = DHF_ERR_NO_PERMISSION;
        return -1;
    }

    if (rmdir(target) != 0) {
        if (status) *status = errno_to_dhf(errno);
        return -1;
    }
    if (status) *status = DHF_ERR_OK;
    return 0;
}

int dhf_host_fs_unlink(dhf_host_fs_t *fs, const char *path, uint8_t *status) {
    char target[DHF_PATH_MAX];
    if (resolve_confined_path(fs, path, target, sizeof(target)) != 0) {
        if (status) *status = DHF_ERR_NO_PERMISSION;
        return -1;
    }

    if (unlink(target) != 0) {
        if (status) *status = errno_to_dhf(errno);
        return -1;
    }
    if (status) *status = DHF_ERR_OK;
    return 0;
}

int dhf_host_fs_rename(dhf_host_fs_t *fs, const char *oldp, const char *newp, uint8_t *status) {
    char target_old[DHF_PATH_MAX];
    char target_new[DHF_PATH_MAX];

    if (resolve_confined_path(fs, oldp, target_old, sizeof(target_old)) != 0 ||
        resolve_confined_path(fs, newp, target_new, sizeof(target_new)) != 0) {
        if (status) *status = DHF_ERR_NO_PERMISSION;
        return -1;
    }

    if (rename(target_old, target_new) != 0) {
        if (status) *status = errno_to_dhf(errno);
        return -1;
    }
    if (status) *status = DHF_ERR_OK;
    return 0;
}

/* 2026-09-26: I$SetStt SS_Rename, Handle-basiert -- die alte Datei ist bereits ueber ihre
   OS-9-Pfadnummer offen (kein Pfadname mehr bekannt, wie bei allen anderen `_at`-
   Funktionen), nur der neue Name kommt vom Aufrufer als String. Nutzt den beim Open/Create
   gespeicherten Host-Pfad (`fs->handles[handle].path`) als "alt"-Seite von rename(), und
   aktualisiert ihn danach, damit spaetere Aufrufe auf demselben Handle (z.B. ein
   anschliessendes GetStt/Close) nicht auf einen veralteten Pfad zeigen. */
int dhf_host_fs_rename_at(dhf_host_fs_t *fs, int handle, const char *newname, uint8_t *status) {
    if (!fs || handle < 0 || handle >= DHF_MAX_HANDLES || !fs->handles[handle].in_use) {
        if (status) *status = DHF_ERR_BAD_PATH;
        return -1;
    }
    char target_new[DHF_PATH_MAX];
    if (resolve_confined_path(fs, newname, target_new, sizeof(target_new)) != 0) {
        if (status) *status = DHF_ERR_NO_PERMISSION;
        return -1;
    }
    if (rename(fs->handles[handle].path, target_new) != 0) {
        if (status) *status = errno_to_dhf(errno);
        return -1;
    }
    strncpy(fs->handles[handle].path, target_new, sizeof(fs->handles[handle].path) - 1);
    fs->handles[handle].path[sizeof(fs->handles[handle].path) - 1] = '\0';
    if (status) *status = DHF_ERR_OK;
    return 0;
}

/* I$GetStt SS_Free -- freier Speicherplatz im Basisverzeichnis. Braucht kein Handle (wirkt
   auf das gesamte DHF-"Geraet", nicht auf eine einzelne Datei) -- der Aufrufer liefert per
   Spezifikation trotzdem eine Pfadnummer mit, die hier aber ungenutzt bleibt (DHF hat nur
   einen einzigen Basispfad je Deskriptor, keine Mehrfach-Volumes). Ergebnis auf 32 Bit
   gekappt (OS-9s d0.l ist 32 Bit; ein Host-Dateisystem kann theoretisch mehr freien Platz
   melden als das darstellbar ist). */
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
        if (status) *status = DHF_ERR_NO_PERMISSION;
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
