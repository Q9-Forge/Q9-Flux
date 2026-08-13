/*═══════════════════════════════════════════════════════════════════════════════════════════════
  File:   start.S                                                                     Ver. 1.00
  Owner:  Claudia
  Desc.:  Einsprung. Setzt den Stapel, ruft main(); main() kehrt hier nicht zurueck (endet selbst
          in einer wfi-Schleife), das "j 1b" ist nur die Absicherung, falls main() doch zurueckkehrt.
 ═══════════════════════════════════════════════════════════════════════════════════════════════*/
    .section .text.init, "ax"
    .globl _start
_start:
    la      sp, _stack_top
    call    main
1:  wfi
    j       1b
