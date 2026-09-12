//════════════════════════════════════════════════════════════════════════════════════════════════
// File:   nettty.h                                                                        Ver. 1.00
// Owner:  AF
// Desc.:  OS-9-Netzwerk-Terminal-Server -- 8 virtuelle serielle Kanaele /x1../x8, jeder per Raw-TCP
//         (mit minimaler Telnet-NVT-Filterung) an einen eigenen Host-Client gebunden. I/O-Block je
//         Kanal: +0 Status (Bit 0 RX Ready, Bit 1 TX Empty), +2 RX-Data, +4 TX-Data. Register-
//         Layout/Adresslage s. docs/BOARD.md.
//
//         2026-08-21 (Hardware-Vereinheitlichung, Folgeschritt): aus src/kernel/m68krt.c/q9board.h
//         HIERHER verschoben. ANDERS als bei duart68681/rtc72421/timer_irq (deren Zustand weiterhin
//         in q9_board_t steckt, s. dortige Kopfkommentare) UND anders als bei den bisherigen
//         Aufrufmuster von quicc/mc6845/framebuf/clut (die q9boardrun.c-verwaltete Zustands-
//         Zeiger per eigener q9_m68krt_attach_*()-Funktion uebergeben bekommen): `channels[]` war
//         schon vor dieser Runde vollstaendig SELBSTVERWALTET (kein externer Eigentuemer, kein von
//         aussen hereingereichter Zeiger) -- bleibt deshalb `static` in nettty.c, `q9_nettty_
//         attach()` kapselt die komplette Registrierung selbst (statt wie bei den anderen Typen
//         den Registrierungs-Code direkt in m68krt.c stehen zu lassen).
//
//         **Kernaenderung dieser Runde (Andreas' Vorgabe: "jedes Geraet einzeln behandeln, mit
//         eigenem Descriptor, eigener Adresse, im Array"):** bisher EIN gemeinsamer devreg-Eintrag
//         fuer alle acht Kanaele (dev->state=NULL, read8/write8/irq_pending/irq_vector_fn mussten
//         intern ueber alle acht suchen) -- jetzt ACHT separate q9_devreg_add()-Aufrufe, je einer
//         pro Kanal mit dev->state = &channels[i] (der eigene "Descriptor") und der eigenen,
//         bereits seit 2026-08-14 vorhandenen Basisadresse (Q9_BOARD_NET_X1..X8_BASE). Vorteile:
//         (a) entspricht dem einheitlichen Registrierungsmuster aller anderen acht Hardware-Typen,
//         (b) read8/write8/irq_pending werden dadurch trivial (kein Suchen mehr noetig), (c) der
//         schon bestehende generische IACK-/Reassert-Mechanismus (m68krt.c devreg_pending_level_
//         held(), durchlaeuft die Registry in Registrierungsreihenfolge) liefert JETZT SCHON GENAU
//         das dokumentierte Verhalten "Vektor des ERSTEN Kanals mit gesetztem RX-Ready-Bit" ganz
//         von selbst -- kein irq_vector_fn mehr noetig, jeder Kanal traegt seinen (weiterhin
//         festen) Vektor direkt in dev->irq_vector.
//
// Call:   q9_nettty_init();               // TCP-Listen-Socket oeffnen (einmalig, aus attach_board)
//         q9_nettty_attach();             // acht devreg-Eintraege registrieren (aus attach_board)
//         q9_nettty_poll();               // pro Hauptschleifen-Runde (aus q9_m68krt_execute)
//
// Edition History
//─────────┬──────┬────────────────────────────────────────────────────────────────────────┬──────
// Date    │ Ver. │ Description                                                            │ By
//─────────┼──────┼────────────────────────────────────────────────────────────────────────┬──────
// 25/26-xx│ 1.xx │ 5.10/5.16/5.17/5.20/6.x: urspruenglich Teil von m68krt.c/q9board.h, s.    │ CF/AF
//         │      │ dortige Historie fuer die volle Entwicklungsgeschichte (Telnet-NVT-       │
//         │      │ Filterung, Doppel-Echo-Fix, 256-Byte-Adressraster, Windows-Portabilitaet) │
// 26-08-21│ 1.00 │ Hardware-Vereinheitlichung: aus m68krt.c/q9board.h hierher verschoben,     │ Cld
//         │      │ acht separate devreg-Eintraege statt einem gemeinsamen (s.o.), neu         │
//         │      │ q9_devdesc_nettty                                                          │
//═════════╧══════╧════════════════════════════════════════════════════════════════════════╧══════
#ifndef Q9_NETTTY_H
#define Q9_NETTTY_H

#include "../../kernel/devdesc.h"                           /* q9_devdesc_t, q9_device_vtable_t   */

#define MAX_CHANNELS 8
#define MAIN_LISTEN_PORT 2000

/* I/O-Bloecke je Kanal (3 Register: +0 Status, +2 RX-Data, +4 TX-Data), freie Luecke zwischen
   ROM-Spiegelgrenze (bis 0xFFFF_0000 frei) und REMAP-Register (0xFFFF_8000) — kollidiert bewusst
   NICHT mit dem RAM. OS-9-Geraetenamen sind /x1../x8 (t1.. existiert im MWOS-Port schon anderweitig).

   2026-08-14 (ARBEITSPLAN 5.18-Fortsetzung, Andreas' Entscheidung: eigener 256-Byte-Bereich statt
   Index/Daten-Registerpaar -- "denke das ist erst mal einfacher"): 256 Byte Abstand, damit jeder
   Kanal seinen EIGENEN Slot in der I/O-Dispatch-Tabelle bekommt (s. m68krt.c g_io_table). MUSS mit
   den entsprechenden Konstanten in der OS-9-seitigen systype.d (Q9-Port-Repo, _NETX1_Base..
   _NETX8_Base/_NETX_Spacing) synchron gehalten werden -- sonst findet der Treiber die Kanaele
   nicht mehr. */
#define Q9_BOARD_NET_X1_BASE       0xFFFF1000u
#define Q9_BOARD_NET_X2_BASE       0xFFFF1100u
#define Q9_BOARD_NET_X3_BASE       0xFFFF1200u
#define Q9_BOARD_NET_X4_BASE       0xFFFF1300u
#define Q9_BOARD_NET_X5_BASE       0xFFFF1400u
#define Q9_BOARD_NET_X6_BASE       0xFFFF1500u
#define Q9_BOARD_NET_X7_BASE       0xFFFF1600u
#define Q9_BOARD_NET_X8_BASE       0xFFFF1700u
#define Q9_BOARD_NET_BASE          Q9_BOARD_NET_X1_BASE
#define Q9_BOARD_NET_TOP           0xFFFF17FFu             /* X8_BASE + 0xFF: letzter Kanal-Slot voll erfasst */

typedef struct {
    int client_fd;
    unsigned char rx_data;
    unsigned char tx_data;
    unsigned char status;  // Bit 0 = RX Ready, Bit 1 = TX Empty
    unsigned int base_addr;
    int irq_level;
    int irq_vector;
    unsigned char last_was_cr;  // Telnet-NVT-Normalisierung: LF nach CR verwerfen (5.16)
    unsigned char telnet_state; // IAC-Optionsverhandlung rausfiltern statt an OS-9 durchzureichen
                                 // (0=Daten, 1=nach IAC, 2=nach WILL/WONT/DO/DONT, 3=in SB, 4=in SB nach IAC)
} os9_uart_t;

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_nettty_init / q9_nettty_attach / q9_nettty_poll
// Desc.:    init() oeffnet den TCP-Listen-Socket (Port MAIN_LISTEN_PORT bzw. Q9_NETTTY_PORT-Env-
//           Var), NO-OP-artiges Verhalten bei Fehlschlag (Warnung auf stderr, Kanaele bleiben
//           unerreichbar statt Absturz -- s. nettty.c). attach() registriert die acht devreg-
//           Eintraege (s. Kopfkommentar). poll() bedient Verbindungsaufbau/-abbau und Byte-I/O fuer
//           alle acht Kanaele, EINMAL pro Hauptschleifen-Runde aufzurufen.
// Call:     q9_nettty_init(); q9_nettty_attach(); /* ... Hauptschleife: */ q9_nettty_poll();
//════════════════════════════════════════════════════════════════════════════════════════════════
void q9_nettty_init(void);
void q9_nettty_attach(void);
void q9_nettty_poll(void);
void q9_nettty_shutdown(void);      /* schliesst alle Kanal-/Server-Sockets, s. q9_m68krt_free() */

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_nettty_set_irq_hook
// Desc.:    2026-08-21: Entkopplung von Musashi (s. nettty.c-Kopfkommentar) -- nettty ruft anders
//           als alle anderen Hardware-Typen bei jedem ankommenden Byte SOFORT eine IRQ-Anforderung
//           aus (minimale Latenz), nicht nur ueber die generische devreg-Poll-Schleife. fn wird
//           dafuer als Callback hinterlegt (Signatur wie m68k_set_irq/q9_m68krt_set_irq: level=0
//           loescht die Anforderung). NULL (Default) macht jede IRQ-Anforderung zum stillen No-op
//           -- sicher fuer Aufrufer, die nettty nur ueber q9_devdesc_lookup() betrachten, ohne
//           q9_nettty_attach()/_poll() je auszufuehren.
// Call:     q9_nettty_set_irq_hook(q9_m68krt_set_irq);   // echter Betrieb, s. m68krt.c
//════════════════════════════════════════════════════════════════════════════════════════════════
void q9_nettty_set_irq_hook(void (*fn)(int level));

extern const q9_device_vtable_t q9_devtype_nettty;
extern const q9_devdesc_t       q9_devdesc_nettty;          /* 2026-08-21: Vtable+Schema vereint   */

#endif /* Q9_NETTTY_H */
//────────────────────────────────────────────────────────────────────────────────────────────────
// EOF nettty.h                                                                            Ver. 1.00
//────────────────────────────────────────────────────────────────────────────────────────────────
