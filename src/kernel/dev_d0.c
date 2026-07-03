//════════════════════════════════════════════════════════════════════════════════════════════════
// File:   dev_d0.c                                                                       Ver. 1.00
// Owner:  AF
// Desc.:  Block-Device /d0 als internes Modul (OS-9-Vorbild: RBF-Treiber, roh ohne Dateisystem).
//         Reiner Blockzugriff über I$GetStt/I$SetStt (SS.BlkRd/SS.BlkWr), delegiert an die HAL
//         (q9_hal_blk_read/write). Normales I$Read/I$Write ergibt (noch) keinen Sinn ohne VFS —
//         Phase 3.2 baut die Pfad-Routing-Schicht darüber.
//
// Call:   über die q9_drv_d0-Ops, registriert von q9_dev_init() (device.c)
//
// Edition History
//─────────┬──────┬────────────────────────────────────────────────────────────────────────┬──────
// Date    │ Ver. │ Description                                                            │ By
//─────────┼──────┼────────────────────────────────────────────────────────────────────────┼──────
// 26-07-03│ 1.00 │ 3.1: Initiale Version (SS.BlkRd/SS.BlkWr über q9_hal_blk_read/write)   │ CF
//═════════╧══════╧════════════════════════════════════════════════════════════════════════╧══════

#include "../hal/q9_hal.h"
#include "device.h"
#include "syscall.h"

//╔══════════════════════════════════════════════════════════════════════════════════════════════╗
//║ DRIVER OPERATIONS                                                                            ║
//╚══════════════════════════════════════════════════════════════════════════════════════════════╝

static int d0_init(q9_dev_t *dev)
{
    dev->storage = 0;                                   /* stateless, HAL haelt das Image offen   */
    return 0;
}

//────────────────────────────────────────────────────────────────────────────────────────────────
// Function: d0_read / d0_write / d0_readln / d0_writln
// Desc.:    Kein Byte-Strom ohne Dateisystem (kommt mit der VFS-Schicht, 3.2) — E$UnkSvc.
//────────────────────────────────────────────────────────────────────────────────────────────────
static int d0_read(q9_dev_t *dev, uint8_t *buf, uint32_t *n)
{
    (void)dev; (void)buf; (void)n;
    return E_UNKSVC;
}

static int d0_write(q9_dev_t *dev, const uint8_t *buf, uint32_t *n)
{
    (void)dev; (void)buf; (void)n;
    return E_UNKSVC;
}

static int d0_readln(q9_dev_t *dev, uint8_t *buf, uint32_t *n)
{
    (void)dev; (void)buf; (void)n;
    return E_UNKSVC;
}

static int d0_writln(q9_dev_t *dev, const uint8_t *buf, uint32_t *n)
{
    (void)dev; (void)buf; (void)n;
    return E_UNKSVC;
}

//────────────────────────────────────────────────────────────────────────────────────────────────
// Function: d0_getstat
// Desc.:    SS.BlkRd: liest Block d2.l (LBA) über die HAL nach a0 (Q9_BLK_SIZE Byte).
//           E$Param ohne Puffer, E$NotRdy bei HAL-Fehler (Image fehlt/Lesefehler).
// Call:     Treiber-Op getstat (I$GetStt)
//────────────────────────────────────────────────────────────────────────────────────────────────
static int d0_getstat(q9_dev_t *dev, uint32_t code, q9_regs_t *r)
{
    (void)dev;
    if (code != SS_BLKRD) {
        return E_UNKSVC;
    }
    if (!r->a[0]) {
        return E_PARAM;
    }
    if (q9_hal_blk_read(r->d[2], r->a[0]) != 0) {
        return E_NOTRDY;
    }
    return 0;
}

//────────────────────────────────────────────────────────────────────────────────────────────────
// Function: d0_setstat
// Desc.:    SS.BlkWr: schreibt Block d2.l (LBA) aus a0 über die HAL (Q9_BLK_SIZE Byte).
//           E$Param ohne Puffer, E$NotRdy bei HAL-Fehler.
// Call:     Treiber-Op setstat (I$SetStt)
//────────────────────────────────────────────────────────────────────────────────────────────────
static int d0_setstat(q9_dev_t *dev, uint32_t code, q9_regs_t *r)
{
    (void)dev;
    if (code != SS_BLKWR) {
        return E_UNKSVC;
    }
    if (!r->a[0]) {
        return E_PARAM;
    }
    if (q9_hal_blk_write(r->d[2], r->a[0]) != 0) {
        return E_NOTRDY;
    }
    return 0;
}

//╔══════════════════════════════════════════════════════════════════════════════════════════════╗
//║ MODULE EXPORT                                                                                ║
//╚══════════════════════════════════════════════════════════════════════════════════════════════╝

const q9_drv_t q9_drv_d0 = {
    "d0",
    d0_init,
    d0_read,
    d0_write,
    d0_readln,
    d0_writln,
    d0_getstat,
    d0_setstat,
};

//────────────────────────────────────────────────────────────────────────────────────────────────
// EOF dev_d0.c                                                                            Ver. 1.00
//────────────────────────────────────────────────────────────────────────────────────────────────
