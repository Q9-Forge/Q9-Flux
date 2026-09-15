/*
 * Q9 board: Compact-Flash interface (ATA-PIO minimal protocol).
 *
 * Ported from the Musashi-based implementation in
 * Q9-Flux-68k/src/devices/cf/cf.c -- register map, ATA-PIO protocol and
 * the RBF/PCF sector-size heuristic are unchanged; only the QOM/
 * MemoryRegion plumbing is new. This single device type serves both the
 * onboard interface and the RC2014-SC145 second interface -- q9board.c
 * simply instantiates it twice at their respective base addresses (s.
 * the original's own q9_cf_attach(), which is likewise interface-
 * agnostic). Each instance carries BOTH units (master/slave, selected
 * by the DEV bit in LBA3, s. q9_cf_cur_unit()); unit 1 (slave) is
 * unfitted -- and every ATA command against it answers ERR, like an
 * empty slot -- unless a "slave-image" property is given, same as
 * master/unit 0 with "image".
 *
 * Deliberately still plain stdio file I/O (fopen/fread/fwrite), like the
 * Musashi original -- NOT QEMU's block layer (BlockBackend/"-drive").
 * That's not an oversight: the RBF-vs-PCF sector-size heuristic (reading
 * raw header bytes at fixed file offsets to tell 256-byte-LSN OS-9
 * images from 512-byte-sector FAT images) is a host-file-format
 * heuristic this specific device needs to run itself, and QEMU's block
 * layer has no hook for a device to influence how its own backing file
 * is probed. Going through BlockBackend would mean re-deriving that
 * heuristic against QEMU's format-probing instead of the real file --
 * out of scope for a faithful port; plain stdio keeps this device's
 * behaviour byte-for-byte identical to the original, including the
 * heuristic's documented edge cases.
 *
 * No IRQ, matching the original (CF was never polled by the interpreter
 * main loop even before the Musashi->device-registry migration).
 *
 * This file is not (yet) part of Q9-Flux's own MIT-licensed code base --
 * it lives inside the vendored QEMU source tree and is licensed under
 * QEMU's own terms (GPL), like the rest of this directory.
 */

#include "qemu/osdep.h"
#include "qapi/error.h"
#include "hw/core/sysbus.h"
#include "hw/core/qdev-properties.h"
#include "qom/object.h"
#include <stdio.h>
#include <string.h>

#define TYPE_Q9_CF "q9-cf"
OBJECT_DECLARE_SIMPLE_TYPE(Q9CFState, Q9_CF)

/* Matches Q9_BOARD_CF_BASE/_TOP in Q9-Flux-68k/src/kernel/q9board.h (now
 * Q9-Flux-68k/src/devices/cf/cf.h). */
#define Q9_CF_WINDOW_SIZE 0x100

/* ATA register offsets relative to the device base -- s. cf.c CF_REG_*. */
#define CF_REG_DATA     0x00u
#define CF_REG_FEAT     0x01u
#define CF_REG_SECCNT   0x02u
#define CF_REG_LBA0     0x03u
#define CF_REG_LBA1     0x04u
#define CF_REG_LBA2     0x05u
#define CF_REG_LBA3     0x06u
#define CF_REG_CMD      0x07u

#define CF_CMD_READ     0x20u
#define CF_CMD_WRITE    0x30u
#define CF_CMD_SETFEAT  0xEFu
#define CF_CMD_IDENTIFY 0xECu

#define CF_STAT_ERR     0x01u
#define CF_STAT_DRQ     0x08u
#define CF_STAT_RDY     0x40u

#define CF_SECTOR_SIZE  512u

#define CF_FMT_AUTO 0
#define CF_FMT_RBF  1
#define CF_FMT_PCF  2

#define RBF_DD_DIR     0x08u
#define RBF_DD_LSNSIZE 0x68u

typedef struct {
    char    *path;               /* qdev property, NULL = unit not fitted */
    FILE    *file;                /* lazily opened, like the original    */
    uint32_t image_sector_size;   /* 0 = not yet detected; 256/512        */
    uint32_t start_sector;
    int      format;              /* CF_FMT_*                             */
} Q9CFUnit;

struct Q9CFState {
    SysBusDevice parent_obj;
    MemoryRegion iomem;

    Q9CFUnit unit[2];             /* [0] = master, [1] = slave (unused so far) */
    uint32_t lba;
    uint8_t  lba3;
    uint8_t  sectcnt;
    uint8_t  status;
    uint8_t  sector[CF_SECTOR_SIZE];
    uint32_t pos;
    uint32_t transfer_size;
    bool     write_pending;
    uint32_t remaining;

    /* qdev properties, one triplet per unit (s. Dateikopf) */
    char *image;
    char *format_str;
    uint32_t start_sector_prop;
    char *slave_image;
    char *slave_format_str;
    uint32_t slave_start_sector_prop;
};

/* Shared by q9_cf_realize() for both units -- name is only used in the
 * error message, so the same helper serves "format"/"slave-format". */
static bool q9_cf_parse_format(const char *name, const char *str, int *out,
                                Error **errp)
{
    if (!str) {
        *out = CF_FMT_AUTO;
        return true;
    }
    if (strcmp(str, "auto") == 0) {
        *out = CF_FMT_AUTO;
    } else if (strcmp(str, "rbf") == 0) {
        *out = CF_FMT_RBF;
    } else if (strcmp(str, "pcf") == 0 || strcmp(str, "fat") == 0) {
        *out = CF_FMT_PCF;
    } else {
        error_setg(errp, "q9-cf: invalid '%s' value '%s' "
                   "(expected auto/rbf/pcf)", name, str);
        return false;
    }
    return true;
}

static Q9CFUnit *q9_cf_cur_unit(Q9CFState *s)
{
    return &s->unit[(s->lba3 >> 4) & 1u];
}

static int q9_cf_ensure_open(Q9CFState *s)
{
    Q9CFUnit *u = q9_cf_cur_unit(s);

    if (u->file) {
        return 1;
    }
    if (!u->path) {
        return 0;
    }
    u->file = fopen(u->path, "r+b");
    if (!u->file) {
        u->file = fopen(u->path, "w+b");
    }
    if (u->file && u->image_sector_size == 0) {
        u->image_sector_size = CF_SECTOR_SIZE;

        if (u->format != CF_FMT_PCF) {
            uint8_t hdr[CF_SECTOR_SIZE];
            size_t  n;
            long    cur;

            cur = ftell(u->file);
            fseek(u->file, (long)u->start_sector * (long)CF_SECTOR_SIZE, SEEK_SET);
            n = fread(hdr, 1, sizeof(hdr), u->file);
            fseek(u->file, cur, SEEK_SET);

            if (n >= 128) {
                uint32_t root_lsn = ((uint32_t)hdr[RBF_DD_DIR] << 16) |
                                    ((uint32_t)hdr[RBF_DD_DIR + 1] << 8) |
                                     (uint32_t)hdr[RBF_DD_DIR + 2];
                uint16_t lsn_size = (uint16_t)(((uint16_t)hdr[RBF_DD_LSNSIZE] << 8) |
                                                (uint16_t)hdr[RBF_DD_LSNSIZE + 1]);
                if (lsn_size == 256u) {
                    u->image_sector_size = 256u;
                } else if (lsn_size == 512u) {
                    u->image_sector_size = 512u;
                } else if (root_lsn > 0) {
                    uint8_t fd0 = 0;
                    fseek(u->file, (long)root_lsn * 256L, SEEK_SET);
                    if (fread(&fd0, 1, 1, u->file) == 1 && (fd0 & 0x80u)) {
                        u->image_sector_size = 256u;
                    }
                    fseek(u->file, cur, SEEK_SET);
                }
            }
        }
    }
    return u->file != NULL;
}

static void q9_cf_load_sector(Q9CFState *s)
{
    memset(s->sector, 0, CF_SECTOR_SIZE);
    if (q9_cf_ensure_open(s)) {
        Q9CFUnit *u = q9_cf_cur_unit(s);
        uint32_t img_sec = u->image_sector_size ? u->image_sector_size : CF_SECTOR_SIZE;
        fseek(u->file, (long)(u->start_sector + s->lba) * (long)img_sec, SEEK_SET);
        fread(s->sector, 1, img_sec, u->file);
    }
    s->pos = 0;
}

static void q9_cf_load_write_buffer(Q9CFState *s)
{
    memset(s->sector, 0, CF_SECTOR_SIZE);
    if (q9_cf_ensure_open(s)) {
        Q9CFUnit *u = q9_cf_cur_unit(s);
        uint32_t img_sec = u->image_sector_size ? u->image_sector_size : CF_SECTOR_SIZE;
        fseek(u->file, (long)(u->start_sector + s->lba) * (long)img_sec, SEEK_SET);
        fread(s->sector, 1, img_sec, u->file);
        if (img_sec < CF_SECTOR_SIZE) {
            fseek(u->file, (long)(u->start_sector + s->lba + 1u) * (long)img_sec, SEEK_SET);
            fread(s->sector + img_sec, 1, CF_SECTOR_SIZE - img_sec, u->file);
        }
    }
    s->pos = 0;
}

static void q9_cf_store_sector(Q9CFState *s)
{
    if (q9_cf_ensure_open(s)) {
        Q9CFUnit *u = q9_cf_cur_unit(s);
        uint32_t img_sec = u->image_sector_size ? u->image_sector_size : CF_SECTOR_SIZE;
        uint32_t written = s->pos;

        if (written == 0 || written > CF_SECTOR_SIZE) {
            written = CF_SECTOR_SIZE;
        }
        fseek(u->file, (long)(u->start_sector + s->lba) * (long)img_sec, SEEK_SET);
        if (img_sec < CF_SECTOR_SIZE) {
            fwrite(s->sector, 1, img_sec, u->file);
            fseek(u->file, (long)(u->start_sector + s->lba + 1u) * (long)img_sec, SEEK_SET);
            fwrite(s->sector + img_sec, 1, CF_SECTOR_SIZE - img_sec, u->file);
        } else {
            fwrite(s->sector, 1, written < img_sec ? written : img_sec, u->file);
        }
        fflush(u->file);
    }
}

static void q9_cf_identify(Q9CFState *s)
{
    uint32_t sectors = 0;

    memset(s->sector, 0, CF_SECTOR_SIZE);
    if (q9_cf_ensure_open(s)) {
        Q9CFUnit *u = q9_cf_cur_unit(s);
        long cur;

        if (u->format != CF_FMT_PCF) {
            uint8_t hdr[16];
            cur = ftell(u->file);
            fseek(u->file, 0, SEEK_SET);
            if (fread(hdr, 1, sizeof(hdr), u->file) == sizeof(hdr)) {
                uint32_t dd_tot = ((uint32_t)hdr[0] << 16) |
                                  ((uint32_t)hdr[1] << 8) |
                                   (uint32_t)hdr[2];
                if (dd_tot > 0) {
                    sectors = dd_tot;
                }
            }
            fseek(u->file, cur, SEEK_SET);
        }

        fseek(u->file, 0, SEEK_END);
        cur = ftell(u->file);
        if (sectors == 0 && cur > 0) {
            uint32_t img_sec = u->image_sector_size ? u->image_sector_size : CF_SECTOR_SIZE;
            unsigned long skip = (unsigned long)u->start_sector * (unsigned long)img_sec;
            sectors = cur > (long)skip ? (uint32_t)(((unsigned long)cur - skip) / img_sec) : 0;
        }
    }

    s->sector[120] = (uint8_t)(sectors & 0xFFu);
    s->sector[121] = (uint8_t)((sectors >> 8) & 0xFFu);
    s->sector[122] = (uint8_t)((sectors >> 16) & 0xFFu);
    s->sector[123] = (uint8_t)((sectors >> 24) & 0xFFu);
    s->pos = 0;
    s->transfer_size = CF_SECTOR_SIZE;
}

static uint32_t q9_cf_transfer_size(Q9CFState *s)
{
    uint32_t size;

    if (!q9_cf_ensure_open(s)) {
        return CF_SECTOR_SIZE;
    }
    size = q9_cf_cur_unit(s)->image_sector_size;
    if (size == 0) {
        size = CF_SECTOR_SIZE;
    }
    return (size < CF_SECTOR_SIZE) ? size : CF_SECTOR_SIZE;
}

static uint8_t q9_cf_reg_read(Q9CFState *s, uint32_t off)
{
    if (off == CF_REG_CMD) {
        return s->status;
    }
    if (off == CF_REG_DATA) {
        uint32_t xfer_size = s->transfer_size ? s->transfer_size : CF_SECTOR_SIZE;
        if (s->pos < xfer_size) {
            uint8_t v = s->sector[s->pos++];
            if (s->pos >= xfer_size) {
                s->remaining--;
                s->lba++;
                if (s->remaining > 0) {
                    q9_cf_load_sector(s);
                    s->status = (uint8_t)(CF_STAT_RDY | CF_STAT_DRQ);
                } else {
                    s->status = (uint8_t)CF_STAT_RDY;
                }
            }
            return v;
        }
        return 0;
    }
    return 0;
}

static void q9_cf_reg_write(Q9CFState *s, uint32_t off, uint8_t val)
{
    switch (off) {
    case CF_REG_FEAT:
        return;
    case CF_REG_SECCNT:
        s->sectcnt = val;
        return;
    case CF_REG_LBA0:
        s->lba = (s->lba & 0xFFFFFF00u) | val;
        return;
    case CF_REG_LBA1:
        s->lba = (s->lba & 0xFFFF00FFu) | ((uint32_t)val << 8);
        return;
    case CF_REG_LBA2:
        s->lba = (s->lba & 0xFF00FFFFu) | ((uint32_t)val << 16);
        return;
    case CF_REG_LBA3:
        s->lba3 = val;
        s->lba = (s->lba & 0x00FFFFFFu) | ((uint32_t)(val & 0x0Fu) << 24);
        return;
    case CF_REG_DATA:
        if (s->write_pending && s->pos < s->transfer_size) {
            s->sector[s->pos++] = val;
            if (s->pos >= s->transfer_size) {
                q9_cf_store_sector(s);
                s->remaining--;
                s->lba++;
                if (s->remaining > 0) {
                    s->pos = 0;
                    s->status = (uint8_t)(CF_STAT_RDY | CF_STAT_DRQ);
                } else {
                    s->write_pending = false;
                    s->status = CF_STAT_RDY;
                }
            }
        }
        return;
    case CF_REG_CMD:
        if (!q9_cf_cur_unit(s)->path) {
            s->write_pending = false;
            s->status = (uint8_t)(CF_STAT_RDY | CF_STAT_ERR);
            return;
        }
        if (val == CF_CMD_READ) {
            s->remaining = s->sectcnt ? s->sectcnt : 256u;
            s->transfer_size = q9_cf_transfer_size(s);
            q9_cf_load_sector(s);
            s->write_pending = false;
            s->status = (uint8_t)(CF_STAT_RDY | CF_STAT_DRQ);
        } else if (val == CF_CMD_WRITE) {
            s->remaining = s->sectcnt ? s->sectcnt : 256u;
            s->transfer_size = q9_cf_transfer_size(s);
            q9_cf_load_write_buffer(s);
            s->write_pending = true;
            s->status = (uint8_t)(CF_STAT_RDY | CF_STAT_DRQ);
        } else if (val == CF_CMD_SETFEAT) {
            s->status = CF_STAT_RDY;
        } else if (val == CF_CMD_IDENTIFY) {
            s->remaining = 1;
            q9_cf_identify(s);
            s->write_pending = false;
            s->status = (uint8_t)(CF_STAT_RDY | CF_STAT_DRQ);
        } else {
            s->status = (uint8_t)(CF_STAT_RDY | CF_STAT_ERR);
        }
        return;
    default:
        return;
    }
}

/* CF keeps its own 16/32-bit paths rather than QEMU's usual byte-address
 * synthesis: a wide access AT the data register (offset 0) reads/writes
 * several consecutive byte transfers of the SAME ATA data register (no
 * address progression -- matches cf_dev_read16/32 in the original).
 * Everywhere else a wide access is just independent per-byte dispatch at
 * incrementing offsets. Big-endian (MSB first), matching the rest of
 * this board and the original's explicit hi/lo ordering. */
static uint64_t q9_cf_read(void *opaque, hwaddr addr, unsigned size)
{
    Q9CFState *s = opaque;
    uint64_t val = 0;
    unsigned i;

    if (addr == CF_REG_DATA) {
        for (i = 0; i < size; i++) {
            val = (val << 8) | q9_cf_reg_read(s, CF_REG_DATA);
        }
        return val;
    }
    for (i = 0; i < size; i++) {
        val = (val << 8) | q9_cf_reg_read(s, (uint32_t)addr + i);
    }
    return val;
}

static void q9_cf_write(void *opaque, hwaddr addr, uint64_t val, unsigned size)
{
    Q9CFState *s = opaque;
    unsigned i;

    if (addr == CF_REG_DATA) {
        for (i = 0; i < size; i++) {
            q9_cf_reg_write(s, CF_REG_DATA, (uint8_t)(val >> (8 * (size - 1 - i))));
        }
        return;
    }
    for (i = 0; i < size; i++) {
        q9_cf_reg_write(s, (uint32_t)addr + i, (uint8_t)(val >> (8 * (size - 1 - i))));
    }
}

static const MemoryRegionOps q9_cf_ops = {
    .read = q9_cf_read,
    .write = q9_cf_write,
    .valid.min_access_size = 1,
    .valid.max_access_size = 4,
    .impl.min_access_size = 1,
    .impl.max_access_size = 4,
    .endianness = DEVICE_BIG_ENDIAN,
};

static void q9_cf_realize(DeviceState *dev, Error **errp)
{
    Q9CFState *s = Q9_CF(dev);
    int format, slave_format;

    if (!q9_cf_parse_format("format", s->format_str, &format, errp)) {
        return;
    }
    if (!q9_cf_parse_format("slave-format", s->slave_format_str,
                             &slave_format, errp)) {
        return;
    }

    /* image==NULL leaves a unit unfitted -- every command against it
     * answers ERR, like a real interface with no card inserted (matches
     * q9_cf_reg_write's own CF_REG_CMD handling). */
    s->unit[0].path = s->image;
    s->unit[0].format = format;
    s->unit[0].start_sector = s->start_sector_prop;
    s->unit[1].path = s->slave_image;
    s->unit[1].format = slave_format;
    s->unit[1].start_sector = s->slave_start_sector_prop;
    s->status = CF_STAT_RDY;

    memory_region_init_io(&s->iomem, OBJECT(dev), &q9_cf_ops, s,
                           TYPE_Q9_CF, Q9_CF_WINDOW_SIZE);
    sysbus_init_mmio(SYS_BUS_DEVICE(dev), &s->iomem);
}

static const Property q9_cf_properties[] = {
    DEFINE_PROP_STRING("image", Q9CFState, image),
    DEFINE_PROP_STRING("format", Q9CFState, format_str),
    DEFINE_PROP_UINT32("start-sector", Q9CFState, start_sector_prop, 0),
    DEFINE_PROP_STRING("slave-image", Q9CFState, slave_image),
    DEFINE_PROP_STRING("slave-format", Q9CFState, slave_format_str),
    DEFINE_PROP_UINT32("slave-start-sector", Q9CFState,
                        slave_start_sector_prop, 0),
};

static void q9_cf_class_init(ObjectClass *oc, const void *data)
{
    DeviceClass *dc = DEVICE_CLASS(oc);

    dc->desc = "Q9 board Compact-Flash interface (ATA-PIO, RBF/PCF images)";
    dc->realize = q9_cf_realize;
    device_class_set_props(dc, q9_cf_properties);
    set_bit(DEVICE_CATEGORY_STORAGE, dc->categories);
}

static const TypeInfo q9_cf_info = {
    .name          = TYPE_Q9_CF,
    .parent        = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(Q9CFState),
    .class_init    = q9_cf_class_init,
};

/* RC2014-SC145 second interface: a QOM *subclass* of "q9-cf" (not a
 * second top-level TypeInfo with a copy-pasted class_init) -- it needs
 * no state/behaviour of its own, only a distinct type name so
 * q9board.c's two instances can be configured independently via
 * "-global q9-cf.*"/"-global q9-cf2.*" (s. q9board.c's own comment at
 * the second instantiation -- "-global" keys off the type name, not
 * the instance, so two same-typed instances could not otherwise take
 * different property values from the command line). Subclassing means
 * realize/properties are simply inherited unchanged; a second sibling
 * TypeInfo of "sysbus-device" would instead need its own class_init,
 * AND its instances would fail Q9_CF()'s QOM type check inside the
 * (shared) realize function, since that check is tied to the literal
 * type name given to OBJECT_DECLARE_SIMPLE_TYPE. */
#define TYPE_Q9_CF2 "q9-cf2"

static const TypeInfo q9_cf2_info = {
    .name          = TYPE_Q9_CF2,
    .parent        = TYPE_Q9_CF,
};

static void q9_cf_register_types(void)
{
    type_register_static(&q9_cf_info);
    type_register_static(&q9_cf2_info);
}

type_init(q9_cf_register_types)
