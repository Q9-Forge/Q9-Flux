//════════════════════════════════════════════════════════════════════════════════════════════════
// File:   vtmode.c                                                                        Ver. 1.00
// Owner:  AF
// Desc.:  Kleines Windows-Host-Werkzeug (NICHT Teil des Q9-Builds): schaltet die Windows-Konsole
//         in den transparenten VT100-Durchreichbetrieb und zeigt danach jeden empfangenen
//         Tastendruck als Hex-Bytes an — damit sieht man direkt, dass z.B. Pfeil-hoch als
//         ESC [ A (1B 5B 41) ankommt, statt von der Konsole verschluckt zu werden.
//
//         Eingeschaltet werden: ENABLE_VIRTUAL_TERMINAL_INPUT (Sondertasten als ESC-Sequenzen
//         in den Eingabestrom) auf stdin, ENABLE_VIRTUAL_TERMINAL_PROCESSING (ESC-Sequenzen
//         der Ausgabe interpretieren) auf stdout; ausgeschaltet: ENABLE_LINE_INPUT +
//         ENABLE_ECHO_INPUT (Zeilenpuffer/lokales Echo — das "Raw-Mode"-Pendant zu termios).
//         Beim Beenden (Taste 'q') wird der urspruengliche Modus wiederhergestellt.
//
//         Hinweis: Der Modus haengt am Konsolen-Puffer, aber cmd/PowerShell setzen den
//         Eingabemodus fuer ihre eigene Zeileneingabe laufend zurueck — als dauerhafter
//         "Einschalter" fuer fremde Programme taugt das daher nur bedingt (s. Diskussion
//         2026-07-05). Der eigentliche Zweck ist Anschauung/Diagnose; Q9 selbst setzt die
//         Flags spaeter selbst in q9_hal_init() (hal_native.c).
//
// Build:  Windows, MinGW:  gcc -std=c99 -Wall -Wextra vtmode.c -o vtmode.exe
//         Windows, MSVC:   cl vtmode.c
//
// Call:   vtmode.exe   (dann Tasten druecken; 'q' beendet und stellt den alten Modus wieder her)
//
// Edition History
//─────────┬──────┬────────────────────────────────────────────────────────────────────────┬──────
// Date    │ Ver. │ Description                                                            │ By
//─────────┼──────┼────────────────────────────────────────────────────────────────────────┼──────
// 26-07-05│ 1.00 │ Erste Version (Diskussion Pfeiltasten im Windows Terminal)              │ CF
//═════════╧══════╧════════════════════════════════════════════════════════════════════════╧══════
#include <windows.h>
#include <stdio.h>

int main(void)
{
    HANDLE hin  = GetStdHandle(STD_INPUT_HANDLE);
    HANDLE hout = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD  in_old, out_old, mode;

    if (!GetConsoleMode(hin, &in_old) || !GetConsoleMode(hout, &out_old)) {
        fprintf(stderr, "vtmode: stdin/stdout ist keine Konsole (umgeleitet?)\n");
        return 1;
    }

    /* Eingabe: VT-Sequenzen an, Zeilenpuffer + lokales Echo aus (Raw-Mode). */
    mode = (in_old | ENABLE_VIRTUAL_TERMINAL_INPUT) & ~(ENABLE_LINE_INPUT | ENABLE_ECHO_INPUT);
    if (!SetConsoleMode(hin, mode)) {
        fprintf(stderr, "vtmode: SetConsoleMode(stdin) fehlgeschlagen (Windows < 10?)\n");
        return 1;
    }

    /* Ausgabe: ESC-Sequenzen interpretieren statt drucken. */
    if (!SetConsoleMode(hout, out_old | ENABLE_VIRTUAL_TERMINAL_PROCESSING)) {
        SetConsoleMode(hin, in_old);
        fprintf(stderr, "vtmode: SetConsoleMode(stdout) fehlgeschlagen (Windows < 10?)\n");
        return 1;
    }

    printf("VT100-Durchreichbetrieb aktiv. Tasten druecken (Pfeiltasten probieren!), 'q' beendet.\n\n");

    for (;;) {
        char  buf[16];
        DWORD got = 0;

        if (!ReadFile(hin, buf, sizeof(buf), &got, NULL) || got == 0) {
            break;
        }

        printf("empfangen (%lu Byte):", (unsigned long)got);
        for (DWORD i = 0; i < got; i++) {
            printf(" %02X", (unsigned char)buf[i]);
        }
        printf("  [");
        for (DWORD i = 0; i < got; i++) {
            unsigned char c = (unsigned char)buf[i];
            printf("%c", (c >= 0x20 && c < 0x7F) ? c : '.');
        }
        printf("]\n");

        if (got == 1 && buf[0] == 'q') {
            break;
        }
    }

    SetConsoleMode(hin, in_old);
    SetConsoleMode(hout, out_old);
    printf("\nAlter Konsolen-Modus wiederhergestellt.\n");
    return 0;
}

//────────────────────────────────────────────────────────────────────────────────────────────────
// EOF vtmode.c                                                                            Ver. 1.00
//────────────────────────────────────────────────────────────────────────────────────────────────
