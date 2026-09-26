/* dhf_host_fs.h - Host Filesystem implementation with path confinement
 * Executes POSIX filesystem operations safely within a base directory.
 */

#ifndef DHF_HOST_FS_H
#define DHF_HOST_FS_H

#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>

#define DHF_MAX_HANDLES 64
#define DHF_PATH_MAX    1024
#define DHF_DELDIR_SLOTS 8
#define DHF_DIRCACHE_SLOTS 32

typedef struct {
    char basepath[DHF_PATH_MAX];
    int  readonly;          /* 2026-09-26: Laufwerk nur lesbar (DevCon-Flag Bit 0) -> E$WP */
    char cwd[DHF_PATH_MAX];
    struct {
        int in_use;
        int is_dir;
        int fd;
        DIR *dir;
        uint32_t dir_pos;   /* Verzeichnis-Handles: Byteposition in der virtuellen RBF-Datei */
        int      is_raw;    /* "/<geraet>@": virtuelles Rohgeraet (LSN0 + Bitmap), nur lesen */
        char   **dir_names; /* Verzeichnis-Handles: Momentaufnahme der Eintraege beim Open */
        uint32_t dir_count; /* (ohne "."/".."); feste Positionen wie RBF, s. dhf_host_fs.c */
        char path[DHF_PATH_MAX];
    } handles[DHF_MAX_HANDLES];
    /* 2026-09-26: Pseudo-Sektornummern (Verzeichniseintraege Byte 29-31, SS_FDInf, pd_fd,
       P$DIO-Verzeichnis je Prozess): Nummer n (1..) steht fuer lsn_tab[n-1]. Dauerhaft fuer
       die Laufzeit (kein Ring -- ein wiederverwendeter Platz haette gemerkte Verzeichnisse
       von Prozessen auf fremde Pfade umgebogen), Suche per Hash. */
    char   **lsn_tab;
    uint32_t lsn_cnt, lsn_cap;
    uint32_t *lsn_hash;     /* offene Adressierung, Eintrag = Index+1, 0 = frei */
    uint32_t lsn_hcap;
    /* 2026-09-26: Verzeichnisse, deren Dir-Bit per SS_Attr entfernt wurde -- nur die darf
       I$Delete (wie RBF, so arbeitet "deldir": erst Attribut weg, dann loeschen). */
    char     deldir_ok[DHF_DELDIR_SLOTS][DHF_PATH_MAX];
    uint32_t deldir_next;
    /* 2026-09-26: stabile Eintragsplaetze je Verzeichnis ueber Handles hinweg (wie die
       Verzeichnisdatei auf RBF): geloeschte Eintraege bleiben freie Plaetze (NULL), neue
       belegen den ersten freien Platz oder kommen hinten dran. */
    struct {
        char     path[DHF_PATH_MAX];
        char   **names;
        uint32_t count;
        uint32_t used;      /* LRU-Zaehler */
    } dircache[DHF_DIRCACHE_SLOTS];
    uint32_t dircache_clock;
} dhf_host_fs_t;

/* Public API */
int     dhf_host_fs_init(dhf_host_fs_t *fs, const char *basepath);
void    dhf_host_fs_cleanup(dhf_host_fs_t *fs);

int     dhf_host_fs_open(dhf_host_fs_t *fs, const char *path, int flags, uint8_t *status);
int     dhf_host_fs_create(dhf_host_fs_t *fs, const char *path, int flags, int mode, uint8_t *status);
/* 2026-09-26: wie oben, aber mit vom Aufrufer vorgegebenem Index (OS-9-Pfadnummer) statt
 * automatischer Vergabe -- s. dhf_host_fs.c Kommentar bei alloc_handle_at. Fuer den
 * Q9-DHF-Manager (Q9-OS/Q9-DHF-68k/manager/dhfmgr_68k.a), der dadurch kein eigenes Handle
 * mehr merken muss. */
int     dhf_host_fs_open_at(dhf_host_fs_t *fs, int idx, const char *path, int flags, uint8_t *status);
int     dhf_host_fs_create_at(dhf_host_fs_t *fs, int idx, const char *path, int flags, int mode, uint8_t *status);
int     dhf_host_fs_close(dhf_host_fs_t *fs, int handle, uint8_t *status);

ssize_t dhf_host_fs_read(dhf_host_fs_t *fs, int handle, void *buf, size_t count, uint8_t *status);
ssize_t dhf_host_fs_write(dhf_host_fs_t *fs, int handle, const void *buf, size_t count, uint8_t *status);
off_t   dhf_host_fs_seek(dhf_host_fs_t *fs, int handle, off_t offset, int whence, uint8_t *status);

int     dhf_host_fs_readln(dhf_host_fs_t *fs, int handle, char *buf, size_t maxlen, uint8_t *status);
int     dhf_host_fs_writeln(dhf_host_fs_t *fs, int handle, const char *buf, size_t len, uint8_t *status);

int     dhf_host_fs_getstat(dhf_host_fs_t *fs, const char *path, void *statbuf, size_t *out_size, uint8_t *status);
int     dhf_host_fs_getstat_at(dhf_host_fs_t *fs, int handle, void *statbuf, size_t *out_size, uint8_t *status);
int     dhf_host_fs_setstat(dhf_host_fs_t *fs, const char *path, const void *statbuf, uint8_t *status);
int     dhf_host_fs_setsize_at(dhf_host_fs_t *fs, int handle, uint32_t new_size, uint8_t *status);
int     dhf_host_fs_getfd_at(dhf_host_fs_t *fs, int handle, void *buf, size_t want_len, size_t *out_len, uint8_t *status);
int     dhf_host_fs_setattr_at(dhf_host_fs_t *fs, int handle, uint8_t attr, uint8_t *status);
int     dhf_host_fs_getpos_at(dhf_host_fs_t *fs, int handle, uint32_t *out_pos, uint8_t *status);
int     dhf_host_fs_iseof_at(dhf_host_fs_t *fs, int handle, uint8_t *status);
int     dhf_host_fs_rename_at(dhf_host_fs_t *fs, int handle, const char *oldname, const char *newname, uint8_t *status);
uint32_t dhf_host_fs_container_lsn(dhf_host_fs_t *fs, const char *path);
int     dhf_host_fs_handle_lsns(dhf_host_fs_t *fs, int handle, uint32_t *self, uint32_t *parent);
int     dhf_host_fs_setfd_at(dhf_host_fs_t *fs, int handle, const unsigned char *fdimg, uint8_t *status);
int     dhf_host_fs_getfd_lsn(dhf_host_fs_t *fs, uint32_t lsn, void *buf, size_t want_len, size_t *out_len, uint8_t *status);
int     dhf_host_fs_volstore(dhf_host_fs_t *fs, uint32_t out[4], uint8_t *status);
int     dhf_host_fs_getfree(dhf_host_fs_t *fs, uint32_t *out_free, uint8_t *status);

int     dhf_host_fs_chdir(dhf_host_fs_t *fs, const char *path, uint32_t *out_lsn, uint8_t *status);
void    dhf_host_fs_set_cwd_lsn(dhf_host_fs_t *fs, uint32_t lsn);
int     dhf_host_fs_mkdir(dhf_host_fs_t *fs, const char *path, int mode, uint8_t *status);
int     dhf_host_fs_rmdir(dhf_host_fs_t *fs, const char *path, uint8_t *status);
int     dhf_host_fs_unlink(dhf_host_fs_t *fs, const char *path, uint8_t *status);
int     dhf_host_fs_rename(dhf_host_fs_t *fs, const char *oldp, const char *newp, uint8_t *status);

int     dhf_host_fs_opendir(dhf_host_fs_t *fs, const char *path, uint8_t *status);
int     dhf_host_fs_readdir(dhf_host_fs_t *fs, int handle, char *name_out, size_t max_name, uint32_t *size_out, uint32_t *mode_out, uint8_t *status);

#endif /* DHF_HOST_FS_H */
