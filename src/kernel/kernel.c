//════════════════════════════════════════════════════════════════════════════════════════════════
// File:   kernel.c                                                                        Ver. 1.00
// Owner:  AF
// Desc.:  Q9-Kernel, Phase 0: Boot-Banner + Echo-Loop über die HAL-Konsole.
//         Beweist die Architektur: identische Kernel-Quelle auf allen Targets.
//
// Call:   q9_kernel_init(); danach zyklisch q9_kernel_step();
//
// Edition History
//─────────┬──────┬────────────────────────────────────────────────────────────────────────┬──────
// Date    │ Ver. │ Description                                                            │ By
//─────────┼──────┼────────────────────────────────────────────────────────────────────────┼──────
// 26-07-02│ 1.00 │ Initiale Version: Banner, Echo-Loop                                    │ CF
//═════════╧══════╧════════════════════════════════════════════════════════════════════════╧══════

#include "../hal/q9_hal.h"
#include "kernel.h"

//╔══════════════════════════════════════════════════════════════════════════════════════════════╗
//║ INTERNAL HELPERS                                                                             ║
//╚══════════════════════════════════════════════════════════════════════════════════════════════╝

//────────────────────────────────────────────────────────────────────────────────────────────────
// Function: con_puts
// Desc.:    Gibt einen nullterminierten String auf der HAL-Konsole aus ('\n' wird zu CR+LF).
// Call:     con_puts("text\n")
//────────────────────────────────────────────────────────────────────────────────────────────────
static void con_puts(const char *s)
{
    while (*s) {
        if (*s == '\n') {
            q9_hal_con_put('\r');
        }
        q9_hal_con_put(*s++);
    }
}

//╔══════════════════════════════════════════════════════════════════════════════════════════════╗
//║ KERNEL API                                                                                   ║
//╚══════════════════════════════════════════════════════════════════════════════════════════════╝

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_kernel_init
// Desc.:    Kernel-Initialisierung, gibt das Boot-Banner aus.
// Call:     q9_kernel_init()
//════════════════════════════════════════════════════════════════════════════════════════════════
void q9_kernel_init(void)
{
    con_puts("\n");
    con_puts("  ═══════════════════════════════════════\n");
    con_puts("   Q9 v" Q9_VERSION " alpha\n");
    con_puts("   modulares Mini-OS in OS-9-Tradition\n");
    con_puts("  ═══════════════════════════════════════\n");
    con_puts("  target: ");
    con_puts(q9_hal_target());
    con_puts("\n\n");
    con_puts("  Phase 0: Echo-Modus. Tippe etwas!\n\n");
    con_puts("Q9> ");
}

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_kernel_step
// Desc.:    Ein Kernel-Tick: alle anstehenden Konsolen-Zeichen einlesen und zurückgeben (Echo).
// Call:     q9_kernel_step()
//════════════════════════════════════════════════════════════════════════════════════════════════
void q9_kernel_step(void)
{
    int c;

    while ((c = q9_hal_con_get()) >= 0) {
        if (c == '\r' || c == '\n') {
            con_puts("\nQ9> ");
        } else if (c == 0x7f || c == 0x08) {           /* backspace / delete                     */
            con_puts("\b \b");
        } else {
            q9_hal_con_put((char)c);
        }
    }
}

//────────────────────────────────────────────────────────────────────────────────────────────────
// EOF kernel.c                                                                            Ver. 1.00
//────────────────────────────────────────────────────────────────────────────────────────────────
