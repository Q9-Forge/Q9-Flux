//════════════════════════════════════════════════════════════════════════════════════════════════
// File:   dev_nil.c                                                                       Ver. 1.00
// Owner:  AF
// Desc.:  Null-Device /nil als internes Modul (OS-9-Vorbild: /nil). Schreiben verwirft die
//         Daten, Lesen liefert E$EOF. Zweiter Treiber im System — beweist, dass das
//         Device-Modell trägt (2 Treiber, 1 Schnittstelle).
//
// Call:   über die q9_drv_nil-Ops, registriert von q9_dev_init() (device.c)
//
// Edition History
//─────────┬──────┬────────────────────────────────────────────────────────────────────────┬──────
// Date    │ Ver. │ Description                                                            │ By
//─────────┼──────┼────────────────────────────────────────────────────────────────────────┬──────
// 26-07-03│ 1.00 │ Initiale Version                                                       │ CF
//═════════╧══════╧════════════════════════════════════════════════════════════════════════╧══════

#include "device.h"
#include "syscall.h"

//╔══════════════════════════════════════════════════════════════════════════════════════════════╗
//║ DRIVER OPERATIONS                                                                            ║
//╚══════════════════════════════════════════════════════════════════════════════════════════════╝

static int nil_init(q9_dev_t *dev)
{
    dev->storage = 0;                                  /* stateless                              */
    return 0;
}

static int nil_read(q9_dev_t *dev, uint8_t *buf, uint32_t *n)
{
    (void)dev; (void)buf; (void)n;
    return E_EOF;                                      /* /nil is always at end of file          */
}

static int nil_readln(q9_dev_t *dev, uint8_t *buf, uint32_t *n)
{
    (void)dev; (void)buf; (void)n;
    return E_EOF;
}

static int nil_write(q9_dev_t *dev, const uint8_t *buf, uint32_t *n)
{
    (void)dev; (void)buf; (void)n;                     /* discard, report all bytes written      */
    return 0;
}

//────────────────────────────────────────────────────────────────────────────────────────────────
// Function: nil_writln
// Desc.:    Verwirft die Daten, meldet aber wie I$WritLn nur bis einschließlich CR/LF.
// Call:     Treiber-Op writln
//────────────────────────────────────────────────────────────────────────────────────────────────
static int nil_writln(q9_dev_t *dev, const uint8_t *buf, uint32_t *n)
{
    uint32_t max = *n;
    uint32_t i;

    (void)dev;
    for (i = 0; i < max; i++) {
        if (buf[i] == '\r' || buf[i] == '\n') {
            i++;
            break;
        }
    }
    *n = i;
    return 0;
}

//╔══════════════════════════════════════════════════════════════════════════════════════════════╗
//║ MODULE EXPORT                                                                                ║
//╚══════════════════════════════════════════════════════════════════════════════════════════════╝

const q9_drv_t q9_drv_nil = {
    "nil",
    nil_init,
    nil_read,
    nil_write,
    nil_readln,
    nil_writln,
};

//────────────────────────────────────────────────────────────────────────────────────────────────
// EOF dev_nil.c                                                                           Ver. 1.00
//────────────────────────────────────────────────────────────────────────────────────────────────
