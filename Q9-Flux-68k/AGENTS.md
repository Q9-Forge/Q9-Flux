# Q9 Arbeitsregeln fuer Codex

Diese Datei bleibt bewusst im Repo-Root -- sie ist die "erste Datei", die
Codex/Claude beim Start automatisch findet. Die eigentlichen Arbeitsdokumente
liegen unter `docs/` (frueher unter `.claude/` -- das Verzeichnis ist jetzt
rein lokales Tool-Settings-Verzeichnis und wird nicht mehr versioniert, s.
`.gitignore`).

Beim Arbeiten in diesem Projekt zuerst lesen:

1. `PROJECT.md` fuer Vision, Architektur und Entscheidungen (aktuell nicht
   vorhanden -- historischer Verweis, s. `docs/PROJECT_VISION_ARCHIV.md`).
2. `docs/ARBEITSPLAN.md` fuer aktuellen Status und naechste freigegebene
   Schritte (deutsche Uebersetzung: `docs/ARBEITSPLAN_de.md`; Volltext-Archiv
   aller erledigten Schritte: `docs/ARBEITSPLAN_ARCHIV.md`).
3. `docs/context.txt` fuer den kompakten letzten Arbeitsstand.
4. Bei Build-, Toolchain- oder Architekturfragen: `docs/HANDBOOK.md` (deutsche
   Uebersetzung: `docs/HANDBUCH_de.md`).

Weitere Dokumente unter `docs/`: `Q9_CURRENT_STATUS.md` (Windows-
Arbeitsstand-Snapshot), `BUGFIX_CF_WRITE.md`/`TEST_CF_WRITE.md` (CF-Write-
Bugfix-Dokumentation).

Arbeitsweise:

- Nur Schritte mit Status `Ready` im Arbeitsplan aktiv bearbeiten, falls der Benutzer nichts anderes sagt.
- Uncommitted und untracked Dateien gehoeren zum aktuellen Arbeitsstand und duerfen nicht verworfen werden.
- Grosse lokale Images und proprietaere OS-9-ROMs/Module nicht ins Repo aufnehmen oder weitergeben.
- Vor groesseren Aenderungen Git-Status pruefen.
- Nach Codeaenderungen passende Builds oder Tests aus `Makefile`/`docs/HANDBOOK.md` verwenden.
# Projektübergreifender Kontext

Vor Arbeiten am Teilprojekt zusätzlich den zentralen Kontext
[Q9-Forge/AI_CONTEXT.md](../AI_CONTEXT.md) beachten. Er enthält die
verbindlichen Namen und die Einordnung von Q9 Forge, Q9 Flux, Q9-OS, QCC und
Parsec.
