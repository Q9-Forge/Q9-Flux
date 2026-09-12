//════════════════════════════════════════════════════════════════════════════════════════════════
// File:   nettty.c                                                                        Ver. 1.00
// Owner:  AF
// Desc.:  Implementierung, siehe nettty.h. Reine Verschiebung aus src/kernel/m68krt.c (2026-08-21,
//         Hardware-Vereinheitlichung) -- Netzwerk-/Telnet-/Registerlogik UNVERAENDERT, s. dortiger
//         Kopfkommentar. NEU: acht separate devreg-Eintraege statt einem gemeinsamen (q9_nettty_
//         attach()), q9_devdesc_nettty, und eine kleine Entkopplung von Musashi (s.u.).
//
//         **Bewusst KEIN direktes `#include "m68k.h"`/Musashi-Linken:** anders als alle anderen
//         acht Hardware-Typen ruft nettty (schon vor dieser Runde) `m68k_set_irq()` DIREKT auf --
//         nicht nur ueber die generische devreg-Poll-Schleife (q9boardrun.c), sondern zusaetzlich
//         bei jedem einzelnen ankommenden Byte (network_irq_resync/q9_nettty_poll, fuer minimale
//         Latenz). Ein direktes Musashi-Include haette JEDEN Aufrufer von devdesc.c (test-devdesc,
//         den Editor, ...) gezwungen, die volle Musashi-CPU-Kernobjekte mitzulinken -- nur wegen
//         eines Hardware-Typs, den diese Aufrufer nie ausfuehren. Stattdessen: ein simpler
//         Funktionszeiger-Hook (`q9_nettty_set_irq_hook()`), den m68krt.c beim echten Attach mit
//         `q9_m68krt_set_irq` (dessen bereits vorhandener duenner Musashi-Wrapper, s. dortiger
//         Kopfkommentar "damit ... nicht direkt gegen third_party/musashi linken muss" -- exakt
//         dasselbe Entkopplungsmuster, hier nur eine Ebene weitergereicht) verdrahtet. Ungewurzelt
//         (Hook==NULL, z.B. in Tests, die nur q9_devdesc_lookup() brauchen) ist ein IRQ-Anforderung-
//         Versuch ein stilles No-op -- unschaedlich, da dort ohnehin nie echte Bytes ankommen.
//
// Edition History
//─────────┬──────┬────────────────────────────────────────────────────────────────────────┬──────
// Date    │ Ver. │ Description                                                            │ By
//─────────┼──────┼────────────────────────────────────────────────────────────────────────┬──────
// 25/26-xx│ 1.xx │ 5.10/5.16/5.17/5.20/6.x: urspruenglich Teil von m68krt.c, s. dortige      │ CF/AF
//         │      │ Historie fuer die volle Entwicklungsgeschichte                            │
// 26-08-21│ 1.00 │ Hardware-Vereinheitlichung: aus m68krt.c hierher verschoben, acht          │ Cld
//         │      │ separate devreg-Eintraege (q9_nettty_attach), Musashi-Entkopplung per       │
//         │      │ Funktionszeiger-Hook, neu q9_devdesc_nettty                                │
//═════════╧══════╧════════════════════════════════════════════════════════════════════════╧══════
#include "nettty.h"
#include "../../kernel/devreg.h"
#include "../../kernel/q9_sockcompat.h"     /* Windows-Build: Windows/Winsock-Portabilitaet     */
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* 5.10: 8 virtuelle Netzwerk-Terminals /x1../x8 (vorher 4x /t1../t4) — Registerlayout je Kanal
   s. nettty.h. Jeder Kanal hat seinen EIGENEN Autovektor (70..77). */
static os9_uart_t channels[MAX_CHANNELS] = {
    {-1, 0, 0, 0x02, Q9_BOARD_NET_X1_BASE, 4, 70, 0, 0}, // /x1
    {-1, 0, 0, 0x02, Q9_BOARD_NET_X2_BASE, 4, 71, 0, 0}, // /x2
    {-1, 0, 0, 0x02, Q9_BOARD_NET_X3_BASE, 4, 72, 0, 0}, // /x3
    {-1, 0, 0, 0x02, Q9_BOARD_NET_X4_BASE, 4, 73, 0, 0}, // /x4
    {-1, 0, 0, 0x02, Q9_BOARD_NET_X5_BASE, 4, 74, 0, 0}, // /x5
    {-1, 0, 0, 0x02, Q9_BOARD_NET_X6_BASE, 4, 75, 0, 0}, // /x6
    {-1, 0, 0, 0x02, Q9_BOARD_NET_X7_BASE, 4, 76, 0, 0}, // /x7
    {-1, 0, 0, 0x02, Q9_BOARD_NET_X8_BASE, 4, 77, 0, 0}  // /x8
};

static int main_server_fd = -1;

/* s. Kopfkommentar -- Entkopplung von Musashi. */
static void (*g_irq_raise_fn)(int level) = NULL;

void q9_nettty_set_irq_hook(void (*fn)(int level))
{
    g_irq_raise_fn = fn;
}

static void raise_irq(int level)
{
    if (g_irq_raise_fn) {
        g_irq_raise_fn(level);
    }
}

/* 5.10: Die IRQ-Leitung ist das ODER aller RX-Ready-Bits (level-getriggert). Nach jedem Verbrauch
   eines Bytes bzw. nach jedem globalen Absenken (int_ack) muss sie erneut angehoben werden, wenn
   IRGENDEIN anderer Kanal noch ein unabgeholtes Byte hat — sonst verliert der Kanal seinen
   Interrupt und bekommt erst beim NAECHSTEN Byte wieder einen. */
static void network_irq_resync(void) {
    for (int i = 0; i < MAX_CHANNELS; i++) {
        if (channels[i].status & 0x01) {
            raise_irq(channels[i].irq_level);
            return;
        }
    }
    raise_irq(0);
}

/* 5.20 (Andreas' Doppel-Echo-Bugreport): der Server hat bisher gar keine Telnet-Optionsverhandlung
   gemacht -- reines Raw-TCP. Ein echter Telnet-Client (Windows telnet.exe, PuTTY) faengt deshalb
   OHNE Gegensignal an, selbst lokal zu echoen (NVT-Default), UND der OS-9-Treiber echoet jedes
   eingegangene Zeichen wie ein echtes serielles Terminal -- macht "dir" zu "ddiirr". Fix in zwei
   Teilen: (1) bei Verbindungsaufbau IAC WILL ECHO + IAC WILL SUPPRESS-GO-AHEAD schicken, damit sich
   ein RFC854-konformer Client selbst abschaltet; (2) die IAC-Antwortsequenzen, die der Client
   daraufhin zurückschickt (z.B. IAC DO ECHO), aus dem eingehenden Bytestrom rausfiltern statt sie
   als Tippzeichen an OS-9 durchzureichen -- sonst landet z.B. ein rohes 0xFF im Login-Prompt. */
#define Q9_TELNET_IAC   0xFF
#define Q9_TELNET_WILL  0xFB
#define Q9_TELNET_WONT  0xFC
#define Q9_TELNET_DO    0xFD
#define Q9_TELNET_DONT  0xFE
#define Q9_TELNET_SB    0xFA
#define Q9_TELNET_SE    0xF0
#define Q9_TELNET_ECHO           0x01
#define Q9_TELNET_SUPPRESS_GA    0x03

static const unsigned char g_telnet_negotiate[] = {
    Q9_TELNET_IAC, Q9_TELNET_WILL, Q9_TELNET_ECHO,
    Q9_TELNET_IAC, Q9_TELNET_WILL, Q9_TELNET_SUPPRESS_GA
};

/* Andreas' Wunsch (2026-08-07): ein Strg-Zeichen soll NUR die eigene Telnet-Verbindung sauber
   trennen (wie ein Logout), ohne den Rest des Emulators anzufassen -- anders als der lokale
   Ctrl-Q-Host-Escape (hal_windows.c/hal_posix.c), der den GANZEN Prozess beendet. Gleicher Buchstabe
   (Ctrl-Q) als Default wie dort, bewusst: Andreas hatte instinktiv genau das in einer Telnet-Session
   probiert, "mein Exit-Reflex" soll ueberall gleich funktionieren (nur die Reichweite unterscheidet
   sich: lokal = ganzer Prozess, hier = nur die eine Verbindung). Per Env-Var
   Q9_NET_DISCONNECT_CTRL (ein Buchstabe A-Z) uebersteuerbar, falls Ctrl-Q in einer Session gebraucht
   wird (z.B. als XON fuer ein OS-9-Programm mit eigener Flow-Control). */
static int g_net_disconnect_ctrl = 0x11;               /* Ctrl-Q (Default) */

static int q9_net_ctrl_from_env(const char *var, int fallback) {
    const char *s = getenv(var);
    char        c;
    if (!s || !s[0] || s[1] != '\0') return fallback;
    c = (char)toupper((unsigned char)s[0]);
    return (c >= 'A' && c <= 'Z') ? (c - 'A' + 1) : fallback;
}

/* Rueckgabe 1: byte_in ist echtes Nutzdatum (an OS-9 weiterreichen). Rueckgabe 0: Teil einer
   IAC-Sequenz, verschluckt -- naechstes Byte kommt im naechsten Poll-Durchlauf (recv liest ohnehin
   nur je 1 Byte pro Aufruf, s. q9_nettty_poll). */
static int telnet_filter_byte(os9_uart_t *ch, unsigned char byte_in) {
    switch (ch->telnet_state) {
    case 0:                                            /* ST_DATA */
        if (byte_in == Q9_TELNET_IAC) { ch->telnet_state = 1; return 0; }
        return 1;
    case 1:                                            /* ST_IAC: Kommandobyte erwartet */
        if (byte_in == Q9_TELNET_IAC) { ch->telnet_state = 0; return 1; }  /* IAC IAC = 0xFF-Nutzdatum */
        if (byte_in == Q9_TELNET_SB)  { ch->telnet_state = 3; return 0; }
        if (byte_in == Q9_TELNET_WILL || byte_in == Q9_TELNET_WONT ||
            byte_in == Q9_TELNET_DO   || byte_in == Q9_TELNET_DONT) {
            ch->telnet_state = 2;
            return 0;
        }
        ch->telnet_state = 0;                          /* 1-Byte-Kommando (NOP, GA, ...) */
        return 0;
    case 2:                                             /* ST_OPT: Optionsbyte von WILL/WONT/DO/DONT */
        ch->telnet_state = 0;
        return 0;
    case 3:                                             /* ST_SB: Subnegotiation, bis IAC ueberlesen */
        if (byte_in == Q9_TELNET_IAC) ch->telnet_state = 4;
        return 0;
    case 4:                                             /* ST_SB_IAC: IAC innerhalb SB gesehen */
        ch->telnet_state = (byte_in == Q9_TELNET_SE) ? 0 : 3;
        return 0;
    default:
        ch->telnet_state = 0;
        return 1;
    }
}

void q9_nettty_init(void) {
    struct sockaddr_in addr;
    int opt = 1;
    int listen_port = MAIN_LISTEN_PORT;
    const char *port_env = getenv("Q9_NETTTY_PORT");

    if (port_env != NULL && port_env[0] != '\0') {
        int parsed = atoi(port_env);
        if (parsed > 0 && parsed <= 65535) {
            listen_port = parsed;
        }
    }

    g_net_disconnect_ctrl = q9_net_ctrl_from_env("Q9_NET_DISCONNECT_CTRL", 0x11);

    main_server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (main_server_fd < 0) return;

    setsockopt(main_server_fd, SOL_SOCKET, SO_REUSEADDR, (const char *)&opt, sizeof(opt));
    Q9_SOCK_NONBLOCK(main_server_fd);

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons((uint16_t)listen_port);

    /* 2026-08-14 (beim Bau von test/09_test_io_dispatch.c gefunden, s. ARBEITSPLAN 5.18): bind()/
       listen() wurden bisher UNGEPRUEFT aufgerufen -- bei einem belegten Port (z.B. eine zweite
       q9.exe-Instanz oder ein eigener Testlauf auf demselben Port) blieb main_server_fd trotzdem
       ein gueltiger, aber NICHT lauschender Socket-Deskriptor, und die "gestartet"-Meldung log
       unveraendert weiter -- q9_nettty_poll()s spaetere accept()-Aufrufe (dort schon gegen
       main_server_fd>=0 abgesichert) liefen dann fuer immer sinnlos ins Leere, ohne dass das je
       sichtbar geworden waere. Jetzt: bei Fehlschlag Socket wieder schliessen UND main_server_fd
       auf -1 zuruecksetzen -- der bestehende main_server_fd>=0-Schutz in q9_nettty_poll() greift
       dann automatisch, keine weitere Aenderung dort noetig. */
    if (bind(main_server_fd, (struct sockaddr *)&addr, sizeof(addr)) != 0) {
        fprintf(stderr, "[OS-9 Net] WARNUNG: bind() auf Port %d fehlgeschlagen (Port belegt?) -- "
                         "Netz-Terminals x1-x8 bleiben in dieser Sitzung nicht erreichbar.\r\n",
                listen_port);
        Q9_SOCK_CLOSE(main_server_fd);
        main_server_fd = -1;
        return;
    }
    if (listen(main_server_fd, 5) != 0) {
        fprintf(stderr, "[OS-9 Net] WARNUNG: listen() auf Port %d fehlgeschlagen -- "
                         "Netz-Terminals x1-x8 bleiben in dieser Sitzung nicht erreichbar.\r\n",
                listen_port);
        Q9_SOCK_CLOSE(main_server_fd);
        main_server_fd = -1;
        return;
    }
    printf("[OS-9 Net] Multi-Terminal Server gestartet auf Mac-Port %d\r\n", listen_port);
}

void q9_nettty_poll(void) {
    if (main_server_fd < 0) return;

    int incoming = accept(main_server_fd, NULL, NULL);
    if (incoming >= 0) {
        Q9_SOCK_NONBLOCK(incoming);
        int assigned = 0;
        for (int i = 0; i < MAX_CHANNELS; i++) {
            if (channels[i].client_fd < 0) {
                channels[i].client_fd = incoming;
                channels[i].last_was_cr = 0;
                channels[i].telnet_state = 0;
                send(incoming, (const char *)g_telnet_negotiate, (int)sizeof(g_telnet_negotiate), 0);
                printf("[OS-9 Net] Gast dynamisch an /x%d uebergeben.\r\n", i + 1);
                assigned = 1;
                break;
            }
        }
        if (!assigned) {
            send(incoming, "OS-9: All lines busy.\r\n", 23, 0);
            Q9_SOCK_CLOSE(incoming);
        }
    }

    for (int i = 0; i < MAX_CHANNELS; i++) {
        if (channels[i].client_fd < 0) {
            continue;
        }

        /* 5.10: Solange das 1-Byte-Latch belegt ist, wird NICHT konsumiert — die Daten
           stauen sich im TCP-Puffer (Backpressure), statt verworfen zu werden (vorher
           gingen bei Burst-Eingabe auf mehreren Kanaelen Bytes verloren, z.B. 'super'
           -> 'sper' beim 8-Kanal-Login-Test). Der Verbindungsabbruch wird trotzdem
           erkannt: bei freiem Latch durch das normale read() (n==0), bei belegtem
           Latch durch ein nicht-konsumierendes recv(MSG_PEEK) — damit bleibt der
           CLOSE_WAIT-Bugfix vom 2026-07-10 wirksam. */
        unsigned char byte_in;
        int n;
        if (!(channels[i].status & 0x01)) {
            n = (int)recv(channels[i].client_fd, (char *)&byte_in, 1, 0);
            if (n == 1 && telnet_filter_byte(&channels[i], byte_in)) {
                if (byte_in == (unsigned char)g_net_disconnect_ctrl) {
                    /* Selbst-Trennen der eigenen Verbindung (Andreas' Wunsch, s. Kommentar bei
                       g_net_disconnect_ctrl oben) -- NUR dieser eine Kanal, Rest des Emulators
                       laeuft unbeeindruckt weiter. Verschluckt, landet nie bei OS-9. */
                    printf("[OS-9 Net] Gast von /x%d hat sich selbst getrennt (Ctrl-%c).\r\n",
                           i + 1, (char)('A' + g_net_disconnect_ctrl - 1));
                    Q9_SOCK_CLOSE(channels[i].client_fd);
                    channels[i].client_fd    = -1;
                    channels[i].status      &= ~0x01;
                    channels[i].last_was_cr  = 0;
                    channels[i].telnet_state = 0;
                    continue;
                }
                if (byte_in == '\n' && channels[i].last_was_cr) {
                    /* 5.16: Telnet-NVT-Normalisierung. Echte Telnet-Clients senden bei ENTER
                       CR+LF, OS-9 kennt als klassisches serielles System nur ein einzelnes CR
                       als Zeilenende. Ungefiltert landete das LF als erstes Byte im naechsten
                       Login-Prompt und wurde dort als nicht druckbares Zeichen ('.') sichtbar
                       und nicht mehr loeschbar (bestaetigt per Live-Test gegen Port 2000). */
                    channels[i].last_was_cr = 0;
                } else {
                    channels[i].last_was_cr = (byte_in == '\r');
                    channels[i].rx_data = byte_in;
                    channels[i].status |= 0x01;
                    raise_irq(channels[i].irq_level);
                }
            }
            /* n==1, aber telnet_filter_byte() hat 0 zurueckgegeben: Byte war Teil einer
               IAC-Sequenz, verschluckt -- n bleibt 1, loest den Disconnect-Check unten NICHT
               aus (der reagiert nur auf n==0/EOF oder n<0 mit echtem Socket-Fehler). */
        } else {
            n = (int)recv(channels[i].client_fd, (char *)&byte_in, 1, MSG_PEEK);
        }
        if (n == 0 || (n < 0 && !Q9_SOCK_WOULDBLOCK())) {
            Q9_SOCK_CLOSE(channels[i].client_fd);
            channels[i].client_fd = -1;
            channels[i].status &= ~0x01;
            channels[i].last_was_cr = 0;
            channels[i].telnet_state = 0;
            printf("[OS-9 Net] Gast von /x%d getrennt.\r\n", i + 1);
        }
    }

    /* 5.10: Leitung erneut anheben, falls noch irgendein Kanal ein unabgeholtes Byte hat —
       deckt den Fall ab, dass int_ack die gemeinsame Leitung global gesenkt hat, bevor alle
       anstehenden Kanaele bedient waren. Bewusst nur anheben, nie senken (das Senken passiert
       ausschliesslich beim Verbrauch in nettty_dev_read8, wie bisher). */
    for (int i = 0; i < MAX_CHANNELS; i++) {
        if (channels[i].status & 0x01) {
            raise_irq(channels[i].irq_level);
            break;
        }
    }
}

void q9_nettty_shutdown(void)
{
    int i;
    for (i = 0; i < MAX_CHANNELS; i++) {
        if (channels[i].client_fd >= 0) {
            Q9_SOCK_CLOSE(channels[i].client_fd);
        }
    }
    if (main_server_fd >= 0) {
        Q9_SOCK_CLOSE(main_server_fd);
    }
}

//────────────────────────────────────────────────────────────────────────────────────────────────
// Function: nettty_dev_* / q9_devtype_nettty
// Desc.:    2026-08-21: jetzt EIN devreg-Eintrag PRO KANAL (s. Kopfkommentar) -- dev->state zeigt
//           auf genau EIN os9_uart_t-Element (statt vorher NULL + Suche ueber alle acht). Dadurch
//           kein Adressvergleich mehr noetig: der Offset relativ zu dev->base entscheidet direkt.
//           Kein irq_vector_fn mehr noetig -- jeder Kanal traegt seinen (weiterhin festen) Vektor
//           direkt in dev->irq_vector.
//────────────────────────────────────────────────────────────────────────────────────────────────
static uint8_t nettty_dev_read8(q9_device_t *dev, uint32_t addr)
{
    os9_uart_t *ch = (os9_uart_t *)dev->state;
    uint32_t off = addr - dev->base;
    if (off == 0) {
        return ch->status;
    }
    if (off == 2) {
        ch->status &= ~0x01;                            // RX Ready löschen
        network_irq_resync();                           // Pin absenken -- oder oben halten, wenn
                                                          // ein anderer Kanal noch ein
                                                          // unabgeholtes Byte hat (5.10)
        return ch->rx_data;
    }
    return 0;
}

static void nettty_dev_write8(q9_device_t *dev, uint32_t addr, uint8_t val)
{
    os9_uart_t *ch = (os9_uart_t *)dev->state;
    uint32_t off = addr - dev->base;
    if (off == 4) {
        ch->tx_data = val;
        if (ch->client_fd >= 0) {
            send(ch->client_fd, (const char *)&ch->tx_data, 1, 0);
        }
        ch->status |= 0x02;                              // TX wieder leer/bereit
    }
}

static int nettty_dev_irq_pending(q9_device_t *dev)
{
    return ((os9_uart_t *)dev->state)->status & 0x01;
}

const q9_device_vtable_t q9_devtype_nettty = {
    .read8         = nettty_dev_read8,
    .write8        = nettty_dev_write8,
    .read16        = NULL,
    .write16       = NULL,
    .read32        = NULL,
    .write32       = NULL,
    .poll          = NULL,       /* q9_nettty_poll() laeuft weiterhin ueber q9_m68krt_execute */
    .irq_pending   = nettty_dev_irq_pending,
    .reset         = NULL,
    .irq_vector_fn = NULL,       /* 2026-08-21: statischer dev->irq_vector je Kanal reicht jetzt */
};

void q9_nettty_attach(void)
{
    int i;
    for (i = 0; i < MAX_CHANNELS; i++) {
        q9_device_t d;
        memset(&d, 0, sizeof(d));
        d.type       = "nettty";
        d.name       = channels[i].base_addr == Q9_BOARD_NET_X1_BASE ? "x1" :
                        channels[i].base_addr == Q9_BOARD_NET_X2_BASE ? "x2" :
                        channels[i].base_addr == Q9_BOARD_NET_X3_BASE ? "x3" :
                        channels[i].base_addr == Q9_BOARD_NET_X4_BASE ? "x4" :
                        channels[i].base_addr == Q9_BOARD_NET_X5_BASE ? "x5" :
                        channels[i].base_addr == Q9_BOARD_NET_X6_BASE ? "x6" :
                        channels[i].base_addr == Q9_BOARD_NET_X7_BASE ? "x7" : "x8";
        d.base       = channels[i].base_addr;
        d.size       = 256u;                              /* eigener Fast-Table-Slot je Kanal, s.
                                                               nettty.h-Kommentar bei den X1..X8_BASE */
        d.irq_level  = channels[i].irq_level;
        d.irq_vector = channels[i].irq_vector;
        d.level_held = 1;
        d.use_table  = 1;
        d.vt         = &q9_devtype_nettty;
        d.state      = &channels[i];
        q9_devreg_add(d);
    }
}

/* 2026-08-21 (Hardware-Vereinheitlichung, Folgeschritt nach dem "cf"-Piloten): q9_devdesc_nettty --
   noch OHNE extra_fields (kein Config-Schema, immer hartkodiert instanziiert, s. q9_nettty_attach()
   oben). use_table_default=1 (liegt im Fast-Table-Cluster). */
const q9_devdesc_t q9_devdesc_nettty = {
    .type              = "nettty",
    .desc              = "Netzwerk-Terminal (Telnet-NVT, ein Kanal von acht)",
    .vt                = &q9_devtype_nettty,
    .use_table_default = 1,
    .extra_fields      = NULL,
    .extra_field_count = 0,
};

//────────────────────────────────────────────────────────────────────────────────────────────────
// EOF nettty.c                                                                            Ver. 1.00
//────────────────────────────────────────────────────────────────────────────────────────────────
