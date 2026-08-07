/* Minimaler Reproducer fuer den hostfwd-Absturz, s. Q9_VENDOR.md "Vertiefte Untersuchung".
 * KEIN Q9-Code -- reines libslirp+glib2, isoliert den Bug ausserhalb des Emulators.
 *
 * Bauen (nach third_party/slirp/windows/ vendorte Header/Libs, s. Q9_VENDOR.md):
 *   gcc -std=c99 -O0 -g \
 *     -Ithird_party/slirp/windows/include \
 *     -Ithird_party/slirp/windows/include/glib-2.0 \
 *     -Ithird_party/slirp/windows/lib/glib-2.0/include \
 *     repro_hostfwd_crash.c \
 *     -Lthird_party/slirp/windows/lib -lslirp -lglib-2.0 -lws2_32 \
 *     -o repro.exe
 *   # third_party/slirp/windows/bin/*.dll muessen neben repro.exe liegen (PATH oder cwd)
 *   ./repro.exe
 *
 * Erwartung (Stand 2026-08-07): "vor slirp_add_hostfwd" wird ausgegeben, dann Segfault
 * (0xC0000005) OHNE "nach slirp_add_hostfwd". slirp_new() selbst und normale Poll-Zyklen
 * OHNE hostfwd sind stabil (einfach den Block unten auskommentieren zum Gegentest).
 *
 * Fuer einen symbolisierten Backtrace: gegen einen SELBST GEBAUTEN Debug-libslirp linken
 * (third_party/slirp/windows/lib austauschen) und unter gdb laufen lassen:
 *   gdb -batch -ex run -ex bt -ex "frame 1" -ex "x/40i \$pc-80" ./repro.exe
 */
#include <winsock2.h>
#include <ws2tcpip.h>
#include <slirp/libslirp.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

static slirp_ssize_t my_send_packet(const void *buf, size_t len, void *opaque)
{
    (void)buf; (void)opaque;
    return (slirp_ssize_t)len;
}
static void my_guest_error(const char *msg, void *opaque)
{
    (void)opaque;
    printf("guest_error: %s\n", msg);
}
static int64_t my_clock_get_ns(void *opaque)
{
    (void)opaque;
    return (int64_t)GetTickCount64() * 1000000;
}
static void *my_timer_new(SlirpTimerCb cb, void *cb_opaque, void *opaque)
{
    (void)cb; (void)cb_opaque; (void)opaque;
    return (void *)1;                       /* Platzhalter -- reicht, da nie dereferenziert */
}
static void my_timer_free(void *t, void *o) { (void)t; (void)o; }
static void my_timer_mod(void *t, int64_t e, void *o) { (void)t; (void)e; (void)o; }

/* No-Op-Stubs fuer die "optionalen" SlirpCb-Felder -- im Standalone-Test verifiziert (per
   eigenem printf), dass KEINER davon je aufgerufen wird, bevor der Absturz passiert. */
static void my_notify(void *o) { (void)o; }
static void my_init_completed(Slirp *s, void *o) { (void)s; (void)o; }
static void my_register_poll_socket(slirp_os_socket s, void *o) { (void)s; (void)o; }
static void my_unregister_poll_socket(slirp_os_socket s, void *o) { (void)s; (void)o; }

int main(void)
{
    WSADATA wsa;
    WSAStartup(MAKEWORD(2, 2), &wsa);

    SlirpConfig cfg;
    memset(&cfg, 0, sizeof(cfg));
    cfg.version = 1;
    cfg.in_enabled = true;
    cfg.in6_enabled = false;
    inet_pton(AF_INET, "192.168.200.2", &cfg.vdhcp_start);
    inet_pton(AF_INET, "192.168.200.1", &cfg.vhost);
    inet_pton(AF_INET, "255.255.255.0", &cfg.vnetmask);
    cfg.vnetwork.s_addr = cfg.vhost.s_addr & cfg.vnetmask.s_addr;
    cfg.vnameserver = cfg.vhost;

    SlirpCb cb;
    memset(&cb, 0, sizeof(cb));
    cb.send_packet             = my_send_packet;
    cb.guest_error              = my_guest_error;
    cb.clock_get_ns             = my_clock_get_ns;
    cb.timer_new                = my_timer_new;
    cb.timer_free                = my_timer_free;
    cb.timer_mod                 = my_timer_mod;
    cb.notify                    = my_notify;
    cb.init_completed            = my_init_completed;
    cb.register_poll_socket      = my_register_poll_socket;
    cb.unregister_poll_socket    = my_unregister_poll_socket;

    printf("vor slirp_new\n"); fflush(stdout);
    Slirp *s = slirp_new(&cfg, &cb, NULL);
    printf("nach slirp_new: %p\n", (void *)s); fflush(stdout);
    if (!s) return 1;

    /* --- Kernfunktion (stabil, s. Q9_VENDOR.md): auskommentieren zum Gegentest --- */
    struct in_addr host_addr, guest_addr;
    host_addr.s_addr = 0;                   /* INADDR_ANY */
    guest_addr = cfg.vdhcp_start;
    printf("vor slirp_add_hostfwd\n"); fflush(stdout);
    int r = slirp_add_hostfwd(s, 0, host_addr, 2323, guest_addr, 23);
    printf("nach slirp_add_hostfwd: %d\n", r); fflush(stdout);   /* wird NIE erreicht (Stand 2026-08-07) */
    /* --- Ende hostfwd-Block --- */

    printf("OK\n");
    return 0;
}
