/* dhf_socket_client.c - Client side for remote DHF socket forwarding
 */

#include "dhf_socket.h"
#include "dhf_proto.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>

static int do_connect(const char *host, int port) {
    struct sockaddr_in sin;
    memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_port = htons((uint16_t)port);

    if (inet_pton(AF_INET, host, &sin.sin_addr) <= 0) {
        struct hostent *he = gethostbyname(host);
        if (!he) return -1;
        memcpy(&sin.sin_addr, he->h_addr_list[0], sizeof(sin.sin_addr));
    }

    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) return -1;

    int flag = 1;
    setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, (char*)&flag, sizeof(flag));

    if (connect(fd, (struct sockaddr*)&sin, sizeof(sin)) != 0) {
        close(fd);
        return -1;
    }
    return fd;
}

static int send_all(int fd, const void *buf, size_t len) {
    size_t total = 0;
    const char *p = (const char*)buf;
    while (total < len) {
        ssize_t n = send(fd, p + total, len - total, 0);
        if (n <= 0) return -1;
        total += (size_t)n;
    }
    return 0;
}

static int recv_all(int fd, void *buf, size_t len) {
    size_t total = 0;
    char *p = (char*)buf;
    while (total < len) {
        ssize_t n = recv(fd, p + total, len - total, 0);
        if (n <= 0) return -1;
        total += (size_t)n;
    }
    return 0;
}

int dhf_socket_forward_cmd(int *cached_sock, const char *host, int port,
                           uint8_t cmd, uint32_t *d0, uint32_t *d1, uint32_t *d2,
                           const char *path,
                           const void *send_data, size_t send_len,
                           void *recv_data, size_t *recv_len,
                           uint8_t *out_status) {
    if (!host || !cached_sock) return -1;

    if (*cached_sock < 0) {
        *cached_sock = do_connect(host, port);
        if (*cached_sock < 0) {
            if (out_status) *out_status = DHF_ERR_NET;
            return -1;
        }
    }

    uint32_t plen = (path && path[0]) ? (uint32_t)strlen(path) + 1 : 0;
    uint32_t dlen = (uint32_t)send_len;

    dhf_net_header_t hdr;
    memset(&hdr, 0, sizeof(hdr));
    hdr.magic = htonl(DHF_NET_MAGIC);
    hdr.command = cmd;
    hdr.status = 0;
    hdr.d0 = d0 ? htonl(*d0) : 0;
    hdr.d1 = d1 ? htonl(*d1) : 0;
    hdr.d2 = d2 ? htonl(*d2) : 0;
    hdr.path_len = htonl(plen);
    hdr.data_len = htonl(dlen);

    /* Send header */
    if (send_all(*cached_sock, &hdr, sizeof(hdr)) != 0 ||
        (plen > 0 && send_all(*cached_sock, path, plen) != 0) ||
        (dlen > 0 && send_all(*cached_sock, send_data, dlen) != 0)) {
        close(*cached_sock);
        *cached_sock = -1;
        if (out_status) *out_status = DHF_ERR_NET;
        return -1;
    }

    /* Receive response header */
    dhf_net_header_t resp;
    if (recv_all(*cached_sock, &resp, sizeof(resp)) != 0 || ntohl(resp.magic) != DHF_NET_MAGIC) {
        close(*cached_sock);
        *cached_sock = -1;
        if (out_status) *out_status = DHF_ERR_NET;
        return -1;
    }

    if (d0) *d0 = ntohl(resp.d0);
    if (d1) *d1 = ntohl(resp.d1);
    if (d2) *d2 = ntohl(resp.d2);
    if (out_status) *out_status = resp.status;

    uint32_t resp_data_len = ntohl(resp.data_len);
    if (resp_data_len > 0 && recv_data && recv_len && *recv_len > 0) {
        size_t to_read = resp_data_len;
        if (to_read > *recv_len) to_read = *recv_len;
        if (recv_all(*cached_sock, recv_data, to_read) != 0) {
            close(*cached_sock);
            *cached_sock = -1;
            return -1;
        }
        *recv_len = to_read;
        /* Drain any remainder */
        if (resp_data_len > to_read) {
            char junk[256];
            size_t rem = resp_data_len - to_read;
            while (rem > 0) {
                size_t step = rem > sizeof(junk) ? sizeof(junk) : rem;
                recv_all(*cached_sock, junk, step);
                rem -= step;
            }
        }
    } else if (recv_len) {
        *recv_len = 0;
    }

    return 0;
}
