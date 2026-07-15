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

## OS-9-Portierungsstand

Der komplette Editor baut inzwischen mit xcc als `os9/CMDS/qe`. Ersetzt oder
gekapselt wurden:

- `termios` und `ioctl` hinter `qe_platform_*`
- `getline` durch einen dynamisch wachsenden C89-Zeilenleser
- `ftruncate`/POSIX-Deskriptorzugriff durch `fopen`/`fwrite`
- `snprintf`/`vsnprintf` durch den begrenzten, puffersicheren Formatter
  `qe_format.c` (`%s`, `%.Ns`, `%d` und `%%`)
- `SIGWINCH` nur noch im POSIX-Hostbuild; OS-9 nutzt termcap `co`/`li`
- Unix-spezifische Header aus dem gemeinsamen Editor-Kern entfernt

Noch nicht abgeschlossen ist der Lauf- und Speichertest des echten `qe` im
Gast. Der einfache `fopen("w")`-Speicherweg ist portabel, aber noch nicht als
absturzsichere/atomare Speicherroutine zu verstehen.

Auf der emulierten seriellen Konsole ist Kilos kompletter Neuaufbau nach jeder
Taste zu langsam. Funktional werden Zeichen korrekt gelesen und eingefuegt;
vor weiterer Bedienungsarbeit muss die Ausgabe auf geaenderte Zeilen und
Statusbereiche begrenzt werden.

Der erste Differenzpfad steht jetzt: Zeicheneingabe aktualisiert eine Zeile,
normale Cursorbewegung nur die ANSI-Cursorposition und Ctrl-Q nur die
Meldungszeile. Syntaxfarben werden auf einer gerade editierten Zeile vorerst
erst beim naechsten Vollaufbau wiederhergestellt; schnelle Grundbedienung hat
hier bewusst Vorrang.

Der Vollaufbau positioniert jede Bildschirmzeile absolut (`CSI row;1 H`). Das
vermeidet den doppelten Vorschub aus automatischem Umbruch an Spalte 80 plus
explizitem CR/LF, der Status/Hilfe nach oben scrollen liess.
