//════════════════════════════════════════════════════════════════════════════════════════════════
// File:   videobridge.c                                                                  Ver. 1.00
// Owner:  AF
// Desc.:  5.27: Implementierung, s. videobridge.h fuer Design/Vereinfachungen.
//
// Edition History
//─────────┬──────┬────────────────────────────────────────────────────────────────────────┬──────
// Date    │ Ver. │ Description                                                            │ By
//─────────┼──────┼────────────────────────────────────────────────────────────────────────┼──────
// 26-08-03│ 1.00 │ 5.27: Erster Wurf                                                       │ Ada
//═════════╧══════╧════════════════════════════════════════════════════════════════════════╧══════
#include "videobridge.h"
#include "../../kernel/q9_sockcompat.h"    /* Windows-Build: Windows/Winsock-Portabilitaet fuer den Netzwerk-Teil */
#include <string.h>
#include <stdio.h>

//────────────────────────────────────────────────────────────────────────────────────────────────
// Wire-Format: spiegelt Q9-Frame `src/protocol.h` + `src/framebuffer.h` 1:1 (identisches Byte-
// Layout, alle Felder network-byte-order) -- KEIN Cross-Project-Include (dort C++, hier C99), nur
// die Struktur muss uebereinstimmen. Keine Paddings: jedes Feld liegt bereits an seiner natuerlichen
// Alignment-Grenze (die Reihenfolge in beiden Projekten ist bewusst so gewaehlt), daher genuegt ein
// gewoehnliches struct ohne #pragma pack.
//────────────────────────────────────────────────────────────────────────────────────────────────
#define Q9_MAGIC "Q9VF"
#define Q9_PROTO_VERSION_WIRE 1

enum {
    Q9_HELLO         = 1,
    Q9_VIDEO_INFO    = 2,
    Q9_FRAME_FULL    = 3,
    Q9_FRAME_UPDATE  = 4,
    Q9_PALETTE       = 9
};

struct Q9MsgHeader {
    char     magic[4];
    uint16_t version;
    uint16_t type;
    uint32_t length;
    uint32_t sequence;
};

struct Q9VideoInfoWire {
    uint16_t width;
    uint16_t height;
    uint16_t stride;
    uint8_t  bpp;
    uint8_t  mode;
    uint8_t  reserved[2];
};

struct Q9DirtyRectWire {
    uint16_t x, y, w, h;
};

struct Q9ClutEntryWire {
    uint8_t r, g, b;
};

#define Q9_CLUT_ENTRIES 256

//────────────────────────────────────────────────────────────────────────────────────────────────
// Function: build_grayscale_clut
// Desc.:    Portierung aus Q9-Frame `src/framebuffer.cpp` -- Platzhalter-Palette fuer indizierte
//           Modi, solange kein eigenes CLUT-Geraet existiert (s. videobridge.h, "Bewusste
//           Vereinfachungen").
//────────────────────────────────────────────────────────────────────────────────────────────────
static void build_grayscale_clut(struct Q9ClutEntryWire *clut, int bpp)
{
    int entries = 1 << bpp;
    int i;
    for (i = 0; i < entries; i++) {
        uint8_t v = (uint8_t)(entries > 1 ? i * 255 / (entries - 1) : 255);
        clut[i].r = v; clut[i].g = v; clut[i].b = v;
    }
}

#define HELLO_SIZE ((int)sizeof(struct Q9MsgHeader))

static void disconnect_client(q9_videobridge_t *vb)
{
    if (vb->client_fd >= 0) {
        Q9_SOCK_CLOSE(vb->client_fd);
    }
    vb->client_fd        = -1;
    vb->handshake_have   = 0;
    vb->handshake_done   = 0;
    vb->out_pending      = 0;
    vb->out_sent         = 0;
    vb->out_len          = 0;
}

//────────────────────────────────────────────────────────────────────────────────────────────────
// Function: q9_videobridge_init
//────────────────────────────────────────────────────────────────────────────────────────────────
int q9_videobridge_init(q9_videobridge_t *vb, q9_framebuf_t *fb, const q9_mc6845_t *crtc,
                         const q9_clut_t *clut, uint16_t tcp_port, uint16_t udp_port,
                         const char *name)
{
    struct sockaddr_in addr;
    int reuse = 1;

    memset(vb, 0, sizeof(*vb));
    vb->client_fd = -1;
    vb->fb        = fb;
    vb->crtc      = crtc;
    vb->clut      = clut;
    vb->tcp_port  = tcp_port;
    vb->seq       = 2;                     /* 1 ist implizit die HELLO-Sequence des Clients */
    strncpy(vb->name, (name && name[0]) ? name : "Q9Flux", sizeof(vb->name) - 1);

    q9_sock_startup();                     /* Windows-Build: WSAStartup unter Windows, no-op auf POSIX */
    Q9_SOCK_IGNORE_SIGPIPE();              /* getrennter Client waehrend send() darf den Emulator */
                                            /* nicht per Default-SIGPIPE-Handler beenden           */

    vb->udp_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (vb->udp_fd < 0) {
        return -1;
    }
    Q9_SOCK_NONBLOCK(vb->udp_fd);
    memset(&addr, 0, sizeof(addr));
    addr.sin_family      = AF_INET;
    addr.sin_port        = htons(udp_port);
    addr.sin_addr.s_addr = INADDR_ANY;
    if (bind(vb->udp_fd, (struct sockaddr *)&addr, sizeof(addr)) != 0) {
        q9_sock_perror("q9_videobridge: bind udp");
        return -1;
    }

    vb->tcp_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (vb->tcp_fd < 0) {
        return -1;
    }
    setsockopt(vb->tcp_fd, SOL_SOCKET, SO_REUSEADDR, (const char *)&reuse, sizeof(reuse));
    Q9_SOCK_NONBLOCK(vb->tcp_fd);
    memset(&addr, 0, sizeof(addr));
    addr.sin_family      = AF_INET;
    addr.sin_port        = htons(tcp_port);
    addr.sin_addr.s_addr = INADDR_ANY;
    if (bind(vb->tcp_fd, (struct sockaddr *)&addr, sizeof(addr)) != 0) {
        q9_sock_perror("q9_videobridge: bind tcp");
        return -1;
    }
    if (listen(vb->tcp_fd, 1) != 0) {
        q9_sock_perror("q9_videobridge: listen tcp");
        return -1;
    }

    printf("[Q9 Video] Bridge auf TCP %u (Video) / UDP %u (Discovery), Name '%s'\r\n",
           (unsigned)tcp_port, (unsigned)udp_port, vb->name);
    return 0;
}

//────────────────────────────────────────────────────────────────────────────────────────────────
// Function: poll_udp_discovery / poll_accept
// Desc.:    5.27: je einmal pro Poll-Runde -- beide nichtblockierend (O_NONBLOCK-Sockets).
//────────────────────────────────────────────────────────────────────────────────────────────────
static void poll_udp_discovery(q9_videobridge_t *vb)
{
    char buf[64];
    struct sockaddr_in from;
    socklen_t fromlen = sizeof(from);
    ssize_t n = recvfrom(vb->udp_fd, buf, sizeof(buf) - 1, 0, (struct sockaddr *)&from, &fromlen);

    if (n > 0) {
        buf[n] = '\0';
        if (strncmp(buf, "Q9_VIDEO_DISCOVER", 17) == 0) {
            char reply[96];
            uint32_t w = q9_mc6845_width_px(vb->crtc);
            uint32_t h = q9_mc6845_height(vb->crtc);
            int rlen = snprintf(reply, sizeof(reply), "%s;%u;%u;%u",
                                 vb->name, (unsigned)vb->tcp_port, (unsigned)w, (unsigned)h);
            sendto(vb->udp_fd, reply, (size_t)rlen, 0, (struct sockaddr *)&from, fromlen);
        }
    }
}

static void poll_accept(q9_videobridge_t *vb)
{
    int incoming = accept(vb->tcp_fd, NULL, NULL);
    if (incoming < 0) {
        return;
    }
    if (vb->client_fd >= 0) {
        /* Kein Multi-Client (s. videobridge.h) -- Verbindung sofort wieder schliessen. */
        Q9_SOCK_CLOSE(incoming);
        return;
    }
    Q9_SOCK_NONBLOCK(incoming);
    {
        int one = 1;
        setsockopt(incoming, IPPROTO_TCP, TCP_NODELAY, (const char *)&one, sizeof(one));
    }
    vb->client_fd      = incoming;
    vb->handshake_have = 0;
    vb->handshake_done = 0;
    vb->out_pending    = 0;
    printf("[Q9 Video] Client verbunden.\r\n");
}

//────────────────────────────────────────────────────────────────────────────────────────────────
// Function: try_recv_hello
// Desc.:    HELLO-Header des Clients einsammeln (Inhalt wird wie in dummy_server.cpp nicht
//           geprueft) -- akkumuliert ueber mehrere Poll-Runden, falls der Header fragmentiert
//           eintrifft (bei 16 Byte in der Praxis fast immer in einem Rutsch).
//────────────────────────────────────────────────────────────────────────────────────────────────
static void try_recv_hello(q9_videobridge_t *vb)
{
    uint8_t scratch[HELLO_SIZE];
    int need = HELLO_SIZE - vb->handshake_have;
    ssize_t n = recv(vb->client_fd, (char *)scratch, (size_t)need, 0);

    if (n > 0) {
        vb->handshake_have += (int)n;
        return;
    }
    if (n == 0 || (n < 0 && !Q9_SOCK_WOULDBLOCK())) {
        disconnect_client(vb);
    }
}

//────────────────────────────────────────────────────────────────────────────────────────────────
// Function: build_full_setup
// Desc.:    VIDEO_INFO(+PALETTE bei indizierten Modi)+FRAME_FULL in vb->out_buf zusammenstellen
//           (ein zusammenhaengender Sendepuffer, s. videobridge.h "nicht-blockierend") und den
//           aktuellen Geometrie-/Modus-Stand als "an den Client gemeldet" festhalten. Dirty-Liste
//           wird SOFORT geleert (Snapshot-Zeitpunkt = jetzt) -- waehrend des mehrere Poll-Runden
//           dauernden Absendens neu eintreffende Aenderungen sammeln sich als ganz normale Dirty-
//           Rects fuer die naechste Update-Runde NACH abgeschlossenem Handshake.
//────────────────────────────────────────────────────────────────────────────────────────────────
static void build_full_setup(q9_videobridge_t *vb)
{
    uint32_t stride = q9_mc6845_stride(vb->crtc);
    uint32_t height = q9_mc6845_height(vb->crtc);
    int      mode   = q9_mc6845_mode(vb->crtc);
    int      bpp    = q9_mc6845_bpp(mode);
    uint32_t width  = q9_mc6845_width_px(vb->crtc);
    uint32_t frame_size = stride * height;
    uint8_t *out = vb->out_buf;
    uint32_t len = 0;
    struct Q9MsgHeader h;

    if (frame_size > q9_framebuf_size(vb->fb)) {
        /* Sicherheitsclamp: OS-9-Treiber hat eine Geometrie programmiert, die groesser ist als
           das konfigurierte VRAM-Fenster (s. framebuf.h) -- kein Puffer-Ueberlauf, Rest bleibt
           schwarz/undefiniert statt eines Crashes (analog "alles R/W ohne Grenzpruefung" bei den
           6845-Registern). */
        frame_size = q9_framebuf_size(vb->fb);
    }

    memcpy(h.magic, Q9_MAGIC, 4);
    h.version = htons(Q9_PROTO_VERSION_WIRE);
    h.type = htons(Q9_VIDEO_INFO);
    h.length = htonl(sizeof(struct Q9VideoInfoWire));
    h.sequence = htonl(vb->seq++);
    memcpy(out + len, &h, sizeof(h)); len += sizeof(h);
    {
        struct Q9VideoInfoWire info;
        info.width  = htons((uint16_t)width);
        info.height = htons((uint16_t)height);
        info.stride = htons((uint16_t)stride);
        info.bpp    = (uint8_t)bpp;
        info.mode   = (uint8_t)mode;
        info.reserved[0] = 0; info.reserved[1] = 0;
        memcpy(out + len, &info, sizeof(info)); len += sizeof(info);
    }

    if (mode >= Q9_MC6845_MODE_INDEXED1 && mode <= Q9_MC6845_MODE_INDEXED8) {
        struct Q9ClutEntryWire clut[Q9_CLUT_ENTRIES];
        memset(clut, 0, sizeof(clut));
        if (vb->clut) {
            /* 5.29-Nachtrag: echte, vom Gast per SS_clut/SS_clutall programmierte CLUT (clut.h) --
               vorher gab es hier nur die feste Graustufen-Platzhalterpalette (s. build_grayscale_
               clut-Kommentar). */
            int i;
            for (i = 0; i < Q9_CLUT_ENTRIES; i++) {
                clut[i].r = vb->clut->r[i];
                clut[i].g = vb->clut->g[i];
                clut[i].b = vb->clut->b[i];
            }
            vb->adv_clut_gen = vb->clut->generation;
        } else {
            build_grayscale_clut(clut, bpp);
        }
        memcpy(h.magic, Q9_MAGIC, 4);
        h.version = htons(Q9_PROTO_VERSION_WIRE);
        h.type = htons(Q9_PALETTE);
        h.length = htonl(sizeof(clut));
        h.sequence = htonl(vb->seq++);
        memcpy(out + len, &h, sizeof(h)); len += sizeof(h);
        memcpy(out + len, clut, sizeof(clut)); len += sizeof(clut);
    }

    memcpy(h.magic, Q9_MAGIC, 4);
    h.version = htons(Q9_PROTO_VERSION_WIRE);
    h.type = htons(Q9_FRAME_FULL);
    h.length = htonl(frame_size);
    h.sequence = htonl(vb->seq++);
    memcpy(out + len, &h, sizeof(h)); len += sizeof(h);
    memcpy(out + len, q9_framebuf_vram(vb->fb), frame_size); len += frame_size;

    vb->out_len     = len;
    vb->out_sent    = 0;
    vb->out_pending = 1;
    vb->adv_stride  = stride;
    vb->adv_height  = height;
    vb->adv_mode    = mode;
    q9_framebuf_dirty_clear(vb->fb);
}

//────────────────────────────────────────────────────────────────────────────────────────────────
// Function: build_dirty_update
// Desc.:    Alle aktuell anstehenden Dirty-Rects (framebuf.h, Byte-Spalte x Zeile) als eine Folge
//           von FRAME_UPDATE-Nachrichten in vb->out_buf zusammenstellen -- Byte->Pixel-Umrechnung
//           ueber q9_mc6845_bpp() (s. videobridge.h-Kopf).
//────────────────────────────────────────────────────────────────────────────────────────────────
static void build_dirty_update(q9_videobridge_t *vb)
{
    int      n      = q9_framebuf_dirty_count(vb->fb);
    int      bpp    = q9_mc6845_bpp(vb->adv_mode);
    uint32_t stride = vb->adv_stride;
    uint8_t *out    = vb->out_buf;
    uint32_t len    = 0;
    int      i;

    for (i = 0; i < n; i++) {
        q9_fb_dirty_rect_t r = q9_framebuf_dirty_rect(vb->fb, i);
        uint32_t byte_w = (uint32_t)(r.x1 - r.x0);
        uint32_t rows   = (uint32_t)(r.y1 - r.y0);
        uint32_t rect_size = byte_w * rows;
        uint32_t px_x, px_w;
        uint32_t row;
        struct Q9MsgHeader h;
        struct Q9DirtyRectWire rect;

        if (bpp < 8) {
            px_x = (uint32_t)r.x0 * (uint32_t)(8 / bpp);
            px_w = byte_w        * (uint32_t)(8 / bpp);
        } else {
            px_x = (uint32_t)r.x0 / (uint32_t)(bpp / 8);
            px_w = byte_w        / (uint32_t)(bpp / 8);
        }

        memcpy(h.magic, Q9_MAGIC, 4);
        h.version = htons(Q9_PROTO_VERSION_WIRE);
        h.type = htons(Q9_FRAME_UPDATE);
        h.length = htonl((uint32_t)sizeof(rect) + rect_size);
        h.sequence = htonl(vb->seq++);
        memcpy(out + len, &h, sizeof(h)); len += sizeof(h);

        rect.x = htons((uint16_t)px_x);
        rect.y = htons((uint16_t)r.y0);
        rect.w = htons((uint16_t)px_w);
        rect.h = htons((uint16_t)rows);
        memcpy(out + len, &rect, sizeof(rect)); len += sizeof(rect);

        for (row = 0; row < rows; row++) {
            uint32_t voff = (uint32_t)(r.y0 + (int)row) * stride + (uint32_t)r.x0;
            memcpy(out + len, q9_framebuf_vram(vb->fb) + voff, byte_w);
            len += byte_w;
        }
    }

    vb->out_len     = len;
    vb->out_sent    = 0;
    vb->out_pending = (len > 0);
    q9_framebuf_dirty_clear(vb->fb);
}

//────────────────────────────────────────────────────────────────────────────────────────────────
// Function: drain_output
// Desc.:    Genau EIN nichtblockierender send()-Versuch je Poll-Runde -- grosse Payloads (volles
//           Frame) ziehen sich so ueber mehrere Runden, ohne die CPU-Emulation je zu blockieren
//           (s. videobridge.h). EAGAIN/EWOULDBLOCK: naechste Runde erneut versuchen. Jeder andere
//           Fehler oder ein kurzer Schreibzugriff von 0 Byte: Client gilt als weg.
//────────────────────────────────────────────────────────────────────────────────────────────────
static void drain_output(q9_videobridge_t *vb)
{
    ssize_t n = send(vb->client_fd, (const char *)(vb->out_buf + vb->out_sent), vb->out_len - vb->out_sent, 0);

    if (n > 0) {
        vb->out_sent += (uint32_t)n;
        if (vb->out_sent >= vb->out_len) {
            vb->out_pending    = 0;
            vb->handshake_done = 1;
        }
        return;
    }
    if (n < 0 && Q9_SOCK_WOULDBLOCK()) {
        return;
    }
    disconnect_client(vb);
}

static int geometry_changed(const q9_videobridge_t *vb)
{
    int mode = q9_mc6845_mode(vb->crtc);
    int clut_changed = 0;
    /* 5.29-Nachtrag: eine per SS_clut/SS_clutall geaenderte Palette braucht ebenfalls ein neues
       PALETTE-Telegramm -- nur relevant fuer indizierte Modi (dort wird ueberhaupt eine CLUT
       gesendet, s. build_handshake), sonst wuerde ein Gast, der die CLUT-Register in einem RGB-
       Modus anfasst (ungewoehnlich, aber nicht verboten), unnoetige Re-Handshakes ausloesen. */
    if (vb->clut && mode >= Q9_MC6845_MODE_INDEXED1 && mode <= Q9_MC6845_MODE_INDEXED8) {
        clut_changed = (vb->clut->generation != vb->adv_clut_gen);
    }
    return q9_mc6845_stride(vb->crtc) != vb->adv_stride ||
           q9_mc6845_height(vb->crtc) != vb->adv_height ||
           mode                       != vb->adv_mode    ||
           clut_changed;
}

//────────────────────────────────────────────────────────────────────────────────────────────────
// Function: q9_videobridge_poll
//────────────────────────────────────────────────────────────────────────────────────────────────
void q9_videobridge_poll(q9_videobridge_t *vb, uint32_t now_ms)
{
    poll_udp_discovery(vb);
    poll_accept(vb);

    if (vb->client_fd < 0) {
        return;
    }

    if (vb->out_pending) {
        drain_output(vb);
        return;
    }

    if (!vb->handshake_done) {
        if (vb->handshake_have < HELLO_SIZE) {
            try_recv_hello(vb);
            if (vb->client_fd < 0 || vb->handshake_have < HELLO_SIZE) {
                return;                     /* getrennt oder noch nicht komplett */
            }
        }
        build_full_setup(vb);
        vb->last_send_ms = now_ms;
        return;
    }

    if (geometry_changed(vb)) {
        build_full_setup(vb);
        vb->last_send_ms = now_ms;
        return;
    }

    /* 5.27: Rate kommt aus R19 (q9_mc6845_net_update_hz), nicht mehr aus einer festen Host-
       Konstante -- Andreas' Wunsch, den Wert per Register (Treiber/Tuning-Tool 5.31) einstellbar
       zu machen. R19=0 (Reset) liefert Q9_MC6845_NET_HZ_DEFAULT. */
    if (now_ms - vb->last_send_ms < 1000u / q9_mc6845_net_update_hz(vb->crtc)) {
        return;
    }
    vb->last_send_ms = now_ms;

    if (q9_framebuf_dirty_count(vb->fb) == 0) {
        return;
    }
    build_dirty_update(vb);
}
//────────────────────────────────────────────────────────────────────────────────────────────────
// EOF videobridge.c                                                                       Ver. 1.00
//────────────────────────────────────────────────────────────────────────────────────────────────
