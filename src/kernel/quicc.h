//════════════════════════════════════════════════════════════════════════════════════════════════
// File:   quicc.h                                                                         Ver. 1.40
// Owner:  AF
// Desc.:  5.11: QUICC-Ethernet-Emulation (MC68360, SCC1 im Ethernet-Modus) fuer den Board-Runner —
//         das Hardware-Gegenstueck zum originalen Microware-SPF-Treiber `sp360` (MWOS-SDK,
//         SRC/DPIO/SPF/DRVR/SPQUICC). Emuliert wird NUR das, was dieser Treiber tatsaechlich
//         anfasst: 8K-Fenster ab Q9_QUICC_BASE mit Dual-Port-RAM (BD-Ringe), SCC1-Parameter-RAM
//         und der Registerbank (CP-Kommandoregister, CPM-Interrupt-Controller, SCC1-Register).
//
//         Adressmodell (Entscheidung Andreas+Claudia 2026-07-12, s. ARBEITSPLAN 5.11):
//         QUICC-Basis $FFFF2000 (8K-aligned, kollisionsfrei zwischen Netz-Terminals $FFFF104F
//         und REMAP $FFFF8000). Der Treiber bekommt vom Descriptor `spqe0` die PRAM-Adresse
//         $FFFF2C00 und rechnet selbst: Basis = PORTADDR & $FFFFF000, Kanal = Bits 8-9 ($C00 =
//         Kanal 0 = SCC1). Register-Offsets exakt wie MC68360 (per offsetof aus Motorolas
//         quicc.h verifiziert): PRAM +$C00, CPM-Int-Ctrl +$1540, CR +$15C0, SCC1 +$1600.
//
//         Die Frame-Puffer selbst liegen NICHT im Fenster: die Buffer-Descriptoren im DPRAM
//         zeigen auf mbufs im Gast-RAM — die SDMA-Emulation liest/schreibt dort direkt.
//
//         Host-Backend, waehlbar per `--net` (s. q9_quicc_net_mode):
//           nat    (Default) User-Mode-Mini-NAT, kein Root: der Host beantwortet als Gegenstelle
//                  192.168.200.1 ARP und ICMP-Echo — reicht fuer den Treiber-/Stack-Test.
//           vmnet  (5.12, macOS, braucht sudo ODER das von Apple gesperrte Entitlement
//                  com.apple.vm.networking — Ad-hoc-Signierung reicht dafuer NICHT, macOS killt
//                  den Prozess dann per SIGKILL, s. ARBEITSPLAN 5.13) Frames gehen roh an Apples
//                  vmnet.framework (Shared Mode, Subnetz 192.168.200.0/24, Gateway 192.168.200.1 =
//                  dieselbe Adresse): OS-9 kommt echt ins Netz (raus und rein). vmnet erzwingt
//                  seine zugewiesene Absender-MAC, der spqe0-Descriptor hat aber eine feste —
//                  deshalb uebersetzt das Backend die Gast-MAC in beiden Richtungen (inkl. der
//                  MAC-Felder in ARP-Paketen).
//           bridge (5.13, macOS, --net bridge:<ifname>, z.B. bridge:en5) Frames gehen roh per BPF
//                  (/dev/bpf*) an eine PHYSISCHE Netzwerkschnittstelle — echtes Layer-2-Bridging,
//                  KEINE MAC-Uebersetzung noetig, der Gast bekommt seine IP direkt vom echten
//                  Router per DHCP. Braucht KEIN root/Entitlement, sondern einmalig per Setup-
//                  Skript angepasste BPF-Geraeterechte (tools/macos/setup_bpf_access.sh) — dafuer
//                  eine dedizierte Kabel-Ethernet-Schnittstelle (WLAN laesst sich meist nicht
//                  bridgen). Status 2026-07-13: Code vorbereitet, mangels zweiter physischer
//                  Schnittstelle noch NICHT end-to-end getestet (s. bpf_net.h).
//           slirp  (5.14, PLATTFORMUEBERGREIFEND -- Mac/Linux/Windows gleich, anders als vmnet/
//                  bridge) libslirp (dieselbe Bibliothek wie QEMU/VirtualBox "User-Mode-Networking"):
//                  simuliert NAT+DHCP komplett in-process, kein root, kein Kernel-Treiber, keine
//                  physische Netzwerkschnittstelle noetig -- UND kann Host-Ports gezielt zu
//                  Gast-Ports durchreichen (net_hostfwd in der .q9-Config, s. slirp_net.h). Braucht
//                  third_party/slirp (vendorte libslirp+glib2, Q9_HAVE_SLIRP), s. dortige Q9_VENDOR.md.
//
// Edition History
//─────────┬──────┬────────────────────────────────────────────────────────────────────────┬──────
// Date    │ Ver. │ Description                                                            │ By
//─────────┼──────┼────────────────────────────────────────────────────────────────────────┼──────
// 26-07-12│ 1.00 │ 5.11: Erster Wurf — Registerfenster, BD-Ringe, IRQ, ARP/ICMP-Backend    │ CF
// 26-07-13│ 1.10 │ 5.12: vmnet-Backend (--net vmnet) + MAC-Uebersetzung Gast<->vmnet       │ CF
// 26-07-13│ 1.20 │ 5.13: bridge-Backend (--net bridge:<ifname>) per BPF, kein root noetig  │ CF
// 26-07-14│ 1.30 │ 5.17: q9_devtype_quicc-Vtable fuer die Geraete-Registry exportiert       │ CF
// 26-08-07│ 1.40 │ 5.14: slirp-Backend (--net slirp), plattformuebergreifend, mit Host->    │ AF
//         │      │ Gast-Portweiterleitung (Andreas' Wunsch nach echter IP-Verbindung ohne   │
//         │      │ physische Netzwerkschnittstelle)                                         │
//═════════╧══════╧════════════════════════════════════════════════════════════════════════╧══════
#ifndef Q9_QUICC_H
#define Q9_QUICC_H

#include <stdint.h>
#include "devreg.h"                                    /* 5.17: q9_device_t/Vtable, s. devreg.h  */
#include "vmnet_net.h"                                  /* vmnet-Konfiguration aus .q9            */
#include "slirp_net.h"                                  /* 5.14: slirp-Konfiguration aus .q9      */

//─── Adressfenster ────────────────────────────────────────────────────────────────────────────────
#define Q9_QUICC_BASE        0xFFFF2000u              /* QUICC-Basis ("MBAR"), 8K-aligned         */
#define Q9_QUICC_TOP         0xFFFF3FFFu              /* Fensterende (8K)                         */
#define Q9_QUICC_MEM_LEN     0x1800u                  /* belegt: DPRAM+PRAM+Register ($0-$17FF)   */

//─── Interrupt (Werte aus dem spqe0-Descriptor im MWOS-Q9-Port) ──────────────────────────────────
#define Q9_QUICC_IRQ_LEVEL   5                        /* Port IRQ Level                           */
#define Q9_QUICC_IRQ_VECTOR  254                      /* Port vector                              */

//─── Backend-Auswahl ──────────────────────────────────────────────────────────────────────────────
#define Q9_NET_NAT     0                                  /* Mini-NAT (Default), kein root          */
#define Q9_NET_VMNET   1                                  /* vmnet.framework, macOS, braucht sudo    */
#define Q9_NET_BRIDGE  2                                  /* 5.13: BPF an physischer NIC, kein root  */
#define Q9_NET_SLIRP   3                                  /* 5.14: libslirp, plattformuebergreifend  */

//─── Zustand ──────────────────────────────────────────────────────────────────────────────────────
/* Gesamter QUICC-Zustand. mem[] haelt das Fenster byteweise in Big-Endian-Sicht (wie der 68k es
   liest) — DPRAM, PRAM und alle Register leben dort; nur Zugriffe mit Nebenwirkung (CR, TODR,
   SCCE, CISR) werden beim Schreiben abgefangen. ram/ram_len ist das Gast-RAM fuer die
   SDMA-Zugriffe der BD-Ringe (Frame-Puffer = mbufs im System-RAM). */
typedef struct q9_quicc {
    uint8_t   mem[Q9_QUICC_MEM_LEN];                  /* DPRAM + PRAM + Registerbank              */
    uint8_t  *ram;                                    /* Gast-RAM (SDMA-Ziel/-Quelle)             */
    uint32_t  ram_len;
    int       net_backend;                            /* 5.13: Q9_NET_NAT/VMNET/BRIDGE            */
    uint8_t   guest_mac[6];                           /* 5.12: aus dem ersten TX-Frame gelernt    */
    int       guest_mac_ok;
    /* 5.15-Diagnose (TCP-Haenger): trennt Emulator-Interrupt-Verlust vom Gast-Treiber-Bug.
       rxf = wie oft ein RX-Frame in den Ring gelegt + RXF-Ereignis gesetzt wurde;
       bsy = wie oft ein RX-Frame mangels leerem BD VERWORFEN wurde (BSY, sonst unsichtbar!);
       txb = wie oft ein TX-Frame abgearbeitet + TXB gesetzt wurde. Vergleicht man rxf mit dem
       QUICC-IACK-Zaehler (m68krt.c) waehrend eines Haengers, zeigt sich, ob der Level-5-Interrupt
       ueberhaupt noch zugestellt wird (ISR laeuft -> Gast-Bug) oder ausbleibt (Emulator-Bug). */
    uint32_t  diag_rxf;
    uint32_t  diag_bsy;
    uint32_t  diag_txb;
} q9_quicc_t;

//─── API ──────────────────────────────────────────────────────────────────────────────────────────
void     q9_quicc_init(q9_quicc_t *q, uint8_t *ram, uint32_t ram_len);

/* 5.14: Backend waehlen — mode NULL/"nat" = Mini-NAT (Default), "vmnet" = vmnet.framework (macOS,
   braucht sudo/Entitlement), "bridge:<ifname>" = BPF an physischer NIC (macOS, kein root, s.
   bpf_net.h), "slirp" = libslirp (plattformuebergreifend, s. slirp_net.h) -- slirp_config/hostfwd
   werden nur fuer mode="slirp" ausgewertet, sonst duerfen sie NULL/0 sein. Rueckgabe 0 = ok; sonst
   ist die Fehlermeldung schon ausgegeben. */
int      q9_quicc_net_mode(q9_quicc_t *q, const char *mode,
                           const q9_vmnet_config_t *vmnet_config,
                           const q9_slirp_config_t *slirp_config,
                           const q9_slirp_hostfwd_t *slirp_hostfwd, int slirp_hostfwd_count);

/* Trifft die Adresse das QUICC-Fenster? (fuer den Dispatch in m68krt.c) */
int      q9_quicc_hit(uint32_t addr);

/* Speicherzugriffe des 68k auf das Fenster (Adressen absolut, Basis wird intern abgezogen) */
uint8_t  q9_quicc_read8 (q9_quicc_t *q, uint32_t addr);
uint16_t q9_quicc_read16(q9_quicc_t *q, uint32_t addr);
uint32_t q9_quicc_read32(q9_quicc_t *q, uint32_t addr);
void     q9_quicc_write8 (q9_quicc_t *q, uint32_t addr, uint8_t  val);
void     q9_quicc_write16(q9_quicc_t *q, uint32_t addr, uint16_t val);
void     q9_quicc_write32(q9_quicc_t *q, uint32_t addr, uint32_t val);

/* 1, wenn der QUICC gerade einen Interrupt anfordert (SCCE&SCCM aktiv, CIMR offen) —
   der Runner setzt dann IRQ-Level 5, der IACK-Callback liefert Q9_QUICC_IRQ_VECTOR. */
int      q9_quicc_irq_pending(const q9_quicc_t *q);

/* Einmal pro Runner-Runde: Backend bedienen (eingehende Frames in den RX-Ring legen). */
void     q9_quicc_poll(q9_quicc_t *q);

/* 5.15-Diagnose: Anzahl GEFUELLTER (vom Gast noch nicht abgeholter) RX-BDs im Ring. */
int      q9_quicc_rx_filled(const q9_quicc_t *q);

/* Frame von aussen in den RX-Ring einspeisen (Backend/Test). Verworfen, wenn Empfaenger
   nicht aktiviert (GSMR ENR=0) oder kein leerer RX-BD bereitsteht (dann SCCE.BSY). */
void     q9_quicc_rx_frame(q9_quicc_t *q, const uint8_t *frame, uint32_t len);

/* 5.17: Vtable fuer die Geraete-Registry (devreg.h) -- Instanz wird in m68krt.c angelegt,
   dev->state zeigt auf das q9_quicc_t-Handle. */
extern const q9_device_vtable_t q9_devtype_quicc;

#endif /* Q9_QUICC_H */
//────────────────────────────────────────────────────────────────────────────────────────────────
// EOF quicc.h                                                                             Ver. 1.40
//────────────────────────────────────────────────────────────────────────────────────────────────
