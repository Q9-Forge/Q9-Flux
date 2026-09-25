/* dhf_emu_device.h - Simulated Hardware Device for Q9-Flux
 * Connects Q9-Flux 68k address space to host filesystem or remote socket.
 * 68k addresses in A0/A1 are direct offsets into the emulator memory array.
 */

#ifndef DHF_EMU_DEVICE_H
#define DHF_EMU_DEVICE_H

#include <stdint.h>
#include <stddef.h>
#include "dhf_shared.h"
#include "dhf_host_fs.h"

typedef enum {
    DHF_BACKEND_LOCAL = 0,
    DHF_BACKEND_REMOTE_SOCKET = 1
} dhf_backend_type_t;

typedef struct dhf_emu_device {
    dhf_backend_type_t  backend;
    dhf_host_fs_t       host_fs;
    struct dhf_shared  *shared_mem;
    uint8_t            *emu_memory;      /* Direct pointer to simulated 68k RAM array */
    size_t              emu_memory_size; /* Size of simulated RAM */
    int                 socket_fd;
    char                remote_host[128];
    int                 remote_port;
    uint32_t            base_addr;
    uint32_t            size;
} dhf_emu_device_t;

/* Device lifecycle */
int  dhf_emu_device_init_local(dhf_emu_device_t *dev, struct dhf_shared *mem, const char *basepath);
int  dhf_emu_device_init_remote(dhf_emu_device_t *dev, struct dhf_shared *mem, const char *host, int port);
void dhf_emu_device_set_ram(dhf_emu_device_t *dev, uint8_t *ram, size_t size);
void dhf_emu_device_cleanup(dhf_emu_device_t *dev);

/* Process current command in shared memory area */
int  dhf_emu_device_process(dhf_emu_device_t *dev);

/* 68k Bus Interface (Q9-Flux devreg vtable compatible) */
uint8_t  dhf_emu_device_read8(dhf_emu_device_t *dev, uint32_t offset);
void     dhf_emu_device_write8(dhf_emu_device_t *dev, uint32_t offset, uint8_t val);
uint16_t dhf_emu_device_read16(dhf_emu_device_t *dev, uint32_t offset);
void     dhf_emu_device_write16(dhf_emu_device_t *dev, uint32_t offset, uint16_t val);
uint32_t dhf_emu_device_read32(dhf_emu_device_t *dev, uint32_t offset);
void     dhf_emu_device_write32(dhf_emu_device_t *dev, uint32_t offset, uint32_t val);

#endif /* DHF_EMU_DEVICE_H */
