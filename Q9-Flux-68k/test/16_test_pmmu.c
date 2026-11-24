/* P1: Musashi/68030 PMMU integration test, without a Q9 board or OS. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "m68k.h"
#include "m68kcpu.h"

#define RAM_SIZE 65536u
static uint8_t ram[RAM_SIZE];
static unsigned accesses_outside_ram;
unsigned int m68k_read_memory_8(unsigned int a) {
    if (a >= RAM_SIZE) { accesses_outside_ram++; return 0; }
    return ram[a];
}
unsigned int m68k_read_memory_16(unsigned int a) {
    return (m68k_read_memory_8(a) << 8) | m68k_read_memory_8(a + 1);
}
unsigned int m68k_read_memory_32(unsigned int a) {
    return (m68k_read_memory_16(a) << 16) | m68k_read_memory_16(a + 2);
}
void m68k_write_memory_8(unsigned int a, unsigned int v) {
    if (a >= RAM_SIZE) { accesses_outside_ram++; return; }
    ram[a] = (uint8_t)v;
}
void m68k_write_memory_16(unsigned int a, unsigned int v) {
    m68k_write_memory_8(a, v >> 8); m68k_write_memory_8(a + 1, v);
}
void m68k_write_memory_32(unsigned int a, unsigned int v) {
    m68k_write_memory_16(a, v >> 16); m68k_write_memory_16(a + 2, v);
}
void m68k_write_memory_32_pd(unsigned int a, unsigned int v) {
    m68k_write_memory_32(a, v);
}
static void put16(unsigned a, unsigned v) { m68k_write_memory_16(a, v); }
static void put32(unsigned a, unsigned v) { m68k_write_memory_32(a, v); }

int main(void) {
    /* PMOVE ($300).L,CRP; PMOVE ($308).L,TC; MOVE.L ($400).L,D0;
       STOP #$2700. Encodings: M68000 PRM 6-48, 6-49. */
    memset(ram, 0, sizeof ram);
    put32(0, 0x8000); put32(4, 0x100);
    put16(0x100, 0xf039); put16(0x102, 0x4c00); put32(0x104, 0x300);
    put16(0x108, 0xf039); put16(0x10a, 0x4000); put32(0x10c, 0x308);
    put16(0x110, 0x2039); put32(0x112, 0x400);
    put16(0x116, 0x4e72); put16(0x118, 0x2700);
    put32(0x300, 1); put32(0x304, 0); /* root DT=1, flat mapping */
    put32(0x308, 0x80c08840); /* enabled, 4 KiB pages, 8+8+4 index bits */
    put32(0x400, 0x12345678);
    m68k_init(); m68k_set_cpu_type(M68K_CPU_TYPE_68030); m68k_pulse_reset();
    m68k_execute(1000);
    if (!m68k_is_stopped() || !m68ki_cpu.pmmu_enabled ||
        m68k_get_reg(NULL, M68K_REG_D0) != 0x12345678 || accesses_outside_ram) {
        fprintf(stderr, "FAIL PMMU flat root: stopped=%d enabled=%u D0=%08x outside=%u\n",
                m68k_is_stopped(), m68ki_cpu.pmmu_enabled,
                m68k_get_reg(NULL, M68K_REG_D0), accesses_outside_ram);
        return 1;
    }
    /* Restart with a non-zero physical offset. The instruction after PMOVE TC
       and its operand must now be fetched from the relocated physical RAM. */
    put32(0x304, 0x1000);
    memcpy(ram + 0x1110, ram + 0x110, 10);
    put32(0x400, 0xdeadbeef);
    put32(0x1400, 0xa5c36987);
    accesses_outside_ram = 0;
    m68k_pulse_reset();
    m68k_execute(1000);
    if (!m68k_is_stopped() || !m68ki_cpu.pmmu_enabled ||
        m68k_get_reg(NULL, M68K_REG_D0) != 0xa5c36987 || accesses_outside_ram) {
        fprintf(stderr, "FAIL PMMU relocated root: stopped=%d enabled=%u D0=%08x outside=%u\n",
                m68k_is_stopped(), m68ki_cpu.pmmu_enabled,
                m68k_get_reg(NULL, M68K_REG_D0), accesses_outside_ram);
        return 1;
    }
    puts("PASS PMMU root DT=1: identity and relocated guest instruction/data reads");
    return 0;
}
