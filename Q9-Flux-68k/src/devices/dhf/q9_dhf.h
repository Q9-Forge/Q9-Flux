//════════════════════════════════════════════════════════════════════════════════════════════════
// File:   q9_dhf.h                                                                        Ver. 1.00
// Owner:  Claude
// Desc.:  DHF (Direct Host Filesystem) -- MMIO-Bruecke zwischen dem OS-9-Treiber dhfdrv-68k.c
//         (Q9-OS/Q9-DHF-68k/driver/) und dem echten Host-Dateisystem. Erster Schritt aus
//         docs/HOSTFS_MANAGER_de.md ("Q9-Flux-Emulator zuerst"): der Gast schreibt ein Kommando
//         + Argumente in dieses Registerfenster, dieses Geraet fuehrt den ECHTEN Host-Aufruf aus
//         (fopen/fread/mkdir/opendir/...) und legt das Ergebnis zurueck -- exakt das Verfahren,
//         das im WIP-Treiber bisher direkt (und damit nur im reinen Host-Simulator lauffaehig)
//         drinstand; hier zieht es auf die richtige Seite der Gast/Host-Grenze um.
//
//         Registerfenster (byte-adressiert, alle Mehrbyte-Felder big-endian wie 68k):
//           0x000  CMD      (1 Byte, schreiben loest die Ausfuehrung aus, s. Q9_DHF_CMD_*)
//           0x001  HANDLE   (1 Byte, Slot-Index 0..Q9_DHF_MAX_HANDLES-1 fuer datei-/dirbezogene
//                            Kommandos; bei OPEN/OPENDIR schreibt das Geraet den zugeteilten
//                            Slot hierher zurueck)
//           0x004  RESULT   (4 Byte signed long) -- >=0 Erfolg (Bytes/Ergebniswert), <0 Fehler
//           0x008  ERRNO    (4 Byte) -- Host-errno bei RESULT<0, sonst 0
//           0x00C  ARG1     (4 Byte) -- Bedeutung je Kommando (Laenge/Offset/Modus)
//           0x010  ARG2     (4 Byte) -- Bedeutung je Kommando (whence/Flags)
//           0x100  PATH     (256 Byte, NUL-terminiert, relativ zum konfigurierten Basepath)
//           0x200  PATH2    (256 Byte, NUL-terminiert, nur RENAME = Zielpfad)
//           0x300  DATA     (Q9_DHF_DATA_SIZE Byte, READ/WRITE-Nutzlast, READDIR-Name)
//
// Edition History
//─────────┬──────┬────────────────────────────────────────────────────────────────────────┬──────
// Date    │ Ver. │ Description                                                            │ By
//─────────┼──────┼────────────────────────────────────────────────────────────────────────┬──────
// 26-09-25│ 1.00 │ Erster Wurf: Registerfenster + Vtable, Musashi zuerst (docs/             │ Cld
//         │      │ HOSTFS_MANAGER_de.md). Basepath vorerst hartkodiert bei attach() gesetzt,│
//         │      │ noch ohne extra_fields/Config-Schema (wie rtc72421 anfangs).             │
//═════════╧══════╧════════════════════════════════════════════════════════════════════════╧══════
#ifndef Q9_DHF_H
#define Q9_DHF_H

#include <stdint.h>
#include <stdio.h>
#include "../../kernel/devdesc.h"

#define Q9_DHF_MAX_HANDLES   8
#define Q9_DHF_PATH_SIZE     256
#define Q9_DHF_DATA_SIZE     3328          /* 0x300..0xFFF, Fenstergroesse 0x1000 */
#define Q9_DHF_WINDOW_SIZE   0x1000u

#define Q9_DHF_OFF_CMD       0x000
#define Q9_DHF_OFF_HANDLE    0x001
#define Q9_DHF_OFF_RESULT    0x004
#define Q9_DHF_OFF_ERRNO     0x008
#define Q9_DHF_OFF_ARG1      0x00C
#define Q9_DHF_OFF_ARG2      0x010
#define Q9_DHF_OFF_PATH      0x100
#define Q9_DHF_OFF_PATH2     0x200
#define Q9_DHF_OFF_DATA      0x300

/* Kommandos, s. Kopfkommentar/docs/PROTOCOL.md fuer die 13 Manager-Standardaufrufe. Deckungsgleich
   mit den bereits im WIP-Treiber (dhfdrv-68k.c) vorhandenen Host-Operationen -- GETSTAT/SETSTAT
   bewusst nicht dabei, waren dort auch nur Stubs (return -1). */
enum {
    Q9_DHF_CMD_NONE     = 0,
    Q9_DHF_CMD_OPEN     = 1,   /* PATH, ARG1=flags (O_RDONLY/O_WRONLY/O_RDWR/O_CREAT/O_TRUNC), ARG2=mode
                                   -> HANDLE, RESULT=0/-1 */
    Q9_DHF_CMD_CLOSE    = 2,   /* HANDLE -> RESULT=0/-1 */
    Q9_DHF_CMD_READ     = 3,   /* HANDLE, ARG1=count (<=DATA_SIZE) -> DATA gefuellt, RESULT=gelesene Bytes */
    Q9_DHF_CMD_WRITE    = 4,   /* HANDLE, ARG1=count (<=DATA_SIZE), DATA gefuellt -> RESULT=geschriebene Bytes */
    Q9_DHF_CMD_SEEK     = 5,   /* HANDLE, ARG1=offset, ARG2=whence (0/1/2=SEEK_SET/CUR/END) -> RESULT=neue Position */
    Q9_DHF_CMD_MKDIR    = 6,   /* PATH, ARG1=mode -> RESULT=0/-1 */
    Q9_DHF_CMD_RMDIR    = 7,   /* PATH -> RESULT=0/-1 */
    Q9_DHF_CMD_UNLINK   = 8,   /* PATH -> RESULT=0/-1 */
    Q9_DHF_CMD_RENAME   = 9,   /* PATH (alt), PATH2 (neu) -> RESULT=0/-1 */
    Q9_DHF_CMD_OPENDIR  = 10,  /* PATH -> HANDLE, RESULT=0/-1 */
    Q9_DHF_CMD_READDIR  = 11,  /* HANDLE -> DATA=Name (NUL-terminiert), RESULT=1 (Eintrag) / 0 (Ende) / -1 */
    Q9_DHF_CMD_CLOSEDIR = 12,  /* HANDLE -> RESULT=0/-1 */
    Q9_DHF_CMD_TRUNCATE = 13   /* PATH, ARG1=length -> RESULT=0/-1 */
};

typedef enum { Q9_DHF_SLOT_FREE = 0, Q9_DHF_SLOT_FILE, Q9_DHF_SLOT_DIR } q9_dhf_slot_kind_t;

typedef struct {
    q9_dhf_slot_kind_t kind;
    union {
        FILE *file;
        void *dir;              /* DIR*, per void* um <dirent.h> hier nicht importieren zu muessen */
    } h;
} q9_dhf_slot_t;

typedef struct {
    char           basepath[512];          /* Host-Wurzel, s. attach()-Aufrufer (m68krt.c) */
    q9_dhf_slot_t  slots[Q9_DHF_MAX_HANDLES];

    /* Register-/Puffer-Abbild, wie vom Gast zuletzt beschrieben/wie es das Geraet zurueckliefert. */
    uint8_t        regs[Q9_DHF_WINDOW_SIZE];
} q9_dhf_t;

extern const q9_device_vtable_t q9_devtype_dhf;
extern const q9_devdesc_t       q9_devdesc_dhf;

/* Initialisiert state->basepath (m68krt.c ruft dies beim Attach auf, vor q9_devreg_add). */
void q9_dhf_init(q9_dhf_t *state, const char *basepath);

#endif /* Q9_DHF_H */
//────────────────────────────────────────────────────────────────────────────────────────────────
// EOF q9_dhf.h                                                                            Ver. 1.00
//────────────────────────────────────────────────────────────────────────────────────────────────
