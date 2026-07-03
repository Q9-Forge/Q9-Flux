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

1. **`.claude/settings.json`** (in diesem Repo, wandert per git auf jeden Rechner):
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
   Pfade außerhalb des Projektordners wie `M:\MWOS`, siehe unten).

**Bekannte Lücke:** Alles außerhalb der gelisteten Muster bzw. außerhalb des
Projektordners löst weiterhin eine Rückfrage aus — bei einem unbeaufsichtigten
Lauf bleibt das dann vermutlich hängen. `M:\MWOS` (Referenz für den
MWOS-Abgleich) ist deshalb explizit in `additionalDirectories` +
`Read(M:/MWOS/**)` aufgenommen. Fällt künftig ein weiterer Pfad/Befehl auf,
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
   🔄 in Arbeit · ✅ fertig · ⛔ blockiert.
3. Beende OHNE Änderungen, wenn: kein 🟢-Ready-Schritt mit Wer=Claudia
   existiert, ODER bereits ein Schritt auf 🔄 steht, ODER git pull fehlschlägt.
4. Sonst: arbeite die 🟢-Ready-Schritte NACHEINANDER ab (niedrigste Nummer
   zuerst). Immer nur EIN Schritt gleichzeitig in Arbeit: jeden Schritt
   komplett abschließen (implementieren, testen, Status ✅ + Notiz im
   ARBEITSPLAN, git commit + git push), bevor der nächste beginnt. Erst
   aufhören, wenn kein Ready-Schritt mehr übrig oder einer blockiert ist.

Regeln: 💡/💤 niemals anfassen. Bei Blockade eines Schritts: ⛔ setzen, unter
"Geparkt" dokumentieren, committen — dann mit dem nächsten unabhängigen
Ready-Schritt weitermachen, sofern sinnvoll, sonst beenden. Commit-Stil:
"Release X.YZ: <Titel> (<Nr>)", Deutsch, ASCII im Body. Code-Stil exakt wie
im Bestand (Box-Header, Edition History mit CF, portables C99, kein malloc im
Kernel); Versionsnummern pflegen. Doku (docs/SYSCALLS.md, docs/DEVICES.md)
mitziehen, Selbsttest + test/-Skripte erweitern.

Build & Test: make test muss PASS sein, Build warnungsfrei.
- Windows: vorher $env:PATH = "C:\Users\AF\w64devkit\bin;$env:PATH"
- macOS: clang/make aus den Xcode CLT; erst möglich ab POSIX-HAL (1.10)
- make wasm nur, wo emsdk installiert ist — sonst Code schreiben und
  "wasm ungetestet" in den Notizen vermerken.

Fasse am Ende in 2–3 Sätzen zusammen, was getan wurde (oder warum nichts).
```

## Betriebsregeln

- **Nur EIN Rechner** sollte die Aufgabe aktiv laufen haben (sonst Push-Konflikte;
  das 🔄-Kriterium schützt nur teilweise). Beim Umzug auf den Mac Mini die
  Aufgabe auf dem Windows-PC pausieren/löschen.
- Läufe verbrauchen die normalen Abo-Limits (5h-Fenster/Woche). Drosseln =
  weniger Ready-Punkte freigeben oder Zeitplan strecken (z.B. alle 2h).

**Erstellt**: 2026-07-03
