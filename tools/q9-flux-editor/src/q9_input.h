//════════════════════════════════════════════════════════════════════════════════════════════════
// File:   q9_input.h                                                                      Ver. 1.00
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
    Q9_KEY_BACKSPACE,                                      /* 0x7F (DEL) oder 0x08 (BS) -- beide ueblich
                                                             je nach Terminal/Plattform, gleich behandelt */
    Q9_KEY_UP,
    Q9_KEY_DOWN,
    Q9_KEY_LEFT,
    Q9_KEY_RIGHT,
    Q9_KEY_CTRL_C,                                          /* 0x03 -- Rohmodus deaktiviert ISIG, kommt
                                                             als normales Byte an statt SIGINT auszuloesen */
    Q9_KEY_EOF,                                            /* stdin geschlossen (z.B. Pipe/Skript-Ende) */
    Q9_KEY_UNKNOWN                                          /* erkannter, aber nicht behandelter Byte-Wert
                                                             (z.B. andere Steuerzeichen) -- wird verworfen,
                                                             read_key() liest automatisch weiter          */
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
// Function: q9_input_read_key
// Desc.:    Blockiert, bis eine vollstaendige Taste vorliegt (echtes stdin-I/O -- NICHT
//           automatisiert testbar, duenne Huelle um q9_input_decode). Braucht q9_input_init() vorher.
// Call:     q9_key_t k = q9_input_read_key()
//════════════════════════════════════════════════════════════════════════════════════════════════
q9_key_t q9_input_read_key(void);

#endif /* Q9_INPUT_H */
//────────────────────────────────────────────────────────────────────────────────────────────────
// EOF q9_input.h                                                                          Ver. 1.00
//────────────────────────────────────────────────────────────────────────────────────────────────
