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

/* Vector 56 is a post-instruction exception with a format-2 frame:
   MC68030UM pp. 8-21/8-22, 9-52. */
static int check_config_error(unsigned ext, unsigned high, unsigned low) {
    unsigned sp;
    memset(ram, 0, sizeof ram);
    put32(0, 0x8000); put32(4, 0x100); put32(56 * 4, 0x200);
    put16(0x100, 0xf039); put16(0x102, ext); put32(0x104, 0x300);
    put16(0x108, 0x4e72); put16(0x10a, 0x2700);
    put16(0x200, 0x4e72); put16(0x202, 0x2700);
    put32(0x300, high); put32(0x304, low);
    accesses_outside_ram = 0;
    m68k_pulse_reset(); m68k_execute(1000);
    sp = m68k_get_reg(NULL, M68K_REG_SP);
    if (!m68k_is_stopped() || m68k_get_reg(NULL, M68K_REG_PC) != 0x204 ||
        sp != 0x7ff4 || m68k_read_memory_32(sp + 2) != 0x108 ||
        m68k_read_memory_16(sp + 6) != 0x20e0 ||
        m68k_read_memory_32(sp + 8) != 0x100 || accesses_outside_ram) {
        fprintf(stderr, "FAIL config exception ext=%04x PC=%08x SP=%08x\n", ext,
                m68k_get_reg(NULL, M68K_REG_PC), sp); return 1;
    }
    if (ext == 0x4000 && (m68ki_cpu.mmu_tc != (high & 0x7fffffff) || m68ki_cpu.pmmu_enabled)) {
        fputs("FAIL invalid TC not retained with E cleared\n", stderr); return 1;
    }
    if (ext == 0x4c00 && (m68ki_cpu.mmu_crp_limit != high || m68ki_cpu.mmu_crp_aptr != low)) {
        fputs("FAIL invalid CRP not retained\n", stderr); return 1;
    }
    if (ext == 0x4800 && (m68ki_cpu.mmu_srp_limit != high || m68ki_cpu.mmu_srp_aptr != low)) {
        fputs("FAIL invalid SRP not retained\n", stderr); return 1;
    }
    return 0;
}

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
    /* PMOVE TT0/TT1 and MMUSR roundtrips, with adjacent sentinel words.
       Format and operand sizes: Motorola M68000 PRM pp. 6-49/6-50. */
    memset(ram, 0, sizeof ram);
    put32(0, 0x8000); put32(4, 0x100);
    {
        unsigned pc = 0x100;
        const unsigned ext[] = {0x0800, 0x0c00, 0x0a00, 0x0e00, 0x6000, 0x6200};
        const unsigned addr[] = {0x300, 0x304, 0x310, 0x314, 0x320, 0x324};
        unsigned i;
        for (i = 0; i < 6; i++) {
            put16(pc, 0xf039); put16(pc + 2, ext[i]); put32(pc + 4, addr[i]); pc += 8;
        }
        put16(pc, 0x4e72); put16(pc + 2, 0x2700);
    }
    put32(0x300, 0x12345678); put32(0x304, 0x23456789);
    put16(0x320, 0xa5c3); put16(0x322, 0x5678);
    put32(0x324, 0xdeadbeef);
    accesses_outside_ram = 0;
    m68k_pulse_reset();
    m68ki_cpu.mmu_crp_limit = 0x12340001; m68ki_cpu.mmu_crp_aptr = 0x56789000;
    m68ki_cpu.mmu_srp_limit = 0x87650001; m68ki_cpu.mmu_srp_aptr = 0x43210000;
    m68k_execute(1000);
    if (m68ki_cpu.mmu_crp_limit != 0x12340001 || m68ki_cpu.mmu_crp_aptr != 0x56789000 ||
        m68ki_cpu.mmu_srp_limit != 0x87650001 || m68ki_cpu.mmu_srp_aptr != 0x43210000 ||
        !m68k_is_stopped() || accesses_outside_ram ||
        m68k_read_memory_32(0x310) != 0x12345678 ||
        m68k_read_memory_32(0x314) != 0x23456789 ||
        m68k_read_memory_16(0x324) != 0xa5c3 ||
        m68k_read_memory_16(0x326) != 0xbeef) {
        fprintf(stderr, "FAIL PMOVE TT/MMUSR: tt0=%08x tt1=%08x status=%08x\n",
                m68k_read_memory_32(0x310), m68k_read_memory_32(0x314), m68k_read_memory_32(0x324));
        return 1;
    }
    puts("PASS PMOVE TT0/TT1 and 16-bit MMUSR roundtrips");
    m68ki_cpu.mmu_tc = 0x80c08840;
    m68ki_cpu.mmu_tt0 = 0x1234d678; m68ki_cpu.mmu_tt1 = 0x2345e789;
    m68k_pulse_reset();
    if (m68ki_cpu.mmu_tc != 0x00c08840 || m68ki_cpu.mmu_tt0 != 0x12345678 ||
        m68ki_cpu.mmu_tt1 != 0x23456789 || m68ki_cpu.pmmu_enabled) {
        fprintf(stderr, "FAIL reset must clear TC/TT enable bits\n"); return 1;
    }
    puts("PASS reset clears only TC/TT enable bits");
    if (check_config_error(0x4000, 0x80000000, 0) ||
        check_config_error(0x4000, 0x80c08440, 0) ||
        check_config_error(0x4c00, 0x12340000, 0x12345678) ||
        check_config_error(0x4800, 0x12340000, 0x87654321)) return 1;
    puts("PASS invalid TC, CRP and SRP raise vector 56");
    return 0;
}
