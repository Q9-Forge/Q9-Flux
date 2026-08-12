/*═══════════════════════════════════════════════════════════════════════════════════════════════
  File:   start.S                                                                     Ver. 1.00
  Owner:  Claudia
  Desc.:  Einsprung des Bare-Metal-Lebenszeichens. Setzt den Stapelzeiger, ruft main() und legt
          die CPU danach schlafen. Das "wfi" ist zugleich das ENDESIGNAL an den Wirt: der
          Prueflauf fragt riscv_cpu_get_power_down() ab und beendet den Lauf, sobald es zutrifft.
          Kein HTIF, kein Syscall -- fuer ein Lebenszeichen braucht es beides nicht.
 ═══════════════════════════════════════════════════════════════════════════════════════════════*/
    .section .text.init, "ax"
    .globl _start
_start:
    la      sp, _stack_top
    call    main
1:  wfi
    j       1b
