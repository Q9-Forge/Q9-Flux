/*═══════════════════════════════════════════════════════════════════════════════════════════════
  File:   timer_test.c                                                                Ver. 1.00
  Owner:  Claudia
  Desc.:  Stufe 2b des RISC-V-Bring-up: CLINT-Timer-Interrupt sauber abfangen. Kein fremdes
          Betriebssystem -- das ist bewusst noch ein eigenes Testprogramm, damit ein Fehlschlag
          eindeutig auf die Interrupt-Maschinerie zeigt und nicht in einem fremden Kernel gesucht
          werden muss.

          Ablauf: mtvec auf den Trap-Einsprung setzen, mie.MTIE und mstatus.MIE setzen, das erste
          mtimecmp programmieren, dann in "wfi" warten. Der Trap-Handler zaehlt mit und
          programmiert das naechste mtimecmp selbst nach -- der Interrupt "tickt" dadurch
          periodisch weiter, ohne dass main() je herausfinden muss, wie spaet es gerade ist.
          Nach TARGET_COUNT Interrupts werden beide Freigaben wieder abgeschaltet und ein letztes
          "wfi" haelt sauber an (keine unmaskierten Interrupts mehr -> bleibt im Stromsparzustand,
          dasselbe Endekriterium wie in test/riscv/hello/hello.c).

          Kein libc: dieselbe Randbedingung wie beim Lebenszeichen aus Stufe 2 (die Homebrew-
          Formel bringt keine newlib mit).
 ═══════════════════════════════════════════════════════════════════════════════════════════════*/

#define UART0     0x10000000u
#define UART_THR  ((volatile unsigned char *)(UART0 + 0))
#define UART_LSR  ((volatile unsigned char *)(UART0 + 5))
#define LSR_THRE  0x20u

#define CLINT0            0x02000000u
#define CLINT_MTIMECMP_LO ((volatile unsigned int *)(CLINT0 + 0x4000))
#define CLINT_MTIMECMP_HI ((volatile unsigned int *)(CLINT0 + 0x4004))

#define MIE_MTIE     (1u << 7)   /* mie:     Machine Timer Interrupt Enable */
#define MSTATUS_MIE  (1u << 3)   /* mstatus: globale Interruptfreigabe im Machine-Mode */

#define DELTA         5000u      /* Zyklen zwischen zwei Interrupts -- willkuerlich, aber
                                     deterministisch: gross genug fuer periodisches Verhalten,
                                     klein genug, um den Zyklenvorrat des Boards nicht zu sprengen */
#define TARGET_COUNT  5u

static volatile unsigned int interrupt_count = 0;
static unsigned long long next_cmp;

//───────────────────────────────────────────────────────────────────────────────────────────────
// UART -- wortgleich zu test/riscv/hello/hello.c (kein libc, also keine gemeinsame Bibliothek
// ohne Zusatzaufwand; fuer zwei kleine Funktionen ist die Verdopplung hier vertretbar)
//───────────────────────────────────────────────────────────────────────────────────────────────

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

//───────────────────────────────────────────────────────────────────────────────────────────────

/* mtimecmp ist ein 64-Bit-Register, aber nur 32-Bit-weise beschreibbar (s. clint.h). Der uebliche
   Kunstgriff: erst die obere Haelfte auf einen weit in der Zukunft liegenden Wert setzen, damit
   der Vergleich waehrend des zweigeteilten Schreibens nicht zwischenzeitlich anspricht, dann die
   untere Haelfte, dann die obere endgueltig. */
static void clint_set_mtimecmp(unsigned long long v)
{
    *CLINT_MTIMECMP_HI = 0xffffffffu;
    *CLINT_MTIMECMP_LO = (unsigned int)(v & 0xffffffffu);
    *CLINT_MTIMECMP_HI = (unsigned int)(v >> 32);
}

/* Von trap.S aufgerufen, RISC-V-Aufrufkonvention: darf a0-a7/t0-t6/ra beliebig benutzen, die
   Register sind bereits gerettet. */
void trap_c_handler(void)
{
    unsigned int cause;
    __asm__ volatile ("csrr %0, mcause" : "=r"(cause));

    /* Bit 31 = Interrupt (statt Ausnahme). Wir haben nur MTIE freigegeben, koennten also nur
       hierher kommen -- die Pruefung ist trotzdem billig und macht die Annahme sichtbar, statt
       sie stillschweigend vorauszusetzen. */
    if ((cause & 0x80000000u) == 0) return;

    interrupt_count++;
    next_cmp += DELTA;
    clint_set_mtimecmp(next_cmp);
}

int main(void)
{
    extern void trap_entry(void);

    __asm__ volatile ("csrw mtvec, %0" :: "r"(trap_entry));

    next_cmp = DELTA;
    clint_set_mtimecmp(next_cmp);

    __asm__ volatile ("csrs mie, %0" :: "r"(MIE_MTIE));
    __asm__ volatile ("csrs mstatus, %0" :: "r"(MSTATUS_MIE));

    uart_puts("Q9 RISC-V: CLINT-Timer-Interrupt-Test\n");

    while (interrupt_count < TARGET_COUNT) {
        __asm__ volatile ("wfi");
    }

    /* Beide Freigaben zurueck -- ab hier darf kein weiterer Timer-Interrupt mehr eintreffen,
       damit das abschliessende "wfi" (in start.S) sauber im Stromsparzustand bleibt. */
    __asm__ volatile ("csrc mie, %0" :: "r"(MIE_MTIE));
    __asm__ volatile ("csrc mstatus, %0" :: "r"(MSTATUS_MIE));

    uart_puts("Interrupts behandelt: ");
    uart_putdec(interrupt_count);
    uart_puts("  (erwartet 5)\n");

    return 0;
}

/* EOF timer_test.c                                                                    Ver. 1.00 */
