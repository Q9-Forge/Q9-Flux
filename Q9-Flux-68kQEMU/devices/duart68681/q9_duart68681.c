/*
 * Q9 board: 68681-DUART (console, channel A only).
 *
 * Matches the Musashi original's deliberately partial scope (s.
 * Q9-Flux-68k/src/devices/duart68681/duart68681.h): channel A is the
 * console (THRA -> host chardev, RHRA <- host chardev via an RX FIFO);
 * channel B is unconnected (sends into the void, never receives -- SRB
 * always reads back "ready to send", THRB writes are discarded). Only
 * MRA/SRA/THRA/RHRA/IMR/ISR/IVR and the channel-B stub registers are
 * wired up; the rest of the register map reads 0 / discards writes,
 * exactly like the original.
 *
 * IRQ handling differs structurally from the Musashi port, for the
 * opposite reason timer_irq's did: the 68681 is genuinely *vectored*
 * (the driver programs IVR at runtime and the interrupt is level-held
 * until the condition that raised it clears), which maps directly onto
 * QEMU's model -- m68k_set_irq_level(cpu, level, vector) is simply
 * called again whenever TxRDY/RxRDY-with-its-IMR-bit-enabled changes,
 * no pulsing needed (contrast timer_irq.c's autovectored, un-acked,
 * pulsed level 6).
 *
 * RX arrival also differs in mechanism, not behaviour: the Musashi
 * original polls the host terminal on every guest register access
 * (board_uart_poll_rx, so a large 4 MiB software FIFO absorbs bursts
 * between polls). QEMU's chardev backend delivers input via its own
 * can_receive/receive callbacks as it arrives, with can_receive already
 * providing real flow control -- so a small, realistic FIFO is enough
 * here; nothing needs to be polled from the read path.
 *
 * Ported from the Musashi-based implementation in
 * Q9-Flux-68k/src/devices/duart68681/duart68681.c -- register map and
 * externally visible semantics are unchanged; only the QOM/MemoryRegion/
 * chardev/IRQ plumbing is new. Structurally modelled on QEMU's own
 * hw/char/mcf_uart.c (ColdFire UART), the closest existing m68k-board
 * chardev-backed UART in this tree.
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

#define TYPE_Q9_DUART68681 "q9-duart68681"
OBJECT_DECLARE_SIMPLE_TYPE(Q9Duart68681State, Q9_DUART68681)

/* Matches Q9_BOARD_UART_* register offsets in
 * Q9-Flux-68k/src/kernel/q9board.h (relative to Q9_BOARD_UART_BASE). */
#define Q9_DUART_REG_MRA   0x00
#define Q9_DUART_REG_SRA   0x02
#define Q9_DUART_REG_CRA   0x04
#define Q9_DUART_REG_RHRA  0x06   /* = THRA on write */
#define Q9_DUART_REG_IMR   0x0A   /* write; ISR on read */
#define Q9_DUART_REG_ISR   0x0A
#define Q9_DUART_REG_MRB   0x10
#define Q9_DUART_REG_SRB   0x12
#define Q9_DUART_REG_CRB   0x14
#define Q9_DUART_REG_THRB  0x16
#define Q9_DUART_REG_IVR   0x18
#define Q9_DUART_WINDOW_SIZE 0x1C

/* Real 68681 RX FIFOs are 3 bytes deep; QEMU's chardev can_receive
 * already throttles the host side, so there is no need for the
 * Musashi original's oversized (4 MiB) software buffer -- a modest
 * FIFO here is both realistic and sufficient. */
#define Q9_DUART_RX_FIFO_SIZE 16

#define Q9_DUART_IRQ_LEVEL 3

struct Q9Duart68681State {
    SysBusDevice parent_obj;
    MemoryRegion iomem;
    M68kCPU *cpu;
    CharFrontend chr;

    uint8_t mr_a[2];
    int     mr_ptr_a;
    uint8_t mr_b[2];
    int     mr_ptr_b;
    uint8_t imr;
    uint8_t ivr;

    uint8_t  rx_fifo[Q9_DUART_RX_FIFO_SIZE];
    unsigned rx_head, rx_tail, rx_count;

    bool irq_asserted;
};

static bool q9_duart_irq_condition(Q9Duart68681State *s)
{
    /* TxRDYA (0x01): always ready -- TX completes synchronously below,
     * matching mcf_uart's own simplifying assumption (a real driver
     * blocking write has no meaningful "not ready" state to model here).
     * RxRDYA (0x02): only while the enabled IMR bit and the FIFO agree,
     * same condition as SRA/ISR's RxRDY bit below. */
    if ((s->imr & 0x01u) != 0) {
        return true;
    }
    if ((s->imr & 0x02u) != 0 && s->rx_count != 0) {
        return true;
    }
    return false;
}

static void q9_duart_update(Q9Duart68681State *s)
{
    bool pending = q9_duart_irq_condition(s);

    if (pending == s->irq_asserted) {
        return;
    }
    s->irq_asserted = pending;
    m68k_set_irq_level(s->cpu, pending ? Q9_DUART_IRQ_LEVEL : 0,
                        pending ? s->ivr : 0);
}

static uint8_t q9_duart_rx_pop(Q9Duart68681State *s)
{
    uint8_t c;

    if (s->rx_count == 0) {
        return 0;
    }
    c = s->rx_fifo[s->rx_tail];
    s->rx_tail = (s->rx_tail + 1u) % Q9_DUART_RX_FIFO_SIZE;
    s->rx_count--;
    return c;
}

static uint64_t q9_duart_read(void *opaque, hwaddr addr, unsigned size)
{
    Q9Duart68681State *s = opaque;
    uint64_t ret = 0;

    switch (addr) {
    case Q9_DUART_REG_MRA:
        ret = s->mr_a[s->mr_ptr_a];
        s->mr_ptr_a = 1;
        break;
    case Q9_DUART_REG_SRA:
        ret = (s->rx_count != 0 ? 0x01u : 0u) | 0x04u /* TxRDY */
              | 0x08u /* TxEMT */;
        break;
    case Q9_DUART_REG_RHRA:
        ret = q9_duart_rx_pop(s);
        qemu_chr_fe_accept_input(&s->chr);
        break;
    case Q9_DUART_REG_ISR:
        ret = 0x11u | (s->rx_count != 0 ? 0x02u : 0u);
        break;
    case Q9_DUART_REG_MRB:
        ret = s->mr_b[s->mr_ptr_b];
        s->mr_ptr_b = 1;
        break;
    case Q9_DUART_REG_SRB:
        ret = 0x0Cu;             /* sends immediately, never receives */
        break;
    case Q9_DUART_REG_IVR:
        ret = s->ivr;
        break;
    default:
        ret = 0;                 /* rest of the register set: accepted, reads 0 */
        break;
    }
    q9_duart_update(s);
    return ret;
}

static void q9_duart_write(void *opaque, hwaddr addr, uint64_t val,
                            unsigned size)
{
    Q9Duart68681State *s = opaque;
    uint8_t v = (uint8_t)val;

    switch (addr) {
    case Q9_DUART_REG_MRA:
        s->mr_a[s->mr_ptr_a] = v;
        s->mr_ptr_a = 1;
        break;
    case Q9_DUART_REG_CRA:
        if (((v >> 4) & 0x07u) == 1u) {   /* "reset MR pointer" command */
            s->mr_ptr_a = 0;
        }
        break;
    case Q9_DUART_REG_RHRA:                /* = THRA on write */
        qemu_chr_fe_write_all(&s->chr, &v, 1);
        break;
    case Q9_DUART_REG_MRB:
        s->mr_b[s->mr_ptr_b] = v;
        s->mr_ptr_b = 1;
        break;
    case Q9_DUART_REG_CRB:
        if (((v >> 4) & 0x07u) == 1u) {
            s->mr_ptr_b = 0;
        }
        break;
    case Q9_DUART_REG_IMR:
        s->imr = v;
        break;
    case Q9_DUART_REG_THRB:
        break;                              /* channel B unconnected: discard */
    case Q9_DUART_REG_IVR:
        s->ivr = v;
        break;
    default:
        break;                              /* rest of the register set: accepted, discarded */
    }
    q9_duart_update(s);
}

static const MemoryRegionOps q9_duart68681_ops = {
    .read = q9_duart_read,
    .write = q9_duart_write,
    .valid.min_access_size = 1,
    .valid.max_access_size = 1,
    .endianness = DEVICE_BIG_ENDIAN,
};

static int q9_duart_can_receive(void *opaque)
{
    Q9Duart68681State *s = opaque;

    return (int)(Q9_DUART_RX_FIFO_SIZE - s->rx_count);
}

static void q9_duart_receive(void *opaque, const uint8_t *buf, int size)
{
    Q9Duart68681State *s = opaque;
    int i;

    for (i = 0; i < size && s->rx_count < Q9_DUART_RX_FIFO_SIZE; i++) {
        s->rx_fifo[s->rx_head] = buf[i];
        s->rx_head = (s->rx_head + 1u) % Q9_DUART_RX_FIFO_SIZE;
        s->rx_count++;
    }
    q9_duart_update(s);
}

static void q9_duart_event(void *opaque, QEMUChrEvent event)
{
    /* No break/other special-event handling -- the Musashi original has
     * none either (board_uart_poll_rx only ever pulls plain bytes). */
    (void)opaque;
    (void)event;
}

static void q9_duart68681_realize(DeviceState *dev, Error **errp)
{
    Q9Duart68681State *s = Q9_DUART68681(dev);

    if (!s->cpu) {
        error_setg(errp, "q9-duart68681: 'm68k-cpu' link property not set");
        return;
    }

    memory_region_init_io(&s->iomem, OBJECT(dev), &q9_duart68681_ops, s,
                           TYPE_Q9_DUART68681, Q9_DUART_WINDOW_SIZE);
    sysbus_init_mmio(SYS_BUS_DEVICE(dev), &s->iomem);

    qemu_chr_fe_set_handlers(&s->chr, q9_duart_can_receive, q9_duart_receive,
                              q9_duart_event, NULL, s, NULL, true);
}

static const Property q9_duart68681_properties[] = {
    DEFINE_PROP_LINK("m68k-cpu", Q9Duart68681State, cpu, TYPE_M68K_CPU, M68kCPU *),
    DEFINE_PROP_CHR("chardev", Q9Duart68681State, chr),
};

static void q9_duart68681_class_init(ObjectClass *oc, const void *data)
{
    DeviceClass *dc = DEVICE_CLASS(oc);

    dc->desc = "Q9 board 68681 DUART (console, channel A only)";
    dc->realize = q9_duart68681_realize;
    device_class_set_props(dc, q9_duart68681_properties);
    set_bit(DEVICE_CATEGORY_INPUT, dc->categories);
}

static const TypeInfo q9_duart68681_info = {
    .name          = TYPE_Q9_DUART68681,
    .parent        = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(Q9Duart68681State),
    .class_init    = q9_duart68681_class_init,
};

static void q9_duart68681_register_types(void)
{
    type_register_static(&q9_duart68681_info);
}

type_init(q9_duart68681_register_types)
