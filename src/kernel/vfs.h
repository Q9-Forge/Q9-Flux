//════════════════════════════════════════════════════════════════════════════════════════════════
// File:   vfs.h                                                                           Ver. 1.00
// Owner:  AF
// Desc.:  Q9 VFS-Schicht (Phase 3.2, OS-9-Vorbild: IOMan/RBF-Trennung, docs/MODULES.md). Routet
//         Pfade der Form "/d0/pfad/datei" (F$PrsNam trennt Geraet/Rest) an einen optionalen
//         File-Manager hinter dem Geraet (q9_dev_t.fm) — analog zum Treiber-Interface q9_drv_t,
//         aber eine Stufe hoeher: der Treiber spricht Bloecke/Zeichen, der File-Manager spricht
//         Pfade/Dateien. Geraete OHNE File-Manager (/term, /nil) verhalten sich wie bisher
//         (Rest-Pfad muss leer sein) — rueckwaertskompatibel zu Phase 1/2.
//
//         Bewusst KEIN konkretes Dateisystem hier (FAT16 kommt in 3.3/3.4, docs/ARBEITSPLAN.md) —
//         diese Datei kennt nur die schmale Schnittstelle, hinter der ein File-Manager andockt.
//         Schnitt orientiert an "OS-9 Insights" (Dibble): open/create/makdir/delete/chdir nehmen
//         einen Rest-Pfad-String entgegen (kein Struct mit OS-9-Pfaddeskriptor-Internas), damit
//         später auch ein 68k-Manager-Adapter (Ideenspeicher, ARBEITSPLAN.md) dieselbe
//         C-Schnittstelle hinter einem Trap-Bridge-Aufruf bedienen kann.
//
// Call:   err = q9_vfs_open(pathlist, mode);  err = q9_vfs_chdir(pathlist);
//
// Edition History
//─────────┬──────┬────────────────────────────────────────────────────────────────────────┬──────
// Date    │ Ver. │ Description                                                            │ By
//─────────┼──────┼────────────────────────────────────────────────────────────────────────┼──────
// 26-07-04│ 1.00 │ 3.2: Initiale Version — q9_fm_t, q9_vfs_open/chdir, globales Arbeits-   │ CF
//         │      │ verzeichnis                                                            │ CF
//═════════╧══════╧════════════════════════════════════════════════════════════════════════╧══════
#ifndef Q9_VFS_H
#define Q9_VFS_H

#include <stdint.h>
#include "device.h"

#define Q9_CWD_MAXLEN 63                               /* globales Arbeitsverzeichnis (3.2);     */
                                                        /*   pro-Prozess-Variante erst Phase 4    */

//╔══════════════════════════════════════════════════════════════════════════════════════════════╗
//║ FILE-MANAGER OPERATIONS (austauschbare Einheit hinter dem Geraet, wie q9_drv_t fuer Treiber)  ║
//╚══════════════════════════════════════════════════════════════════════════════════════════════╝
//  "restpath" ist der Teil hinter dem Geraetenamen, OHNE fuehrenden '/' ("pfad/datei" bei
//  "/d0/pfad/datei"), leerer String = Geraete-Wurzel. Der File-Manager fuellt bei open/create
//  den Datei-Kontext im Pfad-Deskriptor (q9_path_t.fmctx, device.h) — Groesse/Format sind
//  reine File-Manager-Sache, der Kernel fasst das Feld nicht an. Rueckgabe 0 oder OS-9-Fehlercode.

typedef struct q9_fm {
    const char *name;                                  /* File-Manager-Name, z.B. "FAT16"        */

    //────────────────────────────────────────────────────────────────────────────────────────────
    // open: vorhandene Datei/Verzeichnis "restpath" oeffnen, Kontext in p->fmctx ablegen.
    //────────────────────────────────────────────────────────────────────────────────────────────
    int (*open)  (q9_dev_t *dev, q9_path_t *p, const char *restpath, uint32_t len, uint8_t mode);

    //────────────────────────────────────────────────────────────────────────────────────────────
    // create: neue Datei "restpath" anlegen (I$Create). Gerüst bis 3.4 (FAT16 schreibend) —
    // darf bis dahin unimplementiert E$UnkSvc liefern.
    //────────────────────────────────────────────────────────────────────────────────────────────
    int (*create)(q9_dev_t *dev, q9_path_t *p, const char *restpath, uint32_t len, uint8_t mode);

    //────────────────────────────────────────────────────────────────────────────────────────────
    // makdir: neues Verzeichnis "restpath" anlegen (I$MakDir). Gerüst bis 3.4.
    //────────────────────────────────────────────────────────────────────────────────────────────
    int (*makdir)(q9_dev_t *dev, const char *restpath, uint32_t len);

    //────────────────────────────────────────────────────────────────────────────────────────────
    // remove: Datei/Verzeichnis "restpath" loeschen (I$Delete). Gerüst bis 3.4.
    //────────────────────────────────────────────────────────────────────────────────────────────
    int (*remove)(q9_dev_t *dev, const char *restpath, uint32_t len);
} q9_fm_t;

//╔══════════════════════════════════════════════════════════════════════════════════════════════╗
//║ API                                                                                          ║
//╚══════════════════════════════════════════════════════════════════════════════════════════════╝

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_vfs_open
// Desc.:    Öffnet einen Pfad (I$Open-Unterbau). "pathlist" wird per F$PrsNam in Gerätename +
//           Rest-Pfad zerlegt (relative Pfade ohne führenden '/' werden zuerst gegen das globale
//           Arbeitsverzeichnis aufgelöst, q9_vfs_chdir). Hat das Gerät einen File-Manager
//           (q9_dev_t.fm), wird der Rest an dessen open()-Op weitergereicht (leerer Rest = Wurzel
//           des Dateisystems, z.B. "/d0" selbst). Hat das Gerät KEINEN File-Manager (z.B. /term,
//           /nil), MUSS der Rest-Pfad leer sein — sonst E$PNNF (bisheriges q9_path_open-Verhalten,
//           rückwärtskompatibel). Rückgabe >= 0: Pfadnummer, < 0: negierter Fehlercode.
// Call:     path = q9_vfs_open("/d0/pfad/datei", Q9_MODE_READ)
//════════════════════════════════════════════════════════════════════════════════════════════════
int q9_vfs_open(const char *pathlist, uint8_t mode);

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_vfs_chdir
// Desc.:    I$ChgDir-Unterbau: setzt das globale Arbeitsverzeichnis (Phase 3 — EIN globaler
//           String, kein Pfad pro Prozess; das kommt erst mit echten Prozessen in Phase 4).
//           0 = ok, E$PNNF = Pfad zu lang (Q9_CWD_MAXLEN) oder leer.
// Call:     err = q9_vfs_chdir("/d0/pfad")
//════════════════════════════════════════════════════════════════════════════════════════════════
int q9_vfs_chdir(const char *pathlist);

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_vfs_cwd
// Desc.:    Liefert das aktuelle globale Arbeitsverzeichnis (nullterminiert). Direkt nach dem
//           Boot: "/" (Wurzel, kein Gerät gewählt — absolute Pfade sind bis dahin Pflicht).
// Call:     cwd = q9_vfs_cwd()
//════════════════════════════════════════════════════════════════════════════════════════════════
const char *q9_vfs_cwd(void);

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_dev_set_fm
// Desc.:    Trägt einen File-Manager an einem Gerät ein (z.B. FAT16 an /d0, ab 3.3). Für 3.2 vor
//           allem für Tests gedacht — kein Gerät hat vor 3.3 produktiv einen File-Manager.
// Call:     q9_dev_set_fm(dev, &my_fm)
//════════════════════════════════════════════════════════════════════════════════════════════════
void q9_dev_set_fm(q9_dev_t *dev, const q9_fm_t *fm);

#endif // Q9_VFS_H

//────────────────────────────────────────────────────────────────────────────────────────────────
// EOF vfs.h                                                                               Ver. 1.00
//────────────────────────────────────────────────────────────────────────────────────────────────
