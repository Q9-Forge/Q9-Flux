//════════════════════════════════════════════════════════════════════════════════════════════════
// File:   rvelf.c                                                                         Ver. 1.00
// Owner:  Claudia
// Desc.:  Umsetzung des winzigen ELF32-Lesers, s. rvelf.h.
//════════════════════════════════════════════════════════════════════════════════════════════════

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "rvelf.h"

#define PT_LOAD    1
#define SHT_SYMTAB 2

static uint16_t rd16(const uint8_t *p) { return (uint16_t)(p[0] | (p[1] << 8)); }
static uint32_t rd32(const uint8_t *p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

int rvelf_read(const char *path, rvelf_t *out)
{
    FILE *f = fopen(path, "rb");
    long n;
    const uint8_t *e;

    out->data = NULL; out->size = 0; out->entry = 0;
    if (!f) { fprintf(stderr, "  kann '%s' nicht oeffnen\n", path); return -1; }
    fseek(f, 0, SEEK_END); n = ftell(f); fseek(f, 0, SEEK_SET);
    if (n <= 0) { fclose(f); return -1; }
    out->data = malloc((size_t)n);
    out->size = (size_t)n;
    if (fread(out->data, 1, (size_t)n, f) != (size_t)n) {
        fclose(f); free(out->data); out->data = NULL; return -1;
    }
    fclose(f);

    e = out->data;
    if (out->size < 52 || memcmp(e, "\177ELF", 4) != 0) {
        fprintf(stderr, "  '%s' ist keine ELF-Datei\n", path); return -1;
    }
    if (e[4] != 1) { fprintf(stderr, "  '%s': nur ELF32 wird hier gelesen\n", path); return -1; }
    if (e[5] != 1) { fprintf(stderr, "  '%s': nur Little-Endian\n", path); return -1; }
    out->entry = rd32(e + 24);
    return 0;
}

void rvelf_free(rvelf_t *ef)
{
    free(ef->data);
    ef->data = NULL;
}

int rvelf_load_segments(rvelf_t *ef, PhysMemoryMap *map)
{
    const uint8_t *e = ef->data;
    uint32_t phoff   = rd32(e + 28);
    uint16_t phentsz = rd16(e + 42);
    uint16_t phnum   = rd16(e + 44);
    int loaded = 0;
    uint16_t i;

    for (i = 0; i < phnum; i++) {
        const uint8_t *ph = e + phoff + (size_t)i * phentsz;
        uint32_t offset, paddr, filesz, memsz;
        uint8_t *dst;

        if (rd32(ph) != PT_LOAD) continue;
        offset = rd32(ph + 4);
        paddr  = rd32(ph + 12);
        filesz = rd32(ph + 16);
        memsz  = rd32(ph + 20);
        if (filesz == 0 && memsz == 0) continue;

        dst = phys_mem_get_ram_ptr(map, paddr, TRUE);
        if (!dst) {
            fprintf(stderr, "  Segment %u will nach 0x%08x -- dort ist kein RAM\n",
                    (unsigned)i, paddr);
            return -1;
        }
        memcpy(dst, e + offset, filesz);
        if (memsz > filesz) memset(dst + filesz, 0, memsz - filesz);   /* .bss */
        loaded++;
    }
    if (!loaded) { fprintf(stderr, "  keine ladbaren Segmente\n"); return -1; }
    return 0;
}

uint32_t rvelf_find_symbol(rvelf_t *ef, const char *name)
{
    const uint8_t *e = ef->data;
    uint32_t shoff   = rd32(e + 32);
    uint16_t shentsz = rd16(e + 46);
    uint16_t shnum   = rd16(e + 48);
    uint16_t i;

    for (i = 0; i < shnum; i++) {
        const uint8_t *sh = e + shoff + (size_t)i * shentsz;
        uint32_t symoff, symsz, link, entsz, o;
        const uint8_t *strsh;
        const char *strtab;

        if (rd32(sh + 4) != SHT_SYMTAB) continue;
        symoff = rd32(sh + 16);
        symsz  = rd32(sh + 20);
        link   = rd32(sh + 24);            /* zugehoerige Stringtabelle */
        entsz  = rd32(sh + 36);
        if (entsz == 0) continue;

        strsh  = e + shoff + (size_t)link * shentsz;
        strtab = (const char *)(e + rd32(strsh + 16));

        for (o = 0; o + entsz <= symsz; o += entsz) {
            const uint8_t *sym = e + symoff + o;
            uint32_t nameoff = rd32(sym);
            if (nameoff == 0) continue;
            if (strcmp(strtab + nameoff, name) == 0) return rd32(sym + 4);   /* st_value */
        }
    }
    return 0;
}

int rvelf_place_boot_stub(PhysMemoryMap *map, uint64_t boot_addr, uint32_t entry)
{
    uint8_t *boot = phys_mem_get_ram_ptr(map, boot_addr, TRUE);
    uint32_t hi, lo, lui, jalr;

    if (!boot) return -1;
    hi   = (entry + 0x800u) & 0xfffff000u;              /* Vorzeichenkorrektur fuer lo12 */
    lo   = entry - hi;
    lui  = hi | (5u << 7) | 0x37u;                      /* lui  t0, hi     */
    jalr = ((lo & 0xfffu) << 20) | (5u << 15) | 0x67u;  /* jalr x0, lo(t0) */
    memcpy(boot + 0, &lui,  4);
    memcpy(boot + 4, &jalr, 4);
    return 0;
}

// EOF rvelf.c                                                                              Ver. 1.00
