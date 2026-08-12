/* Bindet ab 0x80000000 -- dieselbe Lage wie die riscv-tests, damit Wirt und Speicherkarte
   fuer beide Prueflaeufe gleich bleiben. */
OUTPUT_ARCH( "riscv" )
ENTRY(_start)
SECTIONS
{
  . = 0x80000000;
  .text.init : { *(.text.init) }
  .text      : { *(.text*) }
  .rodata    : { *(.rodata*) }
  .data      : { *(.data*) }
  .bss       : { *(.bss*) *(COMMON) }
  . = ALIGN(16);
  . = . + 0x1000;
  _stack_top = .;
  _end = .;
}
