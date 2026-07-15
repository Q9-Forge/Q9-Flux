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
- `qe_platform_os9.c`: noch zu implementieren; SCF `_gs_opt`/`_ss_opt`,
  `_gs_rdy`, termcap und normale OS-9-Pfade

## Noch ausstehende API-Bereiche

Dateilesen, sicheres Speichern und portable Formatierung sind noch nicht aus
dem Editor-Kern herausgezogen. Sie werden erst nach der funktionierenden
OS-9-Terminalschicht ergaenzt, damit jede Aenderung einzeln testbar bleibt.

