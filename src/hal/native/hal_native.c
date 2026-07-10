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
static unsigned char keybuf[8];
static int keybuf_head = 0;
static int keybuf_tail = 0;

static BOOL WINAPI q9_console_ctrl_handler(DWORD event)
{
    if (event == CTRL_C_EVENT || event == CTRL_BREAK_EVENT) {
        return TRUE;
    }
    return FALSE;
}

static void keybuf_push(unsigned char c)
{
    int next = (keybuf_tail + 1) % (int)sizeof(keybuf);
    if (next != keybuf_head) {
        keybuf[keybuf_tail] = c;
        keybuf_tail = next;
    }
}

static int keybuf_pop(void)
{
    int c;
    if (keybuf_head == keybuf_tail) {
        return -1;
    }
    c = keybuf[keybuf_head];
    keybuf_head = (keybuf_head + 1) % (int)sizeof(keybuf);
    return c;
}

static void keybuf_push_csi(char final)
{
    keybuf_push(0x1b);
    keybuf_push('[');
    keybuf_push((unsigned char)final);
}

static void keybuf_push_csi_tilde(char code)
{
    keybuf_push(0x1b);
    keybuf_push('[');
    keybuf_push((unsigned char)code);
    keybuf_push('~');
}

//╔══════════════════════════════════════════════════════════════════════════════════════════════╗
//║ HAL IMPLEMENTATION                                                                           ║
//╚══════════════════════════════════════════════════════════════════════════════════════════════╝

void q9_hal_init(void)
{
    HANDLE input = GetStdHandle(STD_INPUT_HANDLE);
    DWORD  mode;

    SetConsoleOutputCP(CP_UTF8);                       /* kernel output is a UTF-8 byte stream   */
    SetConsoleCtrlHandler(q9_console_ctrl_handler, TRUE);

    if (input != INVALID_HANDLE_VALUE && GetConsoleMode(input, &mode)) {
        mode &= ~ENABLE_PROCESSED_INPUT;               /* pass Ctrl-C and extended keys to guest */
        SetConsoleMode(input, mode);
    }
}

void q9_hal_con_put(char c)
{
    fputc(c, stdout);
    fflush(stdout);
}

/* 5.7: kein Software-TX-Puffer auf diesem Target -- con_put oben ist bereits synchron/blockierend,
   also ist der Puffer immer sofort leer. Reine Interface-Erfuellung (s. q9_hal.h), Windows-seitig
   nicht Teil von Schritt 5.7. */
void q9_hal_con_flush(void) { }
int  q9_hal_con_tx_ready(void) { return 1; }
int  q9_hal_con_tx_empty(void) { return 1; }

int q9_hal_con_get(void)
{
    int queued = keybuf_pop();
    if (queued >= 0) {
        return queued;
    }

    if (_kbhit()) {
        int c = _getch();
        if (c == 0 || c == 0xe0) {
            int scan = _getch();
            switch (scan) {
            case 0x48: keybuf_push_csi('A'); break;     /* Up       */
            case 0x50: keybuf_push_csi('B'); break;     /* Down     */
            case 0x4b: keybuf_push_csi('D'); break;     /* Left     */
            case 0x4d: keybuf_push_csi('C'); break;     /* Right    */
            case 0x47: keybuf_push_csi('H'); break;     /* Home     */
            case 0x4f: keybuf_push_csi('F'); break;     /* End      */
            case 0x52: keybuf_push_csi_tilde('2'); break; /* Insert */
            case 0x53: keybuf_push_csi_tilde('3'); break; /* Delete */
            case 0x49: keybuf_push_csi_tilde('5'); break; /* PageUp */
            case 0x51: keybuf_push_csi_tilde('6'); break; /* PageDn */
            default: return -1;
            }
            return keybuf_pop();
        }
        return c;
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
