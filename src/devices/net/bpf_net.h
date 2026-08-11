//════════════════════════════════════════════════════════════════════════════════════════════════
// File:   bpf_net.h                                                                      Ver. 1.00
// Owner:  AF
// Desc.:  5.13: Bridge-Netzwerk-Backend fuer die QUICC-Ethernet-Emulation (macOS-only). Statt vmnet
//         (braucht root oder ein von Apple provisioniertes Entitlement, s. vmnet_net.h) haengt
//         dieses Backend den Gast per BPF (/dev/bpf*) direkt an eine PHYSISCHE Netzwerkschnittstelle
//         — echtes Layer-2-Bridging, keine MAC-Uebersetzung noetig (anders als beim vmnet-Backend),
//         der Gast bekommt seine IP wie ein normales Geraet per DHCP vom echten Router.
//
//         Voraussetzung: WLAN-Interfaces lassen sich auf den meisten Access Points NICHT bridgen
//         (die Basisstation verwirft Frames mit fremder Absender-MAC) — es wird eine dedizierte
//         Kabel-Ethernet-Schnittstelle empfohlen (z.B. USB-Ethernet-Adapter), die exklusiv fuer
//         den Emulator reserviert ist.
//
//         Rechte: BPF-Devices gehoeren standardmaessig root:wheel (0600). Damit q9.exe OHNE sudo
//         laeuft, muss einmalig die Zugriffsberechtigung angepasst werden (ChmodBPF-Muster, wie es
//         z.B. Wireshark installiert) — s. tools/macos/setup_bpf_access.sh. Ohne dieses Setup
//         schlaegt q9_bpf_start() mit einer klaren Fehlermeldung fehl.
//
//         Status (2026-07-13): Code vorbereitet, aber MANGELS zweiter physischer Netzwerkschnitt-
//         stelle noch nicht end-to-end getestet (nur Kompilierbarkeit + Fehlerpfad ohne Interface
//         verifiziert). Vor produktivem Einsatz: mit echter Hardware nachtesten.
//
// Edition History
//─────────┬──────┬────────────────────────────────────────────────────────────────────────┬──────
// Date    │ Ver. │ Description                                                            │ By
//─────────┼──────┼────────────────────────────────────────────────────────────────────────┬──────
// 26-07-13│ 1.00 │ 5.13: Erster Wurf — BPF-Ringpuffer, Reader-Thread, kein MAC-Mapping     │ CF
//═════════╧══════╧════════════════════════════════════════════════════════════════════════╧══════
#ifndef Q9_BPF_NET_H
#define Q9_BPF_NET_H

#include <stdint.h>

/* Interface starten (BPF an <ifname> gebunden, Promiscuous+Immediate). 0 = ok, sonst ist die
   Fehlermeldung (inkl. Hinweis auf setup_bpf_access.sh) schon auf stderr ausgegeben. */
int q9_bpf_start(const char *ifname);

/* Frame raus (komplettes Ethernet-Frame inkl. Header, Absender-MAC bleibt die des Gastes —
   echtes Bridging, keine Uebersetzung). Fehler werden still verworfen. */
void q9_bpf_send(const uint8_t *frame, uint32_t len);

/* Naechstes empfangenes Frame aus dem Ringpuffer holen. Rueckgabe: Framelaenge in Bytes,
   0 = nichts da (oder Puffer zu klein — dann wird das Frame verworfen). */
uint32_t q9_bpf_recv(uint8_t *buf, uint32_t maxlen);

#endif /* Q9_BPF_NET_H */
//────────────────────────────────────────────────────────────────────────────────────────────────
// EOF bpf_net.h                                                                          Ver. 1.00
//────────────────────────────────────────────────────────────────────────────────────────────────
