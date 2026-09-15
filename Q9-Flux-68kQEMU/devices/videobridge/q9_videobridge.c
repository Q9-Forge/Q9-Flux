/*
 * Q9 board: host video bridge (Q9 Frame protocol).
 *
 * Ported from the Musashi-based implementation in
 * Q9-Flux-68k/src/devices/videobridge/videobridge.c -- the Q9 Frame
 * wire protocol (HELLO/VIDEO_INFO/PALETTE/FRAME_FULL/FRAME_UPDATE over
 * TCP + UDP discovery, s. the original's own header comment for the
 * exact byte layout and the deliberate single-client/no-backpressure
 * simplifications it already documents) is ported 1:1, including the
 * single-non-blocking-send-attempt-per-poll drain and the dirty-rect
 * byte-to-pixel conversion.
 *
 * Unlike every other ported device, this one is NOT part of the guest-
 * visible hardware at all -- the original never registered it with
 * devreg (no q9_devdesc_videobridge, no vtable, s. its own header
 * comment): it's a host-side background service that reads the
 * already-ported framebuf/mc6845/clut devices' internal state (via
 * small cross-file accessors, same pattern as q9_mc6845_get_stride()
 * et al.) and streams it to an external Q9 Frame viewer. Ported here as
 * a plain (non-sysbus, no MemoryRegion/no guest-visible address) QOM
 * device purely for lifecycle management (qdev properties for the
 * ports/name, clean realize/link wiring) -- linked to its three source
 * devices via "framebuf"/"mc6845"/"clut" qdev link properties ("clut"
 * may be left unset, matching the original's own optional-CLUT
 * fallback to a grayscale ramp).
 *
 * The original's own poll-once-per-main-loop-round call site
 * (q9boardrun.c) has no QEMU equivalent (no fixed "round"); replaced
 * with a fixed-rate QEMUTimer (Q9_VIDEOBRIDGE_POLL_MS) driving the same
 * state machine -- fast enough for low handshake/accept latency, with
 * actual frame-send throttling still governed by the CRTC's R19 (net
 * update Hz) exactly as in the original.
 *
 * Plain POSIX sockets (this board's QEMU port is POSIX-only so far, s.
 * the rest of this devices/ tree) rather than QEMU's own qio_channel
 * abstraction -- keeps this a direct, easily-diffable port of the
 * original's own socket calls instead of a rewrite against a different
 * async I/O model.
 *
 * This file is not (yet) part of Q9-Flux's own MIT-licensed code base --
 * it lives inside the vendored QEMU source tree and is licensed under
 * QEMU's own terms (GPL), like the rest of this directory.
 */

#include "qemu/osdep.h"
#include "qapi/error.h"
#include "qemu/error-report.h"
#include "hw/core/qdev-properties.h"
#include "qemu/timer.h"
#include "qom/object.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <signal.h>

#define TYPE_Q9_VIDEOBRIDGE "q9-videobridge"
OBJECT_DECLARE_SIMPLE_TYPE(Q9VideobridgeState, Q9_VIDEOBRIDGE)

/* Matches Q9_VIDEOBRIDGE_TCP_PORT/_UDP_PORT/_NAME_MAX in
 * Q9-Flux-68k/src/devices/videobridge/videobridge.h. */
#define Q9_VIDEOBRIDGE_TCP_PORT_DEFAULT 2001
#define Q9_VIDEOBRIDGE_UDP_PORT_DEFAULT 2000

/* No "once per main loop round" in QEMU, s. Dateikopf -- fixed-rate
 * housekeeping poll instead (accept/handshake/drain progress); actual
 * frame-send rate is still throttled by the CRTC's R19 further down. */
#define Q9_VIDEOBRIDGE_POLL_MS 10

#define Q9_MAGIC "Q9VF"
#define Q9_PROTO_VERSION_WIRE 1

enum {
    Q9_HELLO        = 1,
    Q9_VIDEO_INFO   = 2,
    Q9_FRAME_FULL   = 3,
    Q9_FRAME_UPDATE = 4,
    Q9_PALETTE      = 9,
};

struct Q9MsgHeader {
    char magic[4];
    uint16_t version;
    uint16_t type;
    uint32_t length;
    uint32_t sequence;
} QEMU_PACKED;

struct Q9VideoInfoWire {
    uint16_t width;
    uint16_t height;
    uint16_t stride;
    uint8_t bpp;
    uint8_t mode;
    uint8_t reserved[2];
} QEMU_PACKED;

struct Q9DirtyRectWire {
    uint16_t x, y, w, h;
} QEMU_PACKED;

struct Q9ClutEntryWire {
    uint8_t r, g, b;
} QEMU_PACKED;

#define Q9_CLUT_ENTRIES 256
#define HELLO_SIZE ((int)sizeof(struct Q9MsgHeader))

/* Cross-device accessors, s. Dateikopf. */
void q9_mc6845_get_info(DeviceState *dev, uint32_t *stride, uint32_t *height,
                         int *mode, int *bpp, uint32_t *width_px,
                         uint32_t *net_hz);
uint32_t q9_clut_get_table(DeviceState *dev, uint8_t *entry_r, uint8_t *entry_g,
                            uint8_t *entry_b);
uint8_t *q9_framebuf_get_vram(DeviceState *dev, uint32_t *size);
int q9_framebuf_get_dirty_count(DeviceState *dev);
void q9_framebuf_get_dirty_rect(DeviceState *dev, int index, int *x0, int *y0,
                                 int *x1, int *y1);
void q9_framebuf_clear_dirty(DeviceState *dev);

struct Q9VideobridgeState {
    DeviceState parent_obj;

    DeviceState *fb;      /* "framebuf" link, required   */
    DeviceState *crtc;    /* "mc6845" link, required     */
    DeviceState *clut;    /* "clut" link, optional       */

    uint16_t tcp_port;
    uint16_t udp_port;
    char *name;

    int udp_fd, tcp_fd, client_fd;
    int handshake_have, handshake_done;
    uint32_t seq;
    int64_t last_send_ms;

    uint32_t adv_stride, adv_height;
    int adv_mode;
    uint32_t adv_clut_gen;

    uint8_t *out_buf;      /* g_malloc'd, s. realize() */
    uint32_t out_buf_cap;
    uint32_t out_len, out_sent;
    int out_pending;

    QEMUTimer *poll_timer;
};

static void q9_set_nonblock(int fd)
{
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags >= 0) {
        fcntl(fd, F_SETFL, flags | O_NONBLOCK);
    }
}

static void disconnect_client(Q9VideobridgeState *s)
{
    if (s->client_fd >= 0) {
        close(s->client_fd);
    }
    s->client_fd = -1;
    s->handshake_have = 0;
    s->handshake_done = 0;
    s->out_pending = 0;
    s->out_sent = 0;
    s->out_len = 0;
}

static void build_grayscale_clut(struct Q9ClutEntryWire *clut, int bpp)
{
    int entries = 1 << bpp;
    int i;

    for (i = 0; i < entries; i++) {
        uint8_t v = (uint8_t)(entries > 1 ? i * 255 / (entries - 1) : 255);
        clut[i].r = v;
        clut[i].g = v;
        clut[i].b = v;
    }
}

static void poll_udp_discovery(Q9VideobridgeState *s)
{
    char buf[64];
    struct sockaddr_in from;
    socklen_t fromlen = sizeof(from);
    ssize_t n = recvfrom(s->udp_fd, buf, sizeof(buf) - 1, 0,
                          (struct sockaddr *)&from, &fromlen);

    if (n > 0) {
        buf[n] = '\0';
        if (strncmp(buf, "Q9_VIDEO_DISCOVER", 17) == 0) {
            char reply[96];
            uint32_t w, h;
            int rlen;

            q9_mc6845_get_info(s->crtc, NULL, &h, NULL, NULL, &w, NULL);
            rlen = snprintf(reply, sizeof(reply), "%s;%u;%u;%u", s->name,
                             (unsigned)s->tcp_port, (unsigned)w, (unsigned)h);
            sendto(s->udp_fd, reply, (size_t)rlen, 0,
                   (struct sockaddr *)&from, fromlen);
        }
    }
}

static void poll_accept(Q9VideobridgeState *s)
{
    int incoming = accept(s->tcp_fd, NULL, NULL);
    int one = 1;

    if (incoming < 0) {
        return;
    }
    if (s->client_fd >= 0) {
        close(incoming);   /* no multi-client, s. Dateikopf */
        return;
    }
    q9_set_nonblock(incoming);
    setsockopt(incoming, IPPROTO_TCP, TCP_NODELAY, &one, sizeof(one));
    s->client_fd = incoming;
    s->handshake_have = 0;
    s->handshake_done = 0;
    s->out_pending = 0;
}

static void try_recv_hello(Q9VideobridgeState *s)
{
    uint8_t scratch[HELLO_SIZE];
    int need = HELLO_SIZE - s->handshake_have;
    ssize_t n = recv(s->client_fd, scratch, (size_t)need, 0);

    if (n > 0) {
        s->handshake_have += (int)n;
        return;
    }
    if (n == 0 || (n < 0 && errno != EAGAIN && errno != EWOULDBLOCK)) {
        disconnect_client(s);
    }
}

static void put_header(uint8_t *out, uint32_t *len, uint16_t type,
                        uint32_t length, uint32_t seq)
{
    struct Q9MsgHeader h;

    memcpy(h.magic, Q9_MAGIC, 4);
    h.version = htons(Q9_PROTO_VERSION_WIRE);
    h.type = htons(type);
    h.length = htonl(length);
    h.sequence = htonl(seq);
    memcpy(out + *len, &h, sizeof(h));
    *len += sizeof(h);
}

static void build_full_setup(Q9VideobridgeState *s)
{
    uint32_t stride, height, width, net_hz;
    int mode, bpp;
    uint32_t frame_size, fb_size;
    uint8_t *vram = q9_framebuf_get_vram(s->fb, &fb_size);
    uint8_t *out = s->out_buf;
    uint32_t len = 0;

    q9_mc6845_get_info(s->crtc, &stride, &height, &mode, &bpp, &width, &net_hz);
    frame_size = stride * height;
    if (frame_size > fb_size) {
        frame_size = fb_size;   /* safety clamp, s. original */
    }

    put_header(out, &len, Q9_VIDEO_INFO, sizeof(struct Q9VideoInfoWire), s->seq++);
    {
        struct Q9VideoInfoWire info;
        info.width = htons((uint16_t)width);
        info.height = htons((uint16_t)height);
        info.stride = htons((uint16_t)stride);
        info.bpp = (uint8_t)bpp;
        info.mode = (uint8_t)mode;
        info.reserved[0] = 0;
        info.reserved[1] = 0;
        memcpy(out + len, &info, sizeof(info));
        len += sizeof(info);
    }

    if (mode >= 0 && mode <= 3) {   /* Q9_MC6845_MODE_INDEXED1..8 */
        struct Q9ClutEntryWire clut[Q9_CLUT_ENTRIES];
        uint8_t cr[Q9_CLUT_ENTRIES], cg[Q9_CLUT_ENTRIES], cb[Q9_CLUT_ENTRIES];
        uint32_t gen;
        int i;

        memset(clut, 0, sizeof(clut));
        gen = s->clut ? q9_clut_get_table(s->clut, cr, cg, cb) : 0;
        if (s->clut) {
            for (i = 0; i < Q9_CLUT_ENTRIES; i++) {
                clut[i].r = cr[i];
                clut[i].g = cg[i];
                clut[i].b = cb[i];
            }
            s->adv_clut_gen = gen;
        } else {
            build_grayscale_clut(clut, bpp);
        }
        put_header(out, &len, Q9_PALETTE, sizeof(clut), s->seq++);
        memcpy(out + len, clut, sizeof(clut));
        len += sizeof(clut);
    }

    put_header(out, &len, Q9_FRAME_FULL, frame_size, s->seq++);
    memcpy(out + len, vram, frame_size);
    len += frame_size;

    s->out_len = len;
    s->out_sent = 0;
    s->out_pending = 1;
    s->adv_stride = stride;
    s->adv_height = height;
    s->adv_mode = mode;
    q9_framebuf_clear_dirty(s->fb);
}

static void build_dirty_update(Q9VideobridgeState *s)
{
    int n = q9_framebuf_get_dirty_count(s->fb);
    uint32_t stride, height, width, net_hz;
    int mode, bpp;
    uint8_t *vram = q9_framebuf_get_vram(s->fb, NULL);
    uint8_t *out = s->out_buf;
    uint32_t len = 0;
    int i;

    q9_mc6845_get_info(s->crtc, &stride, &height, &mode, &bpp, &width, &net_hz);
    (void)height; (void)width; (void)net_hz; (void)mode;

    for (i = 0; i < n; i++) {
        int x0, y0, x1, y1;
        uint32_t byte_w, rows, rect_size, px_x, px_w, row;
        struct Q9DirtyRectWire rect;

        q9_framebuf_get_dirty_rect(s->fb, i, &x0, &y0, &x1, &y1);
        byte_w = (uint32_t)(x1 - x0);
        rows = (uint32_t)(y1 - y0);
        rect_size = byte_w * rows;

        if (bpp < 8) {
            px_x = (uint32_t)x0 * (uint32_t)(8 / bpp);
            px_w = byte_w * (uint32_t)(8 / bpp);
        } else {
            px_x = (uint32_t)x0 / (uint32_t)(bpp / 8);
            px_w = byte_w / (uint32_t)(bpp / 8);
        }

        put_header(out, &len, Q9_FRAME_UPDATE,
                   (uint32_t)sizeof(rect) + rect_size, s->seq++);
        rect.x = htons((uint16_t)px_x);
        rect.y = htons((uint16_t)y0);
        rect.w = htons((uint16_t)px_w);
        rect.h = htons((uint16_t)rows);
        memcpy(out + len, &rect, sizeof(rect));
        len += sizeof(rect);

        for (row = 0; row < rows; row++) {
            uint32_t voff = (uint32_t)(y0 + (int)row) * stride + (uint32_t)x0;
            memcpy(out + len, vram + voff, byte_w);
            len += byte_w;
        }
    }

    s->out_len = len;
    s->out_sent = 0;
    s->out_pending = (len > 0);
    q9_framebuf_clear_dirty(s->fb);
}

static void drain_output(Q9VideobridgeState *s)
{
    ssize_t n = send(s->client_fd, s->out_buf + s->out_sent,
                      s->out_len - s->out_sent, 0);

    if (n > 0) {
        s->out_sent += (uint32_t)n;
        if (s->out_sent >= s->out_len) {
            s->out_pending = 0;
            s->handshake_done = 1;
        }
        return;
    }
    if (n < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
        return;
    }
    disconnect_client(s);
}

static bool geometry_changed(Q9VideobridgeState *s)
{
    uint32_t stride, height, width, net_hz;
    int mode, bpp;
    bool clut_changed = false;

    q9_mc6845_get_info(s->crtc, &stride, &height, &mode, &bpp, &width, &net_hz);
    if (s->clut && mode >= 0 && mode <= 3) {
        uint8_t cr[Q9_CLUT_ENTRIES], cg[Q9_CLUT_ENTRIES], cb[Q9_CLUT_ENTRIES];
        uint32_t gen = q9_clut_get_table(s->clut, cr, cg, cb);
        clut_changed = (gen != s->adv_clut_gen);
    }
    return stride != s->adv_stride || height != s->adv_height ||
           mode != s->adv_mode || clut_changed;
}

static void q9_videobridge_tick(void *opaque)
{
    Q9VideobridgeState *s = opaque;
    int64_t now_ms = qemu_clock_get_ms(QEMU_CLOCK_VIRTUAL);
    uint32_t stride, height, width, net_hz;
    int mode, bpp;

    timer_mod(s->poll_timer, now_ms + Q9_VIDEOBRIDGE_POLL_MS);

    poll_udp_discovery(s);
    poll_accept(s);

    if (s->client_fd < 0) {
        return;
    }
    if (s->out_pending) {
        drain_output(s);
        return;
    }
    if (!s->handshake_done) {
        if (s->handshake_have < HELLO_SIZE) {
            try_recv_hello(s);
            if (s->client_fd < 0 || s->handshake_have < HELLO_SIZE) {
                return;
            }
        }
        build_full_setup(s);
        s->last_send_ms = now_ms;
        return;
    }
    if (geometry_changed(s)) {
        build_full_setup(s);
        s->last_send_ms = now_ms;
        return;
    }

    q9_mc6845_get_info(s->crtc, &stride, &height, &mode, &bpp, &width, &net_hz);
    if ((uint32_t)(now_ms - s->last_send_ms) < 1000u / net_hz) {
        return;
    }
    s->last_send_ms = now_ms;

    if (q9_framebuf_get_dirty_count(s->fb) == 0) {
        return;
    }
    build_dirty_update(s);
}

static void q9_videobridge_realize(DeviceState *dev, Error **errp)
{
    Q9VideobridgeState *s = Q9_VIDEOBRIDGE(dev);
    struct sockaddr_in addr;
    int reuse = 1;
    uint32_t fb_size;

    if (!s->fb || !s->crtc) {
        error_setg(errp, "q9-videobridge: 'framebuf' and 'mc6845' link "
                   "properties are required");
        return;
    }

    q9_framebuf_get_vram(s->fb, &fb_size);
    s->out_buf_cap = fb_size + 4096u;
    s->out_buf = g_malloc(s->out_buf_cap);

    s->client_fd = -1;
    s->seq = 2;   /* 1 is implicitly the client's own HELLO sequence */
    if (!s->name || !s->name[0]) {
        g_free(s->name);
        s->name = g_strdup("Q9Flux");
    }

    signal(SIGPIPE, SIG_IGN);   /* s. original: a dropped client must not kill us */

    /* Socket setup failures (most commonly: another Q9-Flux instance --
     * Musashi or this one -- already holding the port) are warnings, not
     * fatal machine-startup errors, matching the original's own
     * q9_nettty_init()/q9_videobridge_init(): log and leave the bridge
     * unreachable for this session rather than crash. warn-and-return
     * below skips arming the poll timer, so nothing ever touches a
     * half-set-up fd afterwards. */
    s->udp_fd = -1;
    s->tcp_fd = -1;

    s->udp_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (s->udp_fd < 0) {
        warn_report("q9-videobridge: socket(udp) failed: %s -- "
                    "video bridge unavailable this session", strerror(errno));
        return;
    }
    q9_set_nonblock(s->udp_fd);
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(s->udp_port);
    addr.sin_addr.s_addr = INADDR_ANY;
    if (bind(s->udp_fd, (struct sockaddr *)&addr, sizeof(addr)) != 0) {
        warn_report("q9-videobridge: bind(udp %u) failed: %s -- "
                    "video bridge unavailable this session (port busy?)",
                    (unsigned)s->udp_port, strerror(errno));
        close(s->udp_fd);
        s->udp_fd = -1;
        return;
    }

    s->tcp_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (s->tcp_fd < 0) {
        warn_report("q9-videobridge: socket(tcp) failed: %s -- "
                    "video bridge unavailable this session", strerror(errno));
        close(s->udp_fd);
        s->udp_fd = -1;
        return;
    }
    setsockopt(s->tcp_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
    q9_set_nonblock(s->tcp_fd);
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(s->tcp_port);
    addr.sin_addr.s_addr = INADDR_ANY;
    if (bind(s->tcp_fd, (struct sockaddr *)&addr, sizeof(addr)) != 0) {
        warn_report("q9-videobridge: bind(tcp %u) failed: %s -- "
                    "video bridge unavailable this session (port busy?)",
                    (unsigned)s->tcp_port, strerror(errno));
        close(s->udp_fd);
        close(s->tcp_fd);
        s->udp_fd = s->tcp_fd = -1;
        return;
    }
    if (listen(s->tcp_fd, 1) != 0) {
        warn_report("q9-videobridge: listen(tcp) failed: %s -- "
                    "video bridge unavailable this session", strerror(errno));
        close(s->udp_fd);
        close(s->tcp_fd);
        s->udp_fd = s->tcp_fd = -1;
        return;
    }

    s->poll_timer = timer_new_ms(QEMU_CLOCK_VIRTUAL, q9_videobridge_tick, s);
    timer_mod(s->poll_timer,
              qemu_clock_get_ms(QEMU_CLOCK_VIRTUAL) + Q9_VIDEOBRIDGE_POLL_MS);
}

static const Property q9_videobridge_properties[] = {
    DEFINE_PROP_LINK("framebuf", Q9VideobridgeState, fb, TYPE_DEVICE, DeviceState *),
    DEFINE_PROP_LINK("mc6845", Q9VideobridgeState, crtc, TYPE_DEVICE, DeviceState *),
    DEFINE_PROP_LINK("clut", Q9VideobridgeState, clut, TYPE_DEVICE, DeviceState *),
    DEFINE_PROP_UINT16("tcp-port", Q9VideobridgeState, tcp_port,
                        Q9_VIDEOBRIDGE_TCP_PORT_DEFAULT),
    DEFINE_PROP_UINT16("udp-port", Q9VideobridgeState, udp_port,
                        Q9_VIDEOBRIDGE_UDP_PORT_DEFAULT),
    DEFINE_PROP_STRING("name", Q9VideobridgeState, name),
};

static void q9_videobridge_class_init(ObjectClass *oc, const void *data)
{
    DeviceClass *dc = DEVICE_CLASS(oc);

    dc->desc = "Q9 board host video bridge (Q9 Frame protocol)";
    dc->realize = q9_videobridge_realize;
    device_class_set_props(dc, q9_videobridge_properties);
    set_bit(DEVICE_CATEGORY_DISPLAY, dc->categories);
}

static const TypeInfo q9_videobridge_info = {
    .name          = TYPE_Q9_VIDEOBRIDGE,
    .parent        = TYPE_DEVICE,
    .instance_size = sizeof(Q9VideobridgeState),
    .class_init    = q9_videobridge_class_init,
};

static void q9_videobridge_register_types(void)
{
    type_register_static(&q9_videobridge_info);
}

type_init(q9_videobridge_register_types)
