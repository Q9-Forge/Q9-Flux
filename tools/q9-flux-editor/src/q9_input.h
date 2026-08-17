//════════════════════════════════════════════════════════════════════════════════════════════════
// File:   q9_input.h                                                                      Ver. 1.40
// Owner:  Claudia
// Desc.:  Tastatur-Eingabe fuer den Q9-Flux-Editor -- Rohmodus + Tastenerkennung (Pfeiltasten,
//         Enter, Escape, Tab, Backspace, Strg-C). Letzter fehlender Baustein aus
//         Q9FLUX_EDITOR_de.md, um die bisherigen Teile (q9_screenbuf/q9_widgets/q9_procspawn) zu
//         echten, bedienbaren Bildschirmen zusammenzusetzen.
//
//         BEWUSST EIN EIGENER, VOM EMULATOR-HAL UNABHAENGIGER Rohmodus (nicht src/hal/*
//         wiederverwendet): src/hal/posix/hal_posix.c ist auf TRANSPARENTE DURCHREICHUNG an das
//         emulierte OS-9 zugeschnitten (Ctrl-C soll dort als echtes Byte beim Gast ankommen, CR
//         bleibt CR statt zu LF verbogen zu werden). Der Editor will das Gegenteil: Pfeiltasten/
//         Enter/Escape/Strg-C sollen als NAVIGATIONS-Befehle erkannt werden, nicht als Rohdaten
//         durchgereicht -- gleiche Grundtechnik (termios-Rohmodus), andere Verwendung.
//
//         Die eigentliche Escape-Sequenz-Erkennung (q9_input_decode) ist eine REINE Funktion --
//         Byte-Puffer rein, q9_key_t raus, kein echtes Terminal noetig (testbar wie q9_ansi/
//         q9_screenbuf). Nur q9_input_read_key() braucht ein echtes Terminal (liest tatsaechlich
//         von stdin) und ist deshalb NICHT automatisiert testbar -- reine, duenne I/O-Huelle um die
//         getestete Kernlogik.
//
//         ESC-Mehrdeutigkeit (klassisches Terminal-Problem): ein einzelnes ESC (Taste "Escape")
//         sieht auf Byte-Ebene identisch aus wie der ANFANG einer Pfeiltasten-Sequenz ("\x1b[A" fuer
//         Pfeil hoch). Loesung: q9_input_read_key() liest nach einem ESC-Byte mit einem KURZEN
//         Timeout (VTIME) weiter -- kommt innerhalb der Frist nichts nach, war es ein einzelnes ESC;
//         kommt sofort mehr, ist es eine Sequenz. Bei echten Tastaturen erzeugt eine Pfeiltaste die
//         komplette Sequenz binnen Mikrosekunden (ein Tastendruck = ein Escape-Code vom Terminal-
//         Treiber), ein einzelner ESC-Tastendruck erzeugt dagegen genau ein Byte -- das Zeitfenster
//         (50ms) ist fuer beide Faelle eindeutig, ausser bei sehr langsamen Netzwerk-Terminals
//         (fuer den lokalen Editor kein Thema).
//
// Call:   if (q9_input_init() != 0) { /* kein TTY */ }
//         for (;;) {
//             q9_key_t k = q9_input_read_key();
//             if (k.kind == Q9_KEY_CTRL_C) break;
//             ...
//         }
//         q9_input_shutdown();
//
// Edition History
//─────────┬──────┬────────────────────────────────────────────────────────────────────────┬──────
// Date    │ Ver. │ Description                                                            │ By
//─────────┼──────┼────────────────────────────────────────────────────────────────────────┬──────
// 26-08-16│ 1.00 │ Erster Wurf -- POSIX (termios) implementiert+getestet (decode-Kern),     │ Cld
//         │      │ Windows-Zweig (_kbhit/_getch, Scan-Codes wie hal_windows.c) geschrieben, │
//         │      │ mangels Windows-Host hier UNGETESTET                                     │
// 26-08-16│ 1.10 │ Andreas' Wunsch nach "halbwegs dynamischer" Grössenanpassung: q9_term_size│ Cld
//         │      │ (aktuelle Zeilen/Spalten) + Q9_KEY_RESIZE (POSIX: echtes SIGWINCH lost   │
//         │      │ den blockierenden read() sofort aus -- kein Polling noetig, Reaktion so   │
//         │      │ schnell wie das Terminal das Signal schickt; Windows: kein SIGWINCH-      │
//         │      │ Aequivalent, dort per Definition nur "beim naechsten Tastendruck neu      │
//         │      │ nachsehen" moeglich -- s. dortiger Kommentar)                             │
// 26-08-17│ 1.20 │ Resize-Entprellung (~150ms, Andreas: Zieh-Resize flackerte durch viele    │ Cld
//         │      │ SIGWINCH kurz hintereinander) -- Q9_KEY_RESIZE kommt jetzt erst NACH einer │
//         │      │ kurzen Ruhephase, dafuer nur noch einmal pro abgeschlossenem Resize        │
// 26-08-17│ 1.30 │ Entprellung WIEDER ENTFERNT (Andreas will eine LIVE aktualisierte Groessen-  │ Cld
//         │      │ anzeige waehrend des Ziehens sehen, nicht nur eine Neuzeichnung am Ende) --  │
//         │      │ stattdessen neue q9_input_read_key_timeout(), Aufrufer entscheidet selbst,   │
//         │      │ wann er auf "Ruhe" wartet (z.B. 1s ohne Aenderung -> Normalanzeige)           │
// 26-08-17│ 1.40 │ Q9_KEY_SHIFT_TAB (CBT, "ESC[Z") -- rueckwaertige Fokus-Navigation fuer den    │ Cld
//         │      │ kommenden Datei-Auswahl-Dialog                                                │
//═════════╧══════╧════════════════════════════════════════════════════════════════════════╧══════
#ifndef Q9_INPUT_H
#define Q9_INPUT_H

typedef enum {
    Q9_KEY_NONE = 0,                                    /* nur intern/Fehlerfall, sollte read_key()
                                                             nie verlassen (das blockiert bis zu einer
                                                             echten Taste) */
    Q9_KEY_CHAR,                                         /* druckbares Zeichen, s. q9_key_t.ch      */
    Q9_KEY_ENTER,                                         /* CR (0x0D) oder LF (0x0A)                */
    Q9_KEY_ESCAPE,                                         /* einzelnes ESC (s. Kopfkommentar)        */
    Q9_KEY_TAB,
    Q9_KEY_SHIFT_TAB,                                       /* CBT, "ESC[Z" -- rueckwaertige Fokus-
                                                             Navigation (z.B. in Dialogen). Windows:
                                                             _getch() liefert dafuer keinen eigenen
                                                             Scan-Code (UNGETESTET, ob/wie es dort
                                                             ankommt -- s. q9_input.c Windows-Zweig) */
    Q9_KEY_BACKSPACE,                                      /* 0x7F (DEL) oder 0x08 (BS) -- beide ueblich
                                                             je nach Terminal/Plattform, gleich behandelt */
    Q9_KEY_UP,
    Q9_KEY_DOWN,
    Q9_KEY_LEFT,
    Q9_KEY_RIGHT,
    Q9_KEY_CTRL_C,                                          /* 0x03 -- Rohmodus deaktiviert ISIG, kommt
                                                             als normales Byte an statt SIGINT auszuloesen */
    Q9_KEY_EOF,                                            /* stdin geschlossen (z.B. Pipe/Skript-Ende) */
    Q9_KEY_UNKNOWN,                                         /* erkannter, aber nicht behandelter Byte-Wert
                                                             (z.B. andere Steuerzeichen) -- wird verworfen,
                                                             read_key() liest automatisch weiter          */
    Q9_KEY_RESIZE                                            /* Terminal-Groesse hat sich geaendert (POSIX:
                                                             SIGWINCH) -- kein echter Tastendruck, aber
                                                             ueber denselben q9_key_t-Kanal gemeldet, damit
                                                             die Hauptschleife es wie jedes andere Ereignis
                                                             behandeln kann (Groesse neu abfragen, Layout
                                                             neu berechnen, komplett neu zeichnen). ch bleibt
                                                             0 -- die neue Groesse selbst kommt separat ueber
                                                             q9_term_size() (nicht in q9_key_t verpackt, um
                                                             die Struktur nicht mit einem Sonderfall-Feld
                                                             aufzublasen, das bei jeder anderen Q9_KEY_*-Art
                                                             ungenutzt waere) */
} q9_key_kind_t;

typedef struct {
    q9_key_kind_t kind;
    char          ch;                                   /* nur gueltig bei Q9_KEY_CHAR                 */
} q9_key_t;

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_input_decode
// Desc.:    REINE Funktion (kein I/O): interpretiert die ERSTEN Bytes von buf (Laenge len) als EINE
//           Taste. *consumed wird auf die Anzahl der dafuer verbrauchten Bytes gesetzt (>=1, nie 0
//           bei len>=1). more_may_follow=0 bedeutet "es kommen garantiert keine weiteren Bytes mehr
//           nach (Timeout abgelaufen oder EOF)" -- nur DANN wird ein einzelnes fuehrendes ESC-Byte
//           als Q9_KEY_ESCAPE gewertet; bei more_may_follow=1 und buf=[0x1b] (len==1, sonst nichts
//           bekannt) liefert die Funktion Q9_KEY_NONE mit *consumed=0 ("noch nicht entscheidbar,
//           bitte mehr Bytes holen") -- der Aufrufer (read_key()) ist dafuer verantwortlich, in
//           diesem Fall mit Timeout nachzulesen. len==0 liefert Q9_KEY_NONE, *consumed=0.
// Call:     int used; q9_key_t k = q9_input_decode("\x1b[A", 3, 1, &used);  // -> Q9_KEY_UP, used=3
//════════════════════════════════════════════════════════════════════════════════════════════════
q9_key_t q9_input_decode(const char *buf, int len, int more_may_follow, int *consumed);

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_input_init / q9_input_shutdown
// Desc.:    init aktiviert den Rohmodus (termios: kein Zeilenpuffer, kein Echo, ISIG aus -- Ctrl-C
//           kommt als Byte, nicht als Signal) und registriert atexit/SIGTERM/SIGHUP-Handler fuer die
//           automatische Wiederherstellung (Muster wie src/hal/posix/hal_posix.c, aber eigener,
//           unabhaengiger Zustand -- s. Kopfkommentar). Rueckgabe 0 = ok, -1 = kein TTY (z.B. stdin
//           umgeleitet) -- Aufrufer sollte dann nicht in die interaktive Schleife gehen. shutdown
//           stellt den urspruenglichen Zustand explizit wieder her (zusaetzlich zum automatischen
//           atexit-Aufruf, fuer den Fall eines geplanten Neustarts im selben Prozess, z.B. nach
//           q9_procspawn_run() -- s. dortiger Kommentar zur Rohmodus-Verschachtelung).
// Call:     if (q9_input_init() != 0) { fprintf(stderr, "kein TTY\n"); return 1; }
//════════════════════════════════════════════════════════════════════════════════════════════════
int  q9_input_init(void);
void q9_input_shutdown(void);

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_input_read_key_timeout
// Desc.:    Wie q9_input_read_key() (s.u.), blockiert aber HOECHSTENS timeout_ms Millisekunden.
//           Laeuft die Zeit ohne Taste/Resize ab, liefert die Funktion Q9_KEY_NONE zurueck --
//           dafuer gedacht, dass der Aufrufer periodisch etwas pruefen kann (z.B. "seit N ms keine
//           weitere Groessenaenderung -> zur normalen Anzeige zurueck", s. Q9FLUX_EDITOR_de.md).
//           timeout_ms < 0 bedeutet unbegrenzt warten -- q9_input_read_key() ist eine duenne Huelle
//           genau dafuer. POSIX: ein waehrend des Wartens eintreffendes SIGWINCH unterbricht das
//           Warten SOFORT (kein Polling), Q9_KEY_RESIZE kommt dadurch ohne zusaetzliche Verzoegerung
//           an -- KEINE Entprellung mehr auf dieser Ebene (fruehere ~150ms-Entprellung entfernt,
//           2026-08-17: Andreas wollte waehrend eines Zieh-Resizes eine LIVE aktualisierte
//           Groessenanzeige sehen, nicht nur eine einzelne Neuzeichnung am Ende -- das braucht die
//           einzelnen, unverzoegerten Zwischenereignisse. Die eigentliche "nicht bei jedem
//           Zwischenschritt neu zeichnen"-Entscheidung liegt jetzt beim AUFRUFER, ueber genau diesen
//           Timeout-Mechanismus). Windows kennt kein SIGWINCH-Aequivalent -- der Timeout-Anteil ist
//           dort per Busy-Poll umgesetzt (s. q9_input.c, UNGETESTET mangels Windows-Host), liefert
//           aber wie q9_input_read_key() nie von sich aus Q9_KEY_RESIZE.
// Call:     q9_key_t k = q9_input_read_key_timeout(1000);   // hoechstens 1 Sekunde warten
//           if (k.kind == Q9_KEY_NONE) { /* 1s Stille */ }
//════════════════════════════════════════════════════════════════════════════════════════════════
q9_key_t q9_input_read_key_timeout(int timeout_ms);

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_input_read_key
// Desc.:    Blockiert unbegrenzt, bis eine vollstaendige Taste ODER ein Q9_KEY_RESIZE-Ereignis
//           vorliegt -- duenne Huelle um q9_input_read_key_timeout(-1) (s.o. fuer Details zum
//           Resize-Verhalten). NICHT automatisiert testbar (echtes stdin-I/O), duenne Huelle um
//           q9_input_decode. Braucht q9_input_init() vorher.
// Call:     q9_key_t k = q9_input_read_key()
//════════════════════════════════════════════════════════════════════════════════════════════════
q9_key_t q9_input_read_key(void);

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_term_size
// Desc.:    Liefert die aktuelle Terminal-Groesse in Zeilen/Spalten. Rueckgabe 0 = ok, -1 = Groesse
//           konnte nicht ermittelt werden (z.B. stdout umgeleitet, kein echtes Terminal) -- rows/cols
//           bleiben dann unveraendert, Aufrufer sollte auf einen sinnvollen Default zurueckfallen
//           (z.B. 24x80, das klassische Standardmass). Braucht KEIN vorheriges q9_input_init().
// Call:     int rows, cols; if (q9_term_size(&rows, &cols) != 0) { rows = 24; cols = 80; }
//════════════════════════════════════════════════════════════════════════════════════════════════
int q9_term_size(int *rows, int *cols);

#endif /* Q9_INPUT_H */
//────────────────────────────────────────────────────────────────────────────────────────────────
// EOF q9_input.h                                                                          Ver. 1.40
//────────────────────────────────────────────────────────────────────────────────────────────────
