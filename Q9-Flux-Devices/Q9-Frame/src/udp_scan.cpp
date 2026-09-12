#include "udp_scan.h"
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#define Q9_DISCOVER_PORT 2000
#define Q9_DISCOVER_MSG "Q9_VIDEO_DISCOVER"

int udp_scan(struct Q9EmuInfo* results, int max_results, int timeout_ms) {
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) return -1;
    int broadcast = 1;
    setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(broadcast));
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(Q9_DISCOVER_PORT);
    addr.sin_addr.s_addr = htonl(INADDR_BROADCAST);
    sendto(sock, Q9_DISCOVER_MSG, strlen(Q9_DISCOVER_MSG), 0, (struct sockaddr*)&addr, sizeof(addr));
    // Zusätzlich direkt an localhost senden: ein Broadcast an 255.255.255.255
    // kommt nicht auf jedem System beim eigenen Rechner an (z.B. macOS ohne
    // passende Interface-Route), localhost-Emulatoren würden sonst nicht gefunden.
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    sendto(sock, Q9_DISCOVER_MSG, strlen(Q9_DISCOVER_MSG), 0, (struct sockaddr*)&addr, sizeof(addr));
    struct timeval tv;
    tv.tv_sec = timeout_ms / 1000;
    tv.tv_usec = (timeout_ms % 1000) * 1000;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    int found = 0;
    while (found < max_results) {
        char buf[256];
        struct sockaddr_in from;
        socklen_t fromlen = sizeof(from);
        int n = recvfrom(sock, buf, sizeof(buf)-1, 0, (struct sockaddr*)&from, &fromlen);
        if (n < 0) break;
        buf[n] = 0;
        // Erwartetes Format: Name;Port;Width;Height
        struct Q9EmuInfo* info = &results[found];
        strncpy(info->host, inet_ntoa(from.sin_addr), sizeof(info->host));
        sscanf(buf, "%63[^;];%d;%d;%d", info->name, &info->port, &info->width, &info->height);
        found++;
    }
    close(sock);
    return found;
}
