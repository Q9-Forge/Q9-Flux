#ifndef Q9FRAME_PROTOCOL_H
#define Q9FRAME_PROTOCOL_H
#include <stdint.h>

#define Q9_MAGIC "Q9VF"
#define Q9_PROTO_VERSION 1

enum Q9MsgType {
    Q9_HELLO = 1,
    Q9_VIDEO_INFO = 2,
    Q9_FRAME_FULL = 3,
    Q9_FRAME_UPDATE = 4,
    Q9_REQUEST_FULL_FRAME = 5,
    Q9_PING = 6,
    Q9_PONG = 7,
    Q9_ERROR = 8,
    Q9_PALETTE = 9
};

struct Q9MsgHeader {
    char magic[4];
    uint16_t version;
    uint16_t type;
    uint32_t length;
    uint32_t sequence;
};

struct Q9DirtyRectHeader {
    uint16_t x;
    uint16_t y;
    uint16_t w;
    uint16_t h;
};

int send_hello(int sock);
int recv_msg_header(int sock, struct Q9MsgHeader* hdr);
void dirtyrect_hton(struct Q9DirtyRectHeader* r);
void dirtyrect_ntoh(struct Q9DirtyRectHeader* r);

#endif // Q9FRAME_PROTOCOL_H
