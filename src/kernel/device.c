//════════════════════════════════════════════════════════════════════════════════════════════════
// File:   device.c                                                                        Ver. 1.50
// Owner:  AF
// Desc.:  Q9 Device-Modell — Geräte- und Pfadtabelle (OS-9-Vorbild: IOMan). Registriert die
//         internen Treiber-Module und verwaltet offene Pfade. Kein malloc, alles statisch.
//
// Call:   q9_dev_init() beim Boot; danach q9_path_get()/q9_path_open()/q9_path_close()
//
// Edition History
//─────────┬──────┬────────────────────────────────────────────────────────────────────────┬──────
// Date    │ Ver. │ Description                                                            │ By
//─────────┼──────┼────────────────────────────────────────────────────────────────────────┼──────
// 26-07-03│ 1.00 │ Initiale Version: Tabellen, open/close/get, /term auf Pfaden 0/1/2     │ CF
// 26-07-03│ 1.10 │ 1.4: q9_path_dup (niedrigste freie Nummer, OS-9-Semantik)              │ CF
// 26-07-03│ 1.20 │ 1.6: q9_dev_attach/detach ueber Pathlist-Namen                         │ CF
// 26-07-03│ 1.30 │ 1.7: /nil registriert, q9_path_open namensbasiert                      │ CF
// 26-07-03│ 1.40 │ 3.1: /d0 registriert (Roh-Block-Device)                                │ CF
// 26-07-04│ 1.50 │ 3.2: dev_add setzt dev->fm = 0 (File-Manager kommt ueber vfs.c dazu)   │ CF
//═════════╧══════╧════════════════════════════════════════════════════════════════════════╧══════

#include "device.h"
#include "name.h"
#include "syscall.h"

extern const q9_drv_t q9_drv_term;                     /* internal driver modules (dev_term.c,   */
extern const q9_drv_t q9_drv_nil;                      /*   dev_nil.c, dev_d0.c)                 */
extern const q9_drv_t q9_drv_d0;

//╔══════════════════════════════════════════════════════════════════════════════════════════════╗
//║ KERNEL TABLES                                                                                ║
//╚══════════════════════════════════════════════════════════════════════════════════════════════╝

static q9_dev_t  devtab[Q9_NDEVS];
static q9_path_t pathtab[Q9_NPATHS];

//╔══════════════════════════════════════════════════════════════════════════════════════════════╗
//║ INTERNAL HELPERS                                                                             ║
//╚══════════════════════════════════════════════════════════════════════════════════════════════╝

static int str_eq(const char *a, const char *b)
{
    while (*a && *a == *b) {
        a++;
        b++;
    }
    return *a == *b;
}

static uint32_t str_len(const char *s)
{
    uint32_t n = 0;
    while (s[n]) {
        n++;
    }
    return n;
}

//────────────────────────────────────────────────────────────────────────────────────────────────
// Function: dev_add
// Desc.:    Trägt ein Treiber-Modul als Gerät in die Gerätetabelle ein und ruft dessen init.
// Call:     dev_add("term", &q9_drv_term)
//────────────────────────────────────────────────────────────────────────────────────────────────
static int dev_add(const char *name, const q9_drv_t *drv)
{
    for (int i = 0; i < Q9_NDEVS; i++) {
        if (!devtab[i].drv) {
            devtab[i].name    = name;
            devtab[i].drv     = drv;
            devtab[i].fm      = 0;                     /* kein File-Manager (vfs.c traegt das ein) */
            devtab[i].storage = 0;
            devtab[i].links   = 0;
            return drv->init(&devtab[i]);
        }
    }
    return E_PTHFUL;                                   /* device table full                      */
}

//╔══════════════════════════════════════════════════════════════════════════════════════════════╗
//║ API                                                                                          ║
//╚══════════════════════════════════════════════════════════════════════════════════════════════╝

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_dev_init
// Desc.:    Registriert die internen Treiber-Module und öffnet die Standardpfade 0/1/2
//           auf /term im Update-Modus (wie OS-9-Shell-Standardpfade).
// Call:     err = q9_dev_init()
//════════════════════════════════════════════════════════════════════════════════════════════════
int q9_dev_init(void)
{
    int err = dev_add("term", &q9_drv_term);
    if (err != 0) {
        return err;
    }
    err = dev_add("nil", &q9_drv_nil);
    if (err != 0) {
        return err;
    }
    err = dev_add("d0", &q9_drv_d0);
    if (err != 0) {
        return err;
    }
    for (int i = 0; i < 3; i++) {                      /* stdin/stdout/stderr                    */
        if (q9_path_open("term", Q9_MODE_UPDATE) != i) {
            return E_PTHFUL;
        }
    }
    return 0;
}

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_dev_find
// Desc.:    Sucht ein Gerät per Name. NULL = nicht vorhanden.
// Call:     dev = q9_dev_find("term")
//════════════════════════════════════════════════════════════════════════════════════════════════
q9_dev_t *q9_dev_find(const char *name)
{
    for (int i = 0; i < Q9_NDEVS; i++) {
        if (devtab[i].drv && str_eq(devtab[i].name, name)) {
            return &devtab[i];
        }
    }
    return 0;
}

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_dev_attach
// Desc.:    Gerät per Pathlist-Name suchen ("/term" oder "term", case-insensitiv) und
//           Link-Count erhöhen. 0 = ok, E$BPNam = ungültiger Name, E$MNF = unbekannt.
// Call:     err = q9_dev_attach("/term", &dev)
//════════════════════════════════════════════════════════════════════════════════════════════════
int q9_dev_attach(const char *pathlist, q9_dev_t **out)
{
    const char *nm;
    uint32_t    len;
    int         err = q9_name_parse(pathlist, &nm, &len);

    if (err != 0) {
        return err;
    }
    for (int i = 0; i < Q9_NDEVS; i++) {
        if (devtab[i].drv && str_len(devtab[i].name) == len &&
            q9_name_cmp(nm, len, devtab[i].name) == 0) {
            devtab[i].links++;
            *out = &devtab[i];
            return 0;
        }
    }
    return E_MNF;                                      /* no device descriptor of that name      */
}

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_dev_detach
// Desc.:    Per q9_dev_attach geholtes Gerät freigeben (Link-Count runter).
//           E$Param, wenn der Zeiger kein gültiger Gerätetabellen-Eintrag ist.
// Call:     err = q9_dev_detach(dev)
//════════════════════════════════════════════════════════════════════════════════════════════════
int q9_dev_detach(q9_dev_t *dev)
{
    for (int i = 0; i < Q9_NDEVS; i++) {
        if (dev == &devtab[i] && devtab[i].drv) {
            if (devtab[i].links > 0) {
                devtab[i].links--;
            }
            return 0;
        }
    }
    return E_PARAM;
}

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_path_open
// Desc.:    Öffnet einen Pfad auf ein Gerät (niedrigste freie Pfadnummer, wie OS-9).
//           Rückgabe >= 0: Pfadnummer, < 0: negierter Fehlercode.
// Call:     path = q9_path_open("term", Q9_MODE_UPDATE)
//════════════════════════════════════════════════════════════════════════════════════════════════
int q9_path_open(const char *devname, uint8_t mode)
{
    q9_dev_t *dev;
    int       err;

    if (mode == 0) {
        return -E_BMODE;
    }
    err = q9_dev_attach(devname, &dev);                /* name lookup + links++                  */
    if (err != 0) {
        return -err;
    }
    err = q9_path_open_dev(dev, mode);
    if (err < 0) {
        q9_dev_detach(dev);
    }
    return err;
}

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_path_open_dev
// Desc.:    Öffnet einen Pfad auf ein BEREITS aufgelöstes Gerät (kein Namens-Lookup, kein
//           zusätzliches Attach) — Unterbau für q9_path_open und die VFS-Schicht (vfs.c), die
//           das Gerät schon über F$PrsNam+q9_dev_attach ermittelt hat. Rückgabe >= 0: Pfadnummer,
//           < 0: negierter Fehlercode (E$PthFul bei voller Pfadtabelle; Aufrufer muss bei
//           Fehlschlag selbst q9_dev_detach(dev) nachholen, falls er zuvor attach'ed hat).
// Call:     path = q9_path_open_dev(dev, Q9_MODE_READ)
//════════════════════════════════════════════════════════════════════════════════════════════════
int q9_path_open_dev(q9_dev_t *dev, uint8_t mode)
{
    for (int i = 0; i < Q9_NPATHS; i++) {
        if (!pathtab[i].dev) {
            pathtab[i].dev  = dev;
            pathtab[i].mode = mode;
            for (uint32_t j = 0; j < Q9_FMCTX_SIZE; j++) {
                pathtab[i].fmctx[j] = 0;
            }
            return i;
        }
    }
    return -E_PTHFUL;
}

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_path_dup
// Desc.:    Dupliziert einen offenen Pfad auf die niedrigste freie Pfadnummer (OS-9-Semantik).
//           Rückgabe >= 0: neue Pfadnummer, < 0: negierter Fehlercode.
// Call:     newpath = q9_path_dup(path)
//════════════════════════════════════════════════════════════════════════════════════════════════
int q9_path_dup(uint32_t path)
{
    q9_path_t *p = q9_path_get(path);

    if (!p) {
        return -E_BPNUM;
    }
    for (int i = 0; i < Q9_NPATHS; i++) {
        if (!pathtab[i].dev) {
            pathtab[i] = *p;
            p->dev->links++;
            return i;
        }
    }
    return -E_PTHFUL;
}

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_path_close
// Desc.:    Schließt einen Pfad und gibt den Tabelleneintrag frei. 0 = ok, sonst E$BPNum.
// Call:     err = q9_path_close(path)
//════════════════════════════════════════════════════════════════════════════════════════════════
int q9_path_close(uint32_t path)
{
    q9_path_t *p = q9_path_get(path);

    if (!p) {
        return E_BPNUM;
    }
    p->dev->links--;
    p->dev  = 0;
    p->mode = 0;
    return 0;
}

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_path_get
// Desc.:    Pfadnummer -> Deskriptor. NULL = ungültig oder nicht offen.
// Call:     p = q9_path_get(pathnum)
//════════════════════════════════════════════════════════════════════════════════════════════════
q9_path_t *q9_path_get(uint32_t path)
{
    if (path >= Q9_NPATHS || !pathtab[path].dev) {
        return 0;
    }
    return &pathtab[path];
}

//────────────────────────────────────────────────────────────────────────────────────────────────
// EOF device.c                                                                            Ver. 1.50
//────────────────────────────────────────────────────────────────────────────────────────────────
