//════════════════════════════════════════════════════════════════════════════════════════════════
// File:   q9_dhf.c                                                                        Ver. 1.00
// Owner:  Claude
// Desc.:  Implementierung, siehe q9_dhf.h. Die eigentliche Host-Dateisystem-Logik (open/close/
//         read/write/seek/mkdir/rmdir/unlink/rename/opendir/readdir, Basepath-Confinement) ist
//         inhaltlich aus Q9-OS/Q9-DHF-68k/driver/dhfdrv-68k.c uebernommen -- dort lief sie (noch)
//         als 68k-Treibercode, der auf dem Host direkt fopen()/mkdir() aufrief (funktioniert nur
//         im reinen Host-Simulator-Testharness). Hier laeuft dieselbe Logik dort, wo sie auf
//         echter/emulierter Hardware tatsaechlich laufen KANN: als natives Emulator-Geraet. Der
//         68k-Treiber wird dadurch selbst viel kuerzer -- nur noch Register/Puffer beschreiben
//         und CMD ausloesen, s. Q9-OS/Q9-DHF-68k/driver/dhfdrv-68k.c (Neufassung).
//
// Edition History
//─────────┬──────┬────────────────────────────────────────────────────────────────────────┬──────
// Date    │ Ver. │ Description                                                            │ By
//─────────┼──────┼────────────────────────────────────────────────────────────────────────┬──────
// 26-09-25│ 1.00 │ Erster Wurf                                                             │ Cld
//═════════╧══════╧════════════════════════════════════════════════════════════════════════╧══════
#include "q9_dhf.h"
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>

void q9_dhf_init(q9_dhf_t *state, const char *basepath)
{
    memset(state, 0, sizeof(*state));
    strncpy(state->basepath, basepath, sizeof(state->basepath) - 1);
}

//────────────────────────────────────────────────────────────────────────────────────────────────
// Function: dhf_confine
// Desc.:    Haengt PATH/PATH2 (Gast-relativ, '/'-getrennt) an basepath an. Lehnt ".."-Komponenten
//           ab (kein Verlassen des Basepath) -- dieselbe Idee wie confined_path() im Host-
//           Simulator (Q9-DHF-68k/host_simulator/fs_ops.c), hier direkt gegen den Registerpuffer.
//────────────────────────────────────────────────────────────────────────────────────────────────
static int dhf_confine(q9_dhf_t *s, const char *rel, char *out, size_t out_sz)
{
    if (strstr(rel, "..") != NULL) {
        return -1;
    }
    int n = snprintf(out, out_sz, "%s/%s", s->basepath, rel);
    return (n > 0 && (size_t)n < out_sz) ? 0 : -1;
}

/* Registerfenster ist big-endian (wie der 68k-Bus), der Host hier ist es i.d.R. nicht (Apple
   Silicon/x86 = little-endian) -- memcpy(int32_t) waere deshalb falsch; explizit packen/entpacken. */
static uint32_t be32_load(const uint8_t *p)
{
    return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) | ((uint32_t)p[2] << 8) | (uint32_t)p[3];
}

static void be32_store(uint8_t *p, uint32_t v)
{
    p[0] = (uint8_t)(v >> 24);
    p[1] = (uint8_t)(v >> 16);
    p[2] = (uint8_t)(v >> 8);
    p[3] = (uint8_t)(v);
}

static int dhf_alloc_slot(q9_dhf_t *s)
{
    int i;
    for (i = 0; i < Q9_DHF_MAX_HANDLES; i++) {
        if (s->slots[i].kind == Q9_DHF_SLOT_FREE) return i;
    }
    return -1;
}

static const char *dhf_open_mode(uint32_t flags)
{
    /* Gast-ARG1-Flags, grob an O_RDONLY(0)/O_WRONLY(1)/O_RDWR(2), Bit2=O_CREAT, Bit3=O_TRUNC,
       Bit4=O_APPEND angelehnt -- exakte Bitbedeutung ist Vertrag mit dem 68k-Treiber, s. dessen
       Kopfkommentar (dhfdrv-68k.c). */
    int acc = flags & 0x3;
    int creat = (flags & 0x4) != 0;
    int trunc = (flags & 0x8) != 0;
    int append = (flags & 0x10) != 0;

    if (append) return "a+";
    if (creat && trunc) return "wb+";
    if (creat) return "ab+";
    if (acc == 0) return "rb";
    return "rb+";
}

//────────────────────────────────────────────────────────────────────────────────────────────────
// Function: dhf_execute
// Desc.:    Fuehrt genau EIN Kommando aus dem Registerpuffer aus, schreibt RESULT/ERRNO/HANDLE
//           (und ggf. DATA) zurueck. Synchron -- laeuft im Emulator-Hauptthread, kein IRQ noetig
//           (der Gast pollt/wartet nicht: der Schreibzugriff auf CMD kehrt erst zurueck, wenn das
//           Ergebnis schon dasteht, exakt wie bei einem echten synchronen Bus-Zugriff).
//────────────────────────────────────────────────────────────────────────────────────────────────
static void dhf_execute(q9_dhf_t *s, uint8_t cmd)
{
    uint8_t *r = s->regs;
    char path[Q9_DHF_PATH_SIZE + 512];
    char path2[Q9_DHF_PATH_SIZE + 512];
    int32_t result = -1;
    int32_t err = 0;
    uint8_t handle = r[Q9_DHF_OFF_HANDLE];
    uint32_t arg1, arg2;

    arg1 = be32_load(&r[Q9_DHF_OFF_ARG1]);
    arg2 = be32_load(&r[Q9_DHF_OFF_ARG2]);

    switch (cmd) {
    case Q9_DHF_CMD_OPEN: {
        int slot = dhf_alloc_slot(s);
        if (slot < 0 || dhf_confine(s, (char *)&r[Q9_DHF_OFF_PATH], path, sizeof(path)) != 0) {
            err = EMFILE;
            break;
        }
        FILE *f = fopen(path, dhf_open_mode(arg1));
        if (!f) { err = errno; break; }
        s->slots[slot].kind = Q9_DHF_SLOT_FILE;
        s->slots[slot].h.file = f;
        handle = (uint8_t)slot;
        result = 0;
        break;
    }
    case Q9_DHF_CMD_CLOSE:
        if (handle < Q9_DHF_MAX_HANDLES && s->slots[handle].kind == Q9_DHF_SLOT_FILE) {
            fclose(s->slots[handle].h.file);
            s->slots[handle].kind = Q9_DHF_SLOT_FREE;
            result = 0;
        } else {
            err = EBADF;
        }
        break;
    case Q9_DHF_CMD_READ:
        if (handle < Q9_DHF_MAX_HANDLES && s->slots[handle].kind == Q9_DHF_SLOT_FILE) {
            size_t want = arg1;
            if (want > Q9_DHF_DATA_SIZE) want = Q9_DHF_DATA_SIZE;
            result = (int32_t)fread(&r[Q9_DHF_OFF_DATA], 1, want, s->slots[handle].h.file);
        } else {
            err = EBADF;
        }
        break;
    case Q9_DHF_CMD_WRITE:
        if (handle < Q9_DHF_MAX_HANDLES && s->slots[handle].kind == Q9_DHF_SLOT_FILE) {
            size_t want = arg1;
            if (want > Q9_DHF_DATA_SIZE) want = Q9_DHF_DATA_SIZE;
            result = (int32_t)fwrite(&r[Q9_DHF_OFF_DATA], 1, want, s->slots[handle].h.file);
        } else {
            err = EBADF;
        }
        break;
    case Q9_DHF_CMD_SEEK:
        if (handle < Q9_DHF_MAX_HANDLES && s->slots[handle].kind == Q9_DHF_SLOT_FILE) {
            int whence = (arg2 == 1) ? SEEK_CUR : (arg2 == 2) ? SEEK_END : SEEK_SET;
            if (fseek(s->slots[handle].h.file, (long)(int32_t)arg1, whence) == 0) {
                result = (int32_t)ftell(s->slots[handle].h.file);
            } else {
                err = errno;
            }
        } else {
            err = EBADF;
        }
        break;
    case Q9_DHF_CMD_MKDIR:
        if (dhf_confine(s, (char *)&r[Q9_DHF_OFF_PATH], path, sizeof(path)) == 0) {
            result = mkdir(path, (mode_t)(arg1 ? arg1 : 0755)) == 0 ? 0 : -1;
            if (result < 0) err = errno;
        } else {
            err = EACCES;
        }
        break;
    case Q9_DHF_CMD_RMDIR:
        if (dhf_confine(s, (char *)&r[Q9_DHF_OFF_PATH], path, sizeof(path)) == 0) {
            result = rmdir(path) == 0 ? 0 : -1;
            if (result < 0) err = errno;
        } else {
            err = EACCES;
        }
        break;
    case Q9_DHF_CMD_UNLINK:
        if (dhf_confine(s, (char *)&r[Q9_DHF_OFF_PATH], path, sizeof(path)) == 0) {
            result = unlink(path) == 0 ? 0 : -1;
            if (result < 0) err = errno;
        } else {
            err = EACCES;
        }
        break;
    case Q9_DHF_CMD_RENAME:
        if (dhf_confine(s, (char *)&r[Q9_DHF_OFF_PATH], path, sizeof(path)) == 0
            && dhf_confine(s, (char *)&r[Q9_DHF_OFF_PATH2], path2, sizeof(path2)) == 0) {
            result = rename(path, path2) == 0 ? 0 : -1;
            if (result < 0) err = errno;
        } else {
            err = EACCES;
        }
        break;
    case Q9_DHF_CMD_OPENDIR: {
        int slot = dhf_alloc_slot(s);
        if (slot < 0 || dhf_confine(s, (char *)&r[Q9_DHF_OFF_PATH], path, sizeof(path)) != 0) {
            err = EMFILE;
            break;
        }
        DIR *d = opendir(path);
        if (!d) { err = errno; break; }
        s->slots[slot].kind = Q9_DHF_SLOT_DIR;
        s->slots[slot].h.dir = d;
        handle = (uint8_t)slot;
        result = 0;
        break;
    }
    case Q9_DHF_CMD_READDIR:
        if (handle < Q9_DHF_MAX_HANDLES && s->slots[handle].kind == Q9_DHF_SLOT_DIR) {
            struct dirent *de = readdir((DIR *)s->slots[handle].h.dir);
            if (de) {
                strncpy((char *)&r[Q9_DHF_OFF_DATA], de->d_name, Q9_DHF_DATA_SIZE - 1);
                r[Q9_DHF_OFF_DATA + Q9_DHF_DATA_SIZE - 1] = 0;
                result = 1;
            } else {
                result = 0;
            }
        } else {
            err = EBADF;
        }
        break;
    case Q9_DHF_CMD_CLOSEDIR:
        if (handle < Q9_DHF_MAX_HANDLES && s->slots[handle].kind == Q9_DHF_SLOT_DIR) {
            closedir((DIR *)s->slots[handle].h.dir);
            s->slots[handle].kind = Q9_DHF_SLOT_FREE;
            result = 0;
        } else {
            err = EBADF;
        }
        break;
    case Q9_DHF_CMD_TRUNCATE:
        if (dhf_confine(s, (char *)&r[Q9_DHF_OFF_PATH], path, sizeof(path)) == 0) {
            result = truncate(path, (off_t)arg1) == 0 ? 0 : -1;
            if (result < 0) err = errno;
        } else {
            err = EACCES;
        }
        break;
    default:
        err = EINVAL;
        break;
    }

    r[Q9_DHF_OFF_HANDLE] = handle;
    be32_store(&r[Q9_DHF_OFF_RESULT], (uint32_t)result);
    be32_store(&r[Q9_DHF_OFF_ERRNO], (uint32_t)err);
}

//────────────────────────────────────────────────────────────────────────────────────────────────
// Function: dhf_dev_read8 / dhf_dev_write8 / q9_devtype_dhf
// Desc.:    Vtable-Adapter fuer die Geraete-Registry (devreg.h). Ein Schreibzugriff auf CMD
//           (Offset 0) loest dhf_execute() synchron aus; alle anderen Adressen sind reines
//           Byte-Array-Backing (Register/Puffer), von dhf_execute() vor-/nachher befuellt.
//────────────────────────────────────────────────────────────────────────────────────────────────
static uint8_t dhf_dev_read8(q9_device_t *dev, uint32_t addr)
{
    q9_dhf_t *s = (q9_dhf_t *)dev->state;
    uint32_t off = addr - dev->base;             /* absolute Adresse, s. cf_dev_read8-Vorbild */
    if (off >= Q9_DHF_WINDOW_SIZE) return 0xFF;
    return s->regs[off];
}

static void dhf_dev_write8(q9_device_t *dev, uint32_t addr, uint8_t val)
{
    q9_dhf_t *s = (q9_dhf_t *)dev->state;
    uint32_t off = addr - dev->base;
    if (off >= Q9_DHF_WINDOW_SIZE) return;
    s->regs[off] = val;
    if (off == Q9_DHF_OFF_CMD && val != Q9_DHF_CMD_NONE) {
        dhf_execute(s, val);
    }
}

const q9_device_vtable_t q9_devtype_dhf = {
    .read8         = dhf_dev_read8,
    .write8        = dhf_dev_write8,
    .read16        = NULL,
    .write16       = NULL,
    .read32        = NULL,
    .write32       = NULL,
    .poll          = NULL,
    .irq_pending   = NULL,
    .reset         = NULL,
    .irq_vector_fn = NULL,
};

const q9_devdesc_t q9_devdesc_dhf = {
    .type              = "dhf",
    .desc              = "DHF Host-Passthrough-Dateisystem (MMIO-Bruecke zu echten Host-Dateien)",
    .vt                = &q9_devtype_dhf,
    .use_table_default = 1,
    .extra_fields      = NULL,     /* 2026-09-25: noch hartkodierter Basepath, s. m68krt.c attach --
                                       Config-Feld "hostpath" folgt als eigener Schritt, analog "cf"s
                                       "image"-Feld (s. Kopfkommentar q9_dhf.h). */
    .extra_field_count = 0,
};

//────────────────────────────────────────────────────────────────────────────────────────────────
// EOF q9_dhf.c                                                                            Ver. 1.00
//────────────────────────────────────────────────────────────────────────────────────────────────
