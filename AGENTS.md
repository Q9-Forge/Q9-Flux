# Q9 Arbeitsregeln fuer Codex

Beim Arbeiten in diesem Projekt zuerst lesen:

1. `PROJECT.md` fuer Vision, Architektur und Entscheidungen.
2. `ARBEITSPLAN.md` fuer aktuellen Status und naechste freigegebene Schritte.
3. `context.txt` fuer den kompakten letzten Arbeitsstand.
4. Bei Build-, Toolchain- oder Architekturfragen: `docs/HANDBUCH.md`.

Arbeitsweise:

- Nur Schritte mit Status `Ready` im Arbeitsplan aktiv bearbeiten, falls der Benutzer nichts anderes sagt.
- Uncommitted und untracked Dateien gehoeren zum aktuellen Arbeitsstand und duerfen nicht verworfen werden.
- Grosse lokale Images und proprietaere OS-9-ROMs/Module nicht ins Repo aufnehmen oder weitergeben.
- Vor groesseren Aenderungen Git-Status pruefen.
- Nach Codeaenderungen passende Builds oder Tests aus `Makefile`/`docs/HANDBUCH.md` verwenden.
# Projektübergreifender Kontext

Vor Arbeiten am Teilprojekt zusätzlich den zentralen Kontext
[Q9Forge/AI_CONTEXT.md](../Q9Forge/AI_CONTEXT.md) beachten. Er enthält die
verbindlichen Namen und die Einordnung von Q9 Forge, Q9 Flux, Q9-OS, QCC und
Parsec.
