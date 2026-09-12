#include "protocol.h"
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

int send_hello(int sock) {
    struct Q9MsgHeader hdr;
    memcpy(hdr.magic, Q9_MAGIC, 4);
    hdr.version = htons(Q9_PROTO_VERSION);
    hdr.type = htons(Q9_HELLO);
    hdr.length = htonl(0);
    hdr.sequence = htonl(1);
    return write(sock, &hdr, sizeof(hdr)) == sizeof(hdr) ? 0 : -1;
}

int recv_msg_header(int sock, struct Q9MsgHeader* hdr) {
    ssize_t n = read(sock, hdr, sizeof(*hdr));
    if (n != sizeof(*hdr)) return -1;
    // Header-Felder ins Host-Format umwandeln
    hdr->version = ntohs(hdr->version);
    hdr->type = ntohs(hdr->type);
    hdr->length = ntohl(hdr->length);
    hdr->sequence = ntohl(hdr->sequence);
    return 0;
}

void dirtyrect_hton(struct Q9DirtyRectHeader* r) {
    r->x = htons(r->x);
    r->y = htons(r->y);
    r->w = htons(r->w);
    r->h = htons(r->h);
}

void dirtyrect_ntoh(struct Q9DirtyRectHeader* r) {
    r->x = ntohs(r->x);
    r->y = ntohs(r->y);
    r->w = ntohs(r->w);
    r->h = ntohs(r->h);
}
