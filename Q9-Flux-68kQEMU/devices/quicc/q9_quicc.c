/*
 * Q9 board: QUICC Ethernet (MC68360 SCC1, 10Base-T).
 *
 * Ported from the Musashi-based implementation in
 * Q9-Flux-68k/src/devices/quicc/quicc.c -- the register/PRAM window,
 * CP command register, buffer-descriptor rings and the SDMA-style
 * frame transfer to/from guest RAM are ported 1:1 (same offsets,
 * verified against Motorola's MC68360 quicc.h like the original; same
 * write-1-to-clear SCCE/CISR semantics; same "only what the sp360
 * driver actually touches" scope, s. the original's own header
 * comment).
 *
 * What's deliberately NOT ported is the original's four hand-rolled
 * host network backends (nat/vmnet/bridge/slirp, ~500 of quicc.c's 872
 * lines) -- Musashi had no access to a real network stack and had to
 * build its own (down to a proxy-ARP/ICMP-echo mini-NAT). QEMU already
 * ships one: this device is a standard QEMU NIC frontend
 * (qemu_new_nic()/NetClientInfo, DEFINE_NIC_PROPERTIES for the usual
 * "netdev"/"mac" qdev properties), so the SAME command line that gives
 * any other QEMU NIC a network -- "-netdev user,id=net0" (full NAT,
 * DHCP, DNS, ARP/ICMP, strictly more complete than the original's
 * proxy-ARP hack), "-netdev tap,...", "-netdev socket,..." -- works
 * here too, wired up the usual way (s. q9board.c:
 * "-global q9-quicc.netdev=net0"). q_tx_run's entire ~100-line
 * per-backend q_backend_tx dispatch collapses to one
 * qemu_send_packet() call; RX becomes a NetClientInfo.receive()
 * callback instead of a per-backend poll function -- otherwise the
 * same q9_quicc_rx_frame() BD-ring-fill logic as the original.
 *
 * IRQ is level 5 with a FIXED vector (254, from the spqe0 descriptor in
 * the reference Q9 port) -- unlike DUART's driver-programmed IVR, no
 * irq_vector_fn equivalent is needed; m68k_set_irq_level() is called
 * with a constant vector whenever the CIPR&CIMR condition changes,
 * same held-until-condition-clears model as DUART (not pulsed, unlike
 * the un-acked autovectored timer).
 *
 * This file is not (yet) part of Q9-Flux's own MIT-licensed code base --
 * it lives inside the vendored QEMU source tree and is licensed under
 * QEMU's own terms (GPL), like the rest of this directory.
 */

#include "qemu/osdep.h"
#include "qapi/error.h"
#include "hw/core/sysbus.h"
#include "hw/core/qdev-properties.h"
#include "net/net.h"
#include "qom/object.h"
#include "target/m68k/cpu.h"
#include <string.h>

#define TYPE_Q9_QUICC "q9-quicc"
OBJECT_DECLARE_SIMPLE_TYPE(Q9QuiccState, Q9_QUICC)

/* Matches Q9_QUICC_BASE/_TOP/_MEM_LEN in
 * Q9-Flux-68k/src/devices/quicc/quicc.h. */
#define Q9_QUICC_WINDOW_SIZE 0x1800u

/* Register-/PRAM-Offsets, 1:1 aus quicc.c uebernommen (per offsetof aus
 * Motorolas quicc.h verifiziert). */
#define QO_PRAM_RBASE    0x0C00u
#define QO_PRAM_TBASE    0x0C02u
#define QO_PRAM_MRBLR    0x0C06u
#define QO_PRAM_RBPTR    0x0C10u
#define QO_PRAM_TBPTR    0x0C20u
#define QO_INTR_CIPR     0x1544u
#define QO_INTR_CIMR     0x1548u
#define QO_INTR_CISR     0x154Cu
#define QO_CP_CR         0x15C0u
#define QO_SCC1_GSMRA    0x1600u
#define QO_SCC1_TODR     0x160Cu
#define QO_SCC1_SCCE     0x1610u
#define QO_SCC1_SCCM     0x1614u

#define QC_CMD_FLAG      0x0001u
#define QC_CMD_OPMASK    0x0F00u
#define QC_INIT_RXTX     0x0000u
#define QC_GSMR_ENT      0x00000010u
#define QC_GSMR_ENR      0x00000020u
#define QC_INTR_SCC1     0x40000000u
#define QC_EV_BSY        0x0004u
#define QC_EV_TXB        0x0002u
#define QC_EV_RXF        0x0008u
#define QC_BD_TR_RE      0x8000u
#define QC_BD_WRAP       0x2000u
#define QC_BD_IRQ        0x1000u
#define QC_BD_LAST       0x0800u
#define QC_BD_RX_FIRST   0x0400u
#define QC_BD_SIZE       8u

#define QC_FRAME_MAX     1518u
#define QC_TX_RING_MAX   64u

#define Q9_QUICC_IRQ_LEVEL  5
#define Q9_QUICC_IRQ_VECTOR 254

struct Q9QuiccState {
    SysBusDevice parent_obj;
    MemoryRegion iomem;
    M68kCPU *cpu;

    uint8_t mem[Q9_QUICC_WINDOW_SIZE];
    uint8_t *ram;         /* guest RAM, s. q9_quicc_set_ram() below */
    uint32_t ram_len;

    NICState *nic;
    NICConf conf;

    bool irq_asserted;
};

/* devices/quicc/q9_quicc.c is created before q9board.c knows the RAM
 * MemoryRegion's host pointer/size -- set after realize, same rationale
 * (and same plain-function-not-qdev-property choice) as
 * q9_remap_set_targets()/q9_mc6845_get_stride(): this devices/ tree has
 * no shared headers yet. */
void q9_quicc_set_ram(DeviceState *dev, uint8_t *ram, uint32_t ram_len);

void q9_quicc_set_ram(DeviceState *dev, uint8_t *ram, uint32_t ram_len)
{
    Q9QuiccState *s = Q9_QUICC(dev);

    s->ram = ram;
    s->ram_len = ram_len;
}

static uint16_t q_rd16(const Q9QuiccState *s, uint32_t off)
{
    return (uint16_t)(((uint16_t)s->mem[off] << 8) | s->mem[off + 1]);
}

static void q_wr16(Q9QuiccState *s, uint32_t off, uint16_t val)
{
    s->mem[off] = (uint8_t)(val >> 8);
    s->mem[off + 1] = (uint8_t)val;
}

static uint32_t q_rd32(const Q9QuiccState *s, uint32_t off)
{
    return ((uint32_t)s->mem[off] << 24) | ((uint32_t)s->mem[off + 1] << 16) |
           ((uint32_t)s->mem[off + 2] << 8) | (uint32_t)s->mem[off + 3];
}

static void q_wr32(Q9QuiccState *s, uint32_t off, uint32_t val)
{
    s->mem[off] = (uint8_t)(val >> 24);
    s->mem[off + 1] = (uint8_t)(val >> 16);
    s->mem[off + 2] = (uint8_t)(val >> 8);
    s->mem[off + 3] = (uint8_t)val;
}

static void q_irq_update(Q9QuiccState *s)
{
    uint32_t cipr = q_rd32(s, QO_INTR_CIPR);
    bool pending;

    if ((q_rd16(s, QO_SCC1_SCCE) & q_rd16(s, QO_SCC1_SCCM)) != 0) {
        cipr |= QC_INTR_SCC1;
    } else {
        cipr &= ~QC_INTR_SCC1;
    }
    q_wr32(s, QO_INTR_CIPR, cipr);

    pending = (cipr & q_rd32(s, QO_INTR_CIMR) & QC_INTR_SCC1) != 0;
    if (pending == s->irq_asserted) {
        return;
    }
    s->irq_asserted = pending;
    m68k_set_irq_level(s->cpu, pending ? Q9_QUICC_IRQ_LEVEL : 0,
                        pending ? Q9_QUICC_IRQ_VECTOR : 0);
}

static void q_event(Q9QuiccState *s, uint16_t bits)
{
    q_wr16(s, QO_SCC1_SCCE, (uint16_t)(q_rd16(s, QO_SCC1_SCCE) | bits));
    q_irq_update(s);
}

static void q_cp_command(Q9QuiccState *s, uint16_t cmd)
{
    if ((cmd & QC_CMD_FLAG) != 0 && (cmd & 0x00C0u) == 0) {
        if ((cmd & QC_CMD_OPMASK) == QC_INIT_RXTX) {
            q_wr16(s, QO_PRAM_RBPTR, q_rd16(s, QO_PRAM_RBASE));
            q_wr16(s, QO_PRAM_TBPTR, q_rd16(s, QO_PRAM_TBASE));
        }
    }
    q_wr16(s, QO_CP_CR, (uint16_t)(cmd & ~QC_CMD_FLAG));
}

/* TX ring walk, s. Dateikopf: the original's ~100-line q_backend_tx
 * (four host-network backends + a hand-rolled ARP/ICMP mini-NAT)
 * collapses to a single qemu_send_packet() -- QEMU's own netdev layer
 * (matching "-netdev user/tap/socket/..." on the command line) does
 * everything those backends did, and more. */
static void q_tx_run(Q9QuiccState *s)
{
    uint32_t guard;

    if ((q_rd32(s, QO_SCC1_GSMRA) & QC_GSMR_ENT) == 0) {
        return;
    }

    for (guard = 0; guard < QC_TX_RING_MAX; guard++) {
        uint16_t tbptr = q_rd16(s, QO_PRAM_TBPTR);
        uint16_t status;
        uint16_t flen;
        uint32_t buf;

        if (tbptr + QC_BD_SIZE > Q9_QUICC_WINDOW_SIZE) {
            return;
        }
        status = q_rd16(s, tbptr);
        if ((status & QC_BD_TR_RE) == 0) {
            return;
        }
        flen = q_rd16(s, tbptr + 2u);
        buf = q_rd32(s, tbptr + 4u);

        if ((status & QC_BD_LAST) != 0 && flen >= 14u && flen <= QC_FRAME_MAX &&
            buf < s->ram_len && buf + flen <= s->ram_len) {
            qemu_send_packet(qemu_get_queue(s->nic), s->ram + buf, flen);
        }

        q_wr16(s, tbptr, (uint16_t)(status & ~QC_BD_TR_RE));
        q_event(s, QC_EV_TXB);

        q_wr16(s, QO_PRAM_TBPTR,
               (status & QC_BD_WRAP) ? q_rd16(s, QO_PRAM_TBASE)
                                      : (uint16_t)(tbptr + QC_BD_SIZE));
    }
}

/* RX-in: fills the next empty RX BD from an incoming frame -- 1:1 the
 * original's q9_quicc_rx_frame(), just called from a NetClientInfo
 * receive callback instead of a per-backend poll function. */
static void q_rx_frame(Q9QuiccState *s, const uint8_t *frame, uint32_t len)
{
    uint16_t rbptr, status, mrblr;
    uint32_t buf;

    if ((q_rd32(s, QO_SCC1_GSMRA) & QC_GSMR_ENR) == 0 || len < 14u ||
        len > QC_FRAME_MAX) {
        return;
    }

    rbptr = q_rd16(s, QO_PRAM_RBPTR);
    if (rbptr + QC_BD_SIZE > Q9_QUICC_WINDOW_SIZE) {
        return;
    }
    status = q_rd16(s, rbptr);
    if ((status & QC_BD_TR_RE) == 0) {
        q_event(s, QC_EV_BSY);
        return;
    }

    mrblr = q_rd16(s, QO_PRAM_MRBLR);
    buf = q_rd32(s, rbptr + 4u);
    if (len + 4u > mrblr || buf >= s->ram_len || buf + len > s->ram_len) {
        q_event(s, QC_EV_BSY);
        return;
    }

    memcpy(s->ram + buf, frame, len);
    q_wr16(s, rbptr + 2u, (uint16_t)(len + 4u));
    q_wr16(s, rbptr, (uint16_t)((status & (QC_BD_WRAP | QC_BD_IRQ)) |
                                 QC_BD_RX_FIRST | QC_BD_LAST));
    q_event(s, QC_EV_RXF);

    q_wr16(s, QO_PRAM_RBPTR,
           (status & QC_BD_WRAP) ? q_rd16(s, QO_PRAM_RBASE)
                                  : (uint16_t)(rbptr + QC_BD_SIZE));
}

static ssize_t q9_quicc_receive(NetClientState *nc, const uint8_t *buf,
                                 size_t size)
{
    Q9QuiccState *s = qemu_get_nic_opaque(nc);

    q_rx_frame(s, buf, (uint32_t)size);
    return (ssize_t)size;
}

static NetClientInfo net_q9_quicc_info = {
    .type = NET_CLIENT_DRIVER_NIC,
    .size = sizeof(NICState),
    .receive = q9_quicc_receive,
};

static uint64_t q9_quicc_read(void *opaque, hwaddr addr, unsigned size)
{
    Q9QuiccState *s = opaque;
    uint64_t val = 0;
    unsigned i;

    for (i = 0; i < size; i++) {
        uint32_t off = (uint32_t)addr + i;
        val = (val << 8) | (off < Q9_QUICC_WINDOW_SIZE ? s->mem[off] : 0);
    }
    return val;
}

static void q9_quicc_write(void *opaque, hwaddr addr, uint64_t val,
                            unsigned size)
{
    Q9QuiccState *s = opaque;
    uint32_t off = (uint32_t)addr;

    /* Wide accesses get their own path (no byte synthesis), same as the
     * original's q9_quicc_write16/32: several registers (CR, TODR,
     * SCCE, SCCM, CISR, GSMRA/CIMR) have side effects that only make
     * sense as a whole word/longword, never per-byte (the sp360 driver
     * never does byte writes to them either, s. the original's own
     * comment in q9_quicc_write8). */
    if (size == 1) {
        if (off < Q9_QUICC_WINDOW_SIZE) {
            s->mem[off] = (uint8_t)val;
        }
        return;
    }
    if (size == 2) {
        if (off + 1u >= Q9_QUICC_WINDOW_SIZE) {
            return;
        }
        switch (off) {
        case QO_CP_CR:
            q_cp_command(s, (uint16_t)val);
            return;
        case QO_SCC1_TODR:
            q_wr16(s, off, 0);
            if ((val & 0x8000u) != 0) {
                q_tx_run(s);
            }
            return;
        case QO_SCC1_SCCE:
            q_wr16(s, off, (uint16_t)(q_rd16(s, off) & ~val));
            q_irq_update(s);
            return;
        case QO_SCC1_SCCM:
            q_wr16(s, off, (uint16_t)val);
            q_irq_update(s);
            return;
        default:
            q_wr16(s, off, (uint16_t)val);
            return;
        }
    }
    /* size == 4 */
    if (off + 3u >= Q9_QUICC_WINDOW_SIZE) {
        return;
    }
    if (off == QO_INTR_CISR) {
        q_wr32(s, off, q_rd32(s, off) & ~(uint32_t)val);
        return;
    }
    q_wr32(s, off, (uint32_t)val);
    if (off == QO_SCC1_GSMRA || off == QO_INTR_CIMR) {
        q_irq_update(s);
    }
}

static const MemoryRegionOps q9_quicc_ops = {
    .read = q9_quicc_read,
    .write = q9_quicc_write,
    .valid.min_access_size = 1,
    .valid.max_access_size = 4,
    .impl.min_access_size = 1,
    .impl.max_access_size = 4,
    .endianness = DEVICE_BIG_ENDIAN,
};

static void q9_quicc_realize(DeviceState *dev, Error **errp)
{
    Q9QuiccState *s = Q9_QUICC(dev);

    if (!s->cpu) {
        error_setg(errp, "q9-quicc: 'm68k-cpu' link property not set");
        return;
    }

    memory_region_init_io(&s->iomem, OBJECT(dev), &q9_quicc_ops, s,
                           TYPE_Q9_QUICC, Q9_QUICC_WINDOW_SIZE);
    sysbus_init_mmio(SYS_BUS_DEVICE(dev), &s->iomem);

    qemu_macaddr_default_if_unset(&s->conf.macaddr);
    s->nic = qemu_new_nic(&net_q9_quicc_info, &s->conf,
                           object_get_typename(OBJECT(dev)), dev->id,
                           &dev->mem_reentrancy_guard, s);
    qemu_format_nic_info_str(qemu_get_queue(s->nic), s->conf.macaddr.a);
}

static const Property q9_quicc_properties[] = {
    DEFINE_PROP_LINK("m68k-cpu", Q9QuiccState, cpu, TYPE_M68K_CPU, M68kCPU *),
    DEFINE_NIC_PROPERTIES(Q9QuiccState, conf),
};

static void q9_quicc_class_init(ObjectClass *oc, const void *data)
{
    DeviceClass *dc = DEVICE_CLASS(oc);

    dc->desc = "Q9 board QUICC Ethernet (MC68360 SCC1, 10Base-T)";
    dc->realize = q9_quicc_realize;
    device_class_set_props(dc, q9_quicc_properties);
    set_bit(DEVICE_CATEGORY_NETWORK, dc->categories);
}

static const TypeInfo q9_quicc_info = {
    .name          = TYPE_Q9_QUICC,
    .parent        = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(Q9QuiccState),
    .class_init    = q9_quicc_class_init,
};

static void q9_quicc_register_types(void)
{
    type_register_static(&q9_quicc_info);
}

type_init(q9_quicc_register_types)
