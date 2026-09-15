/*
 * Q9 board: network terminals (8 virtual serial channels /x1../x8).
 *
 * Ported from the Musashi-based implementation in
 * Q9-Flux-68k/src/devices/nettty/nettty.c -- register layout and
 * semantics unchanged: one 256-byte I/O block per channel (+0 status,
 * bit0 RX ready/bit1 TX empty; +2 RX data, reading it clears RX ready;
 * +4 TX data), all 8 blocks contiguous ($FFFF1000-$FFFF17FF, channel
 * index = (offset >> 8), matching Q9_BOARD_NET_X1..X8_BASE's 0x100
 * spacing) so this is one QOM device internally dispatching by offset,
 * not eight separate ones. Level 4 IRQ, shared across all 8 channels:
 * whenever ANY channel has RX ready, the line is asserted with the
 * FIXED vector (70..77, s. Q9_NETTTY_IRQ_VECTOR_BASE) of the FIRST such
 * channel in index order -- 1:1 the original's network_irq_resync(),
 * just re-run after every RX arrival and every RX-clearing read instead
 * of via a poll-loop resync call.
 *
 * Two things are deliberately NOT ported, both because QEMU's own
 * chardev layer already does them better than the original's hand-
 * rolled raw-socket code could:
 *
 * - Telnet option negotiation (the original's own IAC/WILL/WONT/DO/
 *   DONT/SB state machine, telnet_filter_byte()) -- QEMU's socket
 *   chardev has full RFC854 negotiation built in ("telnet=on"): bytes
 *   reaching this device's receive callback are already clean
 *   application data, no filtering needed here at all.
 * - The original's single dynamic listen port (2000) that hands an
 *   incoming connection to whichever of the 8 channels is free --
 *   each channel here is instead its own independent qdev "chardevN"
 *   property (N=0..7), attached the usual QEMU way, e.g.
 *   "-chardev socket,id=x1,port=2001,server=on,wait=off,telnet=on
 *    -global q9-nettty.chardev0=x1" (repeat per channel/port). Simpler
 *   to reason about (a fixed port per line, like a real multi-port
 *   serial concentrator) at the cost of the original's single-port
 *   convenience; also drops the original's host-side Ctrl-Q
 *   self-disconnect affordance (no general "hang up this one chardev"
 *   hook exists on the qemu_chr_fe API) -- use the telnet client's own
 *   controls to disconnect instead.
 *
 * What IS ported: the CR-then-LF suppression (OS-9 wants a bare CR as
 * line end, not a telnet client's CR+LF), since that's genuine
 * Q9/OS-9-side protocol behaviour, not generic telnet plumbing.
 *
 * This file is not (yet) part of Q9-Flux's own MIT-licensed code base --
 * it lives inside the vendored QEMU source tree and is licensed under
 * QEMU's own terms (GPL), like the rest of this directory.
 */

#include "qemu/osdep.h"
#include "qapi/error.h"
#include "hw/core/sysbus.h"
#include "hw/core/qdev-properties.h"
#include "hw/core/qdev-properties-system.h"
#include "chardev/char-fe.h"
#include "qom/object.h"
#include "target/m68k/cpu.h"

#define TYPE_Q9_NETTTY "q9-nettty"
OBJECT_DECLARE_SIMPLE_TYPE(Q9NetttyState, Q9_NETTTY)

#define Q9_NETTTY_CHANNELS     8u
#define Q9_NETTTY_CHAN_SPACING 0x100u
#define Q9_NETTTY_WINDOW_SIZE  (Q9_NETTTY_CHANNELS * Q9_NETTTY_CHAN_SPACING)

#define Q9_NETTTY_IRQ_LEVEL       4
#define Q9_NETTTY_IRQ_VECTOR_BASE 70   /* channel i -> vector 70+i, s. Dateikopf */

typedef struct Q9NetttyState Q9NetttyState;

typedef struct {
    Q9NetttyState *parent;
    int idx;
    CharFrontend chr;
    uint8_t rx_data;
    uint8_t status;      /* bit0 RX ready, bit1 TX empty */
    bool last_was_cr;
} Q9NetttyChannel;

struct Q9NetttyState {
    SysBusDevice parent_obj;
    MemoryRegion iomem;
    M68kCPU *cpu;

    Q9NetttyChannel ch[Q9_NETTTY_CHANNELS];
    int irq_vector_asserted;   /* 0 = line down, else the asserted vector */
};

/* 1:1 the original's network_irq_resync(): the shared level-4 line is
 * the OR of all 8 RX-ready bits, vectored to the FIRST channel (index
 * order) that has one -- called after every RX arrival and after every
 * RX-clearing read, same two call sites as the original. */
static void q9_nettty_irq_resync(Q9NetttyState *s)
{
    unsigned i;
    int vector = 0;

    for (i = 0; i < Q9_NETTTY_CHANNELS; i++) {
        if (s->ch[i].status & 0x01u) {
            vector = Q9_NETTTY_IRQ_VECTOR_BASE + (int)i;
            break;
        }
    }
    if (vector == s->irq_vector_asserted) {
        return;
    }
    s->irq_vector_asserted = vector;
    m68k_set_irq_level(s->cpu, vector ? Q9_NETTTY_IRQ_LEVEL : 0, (uint8_t)vector);
}

static uint64_t q9_nettty_read(void *opaque, hwaddr addr, unsigned size)
{
    Q9NetttyState *s = opaque;
    unsigned idx = (unsigned)(addr >> 8) & (Q9_NETTTY_CHANNELS - 1);
    uint32_t off = (uint32_t)addr & 0xFFu;
    Q9NetttyChannel *ch = &s->ch[idx];

    if (off == 0) {
        return ch->status;
    }
    if (off == 2) {
        ch->status &= ~0x01u;
        q9_nettty_irq_resync(s);
        return ch->rx_data;
    }
    return 0;
}

static void q9_nettty_write(void *opaque, hwaddr addr, uint64_t val,
                             unsigned size)
{
    Q9NetttyState *s = opaque;
    unsigned idx = (unsigned)(addr >> 8) & (Q9_NETTTY_CHANNELS - 1);
    uint32_t off = (uint32_t)addr & 0xFFu;
    Q9NetttyChannel *ch = &s->ch[idx];

    if (off == 4) {
        uint8_t v = (uint8_t)val;
        qemu_chr_fe_write_all(&ch->chr, &v, 1);
        ch->status |= 0x02u;
    }
}

static const MemoryRegionOps q9_nettty_ops = {
    .read = q9_nettty_read,
    .write = q9_nettty_write,
    .valid.min_access_size = 1,
    .valid.max_access_size = 1,
    .endianness = DEVICE_BIG_ENDIAN,
};

static int q9_nettty_can_receive(void *opaque)
{
    Q9NetttyChannel *ch = opaque;

    /* Backpressure while the 1-byte RX latch is full, same as the
     * original's "don't consume while status&1" -- bytes queue up in
     * the chardev backend instead of being dropped. */
    return (ch->status & 0x01u) ? 0 : 1;
}

static void q9_nettty_receive(void *opaque, const uint8_t *buf, int size)
{
    Q9NetttyChannel *ch = opaque;
    int i;

    for (i = 0; i < size; i++) {
        uint8_t byte_in = buf[i];

        if (byte_in == '\n' && ch->last_was_cr) {
            ch->last_was_cr = false;
            continue;
        }
        ch->last_was_cr = (byte_in == '\r');
        ch->rx_data = byte_in;
        ch->status |= 0x01u;
        q9_nettty_irq_resync(ch->parent);
        /* can_receive() now reports 0 until this byte is read, so any
         * further bytes in this same call stay queued in the chardev
         * backend -- matches the original's one-byte-per-poll latch. */
        break;
    }
}

static void q9_nettty_event(void *opaque, QEMUChrEvent event)
{
    (void)opaque;
    (void)event;
}

static void q9_nettty_realize(DeviceState *dev, Error **errp)
{
    Q9NetttyState *s = Q9_NETTTY(dev);
    unsigned i;

    if (!s->cpu) {
        error_setg(errp, "q9-nettty: 'm68k-cpu' link property not set");
        return;
    }

    memory_region_init_io(&s->iomem, OBJECT(dev), &q9_nettty_ops, s,
                           TYPE_Q9_NETTTY, Q9_NETTTY_WINDOW_SIZE);
    sysbus_init_mmio(SYS_BUS_DEVICE(dev), &s->iomem);

    for (i = 0; i < Q9_NETTTY_CHANNELS; i++) {
        s->ch[i].parent = s;
        s->ch[i].idx = (int)i;
        s->ch[i].status = 0x02u;   /* TX empty, matching the original's channel init */
        qemu_chr_fe_set_handlers(&s->ch[i].chr, q9_nettty_can_receive,
                                  q9_nettty_receive, q9_nettty_event, NULL,
                                  &s->ch[i], NULL, true);
    }
}

static const Property q9_nettty_properties[] = {
    DEFINE_PROP_LINK("m68k-cpu", Q9NetttyState, cpu, TYPE_M68K_CPU, M68kCPU *),
    DEFINE_PROP_CHR("chardev0", Q9NetttyState, ch[0].chr),
    DEFINE_PROP_CHR("chardev1", Q9NetttyState, ch[1].chr),
    DEFINE_PROP_CHR("chardev2", Q9NetttyState, ch[2].chr),
    DEFINE_PROP_CHR("chardev3", Q9NetttyState, ch[3].chr),
    DEFINE_PROP_CHR("chardev4", Q9NetttyState, ch[4].chr),
    DEFINE_PROP_CHR("chardev5", Q9NetttyState, ch[5].chr),
    DEFINE_PROP_CHR("chardev6", Q9NetttyState, ch[6].chr),
    DEFINE_PROP_CHR("chardev7", Q9NetttyState, ch[7].chr),
};

static void q9_nettty_class_init(ObjectClass *oc, const void *data)
{
    DeviceClass *dc = DEVICE_CLASS(oc);

    dc->desc = "Q9 board network terminals (8 channels, /x1../x8)";
    dc->realize = q9_nettty_realize;
    device_class_set_props(dc, q9_nettty_properties);
    set_bit(DEVICE_CATEGORY_INPUT, dc->categories);
}

static const TypeInfo q9_nettty_info = {
    .name          = TYPE_Q9_NETTTY,
    .parent        = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(Q9NetttyState),
    .class_init    = q9_nettty_class_init,
};

static void q9_nettty_register_types(void)
{
    type_register_static(&q9_nettty_info);
}

type_init(q9_nettty_register_types)
