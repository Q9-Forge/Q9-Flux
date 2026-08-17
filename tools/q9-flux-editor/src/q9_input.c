//════════════════════════════════════════════════════════════════════════════════════════════════
// File:   q9_input.c                                                                      Ver. 1.20
// Owner:  Claudia
// Desc.:  Implementierung, siehe q9_input.h.
//
// Edition History
//─────────┬──────┬────────────────────────────────────────────────────────────────────────┬──────
// Date    │ Ver. │ Description                                                            │ By
//─────────┼──────┼────────────────────────────────────────────────────────────────────────┬──────
// 26-08-16│ 1.00 │ Erster Wurf                                                              │ Cld
// 26-08-16│ 1.10 │ q9_term_size + Q9_KEY_RESIZE (POSIX: SIGWINCH unterbricht read() per EINTR) │ Cld
// 26-08-17│ 1.20 │ wait_for_resize_settle() -- ~150ms Entprellung gegen Flackern bei per Maus  │ Cld
//         │      │ gezogenem Resize (viele SIGWINCH kurz hintereinander)                        │
//═════════╧══════╧════════════════════════════════════════════════════════════════════════╧══════
#include "q9_input.h"
#include <string.h>

//────────────────────────────────────────────────────────────────────────────────────────────────
// q9_input_decode -- reine Funktion, plattformunabhaengig, s. Kopfkommentar in q9_input.h.
//────────────────────────────────────────────────────────────────────────────────────────────────
q9_key_t q9_input_decode(const char *buf, int len, int more_may_follow, int *consumed)
{
    q9_key_t k;
    unsigned char b0;

    k.kind = Q9_KEY_NONE;
    k.ch   = 0;

    if (!buf || len <= 0) {
        if (consumed) { *consumed = 0; }
        return k;
    }
    b0 = (unsigned char)buf[0];

    if (b0 == 0x1b) {
        if (len == 1) {
            if (more_may_follow) {
                if (consumed) { *consumed = 0; }          /* noch nicht entscheidbar */
                return k;
            }
            k.kind = Q9_KEY_ESCAPE;
            if (consumed) { *consumed = 1; }
            return k;
        }
        if ((unsigned char)buf[1] != '[') {
            /* Kein Sequenzbeginn (z.B. Alt+Taste auf manchen Terminals sendet ESC+Zeichen) --
               das ESC allein zaehlt, das naechste Byte kommt beim naechsten Aufruf separat dran. */
            k.kind = Q9_KEY_ESCAPE;
            if (consumed) { *consumed = 1; }
            return k;
        }
        if (len == 2) {
            if (more_may_follow) {
                if (consumed) { *consumed = 0; }
                return k;
            }
            k.kind = Q9_KEY_UNKNOWN;                       /* "ESC [" abgebrochen/unvollstaendig */
            if (consumed) { *consumed = 2; }
            return k;
        }
        switch (buf[2]) {
            case 'A': k.kind = Q9_KEY_UP;    break;
            case 'B': k.kind = Q9_KEY_DOWN;  break;
            case 'C': k.kind = Q9_KEY_RIGHT; break;
            case 'D': k.kind = Q9_KEY_LEFT;  break;
            default:  k.kind = Q9_KEY_UNKNOWN; break;
        }
        if (consumed) { *consumed = 3; }
        return k;
    }

    if (b0 == '\r' || b0 == '\n') {
        k.kind = Q9_KEY_ENTER;
        if (consumed) { *consumed = 1; }
        return k;
    }
    if (b0 == '\t') {
        k.kind = Q9_KEY_TAB;
        if (consumed) { *consumed = 1; }
        return k;
    }
    if (b0 == 0x7f || b0 == 0x08) {
        k.kind = Q9_KEY_BACKSPACE;
        if (consumed) { *consumed = 1; }
        return k;
    }
    if (b0 == 0x03) {
        k.kind = Q9_KEY_CTRL_C;
        if (consumed) { *consumed = 1; }
        return k;
    }
    if (b0 >= 0x20 && b0 < 0x7f) {
        k.kind = Q9_KEY_CHAR;
        k.ch   = buf[0];
        if (consumed) { *consumed = 1; }
        return k;
    }
    k.kind = Q9_KEY_UNKNOWN;                               /* sonstiges Steuerzeichen -- verwerfen */
    if (consumed) { *consumed = 1; }
    return k;
}

#ifdef _WIN32

#include <conio.h>
#include <io.h>

int q9_input_init(void)
{
    if (!_isatty(_fileno(stdin))) {
        return -1;
    }
    /* _kbhit()/_getch() sind bereits "roh" -- kein Zeilenpuffer, kein Echo -- keine explizite
       Moduswechsel-API wie bei POSIX/termios noetig. */
    return 0;
}

void q9_input_shutdown(void)
{
    /* nichts zurueckzusetzen, s.o. */
}

q9_key_t q9_input_read_key(void)
{
    q9_key_t k;
    int c;

    k.ch = 0;
    c = _getch();

    if (c == 0 || c == 0xe0) {
        /* Erweiterte Taste: 0/0xE0-Praefix, dann der eigentliche Scan-Code -- gleiches Muster wie
           src/hal/windows/hal_windows.c q9_hal_con_get(). */
        int scan = _getch();
        switch (scan) {
            case 0x48: k.kind = Q9_KEY_UP;    break;
            case 0x50: k.kind = Q9_KEY_DOWN;  break;
            case 0x4b: k.kind = Q9_KEY_LEFT;  break;
            case 0x4d: k.kind = Q9_KEY_RIGHT; break;
            default:   k.kind = Q9_KEY_UNKNOWN; break;
        }
        return k;
    }

    if (c == '\r' || c == '\n') { k.kind = Q9_KEY_ENTER;     return k; }
    if (c == '\t')              { k.kind = Q9_KEY_TAB;       return k; }
    if (c == 0x7f || c == 0x08) { k.kind = Q9_KEY_BACKSPACE; return k; }
    /* UNSICHER (kein Windows-Host zum Testen hier): _getch() liefert Ctrl-C auf manchen Windows-
       Konsolen NICHT als Byte 0x03, sondern loest stattdessen den Standard-Strg-C-Handler aus,
       bevor _getch() ueberhaupt zurueckkehrt -- dann kommt dieser Zweig nie zum Zug. Muesste auf
       echtem Windows verifiziert werden (ggf. SetConsoleCtrlHandler noetig, um das abzufangen). */
    if (c == 0x03)               { k.kind = Q9_KEY_CTRL_C; return k; }
    if (c == 0x1b)                { k.kind = Q9_KEY_ESCAPE; return k; }
    if (c >= 0x20 && c < 0x7f)    { k.kind = Q9_KEY_CHAR; k.ch = (char)c; return k; }
    k.kind = Q9_KEY_UNKNOWN;
    return k;
}

int q9_term_size(int *rows, int *cols)
{
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);

    if (h == INVALID_HANDLE_VALUE || !GetConsoleScreenBufferInfo(h, &csbi)) {
        return -1;
    }
    if (rows) { *rows = (int)(csbi.srWindow.Bottom - csbi.srWindow.Top + 1); }
    if (cols) { *cols = (int)(csbi.srWindow.Right - csbi.srWindow.Left + 1); }
    return 0;
}

#else /* POSIX */

#include <termios.h>
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <time.h>

static struct termios orig_termios;
static int  termios_saved = 0;
static char pending_buf[8];                               /* Bytes, die q9_input_decode() beim
                                                               letzten Aufruf NICHT verbraucht hat
                                                               (z.B. das Zeichen nach einem einzelnen,
                                                               nicht sequenzeinleitenden ESC) */
static int  pending_len = 0;
static volatile sig_atomic_t g_resize_pending = 0;         /* von on_sigwinch gesetzt, in
                                                               q9_input_read_key() abgefragt+geleert  */

static void on_sigwinch(int sig)
{
    (void)sig;
    g_resize_pending = 1;                                  /* signalsicher: nur ein sig_atomic_t
                                                               setzen, keine weitere Arbeit im Handler */
}

#define Q9_RESIZE_DEBOUNCE_MS 150

/* Andreas' Beobachtung (2026-08-17): waehrend eines Zieh-Resizes feuert JEDE einzelne
   Groessenaenderung ein eigenes SIGWINCH -- ohne Entprellung wuerde die Hauptschleife bei jedem
   einzelnen Zwischenschritt neu zeichnen, was stark flackert. wait_for_resize_settle() wartet, bis
   fuer Q9_RESIZE_DEBOUNCE_MS KEIN weiteres SIGWINCH mehr eintrifft, bevor ueberhaupt EIN
   Q9_KEY_RESIZE zurueckgegeben wird -- ein schneller Zieh-Resize erzeugt so nur EINE Neuzeichnung,
   nachdem die Groesse sich beruhigt hat, statt vieler. Bewusst per nanosleep() statt select()/poll()
   auf dem stdin-fd -- wir wollen hier NICHT auf Tastatureingabe warten, nur auf das Abklingen der
   Resize-Bursts. */
static void wait_for_resize_settle(void)
{
    g_resize_pending = 0;
    for (;;) {
        struct timespec ts;
        ts.tv_sec  = 0;
        ts.tv_nsec = Q9_RESIZE_DEBOUNCE_MS * 1000000L;
        nanosleep(&ts, NULL);                              /* kann durch ein weiteres SIGWINCH per
                                                                EINTR vorzeitig enden -- unschaedlich,
                                                                wir pruefen danach den Zustand statt
                                                                uns auf die exakte Schlafzeit zu
                                                                verlassen */
        if (!g_resize_pending) {
            return;                                         /* keine weiteren Resizes -- fertig */
        }
        g_resize_pending = 0;                               /* neuer Resize -- von vorne warten */
    }
}

/* Bewusst sigaction() statt signal() fuer SIGWINCH: einige signal()-Implementierungen (u.a. macOS/
   BSD-Erbe) installieren per Default mit automatischem Syscall-Neustart (SA_RESTART-aequivalent) --
   ein blockierender read() wuerde dann NICHT mit EINTR abbrechen, sondern transparent weiterlaufen,
   und das Resize-Ereignis bliebe bis zum naechsten ECHTEN Tastendruck unbemerkt (widerspricht
   Andreas' Wunsch nach sofortiger Reaktion). sigaction() mit sa_flags=0 (kein SA_RESTART) erzwingt
   das gewuenschte EINTR-Verhalten explizit, portabel. Real per Pseudo-Terminal-Test gefunden (mit
   signal() kam RESIZE nie an, bis eine Taste gedrueckt wurde) -- s. Q9FLUX_EDITOR_de.md. */
static void install_sigwinch_handler(void)
{
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = on_sigwinch;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;                                       /* explizit OHNE SA_RESTART */
    sigaction(SIGWINCH, &sa, NULL);
}

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

int q9_input_init(void)
{
    struct termios raw;

    if (!isatty(STDIN_FILENO)) {
        return -1;
    }
    if (tcgetattr(STDIN_FILENO, &orig_termios) != 0) {
        return -1;
    }
    termios_saved = 1;
    atexit(restore_termios);
    signal(SIGTERM, restore_termios_on_signal);
    signal(SIGHUP,  restore_termios_on_signal);
    install_sigwinch_handler();
    g_resize_pending = 0;

    raw = orig_termios;
    raw.c_lflag &= (tcflag_t)~(ICANON | ECHO | ISIG | IEXTEN);
                                                            /* kein Zeilenpuffer, kein Echo, kein
                                                               SIGINT bei Ctrl-C (kommt stattdessen
                                                               als Byte 0x03 an, s. q9_input_decode) */
    raw.c_iflag &= (tcflag_t)~(ICRNL | INLCR | IXON | IXOFF);
    raw.c_cc[VMIN]  = 1;                                   /* blockierend: mind. 1 Byte pro read()   */
    raw.c_cc[VTIME] = 0;
    tcsetattr(STDIN_FILENO, TCSANOW, &raw);
    pending_len = 0;
    return 0;
}

void q9_input_shutdown(void)
{
    restore_termios();
}

q9_key_t q9_input_read_key(void)
{
    char buf[8];
    int  len;
    int  consumed;
    q9_key_t k;
    ssize_t r;

    if (g_resize_pending) {
        /* Von einem frueheren SIGWINCH stehengeblieben (s.u.) -- erst entprellen (s.
           wait_for_resize_settle()), dann melden. */
        wait_for_resize_settle();
        k.kind = Q9_KEY_RESIZE;
        k.ch   = 0;
        return k;
    }

    if (pending_len > 0) {
        memcpy(buf, pending_buf, (size_t)pending_len);
        len = pending_len;
        pending_len = 0;
    } else {
        r = read(STDIN_FILENO, buf, 1);
        if (r < 0 && errno == EINTR) {
            /* Ein Signal (in der Praxis: SIGWINCH, s.o.) hat den blockierenden read() unterbrochen,
               BEVOR irgendein Byte ankam -- kein Byte verloren/geraten. Erst entprellen, dann melden. */
            wait_for_resize_settle();
            k.kind = Q9_KEY_RESIZE;
            k.ch   = 0;
            return k;
        }
        if (r <= 0) {
            k.kind = Q9_KEY_EOF;
            k.ch   = 0;
            return k;
        }
        len = 1;
    }

    if ((unsigned char)buf[0] == 0x1b) {
        /* Moeglicher Sequenzbeginn -- kurz (100ms) mit Timeout nachlesen statt blockierend, um ein
           einzelnes ESC von einer Pfeiltasten-Sequenz zu unterscheiden (s. q9_input.h Kopfkommentar). */
        struct termios t;
        tcgetattr(STDIN_FILENO, &t);
        t.c_cc[VMIN]  = 0;
        t.c_cc[VTIME] = 1;                                 /* Zehntelsekunden -> 100ms */
        tcsetattr(STDIN_FILENO, TCSANOW, &t);

        for (;;) {
            int used = 0;
            q9_key_t maybe = q9_input_decode(buf, len, 1, &used);
            (void)maybe;
            if (used > 0) {
                break;                                      /* Sequenz (oder lone-ESC-Sonderfall) klar */
            }
            if (len >= (int)sizeof(buf)) {
                break;                                      /* Puffer voll -- defensiv abbrechen       */
            }
            r = read(STDIN_FILENO, buf + len, 1);
            if (r <= 0) {
                break;                                      /* Timeout oder EOF -- Warten beenden      */
            }
            len++;
        }

        t.c_cc[VMIN]  = 1;                                  /* zurueck in den normalen Blockier-Modus  */
        t.c_cc[VTIME] = 0;
        tcsetattr(STDIN_FILENO, TCSANOW, &t);
    }

    k = q9_input_decode(buf, len, 0, &consumed);
    if (consumed < len) {
        /* Ueberschuss fuer den naechsten Aufruf aufheben (z.B. das Zeichen nach einem einzelnen,
           nicht sequenzeinleitenden ESC, s. q9_input_decode()-Kommentar zu "ESC gefolgt von != '['"). */
        memmove(pending_buf, buf + consumed, (size_t)(len - consumed));
        pending_len = len - consumed;
    }
    return k;
}

int q9_term_size(int *rows, int *cols)
{
    struct winsize ws;

    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) != 0) {
        return -1;
    }
    if (ws.ws_row == 0 || ws.ws_col == 0) {
        return -1;                                          /* z.B. stdout umgeleitet/kein TTY        */
    }
    if (rows) { *rows = ws.ws_row; }
    if (cols) { *cols = ws.ws_col; }
    return 0;
}

#endif /* _WIN32 */

//────────────────────────────────────────────────────────────────────────────────────────────────
// EOF q9_input.c                                                                          Ver. 1.20
//────────────────────────────────────────────────────────────────────────────────────────────────
