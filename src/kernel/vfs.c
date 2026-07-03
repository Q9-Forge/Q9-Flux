//════════════════════════════════════════════════════════════════════════════════════════════════
// File:   vfs.c                                                                          Ver. 1.10
// Owner:  AF
// Desc.:  Q9 VFS-Schicht (Phase 3.2/3.4) — Pfad-Routing "/d0/pfad/datei": F$PrsNam trennt Geraet
//         von Rest-Pfad (name.c), danach entweder klassisches q9_path_open (kein File-Manager,
//         Rest muss leer sein — /term, /nil) oder Weiterreichen an dev->fm->open/create/makdir/
//         remove (File-Manager vorhanden — FAT16, seit 3.4 auch schreibend). Globales
//         Arbeitsverzeichnis fuer I$ChgDir: EIN statischer String (kein malloc), pro-Prozess-
//         Variante erst Phase 4.
//
// Call:   siehe vfs.h
//
// Edition History
//─────────┬──────┬────────────────────────────────────────────────────────────────────────┬──────
// Date    │ Ver. │ Description                                                            │ By
//─────────┼──────┼────────────────────────────────────────────────────────────────────────┼──────
// 26-07-04│ 1.00 │ 3.2: Initiale Version                                                  │ CF
// 26-07-04│ 1.10 │ 3.4: q9_vfs_create/makdir/remove (I$Create/I$MakDir/I$Delete ueber      │ CF
//         │      │ dev->fm->create/makdir/remove, FAT16 schreibend); split_dev_rest als    │ CF
//         │      │ gemeinsamer Unterbau fuer alle vier Vfs-Ops extrahiert                  │ CF
//═════════╧══════╧════════════════════════════════════════════════════════════════════════╧══════

#include "vfs.h"
#include "name.h"
#include "syscall.h"

//╔══════════════════════════════════════════════════════════════════════════════════════════════╗
//║ KERNEL STATE                                                                                 ║
//╚══════════════════════════════════════════════════════════════════════════════════════════════╝

static char cwd[Q9_CWD_MAXLEN + 1] = "/";              /* globales Arbeitsverzeichnis (3.2)       */

//╔══════════════════════════════════════════════════════════════════════════════════════════════╗
//║ INTERNAL HELPERS                                                                             ║
//╚══════════════════════════════════════════════════════════════════════════════════════════════╝

static uint32_t str_len(const char *s)
{
    uint32_t n = 0;
    while (s[n]) {
        n++;
    }
    return n;
}

//────────────────────────────────────────────────────────────────────────────────────────────────
// Function: resolve
// Desc.:    Löst "pathlist" gegen das globale Arbeitsverzeichnis auf: beginnt es mit '/', bleibt
//           es unverändert (absolut); sonst wird cwd davorgesetzt (mit '/' verbunden, falls cwd
//           nicht selbst schon mit '/' endet). Ergebnis in "out" (mind. Q9_CWD_MAXLEN+80 Byte).
//           E$PNNF, wenn das Ergebnis zu lang wäre.
//────────────────────────────────────────────────────────────────────────────────────────────────
static int resolve(const char *pathlist, char *out, uint32_t outsz)
{
    uint32_t clen;
    uint32_t plen = str_len(pathlist);

    if (pathlist[0] == '/') {
        if (plen >= outsz) {
            return E_PNNF;
        }
        for (uint32_t i = 0; i <= plen; i++) {
            out[i] = pathlist[i];
        }
        return 0;
    }
    clen = str_len(cwd);
    if (clen > 0 && cwd[clen - 1] == '/') {
        clen--;                                        /* keinen doppelten Slash einbauen         */
    }
    if (clen + 1 + plen >= outsz) {
        return E_PNNF;
    }
    for (uint32_t i = 0; i < clen; i++) {
        out[i] = cwd[i];
    }
    out[clen] = '/';
    for (uint32_t i = 0; i <= plen; i++) {
        out[clen + 1 + i] = pathlist[i];
    }
    return 0;
}

//────────────────────────────────────────────────────────────────────────────────────────────────
// Function: split_dev_rest
// Desc.:    Gemeinsamer Unterbau fuer open/create/makdir/remove: loest "pathlist" gegen das
//           globale Arbeitsverzeichnis auf (resolve), trennt per F$PrsNam Geraetename/Rest-Pfad,
//           haengt das Geraet an (q9_dev_attach). "*restout" zeigt danach auf den Rest-Pfad OHNE
//           fuehrenden '/' (leer = Geraete-Wurzel). Der Aufrufer MUSS bei Erfolg q9_dev_detach()
//           aufrufen, sobald das Geraet nicht mehr gebraucht wird (bei q9_vfs_open/create haelt
//           der offene Pfad selbst den Link, bei makdir/remove detacht diese Funktion direkt
//           nach getaner Arbeit).
//────────────────────────────────────────────────────────────────────────────────────────────────
static int split_dev_rest(const char *pathlist, q9_dev_t **devout, const char **restout)
{
    static char resolved[Q9_CWD_MAXLEN + 80];              /* kein malloc — kein Reentrancy-Bedarf */
    const char *devname;
    uint32_t    devlen;
    const char *rest;
    int         err;

    err = resolve(pathlist, resolved, sizeof(resolved));
    if (err != 0) {
        return err;
    }
    err = q9_name_parse(resolved, &devname, &devlen);       /* erstes Element = Geraetename         */
    if (err != 0) {
        return err;
    }
    rest = devname + devlen;                                /* Rest inkl. evtl. fuehrendem '/'      */
    if (*rest == '/') {
        rest++;
    }
    err = q9_dev_attach(devname, devout);
    if (err != 0) {
        return err;
    }
    *restout = rest;
    return 0;
}

//╔══════════════════════════════════════════════════════════════════════════════════════════════╗
//║ API                                                                                          ║
//╚══════════════════════════════════════════════════════════════════════════════════════════════╝

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_vfs_open
// Desc.:    I$Open-Unterbau: Geraet + Rest-Pfad per F$PrsNam trennen, dann entweder klassisches
//           q9_path_open (kein File-Manager) oder dev->fm->open (File-Manager vorhanden).
// Call:     path = q9_vfs_open("/d0/pfad/datei", Q9_MODE_READ)
//════════════════════════════════════════════════════════════════════════════════════════════════
int q9_vfs_open(const char *pathlist, uint8_t mode)
{
    char        resolved[Q9_CWD_MAXLEN + 80];
    const char *devname;
    uint32_t    devlen;
    const char *rest;
    int         err;
    q9_dev_t   *dev;
    int         path;

    if (mode == 0) {
        return -E_BMODE;
    }
    err = resolve(pathlist, resolved, sizeof(resolved));
    if (err != 0) {
        return -err;
    }
    err = q9_name_parse(resolved, &devname, &devlen);   /* erstes Pathlist-Element = Geraetename  */
    if (err != 0) {
        return -err;
    }
    rest = devname + devlen;                            /* Rest inkl. evtl. fuehrendem '/'         */

    err = q9_dev_attach(devname, &dev);                  /* q9_dev_attach parst den Namen selbst   */
    if (err != 0) {                                      /*   erneut (name.c) — genau EIN Element  */
        return -err;
    }

    if (!dev->fm) {                                      /* kein File-Manager: altes Verhalten     */
        if (*rest != 0) {                                 /*   (/term, /nil) — Rest MUSS leer sein  */
            q9_dev_detach(dev);
            return -E_PNNF;
        }
        path = q9_path_open_dev(dev, mode);
        if (path < 0) {
            q9_dev_detach(dev);
        }
        return path;
    }

    /* File-Manager vorhanden (ab 3.3: FAT16) — Rest-Pfad an dessen open()-Op weiterreichen. */
    path = q9_path_open_dev(dev, mode);
    if (path < 0) {
        q9_dev_detach(dev);
        return path;
    }
    if (*rest == '/') {
        rest++;
    }
    err = dev->fm->open(dev, q9_path_get((uint32_t)path), rest, str_len(rest), mode);
    if (err != 0) {
        q9_path_close((uint32_t)path);                    /* detacht dev ueber q9_path_close        */
        return -err;
    }
    return path;
}

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_vfs_create
// Desc.:    I$Create-Unterbau (3.4): wie q9_vfs_open, aber ruft dev->fm->create() statt open().
// Call:     path = q9_vfs_create("/d0/NEU.TXT", Q9_MODE_WRITE)
//════════════════════════════════════════════════════════════════════════════════════════════════
int q9_vfs_create(const char *pathlist, uint8_t mode)
{
    q9_dev_t   *dev;
    const char *rest;
    int         err;
    int         path;

    if (mode == 0) {
        return -E_BMODE;
    }
    err = split_dev_rest(pathlist, &dev, &rest);
    if (err != 0) {
        return -err;
    }
    if (!dev->fm || !dev->fm->create) {                     /* Geraet kennt kein I$Create           */
        q9_dev_detach(dev);
        return -E_UNKSVC;
    }
    path = q9_path_open_dev(dev, mode);
    if (path < 0) {
        q9_dev_detach(dev);
        return path;
    }
    err = dev->fm->create(dev, q9_path_get((uint32_t)path), rest, str_len(rest), mode);
    if (err != 0) {
        q9_path_close((uint32_t)path);                      /* detacht dev ueber q9_path_close      */
        return -err;
    }
    return path;
}

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_vfs_makdir
// Desc.:    I$MakDir-Unterbau (3.4): Geraet/Rest trennen, dev->fm->makdir() aufrufen.
// Call:     err = q9_vfs_makdir("/d0/NEUDIR")
//════════════════════════════════════════════════════════════════════════════════════════════════
int q9_vfs_makdir(const char *pathlist)
{
    q9_dev_t   *dev;
    const char *rest;
    int         err;

    err = split_dev_rest(pathlist, &dev, &rest);
    if (err != 0) {
        return err;
    }
    if (!dev->fm || !dev->fm->makdir) {
        q9_dev_detach(dev);
        return E_UNKSVC;
    }
    err = dev->fm->makdir(dev, rest, str_len(rest));
    q9_dev_detach(dev);
    return err;
}

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_vfs_remove
// Desc.:    I$Delete-Unterbau (3.4): Geraet/Rest trennen, dev->fm->remove() aufrufen.
// Call:     err = q9_vfs_remove("/d0/ALT.TXT")
//════════════════════════════════════════════════════════════════════════════════════════════════
int q9_vfs_remove(const char *pathlist)
{
    q9_dev_t   *dev;
    const char *rest;
    int         err;

    err = split_dev_rest(pathlist, &dev, &rest);
    if (err != 0) {
        return err;
    }
    if (!dev->fm || !dev->fm->remove) {
        q9_dev_detach(dev);
        return E_UNKSVC;
    }
    err = dev->fm->remove(dev, rest, str_len(rest));
    q9_dev_detach(dev);
    return err;
}

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_vfs_chdir
// Desc.:    I$ChgDir-Unterbau: setzt das globale Arbeitsverzeichnis. 0 = ok, E$PNNF = leer/zu lang.
// Call:     err = q9_vfs_chdir("/d0/pfad")
//════════════════════════════════════════════════════════════════════════════════════════════════
int q9_vfs_chdir(const char *pathlist)
{
    char     resolved[Q9_CWD_MAXLEN + 80];
    uint32_t len;
    int      err = resolve(pathlist, resolved, sizeof(resolved));

    if (err != 0) {
        return err;
    }
    len = str_len(resolved);
    if (len == 0 || len > Q9_CWD_MAXLEN) {
        return E_PNNF;
    }
    for (uint32_t i = 0; i <= len; i++) {
        cwd[i] = resolved[i];
    }
    return 0;
}

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_vfs_cwd
// Desc.:    Liefert das aktuelle globale Arbeitsverzeichnis.
// Call:     cwd = q9_vfs_cwd()
//════════════════════════════════════════════════════════════════════════════════════════════════
const char *q9_vfs_cwd(void)
{
    return cwd;
}

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_dev_set_fm
// Desc.:    Traegt einen File-Manager an einem Geraet ein.
// Call:     q9_dev_set_fm(dev, &my_fm)
//════════════════════════════════════════════════════════════════════════════════════════════════
void q9_dev_set_fm(q9_dev_t *dev, const q9_fm_t *fm)
{
    if (dev) {
        dev->fm = fm;
    }
}

//────────────────────────────────────────────────────────────────────────────────────────────────
// EOF vfs.c                                                                               Ver. 1.10
//────────────────────────────────────────────────────────────────────────────────────────────────
