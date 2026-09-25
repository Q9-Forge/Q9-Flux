/* dhf_shared.h - Shared Command Area struct for Q9-DHF-68k
 * Packed, big-endian layout.
 * Buffer and pathname pointers point to guest 68k memory (allocated by user program).
 */

#ifndef DHF_SHARED_H
#define DHF_SHARED_H

#include <stdint.h>

#pragma pack(push, 1)
struct dhf_shared {
    uint8_t  version;     /* Protocol version (1) */
    uint8_t  command;     /* 0 = Idle, 1..N = Commands, 255 = Return */
    uint8_t  status;      /* Result error code (0 = OK, OS-9 error) */
    uint8_t  flags;       /* Reserved flags */
    uint32_t seq;         /* Incremented by driver for each request (BE) */

    uint32_t a0;          /* Guest 68k pointer: pathname / string */
    uint32_t a1;          /* Guest 68k pointer: data buffer */
    uint32_t d0;          /* Pathnum / descriptor / handle (BE) */
    uint32_t d1;          /* Status / byte count / max length / offset (BE) */
    uint32_t d2;          /* Attributes / flags / whence (BE) */
};
#pragma pack(pop)

#endif /* DHF_SHARED_H */
