//════════════════════════════════════════════════════════════════════════════════════════════════
// File:   input_selftest.c                                                                Ver. 1.10
// Owner:  Claudia
// Desc.:  Automatischer Nachweis fuer q9_input_decode() -- die reine, plattformunabhaengige
//         Tastenerkennung (Byte-Puffer rein, q9_key_t raus). Deckt normale Zeichen, Sondertasten,
//         Pfeiltasten-Sequenzen UND die ESC-Mehrdeutigkeit (lone ESC vs. Sequenzbeginn, mit/ohne
//         more_may_follow) ab -- genau der Teil, der ohne echtes Terminal testbar ist.
//         q9_input_read_key() selbst (echtes stdin-I/O) ist NICHT Teil dieses Tests, s. Kopf-
//         kommentar in q9_input.h.
//
// Call:   build/input_selftest
//════════════════════════════════════════════════════════════════════════════════════════════════
#include <stdio.h>

#include "../src/q9_input.h"

static int g_fails = 0;

static void check_kind(const char *label, q9_key_kind_t got, q9_key_kind_t want)
{
    if (got == want) {
        printf("    OK   %s\n", label);
    } else {
        printf("    FAIL %s -- erwartet Kind %d, bekommen %d\n", label, (int)want, (int)got);
        g_fails++;
    }
}

static void check_int(const char *label, int got, int want)
{
    if (got == want) {
        printf("    OK   %s (%d)\n", label, got);
    } else {
        printf("    FAIL %s -- erwartet %d, bekommen %d\n", label, want, got);
        g_fails++;
    }
}

int main(void)
{
    int consumed;
    q9_key_t k;

    printf("=== q9_input_decode: normale druckbare Zeichen ===\n");
    k = q9_input_decode("x", 1, 0, &consumed);
    check_kind("'x' -> Q9_KEY_CHAR", k.kind, Q9_KEY_CHAR);
    check_int("'x' -> ch == 'x'", k.ch, 'x');
    check_int("'x' -> consumed == 1", consumed, 1);

    k = q9_input_decode("!", 1, 0, &consumed);
    check_kind("'!' -> Q9_KEY_CHAR", k.kind, Q9_KEY_CHAR);
    check_int("'!' -> ch == '!'", k.ch, '!');

    printf("=== q9_input_decode: Sondertasten (Enter/Tab/Backspace/Strg-C) ===\n");
    k = q9_input_decode("\r", 1, 0, &consumed);
    check_kind("CR -> Q9_KEY_ENTER", k.kind, Q9_KEY_ENTER);
    k = q9_input_decode("\n", 1, 0, &consumed);
    check_kind("LF -> Q9_KEY_ENTER", k.kind, Q9_KEY_ENTER);
    k = q9_input_decode("\t", 1, 0, &consumed);
    check_kind("TAB -> Q9_KEY_TAB", k.kind, Q9_KEY_TAB);
    k = q9_input_decode("\x7f", 1, 0, &consumed);
    check_kind("DEL (0x7F) -> Q9_KEY_BACKSPACE", k.kind, Q9_KEY_BACKSPACE);
    k = q9_input_decode("\x08", 1, 0, &consumed);
    check_kind("BS (0x08) -> Q9_KEY_BACKSPACE", k.kind, Q9_KEY_BACKSPACE);
    k = q9_input_decode("\x03", 1, 0, &consumed);
    check_kind("Ctrl-C (0x03) -> Q9_KEY_CTRL_C", k.kind, Q9_KEY_CTRL_C);

    printf("=== q9_input_decode: Pfeiltasten-Sequenzen (vollstaendig, more_may_follow egal) ===\n");
    k = q9_input_decode("\x1b[A", 3, 0, &consumed);
    check_kind("ESC [ A -> Q9_KEY_UP", k.kind, Q9_KEY_UP);
    check_int("ESC [ A -> consumed == 3", consumed, 3);
    k = q9_input_decode("\x1b[B", 3, 0, &consumed);
    check_kind("ESC [ B -> Q9_KEY_DOWN", k.kind, Q9_KEY_DOWN);
    k = q9_input_decode("\x1b[C", 3, 0, &consumed);
    check_kind("ESC [ C -> Q9_KEY_RIGHT", k.kind, Q9_KEY_RIGHT);
    k = q9_input_decode("\x1b[D", 3, 0, &consumed);
    check_kind("ESC [ D -> Q9_KEY_LEFT", k.kind, Q9_KEY_LEFT);
    k = q9_input_decode("\x1b[A", 3, 1, &consumed);
    check_kind("ESC [ A mit more_may_follow=1 -> trotzdem Q9_KEY_UP (Sequenz ist vollstaendig)",
               k.kind, Q9_KEY_UP);
    k = q9_input_decode("\x1b[Z", 3, 0, &consumed);
    check_kind("ESC [ Z -> Q9_KEY_SHIFT_TAB", k.kind, Q9_KEY_SHIFT_TAB);
    check_int("ESC [ Z -> consumed == 3", consumed, 3);

    printf("=== q9_input_decode: unbekannte Sequenz (ESC [ x) ===\n");
    k = q9_input_decode("\x1b[x", 3, 0, &consumed);
    check_kind("ESC [ x -> Q9_KEY_UNKNOWN", k.kind, Q9_KEY_UNKNOWN);
    check_int("ESC [ x -> trotzdem 3 Bytes verbraucht (Sequenz erkannt, nur der Buchstabe unbekannt)",
              consumed, 3);

    printf("=== q9_input_decode: ESC-Mehrdeutigkeit -- Kernstueck des Moduls ===\n");
    /* Einzelnes ESC, more_may_follow=1 (koennte noch der Sequenzbeginn sein) -> "noch warten". */
    k = q9_input_decode("\x1b", 1, 1, &consumed);
    check_kind("ESC allein, more_may_follow=1 -> Q9_KEY_NONE (noch nicht entscheidbar)",
               k.kind, Q9_KEY_NONE);
    check_int("ESC allein, more_may_follow=1 -> consumed == 0 (nichts verbraucht)", consumed, 0);

    /* Einzelnes ESC, more_may_follow=0 (Timeout ist abgelaufen, es kommt sicher nichts mehr) ->
       jetzt STEHT fest: war ein einzelnes Escape. */
    k = q9_input_decode("\x1b", 1, 0, &consumed);
    check_kind("ESC allein, more_may_follow=0 (Timeout) -> Q9_KEY_ESCAPE", k.kind, Q9_KEY_ESCAPE);
    check_int("ESC allein, more_may_follow=0 -> consumed == 1", consumed, 1);

    /* "ESC [" (2 Bytes), more_may_follow=1 -> immer noch "koennte eine Pfeiltaste werden". */
    k = q9_input_decode("\x1b[", 2, 1, &consumed);
    check_kind("ESC [ (2 Bytes), more_may_follow=1 -> Q9_KEY_NONE (noch nicht entscheidbar)",
               k.kind, Q9_KEY_NONE);
    check_int("ESC [ (2 Bytes), more_may_follow=1 -> consumed == 0", consumed, 0);

    /* "ESC [" (2 Bytes), more_may_follow=0 -> Timeout ist abgelaufen, Sequenz bleibt unvollstaendig. */
    k = q9_input_decode("\x1b[", 2, 0, &consumed);
    check_kind("ESC [ (2 Bytes), more_may_follow=0 -> Q9_KEY_UNKNOWN (abgebrochene Sequenz)",
               k.kind, Q9_KEY_UNKNOWN);
    check_int("ESC [ (2 Bytes), more_may_follow=0 -> beide Bytes verbraucht", consumed, 2);

    /* ESC gefolgt von einem NICHT-'[' Zeichen (z.B. Alt+x auf manchen Terminals) -- gilt SOFORT als
       lone ESC, unabhaengig von more_may_follow (kein Sequenzbeginn moeglich). */
    k = q9_input_decode("\x1bx", 2, 1, &consumed);
    check_kind("ESC gefolgt von 'x' (kein '[') -> Q9_KEY_ESCAPE, sofort entscheidbar",
               k.kind, Q9_KEY_ESCAPE);
    check_int("ESC gefolgt von 'x' -> NUR das ESC verbraucht (1), 'x' bleibt fuer den naechsten Aufruf",
              consumed, 1);

    printf("=== q9_input_decode: Randfaelle (leer, sonstige Steuerzeichen) ===\n");
    k = q9_input_decode("", 0, 0, &consumed);
    check_kind("leerer Puffer -> Q9_KEY_NONE", k.kind, Q9_KEY_NONE);
    check_int("leerer Puffer -> consumed == 0", consumed, 0);

    k = q9_input_decode("\x01", 1, 0, &consumed);
    check_kind("sonstiges Steuerzeichen (Ctrl-A) -> Q9_KEY_UNKNOWN", k.kind, Q9_KEY_UNKNOWN);
    check_int("sonstiges Steuerzeichen -> trotzdem 1 Byte verbraucht (kein Haengenbleiben)",
              consumed, 1);

    k = q9_input_decode(NULL, 5, 0, &consumed);
    check_kind("NULL-Puffer -> Q9_KEY_NONE, kein Crash", k.kind, Q9_KEY_NONE);

    printf("\n=== Zusammenfassung ===\n");
    printf("  Gesamt: %d Checks fehlgeschlagen\n", g_fails);
    return g_fails ? 1 : 0;
}

//────────────────────────────────────────────────────────────────────────────────────────────────
// EOF input_selftest.c                                                                    Ver. 1.10
//────────────────────────────────────────────────────────────────────────────────────────────────
