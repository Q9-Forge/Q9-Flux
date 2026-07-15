#define _GNU_SOURCE
//════════════════════════════════════════════════════════════════════════════════════════════════
// File:   m68krt.c                                                                        Ver. 1.36
// Owner:  AF
// Desc.:  Implementierung des Musashi-Wrappers, siehe m68krt.h. Definiert die sechs Speicherzugriffs-
//         Funktionen, die Musashi vom Host verlangt (m68k_read/write_memory_8/16/32 — deklariert in
//         m68k.h, aber nicht implementiert, s. third_party/musashi/m68kconf.h M68K_SEPARATE_READS
//         OFF: read_immediate/read_pcrelative fallen intern auf dieselben sechs Funktionen zurueck,
//         separate Varianten werden fuer Q9 nicht gebraucht).
//
// Edition History
//─────────┬──────┬────────────────────────────────────────────────────────────────────────┬──────
// Date    │ Ver. │ Description                                                            │ By
//─────────┼──────┼────────────────────────────────────────────────────────────────────────┼──────
// 26-07-04│ 1.00 │ 5.1: Erster Grundbaustein                                               │ CF
// 26-07-04│ 1.10 │ 5.2d: q9_m68krt_set_irq (Wrapper um m68k_set_irq())                     │ CF
// 26-07-05│ 1.20 │ 5.3: q9_m68krt_attach_board — Speicher-Hooks koennen wahlweise ueber     │ CF
//         │      │ den CB030-Adress-Dispatch laufen (inkl. Autovector-Int-Ack)             │
// 26-07-14│ 1.30 │ 5.10: Netzwerk-Terminals 4 → 8 (/x1../x8), Kanaltabelle aus cb030.h      │ CF
//         │      │ hierher, network_irq_resync gegen verlorene Interrupts bei >1 Kanal      │
// 26-07-14│ 1.31 │ 5.17: Geraete-Registry (devreg.h) eingebunden -- 68681-DUART als erstes   │ CF
//         │      │ Geraet umgezogen (Dispatch + IACK/Reassert pruefen jetzt zuerst die       │
//         │      │ Registry); Netz-Terminals/QUICC/CF/Timer/RTC folgen einzeln               │
// 26-07-14│ 1.32 │ 5.17: Compact-Flash umgezogen (q9_devtype_cf, kein IRQ)                   │ CF
// 26-07-14│ 1.33 │ 5.17: Timer/IRQ3-Adress-Trigger umgezogen (q9_devtype_timer_irq)          │ CF
// 26-07-14│ 1.34 │ 5.17: RTC72421 umgezogen (q9_devtype_rtc72421) -- alle vier board-internen│ CF
//         │      │ Geraete jetzt in der Registry, nur noch nettty/QUICC hartkodiert           │
// 26-07-14│ 1.35 │ 5.17: Netz-Terminals /x1../x8 umgezogen (q9_devtype_nettty, EIN Geraet    │ CF
//         │      │ fuer alle acht Kanaele) -- channels[]-Fallback-Loops in IACK/Reassert       │
//         │      │ entfernt, nur noch QUICC hartkodiert                                        │
// 26-07-14│ 1.36 │ 5.17: QUICC umgezogen (q9_devtype_quicc, letztes von sechs Geraeten) --    │ CF
//         │      │ alle g_quicc-Sonderpruefungen in den sechs m68k_read/write_memory_*-        │
//         │      │ Funktionen sowie in IACK/Reassert entfernt; Timer/IRQ3 wird jetzt erst in   │
//         │      │ attach_quicc (nach QUICC) registriert, damit die Registrierungsreihenfolge  │
//         │      │ ueberall aufsteigend nach Level bleibt (3,4,5,6) -- s. dortige Kommentare    │
//═════════╧══════╧════════════════════════════════════════════════════════════════════════╧══════
#include "m68krt.h"
#include "cb030.h"
#include "quicc.h"
#include "devreg.h"
#include "m68k.h"
#include <string.h>
#include <unistd.h>  

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <errno.h>


/* Musashi haelt seinen CPU-Zustand in eigenen globalen Variablen und ruft m68k_read/write_memory_*
   ohne Kontext-Zeiger auf (s. m68krt.h) — deshalb muessen der aktive RAM-Block bzw. das aktive
   CB030-Board hier ebenfalls global liegen, statt im q9_m68krt_t-Handle. Nur EIN q9_m68krt_init()
   gleichzeitig aktiv. Ist g_board gesetzt (q9_m68krt_attach_board, 5.3), laufen ALLE Zugriffe
   ueber den CB030-Adress-Dispatch (RAM/ROM/Remap/UART/CF/Timer); sonst nackter RAM-Block (5.1). */
static uint8_t     *g_ram;
static uint32_t     g_ram_len;
static q9_cb030_t  *g_board;
static q9_quicc_t  *g_quicc;                          /* 5.11: QUICC-Ethernet, optional (attach) */

// === Forward-Deklarationen für den Netzwerk-Server ===
static void init_network_terminals(void);
static void update_network_terminals(void);
static unsigned char network_read8(unsigned int address);
static void network_write8(unsigned int address, unsigned char value);
static int main_server_fd = -1;

/* 5.10: 8 virtuelle Netzwerk-Terminals /x1../x8 (vorher 4x /t1../t4) — Registerlayout je Kanal
   s. cb030.h. Jeder Kanal hat seinen EIGENEN Autovektor (70..77), der IACK-Zyklus liefert genau
   den Vektor des Kanals mit gesetztem RX-Ready-Bit (s. m68krt_board_int_ack). */
static os9_uart_t channels[MAX_CHANNELS] = {
    {-1, 0, 0, 0x02, Q9_CB030_NET_X1_BASE, 4, 70, 0}, // /x1
    {-1, 0, 0, 0x02, Q9_CB030_NET_X2_BASE, 4, 71, 0}, // /x2
    {-1, 0, 0, 0x02, Q9_CB030_NET_X3_BASE, 4, 72, 0}, // /x3
    {-1, 0, 0, 0x02, Q9_CB030_NET_X4_BASE, 4, 73, 0}, // /x4
    {-1, 0, 0, 0x02, Q9_CB030_NET_X5_BASE, 4, 74, 0}, // /x5
    {-1, 0, 0, 0x02, Q9_CB030_NET_X6_BASE, 4, 75, 0}, // /x6
    {-1, 0, 0, 0x02, Q9_CB030_NET_X7_BASE, 4, 76, 0}, // /x7
    {-1, 0, 0, 0x02, Q9_CB030_NET_X8_BASE, 4, 77, 0}  // /x8
};

/* 5.10: Die IRQ-Leitung ist das ODER aller RX-Ready-Bits (level-getriggert). Nach jedem Verbrauch
   eines Bytes bzw. nach jedem globalen Absenken (int_ack) muss sie erneut angehoben werden, wenn
   IRGENDEIN anderer Kanal noch ein unabgeholtes Byte hat — sonst verliert der Kanal seinen
   Interrupt und bekommt erst beim NAECHSTEN Byte wieder einen (die im ARBEITSPLAN dokumentierte
   4-Kanal-Einschraenkung, vor dem 8-Kanal-Betrieb zu beheben). */
static void network_irq_resync(void) {
    for (int i = 0; i < MAX_CHANNELS; i++) {
        if (channels[i].status & 0x01) {
            m68k_set_irq((unsigned int)channels[i].irq_level);
            return;
        }
    }
    m68k_set_irq(0);
}


static void init_network_terminals(void) {
    struct sockaddr_in addr;
    int opt = 1;

    main_server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (main_server_fd < 0) return;

    setsockopt(main_server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    fcntl(main_server_fd, F_SETFL, O_NONBLOCK);

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(MAIN_LISTEN_PORT);

    bind(main_server_fd, (struct sockaddr *)&addr, sizeof(addr));
    listen(main_server_fd, 5);
    printf("[OS-9 Net] Multi-Terminal Server gestartet auf Mac-Port %d\n", MAIN_LISTEN_PORT);
}

static void update_network_terminals(void) {
    if (main_server_fd < 0) return;

    int incoming = accept(main_server_fd, NULL, NULL);
    if (incoming >= 0) {
        fcntl(incoming, F_SETFL, O_NONBLOCK);
        int assigned = 0;
        for (int i = 0; i < MAX_CHANNELS; i++) {
            if (channels[i].client_fd < 0) {
                channels[i].client_fd = incoming;
                channels[i].last_was_cr = 0;
                printf("[OS-9 Net] Gast dynamisch an /x%d uebergeben.\n", i + 1);
                assigned = 1;
                break;
            }
        }
        if (!assigned) {
            write(incoming, "OS-9: All lines busy.\r\n", 23);
            close(incoming);
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
            n = read(channels[i].client_fd, &byte_in, 1);
            if (n == 1) {
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
                    m68k_set_irq((unsigned int)channels[i].irq_level);
                }
            }
        } else {
            n = (int)recv(channels[i].client_fd, &byte_in, 1, MSG_PEEK);
        }
        if (n == 0 || (n < 0 && errno != EAGAIN && errno != EWOULDBLOCK)) {
            close(channels[i].client_fd);
            channels[i].client_fd = -1;
            channels[i].status &= ~0x01;
            channels[i].last_was_cr = 0;
            printf("[OS-9 Net] Gast von /x%d getrennt.\n", i + 1);
        }
    }

    /* 5.10: Leitung erneut anheben, falls noch irgendein Kanal ein unabgeholtes Byte hat —
       deckt den Fall ab, dass int_ack die gemeinsame Leitung global gesenkt hat, bevor alle
       anstehenden Kanaele bedient waren. Bewusst nur anheben, nie senken (das Senken passiert
       ausschliesslich beim Verbrauch in network_read8, wie bisher). */
    for (int i = 0; i < MAX_CHANNELS; i++) {
        if (channels[i].status & 0x01) {
            m68k_set_irq((unsigned int)channels[i].irq_level);
            break;
        }
    }
}


static unsigned char network_read8(unsigned int address) {
    for (int i = 0; i < MAX_CHANNELS; i++) {
        if (address == channels[i].base_addr) {
            return channels[i].status;
        }
        if (address == channels[i].base_addr + 2) {
            channels[i].status &= ~0x01; // RX Ready löschen
            network_irq_resync();        // Pin absenken — oder oben halten, wenn ein anderer
                                         // Kanal noch ein unabgeholtes Byte hat (5.10)
            return channels[i].rx_data;
        }
    }
    return 0;
}

static void network_write8(unsigned int address, unsigned char value) {
    for (int i = 0; i < MAX_CHANNELS; i++) {
        if (address == channels[i].base_addr + 4) {
            channels[i].tx_data = value;
            if (channels[i].client_fd >= 0) {
                write(channels[i].client_fd, &channels[i].tx_data, 1);
            }
            channels[i].status |= 0x02; // TX wieder leer/bereit
            break;
        }
    }
}

//────────────────────────────────────────────────────────────────────────────────────────────────
// Function: nettty_dev_* / q9_devtype_nettty
// Desc.:    5.17: Vtable-Adapter fuer die Geraete-Registry (devreg.h), fuenftes umgezogenes Geraet.
//           EIN einziges Geraet fuer alle acht Kanaele (statt acht Instanzen): read8/write8 delegieren
//           unveraendert an network_read8/network_write8, die selbst schon ueber ALLE Kanaele suchen
//           -- damit bleibt das bestehende Verhalten (inkl. des Randfalls einer 16/32-Bit-
//           Adresse, die rechnerisch die Grenze zwischen zwei Kanaelen ueberschreitet) exakt
//           erhalten, auch nach der generischen Byte-Synthese fuer read16/32 in devreg.c.
//           Die acht Kanaele TEILEN sich Level 4 (level-getriggertes ODER aller RX-Ready-Bits,
//           s. network_irq_resync) aber haben JEWEILS einen EIGENEN Vektor (70..77) -- deshalb
//           wie bei der DUART ein irq_vector_fn statt eines statischen dev->irq_vector:
//           nettty_dev_irq_vector liefert den Vektor des ERSTEN Kanals mit gesetztem RX-Ready-Bit,
//           exakt die Suchreihenfolge des alten hartkodierten channels[]-Loops in
//           m68krt_board_int_ack.
//────────────────────────────────────────────────────────────────────────────────────────────────
static uint8_t nettty_dev_read8(q9_device_t *dev, uint32_t addr)
{
    (void)dev;
    return network_read8(addr);
}

static void nettty_dev_write8(q9_device_t *dev, uint32_t addr, uint8_t val)
{
    (void)dev;
    network_write8(addr, val);
}

static int nettty_dev_irq_pending(q9_device_t *dev)
{
    (void)dev;
    for (int i = 0; i < MAX_CHANNELS; i++) {
        if (channels[i].status & 0x01) {
            return 1;
        }
    }
    return 0;
}

static int nettty_dev_irq_vector(q9_device_t *dev)
{
    (void)dev;
    for (int i = 0; i < MAX_CHANNELS; i++) {
        if (channels[i].status & 0x01) {
            return channels[i].irq_vector;
        }
    }
    return -1;
}

static const q9_device_vtable_t q9_devtype_nettty = {
    .read8         = nettty_dev_read8,
    .write8        = nettty_dev_write8,
    .read16        = NULL,
    .write16       = NULL,
    .read32        = NULL,
    .write32       = NULL,
    .poll          = NULL,       /* update_network_terminals() laeuft weiterhin ueber q9_m68krt_execute */
    .irq_pending   = nettty_dev_irq_pending,
    .reset         = NULL,
    .irq_vector_fn = nettty_dev_irq_vector,
};

/* 5.17: Geraete-Registry -- Geraete, die bereits umgezogen sind (s. devreg.h/cb030.h), werden HIER
   vor dem alten Board-Fallback geprueft; noch nicht migrierte Geraete (Netz-Terminals, QUICC, sowie
   innerhalb von q9_cb030_read8/write8: CF/Timer/RTC) bleiben bis zu ihrem eigenen 5.17-Schritt in
   den bisherigen, direkt danebenstehenden Pruefungen bzw. im Board-Fallback. */
static q9_device_t *devreg_hit(uint32_t address)
{
    int i, n = q9_devreg_count();
    for (i = 0; i < n; i++) {
        q9_device_t *d = q9_devreg_get(i);
        if (q9_device_hit(d, address)) {
            return d;
        }
    }
    return NULL;
}

unsigned int m68k_read_memory_8(unsigned int address)
{
    q9_device_t *dev;

    if ((dev = devreg_hit(address)) != NULL) {
        return q9_device_read8(dev, (uint32_t)address);
    }
    if (g_board) {
        return q9_cb030_read8(g_board, (uint32_t)address);
    }
    return (address < g_ram_len) ? g_ram[address] : 0;
}

unsigned int m68k_read_memory_16(unsigned int address)
{
    q9_device_t *dev;

    if ((dev = devreg_hit(address)) != NULL) {
        return q9_device_read16(dev, (uint32_t)address);
    }
    if (g_board) {
        return q9_cb030_read16(g_board, (uint32_t)address);
    }
    if (address + 1 >= g_ram_len) {
        return 0;
    }
    return ((unsigned int)g_ram[address] << 8) | g_ram[address + 1];
}

unsigned int m68k_read_memory_32(unsigned int address)
{
    q9_device_t *dev;

    if ((dev = devreg_hit(address)) != NULL) {
        return q9_device_read32(dev, (uint32_t)address);
    }
    if (g_board) {
        return q9_cb030_read32(g_board, (uint32_t)address);
    }
    if (address + 3 >= g_ram_len) {
        return 0;
    }
    return ((unsigned int)g_ram[address]     << 24) | ((unsigned int)g_ram[address + 1] << 16) |
           ((unsigned int)g_ram[address + 2] <<  8) |  (unsigned int)g_ram[address + 3];
}

void m68k_write_memory_8(unsigned int address, unsigned int value)
{
    q9_device_t *dev;

    if ((dev = devreg_hit(address)) != NULL) {
        q9_device_write8(dev, (uint32_t)address, (uint8_t)value);
        return;
    }
    if (g_board) {
        q9_cb030_write8(g_board, (uint32_t)address, (uint8_t)value);
        return;
    }
    if (address < g_ram_len) {
        g_ram[address] = (uint8_t)value;
    }
}

void m68k_write_memory_16(unsigned int address, unsigned int value)
{
    q9_device_t *dev;

    if ((dev = devreg_hit(address)) != NULL) {
        q9_device_write16(dev, (uint32_t)address, (uint16_t)value);
        return;
    }
    if (g_board) {
        q9_cb030_write16(g_board, (uint32_t)address, (uint16_t)value);
        return;
    }
    if (address + 1 >= g_ram_len) {
        return;
    }
    g_ram[address]     = (uint8_t)(value >> 8);
    g_ram[address + 1] = (uint8_t)value;
}

void m68k_write_memory_32(unsigned int address, unsigned int value)
{
    q9_device_t *dev;

    if ((dev = devreg_hit(address)) != NULL) {
        q9_device_write32(dev, (uint32_t)address, (uint32_t)value);
        return;
    }
    if (g_board) {
        q9_cb030_write32(g_board, (uint32_t)address, (uint32_t)value);
        return;
    }
    if (address + 3 >= g_ram_len) {
        return;
    }
    g_ram[address]     = (uint8_t)(value >> 24);
    g_ram[address + 1] = (uint8_t)(value >> 16);
    g_ram[address + 2] = (uint8_t)(value >>  8);
    g_ram[address + 3] = (uint8_t)value;
}

//────────────────────────────────────────────────────────────────────────────────────────────────
// Function: m68krt_board_int_ack
// Desc.:    5.3/5.4: Interrupt-Acknowledge im Board-Betrieb — die IRQ-Leitung wird beim Annehmen
//           des Interrupts wieder losgelassen (Puls-Verhalten, sonst wuerde der level-gehaltene
//           IRQ3 die CPU endlos erneut unterbrechen). Vektor-Auswahl wie beim echten Board:
//           Fordert die DUART gerade einen Interrupt an, legt sie ihren IVR-Inhalt auf den Bus
//           (vektorisierter 68681-IACK-Zyklus — der OS-9-Treiber sc68681 registriert seinen
//           Handler auf genau diesem Vektor, z.B. 0x50); sonst Autovector (Timer, Vektor 27 =
//           Autovektor Level 3).
//────────────────────────────────────────────────────────────────────────────────────────────────
static uint32_t g_ack_count;                          /* Diagnose: wie oft wurde IACK durchlaufen */

/* 5.15-Befund: echte Hardware haelt pro Geraet eine EIGENE IRQ-Leitung; quittiert die CPU
   das Level eines Geraets, senken NUR dessen eigene Leitung, alle anderen gleichzeitig
   anliegenden Anforderungen bleiben unberuehrt bestehen und der Prioritaets-Encoder praesentiert
   der CPU sofort wieder das naechsthoehere noch anstehende Level. Dieser Emulator bildet aber
   nur EINEN kombinierten `m68k_set_irq()`-Wert nach (kein Bus mit unabhaengigen Leitungen) -
   das blanke `m68k_set_irq(0)` unten wirft deshalb bislang ALLE gleichzeitig anstehenden
   Anforderungen weg, nicht nur die des gerade quittierten Geraets, und ueberlaesst die
   Wiederherstellung der naechsten Hauptschleifen-Runde (ganze CB030_SLICE_CYCLES spaeter).
   Traf ein SCC1-TXB-Event (Level 5) wiederholt mit dem 100Hz-Timer (Level 6, wird zuletzt
   gesetzt und ueberschreibt daher bewusst 3/5) zusammen, ging die QUICC-Anforderung dadurch
   in einer Weise "verloren", die kein reines Hardware-Aequivalent hat - vermutlicher Ausloeser
   des TCP-Haengers bei sptcp/telnetd nach ~15 Segmenten (der geschlossene Microware-Treiber
   zaehlt vermutlich ausstehende TXB-Bestaetigungen und verliert bei einer verschmolzenen/
   verzoegerten Zustellung die Spur). Fix: nach dem Zuruecksetzen sofort das naechsthoechste
   NOCH anstehende Level neu anlegen, statt bis zur naechsten Runde zu warten - macht die
   Ack-Behandlung analog zum Prioritaets-Encoder echter Hardware selbstheilend. */
/* 5.17: liefert das erste registrierte "level-held" Geraet (s. devreg.h), das GERADE einen IRQ
   anfordert -- Timer/IRQ3-Trigger sind bewusst NICHT level-held (level_held=0) und bleiben daher
   wie vor 5.17 aus dieser Pruefung aussen vor (einmaliger Puls je Runde, s. cb030run.c). */
static q9_device_t *devreg_pending_level_held(int filter_level)
{
    int i, n = q9_devreg_count();
    for (i = 0; i < n; i++) {
        q9_device_t *d = q9_devreg_get(i);
        if (!d->level_held) {
            continue;
        }
        if (filter_level >= 0 && d->irq_level != filter_level) {
            continue;
        }
        if (q9_device_irq_pending(d)) {
            return d;
        }
    }
    return NULL;
}

static void m68krt_reassert_pending_irq(void)
{
    q9_device_t *dev;

    if ((dev = devreg_pending_level_held(-1)) != NULL) {
        m68k_set_irq((unsigned int)dev->irq_level);
        return;
    }
}

static int m68krt_board_int_ack(int int_level)
{
    int vector = M68K_INT_ACK_AUTOVECTOR;
    q9_device_t *dev;

    g_ack_count++;
    m68k_set_irq(0);
    if ((dev = devreg_pending_level_held(int_level)) != NULL) {
        int v = q9_device_irq_vector(dev);
        vector = (v >= 0) ? v : M68K_INT_ACK_AUTOVECTOR;
    }
    /* sonst: Autovektor (z.B. Level 6/Timer, das laut devreg-Eintrag level_held=0 hat und     */
    /* deshalb nie hier landet, s. cb030.c timer_dev_*-Kommentar).                             */

    m68krt_reassert_pending_irq();                     /* 5.15: sofort statt erst naechste Runde  */
    return vector;
}


/* 5.15/Option B (2026-07-14): optionaler Trap#0-Trace fuer die telnetdc-Ev$Wait-Untersuchung
   (docs/re_telnetdc/OPTION_B_STATUS.md) -- nur aktiv, wenn die Env-Var Q9_TRAP_TRACE gesetzt ist,
   sonst No-Op (kein Effekt auf normale Laeufe/Performance).
   WICHTIG: trap #0 ist der Direktaufruf-Trap fuer ALLE F$/I$-Syscalls im GESAMTEN System (jeder
   Prozess, staendig). Testlauf 1 (2026-07-14) mit ungefiltertem, zeilengepuffertem Log bremste
   den Boot so massiv aus, dass er wie haengengeblieben wirkte -- Ursache war das Zeilenpuffer-
   Flushing (ein write()-Syscall pro Trap0), nicht das Tracing an sich. Testlauf 2 mit einem
   Register-Filter (D1==4, D2==0x7fff) ergab NULL Treffer -- die vermutete Registerbelegung war
   falsch. Deshalb jetzt: KEIN Filter mehr (alles loggen), aber gross gepuffert (_IOFBF, 4 MiB)
   statt zeilenweise geflusht -- vermeidet den Testlauf-1-Bremseffekt, ohne auf Verdacht zu
   filtern. Loggt PC (= Adresse der trap-Instruktion, ueber M68K_REG_PPC) + D0..D3/A0/A1;
   zusaetzlich ein Cap (Q9_TRAP_TRACE_CAP, Default 2 Mio. Zeilen) als Sicherheitsnetz gegen
   unbegrenztes Log-Wachstum bei einer sehr langen Session. */
static FILE   *g_trap_trace_fp  = NULL;
static long    g_trap_trace_cap = 2000000;
static long    g_trap_trace_n   = 0;
static uint32_t g_watch_pc  = 0;
static uint32_t g_watch_pc2 = 0;
static int      g_telnetdc_base_known  = 0;
static int      g_watch_manual         = 0;   /* per Q9_WATCH_PC/2 vorgegeben -- Autoerkennung bleibt aus */
static uint32_t g_watch_candidate_base   = 0;
static int      g_watch_candidate_streak = 0;
static uint32_t g_event_return_pc = 0;    /* pkdvr-Diagnose (2026-07-15): naechste Instruktion nach
   einem geloggten F$Event-Trap -- einzelner globaler Slot genuegt, da der Emulator nur einen
   CPU-Kern hat und die Rueckkehr-Instruktion garantiert die naechste ausgefuehrte ist, bevor
   irgendein anderer Trap dazwischenkommen kann. */

/* Testlauf 3 (2026-07-14) zeigte: D0/D1 allein liefern keine brauchbare Filterung -- Grund
   (Ghidra-Nachanalyse): der eigentliche OS-9-Aufrufcode (F$Event = 0x53) steckt NICHT in einem
   Register, sondern klassisch OS-9-typisch als INLINE-DATENWORT direkt hinter der trap#0-
   Instruktion im Code (Kernel liest es beim Rueckkehren und ueberspringt es). Deshalb jetzt
   gezielt genau dieses Wort mitlesen (m68k_read_disassembler_16, seiteneffektfrei) und nur bei
   Treffer F$Event (0x53) loggen -- das ist system-weit selten genug fuer ein sauberes Signal. */
/* Testlauf 7 (2026-07-15): die Ladeadresse von telnetdc variiert JE BOOT (nicht stabil, wie
   zunaechst per manuellem "l telnetdc"/.r7-Test angenommen) -- ein vorab von aussen uebergebenes
   Q9_WATCH_PC passt daher oft nicht mehr. Deshalb jetzt Autoerkennung: der ERSTE F$Sleep-Aufruf
   (callcode=0x0a) im telnetdc-Adressraum verraet die Basis (PC - 0xf4e, statischer Offset unseres
   eigenen Patches bei telnetdc-Offset 0xf4c/0xf4e), daraus werden die beiden GetStt-Watchpoints
   (Erfolg 0x32a8, Fehler 0x32b2) automatisch abgeleitet -- kein manuelles Ausrechnen/Uebergeben
   mehr noetig, funktioniert bootuebergreifend automatisch. */

/* pkdvr-Diagnose (2026-07-15): Modulname am Zeiger `addr` OS-9-typisch lesen (letztes Zeichen
   hat Bit 7 gesetzt, kein NUL-Terminator im Speicher) -- fuer F$Link/F$Load-Namen (A0). */
static void m68krt_read_os9_name(uint32_t addr, char *out, int outsz)
{
    int i = 0;
    while (i < outsz - 1) {
        uint8_t b = (uint8_t)m68k_read_memory_8(addr + i);
        char c = (char)(b & 0x7F);
        if (c == 0) {
            break;
        }
        out[i++] = c;
        if (b & 0x80) {
            break;
        }
    }
    out[i] = '\0';
}

static int m68krt_trap_trace_callback(int trap)
{
    if (trap == 0 && g_trap_trace_fp && g_trap_trace_n < g_trap_trace_cap) {
        uint32_t pc = m68k_get_reg(NULL, M68K_REG_PPC);
        uint32_t callcode = m68k_read_memory_16(pc + 2);
        /* pkdvr-Diagnose (2026-07-15): F$Link(0x00)/F$Load(0x01) zusaetzlich zu F$Event mit-
           verfolgen -- Ziel: klaeren, ob "pkdvr" mehrfach separat gelinkt/geladen wird (Verdacht
           nach der Basis-Analyse: mehrere gleichzeitig aktive Kopien im Speicher, s.
           docs/re_telnetdc/OPTION_B_STATUS.md). A0 = Modulname bei beiden Aufrufen. */
        if (callcode == 0x00 || callcode == 0x01) {
            char name[16];
            m68krt_read_os9_name(m68k_get_reg(NULL, M68K_REG_A0), name, sizeof(name));
            if (name[0] == 'p' && name[1] == 'k') {
                fprintf(g_trap_trace_fp, "linkname pc=%08x callcode=%04x name=%s\n", pc, callcode, name);
                g_trap_trace_n++;
                g_event_return_pc = pc + 4;    /* Rueckgabe (D0/D1/A0-A2) im Watch-Callback mitloggen */
            }
            return 0;
        }
        if (callcode == 0x53 || callcode == 0x0a || callcode == 0x8d) {
            fprintf(g_trap_trace_fp,
                    "trap0 pc=%08x callcode=%04x d0=%08x d1=%08x d2=%08x d3=%08x a0=%08x a1=%08x\n",
                    pc, callcode,
                    m68k_get_reg(NULL, M68K_REG_D0), m68k_get_reg(NULL, M68K_REG_D1),
                    m68k_get_reg(NULL, M68K_REG_D2), m68k_get_reg(NULL, M68K_REG_D3),
                    m68k_get_reg(NULL, M68K_REG_A0), m68k_get_reg(NULL, M68K_REG_A1));
            g_trap_trace_n++;
            if (callcode == 0x53) {
                /* pkdvr-Diagnose (2026-07-15): Rueckgabewert (D0=Fehlercode, D1=Ergebnis/
                   gelesener Event-Wert) mitschneiden -- noetig, um z.B. Ev$Read-Werte oder
                   ob ein Ev$Wait ueberhaupt zurueckkehrt, im Log zu sehen. trap#0 + Inline-
                   Wort sind zusammen 4 Byte lang, die Rueckkehradresse ist also PC+4. */
                g_event_return_pc = pc + 4;
            }
            /* Testlauf 10 zeigte: ein simples "erste Instanz gewinnt" verriegelt sich auf eine
               fruehe, irrelevante Instanz (z.B. vom Login-Prozess), waehrend die eigentlich
               relevante (dominante, dauerhaft aktive) Instanz nie erfasst wird. Deshalb jetzt
               "3x hintereinander dieselbe Basis" als Kriterium -- fegt kurzlebige Strays weg,
               folgt aber zuverlaessig der aktuell aktiven Schleife (die F$Sleep sehr oft in
               Folge aufruft), auch wenn diese erst spaeter im Boot/Verbindungsverlauf startet. */
            if (callcode == 0x0a && !g_watch_manual) {
                uint32_t base = pc - 0xf4e;
                if (base == g_watch_candidate_base) {
                    g_watch_candidate_streak++;
                } else {
                    g_watch_candidate_base   = base;
                    g_watch_candidate_streak = 1;
                }
                if (g_watch_candidate_streak == 3 && g_watch_pc != base + 0x32a8) {
                    g_watch_pc  = base + 0x32a8;
                    g_watch_pc2 = base + 0x32b2;
                    g_telnetdc_base_known = 1;
                    fprintf(g_trap_trace_fp,
                            "auto-base telnetdc=%08x watch1=%08x watch2=%08x\n",
                            base, g_watch_pc, g_watch_pc2);
                }
            }
        }
    }
    return 0;                                         /* nicht behandelt -- normale Exception laeuft weiter */
}

/* Testlauf 4 (2026-07-15): der Trap#0-Callback feuert nur VOR dem Trap (Eingaberegister), nicht
   beim Rueckkehren -- der Rueckgabewert von I$GetStt/SS_Ready (in D1, s. Disassemblierung des
   Wrapers bei telnetdc-Offset 0x32a8) blieb dadurch unsichtbar. Deshalb zusaetzlich ein gezielter
   PC-Watchpoint ueber M68K_INSTRUCTION_HOOK: feuert vor JEDER Instruktion (teuer, aber der
   Vergleich selbst ist trivial und es wird nur bei echtem Treffer geloggt/geflusht -- anders als
   der frueher verworfene ungefilterte Trap-Log-Versuch, der pro Zeile schrieb). Adresse kommt aus
   Q9_WATCH_PC (Hex, ohne 0x-Praefix), da die Ladeadresse je Boot variiert. */

static void m68krt_watch_pc_callback(unsigned int pc)
{
    if (g_trap_trace_fp && g_trap_trace_n < g_trap_trace_cap &&
        ((g_watch_pc && pc == g_watch_pc) || (g_watch_pc2 && pc == g_watch_pc2))) {
        fprintf(g_trap_trace_fp,
                "watch pc=%08x d0=%08x d1=%08x d2=%08x d3=%08x a0=%08x a1=%08x\n",
                pc,
                m68k_get_reg(NULL, M68K_REG_D0), m68k_get_reg(NULL, M68K_REG_D1),
                m68k_get_reg(NULL, M68K_REG_D2), m68k_get_reg(NULL, M68K_REG_D3),
                m68k_get_reg(NULL, M68K_REG_A0), m68k_get_reg(NULL, M68K_REG_A1));
        g_trap_trace_n++;
    }
    /* pkdvr-Diagnose (2026-07-15): Rueckkehrpunkt eines zuvor geloggten F$Event-Traps erreicht --
       D0/D1 jetzt sind der Rueckgabewert (D0=0 Erfolg/sonst Fehlercode, D1=Ergebnis bei
       Ev$Read/Ev$Wait-Erfolg). Sofort konsumieren (auf 0 setzen), damit kein spaeterer,
       zufaelliger Treffer derselben Adresse (z.B. Schleifenrunde) faelschlich mitgeloggt wird. */
    if (g_event_return_pc && pc == g_event_return_pc && g_trap_trace_fp &&
        g_trap_trace_n < g_trap_trace_cap) {
        /* A0-A2 mitgeloggt fuer die F$Link/F$Load-Diagnose (2026-07-15): bei Erfolg liefert
           F$Link/F$Load die Modulbasis typischerweise in A2 -- damit laesst sich jeder Link-
           Aufruf direkt der resultierenden Ladeadresse zuordnen. */
        fprintf(g_trap_trace_fp,
                "eventret pc=%08x d0=%08x d1=%08x a0=%08x a1=%08x a2=%08x\n",
                pc, m68k_get_reg(NULL, M68K_REG_D0), m68k_get_reg(NULL, M68K_REG_D1),
                m68k_get_reg(NULL, M68K_REG_A0), m68k_get_reg(NULL, M68K_REG_A1),
                m68k_get_reg(NULL, M68K_REG_A2));
        g_trap_trace_n++;
        g_event_return_pc = 0;
    }
}

int q9_m68krt_init(q9_m68krt_t *rt, uint8_t *ram, uint32_t ram_len)
{
    if (!ram || ram_len < 8) {
        return Q9_M68KRT_ERR_RAM;
    }

    memset(rt, 0, sizeof(*rt));
    rt->ram     = ram;
    rt->ram_len = ram_len;
    g_ram       = ram;
    g_ram_len   = ram_len;
    g_board     = 0;                                  /* RAM-Modus, bis attach_board (5.3) folgt */
    q9_devreg_clear();                                /* 5.17: frische Geraete-Registry je Boot  */

    m68k_set_cpu_type(M68K_CPU_TYPE_68030);
    m68k_init();
    m68k_set_int_ack_callback(0);

    {
        const char *trace_path = getenv("Q9_TRAP_TRACE");
        if (trace_path && !g_trap_trace_fp) {
            g_trap_trace_fp = fopen(trace_path, "w");
            if (g_trap_trace_fp) {
                setvbuf(g_trap_trace_fp, NULL, _IOFBF, 4 * 1024 * 1024);
                m68k_set_trap_instr_callback(m68krt_trap_trace_callback);
            }
        }
        const char *watch_pc_str  = getenv("Q9_WATCH_PC");
        const char *watch_pc2_str = getenv("Q9_WATCH_PC2");
        if (watch_pc_str || watch_pc2_str) {
            if (watch_pc_str)  g_watch_pc  = (uint32_t)strtoul(watch_pc_str, NULL, 16);
            if (watch_pc2_str) g_watch_pc2 = (uint32_t)strtoul(watch_pc2_str, NULL, 16);
            g_watch_manual = 1;                     /* manuell vorgegeben -- Autoerkennung bleibt komplett aus */
        }
        if (g_trap_trace_fp) {
            m68k_set_instr_hook_callback(m68krt_watch_pc_callback);   /* auch ohne Vorgabe: Autoerkennung */
        }
    }

    // === NEU: Netzwerk-Server beim Start hochfahren ===
    init_network_terminals();

    return Q9_M68KRT_OK;
}



void q9_m68krt_attach_board(q9_cb030_t *board)
{
    g_board = board;
    m68k_set_int_ack_callback(board ? m68krt_board_int_ack : 0);

    /* 5.17: Geraete in die Registry eintragen -- DUART/CF/Timer/RTC (board-intern) + Netz-
       Terminals (unabhaengig vom Board, aber nur beim echten Boot gebraucht, s.u.). Nur QUICC
       bleibt noch hartkodiert (eigener 5.17-Schritt). WICHTIG: die Netz-Terminals (Level 4)
       werden bewusst NACH der DUART (Level 3) registriert -- cb030run.c's erster Poll-Durchgang
       (Level < 5, filtert Timer/Level 6 per Levelvergleich weg, s. dort) durchlaeuft die Registry
       in dieser Reihenfolge, und q9_m68krt_set_irq() bildet nur EINE kombinierte Leitung nach
       (letzter Aufruf gewinnt) -- die Level-3-vor-Level-4-Reihenfolge muss darum erhalten bleiben. */
    if (board) {
        q9_device_t d;
        memset(&d, 0, sizeof(d));
        d.type       = "duart68681";
        d.name       = "uart0";
        d.base       = Q9_CB030_UART_BASE;
        d.size       = Q9_CB030_UART_TOP - Q9_CB030_UART_BASE + 1u;
        d.irq_level  = 3;
        d.irq_vector = -1;                            /* dynamisch, s. irq_vector_fn            */
        d.level_held = 1;
        d.vt         = &q9_devtype_duart68681;
        d.state      = board;
        q9_devreg_add(d);

        /* 5.17: Compact-Flash, zweites umgezogenes Geraet -- kein IRQ (level_held bleibt 0,
           irq_vector -1/unbenutzt: q9_devtype_cf setzt keinen irq_pending). */
        memset(&d, 0, sizeof(d));
        d.type       = "cf";
        d.name       = "cf0";
        d.base       = Q9_CB030_CF_BASE;
        d.size       = Q9_CB030_CF_TOP - Q9_CB030_CF_BASE + 1u;
        d.irq_level  = 0;
        d.irq_vector = -1;
        d.level_held = 0;
        d.vt         = &q9_devtype_cf;
        d.state      = board;
        q9_devreg_add(d);

        /* 5.17: RTC72421, viertes board-internes Geraet -- kein IRQ. (Timer/IRQ3 wird bewusst
           NICHT hier, sondern erst in q9_m68krt_attach_quicc registriert -- s. dort, Grund ist
           Registrierungsreihenfolge/IRQ-Prioritaet, Level 6 muss NACH QUICC/Level 5 kommen.) */
        memset(&d, 0, sizeof(d));
        d.type       = "rtc72421";
        d.name       = "rtc0";
        d.base       = Q9_CB030_RTC_BASE;
        d.size       = Q9_CB030_RTC_TOP - Q9_CB030_RTC_BASE + 1u;
        d.irq_level  = 0;
        d.irq_vector = -1;
        d.level_held = 0;
        d.vt         = &q9_devtype_rtc72421;
        d.state      = board;
        q9_devreg_add(d);

        /* 5.17: Netz-Terminals, fuenftes umgezogenes Geraet -- EIN Geraete-Eintrag fuer alle acht
           Kanaele (s. q9_devtype_nettty-Kommentar weiter oben). Registriert NACH DUART (Level 3),
           damit die Registrierungsreihenfolge innerhalb von cb030run.c's erstem Poll-Durchgang
           (Level < 5) aufsteigend bleibt (3 vor 4) -- wichtig fuer die "letzter Aufruf gewinnt"-
           Semantik von q9_m68krt_set_irq(), s. Kommentar dort. channels[] existiert zwar auch ohne
           Board, aber nettty wird nur beim echten Boot (immer MIT Board) gebraucht. */
        memset(&d, 0, sizeof(d));
        d.type       = "nettty";
        d.name       = "x1-x8";
        d.base       = Q9_CB030_NET_BASE;
        d.size       = Q9_CB030_NET_TOP - Q9_CB030_NET_BASE + 1u;
        d.irq_level  = 4;
        d.irq_vector = -1;                            /* dynamisch, s. irq_vector_fn (pro Kanal) */
        d.level_held = 1;
        d.vt         = &q9_devtype_nettty;
        d.state      = NULL;                          /* nutzt das globale channels[]-Array      */
        q9_devreg_add(d);
    }
}

void q9_m68krt_attach_quicc(q9_quicc_t *quicc)
{
    g_quicc = quicc;                                  /* 5.11: ab jetzt dekodiert das QUICC-     */
                                                       /* Fenster $FFFF2000-$FFFF3FFF             */

    /* 5.17: QUICC, sechstes und letztes umgezogenes Geraet -- fester Vektor 254 (kein
       irq_vector_fn noetig, anders als DUART/nettty). Registriert NACH den Netz-Terminals
       (Level 4 vor Level 5) -- s. q9_devtype_quicc-Kommentar in quicc.c und die Reihenfolge-
       Erklaerung in q9_m68krt_attach_board.
       Timer/IRQ3 (Level 6, viertes 5.17-Geraet inhaltlich, aber ERST HIER registriert statt in
       attach_board) folgt bewusst GANZ ZULETZT: Level 6 muss in der Registrierungsreihenfolge
       NACH QUICC/Level 5 stehen, damit cb030run.c's Poll-Schleife (Level < 5 vor QUICC, Level
       >= 5 danach) und m68krt_board_int_ack/reassert_pending_irq (durchlaufen die GESAMTE
       Registry in Registrierungsreihenfolge) am Ende bei gleichzeitig anstehenden Interrupts
       konsistent das hoechste Level uebrig lassen -- exakt das Verhalten, das vor 5.17 durch die
       hartkodierte Aufrufreihenfolge (DUART 3, QUICC 5, Timer 6) in cb030run.c sichergestellt war. */
    if (quicc && g_board) {
        q9_device_t d;
        memset(&d, 0, sizeof(d));
        d.type       = "quicc";
        d.name       = "enet0";
        d.base       = Q9_QUICC_BASE;
        d.size       = Q9_QUICC_TOP - Q9_QUICC_BASE + 1u;
        d.irq_level  = Q9_QUICC_IRQ_LEVEL;
        d.irq_vector = Q9_QUICC_IRQ_VECTOR;            /* fest, s. q9_devtype_quicc-Kommentar    */
        d.level_held = 1;
        d.vt         = &q9_devtype_quicc;
        d.state      = quicc;
        q9_devreg_add(d);

        memset(&d, 0, sizeof(d));
        d.type       = "timer_irq";
        d.name       = "tirq0";
        d.base       = Q9_CB030_TIRQ_OFF_BASE;
        d.size       = Q9_CB030_TIRQ_ON_TOP - Q9_CB030_TIRQ_OFF_BASE + 1u;
        d.irq_level  = 6;
        d.irq_vector = -1;
        d.level_held = 0;
        d.vt         = &q9_devtype_timer_irq;
        d.state      = g_board;
        q9_devreg_add(d);
    }
}

void q9_m68krt_reset(q9_m68krt_t *rt)
{
    (void)rt;
    m68k_pulse_reset();
}

int q9_m68krt_execute(q9_m68krt_t *rt, int cycles)
{
    (void)rt;
    update_network_terminals();
    return m68k_execute(cycles);
}

uint32_t q9_m68krt_get_d(q9_m68krt_t *rt, int n)
{
    (void)rt;
    return m68k_get_reg(NULL, (m68k_register_t)(M68K_REG_D0 + n));
}

void q9_m68krt_free(q9_m68krt_t *rt)
{
    
    for (int i = 0; i < MAX_CHANNELS; i++) {
        if (channels[i].client_fd >= 0) close(channels[i].client_fd);
    }
    if (main_server_fd >= 0) close(main_server_fd);
    
    g_ram     = NULL;
    g_ram_len = 0;
    g_board   = NULL;
    g_quicc   = NULL;
    m68k_set_int_ack_callback(0);
    memset(rt, 0, sizeof(*rt));
}

void q9_m68krt_set_irq(int level)
{
    m68k_set_irq((unsigned int)level);
}

void q9_m68krt_debug_state(uint32_t *pc, uint32_t *sr, uint32_t *acks)
{
    *pc   = m68k_get_reg(NULL, M68K_REG_PC);
    *sr   = m68k_get_reg(NULL, M68K_REG_SR);
    *acks = g_ack_count;
}

int q9_m68krt_is_stopped(void)
{
    return m68k_is_stopped();
}

//────────────────────────────────────────────────────────────────────────────────────────────────
// EOF m68krt.c                                                                            Ver. 1.36
//────────────────────────────────────────────────────────────────────────────────────────────────
