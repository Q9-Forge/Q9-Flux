/* dhf_emu_device.c - Simulated Hardware Device implementation for Q9-Flux
 * Resolves 68k guest addresses directly as offsets into the emulator memory array.
 */

#include "dhf_emu_device.h"
#include "dhf_proto.h"
#include "dhf_shared.h"
#include "dhf_socket.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <unistd.h>

#define DHF_TEMP_BUF_SIZE 65536

static inline void *resolve_guest_ptr(dhf_emu_device_t *dev, uint32_t guest_addr, size_t len) {
    if (dev->emu_memory) {
        if ((size_t)guest_addr + len <= dev->emu_memory_size) {
            return (void *)(dev->emu_memory + guest_addr);
        }
        return NULL; /* Out of bounds */
    }
    /* Fallback if no explicit RAM array is set */
    return (void *)(uintptr_t)guest_addr;
}

static inline const char *resolve_guest_str(dhf_emu_device_t *dev, uint32_t guest_addr) {
    if (!guest_addr) return NULL;
    if (dev->emu_memory) {
        if ((size_t)guest_addr < dev->emu_memory_size) {
            return (const char *)(dev->emu_memory + guest_addr);
        }
        return NULL;
    }
    return (const char *)(uintptr_t)guest_addr;
}

int dhf_emu_device_init_local(dhf_emu_device_t *dev, struct dhf_shared *mem, const char *basepath) {
    if (!dev || !mem) return -1;
    memset(dev, 0, sizeof(*dev));
    dev->backend = DHF_BACKEND_LOCAL;
    dev->shared_mem = mem;
    dev->socket_fd = -1;
    dev->base_addr = DHF_DEFAULT_HW_BASE;
    dev->size = sizeof(struct dhf_shared);
    return dhf_host_fs_init(&dev->host_fs, basepath);
}

int dhf_emu_device_init_remote(dhf_emu_device_t *dev, struct dhf_shared *mem, const char *host, int port) {
    if (!dev || !mem || !host) return -1;
    memset(dev, 0, sizeof(*dev));
    dev->backend = DHF_BACKEND_REMOTE_SOCKET;
    dev->shared_mem = mem;
    dev->socket_fd = -1;
    strncpy(dev->remote_host, host, sizeof(dev->remote_host) - 1);
    dev->remote_port = port > 0 ? port : DHF_DEFAULT_PORT;
    dev->base_addr = DHF_DEFAULT_HW_BASE;
    dev->size = sizeof(struct dhf_shared);
    return 0;
}

void dhf_emu_device_set_ram(dhf_emu_device_t *dev, uint8_t *ram, size_t size) {
    if (!dev) return;
    dev->emu_memory = ram;
    dev->emu_memory_size = size;
}

void dhf_emu_device_cleanup(dhf_emu_device_t *dev) {
    if (!dev) return;
    if (dev->backend == DHF_BACKEND_LOCAL) {
        dhf_host_fs_cleanup(&dev->host_fs);
    } else if (dev->backend == DHF_BACKEND_REMOTE_SOCKET) {
        if (dev->socket_fd >= 0) {
            close(dev->socket_fd);
            dev->socket_fd = -1;
        }
    }
}

int dhf_emu_device_process(dhf_emu_device_t *dev) {
    if (!dev || !dev->shared_mem) return -1;
    struct dhf_shared *s = dev->shared_mem;
    uint8_t cmd = s->command;

    if (cmd == DHF_CMD_IDLE || cmd == DHF_CMD_RETURN) {
        return 0;
    }

    uint8_t status = DHF_ERR_OK;
    uint32_t a0 = ntohl(s->a0);
    uint32_t a1 = ntohl(s->a1);
    uint32_t d0 = ntohl(s->d0);
    uint32_t d1 = ntohl(s->d1);
    uint32_t d2 = ntohl(s->d2);

    const char *path = resolve_guest_str(dev, a0);

    /* If remote backend, forward request over TCP */
    if (dev->backend == DHF_BACKEND_REMOTE_SOCKET) {
        const void *send_data = NULL;
        size_t send_len = 0;
        char recv_buf[DHF_TEMP_BUF_SIZE];
        size_t recv_len = sizeof(recv_buf);

        if (cmd == DHF_CMD_WRITE || cmd == DHF_CMD_WRITELN) {
            send_len = d1;
            send_data = resolve_guest_ptr(dev, a1, send_len);
        } else if (cmd == DHF_CMD_RENAME) {
            const char *newp = resolve_guest_str(dev, a1);
            if (newp) {
                send_data = newp;
                send_len = strlen(newp) + 1;
            }
        }

        int res = dhf_socket_forward_cmd(&dev->socket_fd, dev->remote_host, dev->remote_port,
                                         cmd, &d0, &d1, &d2, path,
                                         send_data, send_len,
                                         recv_buf, &recv_len,
                                         &status);
        if (res == 0 && status == DHF_ERR_OK) {
            if ((cmd == DHF_CMD_READ || cmd == DHF_CMD_READLN || cmd == DHF_CMD_GETSTT || cmd == DHF_CMD_READDIR) && recv_len > 0) {
                void *dest = resolve_guest_ptr(dev, a1, recv_len);
                if (dest) {
                    memcpy(dest, recv_buf, recv_len);
                }
            }
        }

        s->d0 = htonl(d0);
        s->d1 = htonl(d1);
        s->d2 = htonl(d2);
        s->status = status;
        __sync_synchronize();
        s->command = DHF_CMD_IDLE;
        __sync_synchronize();
        return res;
    }

    /* Local backend: evaluate command */
    switch (cmd) {
        case DHF_CMD_INIT: {
            if (path && path[0]) {
                dhf_host_fs_init(&dev->host_fs, path);
            }
            status = DHF_ERR_OK;
            break;
        }

        case DHF_CMD_TERM: {
            dhf_host_fs_cleanup(&dev->host_fs);
            status = DHF_ERR_OK;
            break;
        }

        case DHF_CMD_OPEN: {
            /* 2026-09-26: d0 ist jetzt EINGABE -- die OS-9-Pfadnummer vom Manager (statt
             * eines hier selbst vergebenen Handles), s. dhf_host_fs_open_at-Kommentar. */
            int h = dhf_host_fs_open_at(&dev->host_fs, (int)d0, path, (int)d2, &status);
            if (h >= 0) {
                s->d0 = htonl((uint32_t)h);
                /* 2026-09-26: d2 ist bei OPEN sonst unbenutzte AUSGABE -- meldet dem Manager,
                 * ob das Ziel ein Host-Verzeichnis war (open_at() erkennt das per stat() und
                 * benutzt opendir() statt open()). Der Manager braucht das, um das RBF-
                 * spezifische PD_ATT-Feld im Pfaddeskriptor zu setzen -- ohne das haelt die
                 * echte "dir"-Utility JEDEN Pfad fuer eine normale Datei, s. STATUS.md. */
                s->d2 = htonl(dev->host_fs.handles[h].is_dir ? 1u : 0u);
            }
            break;
        }

        case DHF_CMD_CREATE: {
            int h = dhf_host_fs_create_at(&dev->host_fs, (int)d0, path, (int)d2, (int)d1, &status);
            if (h >= 0) s->d0 = htonl((uint32_t)h);
            /* 2026-09-26: Modus-Bit ISize_ ($20) -> a1 = Anfangsgroesse (d2.l des Aufrufers) */
            if (h >= 0 && (d2 & 0x20)) {
                uint8_t st2 = DHF_ERR_OK;
                if (dhf_host_fs_setsize_at(&dev->host_fs, h, a1, &st2) != 0) status = st2;
            }
            break;
        }

        case DHF_CMD_CLOSE: {
            dhf_host_fs_close(&dev->host_fs, (int)d0, &status);
            break;
        }

        case DHF_CMD_READ: {
            void *dest = resolve_guest_ptr(dev, a1, d1);
            if (!dest) {
                status = DHF_ERR_BAD_PATH;
                s->d1 = 0;
            } else {
                ssize_t r = dhf_host_fs_read(&dev->host_fs, (int)d0, dest, d1, &status);
                s->d1 = (r >= 0) ? htonl((uint32_t)r) : 0;
            }
            break;
        }

        case DHF_CMD_WRITE: {
            const void *src = resolve_guest_ptr(dev, a1, d1);
            if (!src) {
                status = DHF_ERR_BAD_PATH;
                s->d1 = 0;
            } else {
                ssize_t w = dhf_host_fs_write(&dev->host_fs, (int)d0, src, d1, &status);
                s->d1 = (w >= 0) ? htonl((uint32_t)w) : 0;
            }
            break;
        }

        case DHF_CMD_SEEK: {
            off_t pos = dhf_host_fs_seek(&dev->host_fs, (int)d0, (off_t)d1, (int)d2, &status);
            if (pos != (off_t)-1) s->d1 = htonl((uint32_t)pos);
            break;
        }

        case DHF_CMD_READLN: {
            char tmp[4096];
            size_t req = d1 < sizeof(tmp) ? d1 : sizeof(tmp);
            int r = dhf_host_fs_readln(&dev->host_fs, (int)d0, tmp, req, &status);
            if (r > 0) {
                void *dest = resolve_guest_ptr(dev, a1, (size_t)r);
                if (dest) memcpy(dest, tmp, (size_t)r);
                s->d1 = htonl((uint32_t)r);
            } else {
                s->d1 = 0;
            }
            break;
        }

        case DHF_CMD_WRITELN: {
            const void *src = resolve_guest_ptr(dev, a1, d1);
            if (!src) {
                status = DHF_ERR_BAD_PATH;
                s->d1 = 0;
            } else {
                int w = dhf_host_fs_writeln(&dev->host_fs, (int)d0, (const char*)src, d1, &status);
                s->d1 = (w >= 0) ? htonl((uint32_t)w) : 0;
            }
            break;
        }

        case DHF_CMD_GETSTT: {
            /* 2026-09-26: by HANDLE (d0, the OS-9 path number), not by "path" -- the manager
               has no pathname string for an already-open path (see dhf_host_fs_getstat_at). */
            char statbuf[64] = {0};
            size_t out_size = 0;
            dhf_host_fs_getstat_at(&dev->host_fs, (int)d0, statbuf, &out_size, &status);
            if (out_size > 0 && a1) {
                void *dest = resolve_guest_ptr(dev, a1, out_size);
                if (dest) memcpy(dest, statbuf, out_size);
            }
            s->d1 = htonl((uint32_t)out_size);
            break;
        }

        case DHF_CMD_SETSTT: {
            /* 2026-09-26: by HANDLE (d0), wie DHF_CMD_GETSTT -- der Manager schickt hier
               nur die eine SetStt-Funktion, die er kennt (SS_Size), mit d0=Pfadnummer und
               d1=gewuenschte Groesse (s. dhf_host_fs_setsize_at). Alle anderen SS_-Codes
               bleiben im Manager selbst E$UnkSvc und erreichen dieses Kommando nie. */
            dhf_host_fs_setsize_at(&dev->host_fs, (int)d0, d1, &status);
            break;
        }

        case DHF_CMD_GETFD: {
            /* 2026-09-26: I$GetStt SS_FD -- d0=Pfadnummer (Handle), d1=gewuenschte
             * Byteanzahl (Aufrufer-d2.w), a1=Zielpuffer im Gast-RAM (Aufrufer-a0). Echte
             * RBF-Utilities wie "attr" nutzen das, um Attribute/Groesse/Datum zu lesen --
             * kein physischer Sektor, ein ganz normales GetStt (s. dhf_host_fs_getfd_at). */
            unsigned char fdbuf[256];
            size_t want = d1 > sizeof(fdbuf) ? sizeof(fdbuf) : d1;
            size_t out_len = 0;
            if (dhf_host_fs_getfd_at(&dev->host_fs, (int)d0, fdbuf, want, &out_len, &status) == 0 && a1) {
                void *dest = resolve_guest_ptr(dev, a1, out_len);
                if (dest) memcpy(dest, fdbuf, out_len);
            }
            break;
        }

        case DHF_CMD_FDINF: {
            /* 2026-09-26: I$GetStt SS_FDInf -- d2=Pseudo-Sektornummer, d1=Byteanzahl,
             * a1=Zielpuffer im Gast-RAM (s. dhf_host_fs_getfd_lsn) */
            unsigned char fdbuf[256];
            size_t want = d1 > sizeof(fdbuf) ? sizeof(fdbuf) : d1;
            size_t out_len = 0;
            if (dhf_host_fs_getfd_lsn(&dev->host_fs, d2, fdbuf, want, &out_len, &status) == 0 && a1) {
                void *dest = resolve_guest_ptr(dev, a1, out_len);
                if (dest) memcpy(dest, fdbuf, out_len);
            }
            break;
        }

        case DHF_CMD_VOLSTORE: {
            /* 2026-09-26: I$GetStt SS_VolStore -- a1 = 16-Byte-Puffer im Gast-RAM */
            uint32_t v[4];
            if (dhf_host_fs_volstore(&dev->host_fs, v, &status) == 0 && a1) {
                uint32_t *dest = resolve_guest_ptr(dev, a1, 16);
                if (dest) for (int i = 0; i < 4; i++) dest[i] = htonl(v[i]);
            }
            break;
        }

        case DHF_CMD_SETATTR: {
            /* d0=Pfadnummer, d1=neues Attribut-Byte (SS_Attr, s. dhf_host_fs_setattr_at) */
            dhf_host_fs_setattr_at(&dev->host_fs, (int)d0, (uint8_t)d1, &status);
            break;
        }

        case DHF_CMD_GETPOS: {
            uint32_t pos = 0;
            if (dhf_host_fs_getpos_at(&dev->host_fs, (int)d0, &pos, &status) == 0) {
                s->d1 = htonl(pos);
            }
            break;
        }

        case DHF_CMD_ISEOF: {
            dhf_host_fs_iseof_at(&dev->host_fs, (int)d0, &status);
            break;
        }

        case DHF_CMD_RENAMEAT: {
            /* 2026-09-26, RBF-Konvention: d0=Pfadnummer eines VERZEICHNISSES, a0=alter Name,
             * a1=neuer Name (je ein Eintrag in diesem Verzeichnis, s. dhf_host_fs_rename_at) */
            const char *oldname = resolve_guest_str(dev, a0);
            const char *newname = resolve_guest_str(dev, a1);
            dhf_host_fs_rename_at(&dev->host_fs, (int)d0, oldname, newname, &status);
            break;
        }

        case DHF_CMD_GETFREE: {
            uint32_t freeb = 0;
            if (dhf_host_fs_getfree(&dev->host_fs, &freeb, &status) == 0) {
                s->d1 = htonl(freeb);
            }
            break;
        }

        case DHF_CMD_CHDIR: {
            dhf_host_fs_chdir(&dev->host_fs, path, &status);
            break;
        }

        case DHF_CMD_MKDIR: {
            dhf_host_fs_mkdir(&dev->host_fs, path, (int)d1, &status);
            break;
        }

        case DHF_CMD_RMDIR: {
            dhf_host_fs_rmdir(&dev->host_fs, path, &status);
            break;
        }

        case DHF_CMD_DELETE: {
            dhf_host_fs_unlink(&dev->host_fs, path, &status);
            break;
        }

        case DHF_CMD_RENAME: {
            const char *newp = resolve_guest_str(dev, a1);
            dhf_host_fs_rename(&dev->host_fs, path, newp, &status);
            break;
        }

        case DHF_CMD_OPENDIR: {
            int h = dhf_host_fs_opendir(&dev->host_fs, path, &status);
            if (h >= 0) s->d0 = htonl((uint32_t)h);
            break;
        }

        case DHF_CMD_READDIR: {
            char entry_name[DHF_PATH_MAX];
            uint32_t fsize = 0, fmode = 0;
            int ret = dhf_host_fs_readdir(&dev->host_fs, (int)d0, entry_name, sizeof(entry_name), &fsize, &fmode, &status);
            if (ret > 0 && a1) {
                size_t nlen = strlen(entry_name) + 1;
                size_t total = nlen + 8;
                void *dest = resolve_guest_ptr(dev, a1, total);
                if (dest) {
                    memcpy(dest, entry_name, nlen);
                    uint32_t meta[2] = { htonl(fsize), htonl(fmode) };
                    memcpy((char*)dest + nlen, meta, sizeof(meta));
                }
                s->d1 = htonl((uint32_t)total);
            } else {
                s->d1 = 0;
            }
            break;
        }

        case DHF_CMD_PING: {
            status = DHF_ERR_OK;
            break;
        }

        default:
            status = DHF_ERR_UNSUPPORTED;
            break;
    }

    s->status = status;
    __sync_synchronize();
    s->command = DHF_CMD_IDLE;
    __sync_synchronize();
    return 0;
}

uint8_t dhf_emu_device_read8(dhf_emu_device_t *dev, uint32_t offset) {
    if (!dev || !dev->shared_mem || offset >= sizeof(struct dhf_shared)) return 0xFF;
    return ((uint8_t*)dev->shared_mem)[offset];
}

void dhf_emu_device_write8(dhf_emu_device_t *dev, uint32_t offset, uint8_t val) {
    if (!dev || !dev->shared_mem || offset >= sizeof(struct dhf_shared)) return;
    ((uint8_t*)dev->shared_mem)[offset] = val;

    if (offset == offsetof(struct dhf_shared, command) && val != DHF_CMD_IDLE && val != DHF_CMD_RETURN) {
        dhf_emu_device_process(dev);
    }
}

uint16_t dhf_emu_device_read16(dhf_emu_device_t *dev, uint32_t offset) {
    uint8_t hi = dhf_emu_device_read8(dev, offset);
    uint8_t lo = dhf_emu_device_read8(dev, offset + 1);
    return (uint16_t)((hi << 8) | lo);
}

void dhf_emu_device_write16(dhf_emu_device_t *dev, uint32_t offset, uint16_t val) {
    dhf_emu_device_write8(dev, offset, (uint8_t)(val >> 8));
    dhf_emu_device_write8(dev, offset + 1, (uint8_t)(val & 0xFF));
}

uint32_t dhf_emu_device_read32(dhf_emu_device_t *dev, uint32_t offset) {
    uint16_t hi = dhf_emu_device_read16(dev, offset);
    uint16_t lo = dhf_emu_device_read16(dev, offset + 2);
    return ((uint32_t)hi << 16) | lo;
}

void dhf_emu_device_write32(dhf_emu_device_t *dev, uint32_t offset, uint32_t val) {
    dhf_emu_device_write16(dev, offset, (uint16_t)(val >> 16));
    dhf_emu_device_write16(dev, offset + 2, (uint16_t)(val & 0xFFFF));
}
