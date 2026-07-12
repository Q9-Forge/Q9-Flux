//════════════════════════════════════════════════════════════════════════════════════════════════
// File:   quicc.c                                                                         Ver. 1.00
// Owner:  AF
// Desc.:  Implementierung der QUICC-Ethernet-Emulation, siehe quicc.h. Verhaltens-Referenz ist
//         ausschliesslich der sp360-Treiber (MWOS SRC/DPIO/SPF/DRVR/SPQUICC, init.c/isr.c):
//         emuliert wird genau das Teilverhalten des MC68360, das dieser Treiber benutzt.
//
//         Ablauf aus Treibersicht:
//           init_hdw():  PRAM fuellen (rbase/tbase/mrblr/MAC/...), RX-BDs mit mbuf-Adressen
//                        bestuecken (R_E|R_I), CR = INIT_RXTX_PARAMS|FLG (Emulator: rbptr:=rbase,
//                        tbptr:=tbase, FLG sofort loeschen), CIMR |= INTR_SCC1, SCCE alles
//                        loeschen, SCCM = TXB|RXB|RXF|BSY|TXE, GSMRA zuletzt ENR|ENT setzen.
//           qe_xmit():   TX-BD fuellen (buf/laenge, T_R|T_PAD|T_L|T_C), TODR = $8000 →
//                        Emulator laeuft den TX-Ring ab tbptr ab, uebergibt jeden fertigen
//                        Frame ans Backend, loescht T_R, setzt SCCE.TXB, rueckt tbptr weiter.
//           Empfang:     Backend legt Frame in den naechsten leeren RX-BD (ab rbptr): Daten in
//                        den mbuf im Gast-RAM, Laenge = Framelaenge+4 (Treiber zieht die 4
//                        CRC-Bytes wieder ab: mb->m_size = length-4), R_E loeschen, F|L setzen,
//                        SCCE.RXF setzen → IRQ Level 5, IACK-Vektor 254.
//           isr():       prueft CIPR&INTR_SCC1, quittiert per CISR, liest SCCE, schreibt die
//                        behandelten Bits zurueck (write-1-to-clear).
//
//         Host-Backend (User-Mode-Mini-NAT, kein Root/TAP): der Emulator IST die Gegenstelle
//         10.0.0.2 (Q9_QUICC_HOST_IP) — beantwortet jede ARP-Anfrage mit seiner MAC und
//         ICMP-Echo-Requests an 10.0.0.2 mit einem Echo-Reply. TCP/UDP-NAT: Ausbaustufe.
//
// Edition History
//─────────┬──────┬────────────────────────────────────────────────────────────────────────┬──────
// Date    │ Ver. │ Description                                                            │ By
//─────────┼──────┼────────────────────────────────────────────────────────────────────────┼──────
// 26-07-12│ 1.00 │ 5.11: Erster Wurf — Registerfenster, BD-Ringe, IRQ, ARP/ICMP-Backend    │ CF
//═════════╧══════╧════════════════════════════════════════════════════════════════════════╧══════
#include "quicc.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//─── Register-/PRAM-Offsets (per offsetof aus Motorolas quicc.h verifiziert, 2026-07-12) ─────────
#define QO_PRAM_RBASE    0x0C00u                      /* u16: RX-BD-Ringanfang (DPRAM-Offset)     */
#define QO_PRAM_TBASE    0x0C02u                      /* u16: TX-BD-Ringanfang                    */
#define QO_PRAM_MRBLR    0x0C06u                      /* u16: max. RX-Pufferlaenge                */
#define QO_PRAM_RBPTR    0x0C10u                      /* u16: naechster RX-BD (CP schreibt fort)  */
#define QO_PRAM_TBPTR    0x0C20u                      /* u16: naechster TX-BD (CP schreibt fort)  */
#define QO_PRAM_MFLR     0x0C4Au                      /* u16: max. Framelaenge (1518)             */
#define QO_INTR_CIPR     0x1544u                      /* u32: CPM Interrupt pending               */
#define QO_INTR_CIMR     0x1548u                      /* u32: CPM Interrupt mask                  */
#define QO_INTR_CISR     0x154Cu                      /* u32: CPM Interrupt in-service (W1C)      */
#define QO_CP_CR         0x15C0u                      /* u16: CP-Kommandoregister                 */
#define QO_SCC1_GSMRA    0x1600u                      /* u32: general mode low (ENR/ENT)          */
#define QO_SCC1_TODR     0x160Cu                      /* u16: transmit on demand ($8000 = kick)   */
#define QO_SCC1_SCCE     0x1610u                      /* u16: Ereignisregister (W1C)              */
#define QO_SCC1_SCCM     0x1614u                      /* u16: Ereignismaske                       */

//─── Konstanten aus dem 68360/Treiber (regs360.h/enet360.h/quicc.h des MWOS-SDK) ─────────────────
#define QC_CMD_FLAG      0x0001u                      /* CR: Kommando ausfuehren (CP loescht es)  */
#define QC_CMD_OPMASK    0x0F00u                      /* CR: Opcode-Bits                          */
#define QC_INIT_RXTX     0x0000u                      /* CR: INIT RX & TX PARAMS                  */
#define QC_GSMR_ENT      0x00000010u                  /* GSMRA: Transmitter enable                */
#define QC_GSMR_ENR      0x00000020u                  /* GSMRA: Receiver enable                   */
#define QC_INTR_SCC1     0x40000000u                  /* CIPR/CIMR/CISR: SCC1-Bit                 */
#define QC_EV_TXE        0x0010u                      /* SCCE: transmit error                     */
#define QC_EV_RXF        0x0008u                      /* SCCE: receive frame                      */
#define QC_EV_BSY        0x0004u                      /* SCCE: busy (kein leerer RX-BD)           */
#define QC_EV_TXB        0x0002u                      /* SCCE: transmit buffer                    */
#define QC_BD_TR_RE      0x8000u                      /* BD: TX ready / RX empty                  */
#define QC_BD_WRAP       0x2000u                      /* BD: Ring-Ende, zurueck zum Anfang        */
#define QC_BD_IRQ        0x1000u                      /* BD: Interrupt nach Abschluss             */
#define QC_BD_LAST       0x0800u                      /* BD: letzter Teil des Frames (T_L/R_L)    */
#define QC_BD_RX_FIRST   0x0400u                      /* RX-BD: erster Teil des Frames (R_F)      */
#define QC_BD_SIZE       8u                           /* status(2) + laenge(2) + puffer(4)        */

#define QC_FRAME_MAX     1518u                        /* max. Ethernet-Frame (inkl. Header)       */
#define QC_TX_RING_MAX   64u                          /* Schutz gegen kaputte Ringe               */

//─── Mini-NAT-Gegenstelle (muss zu interfaces.conf im MWOS-Q9-Port passen: Gast = 10.0.0.1) ──────
#define QH_IP0 10
#define QH_IP1 0
#define QH_IP2 0
#define QH_IP3 2                                      /* Host-Gegenstelle: 10.0.0.2               */
static const uint8_t qh_mac[6] = { 0x02, 0x51, 0x39, 0x00, 0x00, 0x02 };   /* lokal verwaltet    */

static int qd_debug = -1;                             /* Q9_QUICC_DEBUG=1: Frame-Trace auf stderr */

//────────────────────────────────────────────────────────────────────────────────────────────────
// Function: qd_trace
// Desc.:    Debug-Ausgabe, nur wenn Q9_QUICC_DEBUG gesetzt ist (einmalig gecacht).
//────────────────────────────────────────────────────────────────────────────────────────────────
static int qd_on(void)
{
    if (qd_debug < 0) {
        qd_debug = getenv("Q9_QUICC_DEBUG") != 0;
    }
    return qd_debug;
}

//────────────────────────────────────────────────────────────────────────────────────────────────
// Function: q_rd16/q_wr16/q_rd32/q_wr32
// Desc.:    Big-Endian-Zugriffe auf das Fensterabbild (Offsets relativ zur QUICC-Basis).
//────────────────────────────────────────────────────────────────────────────────────────────────
static uint16_t q_rd16(const q9_quicc_t *q, uint32_t off)
{
    return (uint16_t)(((uint16_t)q->mem[off] << 8) | q->mem[off + 1]);
}

static void q_wr16(q9_quicc_t *q, uint32_t off, uint16_t val)
{
    q->mem[off]     = (uint8_t)(val >> 8);
    q->mem[off + 1] = (uint8_t)val;
}

static uint32_t q_rd32(const q9_quicc_t *q, uint32_t off)
{
    return ((uint32_t)q->mem[off]     << 24) | ((uint32_t)q->mem[off + 1] << 16) |
           ((uint32_t)q->mem[off + 2] <<  8) |  (uint32_t)q->mem[off + 3];
}

static void q_wr32(q9_quicc_t *q, uint32_t off, uint32_t val)
{
    q->mem[off]     = (uint8_t)(val >> 24);
    q->mem[off + 1] = (uint8_t)(val >> 16);
    q->mem[off + 2] = (uint8_t)(val >>  8);
    q->mem[off + 3] = (uint8_t)val;
}

//────────────────────────────────────────────────────────────────────────────────────────────────
// Function: q_irq_update
// Desc.:    CIPR aus SCCE/SCCM ableiten: steht ein unmaskiertes SCC1-Ereignis an, geht das
//           SCC1-Bit im CIPR hoch (Pegel, nicht Flanke) — sonst wieder runter. Ob daraus ein
//           CPU-Interrupt wird, entscheidet q9_quicc_irq_pending (CIMR) im Runner-Poll.
//────────────────────────────────────────────────────────────────────────────────────────────────
static void q_irq_update(q9_quicc_t *q)
{
    uint32_t cipr = q_rd32(q, QO_INTR_CIPR);

    if ((q_rd16(q, QO_SCC1_SCCE) & q_rd16(q, QO_SCC1_SCCM)) != 0) {
        cipr |= QC_INTR_SCC1;
    } else {
        cipr &= ~QC_INTR_SCC1;
    }
    q_wr32(q, QO_INTR_CIPR, cipr);
}

//────────────────────────────────────────────────────────────────────────────────────────────────
// Function: q_event
// Desc.:    SCC1-Ereignisbit(s) setzen und den Interrupt-Pegel nachziehen.
//────────────────────────────────────────────────────────────────────────────────────────────────
static void q_event(q9_quicc_t *q, uint16_t bits)
{
    q_wr16(q, QO_SCC1_SCCE, (uint16_t)(q_rd16(q, QO_SCC1_SCCE) | bits));
    q_irq_update(q);
}

//────────────────────────────────────────────────────────────────────────────────────────────────
// Mini-NAT-Backend: ARP + ICMP-Echo als Gegenstelle 10.0.0.2
//────────────────────────────────────────────────────────────────────────────────────────────────

//────────────────────────────────────────────────────────────────────────────────────────────────
// Function: q_ipsum
// Desc.:    16-Bit-Einerkomplement-Pruefsumme (IP/ICMP-Standard, RFC 1071).
//────────────────────────────────────────────────────────────────────────────────────────────────
static uint16_t q_ipsum(const uint8_t *data, uint32_t len)
{
    uint32_t sum = 0;
    uint32_t i;

    for (i = 0; i + 1 < len; i += 2) {
        sum += ((uint32_t)data[i] << 8) | data[i + 1];
    }
    if (i < len) {
        sum += (uint32_t)data[i] << 8;
    }
    while (sum >> 16) {
        sum = (sum & 0xFFFFu) + (sum >> 16);
    }
    return (uint16_t)~sum;
}

//────────────────────────────────────────────────────────────────────────────────────────────────
// Function: q_backend_tx
// Desc.:    Ein kompletter Frame aus dem TX-Ring. ARP-Requests werden mit der Host-MAC
//           beantwortet (der Emulator antwortet fuer JEDE erfragte IP — Proxy-ARP, damit der
//           Gast spaeter beliebige Ziele ueber uns erreichen kann); ICMP-Echo-Requests an
//           10.0.0.2 kommen als Echo-Reply zurueck. Alles andere wird (noch) verworfen —
//           TCP/UDP-NAT ist die naechste Ausbaustufe von 5.11.
//────────────────────────────────────────────────────────────────────────────────────────────────
static void q_backend_tx(q9_quicc_t *q, const uint8_t *f, uint32_t len)
{
    uint8_t reply[QC_FRAME_MAX];

    if (qd_on()) {
        fprintf(stderr, "[quicc tx %u] %02x%02x%02x%02x%02x%02x <- %02x%02x%02x%02x%02x%02x typ %02x%02x\n",
                (unsigned)len, f[0], f[1], f[2], f[3], f[4], f[5],
                f[6], f[7], f[8], f[9], f[10], f[11], f[12], f[13]);
    }
    if (len < 14) {
        return;
    }

    /* ARP-Request (Ethertype $0806, Opcode 1, IPv4 ueber Ethernet)? */
    if (f[12] == 0x08 && f[13] == 0x06 && len >= 42 &&
        f[14] == 0x00 && f[15] == 0x01 &&             /* HW-Typ Ethernet   */
        f[16] == 0x08 && f[17] == 0x00 &&             /* Proto IPv4        */
        f[20] == 0x00 && f[21] == 0x01) {             /* Opcode: Request   */

        memcpy(reply,      f + 6,   6);               /* Ziel-MAC   = Absender                    */
        memcpy(reply + 6,  qh_mac,  6);               /* Quell-MAC  = Host                        */
        reply[12] = 0x08; reply[13] = 0x06;
        memcpy(reply + 14, f + 14,  6);               /* HW/Proto/Laengen unveraendert            */
        reply[20] = 0x00; reply[21] = 0x02;           /* Opcode: Reply                            */
        memcpy(reply + 22, qh_mac,  6);               /* Sender-MAC = Host                        */
        memcpy(reply + 28, f + 38,  4);               /* Sender-IP  = erfragte Ziel-IP            */
        memcpy(reply + 32, f + 22, 10);               /* Ziel-MAC/-IP = anfragender Gast          */
        q9_quicc_rx_frame(q, reply, 42);
        return;
    }

    /* ICMP-Echo-Request an die Host-Gegenstelle 10.0.0.2? */
    if (f[12] == 0x08 && f[13] == 0x00 && len >= 14 + 20 + 8 &&
        (f[14] & 0xF0) == 0x40 &&                     /* IPv4                                     */
        f[23] == 1 &&                                 /* Protokoll ICMP                           */
        f[30] == QH_IP0 && f[31] == QH_IP1 && f[32] == QH_IP2 && f[33] == QH_IP3) {

        uint32_t ihl    = (uint32_t)(f[14] & 0x0F) * 4u;
        uint32_t icmpo  = 14u + ihl;
        uint32_t icmpl;

        if (icmpo + 8u > len || f[icmpo] != 8) {      /* nur Echo-Request (Typ 8)                 */
            return;
        }
        icmpl = len - icmpo;
        if (len > sizeof(reply)) {
            return;
        }

        memcpy(reply, f, len);
        memcpy(reply,     f + 6,  6);                 /* MACs tauschen                            */
        memcpy(reply + 6, qh_mac, 6);
        memcpy(reply + 26, f + 30, 4);                /* IPs tauschen                             */
        memcpy(reply + 30, f + 26, 4);
        reply[24] = 0; reply[25] = 0;                 /* IP-Pruefsumme neu                        */
        {
            uint16_t s = q_ipsum(reply + 14, ihl);
            reply[24] = (uint8_t)(s >> 8); reply[25] = (uint8_t)s;
        }
        reply[icmpo] = 0;                             /* Typ 0: Echo-Reply                        */
        reply[icmpo + 2] = 0; reply[icmpo + 3] = 0;   /* ICMP-Pruefsumme neu                      */
        {
            uint16_t s = q_ipsum(reply + icmpo, icmpl);
            reply[icmpo + 2] = (uint8_t)(s >> 8); reply[icmpo + 3] = (uint8_t)s;
        }
        q9_quicc_rx_frame(q, reply, len);
        return;
    }
}

//────────────────────────────────────────────────────────────────────────────────────────────────
// Function: q_tx_run
// Desc.:    TX-Ring ab tbptr abarbeiten, wie es der CP nach TODR=$8000 taete: jeden BD mit
//           gesetztem Ready-Bit ans Backend geben, Ready loeschen, TXB-Ereignis setzen, tbptr
//           fortschreiben (Wrap zurueck auf tbase). Multi-BD-Frames (T_L erst spaeter) kommen
//           beim sp360-Treiber nicht vor (1 mbuf = 1 BD), werden aber verlustfrei uebersprungen.
//────────────────────────────────────────────────────────────────────────────────────────────────
static void q_tx_run(q9_quicc_t *q)
{
    uint32_t guard;

    if ((q_rd32(q, QO_SCC1_GSMRA) & QC_GSMR_ENT) == 0) {
        return;                                       /* Transmitter nicht aktiviert              */
    }

    for (guard = 0; guard < QC_TX_RING_MAX; guard++) {
        uint16_t tbptr  = q_rd16(q, QO_PRAM_TBPTR);
        uint16_t status;
        uint16_t flen;
        uint32_t buf;

        if (tbptr + QC_BD_SIZE > Q9_QUICC_MEM_LEN) {
            return;                                   /* kaputter Zeiger — nichts anfassen        */
        }
        status = q_rd16(q, tbptr);
        if ((status & QC_BD_TR_RE) == 0) {
            return;                                   /* Ring leer — fertig                       */
        }
        flen = q_rd16(q, tbptr + 2u);
        buf  = q_rd32(q, tbptr + 4u);

        if ((status & QC_BD_LAST) != 0 &&
            flen >= 14u && flen <= QC_FRAME_MAX &&
            buf < q->ram_len && buf + flen <= q->ram_len) {
            q_backend_tx(q, q->ram + buf, flen);      /* SDMA: Frame direkt aus dem Gast-RAM      */
        }

        q_wr16(q, tbptr, (uint16_t)(status & ~QC_BD_TR_RE));
        q_event(q, QC_EV_TXB);

        q_wr16(q, QO_PRAM_TBPTR,
               (status & QC_BD_WRAP) ? q_rd16(q, QO_PRAM_TBASE)
                                     : (uint16_t)(tbptr + QC_BD_SIZE));
    }
}

//────────────────────────────────────────────────────────────────────────────────────────────────
// Function: q_cp_command
// Desc.:    CP-Kommandoregister: Kommandos laufen synchron, das FLG-Bit ist beim Zurueckschreiben
//           schon geloescht (der Treiber pollt `while (cp_cr & CMD_FLAG)`). Nur Kanal 0 (SCC1)
//           wird interpretiert; INIT RX&TX PARAMS setzt die Ringzeiger auf die Ringanfaenge,
//           alle uebrigen Kommandos (ENTER HUNT MODE, RESTART TX, ...) sind hier No-Ops.
//────────────────────────────────────────────────────────────────────────────────────────────────
static void q_cp_command(q9_quicc_t *q, uint16_t cmd)
{
    if ((cmd & QC_CMD_FLAG) != 0 && (cmd & 0x00C0u) == 0) {
        if ((cmd & QC_CMD_OPMASK) == QC_INIT_RXTX) {
            q_wr16(q, QO_PRAM_RBPTR, q_rd16(q, QO_PRAM_RBASE));
            q_wr16(q, QO_PRAM_TBPTR, q_rd16(q, QO_PRAM_TBASE));
        }
    }
    q_wr16(q, QO_CP_CR, (uint16_t)(cmd & ~QC_CMD_FLAG));
}

//────────────────────────────────────────────────────────────────────────────────────────────────
// API-Implementierung
//────────────────────────────────────────────────────────────────────────────────────────────────

void q9_quicc_init(q9_quicc_t *q, uint8_t *ram, uint32_t ram_len)
{
    memset(q, 0, sizeof(*q));
    q->ram     = ram;
    q->ram_len = ram_len;
}

int q9_quicc_hit(uint32_t addr)
{
    return addr >= Q9_QUICC_BASE && addr <= Q9_QUICC_TOP;
}

uint8_t q9_quicc_read8(q9_quicc_t *q, uint32_t addr)
{
    uint32_t off = addr - Q9_QUICC_BASE;
    return (off < Q9_QUICC_MEM_LEN) ? q->mem[off] : 0;
}

uint16_t q9_quicc_read16(q9_quicc_t *q, uint32_t addr)
{
    uint32_t off = addr - Q9_QUICC_BASE;
    return (off + 1u < Q9_QUICC_MEM_LEN) ? q_rd16(q, off) : 0;
}

uint32_t q9_quicc_read32(q9_quicc_t *q, uint32_t addr)
{
    uint32_t off = addr - Q9_QUICC_BASE;
    return (off + 3u < Q9_QUICC_MEM_LEN) ? q_rd32(q, off) : 0;
}

void q9_quicc_write8(q9_quicc_t *q, uint32_t addr, uint8_t val)
{
    uint32_t off = addr - Q9_QUICC_BASE;

    if (off >= Q9_QUICC_MEM_LEN) {
        return;
    }
    /* Byteschreiber auf Register mit Nebenwirkung kommen beim sp360-Treiber nicht vor
       (write_word/write_long) — Bytes gehen deshalb schlicht ins Abbild. */
    q->mem[off] = val;
}

void q9_quicc_write16(q9_quicc_t *q, uint32_t addr, uint16_t val)
{
    uint32_t off = addr - Q9_QUICC_BASE;

    if (off + 1u >= Q9_QUICC_MEM_LEN) {
        return;
    }
    switch (off) {
    case QO_CP_CR:                                    /* Kommando synchron ausfuehren             */
        q_cp_command(q, val);
        return;
    case QO_SCC1_TODR:                                /* $8000: TX-Ring jetzt abarbeiten          */
        q_wr16(q, off, 0);                            /* CP nimmt das Bit sofort wieder zurueck   */
        if ((val & 0x8000u) != 0) {
            q_tx_run(q);
        }
        return;
    case QO_SCC1_SCCE:                                /* write-1-to-clear                         */
        q_wr16(q, off, (uint16_t)(q_rd16(q, off) & ~val));
        q_irq_update(q);
        return;
    case QO_SCC1_SCCM:                                /* Maske wirkt sofort auf den Pegel         */
        q_wr16(q, off, val);
        q_irq_update(q);
        return;
    default:
        q_wr16(q, off, val);
        return;
    }
}

void q9_quicc_write32(q9_quicc_t *q, uint32_t addr, uint32_t val)
{
    uint32_t off = addr - Q9_QUICC_BASE;

    if (off + 3u >= Q9_QUICC_MEM_LEN) {
        return;
    }
    if (off == QO_INTR_CISR) {                        /* write-1-to-clear                         */
        q_wr32(q, off, q_rd32(q, off) & ~val);
        return;
    }
    q_wr32(q, off, val);                              /* GSMRA/GSMRB/CIMR/...: reines Abbild      */
    if (off == QO_SCC1_GSMRA || off == QO_INTR_CIMR) {
        q_irq_update(q);                              /* Maskenaenderung kann Pegel freischalten  */
    }
}

int q9_quicc_irq_pending(const q9_quicc_t *q)
{
    return (q_rd32(q, QO_INTR_CIPR) & q_rd32(q, QO_INTR_CIMR) & QC_INTR_SCC1) != 0;
}

void q9_quicc_poll(q9_quicc_t *q)
{
    /* Sicherheitsnetz: haengengebliebene TX-BDs abraeumen (der Treiber kickt zwar bei jedem
       Frame per TODR, aber ein verpasster Kick darf keinen Stillstand bedeuten). Das
       ARP/ICMP-Backend arbeitet synchron im TX-Pfad — hier ist sonst nichts zu tun; die
       TCP/UDP-NAT-Ausbaustufe wird ihre Sockets in dieser Funktion bedienen. */
    q_tx_run(q);
}

void q9_quicc_rx_frame(q9_quicc_t *q, const uint8_t *frame, uint32_t len)
{
    uint16_t rbptr;
    uint16_t status;
    uint16_t mrblr;
    uint32_t buf;

    if ((q_rd32(q, QO_SCC1_GSMRA) & QC_GSMR_ENR) == 0 || len < 14u || len > QC_FRAME_MAX) {
        return;                                       /* Empfaenger aus oder Unsinnslaenge        */
    }

    rbptr = q_rd16(q, QO_PRAM_RBPTR);
    if (rbptr + QC_BD_SIZE > Q9_QUICC_MEM_LEN) {
        return;                                       /* kaputter Zeiger — nichts anfassen        */
    }
    status = q_rd16(q, rbptr);
    if ((status & QC_BD_TR_RE) == 0) {                /* kein leerer BD: Frame verwerfen wie die  */
        q_event(q, QC_EV_BSY);                        /* echte Hardware (busy condition)          */
        return;
    }

    mrblr = q_rd16(q, QO_PRAM_MRBLR);
    buf   = q_rd32(q, rbptr + 4u);
    if (len + 4u > mrblr || buf >= q->ram_len || buf + len > q->ram_len) {
        q_event(q, QC_EV_BSY);
        return;
    }

    memcpy(q->ram + buf, frame, len);                 /* SDMA: Frame direkt in den Gast-mbuf      */
    q_wr16(q, rbptr + 2u, (uint16_t)(len + 4u));      /* Laenge inkl. 4 CRC-Bytes (Treiber: -4)   */
    q_wr16(q, rbptr, (uint16_t)((status & (QC_BD_WRAP | QC_BD_IRQ)) |
                                QC_BD_RX_FIRST | QC_BD_LAST));
    q_event(q, QC_EV_RXF);

    q_wr16(q, QO_PRAM_RBPTR,
           (status & QC_BD_WRAP) ? q_rd16(q, QO_PRAM_RBASE)
                                 : (uint16_t)(rbptr + QC_BD_SIZE));

    if (qd_on()) {
        fprintf(stderr, "[quicc rx %u]\n", (unsigned)len);
    }
}
//────────────────────────────────────────────────────────────────────────────────────────────────
// EOF quicc.c                                                                             Ver. 1.00
//────────────────────────────────────────────────────────────────────────────────────────────────
