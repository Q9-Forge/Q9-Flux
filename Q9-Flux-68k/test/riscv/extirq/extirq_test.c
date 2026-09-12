/*═══════════════════════════════════════════════════════════════════════════════════════════════
  File:   extirq_test.c                                                               Ver. 1.00
  Owner:  Claudia
  Desc.:  Eigenstaendiger Regressionstest fuer den in Stufe 3 gefundenen Kernfehler: die
          Schreibmaske von CSR mie (0x304) liess Bit 11 (MIP_MEIP/MIE_MEIE, Machine External
          Interrupt Enable) nicht durch -- ein Gast konnte externe Interrupts NIE per Software
          freigeben, unabhaengig von PLIC-Verdrahtung oder IRQ-Nummer. Der volle Fund und die
          Begruendung stehen in docs/RISCV.md und third_party/tinyemu/Q9_VENDOR.md.

          BEWUSST OHNE NuttX: der volle Nachweis (Stufe 3, make test-rvnuttx) haengt an einer
          schweren Werkzeugkette (xPack-Toolchain, kconfig-tweak, genromfs, flock). Dieser Test
          isoliert GENAU den Kernfehler mit unserer normalen riscv64-elf-gcc-Toolchain und laeuft
          in Millisekunden -- die einzige Absicherung, die NICHT davon abhaengt, ob die schwere
          Kette gerade installiert ist. Gegengeprueft: mit zurueckgedrehtem Kern-Fix bleibt der
          Interruptzaehler bei 0 (der Trap-Handler wird nie erreicht), dieser Test schlaegt fehl.

          Ablauf: PLIC fuer eine Quelle einrichten (Prioritaet, Freigabe fuer Kontext 0, Schwelle),
          UART.IER.ERBFI setzen, dann -- die eigentliche Pruefstelle -- mie.MEIE per "csrs mie"
          setzen und mstatus.MIE freigeben. Fuenfmal per "wfi" auf ein per stdin eingespeistes
          Zeichen warten; der Trap-Handler claimt/completed bei PLIC und liest das Zeichen ab.

          Anders als test/riscv/timer/ (dort zaehlt die PLIC-Quellennummer nicht, weil kein NuttX
          beteiligt ist) waehlt dieser Test die PLIC-Quelle fuer den UART SELBST -- Quelle 1, die
          niedrigste gueltige (0 ist laut Spezifikation unbenutzt). Keine NuttX-IRQ-Umrechnung
          noetig, s. docs/RISCV.md fuer die dortige Falle.
 ═══════════════════════════════════════════════════════════════════════════════════════════════*/

#define UART0        0x10000000u
#define UART_RBR     ((volatile unsigned char *)(UART0 + 0))
#define UART_IER     ((volatile unsigned char *)(UART0 + 1))
#define UART_THR     ((volatile unsigned char *)(UART0 + 0))
#define UART_LSR     ((volatile unsigned char *)(UART0 + 5))
#define LSR_THRE     0x20u

#define PLIC0             0x0c000000u
#define UART_PLIC_SOURCE  1u    /* frei gewaehlt, s. Kopf -- kein NuttX beteiligt */
#define PLIC_PRIORITY(n)  ((volatile unsigned int *)(PLIC0 + 4u * (n)))
#define PLIC_ENABLE1_CTX0 ((volatile unsigned int *)(PLIC0 + 0x002000u))
#define PLIC_THRESHOLD_CTX0 ((volatile unsigned int *)(PLIC0 + 0x200000u))
#define PLIC_CLAIM_CTX0     ((volatile unsigned int *)(PLIC0 + 0x200004u))

#define MIE_MEIE     (1u << 11)  /* mie: Machine External Interrupt Enable -- die Pruefstelle */
#define MSTATUS_MIE  (1u << 3)

#define TARGET_COUNT 5u

static volatile unsigned int interrupt_count = 0;

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

void trap_c_handler(void)
{
    unsigned int cause;
    __asm__ volatile ("csrr %0, mcause" : "=r"(cause));

    if ((cause & 0x80000000u) == 0) return;         /* nur Interrupts, keine Ausnahmen */
    if ((cause & 0x7fffffffu) != 11u) return;        /* nur "Machine External Interrupt" (11) */

    (void)*UART_RBR;                     /* Zeichen abholen -- senkt UARTs Anforderung */
    *PLIC_CLAIM_CTX0;                    /* CLAIM: liest die uebernommene Quelle (erwartet 1) */
    *PLIC_CLAIM_CTX0 = UART_PLIC_SOURCE; /* COMPLETE: dieselbe Nummer zurueckschreiben */
    interrupt_count++;
}

int main(void)
{
    extern void trap_entry(void);

    __asm__ volatile ("csrw mtvec, %0" :: "r"(trap_entry));

    *PLIC_PRIORITY(UART_PLIC_SOURCE) = 1u;
    *PLIC_ENABLE1_CTX0 = (1u << UART_PLIC_SOURCE);
    *PLIC_THRESHOLD_CTX0 = 0u;
    *UART_IER = 0x01u;   /* ERBFI: UART fordert bei anliegendem Zeichen an */

    /* DIE PRUEFSTELLE: schlaegt diese Zeile fehl (weil der Kern das Bit verwirft), bleibt
       mie.MEIE 0, die CPU nimmt trotz anliegendem mip.MEIP nie den Trap, "wfi" haengt fuer
       immer -- der Test wuerde nach Erschoepfung des Zyklenvorrats mit interrupt_count=0 enden. */
    __asm__ volatile ("csrs mie, %0" :: "r"(MIE_MEIE));
    __asm__ volatile ("csrs mstatus, %0" :: "r"(MSTATUS_MIE));

    uart_puts("Q9 RISC-V: externer PLIC-Interrupt-Test (mie.MEIE)\n");

    while (interrupt_count < TARGET_COUNT) {
        __asm__ volatile ("wfi");
    }

    __asm__ volatile ("csrc mie, %0" :: "r"(MIE_MEIE));
    __asm__ volatile ("csrc mstatus, %0" :: "r"(MSTATUS_MIE));

    uart_puts("Interrupts behandelt: ");
    uart_putdec(interrupt_count);
    uart_puts("  (erwartet 5)\n");

    return 0;
}

/* EOF extirq_test.c                                                                   Ver. 1.00 */
