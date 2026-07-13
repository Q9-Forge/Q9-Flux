#define _GNU_SOURCE
//════════════════════════════════════════════════════════════════════════════════════════════════
// File:   m68krt.c                                                                        Ver. 1.20
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
//═════════╧══════╧════════════════════════════════════════════════════════════════════════╧══════
#include "m68krt.h"
#include "cb030.h"
#include "quicc.h"
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
    {-1, 0, 0, 0x02, Q9_CB030_NET_X1_BASE, 4, 70}, // /x1
    {-1, 0, 0, 0x02, Q9_CB030_NET_X2_BASE, 4, 71}, // /x2
    {-1, 0, 0, 0x02, Q9_CB030_NET_X3_BASE, 4, 72}, // /x3
    {-1, 0, 0, 0x02, Q9_CB030_NET_X4_BASE, 4, 73}, // /x4
    {-1, 0, 0, 0x02, Q9_CB030_NET_X5_BASE, 4, 74}, // /x5
    {-1, 0, 0, 0x02, Q9_CB030_NET_X6_BASE, 4, 75}, // /x6
    {-1, 0, 0, 0x02, Q9_CB030_NET_X7_BASE, 4, 76}, // /x7
    {-1, 0, 0, 0x02, Q9_CB030_NET_X8_BASE, 4, 77}  // /x8
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

        /* read() laeuft IMMER, unabhaengig vom RX-Ready-Status — sonst bleibt jedes Byte,
           das ankommt waehrend OS-9 das vorherige noch nicht abgeholt hat, fuer immer
           ungelesen im Socket-Puffer stehen, und ein Verbindungsabbruch (n==0) wird nie
           erkannt (Socket blieb bisher dauerhaft in CLOSE_WAIT haengen). Ist das Register
           noch belegt, wird das neu gelesene Byte bewusst verworfen (Overrun, wie bei einer
           echten UART ohne FIFO) statt das wartende Byte zu ueberschreiben. */
        unsigned char byte_in;
        int n = read(channels[i].client_fd, &byte_in, 1);
        if (n == 1) {
            if (!(channels[i].status & 0x01)) {
                channels[i].rx_data = byte_in;
                channels[i].status |= 0x01;
                m68k_set_irq((unsigned int)channels[i].irq_level);
            }
        } else if (n == 0 || (n < 0 && errno != EAGAIN && errno != EWOULDBLOCK)) {
            close(channels[i].client_fd);
            channels[i].client_fd = -1;
            channels[i].status &= ~0x01;
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



unsigned int m68k_read_memory_8(unsigned int address)
{
    
      if (address >= Q9_CB030_NET_BASE && address <= Q9_CB030_NET_TOP) {
        return network_read8(address);
    }
    if (g_quicc && q9_quicc_hit(address)) {
        return q9_quicc_read8(g_quicc, (uint32_t)address);
    }
    
    if (g_board) {
        return q9_cb030_read8(g_board, (uint32_t)address);
    }
    return (address < g_ram_len) ? g_ram[address] : 0;
}

unsigned int m68k_read_memory_16(unsigned int address)
{
    
    if (address >= Q9_CB030_NET_BASE && address <= Q9_CB030_NET_TOP) {
        return (network_read8(address) << 8) | network_read8(address + 1);
    }
    if (g_quicc && q9_quicc_hit(address)) {
        return q9_quicc_read16(g_quicc, (uint32_t)address);
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
    
    if (address >= Q9_CB030_NET_BASE && address <= Q9_CB030_NET_TOP) {
        return (network_read8(address) << 24) | (network_read8(address + 1) << 16) |
               (network_read8(address + 2) << 8)  | network_read8(address + 3);
    }
    if (g_quicc && q9_quicc_hit(address)) {
        return q9_quicc_read32(g_quicc, (uint32_t)address);
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
    
    if (address >= Q9_CB030_NET_BASE && address <= Q9_CB030_NET_TOP) {
        network_write8(address, (unsigned char)value);
        return;
    }
    if (g_quicc && q9_quicc_hit(address)) {
        q9_quicc_write8(g_quicc, (uint32_t)address, (uint8_t)value);
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
    
    if (address >= Q9_CB030_NET_BASE && address <= Q9_CB030_NET_TOP) {
        network_write8(address, (unsigned char)(value >> 8));
        network_write8(address + 1, (unsigned char)value);
        return;
    }
    if (g_quicc && q9_quicc_hit(address)) {
        q9_quicc_write16(g_quicc, (uint32_t)address, (uint16_t)value);
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
    
    if (address >= Q9_CB030_NET_BASE && address <= Q9_CB030_NET_TOP) {
        network_write8(address, (unsigned char)(value >> 24));
        network_write8(address + 1, (unsigned char)(value >> 16));
        network_write8(address + 2, (unsigned char)(value >> 8));
        network_write8(address + 3, (unsigned char)value);
        return;
    }
    if (g_quicc && q9_quicc_hit(address)) {
        q9_quicc_write32(g_quicc, (uint32_t)address, (uint32_t)value);
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

static int m68krt_board_int_ack(int int_level)
{
    (void)int_level;
    g_ack_count++;
    m68k_set_irq(0);
    if (g_quicc && int_level == Q9_QUICC_IRQ_LEVEL && q9_quicc_irq_pending(g_quicc)) {
        return Q9_QUICC_IRQ_VECTOR;                   /* 5.11: SCC1-Ethernet, vektorisiert       */
    }
    if (g_board && q9_cb030_uart_irq_pending(g_board)) {
        return g_board->uart_ivr;
    }
    
    for (int i = 0; i < MAX_CHANNELS; i++) {
    if ((channels[i].status & 0x01) && int_level == channels[i].irq_level) {
        return channels[i].irq_vector; 
    }
}

    
    return M68K_INT_ACK_AUTOVECTOR;
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

    m68k_set_cpu_type(M68K_CPU_TYPE_68030);
    m68k_init();
    m68k_set_int_ack_callback(0);

    // === NEU: Netzwerk-Server beim Start hochfahren ===
    init_network_terminals();

    return Q9_M68KRT_OK;
}



void q9_m68krt_attach_board(q9_cb030_t *board)
{
    g_board = board;
    m68k_set_int_ack_callback(board ? m68krt_board_int_ack : 0);
}

void q9_m68krt_attach_quicc(q9_quicc_t *quicc)
{
    g_quicc = quicc;                                  /* 5.11: ab jetzt dekodiert das QUICC-     */
}                                                     /* Fenster $FFFF2000-$FFFF3FFF             */

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
// EOF m68krt.c                                                                            Ver. 1.20
//────────────────────────────────────────────────────────────────────────────────────────────────
