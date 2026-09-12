# Q9-Flux-Launcher/Config-Editor — Grundstein

Anfang des Nachfolgers fuer `../q9-launcher-prototype/` (Turbo Vision, C++,
2026-08-13 als "eine Vollkatastrophe" verworfen — s.
`../../docs/Q9FLUX_EDITOR_de.md` Abschnitt 0). Bewusst **kein TUI-Framework**
(kein Turbo Vision, kein ncurses, kein FTXUI): rohe ANSI-/VT100-Escape-Codes
direkt in C99, damit Zeilen/Spalten/Farben als Zahlen im Code stehen statt
durch mehrere Indirektionsebenen (TRect, TPalette) zu laufen.

## Stand (2026-08-17)

Sechs unabhaengige, jeweils fuer sich getestete Bausteine:

| Modul | Zweck |
|---|---|
| `src/q9_ansi.h/.c` | Rohe ANSI-/VT100-Escape-Codes (Cursor, Farbe, Clear) |
| `src/q9_screenbuf.h/.c` | Bildschirmpuffer, `snapshot()`/`restore()` fuer modale Dialoge |
| `src/q9_widgets.h/.c` | ASCII-Rahmen mit Titel |
| `src/q9_procspawn.h/.c` | Emulator als Kindprozess starten (fuer den "Start"-Button) |
| `src/q9_input.h/.c` | Tastatur-Rohmodus, Pfeiltasten/Enter/Escape/Strg-C, Terminal-Resize |
| `src/q9_listview.h/.c` | Scrollbare Listenansicht inkl. Scrollbalken |

**Noch KEIN echtes, zusammengesetztes Programm** (keine Config-Anbindung,
keine Hardware-Typen, keine echten Buttons/Textfelder) — das kommt erst,
wenn der eigentliche Editor angegangen wird. `demo/integration_demo.c` zeigt
aber bereits alle sechs Bausteine zusammen in einem echten, bedienbaren
Bildschirm (s.u.). Details/offene Punkte: `../../docs/Q9FLUX_EDITOR_de.md`.

## Bauen und ausfuehren

```sh
cd tools/q9-flux-editor
make test               # alle Selbsttests -- kein Terminal noetig, auch
                         # Teil von "make test" im Projekt-Root
make demo                # rohe ANSI-Positionsprobe -- drei farbige Zeilen
                         # an festen Positionen, ECHTES Terminal noetig
make demo-integration   # alle sechs Bausteine zusammen: Rahmen mit Titel,
                         # scrollbare Geraeteliste, Pfeiltasten-Navigation,
                         # reagiert auf Terminal-Groessenaenderungen.
                         # Pfeiltasten: Auswahl bewegen. Strg-C: beenden.
                         # ECHTES Terminal noetig, KEIN echter Editor (nur
                         # Sichtpruefung, dass die Bausteine zusammenpassen)
```
