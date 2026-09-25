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
    DHF_CMD_PING        = 254,
    DHF_CMD_RETURN      = 255
};

/* OS-9 Compatible Error Codes */
enum dhf_error {
    DHF_ERR_OK          = 0,
    DHF_ERR_ERROR       = 1,
    DHF_ERR_BAD_PATH    = 205, /* E: Bad path number */
    DHF_ERR_PATH_FULL   = 206, /* E: Path table full */
    DHF_ERR_EOF         = 211, /* E: End of file reached */
    DHF_ERR_FILE_EXISTS = 213, /* E: File already exists / create error */
    DHF_ERR_DISK_FULL   = 214, /* E: Device full */
    DHF_ERR_NO_PERMISSION=215, /* E: Write protect / Permission denied */
    DHF_ERR_NOT_FOUND   = 216, /* E: Path name not found */
    DHF_ERR_SHARING     = 217, /* E: Sharing violation */
    DHF_ERR_IS_DIR      = 218, /* E: Is a directory */
    DHF_ERR_NOT_DIR     = 219, /* E: Not a directory */
    DHF_ERR_UNSUPPORTED = 240, /* E: Unknown service */
    DHF_ERR_TIMEOUT     = 241, /* Timeout waiting for device */
    DHF_ERR_NET         = 242  /* Remote socket / network error */
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
