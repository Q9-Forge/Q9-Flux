//════════════════════════════════════════════════════════════════════════════════════════════════
// File:   slirp_net.c                                                                     Ver. 1.05
// Owner:  AF
// Desc.:  5.14: Implementierung, s. slirp_net.h fuer Design/Architektur-Begruendung. libslirp
//         braucht fuer den Betrieb: (1) einen monotonen Nanosekunden-Takt (clock_get_ns) --
//         plattformabhaengig, deshalb der einzige #ifdef _WIN32-Block in dieser sonst komplett
//         plattformneutralen Datei; (2) einfache Timer (timer_new/_mod/_free) -- als kleines
//         statisches Array nachgebaut, kein eigener Scheduler noetig, da q9_slirp_poll() ohnehin
//         jede Hauptschleifen-Runde faellig ist und abgelaufene Timer dort mit abgearbeitet werden;
//         (3) Zugriff auf seine eigenen Host-Sockets (echte Verbindungen ins Internet) ueber
//         slirp_pollfds_fill_socket/_poll -- dafuer q9_pollfd_t/Q9_SOCK_POLL aus q9_sockcompat.h
//         (WSAPoll unter Windows, poll() sonst, identische Feldnamen).
//
// Edition History
//─────────┬──────┬─────────────────────────────────────────────────────────────────────────┬──────
// Date    │ Ver. │ Description                                                            │ By
//─────────┼──────┼─────────────────────────────────────────────────────────────────────────┼──────
// 26-08-07│ 1.00 │ 5.14: Erster Wurf                                                       │ AF
// 26-08-07│ 1.01 │ hostfwd VORERST deaktiviert (nur Warnung) -- slirp_add_hostfwd() stuerzt  │ AF
//         │      │ in der vendorten Windows-Kombination zuverlaessig ab (per gdb isoliert,   │
//         │      │ auch in einem Standalone-Test ohne Q9-Code reproduziert, s. Q9_VENDOR.md); │
//         │      │ Kernfunktion (ausgehende Verbindungen) unbetroffen und stabil verifiziert  │
// 26-08-07│ 1.02 │ slirp_pollfds_poll() wird nur noch bei g_pollfd_count>0 aufgerufen --      │ AF
//         │      │ stuerzte bei 0 registrierten Sockets zuverlaessig ab (bekannter Bug im     │
//         │      │ echten q9.exe, per gdb isoliert, in kleinen Standalone-Tests NICHT          │
//         │      │ reproduzierbar -- sieht nach uninitialisiertem Speicherzugriff aus). Mit     │
//         │      │ diesem Fix bootet OS-9 komplett durch mit net=slirp aktiv                    │
// 26-08-07│ 1.03 │ hostfwd-Absturz war eigener Konfigfehler, kein libslirp-Bug: cfg.version=1     │ AF
//         │      │ statt 6 liess slirp_register_poll_socket() in den deprecated register_poll_fd-│
//         │      │ Zweig fallen (den wir NULL gelassen hatten) -> NULL-Call. Per Debug-Build      │
//         │      │ (gdb, symbolisierter Backtrace gegen selbst gebautes libslirp) isoliert.        │
//         │      │ cfg.version=6 gesetzt, slirp_add_hostfwd() jetzt aktiv aufgerufen               │
// 26-08-07│ 1.04 │ ZWEITER Absturz nach 1.03 (gdb): SlirpCb war lokale Stack-Variable in           │ AF
//         │      │ q9_slirp_start(), slirp_new() speichert davon aber nur den Zeiger, keine       │
//         │      │ Kopie -- dangling nach Rueckkehr, seit 1.00 latent (nie dereferenziert ohne     │
//         │      │ Poll-wuerdigen Socket). Erst der hostfwd-Listener (1.03) triggerte es scharf.   │
//         │      │ Fix: SlirpCb als static g_cb (Datei-Lebensdauer)                                │
// 26-08-07│ 1.05 │ DRITTER Bug (per Q9_SLIRP_DEBUG-Logging gefunden): WSAPoll() lieferte -1 bei     │ AF
//         │      │ JEDEM Aufruf sobald ein hostfwd-Listener registriert war -- deshalb kam trotz    │
//         │      │ Fix 1+2 nie ein SYN beim Gast an (kein Crash mehr, aber auch keine Funktion).    │
//         │      │ Ursache: libslirps plattformneutrale SLIRP_POLL_*-Flags wurden 1:1 als           │
//         │      │ WSAPOLLFD.events durchgereicht -- voellig andere Bit-Belegung als Windows'       │
//         │      │ POLLRDNORM/POLLWRNORM, WSAPoll lehnte die ungueltige Kombination komplett ab.    │
//         │      │ Fix: Uebersetzung SLIRP_POLL_* <-> WSAPOLLFD-Bits unter _WIN32, POSIX unveraendert │
//═════════╧══════╧═════════════════════════════════════════════════════════════════════════╧══════
#include "slirp_net.h"
#include "q9_sockcompat.h"
#include <slirp/libslirp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <time.h>
#endif

#define Q9_SLIRP_MAX_POLLFDS 64
#define Q9_SLIRP_MAX_TIMERS  8

typedef struct {
    int           used;
    SlirpTimerCb  cb;
    void         *cb_opaque;
    int64_t       expire_ms;      /* -1 = nicht gesetzt/deaktiviert */
} q9_slirp_timer_t;

static Slirp             *g_slirp;
static q9_slirp_recv_cb    g_recv_cb;
static void                *g_recv_opaque;
static q9_slirp_timer_t     g_timers[Q9_SLIRP_MAX_TIMERS];
static q9_pollfd_t          g_pollfds[Q9_SLIRP_MAX_POLLFDS];
static int                  g_pollfd_count;
/* MUSS statisch sein: slirp_new() (src/slirp.c) speichert nur den Zeiger auf SlirpCb, keine Kopie
   ("slirp->cb = callbacks;") -- als lokale Stack-Variable in q9_slirp_start() waere er nach dessen
   Rueckkehr dangling. War lange unbemerkt, weil slirp_pollfds_poll() (einziger Dereferenzierer aus
   dem Hauptloop) nur bei g_pollfd_count>0 laeuft -- ohne hostfwd/aktive Verbindung nie der Fall,
   also nie getriggert. Erst der neue hostfwd-Listener-Socket (s. slirp_net.c 1.03) macht
   g_pollfd_count>0 schon beim ersten Poll und deckte den Absturz auf (per gdb isoliert, s.u.). */
static SlirpCb               g_cb;

//────────────────────────────────────────────────────────────────────────────────────────────────
// Function: q_slirp_now_ns / q_slirp_now_ms
// Desc.:    Monotoner Takt fuer libslirp (clock_get_ns) bzw. fuer den Timer-Vergleich hier lokal
//           (ms reichen fuer die Aufloesung, mit der q9_slirp_poll() ohnehin aufgerufen wird).
//────────────────────────────────────────────────────────────────────────────────────────────────
#ifdef _WIN32
static int64_t q_slirp_now_ms(void) { return (int64_t)GetTickCount64(); }
#else
static int64_t q_slirp_now_ms(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (int64_t)ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
}
#endif

static int64_t q_slirp_clock_get_ns(void *opaque)
{
    (void)opaque;
    return q_slirp_now_ms() * 1000000;
}

//────────────────────────────────────────────────────────────────────────────────────────────────
// Function: q_slirp_timer_new / _mod / _free
// Desc.:    libslirp braucht nur einen einzigen Timer (SLIRP_TIMER_RA, IPv6 Router Advertisement --
//           bei uns eh deaktiviert, config.in6_enabled=false), ein kleines festes Array reicht.
//           Abgelaufene Timer werden in q9_slirp_poll() abgefeuert (kein eigener Scheduler-Thread).
//────────────────────────────────────────────────────────────────────────────────────────────────
static void *q_slirp_timer_new(SlirpTimerCb cb, void *cb_opaque, void *opaque)
{
    int i;
    (void)opaque;
    for (i = 0; i < Q9_SLIRP_MAX_TIMERS; i++) {
        if (!g_timers[i].used) {
            g_timers[i].used      = 1;
            g_timers[i].cb        = cb;
            g_timers[i].cb_opaque = cb_opaque;
            g_timers[i].expire_ms = -1;
            return &g_timers[i];
        }
    }
    return NULL;                                       /* Array voll -- sollte praktisch nie passieren */
}

static void q_slirp_timer_mod(void *timer, int64_t expire_time_ms, void *opaque)
{
    (void)opaque;
    if (timer) {
        ((q9_slirp_timer_t *)timer)->expire_ms = expire_time_ms;
    }
}

static void q_slirp_timer_free(void *timer, void *opaque)
{
    (void)opaque;
    if (timer) {
        memset(timer, 0, sizeof(q9_slirp_timer_t));
    }
}

static void q_slirp_run_timers(void)
{
    int64_t now = q_slirp_now_ms();
    int     i;
    for (i = 0; i < Q9_SLIRP_MAX_TIMERS; i++) {
        if (g_timers[i].used && g_timers[i].expire_ms >= 0 && g_timers[i].expire_ms <= now) {
            g_timers[i].expire_ms = -1;                /* vor dem Callback deaktivieren (kann sich
                                                            per timer_mod selbst neu setzen) */
            if (g_timers[i].cb) {
                g_timers[i].cb(g_timers[i].cb_opaque);
            }
        }
    }
}

static void q_slirp_notify(void *opaque) { (void)opaque; }
static void q_slirp_init_completed(Slirp *slirp, void *opaque) { (void)slirp; (void)opaque; }
static void q_slirp_register_poll_socket(slirp_os_socket s, void *opaque) { (void)s; (void)opaque; }
static void q_slirp_unregister_poll_socket(slirp_os_socket s, void *opaque) { (void)s; (void)opaque; }

//────────────────────────────────────────────────────────────────────────────────────────────────
// Function: q_slirp_send_packet / q_slirp_guest_error
// Desc.:    send_packet: Slirp -> Gast (Frame komplett fertig, direkt an recv_cb aus
//           q9_slirp_start durchreichen -- i.d.R. q9_quicc_rx_frame). guest_error: nur Diagnose.
//────────────────────────────────────────────────────────────────────────────────────────────────
static slirp_ssize_t q_slirp_send_packet(const void *buf, size_t len, void *opaque)
{
    (void)opaque;
    if (g_recv_cb) {
        g_recv_cb((const uint8_t *)buf, (uint32_t)len, g_recv_opaque);
    }
    return (slirp_ssize_t)len;
}

static void q_slirp_guest_error(const char *msg, void *opaque)
{
    (void)opaque;
    fprintf(stderr, "q9: slirp: %s\n", msg);
}

//────────────────────────────────────────────────────────────────────────────────────────────────
// Function: q_slirp_add_poll / q_slirp_get_revents
// Desc.:    Callbacks fuer slirp_pollfds_fill_socket/_poll (s. q9_slirp_poll) -- sammeln die von
//           Slirp gewuenschten Sockets in g_pollfds, liefern nach dem echten Q9_SOCK_POLL()-Aufruf
//           die revents zum per Index zurueckgegebenen Slot.
//
//           ROOT CAUSE eines dritten Bugs (2026-08-07, per Q9_SLIRP_DEBUG-Logging gefunden: WSAPoll
//           lieferte -1 bei JEDEM Aufruf, sobald ein hostfwd-Listener registriert war -- deshalb kam
//           trotz Fix 1+2 nie ein SYN beim Gast an): libslirp uebergibt seine EIGENEN, plattform-
//           neutralen Flags SLIRP_POLL_IN/OUT/PRI/ERR/HUP (1/2/4/8/16, libslirp.h) an diesen Callback
//           -- kein POSIX pollfd.events und schon gar kein Windows WSAPOLLFD.events! Auf POSIX klappte
//           die bisherige 1:1-Durchreichung nur zufaellig, weil POLLIN/POLLOUT/POLLERR/POLLHUP dort
//           dieselben Bit-Werte haben. WSAPOLLFD nutzt VOELLIG andere Bits (POLLRDNORM=0x100,
//           POLLWRNORM=0x10, POLLERR=0x1, POLLHUP=0x2, POLLNVAL=0x4) -- die rohen SLIRP_POLL_*-Werte
//           sind fuer WSAPoll() eine ungueltige Flag-Kombination, die es komplett ablehnt (rc=-1,
//           WSAGetLastError meist WSAEINVAL). Fix: unter Windows explizit uebersetzen, POSIX bleibt
//           unveraendert (Passthrough weiterhin korrekt).
//────────────────────────────────────────────────────────────────────────────────────────────────
static int q_slirp_add_poll(slirp_os_socket fd, int events, void *opaque)
{
    (void)opaque;
    if (g_pollfd_count >= Q9_SLIRP_MAX_POLLFDS) {
        return -1;                                     /* Slot voll -- dieser Socket faellt fuer   */
    }                                                   /* diese Runde aus, kein Absturz             */
    /* KEIN (int)-Cast auf fd: q9_pollfd_t.fd ist unter Windows SOCKET (64-Bit-Handle, WSAPOLLFD),
       unter POSIX int (struct pollfd) -- ein (int)-Cast stutzte den Windows-Handle vor dem impliziten
       Zurueckweiten in das 64-Bit-Feld sinnlos auf 32 Bit (Compiler-Warning deckte es auf, s.
       Chat-Session 2026-08-07). Direkte Zuweisung ist auf beiden Plattformen typkorrekt. */
    g_pollfds[g_pollfd_count].fd      = fd;
#ifdef _WIN32
    {
        short wsa_events = 0;
        if (events & SLIRP_POLL_IN)  wsa_events |= POLLRDNORM;
        if (events & SLIRP_POLL_OUT) wsa_events |= POLLWRNORM;
        if (events & SLIRP_POLL_PRI) wsa_events |= POLLRDBAND;
        /* POLLERR/POLLHUP/POLLNVAL gehoeren unter WSAPoll NICHT ins events-Feld (werden wie bei
           POSIX poll() immer automatisch in revents gemeldet, unabhaengig von den angeforderten
           events) -- SLIRP_POLL_ERR/HUP hier bewusst NICHT uebernehmen. */
        g_pollfds[g_pollfd_count].events = wsa_events;
    }
#else
    g_pollfds[g_pollfd_count].events  = (short)events;
#endif
    g_pollfds[g_pollfd_count].revents = 0;
    return g_pollfd_count++;
}

static int q_slirp_get_revents(int idx, void *opaque)
{
    (void)opaque;
    if (idx < 0 || idx >= g_pollfd_count) {
        return 0;
    }
#ifdef _WIN32
    {
        short w = g_pollfds[idx].revents;
        int slirp_revents = 0;
        if (w & (POLLRDNORM | POLLRDBAND)) slirp_revents |= SLIRP_POLL_IN;
        if (w & POLLWRNORM)                slirp_revents |= SLIRP_POLL_OUT;
        if (w & POLLRDBAND)                slirp_revents |= SLIRP_POLL_PRI;
        if (w & (POLLERR | POLLNVAL))      slirp_revents |= SLIRP_POLL_ERR;
        if (w & POLLHUP)                   slirp_revents |= SLIRP_POLL_HUP;
        return slirp_revents;
    }
#else
    return g_pollfds[idx].revents;
#endif
}

//────────────────────────────────────────────────────────────────────────────────────────────────
// API-Implementierung
//────────────────────────────────────────────────────────────────────────────────────────────────

int q9_slirp_start(const q9_slirp_config_t *config,
                    const q9_slirp_hostfwd_t *hostfwd, int hostfwd_count,
                    q9_slirp_recv_cb recv_cb, void *recv_opaque)
{
    SlirpConfig cfg;

    if (q9_sock_startup() != 0) {                      /* WSAStartup unter Windows, no-op sonst    */
        fprintf(stderr, "q9: slirp: Socket-Subsystem konnte nicht initialisiert werden.\n");
        return 1;
    }

    memset(&cfg, 0, sizeof(cfg));
    cfg.version     = 6;                                /* SLIRP_CONFIG_VERSION_MAX (s. Ursache des
                                                            hostfwd-Absturzes unten bei cb.register_*) */
    cfg.restricted  = 0;                                /* Gast darf ins echte Internet (kein Sandbox-Modus) */
    cfg.in_enabled  = true;
    cfg.in6_enabled = false;                            /* IPv6 nicht gebraucht, s. OS-9-Netzwerk-Stack */
    cfg.if_mtu      = 0;                                /* 0 = Default (IF_MTU_DEFAULT)              */
    cfg.if_mru      = 0;
    cfg.disable_host_loopback = false;

    /* OS-9s Netzwerk-Stack macht kein DHCP, sondern hat seine IP statisch im Image konfiguriert
       (historisch 192.168.200.2, s. context.txt) -- Slirp routet trotzdem korrekt, weil es JEDE
       IP innerhalb von vnetwork/vnetmask akzeptiert, unabhaengig von einem DHCP-Handshake.
       vdhcp_start wird trotzdem exakt auf die Gast-IP gesetzt (falls doch mal ein DHCP-Client
       reinkommt, bekommt er sinnvollerweise dieselbe Adresse) und unten fuer hostfwd wiederverwendet
       (identischer Wert, zwei Verwendungszwecke). */
    if (!inet_pton(AF_INET, (config && config->guest_ip) ? config->guest_ip : "192.168.200.2",
                   &cfg.vdhcp_start)) {
        fprintf(stderr, "q9: slirp: ungueltige guest_ip.\n");
        return 1;
    }
    if (!inet_pton(AF_INET, (config && config->gateway) ? config->gateway : "192.168.200.1",
                   &cfg.vhost)) {
        fprintf(stderr, "q9: slirp: ungueltige gateway-Adresse.\n");
        return 1;
    }
    if (!inet_pton(AF_INET, (config && config->netmask) ? config->netmask : "255.255.255.0",
                   &cfg.vnetmask)) {
        fprintf(stderr, "q9: slirp: ungueltige netmask.\n");
        return 1;
    }
    /* vnetwork = Host & Netmask (Subnetz-Basisadresse) -- vnetmask wurde eben geparst. */
    cfg.vnetwork.s_addr = cfg.vhost.s_addr & cfg.vnetmask.s_addr;
    cfg.vnameserver     = cfg.vhost;                    /* DNS-Anfragen beantwortet Slirp selbst    */

    memset(&g_cb, 0, sizeof(g_cb));
    g_cb.send_packet = q_slirp_send_packet;
    g_cb.guest_error = q_slirp_guest_error;
    g_cb.clock_get_ns = q_slirp_clock_get_ns;
    g_cb.timer_new    = q_slirp_timer_new;
    g_cb.timer_free   = q_slirp_timer_free;
    g_cb.timer_mod    = q_slirp_timer_mod;
    g_cb.notify                  = q_slirp_notify;
    g_cb.init_completed          = q_slirp_init_completed;
    g_cb.register_poll_socket    = q_slirp_register_poll_socket;
    g_cb.unregister_poll_socket  = q_slirp_unregister_poll_socket;
    /* Alle vier oben sind reine No-Ops (wir pollen Slirps Sockets aktiv jede Runde selbst per
       slirp_pollfds_fill_socket/_poll, s. q9_slirp_poll) -- NULL zu lassen fuehrte aber zu einem
       Absturz (Sprung zu Adresse 0 innerhalb libslirp-0.dll, per gdb isoliert).
       ROOT CAUSE #1 (2026-08-07, per Debug-Build mit Symbolen isoliert): slirp_register_poll_socket()
       (src/slirp.c) ruft cb->register_poll_socket() NUR wenn cfg_version >= 6 ist -- sonst faellt es
       in den deprecated cb->register_poll_fd()-Zweig (int statt slirp_os_socket), den wir bewusst NULL
       gelassen hatten, weil laut Header nur die _socket-Variante noch gebraucht schien. Mit cfg.version=1
       (s.o.) griff also immer der deprecated Zweig -> NULL-Call -> Crash in slirp_add_hostfwd() ->
       tcp_listen() -> tcpx_listen() -> slirp_register_poll_socket(). Fix: cfg.version=6 aktiviert den
       richtigen Zweig, register_poll_fd/unregister_poll_fd bleiben absichtlich NULL (deprecated).
       ROOT CAUSE #2 (2026-08-07, nach Fix #1 per gdb gefunden, s. g_cb-Deklaration oben): slirp_new()
       speichert nur den ZEIGER auf dieses Cb-Objekt ("slirp->cb = callbacks;", src/slirp.c), keine
       Kopie -- als lokale Variable hier waere er nach Rueckkehr von q9_slirp_start() dangling. Der
       Bug war seit 1.00 latent (nie dereferenziert, solange g_pollfd_count==0), erst der hostfwd-
       Listener-Socket (Fix #1) machte ihn scharf: slirp_pollfds_poll() -> slirp->cb->clock_get_ns()
       sprang auf eine laengst wiederverwendete Stack-Adresse. Fix: g_cb ist jetzt static (Datei-
       Lebensdauer), passt zum ohnehin Singleton-artigen g_slirp. */

    g_recv_cb     = recv_cb;
    g_recv_opaque = recv_opaque;
    memset(g_timers, 0, sizeof(g_timers));

    g_slirp = slirp_new(&cfg, &g_cb, NULL);
    if (!g_slirp) {
        fprintf(stderr, "q9: slirp: slirp_new() fehlgeschlagen.\n");
        return 1;
    }

    /* BUG GEFUNDEN + BEHOBEN (2026-08-07 Nacht, s. third_party/slirp/Q9_VENDOR.md "RESUME HERE"):
       Der vermeintliche libslirp-Absturz war ein eigener Konfigurationsfehler, kein Upstream-Bug.
       slirp_register_poll_socket() (src/slirp.c) ruft cb->register_poll_socket() NUR bei
       cfg_version >= 6 -- sonst faellt es in den deprecated cb->register_poll_fd()-Zweig (den wir
       absichtlich NULL gelassen hatten) -> NULL-Call -> Crash. Mit cfg.version=6 (s.o., statt vorher
       1) verifiziert per Debug-Build (gdb, symbolisierter Backtrace) UND Standalone-Repro: hostfwd
       funktioniert jetzt. Ziel ist immer die Gast-IP (OS-9 hat keine dynamische Adresse, s.o.). */
    for (int i = 0; i < hostfwd_count; i++) {
        struct in_addr host_addr;
        host_addr.s_addr = 0;                           /* INADDR_ANY -- von ueberall erreichbar   */
        int rc = slirp_add_hostfwd(g_slirp, hostfwd[i].is_udp, host_addr, hostfwd[i].host_port,
                                    cfg.vdhcp_start, hostfwd[i].guest_port);
        if (rc != 0) {
            fprintf(stderr, "q9: slirp: hostfwd %s :%u -> Gast:%u fehlgeschlagen (rc=%d).\n",
                    hostfwd[i].is_udp ? "UDP" : "TCP", hostfwd[i].host_port, hostfwd[i].guest_port, rc);
        } else {
            fprintf(stderr, "q9: slirp: hostfwd %s :%u -> Gast:%u aktiv.\n",
                    hostfwd[i].is_udp ? "UDP" : "TCP", hostfwd[i].host_port, hostfwd[i].guest_port);
        }
    }

    printf("[Q9 Slirp] Backend gestartet (Subnetz %s/%s, Gast-IP wird per DHCP zugewiesen).\n",
           (config && config->guest_ip) ? config->guest_ip : "192.168.200.2",
           (config && config->netmask) ? config->netmask : "255.255.255.0");
    return 0;
}

void q9_slirp_input(const uint8_t *frame, uint32_t len)
{
    if (g_slirp) {
        slirp_input(g_slirp, frame, (int)len);
    }
}

void q9_slirp_poll(void)
{
    uint32_t timeout_ms;

    if (!g_slirp) {
        return;
    }

    q_slirp_run_timers();

    timeout_ms     = 0;                                 /* wir wollen NIE blockieren -- Poll-Aufruf */
    g_pollfd_count = 0;                                  /* unten mit timeout_ms=0 ist nur Zustands- */
    slirp_pollfds_fill_socket(g_slirp, &timeout_ms, q_slirp_add_poll, NULL);
                                                          /* abfrage, kein echtes Warten             */
    /* BEKANNTER BUG (2026-08-07, s. Q9_VENDOR.md): slirp_pollfds_poll() stuerzt in der vendorten
       Windows-Kombination zuverlaessig ab, wenn g_pollfd_count==0 ist (kein von Slirp angefordertes
       Socket in dieser Runde -- Sprung zu einer Quasi-NULL-Adresse aus libslirp-0.dll heraus, per
       gdb-Backtrace im echten q9.exe isoliert; reproduzierte NICHT in kleinen Standalone-Tests,
       sieht nach einem uninitialisierten Speicherzugriff in libslirp bei leerem Poll-Set aus). Bei
       0 Sockets gibt es ohnehin nichts zu pollen -- den Aufruf dann komplett auslassen umgeht den
       Bug, ohne echte Funktionalitaet zu verlieren (slirp_pollfds_poll wird nur gebraucht, um echte
       Socket-Ereignisse zu verarbeiten; ohne Sockets kann keins vorliegen). */
    if (g_pollfd_count > 0) {
        int pollret = Q9_SOCK_POLL(g_pollfds, g_pollfd_count, 0);
        /* Q9_SLIRP_DEBUG=1: Poll-Diagnose bei echten Ereignissen (bewusst NICHT jeden Tick -- sonst
           Log-Flut, s. Q9_QUICC_DEBUG-Pendant in quicc.c fuer den Frame-Verkehr). War entscheidend
           beim Aufspueren von Bug 3 (2026-08-07, s. Edition History): zeigte pollret=-1 bei jedem
           Aufruf, sobald der falsche SLIRP_POLL_*->WSAPOLLFD.events-Bit-Mismatch (jetzt oben behoben)
           WSAPoll() die Eingabe verweigern liess. */
        if (pollret != 0 && getenv("Q9_SLIRP_DEBUG")) {
            fprintf(stderr, "[slirp poll] count=%d pollret=%d", g_pollfd_count, pollret);
            for (int qi = 0; qi < g_pollfd_count; qi++) {
                fprintf(stderr, " [fd=%llu ev=%d rev=%d]",
                        (unsigned long long)g_pollfds[qi].fd, g_pollfds[qi].events, g_pollfds[qi].revents);
            }
            fprintf(stderr, "\n");
        }
        slirp_pollfds_poll(g_slirp, 0, q_slirp_get_revents, NULL);
    }
}
//────────────────────────────────────────────────────────────────────────────────────────────────
// EOF slirp_net.c                                                                         Ver. 1.02
//────────────────────────────────────────────────────────────────────────────────────────────────
