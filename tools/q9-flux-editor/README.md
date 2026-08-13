# Q9-Flux-Launcher/Config-Editor — Grundstein

Anfang des Nachfolgers fuer `../q9-launcher-prototype/` (Turbo Vision, C++,
2026-08-13 als "eine Vollkatastrophe" verworfen — s.
`../../docs/Q9FLUX_EDITOR_de.md` Abschnitt 0). Bewusst **kein TUI-Framework**
(kein Turbo Vision, kein ncurses, kein FTXUI): rohe ANSI-/VT100-Escape-Codes
direkt in C99, damit Zeilen/Spalten/Farben als Zahlen im Code stehen statt
durch mehrere Indirektionsebenen (TRect, TPalette) zu laufen.

## Stand

Nur `src/q9_ansi.h/.c` (Cursor-Position, RGB-Vorder-/Hintergrund, Clear,
Cursor ein-/ausblenden) + Selbsttest. **Noch kein Dialog-/Bildschirmpuffer-
System, keine Config-Anbindung, kein tatsaechlicher Launcher** — das kommt
erst, wenn der eigentliche Editor angegangen wird (s. offene Punkte in
`../../docs/Q9FLUX_EDITOR_de.md`).

## Bauen und ausfuehren

```sh
cd tools/q9-flux-editor
make test     # Selbsttest -- Byte-Vergleich gegen den ANSI/VT100-Standard,
              # kein Terminal noetig, auch Teil von "make test" im Projekt-Root
make demo     # Sichtpruefung -- drei farbige Zeilen an festen Positionen,
              # ECHTES Terminal noetig (fuer Andreas: dient als Grundlage,
              # um Verschiebe-/Farbanweisungen wie "3 Zeilen tiefer, 4
              # Zeichen nach links" konkret nachzupruefen)
```
