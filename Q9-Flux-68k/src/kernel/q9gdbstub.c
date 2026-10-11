#include "q9gdbstub.h"
#include "m68krt.h"
#include "q9boardrun.h"
#include "q9_sockcompat.h"
#include "m68k.h"
#include <ctype.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define RSP_PACKET_MAX 4096
#define RSP_REPLY_MAX 8192

static int g_listen_fd = -1;
static int g_client_fd = -1;
static int g_no_ack;
static int g_stop_sent;
static int g_active;
static char g_packet[RSP_PACKET_MAX];
static size_t g_packet_len;
static int g_in_packet;
static int g_checksum_left;
static unsigned g_checksum;
static unsigned g_checksum_rx;

static int hexval(char c)
{
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

static int parse_u32(const char *s, const char **end, uint32_t *out)
{
    uint32_t v = 0;
    int digits = 0, h;
    while ((h = hexval(*s)) >= 0) {
        if (v > (UINT32_MAX - (uint32_t)h) / 16u) return -1;
        v = v * 16u + (uint32_t)h;
        s++; digits++;
    }
    if (!digits) return -1;
    *out = v;
    if (end) *end = s;
    return 0;
}

static void append_hex_byte(char *out, size_t *n, unsigned value)
{
    static const char digits[] = "0123456789abcdef";
    out[(*n)++] = digits[(value >> 4) & 15u];
    out[(*n)++] = digits[value & 15u];
}

static void append_hex_u32(char *out, size_t *n, uint32_t value)
{
    int shift;
    for (shift = 24; shift >= 0; shift -= 8)
        append_hex_byte(out, n, (value >> shift) & 255u);
}

static int decode_hex_bytes(const char *s, size_t chars, unsigned char *out)
{
    size_t i;
    if (chars & 1u) return -1;
    for (i = 0; i < chars / 2u; i++) {
        int hi = hexval(s[i * 2u]), lo = hexval(s[i * 2u + 1u]);
        if (hi < 0 || lo < 0) return -1;
        out[i] = (unsigned char)((hi << 4) | lo);
    }
    return (int)(chars / 2u);
}

static int send_all(const char *data, size_t len)
{
    size_t sent = 0;
    while (sent < len) {
        int n = (int)send(g_client_fd, data + sent, (int)(len - sent), 0);
        if (n <= 0) return -1;
        sent += (size_t)n;
    }
    return 0;
}

static int send_reply(const char *payload, size_t len)
{
    char frame[RSP_REPLY_MAX];
    size_t i, at = 0;
    unsigned sum = 0;
    static const char digits[] = "0123456789abcdef";
    if (g_client_fd < 0 || len + 4u > sizeof(frame)) return -1;
    frame[at++] = '$';
    for (i = 0; i < len; i++) { frame[at++] = payload[i]; sum += (unsigned char)payload[i]; }
    frame[at++] = '#'; frame[at++] = digits[(sum >> 4) & 15u]; frame[at++] = digits[sum & 15u];
    return send_all(frame, at);
}

static int send_text(const char *text)
{
    return send_reply(text, strlen(text));
}

static void make_register_packet(char *out, size_t *len)
{
    unsigned i;
    *len = 0;
    for (i = 0; i < 18u; i++) append_hex_u32(out, len, q9_m68krt_debug_reg_get(i));
}

static void reply_memory(const char *arg)
{
    const char *end;
    uint32_t address, length;
    char reply[RSP_REPLY_MAX];
    size_t n = 0, i;
    if (parse_u32(arg, &end, &address) || *end != ',' ||
        parse_u32(end + 1, &end, &length) || *end || length > 2048u ||
        address > UINT32_MAX - length) { send_text("E01"); return; }
    for (i = 0; i < length; i++) append_hex_byte(reply, &n, q9_m68krt_debug_mem_read8(address + (uint32_t)i));
    send_reply(reply, n);
}

static void write_memory(const char *arg)
{
    const char *end;
    uint32_t address, length;
    unsigned char bytes[2048];
    int count;
    if (parse_u32(arg, &end, &address) || *end != ',' ||
        parse_u32(end + 1, &end, &length) || *end != ':' || length > sizeof(bytes) ||
        address > UINT32_MAX - length) { send_text("E01"); return; }
    count = decode_hex_bytes(end + 1, strlen(end + 1), bytes);
    if (count < 0 || (uint32_t)count != length) { send_text("E01"); return; }
    { uint32_t i; for (i = 0; i < length; i++) q9_m68krt_debug_mem_write8(address + i, bytes[i]); }
    send_text("OK");
}

static void handle_packet(const char *p)
{
    char reply[RSP_REPLY_MAX];
    size_t n;
    uint32_t address;
    const char *end;
    if (g_client_fd < 0) return;
    if (!g_no_ack) (void)send_all("+", 1u);
    if (p[0] == '?') { send_text("T05thread:1;"); return; }
    if (p[0] == 'g') { make_register_packet(reply, &n); send_reply(reply, n); return; }
    if (p[0] == 'G') {
        unsigned char bytes[72]; unsigned i;
        if (decode_hex_bytes(p + 1, strlen(p + 1), bytes) != 72) { send_text("E01"); return; }
        for (i = 0; i < 18u; i++) {
            uint32_t v = ((uint32_t)bytes[i*4] << 24) | ((uint32_t)bytes[i*4+1] << 16) |
                         ((uint32_t)bytes[i*4+2] << 8) | bytes[i*4+3];
            q9_m68krt_debug_reg_set(i, v);
        }
        send_text("OK"); return;
    }
    if (p[0] == 'm') { reply_memory(p + 1); return; }
    if (p[0] == 'M') { write_memory(p + 1); return; }
    if ((p[0] == 'Z' || p[0] == 'z') && p[1] == '0' && p[2] == ',') {
        if (parse_u32(p + 3, &end, &address) || *end != ',') { send_text("E01"); return; }
        if (p[0] == 'Z') send_text(q9_m68krt_breakpoint_add(address) < 0 ? "E03" : "OK");
        else send_text(q9_m68krt_breakpoint_remove(address) ? "OK" : "E01");
        return;
    }
    if (p[0] == 'c' || p[0] == 's') {
        if (p[1]) {
            if (parse_u32(p + 1, &end, &address) || *end) { send_text("E01"); return; }
            q9_m68krt_debug_reg_set(17u, address);
        }
        g_stop_sent = 0;
        if (p[0] == 'c') q9_dbg_resume_requested = 1;
        else q9_dbg_step_requested = 1;
        return;
    }
    if (p[0] == 3) {
        q9_dbg_capture_current_snapshot();
        q9_dbg_break_requested = 1;
        g_stop_sent = 0;
        return;
    }
    if (!strncmp(p, "qSupported", 10)) { send_text("PacketSize=1000;swbreak+;QStartNoAckMode+;qXfer:features:read+"); return; }
    if (!strncmp(p, "qXfer:features:read:target.xml:", 31)) {
        static const char xml[] = "<?xml version=\"1.0\"?><target version=\"1.0\"><architecture>m68k</architecture><feature name=\"org.gnu.gdb.m68k.core\"><reg name=\"d0\" bitsize=\"32\" regnum=\"0\"/><reg name=\"d1\" bitsize=\"32\" regnum=\"1\"/><reg name=\"d2\" bitsize=\"32\" regnum=\"2\"/><reg name=\"d3\" bitsize=\"32\" regnum=\"3\"/><reg name=\"d4\" bitsize=\"32\" regnum=\"4\"/><reg name=\"d5\" bitsize=\"32\" regnum=\"5\"/><reg name=\"d6\" bitsize=\"32\" regnum=\"6\"/><reg name=\"d7\" bitsize=\"32\" regnum=\"7\"/><reg name=\"a0\" bitsize=\"32\" regnum=\"8\"/><reg name=\"a1\" bitsize=\"32\" regnum=\"9\"/><reg name=\"a2\" bitsize=\"32\" regnum=\"10\"/><reg name=\"a3\" bitsize=\"32\" regnum=\"11\"/><reg name=\"a4\" bitsize=\"32\" regnum=\"12\"/><reg name=\"a5\" bitsize=\"32\" regnum=\"13\"/><reg name=\"a6\" bitsize=\"32\" regnum=\"14\"/><reg name=\"a7\" bitsize=\"32\" regnum=\"15\"/><reg name=\"sr\" bitsize=\"32\" regnum=\"16\"/><reg name=\"pc\" bitsize=\"32\" type=\"code_ptr\" regnum=\"17\"/></feature></target>";
        const char *arg = p + 31, *end;
        uint32_t offset, want;
        if (parse_u32(arg, &end, &offset) || *end != ',' || parse_u32(end + 1, &end, &want) || *end) { send_text("E01"); return; }
        if (offset > sizeof(xml) - 1u) { send_text("l"); return; }
        n = sizeof(xml) - 1u - offset;
        if (n > want) n = want;
        reply[0] = (offset + n >= sizeof(xml) - 1u) ? 'l' : 'm';
        memcpy(reply + 1, xml + offset, n);
        send_reply(reply, n + 1u);
        return;
    }
    if (!strncmp(p, "vCont?", 6)) { send_text("vCont;c;s"); return; }
    if (!strncmp(p, "vCont;", 6)) {
        if (strchr(p + 6, 's')) { g_stop_sent = 0; q9_dbg_step_requested = 1; }
        else if (strchr(p + 6, 'c')) { g_stop_sent = 0; q9_dbg_resume_requested = 1; }
        else send_text("");
        return;
    }
    if (!strncmp(p, "qAttached", 9)) { send_text("1"); return; }
    if (!strncmp(p, "qC", 2)) { send_text("QC1"); return; }
    if (!strncmp(p, "qfThreadInfo", 12)) { send_text("m1"); return; }
    if (!strncmp(p, "qsThreadInfo", 12)) { send_text("l"); return; }
    if (p[0] == 'H' || p[0] == 'T') { send_text("OK"); return; }
    if (p[0] == '!') { send_text("OK"); return; }
    if (!strncmp(p, "QStartNoAckMode", 15)) { send_text("OK"); g_no_ack = 1; return; }
    if (p[0] == 'D') {
        send_text("OK"); Q9_SOCK_CLOSE(g_client_fd); g_client_fd = -1; g_no_ack = 0; g_stop_sent = 0;
        q9_dbg_resume_requested = 1; return;
    }
    /* RSP requires an empty reply for unsupported optional packets. */
    send_text("");
}

static void accept_client(void)
{
    struct sockaddr_in peer;
    socklen_t len = (socklen_t)sizeof(peer);
    int fd = (int)accept(g_listen_fd, (struct sockaddr *)&peer, &len);
    if (fd < 0) return;
    if (g_client_fd >= 0) Q9_SOCK_CLOSE(g_client_fd);
    g_client_fd = fd;
    q9_dbg_capture_current_snapshot();
    q9_dbg_break_requested = 1;
    g_no_ack = 0; g_stop_sent = 1; g_packet_len = 0; g_in_packet = 0;
    Q9_SOCK_IGNORE_SIGPIPE();
    fprintf(stderr, "[Q9 GDB] GDB-Client verbunden\n");
}

int q9_gdbstub_init(void)
{
    const char *env = getenv("Q9_GDB_PORT");
    char *end;
    unsigned long port;
    struct sockaddr_in addr;
    int one = 1;
    if (!env || !*env) return 0;
    port = strtoul(env, &end, 10);
    if (*end || port == 0 || port > 65535u) { fprintf(stderr, "[Q9 GDB] Ungueltiger Q9_GDB_PORT\n"); return -1; }
    if (q9_sock_startup() != 0) return -1;
    g_listen_fd = (int)socket(AF_INET, SOCK_STREAM, 0);
    if (g_listen_fd < 0) { q9_sock_perror("[Q9 GDB] socket"); return -1; }
    (void)setsockopt(g_listen_fd, SOL_SOCKET, SO_REUSEADDR, (const char *)&one, sizeof(one));
    memset(&addr, 0, sizeof(addr)); addr.sin_family = AF_INET; addr.sin_port = htons((unsigned short)port);
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    if (bind(g_listen_fd, (struct sockaddr *)&addr, sizeof(addr)) != 0 || listen(g_listen_fd, 1) != 0) {
        q9_sock_perror("[Q9 GDB] bind/listen"); Q9_SOCK_CLOSE(g_listen_fd); g_listen_fd = -1; return -1;
    }
    Q9_SOCK_NONBLOCK(g_listen_fd);
    g_active = 1;
    fprintf(stderr, "[Q9 GDB] RSP lauscht auf 127.0.0.1:%lu\n", port);
    return 0;
}

int q9_gdbstub_active(void) { return g_active; }
int q9_gdbstub_connected(void) { return g_client_fd >= 0; }

void q9_gdbstub_poll(int target_stopped)
{
    q9_pollfd_t pfd;
    char buf[512];
    int ready, n;
    if (!g_active) return;
    if (g_client_fd < 0) {
        pfd.fd = g_listen_fd; pfd.events = POLLIN; pfd.revents = 0;
        ready = Q9_SOCK_POLL(&pfd, 1, 0);
        if (ready > 0 && (pfd.revents & POLLIN)) accept_client();
        if (g_client_fd < 0) return;
    }
    if (target_stopped && !g_stop_sent) { (void)send_text("T05thread:1;"); g_stop_sent = 1; }
    pfd.fd = g_client_fd; pfd.events = POLLIN; pfd.revents = 0;
    ready = Q9_SOCK_POLL(&pfd, 1, 0);
    if (ready <= 0) return;
    if (pfd.revents & (POLLERR | POLLHUP | POLLNVAL)) {
        Q9_SOCK_CLOSE(g_client_fd); g_client_fd = -1; g_no_ack = 0; return;
    }
    if (!(pfd.revents & POLLIN)) return;
    n = (int)recv(g_client_fd, buf, sizeof(buf), 0);
    if (n <= 0) { Q9_SOCK_CLOSE(g_client_fd); g_client_fd = -1; g_no_ack = 0; return; }
    for (int i = 0; i < n && g_client_fd >= 0; i++) {
        unsigned char c = (unsigned char)buf[i];
        if (c == 3u && !g_in_packet) { handle_packet("\003"); continue; }
        if (c == '$') { g_in_packet = 1; g_packet_len = 0; g_checksum = 0; g_checksum_rx = 0; g_checksum_left = 0; continue; }
        if (!g_in_packet) continue;
        if (c == '#') { g_checksum_left = 2; continue; }
        if (g_checksum_left > 0) {
            int h = hexval((char)c);
            if (h < 0) { g_in_packet = 0; continue; }
            g_checksum_rx = (g_checksum_rx << 4) | (unsigned)h;
            if (--g_checksum_left == 0) {
                g_in_packet = 0;
                if (g_checksum_rx == (g_checksum & 255u)) {
                    g_packet[g_packet_len] = 0; handle_packet(g_packet);
                } else if (!g_no_ack) (void)send_all("-", 1u);
            }
            continue;
        }
        if (g_packet_len + 1u < sizeof(g_packet)) { g_packet[g_packet_len++] = (char)c; g_checksum = (g_checksum + c) & 255u; }
        else g_in_packet = 0;
    }
}

void q9_gdbstub_close(void)
{
    if (g_client_fd >= 0) Q9_SOCK_CLOSE(g_client_fd);
    if (g_listen_fd >= 0) Q9_SOCK_CLOSE(g_listen_fd);
    g_client_fd = g_listen_fd = -1; g_active = 0;
    q9_sock_cleanup();
}
