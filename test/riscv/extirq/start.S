/*═══════════════════════════════════════════════════════════════════════════════════════════════
  File:   start.S                                                                     Ver. 1.00
  Owner:  Claudia
  Desc.:  Einsprung, identisch zum Muster aus test/riscv/timer/start.S.
 ═══════════════════════════════════════════════════════════════════════════════════════════════*/
    .section .text.init, "ax"
    .globl _start
_start:
    la      sp, _stack_top
    call    main
1:  wfi
    j       1b
