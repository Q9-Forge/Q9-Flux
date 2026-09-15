/*
 * Q9 board: RTC72421 real-time clock emulation.
 *
 * Epson RTC72421, 16 byte-wide nibble registers. Reads return the host
 * clock (via QEMU's own qemu_get_timedate(), so it honours the standard
 * "-rtc" option); writes are ignored -- the host clock is authoritative.
 * Reading register 0 latches a fresh snapshot; all other registers read
 * from that latch, so a driver reading S1..W in order sees one
 * internally consistent timestamp without a rollover race.
 *
 * Ported from the Musashi-based implementation in
 * Q9-Flux-68k/src/devices/rtc72421/rtc72421.c -- register semantics are
 * unchanged, only the QOM/MemoryRegion plumbing is new.
 *
 * This file is not (yet) part of Q9-Flux's own MIT-licensed code base --
 * it lives inside the vendored QEMU source tree and is licensed under
 * QEMU's own terms (GPL), like the rest of this directory.
 */

#include "qemu/osdep.h"
#include "hw/core/sysbus.h"
#include "qom/object.h"
#include "system/rtc.h"

#define TYPE_Q9_RTC72421 "q9-rtc72421"
OBJECT_DECLARE_SIMPLE_TYPE(Q9RTC72421State, Q9_RTC72421)

struct Q9RTC72421State {
    SysBusDevice parent_obj;
    MemoryRegion iomem;
    uint8_t regs[13];   /* S1..W, indices 0..12 */
    bool latch_valid;
};

static void q9_rtc72421_refresh(Q9RTC72421State *s)
{
    struct tm tm;

    qemu_get_timedate(&tm, 0);

    int year2 = tm.tm_year % 100;         /* base 2000, matches the guest driver */
    int month = tm.tm_mon + 1;            /* struct tm: 0-11 -> 1-12 */

    s->regs[0]  = tm.tm_sec  % 10;
    s->regs[1]  = tm.tm_sec  / 10;
    s->regs[2]  = tm.tm_min  % 10;
    s->regs[3]  = tm.tm_min  / 10;
    s->regs[4]  = tm.tm_hour % 10;
    s->regs[5]  = tm.tm_hour / 10;         /* 24h mode: 0..2, no PM bit */
    s->regs[6]  = tm.tm_mday % 10;
    s->regs[7]  = tm.tm_mday / 10;
    s->regs[8]  = month % 10;
    s->regs[9]  = month / 10;
    s->regs[10] = year2 % 10;
    s->regs[11] = year2 / 10;
    s->regs[12] = tm.tm_wday;              /* struct tm: 0 = Sunday, same as 72421 */

    s->latch_valid = true;
}

static uint64_t q9_rtc72421_read(void *opaque, hwaddr addr, unsigned size)
{
    Q9RTC72421State *s = opaque;

    if (addr == 0 || !s->latch_valid) {
        q9_rtc72421_refresh(s);
    }
    if (addr <= 12) {
        return s->regs[addr];
    }
    if (addr == 15) {
        return 0x04;    /* Control F: bit 2 = 24h mode, fixed */
    }
    return 0x00;         /* Control D/E: never HOLD/BUSY/IRQ */
}

static void q9_rtc72421_write(void *opaque, hwaddr addr, uint64_t val,
                               unsigned size)
{
    /* Writes are deliberately ignored -- the host clock is authoritative,
     * matching the original Musashi-based implementation. */
}

static const MemoryRegionOps q9_rtc72421_ops = {
    .read = q9_rtc72421_read,
    .write = q9_rtc72421_write,
    .valid.min_access_size = 1,
    .valid.max_access_size = 1,
    .endianness = DEVICE_BIG_ENDIAN,
};

static void q9_rtc72421_realize(DeviceState *dev, Error **errp)
{
    Q9RTC72421State *s = Q9_RTC72421(dev);

    memory_region_init_io(&s->iomem, OBJECT(dev), &q9_rtc72421_ops, s,
                           TYPE_Q9_RTC72421, 16);
    sysbus_init_mmio(SYS_BUS_DEVICE(dev), &s->iomem);
    s->latch_valid = false;
}

static void q9_rtc72421_class_init(ObjectClass *oc, const void *data)
{
    DeviceClass *dc = DEVICE_CLASS(oc);

    dc->desc = "Q9 board RTC72421 real-time clock";
    dc->realize = q9_rtc72421_realize;
    set_bit(DEVICE_CATEGORY_MISC, dc->categories);
}

static const TypeInfo q9_rtc72421_info = {
    .name          = TYPE_Q9_RTC72421,
    .parent        = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(Q9RTC72421State),
    .class_init    = q9_rtc72421_class_init,
};

static void q9_rtc72421_register_types(void)
{
    type_register_static(&q9_rtc72421_info);
}

type_init(q9_rtc72421_register_types)
