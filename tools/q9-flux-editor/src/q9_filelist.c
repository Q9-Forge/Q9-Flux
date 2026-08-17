//════════════════════════════════════════════════════════════════════════════════════════════════
// File:   q9_filelist.c                                                                   Ver. 1.00
// Owner:  Claudia
// Desc.:  Implementierung, siehe q9_filelist.h.
//
// Edition History
//─────────┬──────┬────────────────────────────────────────────────────────────────────────┬──────
// Date    │ Ver. │ Description                                                            │ By
//─────────┼──────┼────────────────────────────────────────────────────────────────────────┬──────
// 26-08-17│ 1.00 │ Erster Wurf                                                              │ Cld
//═════════╧══════╧════════════════════════════════════════════════════════════════════════╧══════
#include "q9_filelist.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

/* Kleiner, eigenstaendiger Gross-/Kleinschreibungs-unabhaengiger Vergleich -- kein strcasecmp
   (POSIX/BSD, nicht garantiert ueberall verfuegbar, s. gleiches Muster in src/kernel/
   q9boardrun.c q9_streq_ci -- bewusst eine eigene Kopie hier statt eines Cross-Modul-Imports aus
   dem Emulator-Kern, dieses Tool bleibt komplett eigenstaendig). */
static int streq_ci(const char *a, const char *b)
{
    while (*a && *b) {
        char ca = *a, cb = *b;
        if (ca >= 'A' && ca <= 'Z') { ca = (char)(ca - 'A' + 'a'); }
        if (cb >= 'A' && cb <= 'Z') { cb = (char)(cb - 'A' + 'a'); }
        if (ca != cb) { return 0; }
        a++; b++;
    }
    return *a == '\0' && *b == '\0';
}

static int ext_matches(const char *name, const char *ext_filter)
{
    const char *dot;
    const char *want;

    if (!ext_filter || !ext_filter[0]) { return 1; }
    if (strcmp(ext_filter, "*") == 0 || strcmp(ext_filter, "*.*") == 0) { return 1; }

    dot = strrchr(name, '.');
    if (!dot) { return 0; }                                 /* Datei ohne Endung -> kein Filter passt */

    want = ext_filter;
    if (want[0] == '.') { want++; }                         /* fuehrenden Punkt im Filter ignorieren  */
    dot++;                                                  /* fuehrenden Punkt im Namen ueberspringen */
    return streq_ci(dot, want);
}

static void format_size(long bytes, char *out, unsigned out_max)
{
    if (bytes < 1024L) {
        snprintf(out, out_max, "%ldB", bytes);
    } else if (bytes < 1024L * 1024) {
        snprintf(out, out_max, "%ldK", bytes / 1024);
    } else if (bytes < 1024L * 1024 * 1024) {
        snprintf(out, out_max, "%.1fM", (double)bytes / (1024.0 * 1024.0));
    } else {
        snprintf(out, out_max, "%.1fG", (double)bytes / (1024.0 * 1024.0 * 1024.0));
    }
}

static int compare_entries(const void *a, const void *b)
{
    const q9_fileentry_t *ea = (const q9_fileentry_t *)a;
    const q9_fileentry_t *eb = (const q9_fileentry_t *)b;
    return strcmp(ea->name, eb->name);
}

static void copy_name(q9_fileentry_t *e, const char *name)
{
    size_t nlen = strlen(name);
    if (nlen >= sizeof(e->name)) { nlen = sizeof(e->name) - 1; }
    memcpy(e->name, name, nlen);
    e->name[nlen] = '\0';
}

#ifdef _WIN32

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

int q9_filelist_scan(const char *dir, const char *ext_filter, q9_filelist_t *out)
{
    char pattern[1024];
    WIN32_FIND_DATAA fd;
    HANDLE h;

    if (!out) { return -1; }
    out->count = 0;
    if (!dir) { return -1; }

    snprintf(pattern, sizeof(pattern), "%s\\*", dir);
    h = FindFirstFileA(pattern, &fd);
    if (h == INVALID_HANDLE_VALUE) { return -1; }

    do {
        q9_fileentry_t *e;
        SYSTEMTIME st;

        if (fd.cFileName[0] == '.') { continue; }           /* versteckte Dateien + "."/".."       */
        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) { continue; }
        if (!ext_matches(fd.cFileName, ext_filter)) { continue; }
        if (out->count >= Q9_FILELIST_MAX_ENTRIES) { break; }

        e = &out->entry[out->count];
        copy_name(e, fd.cFileName);
        if (FileTimeToSystemTime(&fd.ftLastWriteTime, &st)) {
            snprintf(e->date, sizeof(e->date), "%02d.%02d.%02d", st.wDay, st.wMonth, st.wYear % 100);
        } else {
            snprintf(e->date, sizeof(e->date), "-");
        }
        e->size_bytes = (long)(((unsigned long long)fd.nFileSizeHigh << 32) | fd.nFileSizeLow);
        format_size(e->size_bytes, e->size, sizeof(e->size));
        out->count++;
    } while (FindNextFileA(h, &fd));

    FindClose(h);
    qsort(out->entry, (size_t)out->count, sizeof(out->entry[0]), compare_entries);
    return out->count;
}

#else /* POSIX */

#include <dirent.h>
#include <sys/stat.h>
#include <time.h>

int q9_filelist_scan(const char *dir, const char *ext_filter, q9_filelist_t *out)
{
    DIR *d;
    struct dirent *de;

    if (!out) { return -1; }
    out->count = 0;
    if (!dir) { return -1; }

    d = opendir(dir);
    if (!d) { return -1; }

    while (out->count < Q9_FILELIST_MAX_ENTRIES && (de = readdir(d)) != NULL) {
        char path[1024];
        struct stat st;
        q9_fileentry_t *e;
        struct tm *tmv;

        if (de->d_name[0] == '.') { continue; }             /* versteckte Dateien + "."/".."       */
        if (!ext_matches(de->d_name, ext_filter)) { continue; }

        snprintf(path, sizeof(path), "%s/%s", dir, de->d_name);
        if (stat(path, &st) != 0) { continue; }
        if (!S_ISREG(st.st_mode)) { continue; }             /* keine Verzeichnisse/Sonderdateien    */

        e = &out->entry[out->count];
        copy_name(e, de->d_name);
        tmv = localtime(&st.st_mtime);
        if (tmv) {
            snprintf(e->date, sizeof(e->date), "%02d.%02d.%02d",
                     tmv->tm_mday, tmv->tm_mon + 1, tmv->tm_year % 100);
        } else {
            snprintf(e->date, sizeof(e->date), "-");
        }
        e->size_bytes = (long)st.st_size;
        format_size(e->size_bytes, e->size, sizeof(e->size));
        out->count++;
    }
    closedir(d);

    qsort(out->entry, (size_t)out->count, sizeof(out->entry[0]), compare_entries);
    return out->count;
}

#endif /* _WIN32 */

//────────────────────────────────────────────────────────────────────────────────────────────────
// EOF q9_filelist.c                                                                       Ver. 1.00
//────────────────────────────────────────────────────────────────────────────────────────────────
