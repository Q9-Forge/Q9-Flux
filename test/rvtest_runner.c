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

#include "rvelf.h"

#define RAM_BASE   0x80000000ULL
#define RAM_SIZE   (64u * 1024u * 1024u)
#define BOOT_BASE  0x1000ULL          /* Reset-PC des Kerns, s. riscv_cpu.c */
#define BOOT_SIZE  0x1000u

/* Zyklen je interp()-Aufruf. Klein genug, um das Meldewort zeitnah zu sehen,
   gross genug, dass der Aufrufaufwand nicht dominiert. */
#define CYCLES_PER_SLICE  100000
/* Notbremse: ein haengender Test darf den Sammellauf nicht blockieren. */
#define MAX_SLICES        2000

/* Nur zum Auslesen des Meldeworts -- der ELF-Teil liegt in test/riscv/rvelf.c, gemeinsam
   genutzt mit test/rvboard_hello.c. */
static uint32_t rd32(const uint8_t *p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

//───────────────────────────────────────────────────────────────────────────────────────────────
// Ein Test
//───────────────────────────────────────────────────────────────────────────────────────────────

typedef enum { R_PASS, R_FAIL, R_HANG, R_ERROR } result_t;

static result_t run_one(const char *path, int xlen, uint32_t *fail_case, uint64_t *cycles_out)
{
    rvelf_t ef;
    uint32_t tohost;
    result_t res = R_ERROR;
    *fail_case = 0; *cycles_out = 0;

    if (rvelf_read(path, &ef) != 0) return R_ERROR;

    PhysMemoryMap *map = phys_mem_map_init();
    cpu_register_ram(map, RAM_BASE,  RAM_SIZE,  0);
    cpu_register_ram(map, BOOT_BASE, BOOT_SIZE, 0);

    if (rvelf_load_segments(&ef, map) != 0) goto out;

    tohost = rvelf_find_symbol(&ef, "tohost");
    if (!tohost) { fprintf(stderr, "  kein Symbol 'tohost'\n"); goto out; }

    if (rvelf_place_boot_stub(map, BOOT_BASE, ef.entry) != 0) goto out;

    uint8_t *tohost_ptr = phys_mem_get_ram_ptr(map, tohost, FALSE);
    if (!tohost_ptr) { fprintf(stderr, "  'tohost' (0x%08x) liegt nicht im RAM\n", tohost); goto out; }

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
    rvelf_free(&ef);
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
