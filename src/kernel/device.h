//════════════════════════════════════════════════════════════════════════════════════════════════
// File:   device.h                                                                        Ver. 1.20
// Owner:  AF
// Desc.:  Q9 Device-Modell (OS-9-Vorbild: IOMan). Gerätetabelle + Pfadtabelle im Kernel,
//         Treiber sind interne Module mit einheitlichen I/O-Operationen (q9_drv_t).
//         Kernel-interne Schnittstelle — User-Sicht läuft über die I$-Syscalls.
//
// Call:   q9_dev_init(); p = q9_path_get(pathnum); p->dev->drv->write(...)
//
// Edition History
//─────────┬──────┬────────────────────────────────────────────────────────────────────────┬──────
// Date    │ Ver. │ Description                                                            │ By
//─────────┼──────┼────────────────────────────────────────────────────────────────────────┼──────
// 26-07-03│ 1.00 │ Initiale Version: Geräte-/Pfadtabelle, Treiber-Ops, Modi               │ CF
// 26-07-03│ 1.10 │ 1.4: q9_path_dup ergänzt                                               │ CF
// 26-07-03│ 1.20 │ 1.6: q9_dev_attach/detach (namensbasiert)                              │ CF
//═════════╧══════╧════════════════════════════════════════════════════════════════════════╧══════
#ifndef Q9_DEVICE_H
#define Q9_DEVICE_H

#include <stdint.h>

#define Q9_NDEVS   4                                   /* device table size                      */
#define Q9_NPATHS  8                                   /* path table size                        */

#define Q9_MODE_READ   0x01                            /* access modes (OS-9 style)              */
#define Q9_MODE_WRITE  0x02
#define Q9_MODE_UPDATE (Q9_MODE_READ | Q9_MODE_WRITE)

typedef struct q9_dev q9_dev_t;

//╔══════════════════════════════════════════════════════════════════════════════════════════════╗
//║ DRIVER OPERATIONS (internal module interface)                                                ║
//╚══════════════════════════════════════════════════════════════════════════════════════════════╝
//  Alle I/O-Ops: *n ist in/out (max. Bytes rein, tatsächliche Bytes raus).
//  Rückgabe 0 = Erfolg, sonst OS-9-Fehlercode (z.B. E$NotRdy).

typedef struct q9_drv {
    const char *name;                                  /* driver module name                     */
    int (*init)  (q9_dev_t *dev);                      /* attach static storage, init hardware   */
    int (*read)  (q9_dev_t *dev, uint8_t *buf, uint32_t *n);        /* raw, no echo             */
    int (*write) (q9_dev_t *dev, const uint8_t *buf, uint32_t *n);  /* raw/cooked per driver    */
    int (*readln)(q9_dev_t *dev, uint8_t *buf, uint32_t *n);        /* line incl. CR, edited    */
    int (*writln)(q9_dev_t *dev, const uint8_t *buf, uint32_t *n);  /* stops after CR/LF        */
} q9_drv_t;

//╔══════════════════════════════════════════════════════════════════════════════════════════════╗
//║ DEVICE TABLE ENTRY (like OS-9 device descriptor + static storage)                            ║
//╚══════════════════════════════════════════════════════════════════════════════════════════════╝

struct q9_dev {
    const char     *name;                              /* device name, e.g. "term"               */
    const q9_drv_t *drv;                               /* driver module                          */
    void           *storage;                           /* driver static storage (set by init)    */
    uint8_t         links;                             /* open path count                        */
};

//╔══════════════════════════════════════════════════════════════════════════════════════════════╗
//║ PATH DESCRIPTOR                                                                              ║
//╚══════════════════════════════════════════════════════════════════════════════════════════════╝

typedef struct q9_path {
    q9_dev_t *dev;                                     /* NULL = entry free                      */
    uint8_t   mode;                                    /* Q9_MODE_...                            */
} q9_path_t;

//╔══════════════════════════════════════════════════════════════════════════════════════════════╗
//║ API (kernel-internal)                                                                        ║
//╚══════════════════════════════════════════════════════════════════════════════════════════════╝

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_dev_init
// Desc.:    Baut die Gerätetabelle auf (interne Treiber-Module) und öffnet die Standardpfade
//           0/1/2 auf /term (Update-Modus, wie OS-9-Shell-Standardpfade). Einmalig beim Boot.
// Call:     err = q9_dev_init()
//════════════════════════════════════════════════════════════════════════════════════════════════
int q9_dev_init(void);

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_dev_find
// Desc.:    Sucht ein Gerät per Name in der Gerätetabelle. NULL = nicht vorhanden.
// Call:     dev = q9_dev_find("term")
//════════════════════════════════════════════════════════════════════════════════════════════════
q9_dev_t *q9_dev_find(const char *name);

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_dev_attach
// Desc.:    Sucht ein Gerät per Pathlist-Name ("/term" oder "term", case-insensitiv) und erhöht
//           seinen Link-Count. 0 = ok (*out gesetzt), E$BPNam = ungültiger Name,
//           E$MNF = Gerät unbekannt.
// Call:     err = q9_dev_attach("/term", &dev)
//════════════════════════════════════════════════════════════════════════════════════════════════
int q9_dev_attach(const char *pathlist, q9_dev_t **out);

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_dev_detach
// Desc.:    Gibt ein per q9_dev_attach geholtes Gerät wieder frei (Link-Count runter).
//           E$Param, wenn der Zeiger kein gültiger Gerätetabellen-Eintrag ist.
// Call:     err = q9_dev_detach(dev)
//════════════════════════════════════════════════════════════════════════════════════════════════
int q9_dev_detach(q9_dev_t *dev);

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_path_open
// Desc.:    Öffnet einen Pfad auf ein Gerät. Rückgabe >= 0: Pfadnummer,
//           < 0: negierter OS-9-Fehlercode (-E$PthFul, -E$NotRdy = Gerät unbekannt).
// Call:     path = q9_path_open("term", Q9_MODE_UPDATE)
//════════════════════════════════════════════════════════════════════════════════════════════════
int q9_path_open(const char *devname, uint8_t mode);

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_path_dup
// Desc.:    Dupliziert einen offenen Pfad (gleiches Gerät, gleicher Modus) auf die niedrigste
//           freie Pfadnummer (OS-9-Semantik). Rückgabe >= 0: neue Pfadnummer, < 0: -Fehlercode.
// Call:     newpath = q9_path_dup(path)
//════════════════════════════════════════════════════════════════════════════════════════════════
int q9_path_dup(uint32_t path);

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_path_close
// Desc.:    Schließt einen Pfad, gibt den Tabelleneintrag frei. 0 = ok, sonst E$BPNum.
// Call:     err = q9_path_close(path)
//════════════════════════════════════════════════════════════════════════════════════════════════
int q9_path_close(uint32_t path);

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_path_get
// Desc.:    Liefert den Pfad-Deskriptor zu einer Pfadnummer. NULL = ungültig/nicht offen.
// Call:     p = q9_path_get(pathnum)
//════════════════════════════════════════════════════════════════════════════════════════════════
q9_path_t *q9_path_get(uint32_t path);

#endif // Q9_DEVICE_H

//────────────────────────────────────────────────────────────────────────────────────────────────
// EOF device.h                                                                            Ver. 1.20
//────────────────────────────────────────────────────────────────────────────────────────────────
