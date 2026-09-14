# Autonomer Betrieb — Claudia arbeitet den Arbeitsplan selbstständig ab

Ziel: Eine geplante Aufgabe (Claude Code Desktop, "Scheduled Task") prüft stündlich
den ARBEITSPLAN und arbeitet **alle 🟢-Ready-Schritte nacheinander** ab —
jeden komplett (implementieren, testen, ✅, commit+push), bevor der nächste
beginnt. Die Steuerung liegt komplett beim Statusmodell: **nur was Andreas auf
🟢 Ready stellt, wird angefasst.** Nichts Ready = Lauf beendet sich sofort
(minimaler Verbrauch). Drosseln geht also jederzeit über die Zahl der
freigegebenen Ready-Punkte. (Bis 2026-07-03 galt „genau ein Schritt pro
Lauf" — auf Andreas' Wunsch gelockert, weil kleine Schritte sonst unnötig
je eine Stunde warten.)

## Bausteine

1. **`.claude/settings.json`** (rein lokal, `.claude/` ist nicht mehr Teil des
   Repos -- auf jedem Rechner einmalig selbst anlegen):
   erlaubt git/make/gcc/clang/emcc/python sowie Lesen/Schreiben im Projekt ohne
   Berechtigungsdialog. `git push --force` bleibt verboten. Das ist der robuste
   Baustein: Muster wie `Bash(git *)` oder `Edit(./**)` decken *jeden* künftigen
   Ready-Schritt ab, unabhängig von seinem Inhalt — nicht nur den einen Lauf,
   bei dem sie erteilt wurden.
2. **Geplante Aufgabe** ("Routinen" in der deutschen UI) in der Claude-Code-
   Desktop-App (pro Rechner einmal anlegen, Prompt siehe unten). Die App muss
   laufen; verpasste Läufe werden beim nächsten Start nachgeholt.
3. **Einmal "Run now"/"Jetzt ausführen"** nach dem Anlegen: dabei erteilte
   Genehmigungen werden an der Aufgabe gespeichert — das deckt aber **nur
   das ab, was in genau diesem einen Lauf tatsächlich gebraucht wurde**, nicht
   pauschal alles Künftige. Der eigentliche Schutz vor Ort-1-Lücken ist die
   Allowlist in settings.json; "Run now" ist nur die Ergänzung für alles, was
   nicht über ein Muster in settings.json abgedeckt ist (z.B. Zugriff auf
   Pfade außerhalb des Projektordners wie `M:\REF`, siehe unten).

**Bekannte Lücke:** Alles außerhalb der gelisteten Muster bzw. außerhalb des
Projektordners löst weiterhin eine Rückfrage aus — bei einem unbeaufsichtigten
Lauf bleibt das dann vermutlich hängen. `M:\REF` (Referenz für den
REF-Abgleich) ist deshalb explizit in `additionalDirectories` +
`Read(M:/REF/**)` aufgenommen. Fällt künftig ein weiterer Pfad/Befehl auf,
den ein Ready-Schritt braucht: hier ergänzen statt auf "Run now" verlassen.

## Einrichtung auf einem neuen Rechner (z.B. Mac Mini)

1. Repo clonen: `git clone https://github.com/foellmy51/Q9.git` (gh auth nötig)
2. Toolchain: macOS = Xcode Command Line Tools (`xcode-select --install`, liefert
   clang + make) + Python 3. Windows = w64devkit (siehe docs/TOOLCHAIN.md).
3. **Achtung macOS/Linux:** Bauen/Testen geht erst, wenn die POSIX-HAL existiert
   (Schritt 1.10 im ARBEITSPLAN) — die native HAL ist bis dahin Windows-only
   (conio.h). Schritt 1.10 idealerweise direkt auf dem Mac umsetzen und testen.
4. In der Claude-Code-App eine geplante Aufgabe anlegen (stündlich, `0 * * * *`)
   mit dem Prompt unten — oder einfach Claudia sagen:
   *"Lies docs/AUTONOMIE.md im Q9-Repo und richte die geplante Aufgabe ein."*
5. Einmal "Run now" klicken und die Berechtigungsdialoge durchwinken.

## Prompt für die geplante Aufgabe

```
Du bist Claudia und arbeitest autonom am Q9-Projekt (modulares Mini-OS in
OS-9-Tradition) von Andreas.

1. Wechsle ins Q9-Repo (Windows: D:\projekts\Q9; macOS: ~/projects/Q9 bzw. wo
   es geclont ist) und hole den aktuellen Stand: git pull.
2. Lies ARBEITSPLAN.md. Statusmodell: 💡 Vorschlag · 💤 Idle · 🟢 Ready ·
   🔄 in Arbeit · ✅ fertig · ⛔ blockiert. Die Spalte "Wer" entscheidet, WER
   den Schritt bearbeitet: "Claudia" = du direkt im Hauptrepo; "Codex" = du
   delegierst an die Codex-CLI in einer isolierten Baustelle (Abschnitt
   "Codex-Delegation" unten).
3. Beende OHNE Änderungen, wenn: weder ein 🟢-Ready-Schritt mit Wer=Claudia
   NOCH einer mit Wer=Codex existiert, ODER bereits ein Schritt auf 🔄 steht,
   ODER git pull fehlschlägt.
4. Sonst: arbeite zuerst ALLE 🟢-Ready-Schritte mit Wer=Claudia NACHEINANDER
   ab (niedrigste Nummer zuerst, wie bisher). Arbeite DANACH alle 🟢-Ready-
   Schritte mit Wer=Codex ab (Codex-Delegation unten).

Regeln für Wer=Claudia-Schritte: 💡/💤 niemals anfassen. Bei Blockade eines
Schritts: ⛔ setzen, unter "Geparkt" dokumentieren, committen — dann mit dem
nächsten unabhängigen Ready-Schritt weitermachen, sofern sinnvoll, sonst
beenden. Commit-Stil: "Release X.YZ: <Titel> (<Nr>)", Deutsch, ASCII im Body.
Code-Stil exakt wie im Bestand (Box-Header, Edition History mit CF, portables
C99, kein malloc im Kernel); Versionsnummern pflegen. Doku (docs/SYSCALLS.md,
docs/DEVICES.md, docs/MODULES.md, docs/SYSCALL_ROADMAP.md je nachdem was
betroffen ist) mitziehen, Selbsttest + test/-Skripte erweitern.

## Codex-Delegation (Wer=Codex-Schritte)

Isolationsprinzip (wie bisher von Claudia manuell praktiziert, siehe
ARBEITSPLAN.md Phase U): Codex arbeitet NIE direkt im Hauptrepo, committet/
pusht NIE selbst nach `main`, rührt NIE etwas außerhalb `userland/` an.
Für jeden 🟢-Ready-Schritt mit Wer=Codex:

1. Codex-Binary finden (Pfad ändert sich bei App-Updates — nicht fest
   verdrahten): zuerst `ls -td ~/.codex/packages/standalone/releases/*/bin/codex
   2>/dev/null | head -1` versuchen, sonst `/Applications/Codex.app/Contents/Resources/codex`
   als Fallback. Wenn keins davon existiert: Schritt NICHT bearbeiten, im
   ARBEITSPLAN unter "Geparkt" vermerken ("Codex-Binary nicht gefunden"),
   mit anderen Ready-Schritten weitermachen.
2. Isoliertes Worktree anlegen: `git worktree add ../Q9-codex-<kurzer-slug>
   -b codex-<kurzer-slug> origin/main` (Slug aus dem Schritt ableiten, z.B.
   Schrittnummer + Kurzname).
3. Auftragstext für `codex exec` zusammenstellen: die Schrittbeschreibung aus
   dem ARBEITSPLAN als eigentliche Aufgabe, EINGERAHMT von diesem Standard-
   Vorspann (immer mitschicken, unabhängig vom Inhalt):
   - "Arbeitsbereich STRIKT auf userland/ begrenzt, keine anderen Pfade
     anfassen. Kein git commit/push."
   - "Lies zuerst userland/lib/libq9.{h,c} und die bestehenden userland/
     tools/*.{h,c}, um Stil und Konventionen zu übernehmen."
   - "Code-Stil: Box-Header + Edition-History (By = \"CX\"), C99,
     -Wall -Wextra warnungsfrei, deutsche Kommentare."
   - "Nach jeder Änderung userland/build.sh laufen lassen, sicherstellen
     dass es weiterhin \"userland_test: PASS\" meldet."
4. Aufruf: `"$CODEX" exec --sandbox workspace-write -C <worktree-pfad>
   --skip-git-repo-check -m gpt-5.5 - < <promptdatei>` im Hintergrund, auf
   Beendigung warten (kein festes Zeitlimit — Codex-Läufe können mehrere
   Minuten dauern).
5. UNABHÄNGIG VERIFIZIEREN (nicht Codex' eigenem Abschlussbericht vertrauen):
   `rm -rf build && ./userland/build.sh` im Worktree neu ausführen, PASS +
   warnungsfrei prüfen; `git status`/`git diff --stat` im Worktree prüfen,
   dass NUR Dateien unter `userland/` geändert wurden. Bei Verstoß NICHT
   committen, Schritt im ARBEITSPLAN unter "Geparkt" dokumentieren, Worktree
   trotzdem aufräumen (Schritt 8), mit anderen Ready-Schritten weitermachen.
6. Bei erfolgreicher Verifikation: im Worktree `git add userland && git commit`
   (Commit-Stil wie gehabt, "By: Codex, GPT-5.5" im Body erwähnen), dann
   `git push -u origin codex-<kurzer-slug>`.
7. Pull Request versuchen: `gh pr create --title ... --body ...` — falls das
   fehlschlägt (gh war in dieser Umgebung bisher nicht zuverlässig
   authentifiziert, KEINE Zeit mit `gh auth login` verschwenden), einfach die
   von `git push` ausgegebene Compare-URL
   (`https://github.com/foellmy51/Q9/pull/new/codex-<slug>`) in die
   ARBEITSPLAN-Notiz des Schritts schreiben, mit dem Hinweis "PR muss Andreas
   manuell über diesen Link anlegen".
8. Worktree aufräumen: `git worktree remove ../Q9-codex-<slug>` im
   Haupt-Repo, `git branch -d codex-<slug>` falls lokal noch vorhanden.
9. ARBEITSPLAN.md aktualisieren: Schritt-Status auf ✅ (wenn PR erstellt)
   oder Notiz "PR-Link bereit, wartet auf Andreas" (Status bleibt 🟢, aber
   mit Vermerk "nicht erneut bearbeiten"). git commit + push wie gehabt für
   diese ARBEITSPLAN-Änderung selbst (normaler Claudia-Commit, kein
   Codex-Commit).
10. NIEMALS selbst nach `main` mergen — das bleibt immer Andreas' Entscheidung,
    auch wenn ein PR erfolgreich erstellt wurde.

**docs/HANDBOOK.md (+ docs/HANDBUCH_de.md) mitpflegen**: das
öffentlichkeitstaugliche Gesamt-Handbuch (Werkzeuge, Quellcode-Layout, Build
je Target, Architektur, Lizenzlage, Glossar) — im Gegensatz zu
ARBEITSPLAN.md/diesem Dokument, die intern bleiben. Seit 2026-08-12 zwei
Dateien: `HANDBOOK.md` (Englisch, Original) + `HANDBUCH_de.md` (Deutsch,
Übersetzung) — bei einer Änderung mindestens die deutsche Fassung
aktualisieren und einen Übersetzungs-Nachtrag für `HANDBOOK.md` in der
Notiz vermerken, falls keine Zeit für beide bleibt. Nach jedem
abgeschlossenen Ready-Schritt (auch Codex-delegierten) prüfen, ob das
Handbuch betroffen ist (neue Architekturentscheidung, neue Kernel-
Komponente, neues Werkzeug, neue externe Referenz samt Lizenz, neuer
Fachbegriff, Phase komplett abgeschlossen) und im selben Commit aktualisieren
(bei Codex-Schritten: im Claudia-Commit, der die ARBEITSPLAN-Aktualisierung
macht). Bei rein interner Statuspflege ohne Architekturrelevanz: keine
Handbuch-Änderung, nicht künstlich aufblähen. Bleibt Markdown — PDF-Export
ist ein gezielter, separater Schritt am Ende, nicht Teil der Routine.

Build & Test: make test muss PASS sein, Build warnungsfrei.
- Windows: vorher $env:PATH = "C:\Users\AF\w64devkit\bin;$env:PATH"
- macOS: clang/make aus den Xcode CLT
- Linux: gcc/make aus dem Distributions-Paket (z.B. `build-essential`)
- kein separates wasm-Target mehr (der Mini-Kernel inkl. wasm3-Runtime wurde
  am 2026-07-31 nach Q9RESUME-Kernel ausgelagert) — `make native` deckt alle
  drei Plattformen ab, `make test` reicht als Verifikation.

Fasse am Ende in 2–3 Sätzen zusammen, was getan wurde (oder warum nichts) —
bei Codex-Delegation explizit erwähnen, ob ein PR erstellt wurde oder Andreas
den Compare-Link noch selbst öffnen muss.
```

## Betriebsregeln

- **Nur EIN Rechner** sollte die Aufgabe aktiv laufen haben (sonst Push-Konflikte;
  das 🔄-Kriterium schützt nur teilweise). Beim Umzug auf den Mac Mini die
  Aufgabe auf dem Windows-PC pausieren/löschen.
- Läufe verbrauchen die normalen Abo-Limits (5h-Fenster/Woche). Drosseln =
  weniger Ready-Punkte freigeben oder Zeitplan strecken (z.B. alle 2h).
- **Codex-Delegation** (Wer=Codex-Schritte, seit 2026-07-04) verbraucht
  zusätzlich Andreas' ChatGPT-Kontingent — Codex läuft NUR, wenn ein
  entsprechender Schritt auf 🟢 steht, nicht auf Verdacht. Codex committet/
  merged nie selbst; der PR-Merge bleibt immer bei Andreas (Ausnahme: Fällt
  `gh pr create` aus, muss Andreas den PR sogar selbst über den Compare-Link
  anlegen — bekannte Einschränkung dieser Sandbox-Umgebung, siehe Schritt 7
  im Prompt oben).
- **GitHub Copilot CLI** (`copilot`, seit 2026-07-05, noch KEIN fester
  Delegations-Track wie Codex — bisher nur manuell von Andreas ausprobiert,
  s. Chat-Verlauf) läuft bei Andreas auf dem alten Legacy-Multiplikator-
  Abrechnungsmodell, nicht auf Token-Preisen. Andreas' reale Werte
  (`/model` in der interaktiven CLI, 2026-07-05): **Claude Haiku 4.5 = 0,33×,
  Claude Sonnet 4.5 = 6×, Claude Opus 4.5 = 15×, GPT-5 mini = 0,33×,
  GPT-4.1 = 0× (kostenlos)** — Sonnet verbraucht damit **~18× so viel**
  Kontingent wie Haiku pro Prompt. Faustregel, falls/wenn Copilot als
  zweiter Delegations-Track aufgesetzt wird: **Haiku als Standard**
  (mechanische Arbeit: Boilerplate, Tests nach klarem Muster, Doku-Updates),
  **Sonnet nur gezielt** für Aufgaben mit echter architektonischer Tiefe
  (Analogon zu den harten Musashi-MMU/FPU-Debugging-Sessions), **GPT-4.1**
  als kostenloser Fallback für Wegwerf-Experimente/schnelle Nachfragen.
  Nicht-interaktiver Aufruf: `copilot -p "<prompt>" --model claude-haiku-4.5
  --allow-all-tools --add-dir <verzeichnis>` (Modell explizit setzen, sonst
  greift die zuletzt in der interaktiven Sitzung gewählte Vorgabe).

**Erstellt**: 2026-07-03
