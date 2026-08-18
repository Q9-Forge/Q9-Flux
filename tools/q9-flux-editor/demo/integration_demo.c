//════════════════════════════════════════════════════════════════════════════════════════════════
// File:   integration_demo.c                                                             Ver. 2.70
// Owner:  Claudia
// Desc.:  Reine SICHTPRUEFUNG (kein automatisierter Test, wie ansi_selftest --demo) -- zeigt alle
//         sechs Bausteine zusammen in einem einzigen, echten Bildschirm: Rahmen (q9_widgets),
//         scrollbare Liste (q9_listview) in einem Bildschirmpuffer (q9_screenbuf), Tastatur-
//         Navigation + dynamische Groessenanpassung (q9_input). NICHT der eigentliche Q9-Flux-
//         Editor (keine echte Config-Anbindung, keine Hardware-Typen, keine Buttons) -- nur der
//         Nachweis, dass die Bausteine zusammenpassen.
//
//         Farbpalette (Andreas' Wunsch, 2026-08-17): "aehnliche" Toene statt bunt gemischt --
//         warme Amber-/Beige-/Braun-Palette (Vorbild: Hermes-Farbschema). Eine "kuehle" Variante
//         (Blau/Gruen/Cyan) waere nach demselben Muster (nur die Zahlenwerte unten aendern) ebenso
//         moeglich -- eine echte UMSCHALTBARE Palette (mehrere Saetze + Auswahl zur Laufzeit) ist
//         als "Endausbau"-Idee vorgemerkt, hier erstmal nur EIN fest verdrahteter Satz.
//
// Call:   make -C tools/q9-flux-editor demo-integration
//         Pfeiltasten hoch/runter: Auswahl bewegen. O: Datei-Auswahl-Dialog oeffnen (scannt ".",
//         das Arbeitsverzeichnis der Demo, s. run_file_dialog()). Strg-C: beenden.
//
// Edition History
//─────────┬──────┬────────────────────────────────────────────────────────────────────────┬──────
// Date    │ Ver. │ Description                                                            │ By
//─────────┼──────┼────────────────────────────────────────────────────────────────────────┬──────
// 26-08-17│ 1.00 │ Erster Wurf -- Andreas wollte sich das Ganze mal ansehen                │ Cld
// 26-08-17│ 1.10 │ Andreas' Feedback nach dem ersten Ansehen: Mindestgroesse (60x20, sonst  │ Cld
//         │      │ Hinweis statt verzerrtem Layout), Statuszeile bekommt eine EIGENE Zeile   │
//         │      │ mit eigenem Hintergrund statt in die untere Rahmenkante gemischt zu       │
//         │      │ werden (das war die "kleine Linie ganz rechts unten"), warme Amber-/      │
//         │      │ Beige-Palette statt der bisherigen Zufallsfarben                          │
// 26-08-17│ 1.20 │ Zweite Feedback-Runde: Statuszeile jetzt ALS untere Rahmenkante (nicht mehr │ Cld
//         │      │ eigene separate Zeile darueber) -- Resize-Flackern behoben (150ms-Entprel-  │
//         │      │ lung in q9_input.c), Scrollbalken-Abstand zur markierten Zeile (q9_listview.c)│
// 26-08-17│ 1.30 │ Statuszeile jetzt UEBER DIE VOLLE BREITE (Andreas: "aufgeraeumter als dieser │ Cld
//         │      │ doppelte Strich") -- ueberschreibt auch die beiden unteren Eckzeichen, keine │
//         │      │ Ecken mehr unten                                                             │
// 26-08-17│ 1.40 │ Dritte Feedback-Runde: Kopfzeile jetzt ebenfalls volle Breite (etwas heller  │ Cld
//         │      │ als die Statuszeile), feste Feldbreiten in der Statuszeile (kein Hin- und    │
//         │      │ Herspringen mehr bei unterschiedlich langen Eintragsnamen), neuer Resize-    │
//         │      │ Overlay-Modus: waehrend/nach einer Groessenaenderung wird HOECHSTENS 1s lang │
//         │      │ nur "R Rows - C Columns" zentriert angezeigt (LIVE aktualisiert, kein voller  │
//         │      │ Neuaufbau bei jedem Zwischenschritt) -- erst nach 1s Stille kommt der volle   │
//         │      │ Inhalt zurueck. Ist das Fenster dabei (immer noch) zu klein, bleibt das       │
//         │      │ Overlay dauerhaft sichtbar (plus Zusatzzeile), statt den vollen Inhalt zu     │
//         │      │ versuchen -- ersetzt die vorherige separate "Fenster zu klein"-Anzeige         │
// 26-08-17│ 1.50 │ Vierte Feedback-Runde: Overlay zeigt "Columns - Rows" (statt "Rows - Columns"),│ Cld
//         │      │ Kopfzeile-Titel linksbuendig ab Spalte 3 (statt zentriert), helleres Weiss;    │
//         │      │ bei anhaltend zu kleinem Fenster EINMALIGER Versuch, per XTWINOPS-Escape-       │
//         │      │ Sequenz (q9_ansi_resize_window) automatisch auf die Mindestgroesse zu           │
//         │      │ vergroessern -- nicht universell unterstuetzt, wirkt nur auf Terminals mit      │
//         │      │ aktivierten "Window Ops" (z.B. xterm)                                           │
// 26-08-17│ 1.60 │ Taste 'O' oeffnet den modalen Datei-Auswahl-Dialog (q9_filedialog.h/.c,          │ Cld
//         │      │ task #20/#22) zentriert ueber dem Bildschirm, scannt "." mit Beispielfiltern     │
//         │      │ ("*.*"/".c"/".h"); Ergebnis (Datei gewaehlt/Abbruch) ersetzt bis zur naechsten   │
//         │      │ Dialog-Oeffnung den unteren Hinweistext. DEMO-GRENZE: ein Resize waehrend der    │
//         │      │ Dialog offen ist, wird ignoriert (kein Nachziehen der Dialog-Geometrie)          │
// 26-08-17│ 1.70 │ Andreas' Feedback nach dem ersten Test: Dialog-Hintergrund ist jetzt der ECHTE   │ Cld
//         │      │ Hauptbildschirm (build_full_content() ausgelagert) statt einer reinen Fuellfarbe │
//         │      │ ueber den ganzen Schirm -- vorher sah der (schon immer kleine) Dialog dadurch    │
//         │      │ wie Vollbild aus                                                                 │
// 26-08-17│ 1.80 │ Zweite Feedback-Runde: eigene footer_bg-Palette fuer q9_filedialog.c (Fuss-       │ Cld
//         │      │ bereich mit den "richtigen" Buttons), Scan-Verzeichnis auf getenv("HOME") gestellt │
//         │      │ (Andreas: "stell den Pfad mal auf das ~ Verzeichnis, dann sieht man das besser"), │
//         │      │ DIALOG_ROWS/_COLS vergroessert (neuer Fussbereich braucht mehr Platz)             │
// 26-08-17│ 1.90 │ Dritte Feedback-Runde: Hauptfenster-Liste reicht jetzt bis zur rechten Rahmen-    │ Cld
//         │      │ kante von draw_frame() -- verschmilzt mit q9_listview's Bildlaufleiste zu EINER   │
//         │      │ Linie statt "Fensterkante + separate Bildlaufleiste" (Andreas' Wunsch)            │
// 26-08-17│ 2.00 │ Vierte Feedback-Runde ("wirkt jetzt doch gequetscht"): DIALOG_ROWS/_COLS         │ Cld
//         │      │ vergroessert (neue Filterzeile + Statuszeile im Dialog, plus Luftspalten)         │
// 26-08-17│ 2.10 │ Fuenfte Feedback-Runde ("zwei Zeilen sparen"): DIALOG_ROWS wieder verkleinert    │ Cld
//         │      │ (Buttons teilen sich jetzt Namens-/Filterzeile statt eigene Zeilen zu belegen)    │
// 26-08-17│ 2.20 │ Sechste Feedback-Runde ("Farben Richtung Braun abgerutscht"): komplette Palette   │ Cld
//         │      │ auf durchgehende Gelb-/Orange-Leiter umgerechnet (gleicher Farbton/Saettigung,    │
//         │      │ nur die Helligkeit unterscheidet die Ebenen), Tabellenhintergrund dunkler,         │
//         │      │ groesserer Helligkeitssprung zwischen den Ebenen fuer mehr Kontrast                │
// 26-08-17│ 2.30 │ Siebte Feedback-Runde: Kopfzeilen-Text jetzt dunkel statt fast-weiss (schlecht     │ Cld
//         │      │ lesbar auf hellem Gelb), neue PAL_DIALOG_SUB_FG fuer Tabellenkopf/Namens-Kaestchen/│
//         │      │ OK-Abbrechen (mehr Kontrast), PAL_DIALOG_FOOTER_BG referenziert jetzt PAL_STATUS_BG│
//         │      │ direkt, Hauptfenster-Liste bekommt line_fg=PAL_FRAME (Linien-Farbinkonsistenz-Fix) │
// 26-08-17│ 2.40 │ Achte Feedback-Runde: render_size_overlay() nutzt PAL_STATUS_FG statt PAL_HEADER_FG│ Cld
//         │      │ (war unlesbar dunkel geworden, kein farbiger Hintergrund dort), neue eigene       │
//         │      │ status_fg/bg-Felder fuer die Dialog-Statuszeile (jetzt = Hauptfenster-Statuszeile),│
//         │      │ run_file_dialog() behandelt Resize waehrend der Dialog offen ist (compute_dialog_  │
//         │      │ geometry() zentriert dabei automatisch neu), rows/cols jetzt Zeiger                │
// 26-08-18│ 2.60 │ Sechzehnte Feedback-Runde: g_items durch g_list_items ersetzt (Name + Detailzeilen,│ Cld
//         │      │ q9_listview_item_t), g_expanded-Array, Enter klappt den ausgewaehlten Eintrag      │
//         │      │ auf/zu, Haupt-Listenansicht nutzt jetzt q9_listview_render_ex()/_scroll_ex()/       │
//         │      │ _move_ex() (erweiterbare Eintraege, s. q9_listview.h)                               │
// 26-08-18│ 2.70 │ Siebzehnte Feedback-Runde: neue PAL_LIST_EXP_BG (V=0.44) fuer die Kopfzeile eines  │ Cld
//         │      │ aufgeklappten, nicht ausgewaehlten Eintrags (Andreas: "die Headerzeile geht ein    │
//         │      │ wenig unter")                                                                       │
// 26-08-17│ 2.50 │ Neunte Feedback-Runde ("da wird immer alles neu gezeichnet"): run_file_dialog()    │ Cld
//         │      │ nutzt jetzt dasselbe Overlay-Settle-Muster wie main() -- waehrend des Ziehens nur  │
//         │      │ billiges render_size_overlay(), teurer Dialog-Neuaufbau erst nach RESIZE_SETTLE_MS │
//         │      │ Stille statt bei jedem Zwischenschritt; render_size_overlay() zeigt den Groessen-  │
//         │      │ Text jetzt an fester Position (OVERLAY_ROW/_COL = 3,3) statt zentriert (huepfte     │
//         │      │ sonst waehrend des Ziehens staendig an eine andere Stelle)                          │
//═════════╧══════╧════════════════════════════════════════════════════════════════════════╧══════
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "../src/q9_ansi.h"
#include "../src/q9_screenbuf.h"
#include "../src/q9_widgets.h"
#include "../src/q9_listview.h"
#include "../src/q9_input.h"
#include "../src/q9_filedialog.h"

/* Rein zur Demonstration -- kein echtes Hardware-Modell, s. Kopfkommentar. Erweiterbare Eintraege
   (Andreas' Wunsch, 2026-08-18: "groessere Eintraege... minimiert ein oder zwei Zeilen, aufgeklappt
   so viele wie sie brauchen") -- jeder Eintrag hat jetzt eine Kopfzeile (name, wie vorher g_items[])
   plus ein paar Detailzeilen, die nur sichtbar werden, wenn der Eintrag aufgeklappt ist (Enter auf
   der Auswahl, s. main()). Zwei Zeilen je Eintrag reichen fuer die Vorfuehrung -- das Datenmodell
   (q9_listview_item_t.detail_count, s. q9_listview.h) erlaubt aber pro Eintrag eine BELIEBIGE Anzahl,
   das ist keine feste Grenze der Bibliothek. */
static const char *const g_cf_detail[]     = { "Bus:    onboard         Basis:  $FFFFE000",
                                                "Slot:   -               Aktiv:  ja" };
static const char *const g_net1_detail[]   = { "Port:   2001            Protokoll: Telnet",
                                                "Status: bereit          Baudrate: -" };
static const char *const g_net2_detail[]   = { "Port:   2002            Protokoll: Telnet",
                                                "Status: bereit          Baudrate: -" };
static const char *const g_net3_detail[]   = { "Port:   2003            Protokoll: Telnet",
                                                "Status: bereit          Baudrate: -" };
static const char *const g_net4_detail[]   = { "Port:   2004            Protokoll: Telnet",
                                                "Status: bereit          Baudrate: -" };
static const char *const g_net5_detail[]   = { "Port:   2005            Protokoll: Telnet",
                                                "Status: bereit          Baudrate: -" };
static const char *const g_net6_detail[]   = { "Port:   2006            Protokoll: Telnet",
                                                "Status: bereit          Baudrate: -" };
static const char *const g_net7_detail[]   = { "Port:   2007            Protokoll: Telnet",
                                                "Status: bereit          Baudrate: -" };
static const char *const g_net8_detail[]   = { "Port:   2008            Protokoll: Telnet",
                                                "Status: bereit          Baudrate: -" };
static const char *const g_rtc_detail[]    = { "Basis:  $FFFFA000       IRQ:    -",
                                                "Batterie: ok            Aktiv:  ja" };
static const char *const g_duart_detail[]  = { "Basis:  $FFFFA000       IRQ:    2",
                                                "Kanal A: Konsole        Kanal B: frei" };
static const char *const g_quicc_detail[]  = { "MAC:    00:1A:2B:03:04:05",
                                                "Link:   nein            Aktiv:  nein" };
static const char *const g_mc6845_detail[] = { "Basis:  $FFFF9000       IRQ:    3",
                                                "Modus:  Text 80x25      Aktiv:  ja" };
static const char *const g_clut_detail[]   = { "Basis:  $FFFF9800       Eintraege: 256",
                                                "Tiefe:  8 Bit           Aktiv:  ja" };
static const char *const g_rc2014_detail[] = { "Bus:    rc2014          Basis:  $FFFFC010",
                                                "Slot:   0               Aktiv:  nein" };
static const char *const g_fb_detail[]     = { "Basis:  $00300000       Groesse: 512K",
                                                "Aufloesung: 640x480     Aktiv:  ja" };
static const char *const g_ram_detail[]    = { "Basis:  $00000000       Groesse: 4 MB",
                                                "Parity: nein            Getestet: ja" };
static const char *const g_rom_detail[]    = { "Basis:  $00F00000       Groesse: 256K",
                                                "Schreibschutz: ja       Aktiv:  ja" };
static const char *const g_nvram_detail[]  = { "Basis:  $FFFFB000       Groesse: 2K",
                                                "Batterie: ok            Aktiv:  ja" };
static const char *const g_timer_detail[]  = { "Basis:  $FFFFA800       IRQ:    3",
                                                "Intervall: 10ms         Aktiv:  ja" };

static const q9_listview_item_t g_list_items[] = {
    { "CF-Interface (onboard, c0)", g_cf_detail,     2 },
    { "Netz-Terminal x1",           g_net1_detail,   2 },
    { "Netz-Terminal x2",           g_net2_detail,   2 },
    { "Netz-Terminal x3",           g_net3_detail,   2 },
    { "Netz-Terminal x4",           g_net4_detail,   2 },
    { "Netz-Terminal x5",           g_net5_detail,   2 },
    { "Netz-Terminal x6",           g_net6_detail,   2 },
    { "Netz-Terminal x7",           g_net7_detail,   2 },
    { "Netz-Terminal x8",           g_net8_detail,   2 },
    { "RTC72421 (Echtzeituhr)",     g_rtc_detail,    2 },
    { "DUART 68681 (Konsole)",      g_duart_detail,  2 },
    { "QUICC-Ethernet",             g_quicc_detail,  2 },
    { "MC6845 (GDP/CRTC)",          g_mc6845_detail, 2 },
    { "CLUT (Farbtabelle)",         g_clut_detail,   2 },
    { "RC2014-CF (sekundaer)",      g_rc2014_detail, 2 },
    { "Framebuffer (VRAM)",         g_fb_detail,     2 },
    { "Systemspeicher (RAM)",       g_ram_detail,    2 },
    { "ROM-Spiegel",                g_rom_detail,    2 },
    { "NVRAM (Akku-gepuffert)",     g_nvram_detail,  2 },
    { "Timer/IRQ3-Trigger",         g_timer_detail,  2 },
};
#define ITEM_COUNT (int)(sizeof(g_list_items) / sizeof(g_list_items[0]))

/* 0 = zugeklappt (Default), 1 = aufgeklappt -- Enter auf der Hauptliste klappt den AUSGEWAEHLTEN
   Eintrag auf/zu (s. main()), mehrere gleichzeitig aufgeklappte Eintraege sind ausdruecklich erlaubt
   (kein "nur einer offen"-Akkordeon -- einfacher zu verstehen, kein ueberraschendes Zuklappen
   anderer Eintraege). */
static int g_expanded[ITEM_COUNT];

/* Warme Gelb-/Orange-Palette. Neunte Feedback-Runde (Andreas, 2026-08-17): "die ganzen Farben
   sind jetzt alle so in Richtung Braun abgerutscht... mehr in Richtung gelb orange" -- alle Toene
   HIER auf denselben Farbton (~42 Grad, Orange-Gelb) und dieselbe hohe Saettigung (~82%)
   umgerechnet, nur die HELLIGKEIT (V) unterscheidet die Ebenen -- eine durchgehende Leiter von
   ganz dunkel (Tabellenhintergrund) bis ganz hell (Kopfzeile/Auswahl), damit der Sprung zwischen
   den Ebenen klarer wirkt ("etwas mehr Kontrast"), statt wie zuvor nur unterschiedlich HELLE
   BRAUNTOENE zu sein:
     V=0.15  PAL_DIALOG_BODY_BG (Tabellenhintergrund -- "noch etwas dunkler")
     V=0.32  PAL_DIALOG_FOOTER_BG / PAL_STATUS_BG ("der untere Teil")
     V=0.44  PAL_DIALOG_SUB_BG (Tabellenkopf/Spaltentitel-Zeile, Button-Grundfarbe)
     V=0.72  PAL_FRAME (Rahmenlinien im Hauptfenster + Text auf PAL_STATUS_BG)
     V=0.80  PAL_HEADER_BG
     V=0.90  PAL_SEL_BG (hellster, kraeftigster Farbton -- die Auswahl-Hervorhebung)

   Zehnte Feedback-Runde, selber Tag: "in den beiden Kopfzeile das weiss auf dem hellen Gelb...
   im Tabellenkopf und unter der Tabelle zu wenig Kontrast" -- zwei zusaetzliche Text-Sonderfaelle,
   die NICHT einfach denselben V-Schritt wie ihr Hintergrund bekommen koennen (das waere ja gerade
   der Kontrast-Verlust), sondern bewusst ans jeweils ANDERE Ende der Leiter geholt werden:
   PAL_HEADER_FG jetzt so dunkel wie PAL_DIALOG_BODY_BG (dunkler Text auf hellem PAL_HEADER_BG,
   statt fast-weiss), PAL_DIALOG_SUB_FG (NEU) so hell wie PAL_HEADER_FG vorher war (heller Text auf
   dem mittelhellen PAL_DIALOG_SUB_BG). */
#define PAL_FRAME_R      184
#define PAL_FRAME_G      138
#define PAL_FRAME_B       33
#define PAL_LIST_FG_R    217
#define PAL_LIST_FG_G    191
#define PAL_LIST_FG_B    130
#define PAL_SEL_FG_R      35
#define PAL_SEL_FG_G      25
#define PAL_SEL_FG_B      10
#define PAL_SEL_BG_R     230
#define PAL_SEL_BG_G     173
#define PAL_SEL_BG_B      41
#define PAL_STATUS_FG_R  230
#define PAL_STATUS_FG_G  212
#define PAL_STATUS_FG_B  178
#define PAL_STATUS_BG_R   82
#define PAL_STATUS_BG_G   62
#define PAL_STATUS_BG_B   15
#define PAL_HEADER_FG_R   38                                /* Andreas' Wunsch (2026-08-17, zehnte */
#define PAL_HEADER_FG_G   29                                 /* Runde): "weiss auf hellem Gelb ist  */
#define PAL_HEADER_FG_B    7                                 /* schlecht lesbar" -- jetzt dunkel    */
#define PAL_HEADER_BG_R  204                                /* etwas heller als PAL_STATUS_BG,    */
#define PAL_HEADER_BG_G  154                                 /* gleiche Farbfamilie                */
#define PAL_HEADER_BG_B   37
/* V=0.44 (derselbe Ton/Saettigung wie der Rest der Leiter, s.o.) -- bisher nur im Datei-Dialog
   verwendet (PAL_DIALOG_SUB_BG, Tabellenkopf/Buttons), hier fuer denselben Zweck im Hauptfenster:
   Kopfzeile eines AUFGEKLAPPTEN, aber nicht ausgewaehlten Listeneintrags (Andreas' Feedback,
   2026-08-18, siebzehnte Runde: "die Headerzeile geht ein wenig unter" -- ohne Auswahl sah eine
   aufgeklappte Kopfzeile bisher aus wie jede andere, ging neben den gedaempften Detailzeilen
   darunter optisch unter). Bewusst DIESELBEN Zahlen wie PAL_DIALOG_SUB_BG (keine neue Sprosse auf
   der Leiter noetig), aber als eigene Konstante benannt -- der Datei-Dialog und diese Liste sind
   unabhaengige Aufrufer, sollen aber nicht zufaellig aneinander gekoppelt sein. */
#define PAL_LIST_EXP_BG_R  112
#define PAL_LIST_EXP_BG_G   85
#define PAL_LIST_EXP_BG_B   20

#define MIN_ROWS 20                                        /* Andreas' Wunsch (2026-08-17):     */
#define MIN_COLS 60                                         /* darunter sieht es "sehr komisch"
                                                                aus -- Hinweis statt Versuch      */
#define RESIZE_SETTLE_MS 1000                               /* Andreas' Wunsch: waehrend eines
                                                                Resizes nur die Groesse zeigen,
                                                                nach 1s Stille zurueck zum Inhalt  */

/* Feste Feldbreiten fuer die Statuszeile (Andreas' Wunsch: "sonst huepfen die Texte hin und her").
   NAME_FIELD_WIDTH >= der laengste Name in g_list_items ("CF-Interface (onboard, c0)" = 27 Zeichen). */
#define NAME_FIELD_WIDTH 30

/* Zusaetzliche Toene NUR fuer den Datei-Auswahl-Dialog -- Teil derselben Gelb-/Orange-Leiter wie
   oben (gleicher Farbton/Saettigung, s. dortiger Kopfkommentar), V=0.15 (dunkelster Schritt, "der
   Tabellenhintergrund bitte noch etwas dunkler"). */
#define PAL_DIALOG_BODY_BG_R  38
#define PAL_DIALOG_BODY_BG_G  29
#define PAL_DIALOG_BODY_BG_B   7
/* Tabellenkopf/Spaltentitel-Zeile + Button-Grundfarbe, V=0.44 -- deutlicher Sprung gegenueber
   PAL_DIALOG_BODY_BG (V=0.15) UND gegenueber PAL_DIALOG_FOOTER_BG (V=0.32) fuer "etwas mehr
   Kontrast" (Andreas' Wunsch, 2026-08-17). */
#define PAL_DIALOG_SUB_BG_R  112
#define PAL_DIALOG_SUB_BG_G   85
#define PAL_DIALOG_SUB_BG_B   20
/* TEXT auf PAL_DIALOG_SUB_BG ("Name Datum Groesse", das Namens-Kaestchen, OK/Abbrechen
   unfokussiert) -- Andreas' Feedback, zehnte Runde: "im Tabellenkopf... und OK/Abbrechen zu wenig
   Kontrast". Vorher wurde dafuer PAL_FRAME (V=0.72) verwendet -- zu nah an PAL_DIALOG_SUB_BG
   (V=0.44) fuer guten Kontrast, UND PAL_FRAME wird an anderer Stelle (Rahmenlinien, Hinweistext)
   bewusst NICHT geaendert (haette dort unerwuenschte Nebenwirkungen). Stattdessen ein eigener,
   heller Farbton NUR fuer diese Rolle -- derselbe Wert, den PAL_HEADER_FG bis zur letzten Runde
   hatte (fast-weiss, jetzt frei geworden, s.o.). */
#define PAL_DIALOG_SUB_FG_R  255
#define PAL_DIALOG_SUB_FG_G  248
#define PAL_DIALOG_SUB_FG_B  225
/* Fussbereich ("der untere Teil"), V=0.32 -- zwischen PAL_DIALOG_BODY_BG und PAL_DIALOG_SUB_BG,
   deutlich sichtbar anders als beide. Bewusst IDENTISCH mit PAL_STATUS_BG (Andreas' Wunsch:
   "den Footer des Dialogs in der Farbe des Hauptfensters machen") -- ueber die Konstante selbst
   referenziert statt nur zufaellig gleiche Zahlen zu haben, damit das auch so BLEIBT. */
#define PAL_DIALOG_FOOTER_BG_R  PAL_STATUS_BG_R
#define PAL_DIALOG_FOOTER_BG_G  PAL_STATUS_BG_G
#define PAL_DIALOG_FOOTER_BG_B  PAL_STATUS_BG_B

#define DIALOG_ROWS 18                                      /* Wunschgroesse -- wird in            */
#define DIALOG_COLS 56                                       /* run_file_dialog() an rows/cols geklemmt.
                                                                  18 statt 20 (2026-08-17, sechste Runde:
                                                                  "ich wollte noch zwei Zeilen sparen") --
                                                                  Buttons teilen sich jetzt Namens-/
                                                                  Filterzeile statt eigene Zeilen zu
                                                                  belegen, 2 Zeilen weniger noetig         */

static void write_ansi(unsigned (*fn)(char *, unsigned))
{
    char buf[32];
    unsigned n = fn(buf, sizeof(buf));
    fwrite(buf, 1, n, stdout);
}

static unsigned wrap_hide(char *b, unsigned n)  { return q9_ansi_hide_cursor(b, n); }
static unsigned wrap_show(char *b, unsigned n)  { return q9_ansi_show_cursor(b, n); }

/* q9_screenbuf_init() klemmt intern zwar schon auf Q9_SCREENBUF_MAX_ROWS/_COLS (s. dortiger
   Kopfkommentar) -- ohne diese Klemmung HIER wuerden rows/cols aber trotzdem noch die groesseren,
   unklemmten Werte fuer Layout-Berechnungen (draw_frame/listview-Geometrie) verwenden, was auf sehr
   grossen Terminals (>200 Spalten) zu einem Rahmen fuehren wuerde, der nicht bis zum tatsaechlichen
   rechten/unteren Rand des Puffers reicht -- kein Crash (q9_screenbuf_* klemmt selbst pro Zelle),
   aber ein unschoenes Layout. Deshalb hier vorsorglich mit klemmen. */
static void clamp_dims(int *rows, int *cols)
{
    if (*rows > Q9_SCREENBUF_MAX_ROWS) { *rows = Q9_SCREENBUF_MAX_ROWS; }
    if (*cols > Q9_SCREENBUF_MAX_COLS) { *cols = Q9_SCREENBUF_MAX_COLS; }
}

/* Ersetzt die fruehere separate "Fenster zu klein"-Meldung: EIN einheitliches Overlay fuer zwei
   Faelle -- (a) waehrend/kurz nach einer Groessenaenderung (too_small=0, wird nach
   RESIZE_SETTLE_MS Stille wieder durch den vollen Inhalt ersetzt) und (b) das Fenster ist
   (weiterhin) kleiner als die Mindestgroesse (too_small=1, bleibt dauerhaft sichtbar, zeigt
   zusaetzlich die Mindestgroesse). Beide Faelle sind bewusst DASSELBE einfache "nur die Groesse"-
   Layout -- Andreas: "sieht doof aus, wenn man immer versucht den kompletten Inhalt darzustellen".

   Feste Position OVERLAY_ROW/OVERLAY_COL statt zentriert (Andreas' Feedback, 2026-08-17,
   fuenfzehnte Runde: "das duerfte auch nicht mehr so durch die Gegend huepfen") -- bei zentrierter
   Position haengt die Zeichenposition selbst von rows/cols ab, der Text "sprang" beim Ziehen also
   bei JEDEM Zwischenschritt an eine andere Bildschirmstelle. Feste Position bleibt stattdessen
   immer an derselben Stelle stehen (dieselbe Spalte wie Kopfzeilen-Titel/Hinweistext/Listenansicht,
   s. build_full_content()). */
#define OVERLAY_ROW 3
#define OVERLAY_COL 3
static void render_size_overlay(int rows, int cols, int too_small)
{
    q9_screenbuf_t sb;
    char out[4096];
    char line1[64];

    q9_screenbuf_init(&sb, rows, cols);

    /* Columns vor Rows (Andreas' Wunsch, 2026-08-17: "Columns und Rows solltest du bitte
       tauschen") -- entspricht auch der ueblichen "80x24"-Schreibweise (Spalten x Zeilen). */
    snprintf(line1, sizeof(line1), "%d Columns - %d Rows", cols, rows);
    /* PAL_STATUS_FG statt PAL_HEADER_FG (Andreas' Feedback, 2026-08-17): dieser Text steht auf
       KEINEM eigenen farbigen Hintergrund (nur q9_screenbuf_init(), also Terminal-Default,
       typischerweise dunkel) -- PAL_HEADER_FG wurde in der letzten Runde bewusst DUNKEL gemacht
       (Kontrast auf dem jetzt kraeftigen PAL_HEADER_BG), hier fehlte dieser Hintergrund also
       komplett und der Text war praktisch unsichtbar. PAL_STATUS_FG ist nach wie vor hell und
       genau fuer "Text ohne eigenen Hintergrund" gedacht. */
    q9_screenbuf_puts(&sb, OVERLAY_ROW, OVERLAY_COL, line1,
                       PAL_STATUS_FG_R, PAL_STATUS_FG_G, PAL_STATUS_FG_B);

    if (too_small) {
        char line2[64];
        snprintf(line2, sizeof(line2), "Fenster zu klein (mind. %dx%d)", MIN_COLS, MIN_ROWS);
        q9_screenbuf_puts(&sb, OVERLAY_ROW + 1, OVERLAY_COL, line2,
                           PAL_STATUS_FG_R, PAL_STATUS_FG_G, PAL_STATUS_FG_B);
    }

    {
        unsigned n = q9_screenbuf_render(&sb, 0, 0, out, sizeof(out));
        fwrite(out, 1, n, stdout);
        fflush(stdout);
    }
}

/* Baut den vollen Hauptbildschirm-Inhalt in sb auf (KEIN stdout-Schreiben) -- ausgelagert aus
   render_full_content(), damit run_file_dialog() denselben echten Hintergrund hinter dem Dialog
   zeigen kann (Andreas' Feedback, 2026-08-17: "auf volle Groesse hatte ich mir den nicht
   vorgestellt" -- der Dialog WAR schon immer nur DIALOG_ROWS x DIALOG_COLS gross, sah aber wie
   Vollbild aus, weil vorher der GESAMTE Bildschirm mit der Dialog-Hintergrundfarbe gefuellt wurde,
   statt den echten Hauptbildschirm dahinter stehen zu lassen -- jetzt behoben).
   hint: NULL/leer -> Standardtext ("Pfeiltasten: ..."); sonst wird STATTDESSEN hint angezeigt --
   dient der Demo dazu, das Ergebnis des Datei-Auswahl-Dialogs (task #22) sichtbar zu machen, ohne
   die Statuszeile selbst (feste Feldbreiten, s.o.) umbauen zu muessen. */
static void build_full_content(q9_screenbuf_t *sb, q9_listview_t *lv, int rows, int cols,
                                const char *hint)
{
    char status[256];

    q9_screenbuf_init(sb, rows, cols);
    q9_screenbuf_draw_frame(sb, 0, 0, rows, cols,
                             "Q9-Flux Editor -- Integrations-Demo",
                             PAL_FRAME_R, PAL_FRAME_G, PAL_FRAME_B);

    /* Kopfzeile UEBER DIE VOLLE BREITE (Andreas' Wunsch, 2026-08-17, analog zur Statuszeile) --
       ueberschreibt auch die beiden oberen Eckzeichen von draw_frame(), etwas heller als die
       Statuszeile (PAL_HEADER_* statt PAL_STATUS_*), damit man Kopf/Fuss auf einen Blick
       unterscheiden kann, aber in derselben Farbfamilie bleibt. Der Titeltext von draw_frame()
       wird hier mit demselben Text erneut geschrieben (draw_frame's eigene Titel-Platzierung wird
       durch fill_rect vollstaendig ueberschrieben) -- LINKSBUENDIG ab Spalte 3 statt zentriert
       (Andreas' Wunsch, 2026-08-17: "lass uns mal links versuchen, ab dem dritten Zeichen" --
       Spalte 3 passt auch zur Linksbuendigkeit von Listenansicht/Hinweistext weiter unten). */
    q9_screenbuf_fill_rect(sb, 0, 0, 1, cols, ' ',
                            PAL_HEADER_FG_R, PAL_HEADER_FG_G, PAL_HEADER_FG_B,
                            1, PAL_HEADER_BG_R, PAL_HEADER_BG_G, PAL_HEADER_BG_B);
    q9_screenbuf_puts(sb, 0, 3, "Q9-Flux Editor -- Integrations-Demo",
                       PAL_HEADER_FG_R, PAL_HEADER_FG_G, PAL_HEADER_FG_B);

    q9_screenbuf_puts(sb, rows - 2, 3,
                       (hint && hint[0]) ? hint
                                         : "Pfeiltasten: navigieren   Enter: auf-/zuklappen   "
                                           "O: Datei oeffnen   Strg-C: beenden",
                       PAL_FRAME_R, PAL_FRAME_G, PAL_FRAME_B);

    lv->row    = 2;
    lv->col    = 3;
    lv->height = rows - 5;                                   /* Rand+Hinweis+Statuszeile/-kante s.u. */
    /* Bis zur rechten Rahmenkante des Fensters (Andreas' Wunsch, 2026-08-17, zweite Runde: "im
       Hauptfenster haben wir immer noch rechts die Fensterkante UND zusaetzlich die Bildlauf-
       leiste, das soll jetzt in einem sein" -- q9_listview's eigene rechte Spalte, s. q9_listview.c,
       liegt jetzt direkt AUF draw_frame()'s eigener rechter Kante statt 3 Spalten davor). */
    lv->width  = cols - lv->col;
    if (lv->height < 1) { lv->height = 1; }
    if (lv->width  < 1) { lv->width  = 1; }
    /* _ex statt der einfachen q9_listview_scroll() -- Eintraege koennen jetzt mehr als eine
       Bildschirmzeile brauchen (aufgeklappt), s. q9_listview.h. */
    lv->scroll_offset = q9_listview_scroll_ex(lv->selected, lv->scroll_offset, lv->height,
                                               g_list_items, g_expanded, lv->item_count);
    /* line_fg = PAL_FRAME (NICHT PAL_LIST_FG) -- die Linie liegt seit der dritten Feedback-Runde
       direkt AUF draw_frame()'s eigener Kante (s.o.), muss also auch DIESELBE Farbe zeigen, sonst
       wirken links/rechts UND verschiedene Hoehen der rechten Kante unterschiedlich eingefaerbt
       (Andreas' Feedback, 2026-08-17, siebte Runde: "die Striche links und rechts... sind
       unterschiedlich... das oberste rechts ist noch mal anders" -- die Eckzeichen/Kanten VOR und
       NACH dem Listenbereich blieben in PAL_FRAME, waehrend die Liste selbst bisher PAL_LIST_FG
       zeichnete, obwohl beide auf derselben Spalte liegen). detail_fg = PAL_FRAME ebenfalls -- die
       eingerueckten Detailzeilen bekommen dieselbe gedaempfte Farbe wie der Hinweistext unten
       (strukturell/sekundaer, nicht der "wichtige" Text wie der Eintragsname selbst). exp_bg =
       PAL_LIST_EXP_BG -- Kopfzeile eines aufgeklappten, nicht ausgewaehlten Eintrags (Andreas'
       Feedback, siebzehnte Runde: "die Headerzeile geht ein wenig unter"). */
    q9_listview_render_ex(lv, sb, g_list_items, g_expanded,
                           PAL_LIST_FG_R, PAL_LIST_FG_G, PAL_LIST_FG_B,
                           PAL_SEL_FG_R, PAL_SEL_FG_G, PAL_SEL_FG_B,
                           PAL_SEL_BG_R, PAL_SEL_BG_G, PAL_SEL_BG_B,
                           PAL_FRAME_R, PAL_FRAME_G, PAL_FRAME_B,
                           PAL_FRAME_R, PAL_FRAME_G, PAL_FRAME_B,
                           PAL_LIST_EXP_BG_R, PAL_LIST_EXP_BG_G, PAL_LIST_EXP_BG_B);

    /* Statuszeile ALS untere Rahmenkante, ueber die volle Breite (vorherige Feedback-Runden) --
       jetzt zusaetzlich mit FESTEN Feldbreiten (Andreas: "sonst huepfen die Texte hin und her"):
       der Eintragsname wird auf NAME_FIELD_WIDTH Zeichen aufgefuellt (linksbuendig), die
       Terminal-Groesse mit fester Breite je Zahl (rows dreistellig rechtsbuendig, cols dreistellig
       linksbuendig) -- "Terminal:" steht dadurch bei jeder Auswahl/Groesse an derselben Spalte. */
    q9_screenbuf_fill_rect(sb, rows - 1, 0, 1, cols, ' ',
                            PAL_STATUS_FG_R, PAL_STATUS_FG_G, PAL_STATUS_FG_B,
                            1, PAL_STATUS_BG_R, PAL_STATUS_BG_G, PAL_STATUS_BG_B);
    snprintf(status, sizeof(status), " Ausgewaehlt: %-*.*s | Terminal: %3dx%-3d",
             NAME_FIELD_WIDTH, NAME_FIELD_WIDTH,
             (lv->selected >= 0 && lv->selected < ITEM_COUNT) ? g_list_items[lv->selected].name : "-",
             rows, cols);
    q9_screenbuf_puts(sb, rows - 1, 1, status, PAL_STATUS_FG_R, PAL_STATUS_FG_G, PAL_STATUS_FG_B);
}

static void render_full_content(q9_listview_t *lv, int rows, int cols, const char *hint)
{
    q9_screenbuf_t sb;
    char out[1 << 16];
    unsigned n;

    build_full_content(&sb, lv, rows, cols, hint);
    n = q9_screenbuf_render(&sb, 0, 0, out, sizeof(out));
    fwrite(out, 1, n, stdout);
    fflush(stdout);
}

/* Berechnet dlg_row/dlg_col/dlg_rows/dlg_cols aus der aktuellen Terminal-Groesse -- IMMER zentriert
   (Andreas' Wunsch, 2026-08-17, elfte Runde: "waere auch gut wenn der Dialog nach dem
   Positionieren immer wieder mittig positioniert wird"). Ausgelagert aus run_file_dialog(), damit
   dieselbe Formel sowohl beim ersten Oeffnen ALS AUCH bei einem Resize waehrend der Dialog offen
   ist verwendet wird (s. dort) -- ein Resize zentriert den Dialog dadurch automatisch neu. */
static void compute_dialog_geometry(int rows, int cols, int *dlg_row, int *dlg_col,
                                     int *dlg_rows, int *dlg_cols)
{
    *dlg_rows = DIALOG_ROWS;
    *dlg_cols = DIALOG_COLS;
    if (*dlg_rows > rows - 2) { *dlg_rows = rows - 2; }
    if (*dlg_cols > cols - 2) { *dlg_cols = cols - 2; }
    *dlg_row = (rows - *dlg_rows) / 2;
    *dlg_col = (cols - *dlg_cols) / 2;
    if (*dlg_row < 1) { *dlg_row = 1; }
    if (*dlg_col < 1) { *dlg_col = 1; }
}

/* Oeffnet den modalen Datei-Auswahl-Dialog (q9_filedialog.h/.c, task #20) zentriert ueber dem
   aktuellen Bildschirm, scannt HOME (s.u.) mit ein paar Beispiel-Filtern -- reine Vorfuehrung,
   keine echte Config-Anbindung, s. Kopfkommentar.
   Der Dialog ist bewusst NUR DIALOG_ROWS x DIALOG_COLS gross (Andreas' Feedback, 2026-08-17:
   "auf volle Groesse hatte ich mir den jetzt nicht vorgestellt") -- als Hintergrund steht der
   ECHTE Hauptbildschirm (ueber build_full_content(), s.o.), nicht mehr eine reine Fuellfarbe ueber
   den ganzen Schirm (das war der eigentliche Grund, warum es vorher wie Vollbild aussah, obwohl
   der Dialog selbst schon immer klein war).
   rows/cols sind jetzt Zeiger (nicht mehr nur Eingabe) -- ein Terminal-Resize WAEHREND der Dialog
   offen ist wird jetzt behandelt (Andreas' Feedback, elfte Runde: "Fenster Groesse aendern
   waehrend ein Dialog auf ist funktioniert nicht richtig"). ZWEITER Anlauf, fuenfzehnte Runde
   (Andreas: "geht immer noch nicht, da wird immer alles neu gezeichnet") -- der ERSTE Anlauf
   (vierzehnte Runde) hat bei JEDEM einzelnen Q9_KEY_RESIZE (waehrend des Ziehens an der
   Terminal-Ecke kommen davon viele kurz hintereinander, s. main()) sofort den KOMPLETTEN Dialog
   per q9_filedialog_init() + vollem Redraw neu aufgebaut -- teuer und sichtbar ruckelig/
   flackernd bei jedem Zwischenschritt. Jetzt dasselbe Overlay-Settle-Muster wie main() (s.
   RESIZE_SETTLE_MS oben): waehrend gezogen wird, nur das billige render_size_overlay() (fixe
   Position, kein Dialog-Redraw); q9_term_size() wird pro Q9_KEY_RESIZE-Ereignis neu abgefragt,
   aber der teure Dialog-Neuaufbau (compute_dialog_geometry() + q9_filedialog_init(), zentriert
   dabei automatisch neu -- erledigt den Wunsch "immer wieder mittig") passiert erst EINMAL, nach
   RESIZE_SETTLE_MS Stille. DEMO-GRENZE: das Neu-Initialisieren setzt Fokus/Auswahl/Filter auf ihre
   Startwerte zurueck (kein Nachziehen des BISHERIGEN Zustands) -- ein echter Editor wuerde hier
   gezielter nur die Geometrie aktualisieren; fuer die Demo ist das ein akzeptabler Kompromiss
   (Resize mitten in der Dateiauswahl ist ein Randfall). Die Zeiger werden aktualisiert, damit der
   Aufrufer nach Rueckkehr die dann aktuelle Terminal-Groesse kennt. Strg-C/EOF waehrend des Dialogs
   wird NICHT ignoriert (sonst liesse sich das Programm aus dem Dialog heraus nicht mehr beenden) --
   signalisiert per Rueckgabe 0 an den Aufrufer, der dann seinerseits sauber beendet.
   Rueckgabe: 1 = Datei ausgewaehlt (result_msg beschreibt sie), -1 = abgebrochen (result_msg
   entsprechend gesetzt), 0 = Strg-C/EOF (result_msg unveraendert -- Aufrufer beendet ohnehin).
   result_msg dient WAEHREND der Dialoglaufzeit zusaetzlich als "aktueller Hinweistext" fuer den
   Hintergrund (unveraendert bis zum Ende der Funktion) -- spart einen eigenen Parameter dafuer. */
static int run_file_dialog(int *rows, int *cols, q9_listview_t *lv,
                            char *result_msg, unsigned result_msg_size)
{
    static const char *const filters[] = { "*.*", ".c", ".h" };
    const char *home = getenv("HOME");
    const char *dir = home ? home : ".";
    q9_filedialog_palette_t pal;
    q9_filedialog_t dlg;
    int dlg_row, dlg_col, dlg_rows, dlg_cols;
    int done = 0;
    int want_quit = 0;
    int showing_overlay = 0;                                 /* wie main(): billiges Groessen-
                                                                 Overlay waehrend des Ziehens statt
                                                                 teurem Dialog-Redraw bei jedem
                                                                 Zwischenschritt, s.o. */

    compute_dialog_geometry(*rows, *cols, &dlg_row, &dlg_col, &dlg_rows, &dlg_cols);

    memset(&pal, 0, sizeof(pal));
    pal.header_fg_r = PAL_HEADER_FG_R; pal.header_fg_g = PAL_HEADER_FG_G; pal.header_fg_b = PAL_HEADER_FG_B;
    pal.header_bg_r = PAL_HEADER_BG_R; pal.header_bg_g = PAL_HEADER_BG_G; pal.header_bg_b = PAL_HEADER_BG_B;
    pal.sub_fg_r    = PAL_DIALOG_SUB_FG_R; pal.sub_fg_g = PAL_DIALOG_SUB_FG_G; pal.sub_fg_b = PAL_DIALOG_SUB_FG_B;
    pal.sub_bg_r    = PAL_DIALOG_SUB_BG_R;  pal.sub_bg_g = PAL_DIALOG_SUB_BG_G;  pal.sub_bg_b = PAL_DIALOG_SUB_BG_B;
    pal.body_fg_r   = PAL_LIST_FG_R;   pal.body_fg_g   = PAL_LIST_FG_G;   pal.body_fg_b   = PAL_LIST_FG_B;
    pal.body_bg_r   = PAL_DIALOG_BODY_BG_R; pal.body_bg_g = PAL_DIALOG_BODY_BG_G; pal.body_bg_b = PAL_DIALOG_BODY_BG_B;
    pal.list_fg_r   = PAL_LIST_FG_R;   pal.list_fg_g   = PAL_LIST_FG_G;   pal.list_fg_b   = PAL_LIST_FG_B;
    pal.sel_fg_r    = PAL_SEL_FG_R;    pal.sel_fg_g    = PAL_SEL_FG_G;    pal.sel_fg_b    = PAL_SEL_FG_B;
    pal.sel_bg_r    = PAL_SEL_BG_R;    pal.sel_bg_g    = PAL_SEL_BG_G;    pal.sel_bg_b    = PAL_SEL_BG_B;
    pal.focus_fg_r  = PAL_SEL_FG_R;    pal.focus_fg_g  = PAL_SEL_FG_G;    pal.focus_fg_b  = PAL_SEL_FG_B;
    pal.focus_bg_r  = PAL_SEL_BG_R;    pal.focus_bg_g  = PAL_SEL_BG_G;    pal.focus_bg_b  = PAL_SEL_BG_B;
    /* Andreas' Feedback (2026-08-17): "wenn der Selektor auf die Dateiauswahl steht sehe ich
       nichts" -- die markierte Zeile sah IMMER gleich aus, egal ob die Liste den Fokus hatte oder
       nicht (der Fokus-Wechsel war dadurch unsichtbar). Jetzt gedaempfter Ton, wenn die Liste
       NICHT fokussiert ist -- gleiche Grundfarbe wie die Spaltentitel-Zeile, aber lesbarer Text
       (statt komplett unsichtbarer Markierung), s. q9_filedialog.c list_focus-Unterscheidung. */
    pal.unfocus_sel_fg_r = PAL_LIST_FG_R; pal.unfocus_sel_fg_g = PAL_LIST_FG_G; pal.unfocus_sel_fg_b = PAL_LIST_FG_B;
    pal.unfocus_sel_bg_r = PAL_DIALOG_SUB_BG_R; pal.unfocus_sel_bg_g = PAL_DIALOG_SUB_BG_G; pal.unfocus_sel_bg_b = PAL_DIALOG_SUB_BG_B;
    /* Fussbereich (zweite Feedback-Runde, 2026-08-17) -- eigener Hintergrund ab der Statuszeile,
       s. PAL_DIALOG_FOOTER_BG_* oben. Text darauf in derselben Farbe wie der Dialog-Fliesstext. */
    pal.footer_fg_r = PAL_LIST_FG_R; pal.footer_fg_g = PAL_LIST_FG_G; pal.footer_fg_b = PAL_LIST_FG_B;
    pal.footer_bg_r = PAL_DIALOG_FOOTER_BG_R; pal.footer_bg_g = PAL_DIALOG_FOOTER_BG_G; pal.footer_bg_b = PAL_DIALOG_FOOTER_BG_B;
    /* Untere Statuszeile -- EXAKT dieselbe Farbe wie die Hauptfenster-Statuszeile (Andreas'
       Feedback, elfte Runde: "die Statuszeilen sind noch unterschiedlich"), ueber eigene
       status_fg/bg-Felder statt header_fg/bg (das bliebe sonst an die Dialog-Kopfzeile gekoppelt,
       s. q9_filedialog.h). */
    pal.status_fg_r = PAL_STATUS_FG_R; pal.status_fg_g = PAL_STATUS_FG_G; pal.status_fg_b = PAL_STATUS_FG_B;
    pal.status_bg_r = PAL_STATUS_BG_R; pal.status_bg_g = PAL_STATUS_BG_G; pal.status_bg_b = PAL_STATUS_BG_B;

    /* Andreas' Wunsch (2026-08-17): "stell den Pfad bitte mal auf das ~ Verzeichnis, dann sieht
       man das besser" -- HOME statt "." fuer den Test (mehr/andere Dateien als im leeren
       Demo-Arbeitsverzeichnis). Reine Vorfuehrung, keine echte Config-Anbindung, s. Kopfkommentar.
       Kein HOME gesetzt (z.B. manche minimalen Umgebungen) -> Rueckfall auf ".". */
    if (q9_filedialog_init(&dlg, dlg_row, dlg_col, dlg_rows, dlg_cols,
                            "Konfigurationsauswahl", dir, filters, 3, &pal) != 0) {
        snprintf(result_msg, result_msg_size, "Dateidialog: Fehler beim Start (O: erneut versuchen)");
        return -1;
    }

    for (;;) {
        int too_small = (*rows < MIN_ROWS || *cols < MIN_COLS);

        if (showing_overlay) {
            /* Billig: nur die Groesse anzeigen, KEIN Dialog-/Hintergrund-Redraw (s.o.). */
            render_size_overlay(*rows, *cols, too_small);
        } else {
            q9_screenbuf_t sb;
            char out[1 << 16];

            /* Echter Hauptbildschirm als Hintergrund (s. Funktionskommentar) -- der Dialog selbst
               ueberschreibt danach nur sein EIGENES Rechteck (q9_filedialog_render() faengt mit
               seinem eigenen fill_rect ueber dlg->row/col/rows/cols an), der Rest bleibt sichtbar. */
            build_full_content(&sb, lv, *rows, *cols, result_msg);
            q9_filedialog_render(&dlg, &sb);
            {
                unsigned n = q9_screenbuf_render(&sb, 0, 0, out, sizeof(out));
                fwrite(out, 1, n, stdout);
                fflush(stdout);
            }
        }

        {
            q9_key_t k = showing_overlay ? q9_input_read_key_timeout(RESIZE_SETTLE_MS)
                                          : q9_input_read_key();

            if (showing_overlay && k.kind == Q9_KEY_NONE) {
                /* RESIZE_SETTLE_MS ohne weiteres Resize-Ereignis abgelaufen -- JETZT (und nur
                   jetzt) den teuren Dialog-Neuaufbau machen: Geometrie neu berechnen (zentriert
                   automatisch neu, s. compute_dialog_geometry()) und den Dialog MIT DENSELBEN
                   Filtern/Verzeichnis/Palette neu aufsetzen (s. Funktionskommentar zur
                   DEMO-GRENZE). Bleibt das Fenster zu klein, einfach im Overlay bleiben (naechste
                   Runde zeichnet es idempotent neu, wie in main()). */
                if (!too_small) {
                    compute_dialog_geometry(*rows, *cols, &dlg_row, &dlg_col, &dlg_rows, &dlg_cols);
                    if (q9_filedialog_init(&dlg, dlg_row, dlg_col, dlg_rows, dlg_cols,
                                            "Konfigurationsauswahl", dir, filters, 3, &pal) != 0) {
                        snprintf(result_msg, result_msg_size,
                                 "Dateidialog: Fehler nach Groessenaenderung (Esc: abbrechen)");
                        return -1;
                    }
                    showing_overlay = 0;
                }
                continue;
            }

            if (k.kind == Q9_KEY_CTRL_C || k.kind == Q9_KEY_EOF) { want_quit = 1; break; }

            if (k.kind == Q9_KEY_RESIZE) {
                /* Nur die neue Groesse merken und ins (billige) Overlay wechseln -- der teure
                   Dialog-Neuaufbau passiert oben erst nach RESIZE_SETTLE_MS Stille, NICHT bei
                   jedem einzelnen Zwischenschritt waehrend des Ziehens (das war der eigentliche
                   Bug: "da wird immer alles neu gezeichnet"). */
                if (q9_term_size(rows, cols) == 0) {
                    clamp_dims(rows, cols);
                }
                showing_overlay = 1;
                continue;
            }

            if (showing_overlay) { continue; }              /* andere Tasten waehrend des
                                                                 Overlays: ignorieren (wie main()) */

            done = q9_filedialog_handle_key(&dlg, k);
            if (done != 0) { break; }
        }
    }

    if (want_quit) { return 0; }

    if (done == 1) {
        char name[Q9_FILELIST_NAME_MAX];
        q9_filedialog_selected_name(&dlg, name, sizeof(name));
        snprintf(result_msg, result_msg_size, "Datei gewaehlt: %s   (O: erneut oeffnen)", name);
        return 1;
    }
    snprintf(result_msg, result_msg_size, "Dateiauswahl abgebrochen   (O: erneut oeffnen)");
    return -1;
}

int main(void)
{
    int rows, cols;
    q9_listview_t lv;
    int running = 1;
    int showing_overlay = 0;                                /* 1 = Resize-Overlay statt Vollinhalt */
    int resize_attempted = 0;                                /* s.u.: XTWINOPS-Versuch nur EINMAL
                                                                  pro zu-klein-Phase, nicht bei jeder
                                                                  einzelnen 1s-Wiederholung erneut   */
    char last_dialog_msg[128] = "";                          /* Ergebnis des letzten Datei-Dialogs
                                                                  (task #22) -- ersetzt den Standard-
                                                                  Hinweistext, bis 'O' erneut gedrueckt
                                                                  wird, s. run_file_dialog()          */

    if (q9_term_size(&rows, &cols) != 0) {
        rows = 24;
        cols = 80;
    }
    clamp_dims(&rows, &cols);

    if (q9_input_init() != 0) {
        fprintf(stderr, "integration_demo: kein echtes Terminal (TTY) -- Abbruch.\n");
        return 1;
    }
    write_ansi(wrap_hide);

    q9_listview_init(&lv, 2, 3, 1, 1, ITEM_COUNT);          /* echte Geometrie folgt in render_full_content */

    while (running) {
        int too_small = (rows < MIN_ROWS || cols < MIN_COLS);
        if (too_small) { showing_overlay = 1; }              /* zu klein -> immer Overlay, s.u. */

        if (showing_overlay) {
            render_size_overlay(rows, cols, too_small);
        } else {
            render_full_content(&lv, rows, cols, last_dialog_msg);
        }

        {
            q9_key_t k = showing_overlay ? q9_input_read_key_timeout(RESIZE_SETTLE_MS)
                                          : q9_input_read_key();

            if (showing_overlay && k.kind == Q9_KEY_NONE) {
                /* RESIZE_SETTLE_MS ohne weitere Aenderung abgelaufen. Nur zurueck zum vollen
                   Inhalt, wenn die Groesse tatsaechlich ausreicht -- sonst bleibt das Overlay
                   (mit "zu klein"-Zusatzzeile) einfach stehen und wird in der naechsten Runde
                   identisch neu gezeichnet (idempotent, kein Problem). */
                if (!too_small) {
                    showing_overlay = 0;
                } else if (!resize_attempted) {
                    /* Andreas' Wunsch (2026-08-17): bei anhaltend zu kleinem Fenster EINMAL
                       versuchen, das Terminal per XTWINOPS auf die Mindestgroesse zu bringen
                       (q9_ansi_resize_window, NICHT universell unterstuetzt, s. dortiger
                       Kopfkommentar -- wirkt es, kommt ganz normal ein neues Q9_KEY_RESIZE mit
                       der dann tatsaechlichen Groesse; wirkt es nicht, passiert einfach nichts,
                       das Overlay bleibt unveraendert stehen). Nur EINMAL pro zu-klein-Phase, nicht
                       bei jeder 1s-Wiederholung erneut (sonst wuerde ein Terminal, das die
                       Sequenz konsequent ignoriert, sie trotzdem staendig neu bekommen). */
                    char rbuf[32];
                    unsigned rn = q9_ansi_resize_window(rbuf, sizeof(rbuf), MIN_ROWS, MIN_COLS);
                    fwrite(rbuf, 1, rn, stdout);
                    fflush(stdout);
                    resize_attempted = 1;
                }
                continue;
            }

            switch (k.kind) {
                case Q9_KEY_CTRL_C:
                case Q9_KEY_EOF:
                    running = 0;
                    break;
                case Q9_KEY_UP:
                    if (!showing_overlay) { q9_listview_move_ex(&lv, -1, g_list_items, g_expanded); }
                    break;
                case Q9_KEY_DOWN:
                    if (!showing_overlay) { q9_listview_move_ex(&lv, 1, g_list_items, g_expanded); }
                    break;
                case Q9_KEY_ENTER:
                    /* Andreas' Wunsch (2026-08-18): "erweiterbare Items" -- Enter klappt den
                       AUSGEWAEHLTEN Eintrag auf/zu. delta=0 bewegt die Auswahl nicht, richtet aber
                       scroll_offset neu aus (der aufgeklappte/zugeklappte Eintrag hat jetzt eine
                       andere Zeilenzahl, s. q9_listview_move_ex()). Eintraege ohne Detailzeilen
                       (detail_count<=0) haben ohnehin kein Pfeil-Symbol -- toggeln ist fuer sie
                       wirkungslos (q9_listview_item_rows() liefert immer 1), kein Sonderfall noetig. */
                    if (!showing_overlay && lv.selected >= 0 && lv.selected < ITEM_COUNT) {
                        g_expanded[lv.selected] = !g_expanded[lv.selected];
                        q9_listview_move_ex(&lv, 0, g_list_items, g_expanded);
                    }
                    break;
                case Q9_KEY_RESIZE:
                    if (q9_term_size(&rows, &cols) == 0) {
                        clamp_dims(&rows, &cols);
                    }
                    showing_overlay = 1;                     /* sofort ins Overlay, LIVE aktualisiert
                                                                 bei weiteren RESIZE-Ereignissen     */
                    resize_attempted = 0;                    /* neue zu-klein-Phase (falls es dazu
                                                                 kommt) darf wieder EINEN Versuch
                                                                 machen */
                    break;
                case Q9_KEY_CHAR:
                    if (!showing_overlay && (k.ch == 'o' || k.ch == 'O')) {
                        /* task #22: modaler Datei-Auswahl-Dialog (q9_filedialog.h/.c, task #20). */
                        int r = run_file_dialog(&rows, &cols, &lv, last_dialog_msg, sizeof(last_dialog_msg));
                        if (r == 0) { running = 0; }         /* Strg-C/EOF waehrend des Dialogs */
                        /* naechste Schleifenrunde zeichnet automatisch alles neu (inkl. last_dialog_msg) */
                    }
                    break;
                default:
                    break;                                   /* alle anderen Tasten: ignorieren */
            }
        }
    }

    {
        char buf[64];
        unsigned n = 0;
        n += q9_ansi_reset(buf + n, sizeof(buf) - n);
        n += q9_ansi_clear(buf + n, sizeof(buf) - n);
        n += q9_ansi_move(buf + n, sizeof(buf) - n, 1, 1);
        fwrite(buf, 1, n, stdout);
    }
    write_ansi(wrap_show);
    fflush(stdout);
    q9_input_shutdown();
    printf("integration_demo beendet.\n");
    return 0;
}

//────────────────────────────────────────────────────────────────────────────────────────────────
// EOF integration_demo.c                                                                  Ver. 2.70
//────────────────────────────────────────────────────────────────────────────────────────────────
