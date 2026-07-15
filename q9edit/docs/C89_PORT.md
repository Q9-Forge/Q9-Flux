# Kilo nach ANSI C89 — Portierungsprotokoll

Upstream-Referenz: `vendor/kilo/kilo.c`, Commit siehe
`vendor/kilo/UPSTREAM.md`.

Arbeitskopie: `src/editor/qe.c`.

## Abgeschlossene sprachliche Anpassungen

Der erste strenge Hostbuild fand 17 C89-Verstoesse. Ohne beabsichtigte
Verhaltensaenderung wurden korrigiert:

- Deklarationen an den Anfang ihres Blocks verschoben
- drei Variablendeklarationen aus `for`-Initialisierern herausgezogen
- `unsigned long long` bei der Zeilenpuffergroesse durch `size_t` plus
  Ueberlaufpruefung ersetzt
- Kommentar-Trenner-Felder gross genug fuer das abschliessende Nullbyte
  gemacht; der Upstream deklarierte `"//"` in `char[2]`

Nachweis:

```sh
cc -std=c89 -pedantic -Wall -Wextra -Werror \
  -o /tmp/qe-c89 q9edit/src/editor/qe.c
```

Dieser Befehl laeuft am 2026-07-15 ohne Warnungen und Fehler durch.

## Noch keine OS-9-Portierung

Die Arbeitskopie enthaelt weiterhin Unix-/POSIX-Aufrufe. Der erfolgreiche
C89-Hostbuild bedeutet deshalb noch nicht, dass `qe.c` mit xcc baut.
Insbesondere noch zu kapseln oder zu ersetzen:

- `termios` und Raw-Modus
- `ioctl(TIOCGWINSZ)`
- `getline` und `ssize_t`
- `ftruncate`
- `snprintf` und `vsnprintf`, falls xcc/clib sie nicht bereitstellt
- Unix-Header und POSIX-Feature-Makros

Diese Arbeiten erfolgen hinter einer Plattform-API. Keine OS-9-Sonderfaelle
sollen ungeordnet im Editor-Kern verteilt werden.

