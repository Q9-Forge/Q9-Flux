// Minimaler Dummy-Server für Q9 Frame-Client-Tests
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <signal.h>
#include "../src/protocol.h"
#include "../src/framebuffer.h"

// Default 640x480, per --res=WxH ueberschreibbar.
static int fb_width = 640, fb_height = 480;

// Vom gewaehlten --mode bestimmt, einmalig beim Start gesetzt.
static Q9VideoMode g_mode = Q9_VMODE_INDEXED1;
static int g_bpp = 1;
static int g_stride = 0;
static size_t g_fb_size = 0;

// Pixel pro Byte fuer die aktuelle Bit-Tiefe (nur <8bpp relevant, sonst 1).
static int align_unit() { return g_bpp < 8 ? 8 / g_bpp : 1; }

static void put_pixel_1bpp(uint8_t* vram, int x, int y, bool on) {
    if (x < 0 || x >= fb_width || y < 0 || y >= fb_height) return;
    uint8_t mask = 0x80 >> (x & 7);
    if (on) vram[y * g_stride + (x >> 3)] |= mask;
    else vram[y * g_stride + (x >> 3)] &= ~mask;
}

// Generalisiert fuer 1/2/4/8 bpp: schreibt einen Palette-Index an Pixel (x,y).
static void put_index(uint8_t* vram, int x, int y, int idx) {
    uint8_t* row = vram + (size_t)y * g_stride;
    switch (g_bpp) {
        case 1: {
            uint8_t mask = 0x80 >> (x & 7);
            if (idx) row[x >> 3] |= mask; else row[x >> 3] &= ~mask;
            break;
        }
        case 2: {
            int shift = 6 - 2 * (x & 3);
            row[x >> 2] = (uint8_t)((row[x >> 2] & ~(0x3 << shift)) | ((idx & 0x3) << shift));
            break;
        }
        case 4: {
            int shift = (x & 1) ? 0 : 4;
            row[x >> 1] = (uint8_t)((row[x >> 1] & ~(0xF << shift)) | ((idx & 0xF) << shift));
            break;
        }
        case 8:
            row[x] = (uint8_t)idx;
            break;
    }
}

static void put_rgb565(uint8_t* vram, int x, int y, uint8_t r5, uint8_t g6, uint8_t b5) {
    uint16_t v = (uint16_t)((r5 << 11) | (g6 << 5) | b5);
    uint8_t* px = vram + (size_t)y * g_stride + x * 2;
    px[0] = (uint8_t)(v >> 8); px[1] = (uint8_t)(v & 0xFF); // big-endian ueber die Leitung
}

static void put_rgb555i(uint8_t* vram, int x, int y, uint8_t r5, uint8_t g5, uint8_t b5, bool intensity) {
    uint16_t v = (uint16_t)((intensity ? 0x8000 : 0) | (r5 << 10) | (g5 << 5) | b5);
    uint8_t* px = vram + (size_t)y * g_stride + x * 2;
    px[0] = (uint8_t)(v >> 8); px[1] = (uint8_t)(v & 0xFF);
}

static void put_rgb888(uint8_t* vram, int x, int y, uint8_t r, uint8_t g, uint8_t b) {
    uint8_t* px = vram + (size_t)y * g_stride + x * 3;
    px[0] = r; px[1] = g; px[2] = b;
}

static void draw_q9_logo(uint8_t* vram) {
    // Großes Q
    for (int y = 80; y < 400; ++y) {
        for (int x = 120; x < 320; ++x) {
            int dx = x-220, dy = y-240;
            if (dx*dx + dy*dy < 100*100 && dx*dx + dy*dy > 80*80) put_pixel_1bpp(vram, x, y, true);
            if (dx > 30 && dx < 50 && dy > 30 && dy < 50) put_pixel_1bpp(vram, x, y, true);
        }
    }
    // Großes 9
    for (int y = 80; y < 400; ++y) {
        for (int x = 340; x < 520; ++x) {
            int dx = x-430, dy = y-180;
            if (dx*dx + dy*dy < 90*90 && dx*dx + dy*dy > 70*70 && y < 260) put_pixel_1bpp(vram, x, y, true);
            if (y > 260 && y < 380 && dx > -50 && dx < 50 && (y-330)*(y-330)+(x-430)*(x-430) < 50*50 && (y-330)*(y-330)+(x-430)*(x-430) > 30*30) put_pixel_1bpp(vram, x, y, true);
            if (x > 420 && x < 440 && y > 200 && y < 380) put_pixel_1bpp(vram, x, y, true);
        }
    }
}

// Startbild je Modus: 1bpp bekommt das Q9-Logo, indizierte Modi einen Verlauf
// ueber alle Palette-Indizes, direkte RGB-Modi einen Farbverlauf.
static void draw_test_pattern(uint8_t* vram) {
    if (g_mode == Q9_VMODE_INDEXED1) { draw_q9_logo(vram); return; }
    if (q9_mode_is_indexed(g_mode)) {
        int levels = 1 << g_bpp;
        for (int y = 0; y < fb_height; ++y)
            for (int x = 0; x < fb_width; ++x)
                put_index(vram, x, y, x * levels / fb_width);
        return;
    }
    for (int y = 0; y < fb_height; ++y) {
        for (int x = 0; x < fb_width; ++x) {
            uint8_t r8 = (uint8_t)(x * 255 / fb_width);
            uint8_t g8 = (uint8_t)(y * 255 / fb_height);
            uint8_t b8 = 128;
            if (g_mode == Q9_VMODE_RGB565) put_rgb565(vram, x, y, r8 >> 3, g8 >> 2, b8 >> 3);
            else if (g_mode == Q9_VMODE_RGB555I) put_rgb555i(vram, x, y, r8 >> 3, g8 >> 3, b8 >> 3, (x / 40) % 2 == 0);
            else put_rgb888(vram, x, y, r8, g8, b8);
        }
    }
}

// Schreibt ein zufaelliges Rechteck mit einem fuer den aktuellen Modus passenden
// Inhalt in den VRAM und liefert dessen Koordinaten zurueck. x/w sind auf
// align_unit() ausgerichtet, damit sie stets byte-aligniert bleiben.
static void draw_random_rect(uint8_t* vram, int* out_x, int* out_y, int* out_w, int* out_h) {
    int unit = align_unit();
    int w = 16 + rand() % 145;   // 16..160 Pixel, unabhaengig vom Alignment
    w -= w % unit;
    if (w < unit) w = unit;
    int h = 16 + rand() % 100;
    if (w > fb_width) w = fb_width - (fb_width % unit);
    if (h > fb_height) h = fb_height;
    int x = (rand() % ((fb_width - w) / unit + 1)) * unit;
    int y = rand() % (fb_height - h + 1);

    if (g_mode == Q9_VMODE_INDEXED1) {
        int pattern = rand() % 4;
        for (int yy = 0; yy < h; ++yy) {
            for (int xx = 0; xx < w; ++xx) {
                int px = x + xx, py = y + yy;
                bool on;
                switch (pattern) {
                    case 0: on = true; break;
                    case 1: on = false; break;
                    case 2: on = ((px / 8 + py / 8) % 2) == 0; break;
                    default: on = (py % 4) < 2; break;
                }
                put_pixel_1bpp(vram, px, py, on);
            }
        }
    } else if (q9_mode_is_indexed(g_mode)) {
        int idx = rand() % (1 << g_bpp);
        for (int yy = 0; yy < h; ++yy)
            for (int xx = 0; xx < w; ++xx)
                put_index(vram, x + xx, y + yy, idx);
    } else if (g_mode == Q9_VMODE_RGB565) {
        uint8_t r5 = rand() % 32, g6 = rand() % 64, b5 = rand() % 32;
        for (int yy = 0; yy < h; ++yy)
            for (int xx = 0; xx < w; ++xx)
                put_rgb565(vram, x + xx, y + yy, r5, g6, b5);
    } else if (g_mode == Q9_VMODE_RGB555I) {
        uint8_t r5 = rand() % 32, g5 = rand() % 32, b5 = rand() % 32;
        bool intensity = rand() % 2;
        for (int yy = 0; yy < h; ++yy)
            for (int xx = 0; xx < w; ++xx)
                put_rgb555i(vram, x + xx, y + yy, r5, g5, b5, intensity);
    } else { // RGB888
        uint8_t r = rand() % 256, g = rand() % 256, b = rand() % 256;
        for (int yy = 0; yy < h; ++yy)
            for (int xx = 0; xx < w; ++xx)
                put_rgb888(vram, x + xx, y + yy, r, g, b);
    }
    *out_x = x; *out_y = y; *out_w = w; *out_h = h;
}

struct DirtyRect { int x0, y0, x1, y1; };
const int MAX_DIRTY_RECTS = 16;

static bool rects_overlap(const DirtyRect& a, const DirtyRect& b) {
    return a.x0 < b.x1 && a.x1 > b.x0 && a.y0 < b.y1 && a.y1 > b.y0;
}

static void union_into(DirtyRect* a, const DirtyRect& b) {
    if (b.x0 < a->x0) a->x0 = b.x0;
    if (b.y0 < a->y0) a->y0 = b.y0;
    if (b.x1 > a->x1) a->x1 = b.x1;
    if (b.y1 > a->y1) a->y1 = b.y1;
}

// Fügt ein neues Dirty-Rechteck der Liste hinzu. Überlappt es ein bestehendes,
// wird verschmolzen statt neu angehängt. Läuft die Liste voll, fällt sie auf
// eine einzige Bounding-Box über alle bisherigen Rechtecke zurück, damit der
// Merge-Aufwand pro Tick beschränkt bleibt.
static void add_dirty_rect(DirtyRect list[], int* count, DirtyRect r) {
    for (int i = 0; i < *count; ++i) {
        if (rects_overlap(list[i], r)) {
            union_into(&list[i], r);
            return;
        }
    }
    if (*count < MAX_DIRTY_RECTS) {
        list[(*count)++] = r;
        return;
    }
    DirtyRect merged = r;
    for (int i = 0; i < *count; ++i) union_into(&merged, list[i]);
    list[0] = merged;
    *count = 1;
}

static bool send_dirty_rect(int cli, uint8_t* vram, const DirtyRect& r, uint32_t* seq) {
    int rx = r.x0, ry = r.y0, rw = r.x1 - r.x0, rh = r.y1 - r.y0;
    size_t rect_stride = ((size_t)rw * g_bpp + 7) / 8;
    size_t rect_size = rect_stride * rh;
    size_t x_byte_offset = ((size_t)rx * g_bpp) / 8;

    struct Q9MsgHeader uhdr = {{'Q','9','V','F'}, htons(Q9_PROTO_VERSION), htons(Q9_FRAME_UPDATE),
                                htonl((uint32_t)(sizeof(Q9DirtyRectHeader) + rect_size)), htonl((*seq)++)};
    Q9DirtyRectHeader rect = {(uint16_t)rx, (uint16_t)ry, (uint16_t)rw, (uint16_t)rh};
    dirtyrect_hton(&rect);

    if (write(cli, &uhdr, sizeof(uhdr)) != sizeof(uhdr)) return false;
    if (write(cli, &rect, sizeof(rect)) != sizeof(rect)) return false;
    for (int r2 = 0; r2 < rh; ++r2) {
        ssize_t n = write(cli, vram + (ry + r2) * g_stride + x_byte_offset, rect_stride);
        if (n != (ssize_t)rect_stride) return false;
    }
    printf("Dirty-Update gesendet: x=%d y=%d w=%d h=%d\n", rx, ry, rw, rh);
    return true;
}

// Sendet VIDEO_INFO (+ PALETTE bei indizierten Modi) + FRAME_FULL fuer den
// aktuellen g_mode/fb_width/fb_height-Stand. Genutzt beim initialen Connect
// UND bei einem Live-Moduswechsel waehrend eine Verbindung besteht.
static void send_video_setup(int cli, uint8_t* vram, uint32_t* seq) {
    struct Q9MsgHeader vhdr = {{'Q','9','V','F'}, htons(Q9_PROTO_VERSION), htons(Q9_VIDEO_INFO), htonl(sizeof(Q9VideoInfo)), htonl((*seq)++)};
    write(cli, &vhdr, sizeof(vhdr));
    Q9VideoInfo info = {htons(fb_width), htons(fb_height), htons((uint16_t)g_stride), (uint8_t)g_bpp, (uint8_t)g_mode, {0,0}};
    write(cli, &info, sizeof(info));

    if (q9_mode_is_indexed(g_mode)) {
        Q9ClutEntry clut[Q9_CLUT_ENTRIES];
        memset(clut, 0, sizeof(clut));
        build_grayscale_clut(clut, g_bpp);
        if (g_mode == Q9_VMODE_INDEXED1) clut[1] = {40, 220, 40};
        struct Q9MsgHeader phdr = {{'Q','9','V','F'}, htons(Q9_PROTO_VERSION), htons(Q9_PALETTE), htonl(sizeof(clut)), htonl((*seq)++)};
        write(cli, &phdr, sizeof(phdr));
        write(cli, clut, sizeof(clut));
    }

    memset(vram, 0, g_fb_size);
    draw_test_pattern(vram);
    struct Q9MsgHeader fhdr = {{'Q','9','V','F'}, htons(Q9_PROTO_VERSION), htons(Q9_FRAME_FULL), htonl((uint32_t)g_fb_size), htonl((*seq)++)};
    write(cli, &fhdr, sizeof(fhdr));
    write(cli, vram, g_fb_size);
}

static bool parse_mode(const char* s, Q9VideoMode* mode, int* bpp) {
    struct { const char* name; Q9VideoMode mode; int bpp; } table[] = {
        {"indexed1", Q9_VMODE_INDEXED1, 1},
        {"indexed2", Q9_VMODE_INDEXED2, 2},
        {"indexed4", Q9_VMODE_INDEXED4, 4},
        {"indexed8", Q9_VMODE_INDEXED8, 8},
        {"rgb565",   Q9_VMODE_RGB565,   16},
        {"rgb555i",  Q9_VMODE_RGB555I,  16},
        {"rgb888",   Q9_VMODE_RGB888,   24},
    };
    for (size_t i = 0; i < sizeof(table)/sizeof(table[0]); ++i) {
        if (strcmp(s, table[i].name) == 0) { *mode = table[i].mode; *bpp = table[i].bpp; return true; }
    }
    return false;
}

// Verarbeitet eine Zeile von stdin: "mode <name>" oder "res <BxH>". Bei
// Aenderung wird g_stride/g_fb_size neu berechnet und *vram bei Bedarf
// (Groessenaenderung) neu allokiert. Rueckgabe: ob sich tatsaechlich was aenderte.
static bool apply_command(const char* line, uint8_t** vram) {
    char cmd[32] = {0}, arg[64] = {0};
    if (sscanf(line, "%31s %63s", cmd, arg) != 2) {
        fprintf(stderr, "Unbekannter Befehl (erwartet: 'mode <name>' oder 'res <BxH>')\n");
        return false;
    }
    bool changed = false;
    if (strcmp(cmd, "mode") == 0) {
        Q9VideoMode m; int b;
        if (parse_mode(arg, &m, &b)) { g_mode = m; g_bpp = b; changed = true; }
        else fprintf(stderr, "Unbekannter Modus: %s\n", arg);
    } else if (strcmp(cmd, "res") == 0) {
        int w = 0, h = 0;
        if (sscanf(arg, "%dx%d", &w, &h) == 2 && w > 0 && h > 0) { fb_width = w; fb_height = h; changed = true; }
        else fprintf(stderr, "Ungueltige Aufloesung: %s\n", arg);
    } else {
        fprintf(stderr, "Unbekannter Befehl: %s (erwartet 'mode' oder 'res')\n", cmd);
    }
    if (changed) {
        size_t old_size = g_fb_size;
        g_stride = (int)(((size_t)fb_width * g_bpp + 7) / 8);
        g_fb_size = (size_t)g_stride * fb_height;
        if (g_fb_size != old_size) {
            free(*vram);
            *vram = (uint8_t*)calloc(1, g_fb_size);
        }
        printf("Neuer Stand: %dx%d, Modus %d, bpp=%d, stride=%d\n", fb_width, fb_height, g_mode, g_bpp, g_stride);
    }
    return changed;
}

int main(int argc, char** argv) {
    const char* mode_prefix = "--mode=";
    const char* res_prefix = "--res=";
    for (int i = 1; i < argc; ++i) {
        if (strncmp(argv[i], mode_prefix, strlen(mode_prefix)) == 0) {
            if (!parse_mode(argv[i] + strlen(mode_prefix), &g_mode, &g_bpp)) {
                fprintf(stderr, "Unbekannter Modus: %s (indexed1|indexed2|indexed4|indexed8|rgb565|rgb555i|rgb888)\n", argv[i] + strlen(mode_prefix));
                return 1;
            }
        } else if (strncmp(argv[i], res_prefix, strlen(res_prefix)) == 0) {
            int w = 0, h = 0;
            if (sscanf(argv[i] + strlen(res_prefix), "%dx%d", &w, &h) != 2 || w <= 0 || h <= 0) {
                fprintf(stderr, "Ungueltige Aufloesung: %s (Format: BREITExHOEHE, z.B. 1280x720)\n", argv[i] + strlen(res_prefix));
                return 1;
            }
            fb_width = w; fb_height = h;
        }
    }
    g_stride = (int)(((size_t)fb_width * g_bpp + 7) / 8);
    g_fb_size = (size_t)g_stride * fb_height;
    printf("Aufloesung: %dx%d, Modus: %d, bpp=%d, stride=%d\n", fb_width, fb_height, g_mode, g_bpp, g_stride);

    signal(SIGPIPE, SIG_IGN);
    srand((unsigned)time(NULL));

    // UDP-Discovery-Server
    int udp = socket(AF_INET, SOCK_DGRAM, 0);
    struct sockaddr_in uaddr;
    memset(&uaddr, 0, sizeof(uaddr));
    uaddr.sin_family = AF_INET;
    uaddr.sin_port = htons(2000);
    uaddr.sin_addr.s_addr = INADDR_ANY;
    if (bind(udp, (struct sockaddr*)&uaddr, sizeof(uaddr)) != 0) {
        perror("bind udp:2000");
        return 1;
    }
    // TCP-Server
    int srv = socket(AF_INET, SOCK_STREAM, 0);
    int reuse = 1;
    setsockopt(srv, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(2001);
    addr.sin_addr.s_addr = INADDR_ANY;
    if (bind(srv, (struct sockaddr*)&addr, sizeof(addr)) != 0) {
        perror("bind tcp:2001");
        return 1;
    }
    if (listen(srv, 1) != 0) {
        perror("listen tcp:2001");
        return 1;
    }
    printf("Dummy-Server läuft auf Port 2001...\n");

    uint8_t* vram = (uint8_t*)calloc(1, g_fb_size);

    // Äußere Schleife: nach jedem getrennten Client wieder auf Discovery/neue
    // Verbindung warten, statt den Prozess zu beenden.
    while (1) {
        fd_set fds;
        FD_ZERO(&fds);
        int maxfd = udp > srv ? udp : srv;
        while (1) {
            FD_SET(udp, &fds);
            FD_SET(srv, &fds);
            struct timeval tv = {0, 500000};
            int sel = select(maxfd+1, &fds, 0, 0, &tv);
            if (sel > 0 && FD_ISSET(udp, &fds)) {
                char buf[64];
                struct sockaddr_in from;
                socklen_t fromlen = sizeof(from);
                int n = recvfrom(udp, buf, sizeof(buf)-1, 0, (struct sockaddr*)&from, &fromlen);
                if (n > 0 && strncmp(buf, "Q9_VIDEO_DISCOVER", 17) == 0) {
                    char reply[128];
                    snprintf(reply, sizeof(reply), "DummyQ9;2001;320;240");
                    sendto(udp, reply, strlen(reply), 0, (struct sockaddr*)&from, fromlen);
                }
            }
            if (sel > 0 && FD_ISSET(srv, &fds)) break;
        }
        int cli = accept(srv, 0, 0);
        if (cli < 0) continue;
        printf("Client verbunden.\n");
        struct Q9MsgHeader hdr;
        read(cli, &hdr, sizeof(hdr)); // HELLO empfangen

        uint32_t seq = 2;
        send_video_setup(cli, vram, &seq);

        // Dirty-Tracking: gedeckelte Liste separater Rechtecke (Überlapp-Merge, sonst
        // Bbox-Fallback bei voller Liste). Schreiber (draw_hz) und Sende-Tick (send_hz)
        // sind entkoppelt: der Schreiber läuft kontinuierlich, der Netzwerk-Tick
        // sammelt und sendet die zwischenzeitlich aufgelaufenen Rechtecke gebündelt.
        DirtyRect dirty_rects[MAX_DIRTY_RECTS];
        int dirty_count = 0;
        const double draw_hz = 200.0;
        const double send_hz = 50.0;
        struct timeval draw_tick = {0, (int)(1000000.0 / draw_hz)};
        struct timeval last_send;
        gettimeofday(&last_send, NULL);
        bool disconnected = false;

        while (!disconnected) {
            fd_set readfds;
            FD_ZERO(&readfds);
            FD_SET(0, &readfds); // stdin: Live-Befehle "mode <name>" / "res <BxH>"
            struct timeval tv = draw_tick;
            select(1, &readfds, NULL, NULL, &tv);
            if (FD_ISSET(0, &readfds)) {
                char line[128];
                if (fgets(line, sizeof(line), stdin)) {
                    if (apply_command(line, &vram)) {
                        send_video_setup(cli, vram, &seq);
                        dirty_count = 0;
                    }
                }
            }

            int x, y, w, h;
            draw_random_rect(vram, &x, &y, &w, &h);
            add_dirty_rect(dirty_rects, &dirty_count, {x, y, x + w, y + h});

            struct timeval now;
            gettimeofday(&now, NULL);
            double elapsed_ms = (now.tv_sec - last_send.tv_sec) * 1000.0 + (now.tv_usec - last_send.tv_usec) / 1000.0;
            if (elapsed_ms < 1000.0 / send_hz) continue;
            last_send = now;
            if (dirty_count == 0) continue;

            for (int i = 0; i < dirty_count; ++i) {
                if (!send_dirty_rect(cli, vram, dirty_rects[i], &seq)) { disconnected = true; break; }
            }
            dirty_count = 0;
        }

        printf("Client getrennt, warte auf neue Verbindung...\n");
        close(cli);
    }

    free(vram);
    close(srv);
    close(udp);
    return 0;
}
