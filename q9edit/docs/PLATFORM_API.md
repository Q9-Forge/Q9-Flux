# q9edit-Plattform-API

Die Plattformgrenze liegt in `src/platform/qe_platform.h`. Der Editor-Kern
unter `src/editor/` darf keine direkten Terminal- oder Betriebssystemdetails
enthalten.

## Terminalfunktionen

```c
int qe_term_enable_raw(int fd);
void qe_term_disable_raw(int fd);
int qe_term_read_key(int fd);
int qe_term_get_size(int ifd, int ofd, int *rows, int *cols);
int qe_term_write(int fd, const char *data, int length);
```

Vertrag:

- `enable_raw` sichert den vorherigen Zustand und schaltet auf ungepufferte
  Einzelzeicheneingabe ohne Echo.
- `disable_raw` ist mehrfach sicher aufrufbar und stellt den Zustand wieder
  her.
- `read_key` gibt normale Bytes unveraendert und Sondertasten als
  `ARROW_*`, `HOME_KEY`, `END_KEY`, `PAGE_*` oder `DEL_KEY` zurueck.
- `get_size` liefert Zeilen und Spalten; OS-9 verwendet zuerst termcap und
  darf auf 80x24 zurueckfallen.
- `write` schreibt den vorbereiteten ANSI-Bildschirmpuffer vollstaendig oder
  meldet einen Fehler.

Die gemeinsamen Tastencodes stehen ebenfalls in `qe_platform.h`, damit keine
Plattform eigene, abweichende Werte erfindet.

## Implementierungen

- `qe_platform_posix.c`: Hostentwicklung mit termios und TIOCGWINSZ
- `qe_platform_os9.c`: SCF `_gs_opt`/`_ss_opt`, `_gs_rdy`, termcap und normale
  OS-9-Pfade; auf Konsole und `/x1` getestet

Die OS-9-Implementierung wartet nach `ESC` begrenzt auf weitere Bytes. Das ist
notwendig, weil `/x1` die drei Bytes einer ANSI-Taste nicht zwingend im selben
Scheduler-Durchlauf sichtbar macht. Das Fenster betraegt derzeit 20 Ticks:
lang genug fuer Port 2000, aber endlich, damit eine einzelne Escape-Taste nicht
dauerhaft blockiert.

Fuer die Groesse sind termcap `co`/`li` nur der Rueckfallwert. Wenn SCF bereits
im Raw-Modus ist, bewegt die OS-9-Implementierung den Cursor per ANSI an den
rechten unteren Rand und liest die Antwort auf `CSI 6 n`. Dadurch folgt `qe`
der wirklichen Fenstergroesse statt dem statischen 80x24-Eintrag.

Verifizierte Tests:

- `test/qetermprobe_os9.exp`: Raw-Modus und Wiederherstellung auf der Konsole
- `test/qetermprobe_x1.exp`: ANSI-Pfeiltasten ueber `/x1` und TCP-Port 2000
- `test/qe_x1.exp`: kompletter Editor, Dateilesen, Vollbild und Ctrl-Q ueber
  `/x1`
- `test/host_smoke.exp`: POSIX-Vollbildaufbau und Ctrl-Q

## Noch ausstehende API-Bereiche

Dateilesen und Speichern verwenden inzwischen portable C89-Streams direkt im
Editor-Kern. `qe_format.c` kapselt die kleine benoetigte Teilmenge der
Formatierung. Noch offen ist eine spaetere absturzsichere Speicherroutine; der
erste Port verwendet bewusst den einfachen `fopen("w")`-/`fwrite`-Weg.
