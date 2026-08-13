//════════════════════════════════════════════════════════════════════════════════════════════════
// File:   hal_windows.c                                                                   Ver. 1.30
// Owner:  AF
// Desc.:  HAL-Implementierung für den Windows-Host-Build (w64devkit/gcc, conio).
//         Enthält auch den Host: main() treibt den Kernel-Step-Loop.
//
// Call:   build\windows\q9.exe [--selftest]   (gebaut per "make host")
//
// Edition History
//─────────┬──────┬────────────────────────────────────────────────────────────────────────┬──────
// Date    │ Ver. │ Description                                                            │ By
//─────────┼──────┼────────────────────────────────────────────────────────────────────────┬──────
// 26-07-02│ 1.00 │ Initiale Version: Konsole (conio), Timer, Disk-Image, Selftest         │ CF
// 26-07-03│ 1.10 │ 1.9: q9_hal_time via localtime                                         │ CF
// 26-07-10│ 1.11 │ 5.7/5.9: HAL-Interface-Erfuellung (con_flush/tx_ready/tx_empty trivial, │ CF
//         │      │ q9_hal_sleep_ms via Sleep()) -- kein echter TX-Puffer auf diesem Target │
// 26-08-06│ 1.20 │ Ctrl-]-Host-Escape + Ctrl-^-Debug-Dump (5.8) nachgezogen -- war bisher   │ AF
//         │      │ nur im POSIX-HAL (hal_posix.c) implementiert, unter Windows liess sich   │
//         │      │ der Emulator per Sondertaste gar nicht beenden (Andreas' Bugreport)      │
// 26-08-06│ 1.21 │ timeBeginPeriod(1) gegen Windows' grobe 15.6ms-Timer-Aufloesung (machte   │ AF
//         │      │ q9_hal_sleep_ms(1) im Idle-Pfad effektiv 15x langsamer, Performance-      │
//         │      │ Bugreport); F10/F9 als Layout-unabhaengige Alternative zu Ctrl-]/Ctrl-^   │
//         │      │ (auf DE-Tastaturen liegt ']' auf AltGr+9 = technisch bereits Strg+Alt,    │
//         │      │ Ctrl-] ist so nicht sauber erzeugbar)                                     │
// 26-08-07│ 1.22 │ F10 kollidierte auf Andreas' Rechner mit einem systemweiten Snip-Tool-     │ AF
//         │      │ Hotkey (kam nie bei q9.exe an) -- Host-Escape/Debug-Taste jetzt per         │
//         │      │ Q9_QUIT_SCAN/Q9_DEBUGDUMP_SCAN (Hex-Scancode) konfigurierbar, Default        │
//         │      │ bleibt F10/F9                                                               │
// 26-08-07│ 1.23 │ F1..F10 kommen bei Andreas offenbar generell nie bei _getch() an (F11/F12    │ AF
//         │      │ hardwareseitig auf Home/End gemappt) -- zusaetzlich Strg+<Buchstabe> als     │
//         │      │ Host-Escape (Q9_QUIT_CTRL, Default Ctrl-Q), layoutunabhaengig da Buchstaben   │
//         │      │ (anders als ']') auf jeder Tastatur ohne AltGr erreichbar sind                │
// 26-08-09│ 1.24 │ Cursor als ^P/^N/^B/^F fuer WinEd/umacs als Windows-Standard; ANSI optional  │ AF
// 26-08-09│ 1.29 │ Nicht funktionierende Windows-Editor-Makros entfernt; Cursor bleibt WinEd-kompatibel│ AF

//═════════╧══════╧════════════════════════════════════════════════════════════════════════╧══════

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <conio.h>
#include <windows.h>
#include <mmsystem.h>                                   /* timeBeginPeriod/timeEndPeriod, -lwinmm */

#include "../q9_hal.h"
#include "../../kernel/q9boardrun.h"
#include "../../kernel/boardcfg.h"

#define DISK_IMAGE "local_images/q9disk.img"

static FILE *disk = NULL;
static unsigned char keybuf[8];
static int keybuf_head = 0;
static int keybuf_tail = 0;
static int quit_scan      = 0x44;                      /* F10 (Default), s. q9_scan_from_env  */
static int debugdump_scan = 0x43;                      /* F9  (Default)                       */
static int quit_ctrl      = 0x11;                      /* Ctrl-Q (Default), s. q9_ctrl_from_env */
static int wined_keymode  = 0;                         /* WinEd-Cursormodus, s. q9_hal_init    */

static void q9_timer_resolution_restore(void) { timeEndPeriod(1); }

/* Andreas' Bugreports (2026-08-06/07): F10 wird auf seinem Rechner von einem Snip/Screenshot-Tool
   systemweit als globaler Hotkey abgefangen, kommt bei q9.exe gar nicht erst an -- und F1..F10
   generell scheinen dort (Windows Terminal/Laptop-Fn-Belegung?) nie bei _getch() anzukommen, F11/F12
   sind hardwareseitig auf Home/End gemappt. Tastenbelegungen fuer sowas kollidieren erfahrungsgemaess
   unvorhersehbar von Maschine zu Maschine -- deshalb ALLES per Env-Var konfigurierbar statt eine feste
   Taste zu raten: Q9_QUIT_SCAN/Q9_DEBUGDUMP_SCAN nehmen den _getch()-Scan-Code (hex) einer Extended-
   Taste (Default F10/F9); Q9_QUIT_CTRL nimmt einen Buchstaben A-Z fuer Strg+<Buchstabe> (Default Q --
   Ctrl-C selbst bewusst NICHT als Default, weil Andreas' Ctrl-C-Testlauf bereits belegt, dass OS-9
   das als normales Zeichen sieht und mit einem System-Reset reagiert). Strg+Buchstabe ist plattform-/
   layoutunabhaengig, da Buchstaben (anders als Satzzeichen wie ']') auf JEDER Tastaturbelegung ohne
   AltGr erreichbar sind -- und kommt nachweislich bei _getch() an (Andreas' Ctrl-C ist ja
   durchgeschlagen). */
static int q9_scan_from_env(const char *var, int fallback)
{
    const char *s = getenv(var);
    long        v;
    char       *end;
    if (!s || !s[0]) return fallback;
    v = strtol(s, &end, 16);
    return (*end == '\0' && v > 0 && v < 256) ? (int)v : fallback;
}

static int q9_ctrl_from_env(const char *var, int fallback)
{
    const char *s = getenv(var);
    char        c;
    if (!s || !s[0] || s[1] != '\0') return fallback;
    c = (char)toupper((unsigned char)s[0]);
    return (c >= 'A' && c <= 'Z') ? (c - 'A' + 1) : fallback;
}

/* WinEd 3.9 verarbeitet ANSI-Cursorfolgen nicht als einzelne Tasten, sondern
   kennt fuer die vier Richtungen direkt seine Emacs-Bindungen. */
static int q9_wined_keymode_from_env(void)
{
    const char *mode = getenv("Q9_KEYMODE");

    /* WinEd und uemacs verstehen die klassischen Emacs-Steuercodes.
     * ANSI ist fuer andere terminalorientierte Programme weiterhin waehlbar. */
    return !mode || (strcmp(mode, "ansi") != 0 && strcmp(mode, "vt100") != 0);
}

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

    /* Andreas' Performance-Report (2026-08-06): Windows' System-Timer laeuft per Default mit
       15.6ms-Aufloesung -- q9_hal_sleep_ms(1) im CPU-Idle-Pfad (q9boardrun.c, 5.9) schlaeft dadurch
       oft ~15ms statt 1ms, macht das ganze Idle-Verhalten bis zu 15x langsamer als auf macOS/Linux
       (dort ist usleep/nanosleep von Haus aus fein genug). timeBeginPeriod(1) hebt die System-weite
       Timer-Aufloesung fuer die Laufzeit dieses Prozesses auf 1ms an (Standard-Fix, s. MSDN
       "Timer-Queue Timers"/timeBeginPeriod) -- braucht winmm.lib (-lwinmm im Makefile). */
    timeBeginPeriod(1);
    atexit(q9_timer_resolution_restore);               /* auch exit(0) im Ctrl-]-Pfad rueckt es sauber gerade */

    quit_scan      = q9_scan_from_env("Q9_QUIT_SCAN",      0x44);  /* F10 */
    debugdump_scan = q9_scan_from_env("Q9_DEBUGDUMP_SCAN",  0x43); /* F9  */
    quit_ctrl      = q9_ctrl_from_env("Q9_QUIT_CTRL",       0x11); /* Ctrl-Q */
    wined_keymode  = q9_wined_keymode_from_env();

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
        if (c == 0x1d) {                                /* Host-Escape: Ctrl-] beendet den Emulator
                                                              (5.8, bisher nur im POSIX-HAL portiert) */
            fputs("\nq9: Host-Escape Ctrl-] - Emulator beendet.\n", stderr);
            exit(0);
        }
        if (c == 0x1e) {                                /* Debug-Sondertaste: Ctrl-^ dumpt physischen */
            q9_dbg_dump_requested = 1;                  /* Kernel-Speicher (q9boardrun.c)                */
            return -1;                                  /* schlucken, nicht an den Gast weiterreichen  */
        }
        if (c != 0 && c == quit_ctrl) {                 /* Konfigurierbarer Strg-Buchstabe (Default
                                                              Ctrl-Q), layoutunabhaengige Alternative
                                                              zu Ctrl-], s. q9_ctrl_from_env oben */
            fprintf(stderr, "\nq9: Host-Escape (Ctrl-%c) - Emulator beendet.\n",
                    (char)('A' + quit_ctrl - 1));
            exit(0);
        }
        if (c == 0 || c == 0xe0) {
            int scan = _getch();
            /* Konfigurierbare Host-Escape/Debug-Taste (Default F10/F9, s. q9_scan_from_env) --
               eigene ifs statt switch-case, weil quit_scan/debugdump_scan zur Laufzeit aus der
               Umgebung kommen und damit keine Compile-Zeit-Konstanten fuer ein case-Label sind. */
            if (scan == quit_scan) {
                fprintf(stderr, "\nq9: Host-Escape (Scan-Code %#04x) - Emulator beendet.\n", scan);
                exit(0);
            }
            if (scan == debugdump_scan) {
                q9_dbg_dump_requested = 1;
                return -1;
            }
            switch (scan) {
            case 0x48:                                 /* Up */
                if (wined_keymode) keybuf_push(0x10); else keybuf_push_csi('A');
                break;
            case 0x50:                                 /* Down */
                if (wined_keymode) keybuf_push(0x0e); else keybuf_push_csi('B');
                break;
            case 0x4b:                                 /* Left */
                if (wined_keymode) keybuf_push(0x02); else keybuf_push_csi('D');
                break;
            case 0x4d:                                 /* Right */
                if (wined_keymode) keybuf_push(0x06); else keybuf_push_csi('C');
                break;
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

void q9_hal_sleep_ms(uint32_t ms)
{
    Sleep((DWORD)ms);                                      /* 5.9: Idle-Drossel           */
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
//           5.19: Der ERSTE Parameter OHNE fuehrendes "-" gibt eine Board-Config-Datei an
//           (Extension ".q9" wird angenommen, falls keine da ist, s. boardcfg.h). Darin stehen
//           ROM, Netz-Backend und MEHRERE CF-Images (rbf/pcf); der Emulator startet dann direkt
//           im Board-Modus. Die bestehenden Optionen bleiben und ueberschreiben die Config:
//           --rom <rom>, --cf <image> (Onboard-CF-Master), --net nat|vmnet (5.12, Default nat).
//           Ohne Config UND ohne --rom laeuft wie bisher der reine Q9-Kernel — Ende per Ctrl-C.
// Call:     q9.exe [<config[.q9]>] [--rom <rom>] [--cf <image>] [--net nat|vmnet]
//           q9.exe --selftest
//════════════════════════════════════════════════════════════════════════════════════════════════
int main(int argc, char **argv)
{
    const char *cfg_arg  = NULL;                       /* erster Positionsparameter (ohne '-')   */
    const char *rom_path = NULL;                        /* --rom                                  */
    const char *cf_path  = NULL;                        /* --cf                                   */
    const char *net_mode = NULL;                        /* --net                                  */

    for (int i = 1; i < argc; i++) {
        /* --rom ist die aktuelle Schreibweise; --cb030 bleibt als stilles Alias
           erhalten, damit aeltere Skripte/Testrezepte weiterlaufen. */
        if ((strcmp(argv[i], "--rom") == 0 || strcmp(argv[i], "--cb030") == 0) && i + 1 < argc) {
            rom_path = argv[++i];
        } else if (strcmp(argv[i], "--cf") == 0 && i + 1 < argc) {
            cf_path = argv[++i];
        } else if (strcmp(argv[i], "--net") == 0 && i + 1 < argc) {
            net_mode = argv[++i];
        } else if (argv[i][0] != '-' && cfg_arg == NULL) {
            cfg_arg = argv[i];                         /* 5.19: Board-Config-Datei               */
        }
    }

    if (cfg_arg == NULL && rom_path == NULL) {
        fprintf(stderr,
                "usage: %s <config[.q9]> | --rom <rom> [--cf <image>] [--net nat|vmnet]\n",
                argv[0]);
        return 1;
    }

    {
        q9_board_cfg_t  cfg;
        q9_board_cfg_t *cfgp = NULL;

        q9_board_cfg_default(&cfg);
        if (cfg_arg != NULL) {
            char path[Q9_CFG_PATH_MAX];
            char err[256];
            q9_board_cfg_resolve_path(cfg_arg, path, sizeof(path));
            if (q9_board_cfg_load(&cfg, path, err, sizeof(err)) != 0) {
                fprintf(stderr, "q9board: %s\n", err);
                return 1;
            }
            cfgp = &cfg;
        }
        q9_hal_init();
        return q9_board_boot(rom_path, cf_path, net_mode, cfgp);
    }
}

//────────────────────────────────────────────────────────────────────────────────────────────────
// EOF hal_windows.c                                                                       Ver. 1.30
//────────────────────────────────────────────────────────────────────────────────────────────────
