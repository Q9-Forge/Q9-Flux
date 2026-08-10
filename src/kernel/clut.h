//════════════════════════════════════════════════════════════════════════════════════════════════
// File:   clut.h                                                                          Ver. 1.00
// Owner:  AF
// Desc.:  5.29-Nachtrag: Host-CLUT-Geraet (Color Lookup Table) fuer indizierte Videomodi -- schliesst
//         die in videobridge.h dokumentierte 5.27-Luecke ("CLUT fuer indizierte Modi ist eine
//         Graustufen-Rampe mangels eigenem CLUT-Geraet"). Ohne dieses Geraet konnte der Gast nur die
//         feste Graustufen-Platzhalterpalette sehen -- SS_clut/SS_clutall (mc6845.a) hatten kein
//         echtes Ziel zum Schreiben.
//
//         Adressmodell (analog MC6845/framebuf.h, feste Konstanten statt Board-Config, s. dortige
//         Begruendung): 4-Byte-Fenster ab Q9_CLUT_BASE = $FFFFA010 (direkt hinter dem 2-Byte-MC6845-
//         Fenster $FFFFA000-01, mit Sicherheitsabstand fuer eine spaetere Registererweiterung dort).
//         +0 Index (0-255, waehlt den aktiven Tabelleneintrag -- Lesen liefert den zuletzt gewaehlten
//         Index zurueck, analog MC6845). +1/+2/+3 = R/G/B (0-255) des GEWAEHLTEN Eintrags -- jeder
//         Schreibzugriff auf +1/+2/+3 aktualisiert SOFORT den entsprechenden Kanal (kein "commit"-
//         Schritt noetig, anders als z.B. ein "Write R,G,B dann Commit"-Design -- einfacher, und ein
//         Treiber, der ohnehin alle drei Kanaele hintereinander schreibt, sieht keinen Unterschied).
//
//         **Generation-Zaehler statt Dirty-Bit:** jeder Schreibzugriff erhoeht `generation` um 1 --
//         die Host-Video-Bridge (videobridge.c) vergleicht das mit ihrem zuletzt gesendeten Stand
//         und loest bei Aenderung ein neues PALETTE-Telegramm aus (analog dem bestehenden Stride/
//         Hoehe/Modus-Vergleich fuer VIDEO_INFO). Ein einfaches Dirty-Bit haette denselben Zweck
//         erfuellt, ein Zaehler ist aber unempfindlich gegen "gesetzt-dann-sofort-wieder-geloescht"-
//         Races zwischen Poll-Runden (hier irrelevant, da nur ein Schreiber, aber ohne Mehraufwand).
//
//         **Default-Zustand:** Identitaets-Graustufe (r=g=b=index) -- fuer 8bpp/INDEXED8 deckungsgleich
//         mit der bisherigen build_grayscale_clut()-Formel, fuer 1/2/4bpp NICHT identisch (die alte
//         Formel spreizte die wenigen Eintraege ueber den vollen 0-255-Bereich, hier bleiben es die
//         ERSTEN 2/4/16 Eintraege der vollen 256er-Rampe, also dunklere Werte) -- bewusst in Kauf
//         genommen, da ein Gasttreiber, der indizierte Farben ernsthaft nutzt, die CLUT ohnehin
//         selbst programmiert; der Default ist nur ein "irgendein sichtbares Bild vor dem ersten
//         SS_clutall"-Fallback.
//
// Call:   q9_clut_t clut; q9_clut_init(&clut);
//         q9_m68krt_attach_clut(&clut);   // registriert bei $FFFFA010
//
// Edition History
//─────────┬──────┬────────────────────────────────────────────────────────────────────────┬──────
// Date    │ Ver. │ Description                                                            │ By
//─────────┼──────┼────────────────────────────────────────────────────────────────────────┼──────
// 26-08-10│ 1.00 │ 5.29-Nachtrag: Erster Wurf (Claude)                                     │ Claude
//═════════╧══════╧════════════════════════════════════════════════════════════════════════╧══════
#ifndef Q9_CLUT_H
#define Q9_CLUT_H

#include <stdint.h>
#include "devreg.h"                                    /* q9_device_t/Vtable                     */

//─── Adressfenster ────────────────────────────────────────────────────────────────────────────────
#define Q9_CLUT_BASE   0xFFFFA010u                      /* hinter MC6845 $FFFFA000-01, s. Kopf     */
#define Q9_CLUT_TOP    0xFFFFA013u                      /* 4 Byte: Index + R + G + B                */

#define Q9_CLUT_ENTRIES 256

//─── Zustand ─────────────────────────────────────────────────────────────────────────────────────
typedef struct {
    uint8_t  r[Q9_CLUT_ENTRIES];
    uint8_t  g[Q9_CLUT_ENTRIES];
    uint8_t  b[Q9_CLUT_ENTRIES];
    uint8_t  index;        /* zuletzt per Indexregister gewaehlter Eintrag                          */
    uint32_t generation;   /* erhoeht sich bei JEDEM R/G/B-Schreibzugriff, s. Dateikopf              */
} q9_clut_t;

//────────────────────────────────────────────────────────────────────────────────────────────────
// Function: q9_clut_init
// Desc.:    Identitaets-Graustufe als Default (r=g=b=index), generation=0. Reset-Verhalten bewusst
//           NICHT alles-Null (waere fuer INDEXED8 ein komplett schwarzes Bild ohne jede
//           Unterscheidbarkeit) -- s. Dateikopf.
//────────────────────────────────────────────────────────────────────────────────────────────────
void q9_clut_init(q9_clut_t *c);

//─── Geraete-Vtable fuer die Registry (devreg.h) ────────────────────────────────────────────────
extern const q9_device_vtable_t q9_devtype_clut;

#endif /* Q9_CLUT_H */
//────────────────────────────────────────────────────────────────────────────────────────────────
// EOF clut.h                                                                              Ver. 1.00
//────────────────────────────────────────────────────────────────────────────────────────────────
