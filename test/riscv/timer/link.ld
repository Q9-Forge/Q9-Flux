/* Gleiche Lage wie test/riscv/hello/link.ld -- ab 0x80000000, damit Wirt und Speicherkarte
   fuer alle Prueflaeufe (ISA-Suite, hello, timer) gleich bleiben. */
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
