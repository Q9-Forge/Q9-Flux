//════════════════════════════════════════════════════════════════════════════════════════════════
// File:   bpf_net.c                                                                      Ver. 1.00
// Owner:  AF
// Desc.:  Implementierung des Bridge-Backends, siehe bpf_net.h. macOS-only (BPF, /dev/bpf*) — wird
//         nur auf Darwin mitgebaut (Makefile, Q9_HAVE_BPF). UNGETESTET mit echter Hardware (2026-
//         07-13, keine zweite physische Schnittstelle verfuegbar) — Fehlerpfad (kein Interface,
//         keine BPF-Rechte) ist verifiziert, der Frame-Transport selbst noch nicht.
//
// Edition History
//─────────┬──────┬────────────────────────────────────────────────────────────────────────┬──────
// Date    │ Ver. │ Description                                                            │ By
//─────────┼──────┼────────────────────────────────────────────────────────────────────────┬──────
// 26-07-13│ 1.00 │ 5.13: Erster Wurf — BPF-Ringpuffer, Reader-Thread, kein MAC-Mapping     │ CF
//═════════╧══════╧════════════════════════════════════════════════════════════════════════╧══════
#include "bpf_net.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/ioctl.h>
#include <net/bpf.h>
#include <net/if.h>

//─── Ringpuffer fuer empfangene Frames (Reader-Thread -> Runner-Thread) ──────────────────────────
#define QB_SLOT_LEN   2048u                            /* > max. Ethernet-Frame (1518)              */
#define QB_SLOTS      64u                              /* Ueberlauf: aeltester Slot wird geopfert   */
#define QB_FALLBACK_BUFLEN (64u * 1024u)               /* falls BIOCGBLEN mal 0 meldet              */

typedef struct qb_state {
    int              fd;                               /* offener /dev/bpf*-Deskriptor              */
    uint32_t         buflen;                           /* von BIOCGBLEN gemeldete Lesepuffergroesse */
    pthread_mutex_t  lock;                              /* schuetzt den Ringpuffer                  */
    uint8_t          ring[QB_SLOTS][QB_SLOT_LEN];
    uint32_t         ring_len[QB_SLOTS];
    uint32_t         head;                              /* naechster freier Slot (Schreiber)        */
    uint32_t         tail;                              /* naechster voller Slot (Leser)             */
    uint32_t         dropped;                           /* verworfene Frames (Diagnose)              */
} qb_state_t;

static qb_state_t qb;                                  /* eine Instanz, wie qv in vmnet_net.c       */

//────────────────────────────────────────────────────────────────────────────────────────────────
// Function: qb_reader
// Desc.:    Eigener Thread: blockierend von der BPF-Schnittstelle lesen (ein read() kann mehrere
//           Frames liefern, je mit bpf_hdr-Praefix und BPF_WORDALIGN-Padding dazwischen) und in
//           den Ringpuffer legen.
//────────────────────────────────────────────────────────────────────────────────────────────────
static void *qb_reader(void *arg)
{
    uint8_t *rbuf = malloc(qb.buflen);

    (void)arg;
    if (rbuf == NULL) {
        return NULL;
    }
    for (;;) {
        ssize_t n = read(qb.fd, rbuf, qb.buflen);
        uint8_t *p, *end;

        if (n <= 0) {
            if (n < 0 && errno == EINTR) {
                continue;
            }
            break;                                     /* Interface weg/Fehler: Thread beendet sich */
        }
        p   = rbuf;
        end = rbuf + n;
        while (p < end) {
            struct bpf_hdr *h      = (struct bpf_hdr *)(void *)p;
            uint32_t        caplen = h->bh_caplen;
            uint8_t         *frame = p + h->bh_hdrlen;

            if (caplen >= 14u && caplen <= QB_SLOT_LEN) {
                pthread_mutex_lock(&qb.lock);
                {
                    uint32_t next = (qb.head + 1u) % QB_SLOTS;

                    if (next == qb.tail) {              /* Ring voll: aeltesten Slot freimachen     */
                        qb.tail = (qb.tail + 1u) % QB_SLOTS;
                        qb.dropped++;
                    }
                    memcpy(qb.ring[qb.head], frame, caplen);
                    qb.ring_len[qb.head] = caplen;
                    qb.head = next;
                }
                pthread_mutex_unlock(&qb.lock);
            }
            p += BPF_WORDALIGN(h->bh_hdrlen + caplen);
        }
    }
    free(rbuf);
    return NULL;
}

//────────────────────────────────────────────────────────────────────────────────────────────────
// API-Implementierung
//────────────────────────────────────────────────────────────────────────────────────────────────

int q9_bpf_start(const char *ifname)
{
    struct ifreq ifr;
    unsigned int one = 1;
    pthread_t    reader;

    if (ifname == NULL || ifname[0] == '\0') {
        fprintf(stderr,
                "q9: --net bridge braucht eine Schnittstelle: --net bridge:<ifname> "
                "(z.B. --net bridge:en5, `ifconfig -l` zeigt alle Namen).\n");
        return 1;
    }

    memset(&qb, 0, sizeof(qb));
    pthread_mutex_init(&qb.lock, NULL);

    qb.fd = -1;
    for (int i = 0; i < 256; i++) {
        char path[32];

        snprintf(path, sizeof(path), "/dev/bpf%d", i);
        qb.fd = open(path, O_RDWR);
        if (qb.fd >= 0) {
            break;
        }
        if (errno != EBUSY) {
            break;                                     /* kein weiteres Geraet mehr vorhanden       */
        }
    }
    if (qb.fd < 0) {
        fprintf(stderr,
                "q9: kein /dev/bpf* verfuegbar (%s). Fehlende Rechte? Einmalig einrichten mit:\n"
                "    sudo tools/macos/setup_bpf_access.sh\n"
                "    (oder --net nat/vmnet verwenden)\n", strerror(errno));
        return 1;
    }

    memset(&ifr, 0, sizeof(ifr));
    strlcpy(ifr.ifr_name, ifname, sizeof(ifr.ifr_name));
    if (ioctl(qb.fd, BIOCSETIF, &ifr) < 0) {
        fprintf(stderr, "q9: BIOCSETIF auf '%s' fehlgeschlagen (%s) — Interface-Name korrekt "
                "(`ifconfig -l`)?\n", ifname, strerror(errno));
        close(qb.fd);
        qb.fd = -1;
        return 1;
    }
    (void)ioctl(qb.fd, BIOCIMMEDIATE, &one);             /* keine Lesepuffer-Verzoegerung             */
    (void)ioctl(qb.fd, BIOCPROMISC,   NULL);             /* fremde Ziel-MACs (Gast!) auch empfangen  */
    (void)ioctl(qb.fd, BIOCSHDRCMPLT, &one);             /* Absender-MAC beim Schreiben NICHT ueber-
                                                              schreiben — der Gast liefert sie schon */
    if (ioctl(qb.fd, BIOCGBLEN, &qb.buflen) < 0 || qb.buflen == 0) {
        qb.buflen = QB_FALLBACK_BUFLEN;
    }

    if (pthread_create(&reader, NULL, qb_reader, NULL) != 0) {
        fprintf(stderr, "q9: Reader-Thread fuer BPF-Backend konnte nicht gestartet werden.\n");
        close(qb.fd);
        qb.fd = -1;
        return 1;
    }
    pthread_detach(reader);

    printf("[OS-9 Net] Bridge-Modus: BPF an '%s' gebunden (Puffer %u Byte, Promiscuous).\r\n",
           ifname, qb.buflen);
    return 0;
}

void q9_bpf_send(const uint8_t *frame, uint32_t len)
{
    if (qb.fd < 0 || len < 14u) {
        return;
    }
    (void)write(qb.fd, frame, len);
}

uint32_t q9_bpf_recv(uint8_t *buf, uint32_t maxlen)
{
    uint32_t len = 0;

    if (qb.fd < 0) {
        return 0;
    }
    pthread_mutex_lock(&qb.lock);
    if (qb.tail != qb.head) {
        len = qb.ring_len[qb.tail];
        if (len <= maxlen) {
            memcpy(buf, qb.ring[qb.tail], len);
        } else {
            len = 0;                                   /* zu klein: Frame verwerfen                 */
        }
        qb.tail = (qb.tail + 1u) % QB_SLOTS;
    }
    pthread_mutex_unlock(&qb.lock);
    return len;
}
//────────────────────────────────────────────────────────────────────────────────────────────────
// EOF bpf_net.c                                                                          Ver. 1.00
//────────────────────────────────────────────────────────────────────────────────────────────────
