//════════════════════════════════════════════════════════════════════════════════════════════════
// File:   vmnet_net.h                                                                     Ver. 1.00
// Owner:  AF
// Desc.:  5.12: vmnet-Netzwerk-Backend fuer die QUICC-Ethernet-Emulation (macOS-only). Statt des
//         eingebauten Mini-NAT (quicc.c) reicht dieses Backend rohe Ethernet-Frames an Apples
//         vmnet.framework durch (VMNET_SHARED_MODE): macOS uebernimmt NAT, DHCP und Routing ins
//         echte Netz — das emulierte OS-9 kommt damit raus ins Internet, und der Mac erreicht
//         es direkt unter seiner Gast-IP (rein UND raus).
//
//         Subnetz-Verordnung: vmnet bekommt beim Start das Subnetz 192.168.0.0/16 mit Gateway
//         192.168.200.1 zugewiesen (vmnet_start_address_key) — exakt die Adresse, die bisher das
//         Mini-NAT gespielt hat. Die OS-9-Konfiguration (inetdb2: enet0 = 192.168.200.2/16) bleibt
//         dadurch unveraendert; nur die Gegenstelle ist jetzt echt.
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

/* Interface starten (Shared Mode, Subnetz 192.168.0.0/16, Gateway 192.168.200.1). 0 = ok, sonst ist
   die Fehlermeldung (inkl. sudo-Hinweis) schon auf stderr ausgegeben. */
int q9_vmnet_start(void);

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
