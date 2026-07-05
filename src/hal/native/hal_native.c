//════════════════════════════════════════════════════════════════════════════════════════════════
// File:   hal_native.c                                                                    Ver. 1.10
// Owner:  AF
// Desc.:  HAL-Implementierung für den nativen PC-Build (Windows, w64devkit/gcc).
//         Enthält auch den Host: main() treibt den Kernel-Step-Loop.
//
// Call:   build\native\q9.exe [--selftest]
//
// Edition History
//─────────┬──────┬────────────────────────────────────────────────────────────────────────┬──────
// Date    │ Ver. │ Description                                                            │ By
//─────────┼──────┼────────────────────────────────────────────────────────────────────────┬──────
// 26-07-02│ 1.00 │ Initiale Version: Konsole (conio), Timer, Disk-Image, Selftest         │ CF
// 26-07-03│ 1.10 │ 1.9: q9_hal_time via localtime                                         │ CF
//═════════╧══════╧════════════════════════════════════════════════════════════════════════╧══════

#include <stdio.h>
#include <string.h>
#include <time.h>
#include <conio.h>
#include <windows.h>

#include "../q9_hal.h"
#include "../../kernel/kernel.h"
#ifdef Q9_HAVE_M68K
#include "../../kernel/cb030run.h"
#endif

#define DISK_IMAGE "q9disk.img"

static FILE *disk = NULL;

//╔══════════════════════════════════════════════════════════════════════════════════════════════╗
//║ HAL IMPLEMENTATION                                                                           ║
//╚══════════════════════════════════════════════════════════════════════════════════════════════╝

void q9_hal_init(void)
{
    SetConsoleOutputCP(CP_UTF8);                       /* kernel output is a UTF-8 byte stream   */
}

void q9_hal_con_put(char c)
{
    fputc(c, stdout);
    fflush(stdout);
}

int q9_hal_con_get(void)
{
    if (_kbhit()) {
        return _getch();
    }
    return -1;
}

uint32_t q9_hal_ticks_ms(void)
{
    return (uint32_t)GetTickCount64();
}

//────────────────────────────────────────────────────────────────────────────────────────────────
// Function: disk_open
// Desc.:    Öffnet das Disk-Image lazy (legt es beim ersten Schreibzugriff an).
// Call:     disk_open(1) für Schreibzugriff, disk_open(0) für Lesen
//────────────────────────────────────────────────────────────────────────────────────────────────
static int disk_open(int for_write)
{
    if (disk) {
        return 0;
    }
    disk = fopen(DISK_IMAGE, "r+b");
    if (!disk && for_write) {
        disk = fopen(DISK_IMAGE, "w+b");
    }
    return disk ? 0 : -1;
}

int q9_hal_blk_read(uint32_t lba, void *buf)
{
    if (disk_open(0) != 0) {
        return -1;
    }
    if (fseek(disk, (long)(lba * Q9_BLK_SIZE), SEEK_SET) != 0) {
        return -1;
    }
    return fread(buf, Q9_BLK_SIZE, 1, disk) == 1 ? 0 : -1;
}

int q9_hal_blk_write(uint32_t lba, const void *buf)
{
    if (disk_open(1) != 0) {
        return -1;
    }
    if (fseek(disk, (long)(lba * Q9_BLK_SIZE), SEEK_SET) != 0) {
        return -1;
    }
    return fwrite(buf, Q9_BLK_SIZE, 1, disk) == 1 ? 0 : -1;
}

int q9_hal_time(q9_datetime_t *dt)
{
    time_t     now = time(0);
    struct tm *tm  = localtime(&now);

    if (!tm) {
        return -1;
    }
    dt->year  = (uint16_t)(tm->tm_year + 1900);
    dt->month = (uint8_t)(tm->tm_mon + 1);
    dt->day   = (uint8_t)tm->tm_mday;
    dt->hour  = (uint8_t)tm->tm_hour;
    dt->min   = (uint8_t)tm->tm_min;
    dt->sec   = (uint8_t)tm->tm_sec;
    return 0;
}

const char *q9_hal_target(void)
{
    return "native-win64";
}

//╔══════════════════════════════════════════════════════════════════════════════════════════════╗
//║ HOST (main loop)                                                                             ║
//╚══════════════════════════════════════════════════════════════════════════════════════════════╝

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: main
// Desc.:    Host-Loop: initialisiert HAL + Kernel und ruft q9_kernel_step() zyklisch auf.
//           Mit --selftest: 100 Ticks laufen lassen, "SELFTEST PASS" ausgeben, Exit 0.
//           Mit --cb030 <rom> [--cf <image>]: statt des Q9-Kernels das emulierte CB030-Board
//           mit dem angegebenen Boot-ROM starten (5.3, s. cb030run.h), optional mit eigenem
//           CF-Backing-Image statt "cb030_cf.img" (5.5a) — Ende per Ctrl-C.
// Call:     q9.exe [--selftest | --cb030 <rom-datei> [--cf <image>]]
//════════════════════════════════════════════════════════════════════════════════════════════════
int main(int argc, char **argv)
{
    int selftest = (argc > 1 && strcmp(argv[1], "--selftest") == 0);

#ifdef Q9_HAVE_M68K
    if (argc > 2 && strcmp(argv[1], "--cb030") == 0) {
        const char *cf_path = NULL;
        if (argc > 4 && strcmp(argv[3], "--cf") == 0) {
            cf_path = argv[4];
        }
        q9_hal_init();
        return q9_cb030_boot(argv[2], cf_path);
    }
#endif

    q9_hal_init();
    q9_kernel_init();

    if (selftest) {
        int fails = q9_kernel_selftest();
        for (int i = 0; i < 100; i++) {
            q9_kernel_step();
        }
        if (fails != 0) {
            printf("\nSELFTEST FAIL (%d)\n", fails);
            return 1;
        }
        printf("\nSELFTEST PASS\n");
        return 0;
    }

    for (;;) {
        q9_kernel_step();
        Sleep(1);                                      /* don't burn a whole core               */
    }
}

//────────────────────────────────────────────────────────────────────────────────────────────────
// EOF hal_native.c                                                                        Ver. 1.10
//────────────────────────────────────────────────────────────────────────────────────────────────
