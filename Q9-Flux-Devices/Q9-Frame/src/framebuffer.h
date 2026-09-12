#ifndef Q9FRAME_FRAMEBUFFER_H
#define Q9FRAME_FRAMEBUFFER_H
#include <stdint.h>

// 1-8 bpp: indiziert, CLUT-Groesse = 2^bpp Eintraege (Palette bestimmt Farben,
//          z.B. Graustufen bei INDEXED8 ist nur eine bestimmte Palettenbelegung,
//          kein eigener Modus).
// 16 bpp:  direktes RGB, zwei Varianten weil 16 Bit sich nicht glatt auf 3 Kanaele
//          aufteilen laesst - RGB565 (kein Bit uebrig) vs. RGB555 + globales
//          Intensity-Bit (Amiga-Stil, hellt alle drei Kanaele gemeinsam auf).
// 24 bpp:  direktes RGB888, 8-8-8, keine sinnvollen Varianten (kein Bit uebrig).
enum Q9VideoMode {
    Q9_VMODE_INDEXED1  = 0, // 1 bpp,  2 CLUT-Eintraege
    Q9_VMODE_INDEXED2  = 1, // 2 bpp,  4 CLUT-Eintraege
    Q9_VMODE_INDEXED4  = 2, // 4 bpp, 16 CLUT-Eintraege
    Q9_VMODE_INDEXED8  = 3, // 8 bpp, 256 CLUT-Eintraege
    Q9_VMODE_RGB565    = 4, // 16 bpp, direkt, 5-6-5
    Q9_VMODE_RGB555I   = 5, // 16 bpp, direkt, 5-5-5 + Intensity-Bit
    Q9_VMODE_RGB888    = 6, // 24 bpp, direkt, 8-8-8
};

struct Q9VideoInfo {
    uint16_t width;
    uint16_t height;
    uint16_t stride;
    uint8_t bpp;
    uint8_t mode;       // Q9VideoMode
    uint8_t reserved[2];
};

struct Q9Framebuffer {
    Q9VideoInfo info;
    uint8_t* data;
};

// Eine CLUT fuer alle indizierten Modi (1-8 bpp). Niedrigere bpp-Modi nutzen
// nur die ersten 2^bpp Eintraege, der Rest bleibt ungenutzt.
#define Q9_CLUT_ENTRIES 256
struct Q9ClutEntry {
    uint8_t r, g, b;
};

int recv_video_info(int sock, Q9VideoInfo* info);
int recv_frame_full(int sock, Q9Framebuffer* fb);
int recv_palette(int sock, Q9ClutEntry* clut);
void free_framebuffer(Q9Framebuffer* fb);

static inline bool q9_mode_is_indexed(int mode) {
    return mode == Q9_VMODE_INDEXED1 || mode == Q9_VMODE_INDEXED2 ||
           mode == Q9_VMODE_INDEXED4 || mode == Q9_VMODE_INDEXED8;
}

// Fuellt die ersten 2^bpp Eintraege mit einer Graustufenrampe (0=schwarz..weiss).
// Sinnvoller Default/Fallback fuer indizierte Modi ohne spezifischere Palette.
void build_grayscale_clut(Q9ClutEntry* clut, int bpp);

#endif // Q9FRAME_FRAMEBUFFER_H
