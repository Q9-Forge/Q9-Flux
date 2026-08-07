//════════════════════════════════════════════════════════════════════════════════════════════════
// File:   slirp_net.h                                                                     Ver. 1.00
// Owner:  AF
// Desc.:  5.14: libslirp-Netzwerk-Backend fuer die QUICC-Ethernet-Emulation -- PLATTFORMUEBERGREIFEND
//         (Mac/Linux/Windows gleich, anders als vmnet_net.c/bpf_net.c). libslirp ist dieselbe
//         Bibliothek, die QEMU/VirtualBox fuer "User-Mode-Networking" benutzen: simuliert NAT+DHCP
//         komplett in-process (kein root, kein Kernel-Treiber, keine echte Netzwerkschnittstelle
//         noetig) UND kann gezielt Host-Ports zu Gast-Ports durchreichen (hostfwd) -- genau das,
//         was Andreas' urspruenglicher Wunsch war ("Emu koennte doch einen Listener aufmachen").
//
//         Architektur-Unterschied zu vmnet_net.c: libslirp braucht KEINEN Hintergrund-Thread. Es
//         liefert Frames synchron per Callback (send_packet, s. q9_slirp_start) direkt beim Poll-
//         Aufruf (q9_slirp_poll, aus q9_quicc_poll, einmal je Hauptschleifen-Runde) -- kein
//         Ringpuffer/Mutex noetig, dafuer muss q9_slirp_poll die SLIRP-eigenen Sockets (fuer die
//         echten Verbindungen ins Internet) selbst pollen (slirp_pollfds_fill/_poll, s. .c).
//
//         Abhaengigkeit: third_party/slirp (vendorte MSYS2-Pakete libslirp+glib2, s. dortige
//         Q9_VENDOR.md) -- Q9_HAVE_SLIRP wird vom Makefile gesetzt, wenn third_party/slirp/include
//         existiert bzw. pkg-config slirp findet (Linux/macOS mit System-Paketen).
//
// Call:   q9_slirp_config_t cfg = { "192.168.200.2", "192.168.200.1", "255.255.255.0", 0 };
//         q9_slirp_hostfwd_t fwd[] = { { 0, 2323, 23 } };  // TCP hostport 2323 -> Gast-Port 23
//         q9_slirp_start(&cfg, fwd, 1, quicc_send_cb, quicc_opaque);
//         q9_slirp_input(frame, len);           // Gast -> Slirp (aus q_backend_tx)
//         q9_slirp_poll();                      // einmal je Hauptschleifen-Runde (q9_quicc_poll)
//
// Edition History
//─────────┬──────┬─────────────────────────────────────────────────────────────────────────┬──────
// Date    │ Ver. │ Description                                                            │ By
//─────────┼──────┼─────────────────────────────────────────────────────────────────────────┼──────
// 26-08-07│ 1.00 │ 5.14: Erster Wurf -- Andreas' Wunsch nach echter IP-Verbindung ohne      │ AF
//         │      │ physische Netzwerkschnittstelle, plattformuebergreifend                  │
//═════════╧══════╧═════════════════════════════════════════════════════════════════════════╧══════
#ifndef Q9_SLIRP_NET_H
#define Q9_SLIRP_NET_H

#include <stdint.h>

/* Subnetz-Konfiguration -- bewusst dieselbe Form wie q9_vmnet_config_t (vmnet_net.h), aber ein
   eigener Typ statt Wiederverwendung: vmnet_net.h ist macOS-only inkludierbar/gedacht, slirp_net.h
   soll ueberall (auch ohne Q9_HAVE_VMNET) alleinstehend nutzbar sein. NULL-Felder -> Q9-Defaults
   (192.168.200.0/24, Gateway .1), s. slirp_net.c. */
typedef struct q9_slirp_config {
    const char *guest_ip;
    const char *gateway;
    const char *netmask;
} q9_slirp_config_t;

/* Eine Host->Gast-Portweiterleitung (.q9-Key net_hostfwd, s. boardcfg.c/HANDBUCH.md). is_udp=0
   heisst TCP. host_addr leer/0 = INADDR_ANY (von ueberall erreichbar, wie die bestehenden
   /x1..x8-Terminals auch). */
typedef struct q9_slirp_hostfwd {
    int      is_udp;
    uint16_t host_port;
    uint16_t guest_port;
} q9_slirp_hostfwd_t;

/* Frame Slirp -> Gast (send_packet-Callback von libslirp, s. .c) -- vom Aufrufer bereitgestellt,
   i.d.R. q9_quicc_rx_frame. Rueckgabe: >=0 Anzahl geschriebener Bytes (ignoriert), <0 Fehler. */
typedef int (*q9_slirp_recv_cb)(const uint8_t *frame, uint32_t len, void *opaque);

/* Startet die Slirp-Instanz. hostfwd/hostfwd_count duerfen NULL/0 sein (kein Port-Forwarding).
   0 = ok, sonst ist eine Fehlermeldung schon auf stderr ausgegeben. */
int q9_slirp_start(const q9_slirp_config_t *config,
                    const q9_slirp_hostfwd_t *hostfwd, int hostfwd_count,
                    q9_slirp_recv_cb recv_cb, void *recv_opaque);

/* Frame rein (komplettes Ethernet-Frame inkl. Header) -- Gast -> Slirp. */
void q9_slirp_input(const uint8_t *frame, uint32_t len);

/* Einmal je Hauptschleifen-Runde aufrufen (q9_quicc_poll): pollt Slirps eigene Sockets (echte
   Verbindungen ins Internet) und liefert eingegangene Frames synchron per recv_cb aus q9_slirp_start. */
void q9_slirp_poll(void);

#endif /* Q9_SLIRP_NET_H */
//────────────────────────────────────────────────────────────────────────────────────────────────
// EOF slirp_net.h                                                                         Ver. 1.00
//────────────────────────────────────────────────────────────────────────────────────────────────
