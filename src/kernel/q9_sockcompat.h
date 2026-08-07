//════════════════════════════════════════════════════════════════════════════════════════════════
// File:   q9_sockcompat.h                                                                 Ver. 1.10
// Owner:  AF
// Desc.:  Duenner Portabilitaets-Shim fuer die POSIX-Socket-Aufrufe in m68krt.c (OS-9-Netz-
//         Terminals /x1../x8) und videobridge.c (Framebuffer-Streaming). Unter Windows gibt es kein
//         <sys/socket.h> -- Winsock2 bringt zwar dieselben Funktionsnamen (socket, bind, send,
//         recv, setsockopt, ...) mit, aber andere Header, einen expliziten Init-/Cleanup-Schritt
//         (WSAStartup/WSACleanup) und andere Namen fuer close()/non-blocking/errno. Bewusst NUR
//         Makros + zwei Init/Cleanup-Funktionen, keine eigene Socket-Abstraktion -- der ganze Rest
//         von m68krt.c/videobridge.c bleibt unveraendert POSIX-Code, der unter Windows unveraendert
//         gegen Winsock uebersetzt wird (fast alle Funktions-/Konstantennamen sind identisch).
//
// Call:   #include "q9_sockcompat.h"      -- statt sys/socket.h & co. direkt einzubinden
//         q9_sock_startup()               einmal vor dem ersten Socket-Aufruf
//                                          (WSAStartup unter Windows, no-op auf POSIX)
//         q9_sock_cleanup()               beim Herunterfahren (WSACleanup unter Windows, no-op sonst)
//         Q9_SOCK_CLOSE(fd)               statt close(fd)
//         Q9_SOCK_NONBLOCK(fd)            statt fcntl(fd, F_SETFL, O_NONBLOCK)
//         Q9_SOCK_WOULDBLOCK()            statt errno == EAGAIN || errno == EWOULDBLOCK
//         Q9_SOCK_IGNORE_SIGPIPE()        statt signal(SIGPIPE, SIG_IGN) (Windows kennt kein SIGPIPE
//                                          fuer Sockets -- send() liefert stattdessen WSAECONNRESET)
//         q9_pollfd_t / Q9_SOCK_POLL(fds,n,timeout_ms)  statt struct pollfd/poll() (WSAPoll unter
//                                          Windows -- identische Feldnamen fd/events/revents wie
//                                          POSIX pollfd, deshalb reicht ein Typalias ohne Makro-Zoo)
//
// Edition History
//─────────┬──────┬─────────────────────────────────────────────────────────────────────────┬──────
// Date    │ Ver. │ Description                                                             │ By
//─────────┼──────┼─────────────────────────────────────────────────────────────────────────┼──────
// 26-08-06│ 1.00 │ Erster Wurf: nativer Windows-Build (Winsock2) fuer m68krt.c/videobridge.c │ AF
// 26-08-07│ 1.10 │ q9_pollfd_t/Q9_SOCK_POLL (WSAPoll/poll) fuer slirp_net.c (5.14: libslirp- │ AF
//         │      │ Netzwerk-Backend braucht echtes Socket-Polling fuer seine eigenen Sockets) │
//═════════╧══════╧═════════════════════════════════════════════════════════════════════════╧══════
#ifndef Q9_SOCKCOMPAT_H
#define Q9_SOCKCOMPAT_H

#include <stdio.h>

#ifdef _WIN32

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
/* mingw-w64 (crtdefs.h/corecrt.h, ueber m68krt.h -> stdint.h eingebunden) deklariert ssize_t
   bereits selbst (__int64) -- kein eigenes typedef noetig/erlaubt. */

#define Q9_SOCK_CLOSE(fd)     closesocket(fd)
#define Q9_SOCK_WOULDBLOCK()  (WSAGetLastError() == WSAEWOULDBLOCK)
#define Q9_SOCK_IGNORE_SIGPIPE() ((void)0)

static inline void Q9_SOCK_NONBLOCK(int fd)
{
    u_long q9_nonblock = 1;
    ioctlsocket((SOCKET)fd, FIONBIO, &q9_nonblock);
}

static inline int q9_sock_startup(void)
{
    WSADATA wsa;
    return WSAStartup(MAKEWORD(2, 2), &wsa) == 0 ? 0 : -1;
}

static inline void q9_sock_cleanup(void)
{
    WSACleanup();
}

static inline void q9_sock_perror(const char *msg)
{
    fprintf(stderr, "%s: WSA error %d\n", msg, WSAGetLastError());
}

typedef WSAPOLLFD q9_pollfd_t;
#define Q9_SOCK_POLL(fds, n, timeout_ms) WSAPoll((fds), (ULONG)(n), (timeout_ms))

#else /* POSIX */

#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <poll.h>

#define Q9_SOCK_CLOSE(fd)         close(fd)
#define Q9_SOCK_NONBLOCK(fd)      fcntl((fd), F_SETFL, O_NONBLOCK)
#define Q9_SOCK_WOULDBLOCK()      (errno == EAGAIN || errno == EWOULDBLOCK)
#define Q9_SOCK_IGNORE_SIGPIPE()  signal(SIGPIPE, SIG_IGN)

static inline int  q9_sock_startup(void) { return 0; }
static inline void q9_sock_cleanup(void) { }

static inline void q9_sock_perror(const char *msg) { perror(msg); }

typedef struct pollfd q9_pollfd_t;
#define Q9_SOCK_POLL(fds, n, timeout_ms) poll((fds), (nfds_t)(n), (timeout_ms))

#endif /* _WIN32 */

#endif /* Q9_SOCKCOMPAT_H */
//────────────────────────────────────────────────────────────────────────────────────────────────
// EOF q9_sockcompat.h                                                                     Ver. 1.10
//────────────────────────────────────────────────────────────────────────────────────────────────
