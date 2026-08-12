//════════════════════════════════════════════════════════════════════════════════════════════════
// File:   rvtest_runner.c                                                                 Ver. 1.00
// Owner:  Claudia
// Desc.:  Prueflauf fuer den vendorierten RISC-V-CPU-Kern (third_party/tinyemu) gegen die
//         offizielle ISA-Testsuite riscv-tests. Laedt eine Test-ELF, laesst sie auf dem Kern
//         laufen und wertet das HTIF-Meldewort aus.
//
//         BEWUSST OHNE BOARD: kein UART, kein Timer, kein Interruptcontroller, keine MMU. Die
//         "p"-Varianten der Suite (physische Adressierung) brauchen nur RAM und eine Speicher-
//         stelle zum Melden. Damit prueft dieser Lauf ausschliesslich die CPU -- schlaegt hier
//         etwas fehl, kennt man die INSTRUKTION, nicht nur "bootet nicht".
//
// Call:   build/<platform>/rvtest_runner <test-elf> [...]      (gebaut per "make test-riscv")
//
// Meldeprotokoll (HTIF, siehe riscv-tests/env/p/riscv_test.h):
//         Der Test fuehrt "ecall" mit a7=93 aus; sein eigener Trap-Handler schreibt daraufhin
//         in das globale Wort "tohost":  1 = bestanden,  (n<<1)|1 = Unterfall n fehlgeschlagen.
//         Wir lesen die Adresse von "tohost" aus der Symboltabelle der ELF -- sie ist nicht
//         fest, sondern haengt am Linkerskript (env/p/link.ld legt .tohost hinter .text.init).
//
// Speicherlage: env/p/link.ld linkt alles ab 0x80000000. Der Kern startet nach Reset aber bei
//         0x1000 (riscv_cpu.c). Statt den Vendor-Code anzufassen legen wir dort einen winzigen
//         Sprungbefehl ab -- dieselbe Loesung, die TinyEMUs eigenes riscv_machine.c benutzt.
//════════════════════════════════════════════════════════════════════════════════════════════════

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

/* cutils.h MUSS vor iomem.h stehen -- dort sind BOOL/TRUE/FALSE definiert,
   die iomem.h bereits in seinen Prototypen benutzt. */
#include "cutils.h"
#include "iomem.h"
#include "riscv_cpu.h"

#define RAM_BASE   0x80000000ULL
#define RAM_SIZE   (64u * 1024u * 1024u)
#define BOOT_BASE  0x1000ULL          /* Reset-PC des Kerns, s. riscv_cpu.c */
#define BOOT_SIZE  0x1000u

/* Zyklen je interp()-Aufruf. Klein genug, um das Meldewort zeitnah zu sehen,
   gross genug, dass der Aufrufaufwand nicht dominiert. */
#define CYCLES_PER_SLICE  100000
/* Notbremse: ein haengender Test darf den Sammellauf nicht blockieren. */
#define MAX_SLICES        2000

//───────────────────────────────────────────────────────────────────────────────────────────────
// Minimaler ELF32-Leser. Bewusst von Hand statt libelf: wir brauchen genau zwei Dinge --
// die PT_LOAD-Segmente und die Adresse eines einzigen Symbols.
//───────────────────────────────────────────────────────────────────────────────────────────────

#define PT_LOAD    1
#define SHT_SYMTAB 2

typedef struct {
    uint8_t  *data;
    size_t    size;
    uint32_t  entry;
    uint32_t  tohost;      /* 0 = nicht gefunden */
} elf_file_t;

static uint16_t rd16(const uint8_t *p) { return (uint16_t)(p[0] | (p[1] << 8)); }
static uint32_t rd32(const uint8_t *p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

static int elf_read(const char *path, elf_file_t *out)
{
    FILE *f = fopen(path, "rb");
    if (!f) { fprintf(stderr, "  kann '%s' nicht oeffnen\n", path); return -1; }
    fseek(f, 0, SEEK_END);
    long n = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (n <= 0) { fclose(f); return -1; }
    out->data = malloc((size_t)n);
    out->size = (size_t)n;
    if (fread(out->data, 1, (size_t)n, f) != (size_t)n) { fclose(f); free(out->data); return -1; }
    fclose(f);

    const uint8_t *e = out->data;
    if (out->size < 52 || memcmp(e, "\177ELF", 4) != 0) {
        fprintf(stderr, "  '%s' ist keine ELF-Datei\n", path); return -1;
    }
    if (e[4] != 1) { fprintf(stderr, "  '%s': nur ELF32 wird hier gelesen\n", path); return -1; }
    if (e[5] != 1) { fprintf(stderr, "  '%s': nur Little-Endian\n", path); return -1; }
    out->entry  = rd32(e + 24);
    out->tohost = 0;
    return 0;
}

/* Kopiert alle PT_LOAD-Segmente an ihre physische Adresse. */
static int elf_load_segments(elf_file_t *ef, PhysMemoryMap *map)
{
    const uint8_t *e = ef->data;
    uint32_t phoff  = rd32(e + 28);
    uint16_t phentsz = rd16(e + 42);
    uint16_t phnum   = rd16(e + 44);
    int loaded = 0;

    for (uint16_t i = 0; i < phnum; i++) {
        const uint8_t *ph = e + phoff + (size_t)i * phentsz;
        if (rd32(ph) != PT_LOAD) continue;
        uint32_t offset = rd32(ph + 4);
        uint32_t paddr  = rd32(ph + 12);
        uint32_t filesz = rd32(ph + 16);
        uint32_t memsz  = rd32(ph + 20);
        if (filesz == 0 && memsz == 0) continue;

        uint8_t *dst = phys_mem_get_ram_ptr(map, paddr, TRUE);
        if (!dst) {
            fprintf(stderr, "  Segment %u will nach 0x%08x -- dort ist kein RAM\n", i, paddr);
            return -1;
        }
        memcpy(dst, e + offset, filesz);
        if (memsz > filesz) memset(dst + filesz, 0, memsz - filesz);   /* .bss */
        loaded++;
    }
    if (!loaded) { fprintf(stderr, "  keine ladbaren Segmente\n"); return -1; }
    return 0;
}

/* Sucht die Adresse eines Symbols in der Symboltabelle. */
static uint32_t elf_find_symbol(elf_file_t *ef, const char *name)
{
    const uint8_t *e = ef->data;
    uint32_t shoff   = rd32(e + 32);
    uint16_t shentsz = rd16(e + 46);
    uint16_t shnum   = rd16(e + 48);

    for (uint16_t i = 0; i < shnum; i++) {
        const uint8_t *sh = e + shoff + (size_t)i * shentsz;
        if (rd32(sh + 4) != SHT_SYMTAB) continue;
        uint32_t symoff  = rd32(sh + 16);
        uint32_t symsz   = rd32(sh + 20);
        uint32_t link    = rd32(sh + 24);          /* zugehoerige Stringtabelle */
        uint32_t entsz   = rd32(sh + 36);
        if (entsz == 0) continue;

        const uint8_t *strsh = e + shoff + (size_t)link * shentsz;
        const char *strtab = (const char *)(e + rd32(strsh + 16));

        for (uint32_t o = 0; o + entsz <= symsz; o += entsz) {
            const uint8_t *sym = e + symoff + o;
            uint32_t nameoff = rd32(sym);
            if (nameoff == 0) continue;
            if (strcmp(strtab + nameoff, name) == 0) return rd32(sym + 4);   /* st_value */
        }
    }
    return 0;
}

//───────────────────────────────────────────────────────────────────────────────────────────────
// Ein Test
//───────────────────────────────────────────────────────────────────────────────────────────────

typedef enum { R_PASS, R_FAIL, R_HANG, R_ERROR } result_t;

static result_t run_one(const char *path, int xlen, uint32_t *fail_case, uint64_t *cycles_out)
{
    elf_file_t ef;
    result_t res = R_ERROR;
    *fail_case = 0; *cycles_out = 0;

    if (elf_read(path, &ef) != 0) return R_ERROR;

    PhysMemoryMap *map = phys_mem_map_init();
    cpu_register_ram(map, RAM_BASE,  RAM_SIZE,  0);
    cpu_register_ram(map, BOOT_BASE, BOOT_SIZE, 0);

    if (elf_load_segments(&ef, map) != 0) goto out;

    ef.tohost = elf_find_symbol(&ef, "tohost");
    if (!ef.tohost) { fprintf(stderr, "  kein Symbol 'tohost'\n"); goto out; }

    /* Sprungbefehl am Reset-PC: lui t0, <hi20 von entry>; jalr x0, lo12(t0) */
    {
        uint8_t *boot = phys_mem_get_ram_ptr(map, BOOT_BASE, TRUE);
        if (!boot) goto out;
        uint32_t hi = (ef.entry + 0x800) & 0xfffff000u;   /* Vorzeichenkorrektur fuer lo12 */
        uint32_t lo = ef.entry - hi;
        uint32_t lui  = hi | (5u << 7) | 0x37u;                       /* lui  t0, hi   */
        uint32_t jalr = ((lo & 0xfffu) << 20) | (5u << 15) | 0x67u;   /* jalr x0, lo(t0) */
        memcpy(boot + 0, &lui,  4);
        memcpy(boot + 4, &jalr, 4);
    }

    uint8_t *tohost_ptr = phys_mem_get_ram_ptr(map, ef.tohost, FALSE);
    if (!tohost_ptr) { fprintf(stderr, "  'tohost' (0x%08x) liegt nicht im RAM\n", ef.tohost); goto out; }

    RISCVCPUState *cpu = riscv_cpu_init(map, xlen);
    if (!cpu) { fprintf(stderr, "  CPU laesst sich nicht anlegen (xlen=%d)\n", xlen); goto out; }

    res = R_HANG;
    for (int slice = 0; slice < MAX_SLICES; slice++) {
        riscv_cpu_interp(cpu, CYCLES_PER_SLICE);
        uint32_t th = rd32(tohost_ptr);
        if (th != 0) {
            if (th == 1) { res = R_PASS; }
            else         { res = R_FAIL; *fail_case = th >> 1; }
            break;
        }
        if (riscv_cpu_get_power_down(cpu)) break;   /* CPU haelt an, ohne zu melden */
    }
    *cycles_out = riscv_cpu_get_cycles(cpu);
    riscv_cpu_end(cpu);

out:
    phys_mem_map_end(map);
    free(ef.data);
    return res;
}

//───────────────────────────────────────────────────────────────────────────────────────────────

int main(int argc, char **argv)
{
    int xlen = 32;
    int first = 1;

    if (argc > 2 && strcmp(argv[1], "--xlen") == 0) { xlen = atoi(argv[2]); first = 3; }
    if (first >= argc) {
        fprintf(stderr, "Aufruf: %s [--xlen 32|64] <test-elf> [...]\n", argv[0]);
        return 2;
    }

    int pass = 0, fail = 0, hang = 0, err = 0;
    uint64_t cycles_total = 0;

    for (int i = first; i < argc; i++) {
        uint32_t fc; uint64_t cyc;
        const char *base = strrchr(argv[i], '/');
        base = base ? base + 1 : argv[i];

        result_t r = run_one(argv[i], xlen, &fc, &cyc);
        cycles_total += cyc;
        switch (r) {
        case R_PASS:  pass++; break;
        case R_FAIL:  fail++; printf("FAIL  %-24s Unterfall %u\n", base, fc); break;
        case R_HANG:  hang++; printf("HANG  %-24s keine Meldung nach %d Zyklen\n",
                                     base, MAX_SLICES * CYCLES_PER_SLICE); break;
        default:      err++;  printf("ERR   %-24s\n", base); break;
        }
    }

    int total = pass + fail + hang + err;
    printf("\n  RV%d ISA-Tests: %d/%d bestanden", xlen, pass, total);
    if (fail) printf(", %d fehlgeschlagen", fail);
    if (hang) printf(", %d ohne Meldung", hang);
    if (err)  printf(", %d nicht lauffaehig", err);
    printf("  (%llu Zyklen)\n", (unsigned long long)cycles_total);

    return (fail || hang || err) ? 1 : 0;
}

// EOF rvtest_runner.c                                                                     Ver. 1.00
