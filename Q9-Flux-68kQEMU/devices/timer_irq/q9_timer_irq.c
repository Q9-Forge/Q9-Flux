/*
 * Q9 board: Timer/IRQ3 address-trigger.
 *
 * Pure address-trigger device, no data register: any byte access (read or
 * write) into $FFFF9000-$FFFF97FF turns the 100 Hz tick off, any access
 * into $FFFF9800-$FFFF9FFF turns it on. While on, the device raises the
 * m68k CPU's level 6 autovector interrupt once every 10 ms.
 *
 * Level 6 is deliberately autovectored (no IACK-time vector fetch, matching
 * the original board: real 68k autovectored interrupts bypass the vector
 * bus cycle entirely, the CPU supplies vector 30 = 24 + level 6 itself).
 * That also means there is no register the device could watch to notice
 * "the guest has taken it" and lower the request again -- unlike the
 * Musashi original, where m68krt_board_int_ack() (called for every level
 * alike, autovectored or not, since Musashi's interpreter models IACK as
 * one uniform callback) explicitly clears the request as part of taking
 * ANY interrupt. QEMU's autovector path has no equivalent hook. The
 * request is therefore pulsed: raised on each tick and lowered again by a
 * short one-shot timer 1 ms later -- long enough (thousands of guest
 * instructions) for the guest to take it, short enough to guarantee it is
 * lowered again well before the next 10 ms tick, so it can never retrigger
 * more than once per period. This is the QEMU-side equivalent of the
 * original's "one pulse per interpreter round" behaviour; the externally
 * visible contract (address windows, level, one interrupt per tick) is
 * unchanged.
 *
 * Ported from the Musashi-based implementation in
 * Q9-Flux-68k/src/devices/timer_irq/timer_irq.c -- register/address
 * semantics are unchanged, only the QOM/MemoryRegion/timer plumbing and
 * the IRQ-delivery mechanism (poll()+m68k_set_irq() there vs. a QEMUTimer
 * pulse here) are new.
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
#include "qemu/timer.h"
#include "target/m68k/cpu.h"

#define TYPE_Q9_TIMER_IRQ "q9-timer-irq"
OBJECT_DECLARE_SIMPLE_TYPE(Q9TimerIRQState, Q9_TIMER_IRQ)

/* Matches Q9_BOARD_TIRQ_OFF_BASE/_ON_BASE and Q9_BOARD_TIMER_PERIOD_MS in
 * Q9-Flux-68k/src/kernel/q9board.h. The OFF and ON windows are adjacent
 * 0x800-byte halves of one 0x1000-byte region, so a single MemoryRegion
 * covers both; ON_WINDOW_OFFSET is the OFF/ON split point within it. */
#define Q9_TIMER_IRQ_WINDOW_SIZE   0x1000
#define Q9_TIMER_IRQ_ON_OFFSET     0x800
#define Q9_TIMER_IRQ_PERIOD_NS     (10 * SCALE_MS)   /* 100 Hz, s.o. */
#define Q9_TIMER_IRQ_PULSE_NS      (1 * SCALE_MS)    /* s. Dateikopf */

/* Level 6, always autovectored (s. Dateikopf) -- vector = 24 + level, der
 * uebliche 68k-Autovektor fuer Interrupt-Level 6. */
#define Q9_TIMER_IRQ_LEVEL   6
#define Q9_TIMER_IRQ_VECTOR  30

struct Q9TimerIRQState {
    SysBusDevice parent_obj;
    MemoryRegion iomem;
    M68kCPU *cpu;
    QEMUTimer *tick_timer;
    QEMUTimer *deassert_timer;
    bool active;
};

static void q9_timer_irq_deassert(void *opaque)
{
    Q9TimerIRQState *s = opaque;

    m68k_set_irq_level(s->cpu, 0, 0);
}

static void q9_timer_irq_tick(void *opaque)
{
    Q9TimerIRQState *s = opaque;
    int64_t now = qemu_clock_get_ns(QEMU_CLOCK_VIRTUAL);

    if (s->active) {
        m68k_set_irq_level(s->cpu, Q9_TIMER_IRQ_LEVEL, Q9_TIMER_IRQ_VECTOR);
        timer_mod(s->deassert_timer, now + Q9_TIMER_IRQ_PULSE_NS);
    }
    timer_mod(s->tick_timer, now + Q9_TIMER_IRQ_PERIOD_NS);
}

static uint64_t q9_timer_irq_read(void *opaque, hwaddr addr, unsigned size)
{
    Q9TimerIRQState *s = opaque;
    int64_t now = qemu_clock_get_ns(QEMU_CLOCK_VIRTUAL);

    if (addr < Q9_TIMER_IRQ_ON_OFFSET) {
        s->active = false;
    } else {
        /* Fresh tick epoch: fire immediately, matching TI_IRQ_ON's
         * timer_synced=0 in the Musashi implementation (there, the very
         * first poll() after ON triggers right away instead of waiting
         * out a partial period). */
        s->active = true;
        m68k_set_irq_level(s->cpu, Q9_TIMER_IRQ_LEVEL, Q9_TIMER_IRQ_VECTOR);
        timer_mod(s->deassert_timer, now + Q9_TIMER_IRQ_PULSE_NS);
        timer_mod(s->tick_timer, now + Q9_TIMER_IRQ_PERIOD_NS);
    }
    return 0;
}

static void q9_timer_irq_write(void *opaque, hwaddr addr, uint64_t val,
                                unsigned size)
{
    (void)val;
    (void)q9_timer_irq_read(opaque, addr, size);
}

static const MemoryRegionOps q9_timer_irq_ops = {
    .read = q9_timer_irq_read,
    .write = q9_timer_irq_write,
    .valid.min_access_size = 1,
    .valid.max_access_size = 1,
    .endianness = DEVICE_BIG_ENDIAN,
};

static void q9_timer_irq_realize(DeviceState *dev, Error **errp)
{
    Q9TimerIRQState *s = Q9_TIMER_IRQ(dev);

    if (!s->cpu) {
        error_setg(errp, "q9-timer-irq: 'm68k-cpu' link property not set");
        return;
    }

    memory_region_init_io(&s->iomem, OBJECT(dev), &q9_timer_irq_ops, s,
                           TYPE_Q9_TIMER_IRQ, Q9_TIMER_IRQ_WINDOW_SIZE);
    sysbus_init_mmio(SYS_BUS_DEVICE(dev), &s->iomem);

    s->active = false;
    s->tick_timer = timer_new_ns(QEMU_CLOCK_VIRTUAL, q9_timer_irq_tick, s);
    s->deassert_timer = timer_new_ns(QEMU_CLOCK_VIRTUAL, q9_timer_irq_deassert, s);

    /* The periodic tick runs from power-on even while inactive -- cheap
     * (100 Hz, an idle flag check), and keeps TI_IRQ_OFF/_ON down to a
     * single flag flip each, matching the Musashi original's own
     * poll-every-round design (q9_board_poll_timer). */
    timer_mod(s->tick_timer,
              qemu_clock_get_ns(QEMU_CLOCK_VIRTUAL) + Q9_TIMER_IRQ_PERIOD_NS);
}

static const Property q9_timer_irq_properties[] = {
    DEFINE_PROP_LINK("m68k-cpu", Q9TimerIRQState, cpu, TYPE_M68K_CPU, M68kCPU *),
};

static void q9_timer_irq_class_init(ObjectClass *oc, const void *data)
{
    DeviceClass *dc = DEVICE_CLASS(oc);

    dc->desc = "Q9 board Timer/IRQ3 address-trigger (100 Hz, autovectored)";
    dc->realize = q9_timer_irq_realize;
    device_class_set_props(dc, q9_timer_irq_properties);
    set_bit(DEVICE_CATEGORY_MISC, dc->categories);
}

static const TypeInfo q9_timer_irq_info = {
    .name          = TYPE_Q9_TIMER_IRQ,
    .parent        = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(Q9TimerIRQState),
    .class_init    = q9_timer_irq_class_init,
};

static void q9_timer_irq_register_types(void)
{
    type_register_static(&q9_timer_irq_info);
}

type_init(q9_timer_irq_register_types)
