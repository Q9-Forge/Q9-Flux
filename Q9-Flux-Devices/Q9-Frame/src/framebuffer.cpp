#include "framebuffer.h"
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>

int recv_video_info(int sock, Q9VideoInfo* info) {
    ssize_t n = read(sock, info, sizeof(*info));
    if (n != sizeof(*info)) return -1;
    info->width = ntohs(info->width);
    info->height = ntohs(info->height);
    info->stride = ntohs(info->stride);
    return 0;
}

int recv_frame_full(int sock, Q9Framebuffer* fb) {
    size_t size = fb->info.stride * fb->info.height;
    fb->data = (uint8_t*)malloc(size);
    if (!fb->data) return -1;
    size_t got = 0;
    while (got < size) {
        ssize_t n = read(sock, fb->data + got, size - got);
        if (n <= 0) { free(fb->data); fb->data = 0; return -1; }
        got += n;
    }
    return 0;
}

int recv_palette(int sock, Q9ClutEntry* clut) {
    size_t size = Q9_CLUT_ENTRIES * sizeof(Q9ClutEntry);
    size_t got = 0;
    while (got < size) {
        ssize_t n = read(sock, (uint8_t*)clut + got, size - got);
        if (n <= 0) return -1;
        got += n;
    }
    return 0;
}

void build_grayscale_clut(Q9ClutEntry* clut, int bpp) {
    int entries = 1 << bpp;
    for (int i = 0; i < entries; ++i) {
        uint8_t v = (uint8_t)(entries > 1 ? i * 255 / (entries - 1) : 255);
        clut[i].r = v; clut[i].g = v; clut[i].b = v;
    }
}

void free_framebuffer(Q9Framebuffer* fb) {
    if (fb->data) free(fb->data);
    fb->data = 0;
}
