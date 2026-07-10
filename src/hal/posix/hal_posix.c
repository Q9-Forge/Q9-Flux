//════════════════════════════════════════════════════════════════════════════════════════════════
// File:   hal_posix.c                                                                     Ver. 1.20
// Owner:  AF
// Desc.:  HAL-Implementierung für den nativen POSIX-Build (macOS/Linux, clang/gcc).
//         Enthält auch den Host: main() treibt den Kernel-Step-Loop.
//
// Call:   build/native/q9.exe [--selftest]
//
// Edition History
//─────────┬──────┬────────────────────────────────────────────────────────────────────────┬──────
// Date    │ Ver. │ Description                                                            │ By
//─────────┼──────┼────────────────────────────────────────────────────────────────────────┬──────
// 26-07-03│ 1.00 │ 1.10: Konsole (termios raw+nonblocking), Timer, Disk-Image, Selftest    │ CF
// 26-07-10│ 1.10 │ 5.7: TX-Ringpuffer fuer q9_hal_con_put (nicht-blockierendes write()),    │ CF
//         │      │ statt pro Zeichen zu blockieren -- Gegenstueck zum RX-FIFO (cb030.c)     │
// 26-07-10│ 1.11 │ 5.8: Ctrl-]-Host-Escape + DEL->BS-Mapping in q9_hal_con_get()            │ CF
// 26-07-10│ 1.20 │ 5.9: q9_hal_sleep_ms (usleep) fuer die CB030-Idle-Drossel                │ CF
//═════════╧══════╧════════════════════════════════════════════════════════════════════════╧══════

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <termios.h>
#include <signal.h>

#include "../q9_hal.h"
#include "../../kernel/kernel.h"
#ifdef Q9_HAVE_M68K
#include "../../kernel/cb030run.h"
#endif

#define DISK_IMAGE "q9disk.img"

static FILE *disk = NULL;
static struct termios orig_termios;
static int termios_saved = 0;

/* 5.7: TX-Ringpuffer -- Gegenstueck zum RX-FIFO in cb030.c. q9_hal_con_put() darf den Haupt-Loop
   nie blockieren (sonst friert bei einem langsamen/gestockten Terminal-Leser die GESAMTE Emulation
   ein, s. ARBEITSPLAN 5.7). 256 KiB reichen fuer jeden realistischen Ausgabe-Burst zwischen zwei
   Haupt-Loop-Durchlaeufen bei weitem -- Ueberlauf wird (wie beim RX-FIFO) nur gezaehlt, nicht
   blockierend erzwungen. */
#define TX_BUF_SIZE (256u * 1024u)
static uint8_t tx_buf[TX_BUF_SIZE];
static uint32_t tx_head = 0;
static uint32_t tx_tail = 0;
static uint32_t tx_count = 0;
static uint32_t tx_overflow = 0;
static int tx_nonblock_set = 0;

//────────────────────────────────────────────────────────────────────────────────────────────────
// Function: tx_drain_nonblocking
// Desc.:    Schreibt so viel wie moeglich aus dem TX-Ringpuffer nicht-blockierend nach STDOUT.
//           Bricht bei EAGAIN/EWOULDBLOCK (Leser haelt nicht mit) sofort ab -- der Rest bleibt
//           im Puffer und wird beim naechsten Aufruf weiterversucht.
//────────────────────────────────────────────────────────────────────────────────────────────────
static void tx_drain_nonblocking(void)
{
    if (!tx_nonblock_set) {
        fcntl(STDOUT_FILENO, F_SETFL, fcntl(STDOUT_FILENO, F_GETFL, 0) | O_NONBLOCK);
        tx_nonblock_set = 1;
    }
    while (tx_count > 0) {
        uint32_t chunk = (tx_head < tx_tail) ? (tx_tail - tx_head) : (TX_BUF_SIZE - tx_head);
        ssize_t  n;

        if (chunk > tx_count) {
            chunk = tx_count;
        }
        n = write(STDOUT_FILENO, &tx_buf[tx_head], chunk);
        if (n < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                break;                                    /* Leser blockiert -- Rest bleibt liegen  */
            }
            break;                                        /* anderer Fehler: dieser Durchlauf ist hin */
        }
        if (n == 0) {
            break;
        }
        tx_head   = (tx_head + (uint32_t)n) % TX_BUF_SIZE;
        tx_count -= (uint32_t)n;
    }
}

//╔══════════════════════════════════════════════════════════════════════════════════════════════╗
//║ HAL IMPLEMENTATION                                                                           ║
//╚══════════════════════════════════════════════════════════════════════════════════════════════╝

//────────────────────────────────────────────────────────────────────────────────────────────────
// Function: restore_termios
// Desc.:    Stellt die ursprünglichen Terminal-Einstellungen wieder her (atexit-Handler).
//────────────────────────────────────────────────────────────────────────────────────────────────
static void restore_termios(void)
{
    if (termios_saved) {
        tcsetattr(STDIN_FILENO, TCSANOW, &orig_termios);
    }
}

static void restore_termios_on_signal(int sig)
{
    restore_termios();
    signal(sig, SIG_DFL);
    raise(sig);
}

void q9_hal_init(void)
{
    struct termios raw;

    if (tcgetattr(STDIN_FILENO, &orig_termios) == 0) {
        termios_saved = 1;
        atexit(restore_termios);
        signal(SIGTERM, restore_termios_on_signal);
        signal(SIGHUP,  restore_termios_on_signal);

        raw = orig_termios;
        raw.c_lflag &= (tcflag_t)~(ICANON | ECHO | ISIG | IEXTEN);
                                                           /* kein Zeilenpuffer, kein lokales Echo,
                                                              keine Host-Signale: Ctrl-C/Z/\\ sollen
                                                              als echte Eingaben bei OS-9 ankommen  */
        raw.c_iflag &= (tcflag_t)~(ICRNL | INLCR | IGNCR | IXON | IXOFF | ISTRIP);
                                                           /* 5.2e: transparente Leitung — Enter muss
                                                              als CR (0x0D) durchkommen (OS-9s SCF
                                                              erwartet CR als Zeilenende; ICRNL wuerde
                                                              es zu LF verbiegen), keine LF/CR-Um-
                                                              schreibung, kein Ctrl-S/Q-Abfangen     */
        raw.c_oflag &= (tcflag_t)~OPOST;
        raw.c_cc[VMIN]  = 0;                               /* read() liefert sofort zurueck          */
        raw.c_cc[VTIME] = 0;
        tcsetattr(STDIN_FILENO, TCSANOW, &raw);
    }
    fcntl(STDIN_FILENO, F_SETFL, fcntl(STDIN_FILENO, F_GETFL, 0) | O_NONBLOCK);
}

void q9_hal_con_put(char c)
{
    tx_drain_nonblocking();                               /* zuerst Platz schaffen               */
    if (tx_count >= TX_BUF_SIZE) {
        tx_overflow++;                                    /* Puffer voll -- wie uart_rx_overflow  */
        return;
    }
    tx_buf[tx_tail] = (uint8_t)c;
    tx_tail = (tx_tail + 1u) % TX_BUF_SIZE;
    tx_count++;
    tx_drain_nonblocking();                                /* gleich versuchen loszuwerden         */
}

void q9_hal_con_flush(void)
{
    tx_drain_nonblocking();
}

int q9_hal_con_tx_ready(void)
{
    return tx_count < TX_BUF_SIZE;
}

int q9_hal_con_tx_empty(void)
{
    tx_drain_nonblocking();
    return tx_count == 0;
}

int q9_hal_con_get(void)
{
    unsigned char c;
    ssize_t       n = read(STDIN_FILENO, &c, 1);

    if (n == 1 && c == 0x1d) {                         /* Host-Escape: Ctrl-] beendet den Emulator */
        fputs("\nq9: Host-Escape Ctrl-] — Emulator beendet.\n", stderr);
        exit(0);
    }
    if (n == 1 && c == 0x7f) {
        c = 0x08;                                      /* macOS Backspace (DEL) -> OS-9 BS       */
    }
    return (n == 1) ? c : -1;
}

uint32_t q9_hal_ticks_ms(void)
{
    struct timespec ts;

    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint32_t)((uint64_t)ts.tv_sec * 1000u + (uint64_t)ts.tv_nsec / 1000000u);
}

void q9_hal_sleep_ms(uint32_t ms)
{
    usleep((useconds_t)ms * 1000u);
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
#if defined(__APPLE__)
    return "native-macos";
#else
    return "native-linux";
#endif
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
        q9_hal_init();                                 /* termios raw — die UART braucht das     */
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
        q9_hal_con_flush();                                 /* 5.7: TX-Rest aus vorherigen Runden   */
        usleep(1000);                                      /* don't burn a whole core               */
    }
}

//────────────────────────────────────────────────────────────────────────────────────────────────
// EOF hal_posix.c                                                                         Ver. 1.20
//────────────────────────────────────────────────────────────────────────────────────────────────
