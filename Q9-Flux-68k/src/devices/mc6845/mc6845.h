//════════════════════════════════════════════════════════════════════════════════════════════════
// File:   mc6845.h                                                                        Ver. 1.03
// Owner:  AF
// Desc.:  5.24: MC6845-CRT-Controller-Emulation fuer den Board-Runner — liefert die Register-
//         Grundlage fuer den Framebuffer (5.26) und die Host-Video-Bridge zu Q9 Frame (5.27).
//
//         Adressmodell (Planungsrunde Andreas + Ada, 2026-08-03; Adresse bereits in Q9-Frame
//         WORK_PACKAGES.md WP03 festgelegt): 2-Byte-Fenster ab Q9_MC6845_BASE = $FFFFA000, klassisches
//         Index/Daten-Registerpaar wie beim echten Chip (+0 Adressregister: waehlt R0-R19 aus,
//         +1 Datenregister: liest/schreibt das gewaehlte Register). Bewusste Vereinfachung ggue.
//         echter Hardware: ALLE Register sind frei lesbar UND schreibbar (der echte MC6845 kennt
//         teils schreibgeschuetzte/nur-lesbare Register je nach R/W-Pin-Verdrahtung des Systems) —
//         analog zur RTC72421-Vereinfachung (s. q9board.h), hier aber dokumentiert als "alles R/W".
//
//         Register R0-R15 = Standard-MC6845 (Horizontal Total/Displayed/SyncPos/SyncWidth,
//         Vertical Total/TotalAdjust/Displayed/SyncPos, Interlace/Skew, MaxScanLine, Cursor
//         Start/End, Start-Adresse High/Low, Cursor-Position High/Low) — Bedeutung/Reihenfolge
//         exakt wie im Datenblatt, s. Q9-Frame WORK_PACKAGES.md WP03.
//
//         **R16/R17 = VideoClk High/Low, R18 = Video-Modus (KEINE echten 6845-Register, eigene
//         Ergaenzung, Planungsrunde 2026-08-03):** der reale MC6845 kennt weder eine Bittiefe (reiner
//         Adress-/Timing-Generator) noch einen Takt (der kommt extern vom Video-Oszillator/PLL). Ohne
//         Video-Modus waere nicht bekannt, ob VRAM gerade INDEXED1/2/4/8 oder RGB565/555I/888 enthaelt
//         (Werte identisch zu Q9VideoMode aus Q9-Frame framebuffer.h: 0-6, s.u.). VideoClk ersetzt eine
//         vollstaendige Si5351A-Simulation (bewusst vereinfacht: kein I2C-Bus, kein PLL-Multiplikator/
//         Divider-Modell -- R16/R17 halten direkt die fertige Pixel-Taktfrequenz in kHz als 16-Bit-Wert,
//         High:Low). **Reihenfolge VideoClk VOR Mode (Andreas, 2026-08-03):** VideoClk als
//         zusammenhaengendes Register-PAAR beginnt auf einem GERADEN Index (R16), Mode steht als
//         Einzelregister danach ebenfalls auf einem geraden Index (R18) -- durchgaengig gerade
//         Startadressen fuer alle Register/Registerpaare.
//
//         **Vereinfachte Geometrie-Interpretation (Ada, 2026-08-03, ANDREAS BITTE PRUEFEN):** der
//         echte MC6845 ist zeichenorientiert (R1/R6 zaehlen Zeichen, tatsaechliche Pixelbreite
//         ergibt sich erst zusammen mit der Zeichenzellenbreite; R9 = Scanlines pro Zeichenzelle
//         fuer Textmodus-Mehrfachzeilen). Da wir den 6845 hier ausschliesslich fuer einen
//         Bitmap-Framebuffer nutzen (kein Zeichensatz/Textmodus-Nachbau auf dieser Ebene), wird
//         R1 direkt als **Bytes pro Bildzeile (Stride)** interpretiert (nicht als Zeichenanzahl)
//         und R6 direkt als **Bildhoehe in Pixeln** (R9/MaxScanLine wird NICHT zur Vervielfachung
//         herangezogen) — die eigentliche Pixelbreite ergibt sich aus Stride (R1) + Video-Modus
//         (R18) nach derselben Formel wie in Q9-Ink/Q9-Frame (stride_for_bpp). Das ist eine bewusste
//         Abweichung von echter 6845-Texttreue zugunsten eines einfachen Grafikmodells — falls
//         spaeter doch Text-/Zeichenmodus-Kompatibilitaet gebraucht wird, muesste das nachgezogen
//         werden.
//
//         **Bestaetigt echtes 6845-Verhalten (Andreas, 2026-08-03):** R0-R3 zaehlen beim echten Chip
//         in Zeichen-Takt-Einheiten, nicht Pixel-Takt-Einheiten — die Granularitaet von R1 ist damit
//         IMMER "ein Byte-Fetch", aber wie viele PIXEL das sind, haengt vom Video-Modus (R18) ab:
//           1bpp:  1 Byte = 8 Pixel  -> R1-Schritte = 8-Pixel-Schritte
//           2bpp:  1 Byte = 4 Pixel  -> 4-Pixel-Schritte
//           4bpp:  1 Byte = 2 Pixel  -> 2-Pixel-Schritte
//           8bpp:  1 Byte = 1 Pixel  -> jeder Wert erlaubt, keine Einschraenkung
//           16bpp (RGB565/RGB555I): 1 Pixel = 2 Byte -> R1 sollte ein Vielfaches von 2 sein
//           24bpp (RGB888):         1 Pixel = 3 Byte -> R1 sollte ein Vielfaches von 3 sein
//         Der Treiber/das Tuning-Tool (5.31) muss bei 16/24bpp selbst auf diese Vielfachen achten,
//         der Emulator erzwingt das nicht (analog zur "alles R/W"-Vereinfachung oben).
//
// Call:   q9_mc6845_t crtc; q9_mc6845_init(&crtc);
//         q9_device_t d = {0}; d.type="mc6845"; d.name="crtc0"; d.base=Q9_MC6845_BASE;
//         d.size = Q9_MC6845_TOP - Q9_MC6845_BASE + 1u; d.vt = &q9_devtype_mc6845; d.state = &crtc;
//         q9_devreg_add(d);
//
// Edition History
//─────────┬──────┬────────────────────────────────────────────────────────────────────────┬──────
// Date    │ Ver. │ Description                                                            │ By
//─────────┼──────┼────────────────────────────────────────────────────────────────────────┼──────
// 26-08-03│ 1.00 │ 5.24: Erster Wurf — Index/Daten-Register, R0-R16, Info-Struct           │ Ada
// 26-08-03│ 1.01 │ 5.25: VideoClk R16/17 (Si5351A-Ersatz, kHz), Mode auf R18 verschoben     │ Ada
// 26-08-03│ 1.02 │ 5.27: q9_mc6845_bpp/width_px fuer die Host-Video-Bridge                 │ Ada
// 26-08-03│ 1.03 │ 5.27: R19 Netz-Update-Rate (Hz), auf Wunsch Andreas als Register statt   │ Ada
//         │      │ Host-Konstante                                                          │
//═════════╧══════╧════════════════════════════════════════════════════════════════════════╧══════
#ifndef Q9_MC6845_H
#define Q9_MC6845_H

#include <stdint.h>
#include "../../kernel/devreg.h"                        /* q9_device_t/Vtable                     */
#include "../../kernel/devdesc.h"                       /* 2026-08-21: q9_devdesc_mc6845, s.u.     */

//─── Adressfenster ────────────────────────────────────────────────────────────────────────────────
#define Q9_MC6845_BASE       0xFFFFA000u               /* Q9-Frame WORK_PACKAGES.md WP03          */
#define Q9_MC6845_TOP        0xFFFFA001u               /* 2 Byte: Adress-/Datenregister            */

#define Q9_MC6845_NUM_REGS   20                        /* R0-R15 (Standard) + R16/17 (VideoClk)   */
                                                        /* + R18 (Video-Modus) + R19 (Netz-Update) */

//─── Standard-Registerindizes (R0-R15, Reihenfolge = echter MC6845) ─────────────────────────────
#define Q9_MC6845_R_HTOTAL      0   /* Horizontal Total                                          */
#define Q9_MC6845_R_HDISP       1   /* Horizontal Displayed -- hier: Bytes/Zeile (Stride), s.o.   */
#define Q9_MC6845_R_HSYNCPOS    2   /* Horizontal Sync Position                                  */
#define Q9_MC6845_R_SYNCWIDTH   3   /* Sync-Breite (H low nibble / V high nibble)                */
#define Q9_MC6845_R_VTOTAL      4   /* Vertical Total                                            */
#define Q9_MC6845_R_VTOTALADJ   5   /* Vertical Total Adjust                                     */
#define Q9_MC6845_R_VDISP       6   /* Vertical Displayed -- hier: Hoehe in Pixeln, s.o.          */
#define Q9_MC6845_R_VSYNCPOS    7   /* Vertical Sync Position                                    */
#define Q9_MC6845_R_INTERLACE   8   /* Interlace-Modus/Skew                                      */
#define Q9_MC6845_R_MAXSCAN     9   /* Max Scan Line (hier nicht zur Geometrie herangezogen)      */
#define Q9_MC6845_R_CURSTART   10   /* Cursor Start (+ Blink-Modus-Bits)                         */
#define Q9_MC6845_R_CUREND     11   /* Cursor End                                                */
#define Q9_MC6845_R_STARTADRH  12   /* Start-Adresse High                                        */
#define Q9_MC6845_R_STARTADRL  13   /* Start-Adresse Low                                         */
#define Q9_MC6845_R_CURPOSH    14   /* Cursor-Position High                                      */
#define Q9_MC6845_R_CURPOSL    15   /* Cursor-Position Low                                       */
#define Q9_MC6845_R_VIDEOCLKH  16   /* Eigene Ergaenzung: VideoClk High-Byte (kHz), s. Dateikopf  */
#define Q9_MC6845_R_VIDEOCLKL  17   /* Eigene Ergaenzung: VideoClk Low-Byte (kHz)                 */
#define Q9_MC6845_R_VIDEOMODE  18   /* Eigene Ergaenzung: Q9VideoMode 0-6, s. Dateikopf           */
#define Q9_MC6845_R_NETHZ      19   /* Eigene Ergaenzung: Netz-Update-Rate in Hz, 0=Default, s.u. */

//─── Video-Modus-Werte (identisch zu Q9VideoMode, Q9-Frame src/framebuffer.h) ───────────────────
#define Q9_MC6845_MODE_INDEXED1  0
#define Q9_MC6845_MODE_INDEXED2  1
#define Q9_MC6845_MODE_INDEXED4  2
#define Q9_MC6845_MODE_INDEXED8  3
#define Q9_MC6845_MODE_RGB565    4
#define Q9_MC6845_MODE_RGB555I   5
#define Q9_MC6845_MODE_RGB888    6

//─── Zustand ─────────────────────────────────────────────────────────────────────────────────────
typedef struct {
    uint8_t  reg[Q9_MC6845_NUM_REGS];   /* R0-R19, s.o.                                          */
    uint8_t  addr_ptr;                   /* zuletzt per Adressregister gewaehlter Index            */
} q9_mc6845_t;

//────────────────────────────────────────────────────────────────────────────────────────────────
// Function: q9_mc6845_init
// Desc.:    Nullstellt alle Register (Reset-Zustand: kein sinnvolles Bild, wie beim echten Chip
//           vor der Programmierung durch den Treiber).
//────────────────────────────────────────────────────────────────────────────────────────────────
void q9_mc6845_init(q9_mc6845_t *c);

//────────────────────────────────────────────────────────────────────────────────────────────────
// Function: q9_mc6845_stride / q9_mc6845_height / q9_mc6845_mode
// Desc.:    Bequeme Zugriffe auf die abgeleitete Geometrie (s. Geometrie-Interpretation im
//           Dateikopf), fuer 5.26 (Framebuffer-Geraet) und 5.27 (Host-Video-Bridge).
//────────────────────────────────────────────────────────────────────────────────────────────────
uint32_t q9_mc6845_stride(const q9_mc6845_t *c);   /* R1, Bytes/Zeile                             */
uint32_t q9_mc6845_height(const q9_mc6845_t *c);    /* R6, Pixel                                    */
int      q9_mc6845_mode  (const q9_mc6845_t *c);    /* R18, Q9_MC6845_MODE_*                        */

//────────────────────────────────────────────────────────────────────────────────────────────────
// Function: q9_mc6845_videoclk_khz
// Desc.:    R16/R17 (VideoClk High/Low) als zusammengesetzter 16-Bit-Wert in kHz -- ersetzt die
//           Si5351A-PLL-Rechnung (5.25): kein Multiplikator/Divider-Modell, einfach die fertige
//           Pixel-Taktfrequenz, die der Treiber/das Tuning-Tool (5.31) direkt hineinschreibt.
//────────────────────────────────────────────────────────────────────────────────────────────────
uint32_t q9_mc6845_videoclk_khz(const q9_mc6845_t *c);

//────────────────────────────────────────────────────────────────────────────────────────────────
// Function: q9_mc6845_bpp / q9_mc6845_width_px
// Desc.:    5.27: Bits/Pixel des aktuellen Video-Modus bzw. daraus abgeleitete Pixelbreite (aus
//           Stride, R1) -- fuer die Host-Video-Bridge (Q9_VIDEO_INFO braucht width/bpp in Pixeln,
//           R1/R18 liefern nur Byte-Stride+Modus). q9_mc6845_width_px() nimmt an, dass Stride ein
//           Vielfaches der bpp-Granularitaet ist (s. Tabelle im Dateikopf) -- bei Verletzung rundet
//           Integer-Division schlicht ab (kein Crash, analog zur "Treiber-Verantwortung"-Haltung
//           bei R1/R6).
//────────────────────────────────────────────────────────────────────────────────────────────────
int      q9_mc6845_bpp(int mode);
uint32_t q9_mc6845_width_px(const q9_mc6845_t *c);

//────────────────────────────────────────────────────────────────────────────────────────────────
// Function: q9_mc6845_net_update_hz
// Desc.:    5.27: R19 -- wie oft die Host-Video-Bridge (videobridge.h) geaenderte Bereiche
//           hoechstens pro Sekunde ans Netz sendet (Andreas, 2026-08-03: "ein Register, wie oft
//           der Frame ueber das Netzwerk aktualisiert wird"). Einheit Hz statt ms-Abstand gewaehlt,
//           weil Q9-Frame ARCHITECTURE.md bereits von "update_hz" (Default 30) spricht. R19=0
//           (Reset-Zustand, noch nicht vom Treiber programmiert) liefert Q9_MC6845_NET_HZ_DEFAULT
//           zurueck statt eines sofortigen Dauerfeuers/Div-durch-0.
//────────────────────────────────────────────────────────────────────────────────────────────────
#define Q9_MC6845_NET_HZ_DEFAULT 30u          /* deckungsgleich mit Q9-Frame ARCHITECTURE.md      */

uint32_t q9_mc6845_net_update_hz(const q9_mc6845_t *c);

//─── Geraete-Vtable fuer die Registry (devreg.h) ────────────────────────────────────────────────
extern const q9_device_vtable_t q9_devtype_mc6845;
extern const q9_devdesc_t       q9_devdesc_mc6845;         /* 2026-08-21: Vtable+Schema vereint    */

#endif /* Q9_MC6845_H */
//────────────────────────────────────────────────────────────────────────────────────────────────
// EOF mc6845.h                                                                            Ver. 1.03
//────────────────────────────────────────────────────────────────────────────────────────────────
