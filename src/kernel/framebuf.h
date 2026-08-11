//════════════════════════════════════════════════════════════════════════════════════════════════
// File:   framebuf.h                                                                     Ver. 1.00
// Owner:  AF
// Desc.:  5.26: VRAM-Geraet fuer den Board-Runner — Fenster ab Q9_FRAMEBUF_BASE = $FD000000 (Adresse
//         bereits in Q9-Frame WORK_PACKAGES.md WP02 festgelegt), konfigurierbare Groesse (Default
//         1 MByte, max. 16 MByte), plus Dirty-Rechteck-Tracking direkt in den Schreib-Hooks.
//
//         **Speicherhaltung (Q9-Grundsatz "kein Host-malloc", analog q9boardrun.c RAM/ROM):** der
//         eigentliche Puffer wird NICHT von diesem Modul alloziert, sondern von aussen (statisches
//         Array in q9boardrun.c) per q9_framebuf_init() hereingereicht — dieselbe Struktur wie
//         q9_board_init(&b, rom, rom_len, ram, ram_len).
//
//         **Dirty-Tracking:** jeder Schreibzugriff (write8/16/32) markiert NUR seinen eigenen
//         Byte-Bereich, kein Scannen des kompletten VRAM. Die Byte-Adresse wird ueber die aktuelle
//         MC6845-Zeilenlaenge (q9_mc6845_stride, 5.24) in ein (Zeile, Byte-Spalte)-Rechteck
//         uebersetzt (Spalte in BYTES, nicht Pixeln — die Umrechnung Byte->Pixel haengt vom
//         Video-Modus ab und ist bewusst Sache von 5.27/Host-Video-Bridge, nicht dieses Geraets;
//         die Byte-Granularitaet deckt sich exakt mit der im mc6845.h-Kopf dokumentierten
//         Byte/Pixel-Tabelle). Ist noch keine Zeilenlaenge programmiert (Stride=0, z.B. vor dem
//         ersten CRTC-Setup), gilt der komplette Puffer als EINE Zeile (kein Crash/Div-durch-0).
//         Rechtecke werden in einer gedeckelte Liste (Q9_FRAMEBUF_MAX_DIRTY_RECTS) mit Ueberlapp-
//         Merge gesammelt, bei vollgelaufener Liste Fallback auf eine Bounding-Box ueber alle
//         bisherigen Rechtecke — 1:1 dasselbe, bereits gemessene Verfahren (13x Bandbreiten-Gewinn
//         ggue. einer einzelnen Bounding-Box) wie in Q9-Frame `tests/dummy_server.cpp`
//         (add_dirty_rect/rects_overlap/union_into).
//
//         **Nicht Teil von 5.26 (bewusst vertagt):** Board-Config-Anbindung fuer eine vom Default
//         abweichende Groesse (analog dem CF-Weg aus 5.19) — bisher fester Compile-Zeit-Default
//         Q9_FRAMEBUF_DEFAULT_SIZE, s. q9boardrun.c. Bei Bedarf spaeter nachziehen, kein Blocker fuer
//         5.27 (Host-Video-Bridge).
//
// Call:   static uint8_t vram[Q9_FRAMEBUF_MAX_SIZE];
//         q9_framebuf_t fb; q9_framebuf_init(&fb, vram, sizeof(vram), Q9_FRAMEBUF_DEFAULT_SIZE, &crtc);
//         q9_device_t d = {0}; d.type="framebuf"; d.name="vram0"; d.base=Q9_FRAMEBUF_BASE;
//         d.size = q9_framebuf_size(&fb); d.vt = &q9_devtype_framebuf; d.state = &fb;
//         q9_devreg_add(d);
//
// Edition History
//─────────┬──────┬────────────────────────────────────────────────────────────────────────┬──────
// Date    │ Ver. │ Description                                                            │ By
//─────────┼──────┼────────────────────────────────────────────────────────────────────────┼──────
// 26-08-03│ 1.00 │ 5.26: Erster Wurf — Index/Byte-Zugriffe, Dirty-Rect-Merge-Liste          │ Ada
//═════════╧══════╧════════════════════════════════════════════════════════════════════════╧══════
#ifndef Q9_FRAMEBUF_H
#define Q9_FRAMEBUF_H

#include <stdint.h>
#include "devreg.h"                                    /* q9_device_t/Vtable                     */
#include "mc6845.h"                                    /* q9_mc6845_stride() fuers Dirty-Mapping  */

//─── Adressfenster (Q9-Frame WORK_PACKAGES.md WP02) ─────────────────────────────────────────────
#define Q9_FRAMEBUF_BASE           0xFD000000u
#define Q9_FRAMEBUF_DEFAULT_SIZE   (1u * 1024u * 1024u)   /* 1 MByte Default                     */
#define Q9_FRAMEBUF_MAX_SIZE       (16u * 1024u * 1024u)  /* 16 MByte Obergrenze (statisch, s.o.) */

#define Q9_FRAMEBUF_MAX_DIRTY_RECTS 16                    /* wie dummy_server.cpp MAX_DIRTY_RECTS */

//─── Dirty-Rechteck: x0/x1 = Byte-Spalten INNERHALB einer Zeile, y0/y1 = Zeilenindex ────────────
typedef struct {
    int x0, y0, x1, y1;
} q9_fb_dirty_rect_t;

//─── Zustand ─────────────────────────────────────────────────────────────────────────────────────
typedef struct {
    uint8_t            *vram;        /* extern bereitgestellter Speicher (kein malloc, s.o.)      */
    uint32_t             size;        /* konfigurierte Fenstergroesse, <= Q9_FRAMEBUF_MAX_SIZE     */
    const q9_mc6845_t   *crtc;        /* fuer Stride beim Dirty-Mapping (darf NULL sein)           */
    q9_fb_dirty_rect_t   dirty[Q9_FRAMEBUF_MAX_DIRTY_RECTS];
    int                   dirty_count;
} q9_framebuf_t;

//────────────────────────────────────────────────────────────────────────────────────────────────
// Function: q9_framebuf_init
// Desc.:    vram/vram_cap = extern bereitgestellter Puffer + dessen Kapazitaet (statisches Array,
//           s. Dateikopf). size = tatsaechlich genutzte/adressierte Groesse (<= vram_cap). crtc
//           (darf NULL sein) liefert die Zeilenlaenge fuers Dirty-Mapping. Nullt die ersten `size`
//           Byte (Reset-Zustand). Rueckgabe: 0 = ok, -1 = ungueltige Parameter (size 0 oder > cap).
//────────────────────────────────────────────────────────────────────────────────────────────────
int q9_framebuf_init(q9_framebuf_t *fb, uint8_t *vram, uint32_t vram_cap, uint32_t size,
                      const q9_mc6845_t *crtc);

//────────────────────────────────────────────────────────────────────────────────────────────────
// Function: q9_framebuf_vram / q9_framebuf_size
// Desc.:    Direktzugriff fuer 5.27 (Host-Video-Bridge): Rohpuffer + konfigurierte Groesse.
//────────────────────────────────────────────────────────────────────────────────────────────────
uint8_t *q9_framebuf_vram(q9_framebuf_t *fb);
uint32_t q9_framebuf_size(const q9_framebuf_t *fb);

//────────────────────────────────────────────────────────────────────────────────────────────────
// Function: q9_framebuf_dirty_mark
// Desc.:    Markiert [offset, offset+len) als geaendert (s. Dirty-Tracking im Dateikopf). Wird von
//           den write8/16/32-Hooks der Vtable aufgerufen -- fuer eigene Schreibpfade (z.B. ein
//           spaeterer Blit-Fast-Path) auch direkt nutzbar.
//────────────────────────────────────────────────────────────────────────────────────────────────
void q9_framebuf_dirty_mark(q9_framebuf_t *fb, uint32_t offset, uint32_t len);

//────────────────────────────────────────────────────────────────────────────────────────────────
// Function: q9_framebuf_dirty_count / q9_framebuf_dirty_rect / q9_framebuf_dirty_clear
// Desc.:    Fuer 5.27: aktuelle Dirty-Liste auslesen und nach dem Versand zuruecksetzen.
//────────────────────────────────────────────────────────────────────────────────────────────────
int                 q9_framebuf_dirty_count(const q9_framebuf_t *fb);
q9_fb_dirty_rect_t  q9_framebuf_dirty_rect (const q9_framebuf_t *fb, int index);
void                q9_framebuf_dirty_clear(q9_framebuf_t *fb);

//─── Geraete-Vtable fuer die Registry (devreg.h) ────────────────────────────────────────────────
extern const q9_device_vtable_t q9_devtype_framebuf;

#endif /* Q9_FRAMEBUF_H */
//────────────────────────────────────────────────────────────────────────────────────────────────
// EOF framebuf.h                                                                          Ver. 1.00
//────────────────────────────────────────────────────────────────────────────────────────────────
