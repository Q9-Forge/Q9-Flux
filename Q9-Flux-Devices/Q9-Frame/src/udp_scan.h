#ifndef Q9FRAME_UDP_SCAN_H
#define Q9FRAME_UDP_SCAN_H

struct Q9EmuInfo {
    char name[64];
    char host[128];
    int port;
    int width;
    int height;
};

int udp_scan(struct Q9EmuInfo* results, int max_results, int timeout_ms = 2000);

#endif // Q9FRAME_UDP_SCAN_H
