/*═══════════════════════════════════════════════════════════════════════════════════════════════
  File:   trap.S                                                                      Ver. 1.00
  Owner:  Claudia
  Desc.:  Minimaler Trap-Einsprung fuer den Timer-Interrupt. Rettet nur die Register, die
          trap_c_handler() nach der RISC-V-Aufrufkonvention selbst zerstoeren darf (die
          "caller-saved" ra/t0-t6/a0-a7) -- s/s-Register bleiben unberuehrt, weil eine C-Funktion
          sie unveraendert zurueckgibt (callee-saved) und der unterbrochene Code sich darauf
          verlassen darf. mtvec zeigt DIREKT hierher (der Kern unterstuetzt nur den direkten
          Modus, keinen Vektortisch, s. clint.h-Kommentar zur set_mip-Falle fuer den Hintergrund).
 ═══════════════════════════════════════════════════════════════════════════════════════════════*/
    .section .text, "ax"
    .globl trap_entry
    .align 4                       /* mtvec-Anforderung: die unteren 2 Bit sind der Modus */
trap_entry:
    addi    sp, sp, -68
    sw      ra,   0(sp)
    sw      t0,   4(sp)
    sw      t1,   8(sp)
    sw      t2,  12(sp)
    sw      t3,  16(sp)
    sw      t4,  20(sp)
    sw      t5,  24(sp)
    sw      t6,  28(sp)
    sw      a0,  32(sp)
    sw      a1,  36(sp)
    sw      a2,  40(sp)
    sw      a3,  44(sp)
    sw      a4,  48(sp)
    sw      a5,  52(sp)
    sw      a6,  56(sp)
    sw      a7,  60(sp)

    call    trap_c_handler

    lw      ra,   0(sp)
    lw      t0,   4(sp)
    lw      t1,   8(sp)
    lw      t2,  12(sp)
    lw      t3,  16(sp)
    lw      t4,  20(sp)
    lw      t5,  24(sp)
    lw      t6,  28(sp)
    lw      a0,  32(sp)
    lw      a1,  36(sp)
    lw      a2,  40(sp)
    lw      a3,  44(sp)
    lw      a4,  48(sp)
    lw      a5,  52(sp)
    lw      a6,  56(sp)
    lw      a7,  60(sp)
    addi    sp, sp, 68
    mret
