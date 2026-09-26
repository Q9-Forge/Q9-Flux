/* dhf_proto.h - Protocol constants and definitions for Q9-DHF
 * Defines command IDs, status codes, and hardware defaults.
 */

#ifndef DHF_PROTO_H
#define DHF_PROTO_H

#include <stdint.h>

#define DHF_PROTOCOL_VERSION    1

/* Default hardware base address for shared memory window in Q9-Flux */
#define DHF_DEFAULT_HW_BASE     0xFFFFD000U
#define DHF_DEFAULT_HW_SIZE     0x1000U       /* 4 KB window */

/* Default socket port for remote host filesystem daemon */
#define DHF_DEFAULT_PORT        9988

/* Commands (matching PROTOCOL.md Universeller Kommando-Bereich) */
enum dhf_command {
    DHF_CMD_IDLE        = 0,
    DHF_CMD_CREATE      = 1,
    DHF_CMD_OPEN        = 2,
    DHF_CMD_SEEK        = 3,
    DHF_CMD_READ        = 4,
    DHF_CMD_WRITE       = 5,
    DHF_CMD_READLN      = 6,
    DHF_CMD_WRITELN     = 7,
    DHF_CMD_GETSTT      = 8,
    DHF_CMD_SETSTT      = 9,
    DHF_CMD_CLOSE       = 10,
    DHF_CMD_DELETE      = 11,
    DHF_CMD_MKDIR       = 12,
    DHF_CMD_CHDIR       = 13,
    DHF_CMD_RMDIR       = 14,
    DHF_CMD_RENAME      = 15,
    DHF_CMD_OPENDIR     = 16,
    DHF_CMD_READDIR     = 17,
    DHF_CMD_INIT        = 18,
    DHF_CMD_TERM        = 19,
    /* 2026-09-26: I$GetStt SS_FD ($0F, "Read File Descriptor Sector") -- liefert dem
     * Aufrufer ein synthetisches RBF-FD-Sektor-Abbild (FD_ATT/FD_OWN/FD_DAT/FD_LNK/FD_SIZ/
     * FD_CREAT, Rest genullt), aus stat() auf dem Host-Handle gebaut. Genutzt u.a. von der
     * echten "attr"-Utility, s. Q9-OS/Q9-DHF-68k/STATUS.md. */
    DHF_CMD_GETFD       = 20,
    /* 2026-09-26: I$SetStt SS_Attr ($1C) -- setzt Unix-Rechte-Bits aus dem Attribut-Byte
     * (Gegenstueck zu SS_FD's Lesen). d0=Pfadnummer, d1=neues Attribut-Byte. */
    DHF_CMD_SETATTR     = 21,
    /* I$GetStt SS_Pos ($05) -- aktuelle Dateiposition. d0=Pfadnummer, Ausgabe d1=Position. */
    DHF_CMD_GETPOS      = 22,
    /* I$GetStt SS_EOF ($06) -- Dateiende-Test. d0=Pfadnummer, status=DHF_ERR_EOF wenn am
     * Ende, sonst DHF_ERR_OK mit d1=0. */
    DHF_CMD_ISEOF       = 23,
    /* I$SetStt SS_Rename ($42) -- Handle-basierte Umbenennung (alte Datei bereits offen,
     * a1=Zeiger auf neuen Namen). ACHTUNG: kein echtes Microware-Utility erreicht diesen
     * Aufruf jemals -- das reale "rename"-Kommando verweigert sich bei jedem Nicht-RBF-
     * FileManager schon VOR jedem Syscall hart mit "pathname not RBF device" (empirisch
     * per Q9_TRAP_TRACE_ALL verifiziert, s. Q9-OS/Q9-DHF-68k/STATUS.md). Nur fuer eigene
     * Werkzeuge nutzbar, Registerkonvention daher eigene, nicht gegen ein reales Utility
     * verifizierte Annahme. */
    DHF_CMD_RENAMEAT    = 24,
    /* I$GetStt SS_Free ($43) -- freier Speicherplatz im Basisverzeichnis (statvfs()).
     * ACHTUNG: das reale "free"-Kommando versucht IMMER einen rohen "@"-Physikalzugriff
     * (I$Open("<geraet>@")) und Bitmap-Sektoren zu lesen -- fuer ein Host-Passthrough-
     * Dateisystem ohne Medium/LSNs architektonisch nicht abbildbar (ebenfalls per Trace
     * verifiziert). Auch dieser Aufruf ist daher nur fuer eigene Werkzeuge gedacht. */
    DHF_CMD_GETFREE     = 25,
    /* I$GetStt SS_FDInf ($20) -- FD-Abbild zu einer (Pseudo-)Sektornummer: d2=Sektornummer
     * (aus Byte 29-31 eines Verzeichniseintrags), d1=Byteanzahl, a1=Zielpuffer. DHF hat keine
     * Sektoren; dhf_host_fs vergibt die Nummern beim Verzeichnislesen je Host-Pfad. */
    DHF_CMD_FDINF       = 26,
    /* I$GetStt SS_VolStore ($45) -- Speicherstatistik des Laufwerks: a1=16-Byte-Puffer mit
     * {Bytes/Sektor, Sektoren gesamt, Sektoren frei, groesster freier Block} (je u_int,
     * big-endian). Aufbau aus /CMDS/free ermittelt (nirgends dokumentiert, 2026-09-26). */
    DHF_CMD_VOLSTORE    = 27,
    /* I$SetStt SS_FD ($0F) -- FD-Abbild schreiben (nur FD_DAT wirkt: Aenderungszeit): d0=Handle,
     * a1=Zeiger auf das FD-Abbild im Gast-RAM */
    DHF_CMD_SETFD       = 28,
    DHF_CMD_PING        = 254,
    DHF_CMD_RETURN      = 255
};

/* OS-9-Fehlercodes. Das status-Byte geht UNVERAENDERT als d1.w (Carry gesetzt) an den
 * I$-Aufrufer (driver/dhfdrv_68k.a), die Werte MUESSEN also die echten OS-9-Nummern aus
 * MWOS/SRC/DEFS/errno.h sein. 2026-09-26 korrigiert: vorher stimmten nur EOF ($D3) und
 * NOT_FOUND ($D8); alle anderen lagen um 1-4 daneben und trafen fremde Codes (z.B.
 * FILE_EXISTS=$D5 war E$NES, DISK_FULL=$D6 war E$FNA, BAD_PATH=$CD war E$BMID). */
enum dhf_error {
    DHF_ERR_OK            = 0,
    DHF_ERR_ERROR         = 0xF5, /* E$Write   -- unerwarteter Host-Fehler (Sammelcode) */
    DHF_ERR_BAD_PATH      = 0xC9, /* E$BPNum   -- ungueltige/unbekannte Pfadnummer */
    DHF_ERR_PATH_FULL     = 0xC8, /* E$PthFul  -- keine freien Handles (EMFILE/ENFILE) */
    DHF_ERR_EOF           = 0xD3, /* E$EOF     -- Dateiende */
    DHF_ERR_FILE_EXISTS   = 0xDA, /* E$CEF     -- Creating Existing File (EEXIST) */
    DHF_ERR_DISK_FULL     = 0xF8, /* E$Full    -- Media Full (ENOSPC/EDQUOT/EFBIG) */
    DHF_ERR_NO_PERMISSION = 0xD6, /* E$FNA     -- File Not Accessible (EACCES/EPERM) */
    DHF_ERR_NOT_FOUND     = 0xD8, /* E$PNNF    -- Path Name Not Found (ENOENT) */
    DHF_ERR_SHARING       = 0xFD, /* E$Share   -- Non-sharable file busy (EBUSY/ETXTBSY) */
    DHF_ERR_IS_DIR        = 0xD6, /* E$FNA     -- OS-9 kennt kein EISDIR; RBF meldet E$FNA */
    DHF_ERR_NOT_DIR       = 0xD6, /* E$FNA     -- OS-9 kennt kein ENOTDIR; RBF meldet E$FNA */
    DHF_ERR_BAD_NAME      = 0xD7, /* E$BPNam   -- Bad Path Name (ENAMETOOLONG/ELOOP, Basispfad-Ausbruch) */
    DHF_ERR_DIR_NOT_EMPTY = 0xEE, /* E$DNE     -- Directory not empty (ENOTEMPTY) */
    DHF_ERR_WRITE_PROT    = 0xF2, /* E$WP      -- Write Protect (EROFS) */
    DHF_ERR_UNSUPPORTED   = 0xD0, /* E$UnkSvc  -- unbekanntes Kommando */
    DHF_ERR_TIMEOUT       = 0xF6, /* E$NotRdy  -- Geraet antwortet nicht */
    DHF_ERR_NET           = 0xF6  /* E$NotRdy  -- entfernter Socket-Backend nicht erreichbar */
};

/* File Open / Access Modes (OS-9 compatible) */
#define DHF_MODE_READ       0x01
#define DHF_MODE_WRITE      0x02
#define DHF_MODE_EXEC       0x04
#define DHF_MODE_PREAD      0x08
#define DHF_MODE_PWRITE     0x10
#define DHF_MODE_PEXEC      0x20
#define DHF_MODE_SHARE      0x40
#define DHF_MODE_DIR        0x80

/* Seek Modes */
#define DHF_SEEK_SET        0
#define DHF_SEEK_CUR        1
#define DHF_SEEK_END        2

#endif /* DHF_PROTO_H */
