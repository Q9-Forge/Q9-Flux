//════════════════════════════════════════════════════════════════════════════════════════════════
// File:   quicc.h                                                                         Ver. 1.00
// Owner:  AF
// Desc.:  5.11: QUICC-Ethernet-Emulation (MC68360, SCC1 im Ethernet-Modus) fuer den CB030-Runner —
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
//           nat   (Default) User-Mode-Mini-NAT, kein Root: der Host beantwortet als Gegenstelle
//                 10.0.0.2 ARP und ICMP-Echo — reicht fuer den Treiber-/Stack-Test.
//           vmnet (5.12, macOS, braucht sudo) Frames gehen roh an Apples vmnet.framework
//                 (Shared Mode, Gateway 10.0.0.2 = dieselbe Adresse): OS-9 kommt echt ins
//                 Netz (raus und rein). vmnet erzwingt seine zugewiesene Absender-MAC, der
//                 spqe0-Descriptor hat aber eine feste — deshalb uebersetzt das Backend die
//                 Gast-MAC in beiden Richtungen (inkl. der MAC-Felder in ARP-Paketen).
//
// Edition History
//─────────┬──────┬────────────────────────────────────────────────────────────────────────┬──────
// Date    │ Ver. │ Description                                                            │ By
//─────────┼──────┼────────────────────────────────────────────────────────────────────────┼──────
// 26-07-12│ 1.00 │ 5.11: Erster Wurf — Registerfenster, BD-Ringe, IRQ, ARP/ICMP-Backend    │ CF
// 26-07-13│ 1.10 │ 5.12: vmnet-Backend (--net vmnet) + MAC-Uebersetzung Gast<->vmnet       │ CF
//═════════╧══════╧════════════════════════════════════════════════════════════════════════╧══════
#ifndef Q9_QUICC_H
#define Q9_QUICC_H

#include <stdint.h>

//─── Adressfenster ────────────────────────────────────────────────────────────────────────────────
#define Q9_QUICC_BASE        0xFFFF2000u              /* QUICC-Basis ("MBAR"), 8K-aligned         */
#define Q9_QUICC_TOP         0xFFFF3FFFu              /* Fensterende (8K)                         */
#define Q9_QUICC_MEM_LEN     0x1800u                  /* belegt: DPRAM+PRAM+Register ($0-$17FF)   */

//─── Interrupt (Werte aus dem spqe0-Descriptor im MWOS-Q9-Port) ──────────────────────────────────
#define Q9_QUICC_IRQ_LEVEL   5                        /* Port IRQ Level                           */
#define Q9_QUICC_IRQ_VECTOR  254                      /* Port vector                              */

//─── Zustand ──────────────────────────────────────────────────────────────────────────────────────
/* Gesamter QUICC-Zustand. mem[] haelt das Fenster byteweise in Big-Endian-Sicht (wie der 68k es
   liest) — DPRAM, PRAM und alle Register leben dort; nur Zugriffe mit Nebenwirkung (CR, TODR,
   SCCE, CISR) werden beim Schreiben abgefangen. ram/ram_len ist das Gast-RAM fuer die
   SDMA-Zugriffe der BD-Ringe (Frame-Puffer = mbufs im System-RAM). */
typedef struct q9_quicc {
    uint8_t   mem[Q9_QUICC_MEM_LEN];                  /* DPRAM + PRAM + Registerbank              */
    uint8_t  *ram;                                    /* Gast-RAM (SDMA-Ziel/-Quelle)             */
    uint32_t  ram_len;
    int       use_vmnet;                              /* 5.12: Backend (0 = Mini-NAT, 1 = vmnet)  */
    uint8_t   guest_mac[6];                           /* 5.12: aus dem ersten TX-Frame gelernt    */
    int       guest_mac_ok;
} q9_quicc_t;

//─── API ──────────────────────────────────────────────────────────────────────────────────────────
void     q9_quicc_init(q9_quicc_t *q, uint8_t *ram, uint32_t ram_len);

/* 5.12: Backend waehlen — mode NULL/"nat" = Mini-NAT (Default), "vmnet" = vmnet.framework
   (macOS, braucht sudo). Rueckgabe 0 = ok; sonst ist die Fehlermeldung schon ausgegeben. */
int      q9_quicc_net_mode(q9_quicc_t *q, const char *mode);

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

/* Frame von aussen in den RX-Ring einspeisen (Backend/Test). Verworfen, wenn Empfaenger
   nicht aktiviert (GSMR ENR=0) oder kein leerer RX-BD bereitsteht (dann SCCE.BSY). */
void     q9_quicc_rx_frame(q9_quicc_t *q, const uint8_t *frame, uint32_t len);

#endif /* Q9_QUICC_H */
//────────────────────────────────────────────────────────────────────────────────────────────────
// EOF quicc.h                                                                             Ver. 1.00
//────────────────────────────────────────────────────────────────────────────────────────────────
