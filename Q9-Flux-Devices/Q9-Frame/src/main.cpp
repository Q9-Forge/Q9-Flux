// SDL-Testclient für Q9 Frame: Fenstermodus mit Toolbar (Connect/Trennen/2x/Screenshot).
#include <SDL.h>
#include <SDL_ttf.h>
#include <SDL_image.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <sys/time.h>
#include "net.h"
#include "protocol.h"
#include "framebuffer.h"
#include "udp_scan.h"

const int TOOLBAR_HEIGHT = 32;
const int STATUS_HEIGHT = 22;
const int DEFAULT_W = 640, DEFAULT_H = 480;
// macOS-Systemschriften (kein Fontbundling, daher nicht portabel).
const char* TOOLTIP_FONT_PATH = "/System/Library/Fonts/Supplemental/Arial.ttf";
const char* TITLE_FONT_PATH = "/System/Library/Fonts/Supplemental/Skia.ttf";
// Optionales Startbild statt des programmatischen Q9-Platzhalters. Wenn die
// Datei fehlt, wird einfach weiter das gezeichnete Logo verwendet.
const char* PLACEHOLDER_IMAGE_PATH = "placeholder.png";

enum ConnState { STATE_DISCONNECTED, STATE_CONNECTING, STATE_CONNECTED, STATE_ERROR };
enum ButtonId { BTN_CONNECT, BTN_DISCONNECT, BTN_ZOOM, BTN_SHOT, BTN_COUNT };
const char* BUTTON_LABELS[BTN_COUNT] = {"Connect", "Disconnect", "2x Zoom", "Screenshot"};

static void fill_circle(SDL_Renderer* ren, int cx, int cy, int r) {
    for (int dy = -r; dy <= r; ++dy) {
        int dx = (int)sqrt((double)(r*r - dy*dy));
        SDL_RenderDrawLine(ren, cx-dx, cy+dy, cx+dx, cy+dy);
    }
}

static void fill_triangle_right(SDL_Renderer* ren, int x, int y, int w, int h) {
    for (int i = 0; i < w; ++i) {
        int half = (h/2) * (w - i) / w;
        SDL_RenderDrawLine(ren, x+i, y+h/2-half, x+i, y+h/2+half);
    }
}

// Rendert Text einmalig an Position (x,y). Fuer haeufig wechselnden Text (z.B.
// Statuszeile) bewusst pro Aufruf neu gerendert statt gecacht - einfacher, und
// bei kurzen Strings performant genug.
static void render_text(SDL_Renderer* ren, TTF_Font* font, const char* text, int x, int y, SDL_Color color) {
    if (!font) return;
    SDL_Surface* surf = TTF_RenderUTF8_Blended(font, text, color);
    if (!surf) return;
    SDL_Texture* t = SDL_CreateTextureFromSurface(ren, surf);
    SDL_Rect dst = {x, y, surf->w, surf->h};
    SDL_FreeSurface(surf);
    if (t) { SDL_RenderCopy(ren, t, NULL, &dst); SDL_DestroyTexture(t); }
}

static void setup_buttons(SDL_Rect buttons[]) {
    int x = 6, y = 2, s = 28, gap = 6;
    for (int i = 0; i < BTN_COUNT; ++i) {
        buttons[i] = {x, y, s, s};
        x += s + gap;
    }
}

static void draw_toolbar(SDL_Renderer* ren, int win_w, SDL_Rect buttons[], ConnState state, bool zoom2x, bool server_available) {
    SDL_SetRenderDrawColor(ren, 40, 40, 40, 255);
    SDL_Rect bar = {0, 0, win_w, TOOLBAR_HEIGHT};
    SDL_RenderFillRect(ren, &bar);

    // Connect: gelb waehrend Verbindungsaufbau, gruen wenn Server gefunden,
    // grau wenn keiner da, gedimmt wenn schon verbunden
    if (state == STATE_CONNECTING) {
        SDL_SetRenderDrawColor(ren, 200, 170, 0, 255);
    } else if (state == STATE_CONNECTED) {
        SDL_SetRenderDrawColor(ren, 60, 90, 60, 255);
    } else if (server_available) {
        SDL_SetRenderDrawColor(ren, 0, 150, 0, 255);
    } else {
        SDL_SetRenderDrawColor(ren, 70, 70, 70, 255);
    }
    SDL_RenderFillRect(ren, &buttons[BTN_CONNECT]);
    SDL_SetRenderDrawColor(ren, 255, 255, 255, 255);
    fill_triangle_right(ren, buttons[BTN_CONNECT].x + 8, buttons[BTN_CONNECT].y + 6, 12, 14);

    // Trennen: rot, weißes Stop-Quadrat
    SDL_SetRenderDrawColor(ren, state == STATE_CONNECTED ? 170 : 90, state == STATE_CONNECTED ? 0 : 60, state == STATE_CONNECTED ? 0 : 60, 255);
    SDL_RenderFillRect(ren, &buttons[BTN_DISCONNECT]);
    SDL_SetRenderDrawColor(ren, 255, 255, 255, 255);
    SDL_Rect stop_icon = {buttons[BTN_DISCONNECT].x + 9, buttons[BTN_DISCONNECT].y + 9, 10, 10};
    SDL_RenderFillRect(ren, &stop_icon);

    // 2x: blau (hell wenn aktiv), weißes Plus-Symbol
    SDL_SetRenderDrawColor(ren, zoom2x ? 90 : 55, zoom2x ? 130 : 75, zoom2x ? 230 : 130, 255);
    SDL_RenderFillRect(ren, &buttons[BTN_ZOOM]);
    SDL_SetRenderDrawColor(ren, 255, 255, 255, 255);
    SDL_Rect h_bar = {buttons[BTN_ZOOM].x + 6, buttons[BTN_ZOOM].y + 13, 16, 2};
    SDL_Rect v_bar = {buttons[BTN_ZOOM].x + 13, buttons[BTN_ZOOM].y + 6, 2, 16};
    SDL_RenderFillRect(ren, &h_bar);
    SDL_RenderFillRect(ren, &v_bar);

    // Screenshot: grau, weißer Kamera-Linsenkreis
    SDL_SetRenderDrawColor(ren, 90, 90, 90, 255);
    SDL_RenderFillRect(ren, &buttons[BTN_SHOT]);
    SDL_SetRenderDrawColor(ren, 255, 255, 255, 255);
    fill_circle(ren, buttons[BTN_SHOT].x + 14, buttons[BTN_SHOT].y + 14, 7);
}

// Großes "Q9"-Platzhalterbild, solange nicht verbunden ist.
static void draw_placeholder(SDL_Renderer* ren, SDL_Rect area) {
    SDL_SetRenderDrawColor(ren, 20, 20, 20, 255);
    SDL_RenderFillRect(ren, &area);
    int cx = area.x + area.w / 2;
    int cy = area.y + area.h / 2;
    int r = (area.h < area.w ? area.h : area.w) / 4;
    if (r < 4) return;

    SDL_SetRenderDrawColor(ren, 70, 160, 70, 255);
    int qx = cx - r - r/2;
    fill_circle(ren, qx, cy, r);
    SDL_SetRenderDrawColor(ren, 20, 20, 20, 255);
    fill_circle(ren, qx, cy, r * 2 / 3);
    SDL_SetRenderDrawColor(ren, 70, 160, 70, 255);
    SDL_Rect tail = {qx, cy + r/3, r, r/3};
    SDL_RenderFillRect(ren, &tail);

    int nx = cx + r + r/2;
    fill_circle(ren, nx, cy - r/3, r * 2 / 3);
    SDL_Rect stem = {nx, cy - r/3, r/4 > 0 ? r/4 : 1, r};
    SDL_RenderFillRect(ren, &stem);
}

// Titel-Platzhalter "Q9 Frame" + Untertitel, solange nicht verbunden ist.
static void draw_title_placeholder(SDL_Renderer* ren, SDL_Rect area, TTF_Font* title_font, TTF_Font* subtitle_font) {
    SDL_SetRenderDrawColor(ren, 20, 20, 20, 255);
    SDL_RenderFillRect(ren, &area);

    const char* title = "Q9 Frame";
    const char* subtitle = "Framebuffer for Q9 Flex Emulator";
    const char* version = "Version 0.1 · July 2026";
    int tw = 0, th = 0, sw = 0, sh = 0, vw = 0, vh = 0;
    if (title_font) TTF_SizeUTF8(title_font, title, &tw, &th);
    if (subtitle_font) TTF_SizeUTF8(subtitle_font, subtitle, &sw, &sh);
    if (subtitle_font) TTF_SizeUTF8(subtitle_font, version, &vw, &vh);

    int gap = 20;          // Titel/Untertitel etwas weiter auseinander
    int shift_up = 30;     // Titel/Untertitel-Block etwas hoeher
    int version_gap = 70;  // Versionszeile etwas mehr tiefer, unabhaengig vom Block
    int title_block_h = th + gap + sh;
    int top = area.y + area.h / 2 - title_block_h / 2 - shift_up;

    render_text(ren, title_font, title, area.x + area.w / 2 - tw / 2, top, {70, 160, 70, 255});
    render_text(ren, subtitle_font, subtitle, area.x + area.w / 2 - sw / 2, top + th + gap, {150, 150, 150, 255});
    render_text(ren, subtitle_font, version, area.x + area.w / 2 - vw / 2, top + th + gap + sh + version_gap, {100, 100, 100, 255});
}

static inline uint8_t expand5(uint8_t v5) { return (uint8_t)((v5 << 3) | (v5 >> 2)); }
static inline uint8_t expand6(uint8_t v6) { return (uint8_t)((v6 << 2) | (v6 >> 4)); }
static inline uint8_t brighten(uint8_t v) { return (uint8_t)(v + (255 - v) / 3); } // Intensity-Bit

// Dekodiert ein einzelnes Pixel bei Spalte x einer Zeile gemaess info.mode.
// Indizierte Modi (1-8 bpp) schlagen ueber die CLUT nach, 16/24bpp-Modi direkt.
static void decode_pixel(const uint8_t* row, int x, int mode, const Q9ClutEntry* clut,
                          uint8_t* r, uint8_t* g, uint8_t* b) {
    *r = *g = *b = 0;
    switch (mode) {
        case Q9_VMODE_INDEXED1: {
            int idx = (row[x >> 3] >> (7 - (x & 7))) & 0x1;
            *r = clut[idx].r; *g = clut[idx].g; *b = clut[idx].b;
            break;
        }
        case Q9_VMODE_INDEXED2: {
            int idx = (row[x >> 2] >> (6 - 2 * (x & 3))) & 0x3;
            *r = clut[idx].r; *g = clut[idx].g; *b = clut[idx].b;
            break;
        }
        case Q9_VMODE_INDEXED4: {
            int idx = (row[x >> 1] >> ((x & 1) ? 0 : 4)) & 0xF;
            *r = clut[idx].r; *g = clut[idx].g; *b = clut[idx].b;
            break;
        }
        case Q9_VMODE_INDEXED8: {
            int idx = row[x];
            *r = clut[idx].r; *g = clut[idx].g; *b = clut[idx].b;
            break;
        }
        case Q9_VMODE_RGB565: {
            uint16_t v = (row[x*2] << 8) | row[x*2+1]; // big-endian ueber die Leitung
            *r = expand5((v >> 11) & 0x1F);
            *g = expand6((v >> 5) & 0x3F);
            *b = expand5(v & 0x1F);
            break;
        }
        case Q9_VMODE_RGB555I: {
            uint16_t v = (row[x*2] << 8) | row[x*2+1];
            *r = expand5((v >> 10) & 0x1F);
            *g = expand5((v >> 5) & 0x1F);
            *b = expand5(v & 0x1F);
            if ((v >> 15) & 0x1) { *r = brighten(*r); *g = brighten(*g); *b = brighten(*b); }
            break;
        }
        case Q9_VMODE_RGB888: {
            *r = row[x*3]; *g = row[x*3+1]; *b = row[x*3+2];
            break;
        }
    }
}

// Wandelt den kompletten VRAM-Inhalt in ein RGBA32-Pixelarray (fuer FRAME_FULL).
static void convert_framebuffer(const Q9Framebuffer& fb, const Q9ClutEntry* clut, uint8_t* pixels) {
    const Q9VideoInfo& info = fb.info;
    for (int y = 0; y < info.height; ++y) {
        const uint8_t* row = fb.data + y * info.stride;
        uint8_t* out = pixels + (size_t)y * info.width * 4;
        for (int x = 0; x < info.width; ++x) {
            uint8_t r, g, b;
            decode_pixel(row, x, info.mode, clut, &r, &g, &b);
            out[x*4+0] = r; out[x*4+1] = g; out[x*4+2] = b; out[x*4+3] = 255;
        }
    }
}

// Wandelt nur ein Teilrechteck um (fuer FRAME_UPDATE) - vermeidet, bei jedem
// Dirty-Update den kompletten Frame neu zu konvertieren und hochzuladen.
static void convert_rect(const Q9Framebuffer& fb, const Q9ClutEntry* clut,
                          int rx, int ry, int rw, int rh, uint8_t* out) {
    const Q9VideoInfo& info = fb.info;
    for (int yy = 0; yy < rh; ++yy) {
        const uint8_t* row = fb.data + (ry + yy) * info.stride;
        uint8_t* out_row = out + (size_t)yy * rw * 4;
        for (int xx = 0; xx < rw; ++xx) {
            uint8_t r, g, b;
            decode_pixel(row, rx + xx, info.mode, clut, &r, &g, &b);
            out_row[xx*4+0] = r; out_row[xx*4+1] = g; out_row[xx*4+2] = b; out_row[xx*4+3] = 255;
        }
    }
}

// Bytes pro Pixel-Zeile fuer die gegebene Bit-Tiefe (Ceiling-Division fuer 1/2/4 bpp).
static size_t stride_for_bpp(int width, int bpp) {
    return ((size_t)width * bpp + 7) / 8;
}

static const char* mode_name(int mode) {
    switch (mode) {
        case Q9_VMODE_INDEXED1: return "INDEXED1";
        case Q9_VMODE_INDEXED2: return "INDEXED2";
        case Q9_VMODE_INDEXED4: return "INDEXED4";
        case Q9_VMODE_INDEXED8: return "INDEXED8";
        case Q9_VMODE_RGB565:   return "RGB565";
        case Q9_VMODE_RGB555I:  return "RGB555I";
        case Q9_VMODE_RGB888:   return "RGB888";
        default: return "?";
    }
}

static void take_screenshot(SDL_Renderer* ren, int w, int h) {
    SDL_Surface* surf = SDL_CreateRGBSurfaceWithFormat(0, w, h, 32, SDL_PIXELFORMAT_RGBA32);
    if (!surf) return;
    SDL_RenderReadPixels(ren, NULL, SDL_PIXELFORMAT_RGBA32, surf->pixels, surf->pitch);
    time_t t = time(NULL);
    struct tm tmv;
    localtime_r(&t, &tmv);
    char filename[128];
    strftime(filename, sizeof(filename), "q9frame_screenshot_%Y%m%d_%H%M%S.bmp", &tmv);
    const char* home = getenv("HOME");
    char path[512];
    snprintf(path, sizeof(path), "%s/Desktop/%s", home ? home : ".", filename);
    SDL_SaveBMP(surf, path);
    printf("Screenshot saved: %s\n", path);
    SDL_FreeSurface(surf);
}

// UDP-Scan + Verbindung + Handshake. Blockiert bis zu ~2s (UDP-Scan-Timeout).
// Bei indizierten Modi erwartet der Handshake zusaetzlich eine PALETTE-Nachricht
// zwischen VIDEO_INFO und FRAME_FULL.
static bool try_connect(int* sock_out, Q9VideoInfo* info_out, Q9Framebuffer* fb_out, Q9ClutEntry* clut_out,
                         char* host_out, size_t host_out_size, int* port_out) {
    struct Q9EmuInfo results[8];
    int n = udp_scan(results, 8);
    if (n <= 0) { printf("No emulator found.\n"); return false; }
    printf("Found emulator: %s (%s:%d)\n", results[0].name, results[0].host, results[0].port);
    int sock = tcp_connect(results[0].host, results[0].port);
    if (sock < 0) { printf("Connection failed.\n"); return false; }
    if (send_hello(sock) != 0) { printf("HELLO failed.\n"); close(sock); return false; }

    struct Q9MsgHeader hdr;
    Q9VideoInfo info;
    if (recv_msg_header(sock, &hdr) != 0 || hdr.type != Q9_VIDEO_INFO || recv_video_info(sock, &info) != 0) {
        printf("VIDEO_INFO failed.\n");
        close(sock);
        return false;
    }

    if (q9_mode_is_indexed(info.mode)) {
        struct Q9MsgHeader phdr;
        if (recv_msg_header(sock, &phdr) != 0 || phdr.type != Q9_PALETTE || recv_palette(sock, clut_out) != 0) {
            printf("PALETTE failed.\n");
            close(sock);
            return false;
        }
    }

    Q9Framebuffer fb;
    fb.info = info;
    struct Q9MsgHeader hdr2;
    if (recv_msg_header(sock, &hdr2) != 0 || hdr2.type != Q9_FRAME_FULL || recv_frame_full(sock, &fb) != 0) {
        printf("FRAME_FULL failed.\n");
        close(sock);
        return false;
    }
    printf("Connected to %s:%d, Video: %dx%d, Mode %d\n", results[0].host, results[0].port, info.width, info.height, info.mode);
    *sock_out = sock;
    *info_out = info;
    *fb_out = fb;
    strncpy(host_out, results[0].host, host_out_size - 1);
    host_out[host_out_size - 1] = 0;
    *port_out = results[0].port;
    return true;
}

int main(int argc, char* argv[]) {
    setvbuf(stdout, NULL, _IOLBF, 0);
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        fprintf(stderr, "SDL_Init Error: %s\n", SDL_GetError());
        return 1;
    }
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0"); // Nearest-Neighbor beim Skalieren

    SDL_Window* win = SDL_CreateWindow("Q9 Frame", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                        DEFAULT_W, DEFAULT_H + TOOLBAR_HEIGHT + STATUS_HEIGHT, SDL_WINDOW_SHOWN);
    if (!win) { fprintf(stderr, "SDL_CreateWindow Error: %s\n", SDL_GetError()); SDL_Quit(); return 1; }
    SDL_Renderer* ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!ren) { fprintf(stderr, "SDL_CreateRenderer Error: %s\n", SDL_GetError()); SDL_DestroyWindow(win); SDL_Quit(); return 1; }

    SDL_Rect buttons[BTN_COUNT];
    setup_buttons(buttons);

    // Optionales Startbild statt des gezeichneten Q9-Logos. Fehlt die Datei
    // oder schlaegt das Laden fehl, bleibt placeholder_tex einfach NULL und
    // render_frame() faellt automatisch auf draw_placeholder() zurueck.
    SDL_Texture* placeholder_tex = NULL;
    if (IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG) {
        placeholder_tex = IMG_LoadTexture(ren, PLACEHOLDER_IMAGE_PATH);
        if (!placeholder_tex) {
            printf("No start image found (%s), using built-in title placeholder.\n", PLACEHOLDER_IMAGE_PATH);
        }
    }

    // Font bleibt fuer die gesamte Laufzeit offen: Hover-Tooltips + Button-Labels
    // werden einmalig vorgerendert, die Statuszeile aendert sich aber staendig
    // (Cursor-Position) und wird daher pro Frame neu gerendert.
    TTF_Font* font = NULL;
    TTF_Font* title_font = NULL;    // grosser Font fuer den "Q9 Frame"-Platzhalter-Titel
    TTF_Font* subtitle_font = NULL; // mittlerer Font fuer den Untertitel darunter
    SDL_Texture* label_tex[BTN_COUNT] = {NULL, NULL, NULL, NULL};
    int label_w[BTN_COUNT] = {0}, label_h[BTN_COUNT] = {0};
    if (TTF_Init() == 0) {
        font = TTF_OpenFont(TOOLTIP_FONT_PATH, 13);
        if (font) {
            for (int i = 0; i < BTN_COUNT; ++i) {
                SDL_Surface* surf = TTF_RenderText_Blended(font, BUTTON_LABELS[i], {255, 255, 255, 255});
                if (surf) {
                    label_tex[i] = SDL_CreateTextureFromSurface(ren, surf);
                    label_w[i] = surf->w;
                    label_h[i] = surf->h;
                    SDL_FreeSurface(surf);
                }
            }
        } else {
            fprintf(stderr, "TTF_OpenFont failed (%s): %s\n", TOOLTIP_FONT_PATH, TTF_GetError());
        }
        title_font = TTF_OpenFont(TITLE_FONT_PATH, 64);
        subtitle_font = TTF_OpenFont(TITLE_FONT_PATH, 22);
        if (!title_font) fprintf(stderr, "TTF_OpenFont failed (%s): %s\n", TITLE_FONT_PATH, TTF_GetError());
    }

    ConnState state = STATE_DISCONNECTED;
    int sock = -1;
    Q9VideoInfo info;
    Q9Framebuffer fb;
    fb.data = NULL;
    Q9ClutEntry clut[Q9_CLUT_ENTRIES];
    build_grayscale_clut(clut, 8); // sinnvoller Default/Fallback vor Empfang einer echten Palette
    SDL_Texture* tex = NULL;
    uint8_t* pixels = NULL;
    bool zoom2x = false;
    int video_w = DEFAULT_W, video_h = DEFAULT_H;
    char connected_host[128] = "";
    int connected_port = 0;

    auto apply_window_size = [&]() {
        SDL_SetWindowSize(win, video_w * (zoom2x ? 2 : 1), video_h * (zoom2x ? 2 : 1) + TOOLBAR_HEIGHT + STATUS_HEIGHT);
    };

    auto do_disconnect = [&]() {
        if (sock >= 0) { close(sock); sock = -1; }
        if (tex) { SDL_DestroyTexture(tex); tex = NULL; }
        if (pixels) { free(pixels); pixels = NULL; }
        if (fb.data) { free_framebuffer(&fb); }
        video_w = DEFAULT_W; video_h = DEFAULT_H;
        apply_window_size();
        state = STATE_DISCONNECTED;
    };

    bool server_available = false;
    struct timeval last_probe;
    gettimeofday(&last_probe, NULL);

    auto render_frame = [&]() {
        int ww, wh;
        SDL_GetWindowSize(win, &ww, &wh);
        int mx, my;
        SDL_GetMouseState(&mx, &my);

        SDL_SetRenderDrawColor(ren, 0, 0, 0, 255);
        SDL_RenderClear(ren);
        draw_toolbar(ren, ww, buttons, state, zoom2x, server_available);
        SDL_Rect video_area = {0, TOOLBAR_HEIGHT, ww, wh - TOOLBAR_HEIGHT - STATUS_HEIGHT};
        if (state == STATE_CONNECTED && tex) {
            SDL_RenderCopy(ren, tex, NULL, &video_area);
        } else if (state == STATE_CONNECTING) {
            SDL_SetRenderDrawColor(ren, 220, 180, 40, 255);
            SDL_RenderFillRect(ren, &video_area);
            const char* msg = "Connecting...";
            int tw = 0, th = 0;
            if (font) TTF_SizeText(font, msg, &tw, &th);
            render_text(ren, font, msg, video_area.x + video_area.w/2 - tw/2, video_area.y + video_area.h/2 - th/2, {20, 20, 20, 255});
        } else if (placeholder_tex) {
            SDL_RenderCopy(ren, placeholder_tex, NULL, &video_area);
        } else if (title_font) {
            draw_title_placeholder(ren, video_area, title_font, subtitle_font);
        } else {
            draw_placeholder(ren, video_area);
        }

        // Statuszeile: Verbindung, Modus, Aufloesung, Cursor-Position im Videobereich.
        // Q9-Frame-Gruen, damit sie sich vom meist dunklen Bildschirminhalt abhebt.
        SDL_Rect status_bar = {0, wh - STATUS_HEIGHT, ww, STATUS_HEIGHT};
        SDL_SetRenderDrawColor(ren, 70, 160, 70, 255);
        SDL_RenderFillRect(ren, &status_bar);
        char status_text[192];
        if (state == STATE_CONNECTED) {
            int scale = zoom2x ? 2 : 1;
            int cx = -1, cy = -1;
            if (mx >= video_area.x && mx < video_area.x + video_area.w &&
                my >= video_area.y && my < video_area.y + video_area.h) {
                cx = (mx - video_area.x) / scale;
                cy = (my - video_area.y) / scale;
            }
            if (cx >= 0)
                snprintf(status_text, sizeof(status_text), "Connected: %s:%d   %s  %dx%d   Cursor: %d,%d",
                         connected_host, connected_port, mode_name(info.mode), info.width, info.height, cx, cy);
            else
                snprintf(status_text, sizeof(status_text), "Connected: %s:%d   %s  %dx%d",
                         connected_host, connected_port, mode_name(info.mode), info.width, info.height);
        } else if (state == STATE_CONNECTING) {
            snprintf(status_text, sizeof(status_text), "Connecting...");
        } else if (state == STATE_ERROR) {
            snprintf(status_text, sizeof(status_text), "Connection failed");
        } else {
            snprintf(status_text, sizeof(status_text), "Not connected");
        }
        render_text(ren, font, status_text, status_bar.x + 6, status_bar.y + 3, {20, 20, 20, 255});

        SDL_Point mouse = {mx, my};
        for (int i = 0; i < BTN_COUNT; ++i) {
            if (label_tex[i] && SDL_PointInRect(&mouse, &buttons[i])) {
                int tip_w = label_w[i] + 8, tip_h = label_h[i] + 4;
                int offset = 10;
                int tip_x = mx + offset, tip_y = my - offset - tip_h;
                if (tip_y < 0) tip_y = 0;
                if (tip_x + tip_w > ww) tip_x = ww - tip_w;
                SDL_Rect tip_bg = {tip_x, tip_y, tip_w, tip_h};
                SDL_SetRenderDrawColor(ren, 0, 0, 0, 230);
                SDL_RenderFillRect(ren, &tip_bg);
                SDL_SetRenderDrawColor(ren, 230, 200, 0, 255);
                SDL_RenderDrawRect(ren, &tip_bg);
                SDL_Rect tip_text = {tip_bg.x + 4, tip_bg.y + 2, label_w[i], label_h[i]};
                SDL_RenderCopy(ren, label_tex[i], NULL, &tip_text);
                break;
            }
        }

        SDL_RenderPresent(ren);
    };

    bool running = true;
    SDL_Event e;
    while (running) {
        if (state != STATE_CONNECTED) {
            struct timeval now;
            gettimeofday(&now, NULL);
            double elapsed_ms = (now.tv_sec - last_probe.tv_sec) * 1000.0 + (now.tv_usec - last_probe.tv_usec) / 1000.0;
            if (elapsed_ms >= 2000.0) {
                last_probe = now;
                struct Q9EmuInfo probe[1];
                server_available = udp_scan(probe, 1, 250) > 0; // kurzer Ping statt vollem 2s-Scan
            }
        }
        if (state == STATE_CONNECTED) {
            fd_set fds;
            FD_ZERO(&fds);
            FD_SET(sock, &fds);
            struct timeval tv = {0, 20000}; // 20ms
            int sel = select(sock+1, &fds, 0, 0, &tv);
            if (sel > 0 && FD_ISSET(sock, &fds)) {
                struct Q9MsgHeader uhdr;
                if (recv_msg_header(sock, &uhdr) == 0 && uhdr.type == Q9_FRAME_UPDATE) {
                    Q9DirtyRectHeader rect;
                    size_t got = 0;
                    while (got < sizeof(rect)) {
                        ssize_t rn = read(sock, (uint8_t*)&rect + got, sizeof(rect) - got);
                        if (rn <= 0) break;
                        got += rn;
                    }
                    dirtyrect_ntoh(&rect);
                    size_t rect_stride = stride_for_bpp(rect.w, info.bpp);
                    size_t rect_size = rect_stride * rect.h;
                    uint8_t* rbuf = (uint8_t*)malloc(rect_size);
                    got = 0;
                    while (got < rect_size) {
                        ssize_t rn = read(sock, rbuf + got, rect_size - got);
                        if (rn <= 0) break;
                        got += rn;
                    }
                    size_t x_byte_offset = ((size_t)rect.x * info.bpp) / 8;
                    for (int r = 0; r < rect.h; ++r) {
                        memcpy(fb.data + (rect.y + r) * info.stride + x_byte_offset,
                               rbuf + r * rect_stride, rect_stride);
                    }
                    free(rbuf);
                    uint8_t* rect_pixels = (uint8_t*)malloc((size_t)rect.w * rect.h * 4);
                    convert_rect(fb, clut, rect.x, rect.y, rect.w, rect.h, rect_pixels);
                    SDL_Rect sdl_rect = {rect.x, rect.y, rect.w, rect.h};
                    SDL_UpdateTexture(tex, &sdl_rect, rect_pixels, rect.w * 4);
                    free(rect_pixels);
                } else if (uhdr.type == Q9_VIDEO_INFO) {
                    // Live-Moduswechsel: Server sendet neues VIDEO_INFO (+PALETTE)
                    // + FRAME_FULL mitten in der Verbindung, statt die Verbindung
                    // zu trennen. Komplett rekonfigurieren statt abzubrechen.
                    printf("Video format change detected, reconfiguring...\n");
                    Q9VideoInfo new_info;
                    bool ok = recv_video_info(sock, &new_info) == 0;
                    Q9ClutEntry new_clut[Q9_CLUT_ENTRIES];
                    if (ok && q9_mode_is_indexed(new_info.mode)) {
                        struct Q9MsgHeader phdr;
                        ok = recv_msg_header(sock, &phdr) == 0 && phdr.type == Q9_PALETTE
                             && recv_palette(sock, new_clut) == 0;
                    }
                    Q9Framebuffer new_fb;
                    new_fb.data = NULL;
                    if (ok) {
                        new_fb.info = new_info;
                        struct Q9MsgHeader fhdr2;
                        ok = recv_msg_header(sock, &fhdr2) == 0 && fhdr2.type == Q9_FRAME_FULL
                             && recv_frame_full(sock, &new_fb) == 0;
                    }
                    if (!ok) {
                        printf("Reconfiguration failed.\n");
                        if (new_fb.data) free_framebuffer(&new_fb);
                        do_disconnect();
                        state = STATE_ERROR;
                    } else {
                        free_framebuffer(&fb);
                        fb = new_fb;
                        info = new_info;
                        if (q9_mode_is_indexed(info.mode)) memcpy(clut, new_clut, sizeof(clut));
                        video_w = info.width; video_h = info.height;
                        apply_window_size();
                        SDL_DestroyTexture(tex);
                        free(pixels);
                        tex = SDL_CreateTexture(ren, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STREAMING, info.width, info.height);
                        pixels = (uint8_t*)malloc((size_t)info.width * info.height * 4);
                        convert_framebuffer(fb, clut, pixels);
                        SDL_UpdateTexture(tex, NULL, pixels, info.width * 4);
                        printf("Reconfigured: %dx%d, Mode %d\n", info.width, info.height, info.mode);
                    }
                } else {
                    printf("Connection lost or protocol error.\n");
                    do_disconnect();
                    state = STATE_ERROR;
                }
            }
        } else {
            SDL_Delay(16);
        }

        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) {
                running = false;
            } else if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT) {
                SDL_Point p = {e.button.x, e.button.y};
                if (SDL_PointInRect(&p, &buttons[BTN_CONNECT]) && state != STATE_CONNECTED && state != STATE_CONNECTING) {
                    state = STATE_CONNECTING; // sofortiges optisches Feedback, noch vor dem blockierenden Scan
                    render_frame();
                    Q9VideoInfo new_info;
                    Q9Framebuffer new_fb;
                    Q9ClutEntry new_clut[Q9_CLUT_ENTRIES];
                    memset(new_clut, 0, sizeof(new_clut));
                    int new_sock;
                    char new_host[128];
                    int new_port;
                    if (try_connect(&new_sock, &new_info, &new_fb, new_clut, new_host, sizeof(new_host), &new_port)) {
                        sock = new_sock; info = new_info; fb = new_fb;
                        memcpy(clut, new_clut, sizeof(clut));
                        strncpy(connected_host, new_host, sizeof(connected_host) - 1);
                        connected_port = new_port;
                        video_w = info.width; video_h = info.height;
                        apply_window_size();
                        tex = SDL_CreateTexture(ren, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STREAMING, info.width, info.height);
                        pixels = (uint8_t*)malloc((size_t)info.width * info.height * 4);
                        convert_framebuffer(fb, clut, pixels);
                        SDL_UpdateTexture(tex, NULL, pixels, info.width * 4);
                        state = STATE_CONNECTED;
                    } else {
                        state = STATE_ERROR;
                    }
                } else if (SDL_PointInRect(&p, &buttons[BTN_DISCONNECT]) && state == STATE_CONNECTED) {
                    do_disconnect();
                } else if (SDL_PointInRect(&p, &buttons[BTN_ZOOM])) {
                    zoom2x = !zoom2x;
                    apply_window_size();
                } else if (SDL_PointInRect(&p, &buttons[BTN_SHOT])) {
                    int ww, wh;
                    SDL_GetWindowSize(win, &ww, &wh);
                    take_screenshot(ren, ww, wh);
                }
            }
        }

        render_frame();
    }

    do_disconnect();
    for (int i = 0; i < BTN_COUNT; ++i) {
        if (label_tex[i]) SDL_DestroyTexture(label_tex[i]);
    }
    if (placeholder_tex) SDL_DestroyTexture(placeholder_tex);
    if (font) TTF_CloseFont(font);
    if (title_font) TTF_CloseFont(title_font);
    if (subtitle_font) TTF_CloseFont(subtitle_font);
    TTF_Quit();
    IMG_Quit();
    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(win);
    SDL_Quit();
    return 0;
}
