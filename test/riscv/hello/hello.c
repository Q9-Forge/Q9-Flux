/*═══════════════════════════════════════════════════════════════════════════════════════════════
  File:   hello.c                                                                      Ver. 1.00
  Owner:  Claudia
  Desc.:  Bare-Metal-Lebenszeichen fuer den RISC-V-Kern. Schreibt ueber den 16550-UART an der
          QEMU-"virt"-Adresse und rechnet zum Schluss etwas, damit nicht nur der Speicherpfad,
          sondern auch der Instruktionspfad sichtbar geprueft ist.

          Kein libc: die Homebrew-Formel riscv64-elf-gcc bringt keine newlib mit. Fuer ein
          Lebenszeichen ist das ohnehin unnoetiger Ballast.
 ═══════════════════════════════════════════════════════════════════════════════════════════════*/
#define UART0     0x10000000u
#define UART_THR  ((volatile unsigned char *)(UART0 + 0))
#define UART_LSR  ((volatile unsigned char *)(UART0 + 5))
#define LSR_THRE  0x20u

static void uart_putc(char c)
{
    while ((*UART_LSR & LSR_THRE) == 0) { }
    *UART_THR = (unsigned char)c;
}

static void uart_puts(const char *s)
{
    while (*s) {
        if (*s == '\n') uart_putc('\r');
        uart_putc(*s++);
    }
}

static void uart_putdec(unsigned int v)
{
    char buf[12];
    int i = 0;
    if (v == 0) { uart_putc('0'); return; }
    while (v) { buf[i++] = (char)('0' + (v % 10u)); v /= 10u; }
    while (i) uart_putc(buf[--i]);
}

int main(void)
{
    unsigned int i, sum = 0;

    uart_puts("Q9 RISC-V: Lebenszeichen ueber 16550-UART\n");

    /* Etwas rechnen -- so ist auch der Instruktionspfad belegt, nicht nur der Speicherpfad. */
    for (i = 1; i <= 100u; i++) sum += i;
    uart_puts("Summe 1..100 = ");
    uart_putdec(sum);
    uart_puts("  (erwartet 5050)\n");

    return 0;
}
