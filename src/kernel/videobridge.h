//════════════════════════════════════════════════════════════════════════════════════════════════
// File:   videobridge.h                                                                  Ver. 1.00
// Owner:  AF
// Desc.:  5.27: Host-Video-Bridge -- portiert das Q9-Frame-Protokoll (HELLO/VIDEO_INFO/PALETTE/
//         FRAME_FULL/FRAME_UPDATE ueber TCP + UDP-Discovery, s. Q9-Frame `src/protocol.h`,
//         `src/framebuffer.h`, `tests/dummy_server.cpp`) aus dem Test-Dummy in den Emulator: statt
//         synthetischer Testmuster liest dieses Modul das ECHTE VRAM (framebuf.h, 5.26) und die
//         ECHTE Geometrie/den Modus (mc6845.h, 5.24/5.25) und spiegelt Aenderungen an genau EINEN
//         verbundenen Q9-Frame-Client (analog `dummy_server.cpp`, kein Multi-Client -- s.u.).
//
//         **Nicht-blockierend (Q9-Frame ARCHITECTURE.md: "darf CPU-Emulation ... nicht anhalten"):**
//         alle Sockets laufen mit O_NONBLOCK (exakt das schon etablierte Muster der Netz-Terminals,
//         s. m68krt.c init_network_terminals/update_network_terminals) -- q9_videobridge_poll() wird
//         einmal je Hauptschleifen-Runde aufgerufen (cb030run.c) und kehrt immer sofort zurueck.
//
//         **Bewusste Vereinfachungen ggue. der Zielarchitektur (ARCHITECTURE.md), analog zu den
//         bisherigen "erst einfach"-Entscheidungen (framebuf.h Board-Config, mc6845.h Register-
//         Vereinfachungen):**
//           - GENAU EIN aktiver Client (wie `dummy_server.cpp`) statt Mehrfach-Client-Sitzungen mit
//             eigener Sendewarteschlange je Client -- ein neuer `accept()` wird erst bedient, wenn
//             der bisherige Client getrennt ist.
//           - Kein Backpressure/Queueing bei langsamen Clients: schlaegt ein `write()` fehl (EAGAIN
//             oder kurzer Schreibzugriff), wird die Verbindung getrennt statt Updates zu sammeln/zu
//             verwerfen (ARCHITECTURE.md nennt das als Zielverhalten, hier erstmal einfach: Client
//             fliegt raus, naechster `accept()` bedient ihn/den naechsten neu mit Vollbild).
//           - CLUT fuer indizierte Modi: **NACHTRAG 2026-08-10 (Claude, 5.29):** echtes CLUT-Geraet
//             (clut.h) existiert jetzt, `vb->clut` liefert die vom Gast per SS_clut/SS_clutall
//             programmierte Palette. Die alte Graustufen-Rampe (`build_grayscale_clut`) bleibt nur
//             als Fallback erhalten, falls `q9_videobridge_init()` mit `clut=NULL` aufgerufen wird
//             (z.B. isolierte Tests ohne CLUT-Geraet).
//
//         **Modus-/Geometrieaenderung zur Laufzeit:** jede Poll-Runde wird die aktuelle Stride/
//         Hoehe/Modus (mc6845.h) mit dem zuletzt an den Client gemeldeten Stand verglichen -- bei
//         Aenderung wird komplett neu gehandshaked (VIDEO_INFO(+PALETTE)+FRAME_FULL), analog zum
//         Live-Moduswechsel in `dummy_server.cpp` (dort per stdin-Kommando ausgeloest, hier durch
//         echte Registeraenderungen eines OS-9-Treibers).
//
//         **Dirty-Rects:** framebuf.h liefert sie in (Byte-Spalte, Zeile); die Umrechnung in Pixel-
//         Koordinaten (Q9DirtyRectHeader) nutzt q9_mc6845_bpp() -- Byte-Spalte*[8/4/2/1] bzw.
//         Byte-Spalte/[2/3] je nach bpp (s. mc6845.h-Kopf, dieselbe Granularitaets-Tabelle).
//
// Call:   q9_videobridge_t vb;
//         q9_videobridge_init(&vb, &fb, &crtc, &clut, Q9_VIDEOBRIDGE_TCP_PORT, Q9_VIDEOBRIDGE_UDP_PORT, "Q9Flux");
//         // je Hauptschleifen-Runde:
//         q9_videobridge_poll(&vb, now_ms);
//
// Edition History
//─────────┬──────┬────────────────────────────────────────────────────────────────────────┬──────
// Date    │ Ver. │ Description                                                            │ By
//─────────┼──────┼────────────────────────────────────────────────────────────────────────┼──────
// 26-08-03│ 1.00 │ 5.27: Erster Wurf                                                       │ Ada
//═════════╧══════╧════════════════════════════════════════════════════════════════════════╧══════
#ifndef Q9_VIDEOBRIDGE_H
#define Q9_VIDEOBRIDGE_H

#include <stdint.h>
#include "framebuf.h"
#include "mc6845.h"
#include "clut.h"                                      /* 5.29-Nachtrag: q9_clut_t                 */

#define Q9_VIDEOBRIDGE_TCP_PORT   2001    /* wie Q9-Frame tests/dummy_server.cpp             */
#define Q9_VIDEOBRIDGE_UDP_PORT   2000    /* Discovery, wie Q9-Frame src/udp_scan.cpp         */
#define Q9_VIDEOBRIDGE_NAME_MAX   32
/* Update-Rate: KEINE feste Konstante mehr -- kommt aus MC6845-Register R19 (5.27, Andreas'
   Wunsch, s. q9_mc6845_net_update_hz in mc6845.h), Default Q9_MC6845_NET_HZ_DEFAULT (30 Hz,
   deckungsgleich mit Q9-Frame ARCHITECTURE.md "update_hz"). */

/* WICHTIG: enthaelt einen mehrere MByte grossen Sendepuffer (out_buf, s.u.) -- NIEMALS als lokale
   (Stack-)Variable anlegen (Stack-Overflow), immer `static` oder als Teil einer bereits statischen
   Struktur (analog q9_framebuf_t/q9_mc6845_t in cb030run.c). */
typedef struct {
    int      udp_fd;
    int      tcp_fd;
    int      client_fd;              /* -1 = kein Client verbunden                          */
    int      handshake_have;         /* bereits akkumulierte HELLO-Bytes (0..Header-Groesse) */
    int      handshake_done;         /* VIDEO_INFO/PALETTE/FRAME_FULL fuer diese Verbindung   */
                                      /* bereits gesendet                                     */
    uint32_t seq;
    uint32_t last_send_ms;

    /* Zuletzt an den Client gemeldete Geometrie -- Abweichung loest Neu-Handshake aus. */
    uint32_t adv_stride;
    uint32_t adv_height;
    int      adv_mode;
    uint32_t adv_clut_gen;   /* 5.29-Nachtrag: zuletzt gesendeter clut->generation-Stand, s. clut.h */

    uint16_t tcp_port;
    char     name[Q9_VIDEOBRIDGE_NAME_MAX];

    /* Nicht-blockierender Sendepuffer (s. Dateikopf) -- EIN zusammenhaengender Puffer fuer den
       jeweils aktuellen Handshake- ODER Dirty-Update-Payload; drain_output() (videobridge.c)
       schickt ihn ueber ggf. mehrere Poll-Runden nichtblockierend raus. Obergrenze Q9_FRAMEBUF_
       MAX_SIZE + Kopfzeilen-Spielraum, da ein volles FRAME_FULL bis zu einem ganzen VRAM-Fenster
       gross werden kann (statisch, kein malloc -- Q9-Grundsatz). */
    uint8_t  out_buf[Q9_FRAMEBUF_MAX_SIZE + 4096u];
    uint32_t out_len;
    uint32_t out_sent;
    int      out_pending;

    q9_framebuf_t     *fb;
    const q9_mc6845_t *crtc;
    const q9_clut_t   *clut;   /* 5.29-Nachtrag: echte CLUT statt Graustufen-Platzhalter, s. clut.h */
} q9_videobridge_t;

//────────────────────────────────────────────────────────────────────────────────────────────────
// Function: q9_videobridge_init
// Desc.:    Oeffnet UDP-Discovery- und TCP-Video-Socket (beide O_NONBLOCK, SO_REUSEADDR). fb/crtc/
//           clut muessen die gesamte Laufzeit ueberleben (nur als Zeiger gehalten, analog
//           framebuf.h). clut darf NULL sein (5.29-Nachtrag optional -- faellt dann weiterhin auf
//           die alte Graustufen-Platzhalterpalette zurueck, z.B. fuer Tests ohne CLUT-Geraet).
//           Rueckgabe: 0 = ok, -1 = Socket-Fehler (bind/listen).
//────────────────────────────────────────────────────────────────────────────────────────────────
int q9_videobridge_init(q9_videobridge_t *vb, q9_framebuf_t *fb, const q9_mc6845_t *crtc,
                         const q9_clut_t *clut, uint16_t tcp_port, uint16_t udp_port,
                         const char *name);

//────────────────────────────────────────────────────────────────────────────────────────────────
// Function: q9_videobridge_poll
// Desc.:    Einmal je Hauptschleifen-Runde aufrufen (cb030run.c) -- niemals blockierend. Bedient
//           UDP-Discovery-Anfragen, nimmt (bei freiem Client-Slot) neue TCP-Verbindungen an,
//           handshaked neue Clients, erkennt Modus-/Geometrieaenderungen und sendet faellige
//           Dirty-Updates (gedrosselt auf die Rate aus MC6845-Register R19).
//────────────────────────────────────────────────────────────────────────────────────────────────
void q9_videobridge_poll(q9_videobridge_t *vb, uint32_t now_ms);

#endif /* Q9_VIDEOBRIDGE_H */
//────────────────────────────────────────────────────────────────────────────────────────────────
// EOF videobridge.h                                                                       Ver. 1.00
//────────────────────────────────────────────────────────────────────────────────────────────────
