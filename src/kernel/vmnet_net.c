//════════════════════════════════════════════════════════════════════════════════════════════════
// File:   vmnet_net.c                                                                     Ver. 1.00
// Owner:  AF
// Desc.:  Implementierung des vmnet-Backends, siehe vmnet_net.h. macOS-only (vmnet.framework,
//         Dispatch, Blocks) — wird nur auf Darwin mitgebaut (Makefile, Q9_HAVE_VMNET).
//
// Edition History
//─────────┬──────┬────────────────────────────────────────────────────────────────────────┬──────
// Date    │ Ver. │ Description                                                            │ By
//─────────┼──────┼────────────────────────────────────────────────────────────────────────┼──────
// 26-07-13│ 1.00 │ 5.12: Erster Wurf — Shared Mode, Ringpuffer, feste Subnetz-Zuweisung    │ CF
//═════════╧══════╧════════════════════════════════════════════════════════════════════════╧══════
#include "vmnet_net.h"

#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <dispatch/dispatch.h>
#include <vmnet/vmnet.h>

//─── Subnetz-Zuweisung (muss zur OS-9-Konfiguration im MWOS-Q9-Port passen) ──────────────────────
#define QV_GATEWAY   "192.168.200.1"                  /* vmnet-Gateway = alte Mini-NAT-Adresse    */
#define QV_DHCP_END  "192.168.200.254"                /* DHCP-Bereich (Gast 192.168.200.2 ist statisch) */
#define QV_NETMASK   "255.255.255.0"                  /* /24 statt /16 (5.15): die /16-Maske kollidierte
                                                          mit privaten /16-Heimnetzen (z.B. WLAN-Router mit
                                                          192.168.0.0/16) und schickte den gesamten Traffic
                                                          am vmnet-Interface vorbei; IPs bleiben unveraendert */

//─── Ringpuffer fuer empfangene Frames (Dispatch-Queue -> Runner-Thread) ─────────────────────────
#define QV_SLOT_LEN  2048u                            /* > max. Ethernet-Frame (1518)             */
#define QV_SLOTS     64u                              /* Ueberlauf: aeltester Slot wird geopfert  */

typedef struct qv_state {
    interface_ref    ifr;                             /* laufendes vmnet-Interface                */
    uint8_t          mac[6];                          /* von vmnet zugewiesene Interface-MAC      */
    uint32_t         max_pkt;                         /* vmnet_max_packet_size_key                */
    pthread_mutex_t  lock;                            /* schuetzt den Ringpuffer                  */
    uint8_t          ring[QV_SLOTS][QV_SLOT_LEN];
    uint32_t         ring_len[QV_SLOTS];
    uint32_t         head;                            /* naechster freier Slot (Schreiber)        */
    uint32_t         tail;                            /* naechster voller Slot (Leser)            */
    uint32_t         dropped;                         /* verworfene Frames (Diagnose)             */
} qv_state_t;

static qv_state_t qv;                                 /* eine Instanz, wie q9_quicc_t (statisch)  */
static char       qv_macs[32];                        /* MAC-String aus dem Start-Handler (Blocks
                                                         koennen keine Arrays einfangen)          */

//────────────────────────────────────────────────────────────────────────────────────────────────
// Function: qv_parse_mac
// Desc.:    MAC-String "aa:bb:cc:dd:ee:ff" (vmnet_mac_address_key) in 6 Bytes wandeln.
//────────────────────────────────────────────────────────────────────────────────────────────────
static void qv_parse_mac(const char *s, uint8_t out[6])
{
    unsigned b[6] = { 0, 0, 0, 0, 0, 0 };

    if (s != NULL) {
        sscanf(s, "%x:%x:%x:%x:%x:%x", &b[0], &b[1], &b[2], &b[3], &b[4], &b[5]);
    }
    for (int i = 0; i < 6; i++) {
        out[i] = (uint8_t)b[i];
    }
}

//────────────────────────────────────────────────────────────────────────────────────────────────
// Function: qv_drain
// Desc.:    Alle bei vmnet anstehenden Frames abholen und in den Ringpuffer legen (laeuft auf
//           der vmnet-Dispatch-Queue). Ist der Ring voll, wird das aelteste Frame geopfert —
//           Verhalten wie echte Hardware unter Last, der Gast-Stack macht Retransmits.
//────────────────────────────────────────────────────────────────────────────────────────────────
static void qv_drain(void)
{
    for (;;) {
        uint8_t          buf[QV_SLOT_LEN];
        struct iovec     iov;
        struct vmpktdesc pd;
        int              cnt = 1;

        iov.iov_base   = buf;
        iov.iov_len    = sizeof(buf);
        pd.vm_pkt_size = sizeof(buf);
        pd.vm_pkt_iov  = &iov;
        pd.vm_pkt_iovcnt = 1;
        pd.vm_flags    = 0;

        if (vmnet_read(qv.ifr, &pd, &cnt) != VMNET_SUCCESS || cnt < 1) {
            return;                                   /* nichts (mehr) da                         */
        }
        if (pd.vm_pkt_size < 14u || pd.vm_pkt_size > QV_SLOT_LEN) {
            continue;                                 /* Unsinnslaenge — verwerfen                */
        }

        pthread_mutex_lock(&qv.lock);
        {
            uint32_t next = (qv.head + 1u) % QV_SLOTS;

            if (next == qv.tail) {                    /* Ring voll: aeltesten Slot freimachen     */
                qv.tail = (qv.tail + 1u) % QV_SLOTS;
                qv.dropped++;
            }
            memcpy(qv.ring[qv.head], buf, pd.vm_pkt_size);
            qv.ring_len[qv.head] = (uint32_t)pd.vm_pkt_size;
            qv.head = next;
        }
        pthread_mutex_unlock(&qv.lock);
    }
}

//────────────────────────────────────────────────────────────────────────────────────────────────
// API-Implementierung
//────────────────────────────────────────────────────────────────────────────────────────────────

int q9_vmnet_start(void)
{
    dispatch_queue_t     queue;
    dispatch_semaphore_t sem;
    xpc_object_t         desc;
    __block vmnet_return_t status  = VMNET_FAILURE;
    __block uint64_t       maxpkt  = 1518;

    pthread_mutex_init(&qv.lock, NULL);

    desc  = xpc_dictionary_create(NULL, NULL, 0);
    queue = dispatch_queue_create("q9.vmnet", DISPATCH_QUEUE_SERIAL);
    sem   = dispatch_semaphore_create(0);

    xpc_dictionary_set_uint64(desc, vmnet_operation_mode_key, VMNET_SHARED_MODE);
    xpc_dictionary_set_string(desc, vmnet_start_address_key, QV_GATEWAY);
    xpc_dictionary_set_string(desc, vmnet_end_address_key,   QV_DHCP_END);
    xpc_dictionary_set_string(desc, vmnet_subnet_mask_key,   QV_NETMASK);

    qv.ifr = vmnet_start_interface(desc, queue,
        ^(vmnet_return_t st, xpc_object_t params) {
            status = st;
            if (st == VMNET_SUCCESS && params != NULL) {
                const char *m = xpc_dictionary_get_string(params, vmnet_mac_address_key);
                if (m != NULL) {
                    strlcpy(qv_macs, m, sizeof(qv_macs));
                }
                maxpkt = xpc_dictionary_get_uint64(params, vmnet_max_packet_size_key);
            }
            dispatch_semaphore_signal(sem);
        });
    xpc_release(desc);

    if (qv.ifr != NULL) {
        dispatch_semaphore_wait(sem, DISPATCH_TIME_FOREVER);
    }
    if (qv.ifr == NULL || status != VMNET_SUCCESS) {
        fprintf(stderr,
                "q9: vmnet-Start fehlgeschlagen (Status %d) — das Shared-Mode-Interface braucht\n"
                "    root-Rechte: bitte mit `sudo` starten (oder --net nat verwenden).\n",
                (int)status);
        return 1;
    }

    qv_parse_mac(qv_macs, qv.mac);
    qv.max_pkt = (maxpkt != 0) ? (uint32_t)maxpkt : 1518u;

    vmnet_interface_set_event_callback(qv.ifr, VMNET_INTERFACE_PACKETS_AVAILABLE, queue,
        ^(interface_event_t ev, xpc_object_t event) {
            (void)ev; (void)event;
            qv_drain();
        });

    printf("[OS-9 Net] vmnet Shared Mode: Gateway %s/24, Interface-MAC %s\n", QV_GATEWAY, qv_macs);
    return 0;
}

const uint8_t *q9_vmnet_mac(void)
{
    return qv.mac;
}

void q9_vmnet_send(const uint8_t *frame, uint32_t len)
{
    struct iovec     iov;
    struct vmpktdesc pd;
    int              cnt = 1;

    if (qv.ifr == NULL || len < 14u || len > QV_SLOT_LEN || len > qv.max_pkt) {
        return;
    }
    iov.iov_base     = (void *)(uintptr_t)frame;      /* vmnet_write liest nur                    */
    iov.iov_len      = len;
    pd.vm_pkt_size   = len;
    pd.vm_pkt_iov    = &iov;
    pd.vm_pkt_iovcnt = 1;
    pd.vm_flags      = 0;
    (void)vmnet_write(qv.ifr, &pd, &cnt);
}

uint32_t q9_vmnet_recv(uint8_t *buf, uint32_t maxlen)
{
    uint32_t len = 0;

    if (qv.ifr == NULL) {
        return 0;
    }
    pthread_mutex_lock(&qv.lock);
    if (qv.tail != qv.head) {
        len = qv.ring_len[qv.tail];
        if (len <= maxlen) {
            memcpy(buf, qv.ring[qv.tail], len);
        } else {
            len = 0;                                  /* zu klein: Frame verwerfen                */
        }
        qv.tail = (qv.tail + 1u) % QV_SLOTS;
    }
    pthread_mutex_unlock(&qv.lock);
    return len;
}
//────────────────────────────────────────────────────────────────────────────────────────────────
// EOF vmnet_net.c                                                                         Ver. 1.00
//────────────────────────────────────────────────────────────────────────────────────────────────
