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
//═════════╧══════╧════════════════════════════════════════════════════════════════════════╧══════
#include "m68krt.h"
#include "cb030.h"
#include "m68k.h"
#include <string.h>

/* Musashi haelt seinen CPU-Zustand in eigenen globalen Variablen und ruft m68k_read/write_memory_*
   ohne Kontext-Zeiger auf (s. m68krt.h) — deshalb muessen der aktive RAM-Block bzw. das aktive
   CB030-Board hier ebenfalls global liegen, statt im q9_m68krt_t-Handle. Nur EIN q9_m68krt_init()
   gleichzeitig aktiv. Ist g_board gesetzt (q9_m68krt_attach_board, 5.3), laufen ALLE Zugriffe
   ueber den CB030-Adress-Dispatch (RAM/ROM/Remap/UART/CF/Timer); sonst nackter RAM-Block (5.1). */
static uint8_t     *g_ram;
static uint32_t     g_ram_len;
static q9_cb030_t  *g_board;

unsigned int m68k_read_memory_8(unsigned int address)
{
    if (g_board) {
        return q9_cb030_read8(g_board, (uint32_t)address);
    }
    return (address < g_ram_len) ? g_ram[address] : 0;
}

unsigned int m68k_read_memory_16(unsigned int address)
{
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
    if (g_board && q9_cb030_uart_irq_pending(g_board)) {
        return g_board->uart_ivr;
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
    return Q9_M68KRT_OK;
}

void q9_m68krt_attach_board(q9_cb030_t *board)
{
    g_board = board;
    m68k_set_int_ack_callback(board ? m68krt_board_int_ack : 0);
}

void q9_m68krt_reset(q9_m68krt_t *rt)
{
    (void)rt;
    m68k_pulse_reset();
}

int q9_m68krt_execute(q9_m68krt_t *rt, int cycles)
{
    (void)rt;
    return m68k_execute(cycles);
}

uint32_t q9_m68krt_get_d(q9_m68krt_t *rt, int n)
{
    (void)rt;
    return m68k_get_reg(NULL, (m68k_register_t)(M68K_REG_D0 + n));
}

void q9_m68krt_free(q9_m68krt_t *rt)
{
    g_ram     = NULL;
    g_ram_len = 0;
    g_board   = NULL;
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

//────────────────────────────────────────────────────────────────────────────────────────────────
// EOF m68krt.c                                                                            Ver. 1.20
//────────────────────────────────────────────────────────────────────────────────────────────────
