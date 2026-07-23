//════════════════════════════════════════════════════════════════════════════════════════════════
// File:   vmnet_net.h                                                                     Ver. 1.00
// Owner:  AF
// Desc.:  5.12: vmnet-Netzwerk-Backend fuer die QUICC-Ethernet-Emulation (macOS-only). Statt des
//         eingebauten Mini-NAT (quicc.c) reicht dieses Backend rohe Ethernet-Frames an Apples
//         vmnet.framework durch (VMNET_SHARED_MODE): macOS uebernimmt NAT, DHCP und Routing ins
//         echte Netz — das emulierte OS-9 kommt damit raus ins Internet, und der Mac erreicht
//         es direkt unter seiner Gast-IP (rein UND raus).
//
//         Subnetz-Verordnung: die Adressen kommen aus der .q9-Konfiguration; fehlen sie, werden
//         die bisherigen Q9-Defaults 192.168.200.0/24 und 192.168.200.1 verwendet.
//
//         Voraussetzung: root (sudo) oder das Entitlement com.apple.vm.networking — ohne das
//         schlaegt q9_vmnet_start() mit VMNET_FAILURE fehl (klare Fehlermeldung auf stderr).
//
//         Threading: vmnet liefert Frames auf einer eigenen Dispatch-Queue; sie landen in einem
//         mutex-geschuetzten Ringpuffer und werden vom Runner-Thread per q9_vmnet_recv()
//         abgeholt (Poll im CPU-Loop, s. q9_quicc_poll).
//
// Edition History
//─────────┬──────┬────────────────────────────────────────────────────────────────────────┬──────
// Date    │ Ver. │ Description                                                            │ By
//─────────┼──────┼────────────────────────────────────────────────────────────────────────┼──────
// 26-07-13│ 1.00 │ 5.12: Erster Wurf — Shared Mode, Ringpuffer, feste Subnetz-Zuweisung    │ CF
//═════════╧══════╧════════════════════════════════════════════════════════════════════════╧══════
#ifndef Q9_VMNET_NET_H
#define Q9_VMNET_NET_H

#include <stdint.h>

typedef struct q9_vmnet_config {
    const char *guest_ip;                    /* Dokumentation/Logik fuer die statische Gast-IP */
    const char *gateway;
    const char *netmask;
    const char *dhcp_end;
} q9_vmnet_config_t;

/* Interface im Shared Mode starten. NULL-Felder verwenden die bisherigen Q9-Defaults. */
int q9_vmnet_start(const q9_vmnet_config_t *config);

/* Die von vmnet zugewiesene Interface-MAC — der Gast MUSS mit dieser Absender-MAC senden,
   sonst verwirft vmnet die Frames; die Uebersetzung von/zur Gast-MAC macht quicc.c. */
const uint8_t *q9_vmnet_mac(void);

/* Frame raus (komplettes Ethernet-Frame inkl. Header). Fehler werden still verworfen —
   Netzwerk darf den Emulator nie anhalten. */
void q9_vmnet_send(const uint8_t *frame, uint32_t len);

/* Naechstes empfangenes Frame aus dem Ringpuffer holen. Rueckgabe: Framelaenge in Bytes,
   0 = nichts da (oder Puffer zu klein — dann wird das Frame verworfen). */
uint32_t q9_vmnet_recv(uint8_t *buf, uint32_t maxlen);

#endif /* Q9_VMNET_NET_H */
//────────────────────────────────────────────────────────────────────────────────────────────────
// EOF vmnet_net.h                                                                         Ver. 1.00
//────────────────────────────────────────────────────────────────────────────────────────────────
