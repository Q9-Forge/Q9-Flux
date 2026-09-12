//════════════════════════════════════════════════════════════════════════════════════════════════
// File:   rvelf.h                                                                         Ver. 1.00
// Owner:  Claudia
// Desc.:  Winziger ELF32-Leser fuer die RISC-V-Prueflaeufe. Bewusst von Hand statt libelf: es
//         werden genau zwei Dinge gebraucht -- die PT_LOAD-Segmente und die Adresse einzelner
//         Symbole. Gemeinsam genutzt von test/rvtest_runner.c (ISA-Suite) und
//         test/rvboard_hello.c (Board-Lebenszeichen).
//════════════════════════════════════════════════════════════════════════════════════════════════
#ifndef Q9_RVELF_H
#define Q9_RVELF_H

#include <stdint.h>
#include <stddef.h>

#include "cutils.h"
#include "iomem.h"

typedef struct {
    uint8_t  *data;
    size_t    size;
    uint32_t  entry;
} rvelf_t;

int      rvelf_read(const char *path, rvelf_t *out);
void     rvelf_free(rvelf_t *ef);
int      rvelf_load_segments(rvelf_t *ef, PhysMemoryMap *map);
uint32_t rvelf_find_symbol(rvelf_t *ef, const char *name);   /* 0 = nicht gefunden */

/* Legt am Reset-PC einen Sprung nach `entry` ab. Der Kern startet nach Reset bei 0x1000
   (riscv_cpu.c), die Programme werden aber ab 0x80000000 gebunden -- dieselbe Loesung wie
   TinyEMUs eigenes riscv_machine.c, ohne den Vendor-Code anzufassen. */
int      rvelf_place_boot_stub(PhysMemoryMap *map, uint64_t boot_addr, uint32_t entry);

#endif /* Q9_RVELF_H */
// EOF rvelf.h                                                                              Ver. 1.00
