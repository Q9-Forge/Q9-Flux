//════════════════════════════════════════════════════════════════════════════════════════════════
// File:   m68krt.c                                                                        Ver. 1.00
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
//═════════╧══════╧════════════════════════════════════════════════════════════════════════╧══════
#include "m68krt.h"
#include "m68k.h"
#include <string.h>

/* Musashi haelt seinen CPU-Zustand in eigenen globalen Variablen und ruft m68k_read/write_memory_*
   ohne Kontext-Zeiger auf (s. m68krt.h) — deshalb muss der aktive RAM-Block hier ebenfalls global
   liegen, statt im q9_m68krt_t-Handle. Nur EIN q9_m68krt_init() gleichzeitig aktiv. */
static uint8_t  *g_ram;
static uint32_t  g_ram_len;

unsigned int m68k_read_memory_8(unsigned int address)
{
    return (address < g_ram_len) ? g_ram[address] : 0;
}

unsigned int m68k_read_memory_16(unsigned int address)
{
    if (address + 1 >= g_ram_len) {
        return 0;
    }
    return ((unsigned int)g_ram[address] << 8) | g_ram[address + 1];
}

unsigned int m68k_read_memory_32(unsigned int address)
{
    if (address + 3 >= g_ram_len) {
        return 0;
    }
    return ((unsigned int)g_ram[address]     << 24) | ((unsigned int)g_ram[address + 1] << 16) |
           ((unsigned int)g_ram[address + 2] <<  8) |  (unsigned int)g_ram[address + 3];
}

void m68k_write_memory_8(unsigned int address, unsigned int value)
{
    if (address < g_ram_len) {
        g_ram[address] = (uint8_t)value;
    }
}

void m68k_write_memory_16(unsigned int address, unsigned int value)
{
    if (address + 1 >= g_ram_len) {
        return;
    }
    g_ram[address]     = (uint8_t)(value >> 8);
    g_ram[address + 1] = (uint8_t)value;
}

void m68k_write_memory_32(unsigned int address, unsigned int value)
{
    if (address + 3 >= g_ram_len) {
        return;
    }
    g_ram[address]     = (uint8_t)(value >> 24);
    g_ram[address + 1] = (uint8_t)(value >> 16);
    g_ram[address + 2] = (uint8_t)(value >>  8);
    g_ram[address + 3] = (uint8_t)value;
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

    m68k_set_cpu_type(M68K_CPU_TYPE_68030);
    m68k_init();
    return Q9_M68KRT_OK;
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
    memset(rt, 0, sizeof(*rt));
}

//────────────────────────────────────────────────────────────────────────────────────────────────
// EOF m68krt.c                                                                            Ver. 1.00
//────────────────────────────────────────────────────────────────────────────────────────────────
