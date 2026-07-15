# Option B (telnetdc binär patchen) — Root Cause der Ev$Wait-Luecke behoben, ABER zweiter, tieferer Fehler in SS_Ready gefunden (Stand 2026-07-15)

**Zusammenfassung des Ermittlungswegs (fuer die naechste Session):**

1. Der urspruengliche SS_SEvent-Root-Cause (ARBEITSPLAN 5.15: `telnetdc`
   wartet timeoutlos in `Ev$Wait`, weil das tty-Event nie signalisiert
   wird) wurde durch den 14-Byte-Patch bei `telnetdc`-Offset `0xf4c`
   (Rohpatch + CRC-Reparatur per `fixmod -u`) **erfolgreich behoben** —
   per Live-Trace zweifelsfrei belegt: die alte `Ev$Wait`-Signatur taucht
   nach dem Patch NIE MEHR auf, stattdessen laeuft zuverlaessig ein
   `F$Sleep`-Poll (Ticks=2) in einer Schleife (Zehntausende Iterationen
   pro Testlauf beobachtet).
2. **Trotzdem bleibt die Telnet-Verbindung fuer echte Clients haengen.**
   Grund, per Live-Trace mit zwei PC-Watchpoints direkt hinter dem
   `I$GetStt`/`SS_Ready`-Aufruf (Erfolgspfad `telnetdc+0x32a8`, Fehlerpfad
   `telnetdc+0x32b2`) zweifelsfrei ermittelt: **`SS_Ready` liefert so gut
   wie immer den Fehler `E$NotRdy` (0xF6 = 246), obwohl der Poll-Loop
   dank des Patches jetzt zuverlaessig oft genug nachfragt.** In einem
   10-Minuten-Dauertest (echter `telnet`-Client, `dir -aer /dd`,
   `docs/../repro_hang.exp`-Methodik): 60.025 Poll-Iterationen,
   120.055 `E$NotRdy`-Fehler, aber nur **18 Erfolge — alle in den ersten
   Sekunden** (passend zum initialen Ausgabeschub der Top-Level-Auflistung
   von `/dd`, 19 Eintraege). **Nach diesem Anfangsschub liefert `SS_Ready`
   fuer den Rest des 10-Minuten-Laufs KEIN EINZIGES Mal mehr Erfolg** —
   kein langsames Traepfeln, sondern ein echter, dauerhafter Stillstand
   auf einer TIEFEREN Ebene als der urspruenglich diagnostizierte
   SS_SEvent-Bug.

**Schlussfolgerung:** Es gibt zwei UNABHAENGIGE Bugs im Pfad. Bug 1
(SS_SEvent/`Ev$Wait`-Timeout) ist behoben. Bug 2 (die eigentliche
`SS_Ready`-Bereitschaftserkennung im SCF-Treiber der Telnet-Pseudo-
TTY meldet nach einem kurzen Anfangsfenster dauerhaft "nicht bereit",
obwohl die Shell weiterhin Ausgabe produziert) ist NEU entdeckt und
noch nicht verstanden. Bug 2 war vorher hinter Bug 1 verborgen (Bug 1
verhinderte JEDES Polling, daher wurde Bug 2 nie sichtbar).

**Fehlmeldung im vorherigen Stand dieses Dokuments (jetzt korrigiert):**
Ein Zwischenstand behauptete faelschlich "ERFOLGREICH" — beruhte auf
einem Test-Missverstaendnis (der scheinbar erfolgreiche lange
`dir -aer /dd`-Durchlauf lief auf der LOKALEN KONSOLE, nicht ueber eine
echte Telnet-Session). Die Nutzer-Gegenprobe mit zwei echten
`telnet`-Sessions bestaetigte den fortbestehenden Haenger korrekt.

**Naechste Schritte fuer eine Folge-Session:**
- `SS_Ready`/`I$GetStt` selbst ist closed-source Microware-Kernel-Code
  (Teil von `ioman`/SCF-FM), kein eigenes Modul wie `telnetdc` — Patchen
  ist ungleich schwerer (Kernel-Modul, nicht ein einzelnes CMDS).
- Zu klaeren: liefert `SS_Ready` fuer ANDERE Pseudo-TTY-Nutzer (z.B. die
  funktionierenden `/x1-8`-Terminals via `nettty.a`) ebenfalls dauerhaft
  `E$NotRdy`, oder ist das spezifisch fuer den `telnetd`-Pfad? Falls
  `nettty.a` GAR KEIN `SS_Ready` benutzt (eigener TCP-Server im Emulator
  selbst, unabhaengig von `sptcp`), waere das kein Vergleichstest.
- Live-Trace-Infrastruktur (`Q9_TRAP_TRACE`, `Q9_WATCH_PC`/`Q9_WATCH_PC2`
  in `src/kernel/m68krt.c`, inkl. Autoerkennung der telnetdc-Ladeadresse
  per `F$Sleep`-Haeufung) ist einsatzbereit und funktioniert zuverlaessig
  — fuer die naechste Diagnoserunde direkt wiederverwendbar.
- Reproduktion ist jetzt VOLLSTAENDIG SELBSTSTAENDIG moeglich (kein
  manuelles Eingreifen mehr noetig): `/etc/sudoers.d/q9-emulator` erlaubt
  passwortloses `sudo` fuer `build/native/q9.exe`, Skripte
  `test_telnetdc_write_hang.exp` und `test_telnetdc_procs_check.exp` im
  Q9-Projekt-Root (per `expect` mit echtem `telnet`-Client) starten
  Emulator + Session + Hang-Trigger + sauberes Beenden komplett
  automatisiert.

## Nachtrag 2026-07-15 (spaeter am Tag): Bug 2 lokalisiert — SCHREIBSEITE blockiert, nicht SS_Ready selbst

**Neuer, prazisierter Befund per `procs -e`-Vergleich** (Skript
`test_telnetdc_procs_check.exp`): waehrend eines aktiven Haengers wurden
im laufenden Gast (zweite, parallele Telnet-Session) drei `procs -e`-
Schnappschuesse im 60-Sekunden-Abstand genommen und die CPU-Zeit der
beteiligten Prozesse verglichen:

| Prozess | t=0 | t=60s | t=120s |
|---|---|---|---|
| `dir <>>>pks01` (Shell-Ausgabe) | CPU 0.13, Status `e` | CPU 0.13, Status `e` | CPU 0.13, Status `e` |
| `mshell <>>>pks01` (Elternprozess) | CPU 0.07, Status `w` | CPU 0.07, Status `w` | CPU 0.07, Status `w` |
| `telnetdc <pks01` (unser gepatchtes Modul) | CPU 0.01, Status `a` | CPU 0.01, Status `s` | CPU 0.01, Status `s` |

`dir` verbraucht ueber die vollen 120 Sekunden **exakt null** zusaetzliche
CPU-Zeit — kein Busy-Loop (der wuerde wachsende CPU-Zeit zeigen), sondern
eine echte Blockade: der Prozess bekommt vom Scheduler schlicht keine
Rechenzeit mehr. `telnetdc` bleibt sichtbar aktiv (Statuswechsel `a`→`s`,
passend zum funktionierenden `F$Sleep`-Poll-Loop aus dem Patch).

**Praezisierte Schlussfolgerung:** Nicht `telnetdc` beim LESEN (`SS_Ready`)
ist die blockierende Partei, sondern `dir`/die Shell beim SCHREIBEN in den
vollen pty-Ausgabepuffer. Das erklaert auch das 18-Erfolge-dann-Stille-
Muster: die Shell fuellt den Puffer initial (daher die 18 erfolgreichen
`SS_Ready`-Reads von `telnetdc`), blockiert danach vermutlich selbst in
einem eigenen `Ev$Wait` auf ein "Puffer hat wieder Platz"-Ereignis — UEBER
DENSELBEN kaputten `SS_SEvent`-Mechanismus, nur von der Schreibseite aus,
symmetrisch zum bereits gefixten Lesebug in `telnetdc`. `SS_Ready` selbst
liefert daher schlicht ehrlich `E$NotRdy`, weil tatsaechlich nichts
Neues geschrieben wurde — kein eigener dritter Bug in `SS_Ready`.

**Praktische Konsequenz:** Der noch offene Bug sitzt vermutlich NICHT in
`telnetdc` (das haben wir bereits korrekt gefixt) und nicht spezifisch in
`SS_Ready`, sondern in der GEMEINSAM GENUTZTEN Schreib-Blockierlogik des
`scf`-Filemanager-Kernelmoduls (`OS9/68000/CMDS/BOOTOBJS/scf` im SDK,
Bootlist-Eintrag `scf` in `dev.bl`/`net.bl`) — vermutlich derselbe
`SS_SEvent`-Gap, nur fuer I$Write statt I$Read. Ein Fix dort waere
grundsaetzlicher (kernelweit, nicht nur `telnetd`-spezifisch), aber auch
aufwendiger zu patchen als ein einzelnes CMDS-Modul.

**Empfohlenes Vorgehen fuer die scf-Analyse** (dieselbe Methodik wie bei
`telnetdc`, 1:1 uebertragbar):
1. `scf` aus dem SDK oder vom Image extrahieren (ToolShed, OHNE `-l`!).
2. Ghidra-Import mit `FixEntry.java` (echten `exec`-Einsprungpunkt aus
   Modul-Header-Offset `$50` nutzen, nicht Ghidras Autoerkennung).
3. Nach einem `F$Event`-Aufruf (`Ev$Wait`, `D1=4`) in der Write-Pfad-
   Logik suchen, analog zum bereits gefundenen Muster in `telnetdc`.
4. Falls gefunden: per Live-Trace (`Q9_TRAP_TRACE`, Filter auf
   `callcode=0053`) im laufenden System bestaetigen, dann denselben
   Patch-Ansatz (`Ev$Wait` → `F$Sleep`+Poll, CRC per `fixmod -u`)
   anwenden.
5. **Vorsicht:** `scf` ist ein zentrales, oft geladenes Kernelmodul —
   Tests unbedingt nur auf einem Klon-Image, nie auf `OS9SYS.hda` direkt,
   und `make test` nach jedem Emulator-seitigen Aenderungsschritt pruefen.

## Nachtrag 2026-07-15 (parallele scf-RE durch Codex/Claudia, Windows-Seite)

Eine parallele Session (Codex, Arbeitsverzeichnis `D:\projekts\Q9-Kernel`,
vermutlich Claudias Windows-Toolchain-Umgebung) disassembliert `scf`
bereits mit Ghidra, unabhaengig von dieser Session. Bisherige Funde
(per Nutzer weitergegeben, noch nicht selbst verifiziert):

- **`M$Excpt` ist fuer dieses Modul `0`** (kein eigener Exception-Handler
  im Modulheader) — schliesst einen custom Exception-basierten Wakeup-
  Mechanismus als Erklaerung aus.
- Die sichtbare 13-Eintrag-Tabelle bei `M$Exec=$0080` ist die normale
  Standard-Filemanager-Dispatchliste, nichts Aufaelliges.
- Gefunden wurde bisher eine ZWEITE, interne Dispatch-Tabelle im
  **`ReadLn`-Pfad** (Kontrollzeichenbehandlung `PD_BSP`..`PD_QUT`:
  Backspace/Delete/EOF/Reprint/Duplicate/Interrupt/Quit) bei Offset
  `$04F4`, physische Reihenfolge wegen `D1=8`+`DBEQ`-Schleife umgekehrt.
  Sauber beschriftet in `scf-readln-control-dispatch.md` (Windows-Pfad
  `D:\projekts\Q9-Kernel\exports\`).

**Einordnung fuer UNSERE Fragestellung:** `ReadLn` ist Zeileneditierung
beim LESEN von Eingabe (Tastatur-Handling), NICHT der Schreibpfad fuer
Shell-AUSGABE, den wir als Ursache von Bug 2 verdaechtigen (`dir`
blockiert beim Schreiben in den vollen Puffer, s. `procs`-Befund oben).
Fuer unsere Hypothese waere der **`I$Write`-Pfad** von `scf` (nicht
`ReadLn`) die relevante Stelle, analog zum `Ev$Wait`-Fund in `telnetdc`,
diesmal fuer "Puffer wieder frei" statt "Daten verfuegbar". An Codex/
Claudia zurueckgespiegelt, damit die RE-Arbeit dorthin fokussiert wird
statt auf `ReadLn`, das fuer Bug 2 vermutlich nicht relevant ist.

## Nachtrag 2026-07-15 (Kurskorrektur weg von scf/term, hin zu pk/pkdvr)

**Codex' eigener Zwischenstand zu `scf_fmgr_Write`** (rekonstruierte
Quelle `q9_scf.a`, per Nutzer geteilt) bestaetigt unabhaengig: der
SCF-Schreibpfad (`scf_fmgr_Write` → `scf_write_emit_char_raw`/`_cooked`
bei `$07DE`/`$07F4` → `scf_driver_write_slot`/
`scf_driver_dispatch_preserve_regs` bei `$0872`/`$0874`) enthaelt
KEINEN sichtbaren `F$Event`/`Ev$Wait`-Aufruf — er formatiert Zeichen nur
und reicht sie ueber eine indizierte Sprungtabelle (`jsr
(0x0,A0,D1w*0x1)`) an einen GENERISCHEN Treiber weiter. Die eigentliche
Wartelogik muesste also im TREIBER stecken, nicht in `scf` selbst.

**Wichtigerer Fund (diese Session, unabhaengig von Codex):** `scf`/`term`
sind aber vermutlich GAR NICHT der relevante Codepfad fuer Telnet-
Sessions! Die extrahierten Strings von `telnetd`/`telnetdc`
(`docs/re_telnetdc/telnetd_strings.txt`, `telnetdc_strings.txt`) zeigen
`"open /pk "` und `"open /pkms "` — `telnetd`/`telnetdc` oeffnen ihre
Pseudo-Terminal-Pfade also ueber einen Deskriptor **`/pk`**, nicht ueber
`scf`/`term`. Das passt zu bereits identifizierten Modulen im
`netmods`-Merge (s. Merge-Parser-Ergebnis weiter oben):

- `pkman` (2914 Byte) — vermutlich der Pseudo-Terminal-FILE-MANAGER
  (Pendant zu `scf`, aber fuer `pk`)
- `pkdvr` (4642 Byte) — vermutlich der eigentliche TREIBER dahinter
- `pk`/`pks` (108/124 Byte) — Deskriptoren (`pks01`/`pks02` in unseren
  `procs`-Pfaden passt exakt zu `pks`!)

"PK" vermutlich "Pseudo Keyboard" — ein reines Software-Pty-Paar fuer
Netzwerksessions, ganz ohne Hardware-Bezug. `scf`/`term` sind fuer echte
serielle Ports gedacht und kommen fuer Telnet-Sessions wahrscheinlich gar
nicht zum Einsatz.

**Trap-Scan von `pkdvr`/`pkman`** (`OS9/68020/CMDS/BOOTOBJS/SPF/pkdvr`,
`.../pkman`, per Python-Skript auf `trap#0`+Inline-Wort `$0053`
gefiltert, dieselbe Methodik wie bei `telnetdc`):

- `pkman`: **keine** `F$Event`-Aufrufe (passt zu reiner Dispatcher-Rolle,
  analog zu `scf`).
- `pkdvr`: **sieben** `F$Event`-Aufrufe bei `0x10b8, 0x10e6, 0x110c,
  0x112a, 0x1158, 0x1188, 0x11a8` — ein deutlich ergiebigeres Ziel.

**Register-Analyse der sieben Stellen** (D1-Unterfunktion direkt vor
jedem Trap, `moveq #n,D1`):

| Offset | D1 | vermutete Bedeutung |
|---|---|---|
| `0x10b8` | 4 | **Ev$Wait** (bestaetigtes Muster aus `telnetdc`) |
| `0x10e6` | 2 | evtl. Ev$Link |
| `0x110c` | 3 | unklar (Signal/UnLink?) |
| `0x112a` | 6 | unklar |
| `0x1158` | 0xa | unklar |
| `0x1188` | 0xb | unklar |
| `0x11a8` | 1 | evtl. Ev$Creat |

**Nur EINE Stelle ist ein echter `Ev$Wait`** (`0x10b8`), eingebettet in
eine kleine Wrapper-Funktion mit zwei Stack-Parametern (`min`/`max` in
`D2`/`D3`, klassische `Ev$Wait(event, min, max)`-Signatur):

```
0x10a4  link.w A5,#0
0x10a8  movem.l ...,-(SP)
0x10ac  movea.l D1,A0        ; Entry-Parameter (Event-Handle o.ae.)
0x10b0  move.l (0x8,A5),D2   ; min
0x10b4  move.l (0xc,A5),D3   ; max
0x10b6  moveq  #4,D1         ; Ev$Wait
0x10b8  trap   #0
        dc.w   $0053         ; F$Event
```

**Aufrufer gefunden (Python-Rohbyte-Suche nach `bsr.w`/`bsr.b` mit Ziel
im Wrapper-Bereich):** Die `Ev$Wait`-Wrapper-Funktion bei `0x10a4`
(Parameter: ein Stack-Argument + Entry-`D1`, Ergebnis in einer lokalen
Variable, Aufruf `bsr.w 0x10a4`) wird von genau **einer** Zwischen-
funktion (`0xd10`) mit `min`/`max`-Vorbereitung aufgerufen. Diese
Zwischenfunktion (`0xd10`) wiederum wird von **vier** Stellen im
restlichen Code gerufen: `0x3ee`, `0x4c6`, `0x60e`, `0x784`.

Ueber Prolog-Suche (`link.w A5`, Opcode `4e55`) lassen sich diese vier
Aufrufer zwei umschliessenden Funktionen zuordnen — **je zwei
`Ev$Wait`-Aufrufe pro Funktion:**
- Funktion A, Start `0x38a`: enthaelt die Aufrufe bei `0x3ee` und `0x4c6`
- Funktion B, Start `0x5b6`: enthaelt die Aufrufe bei `0x60e` und `0x784`

Das passt zum erwarteten Bild eines Treibers mit getrennten Read- und
Write-Einsprungpunkten (je einmal `Ev$Wait` fuer den Normalfall + einmal
fuer einen Sonderfall/Retry, oder Lese-/Schreib-Variante mit je 2
Wartebedingungen). **Welche der beiden Funktionen (`0x38a` oder `0x5b6`)
zum `Write`-Treiber-Einsprungpunkt gehoert, liess sich per Rohbyte-Suche
NICHT mehr zuverlaessig klaeren** — die Treiber-Dispatch-Tabelle von
`pkdvr` nutzt offenbar dieselbe zweistufige relative Adressierung wie in
`scf`s Treiber-Aufruf-Wrapper (`add.l (0x30,A0),D1` → indizierter
Tabellen-Lookup → `jsr`, s. `scf_driver_dispatch_preserve_regs` oben) —
das ist mit Ghidras Cross-Reference-/Decompiler-Werkzeugen zuverlaessig
aufzuloesen, mit reiner Rohbyte-Mustersuche nicht mehr sinnvoll.

**Naechster Schritt fuer Codex:** `pkdvr` in Ghidra importieren
(`FixEntry.java`, echten `exec`-Einsprungpunkt nutzen), die Treiber-
Dispatch-Tabelle (analog `scf`s `M$Exec`-Tabelle, aber mit den
treiberueblichen 6 Eintraegen Init/Read/Write/GetStt/SetStt/Term)
identifizieren und pruefen, ob `0x38a` oder `0x5b6` als `Write`-Handler
referenziert wird. Falls `0x5b6` (oder `0x38a`) tatsaechlich der
Write-Pfad ist: die beiden dortigen `Ev$Wait`-Aufrufe genauer
untersuchen (Event-Handle-Herkunft, welches Ereignis erwartet wird) —
das waere der Bug-2-Fundort. **Empfehlung: RE-Fokus von Codex auf
`pkdvr` umlenken statt `scf`/`term`.**

## Nachtrag 2026-07-15 (Microware SCF Driver Training Seminar 1997 — scf DOCH relevant, als Filemanager fuer die pks-Seite)

Nutzer hat ein Original-Microware-Schulungsdokument ("SCF Driver Training
Seminar", 1997, OS-9000/PowerPC/x86-Fokus, nicht 68K) gefunden und geteilt.
Trotz anderer CPU-Generation architektonisch hochrelevant:

**Die Beispiel-`devs`-Ausgabe im Dokument zeigt EXAKT unsere Situation:**
```
Device     Driver    ...   File Mgr   ...
pks01      pkdvr      ...   scf        ...
pkm01      pkdvr      ...   pkman      ...
```
**Ein Pseudo-Terminal-Paar hat zwei Seiten, mit demselben Treiber
(`pkdvr`) aber ZWEI VERSCHIEDENEN Filemanagern:** `pks01` laeuft ueber
`scf` (vermutlich die Shell-Seite, Standard-I/O von `mshell`/`dir`),
`pkm01` laeuft ueber `pkman` (vermutlich die `telnetd`/`telnetdc`-Seite).
Das erklaert die fruehere String-Beobachtung `"open /pk "` — `telnetd`
oeffnet seine eigene `pkm`-Seite, waehrend die Shell automatisch die
gepaarte `pks`-Seite als Standard-I/O bekommt. **`scf` ist also DOCH im
Bild — nicht als Filemanager fuer eine Hardware-Seriellschnittstelle,
sondern gepaart mit `pkdvr` fuer die SCHREIBSEITE (Shell) des
Software-Pty-Paars.** Die fruehere Kurskorrektur "weg von scf" war also
nur teilweise richtig: `scf`s `Write`-Pfad bleibt relevant, ruft aber in
`pkdvr` (nicht in einen Hardware-Treiber).

**Wake-Mechanismus fuer blockierte Schreiber ist im Dokument explizit
beschrieben** (Abschnitt "OUTPUT", Interrupt-Service-Routine-Beispiel):
> "If there was a process blocked trying to write into us, we can now
> wake them up... `proc_id = unit_stat->v_wake; if (proc_id != 0 &&
> unit_stat->v_wait == WT_OUTPUT) { unit_stat->v_wake = 0; ...;
> _os_send(proc_id, S_WAKE); }`"

Bestaetigt architektonisch exakt unsere Hypothese: **der TREIBER
(`pkdvr`) ist dafuer zustaendig, den blockierten Schreiber aufzuwecken**,
sobald wieder Platz im Ausgabepuffer ist — nicht `scf` selbst (passt zu
Codex' Fund: kein `Ev$Wait` in `scf`s Schreibpfad).

**Wichtiger Vorbehalt:** Das Dokument beschreibt OS-9000 (PowerPC/x86),
wo dieser Wakeup ueber ein simples `_os_send(pid, S_WAKE)`-Signal laeuft,
NICHT ueber `F$Event`/`Ev$Wait` wie in unserem OS-9/68K-System. Das
Dokument selbst weist darauf hin: "One of the improvements made to SCF
for OS-9000... the file manager itself maintains both the input and
output buffers" — impliziert, dass OS-9/68K's SCF-Generation ANDERS
arbeitet (vermutlich haelt bei uns der TREIBER selbst die Puffer, nicht
`scf`). Der genaue Wake-Mechanismus (Signal vs. Event) unterscheidet sich
also vermutlich zwischen den Versionen — aber das GRUNDPRINZIP (Treiber
ist fuer das Aufwecken des blockierten Schreibers zustaendig) ist
versionsuebergreifend gueltig und bestaetigt unsere Zielrichtung.

**Neue Arbeitshypothese:** Da `pkdvr` ein reines SOFTWARE-Pty-Paar ohne
echte Hardware-Interrupts implementiert, muss es INTERN zwischen den
zwei virtuellen Seiten (scf/`pks`-Schreiber und pkman/`pkm`-Leser)
synchronisieren — ein Interrupt kann das (anders als bei echter
Hardware) nicht automatisch antreiben. `F$Event`/`Ev$Wait` ist dafuer der
plausible OS-9/68K-Mechanismus. Das wuerde auch erklaeren, warum der
`telnetdc`-Patch allein nicht ausreichte: wir haben `telnetdc`s EIGENEN
AEUSSEREN Wartepunkt gefixt, aber `pkdvr` hat vermutlich eine ZWEITE,
INNERE Ev$Wait-Synchronisation zwischen den beiden Pty-Seiten, die
eventuell auf demselben kaputten `SS_SEvent`-Mechanismus beruht.

**Praezisierter Auftrag an Codex:** Bei der Untersuchung der beiden
`pkdvr`-Kandidatenfunktionen (`0x38a`, `0x5b6`) gezielt pruefen, ob eine
davon vom `scf_fmgr_Write`-Aufruf (ueber die generische Treiber-
Dispatch-Tabelle, `$0872`/`$0874`) erreicht wird — das waere dann exakt
die Stelle, an der die Shell (via `pks01`/`scf`) beim Schreiben haengt.

## Angebrachter Patch

**Modul:** `/dd/CMDS/telnetdc` auf dem Klon-Image (NICHT im `netmods`-
Merge enthalten — verifiziert per Merge-Parser, `netmods` hat nur den
reinen Protokoll-Stack, 24 Module, kein `telnetd`/`telnetdc`; die beiden
werden als eigene Dateien in `/dd/CMDS` zur Laufzeit per Name-Suche
geladen/geforkt, `mdir`-Ladereihenfolge bestaetigt frisches Laden bei
Testverbindungen).

**Vorgehen (sauber, ohne eigenes CRC-Tool):**
1. Rohpatch (nur die 14 Byte bei Offset `0xf4c`, CRC bewusst NICHT
   angefasst) per Python-Skript geschrieben, dann per ToolShed `os9 del`
   + `os9 copy` (ohne `-l`!) aufs Klon-Image kopiert.
2. CRC-Reparatur NATIV im laufenden Emulator per `fixmod -u
   /dd/CMDS/telnetdc` (nicht per eigenem Cross-Tool/Python-CRC —
   Nutzerkorrektur in dieser Session: vorhandene Werkzeuge verwenden statt
   ein viertes selbst zu bauen). `ident` zeigte danach "good CRC".
3. Emulator-Reboot noetig, damit `fixmod`s In-Memory-Aenderung + der
   naechste frische Fork von `/dd/CMDS/telnetdc` sauber zusammenpassen.

**Patch-Bytes bei Offset `0xf4c`** (ersetzt den timeoutlosen `Ev$Wait`):
```
70 02            moveq #2,D0      ; 2 Ticks
4E 40            trap #0
00 0A            dc.w $000A       ; F$Sleep
60 8C            bra.b 0xee0      ; zurueck an den Schleifenkopf
4E 71 4E 71 4E 71  nop nop nop    ; Padding, nie ausgefuehrt
```
Wirkt an der Stelle, wo die Hauptschleife (`FUN_00000e5c`) nach einem
Durchlauf ohne Fortschritt bisher unbegrenzt auf das (fuer's tty-Event
nie signalisierte) gemeinsame Event wartete — jetzt schlaeft sie kurz und
pollt beide Relay-Richtungen (`FUN_000010ae`, `FUN_0000118c`) erneut,
unabhaengig vom kaputten `SS_SEvent`-Mechanismus.

## Naechster Schritt: Produktiv-Deployment

Noch NICHT auf `local_images/OS9SYS.hda` (Produktivimage) angewendet.
Bei Bedarf: Backup ziehen (analog 5.14/5.15-Deployment-Muster), denselben
Rohpatch+`fixmod -u`-Weg auf `/dd/CMDS/telnetdc` des Produktivimages
anwenden, danach `make test` + ein echter Telnet-Client-Test gegen das
Produktivimage.

---

# Vorheriger Stand (Rechercheverlauf, informativ)

Sessions 2026-07-14 nachmittags/abends. Ziel: den timeoutlosen `Ev$Wait`-
Aufruf in `telnetdc` (Root-Cause aus ARBEITSPLAN 5.15, SS_SEvent-Lücke)
durch etwas ersetzen, das periodisch aufwacht, ohne auf den funktionslosen
SS_SEvent-Mechanismus angewiesen zu sein.

**Stand (aktualisiert 2026-07-15):** Statische Analyse abgeschlossen, Live-
Trace-Infrastruktur gebaut, und **der entscheidende Testlauf ist jetzt
erfolgreich durchgeführt** — der `Ev$Wait`-Trap wurde im laufenden System
live eingefangen und die Laufzeit-Ladeadresse von `telnetdc` bestimmt.
Nächster Schritt: den 14-Byte-Patch bei Offset `0xf4c` bauen und auf einem
Klon-Image testen (Details unten).

## Trace-Ergebnis (2026-07-15, Lauf `telnetdc_trap_trace1.log`)

Reproduktion sauber gelungen: `dir -e /dd/CMDS/BOOTOBJS` über echtes
`telnetd`/eth0 blieb nach **1410 Byte** stehen; ein einzelner Tastendruck
löste **sofort +1433 Byte** aus — exakt das SS_SEvent-Root-Cause-Muster.

Trace-Filter `callcode=0053` (F$Event, Inline-Wort bei PC+2) funktionierte
wie geplant: 30.371 F$Event-Aufrufe system-weit geloggt, davon 1560 mit
`d1=00000004` (Ev$Wait). Die Ev$Wait-Aufrufe verteilen sich auf mehrere
Module (je eigene Ladeadresse); **`telnetdc` ist der Cluster bei `0xea5xxx`**:

- Haupt-Wait (Offset `0x303a` im Wrapper `FUN_00003026`): Laufzeit-PC
  **`0x00ea562a`**, Signatur `d1=4, d2=1 (min), d3=0x7fff (max)` — passt
  exakt zum statischen `pea 0x7fff; moveq 1,D1` bei `0xf4c`.
- Gegencheck über den `-t`-Idle-Alarm Ev$Link (statisch `0x3138` in
  `FUN_00003122`): Laufzeit-PC `0x00ea5728` → statischer Offset `0x3138`,
  identische Basis. **Zwei unabhängige Anker bestätigen die Ladebasis.**

**Bestätigte Ladebasis von `telnetdc` in diesem Lauf: `0x00EA25F0`**
(= `0xea562a − 0x303a`). Der Patch-Standort `0xf4c` liegt damit zur Laufzeit
bei `0xEA353C`. Hinweis: Die Basis ist lauf-/ladeabhängig — der **Patch
wird am Modul-Offset `0xf4c` in der Moduldatei auf dem Image** angebracht,
nicht an einer festen Laufzeitadresse; die Basis diente nur der Bestätigung,
dass der geloggte Trap wirklich dieser Aufruf ist.

Die 14 Patch-Bytes am Offset `0xf4c` (aus dem Listing):
`48 78 7f ff` (`pea 0x7fff.w`) · `20 2e 98 7a` (`move.l -0x6786(A6),D0`) ·
`72 01` (`moveq 1,D1`) · `61 00 18 ba` (`bsr.w 0x2812`).

## Statisch exakt lokalisiert (Ghidra 12.1.2, `telnetdc.bin` aus
`local_images/OS9SYS.hda`, 14.858 Byte)

Der blockierende Aufruf sitzt bei **Offset `0x00000f4c`** in `FUN_00000e5c`
(Hauptschleife), 14 Byte:

```
00000f4c  pea (0x7fff).w           ; max = 0x7fff
00000f50  move.l (-0x6786,A6),D0   ; D0 = Event-Handle (gemeinsames tty/net-Event)
00000f54  moveq 0x1,D1             ; min = 1
00000f56  bsr.w FUN_00002812       ; -> FUN_00003026: moveq #4,D1 ; trap #0 (Ev$Wait)
```

`D1=4` = `Ev$Wait`-Unterfunktion von `F$Event`. Bestätigt: **kein Timeout-
Parameter** in diesem Aufruf.

## WICHTIGE KORREKTUR (nicht mehr "Disassemblierung fehlausgerichtet")

Ein früherer Stand dieser Datei behauptete, Ghidras Disassemblierung sei
nach jedem `trap #0` fehlausgerichtet (`ori.w #0x650c,(A3)` als Datenmüll
fehlinterpretiert). **Das war falsch** — die Bytes sind korrekt disassembliert.
Die tatsächliche, jetzt verstandene Erklärung, nach Cross-Check mit der
OS-9-Trap-Konvention:

**Der eigentliche OS-9-Aufrufcode (z.B. `F$Event` = `0x53`) steht NICHT in
einem Register, sondern als INLINE-DATENWORT direkt hinter der `trap #0`-
Instruktion im Code** (klassische OS-9-Konvention, analog dem SWI2+Inline-
Byte-Muster von OS-9/6809). Bei `FUN_00003026` steht bei `0x303c` das Wort
`$0053` — das ist exakt der `F$Event`-Aufrufcode, keine Instruktion. Der
Kernel liest dieses Wort beim Rücksprung vom Trap und überspringt es; der
**echte** nächste ausgeführte Befehl ist `bcs.s +0x0c` bei `0x303e` (Byte
`650c`), das klassische `trap #0; bcs.s <Fehlerpfad>`-Idiom. Der ganz frühe
erste Verdacht (noch vor der fälschlichen "Korrektur") war also im Kern
richtig.

**Praktische Konsequenz:** Register-Snapshots am `trap #0` (D0..D3) allein
reichen NICHT, um einen bestimmten Aufruf (z.B. `Ev$Wait`) zuverlässig zu
identifizieren — man muss zusätzlich das Inline-Wort bei `PC+2` mitlesen
und auf `0x53` (F$Event) prüfen. Register D0=Event-Handle, D1=Unterfunktion
(4=Wait), D2=max, D3=min gelten dann innerhalb eines so bestätigten
F$Event-Aufrufs.

## Weiterer Fund: vorhandener Alarm-Mechanismus in telnetdc

`telnetdc` besitzt bereits Code für ein `-t <idle-time>`-Feature (Auto-
Logout nach Leerlaufzeit, standardmäßig AUS). Dieser Code (`FUN_000030b6`,
`D1=2` im selben `F$Event`-Trap-Dispatcher wie `Ev$Wait`) benutzt eine
weitere `F$Event`-Unterfunktion — vermutlich `Ev$Link` (zeitgesteuertes
Auto-Signal an ein Event koppeln). Er hängt sein Alarm-Event aber an ein
**separates** Event-Objekt (`-0x67c6,A6`), nicht an das gemeinsame
tty/net-Event (`-0x6786,A6`) — wie das Feuern dort genau behandelt wird
(nur Aufwecken vs. Auto-Logout-Pfad), ist NICHT weiter verfolgt.

## Live-Trace-Infrastruktur (NEU, in `main` committet, funktionsfähig)

Da die statische Registeranalyse an Grenzen stieß, wurde ein Live-Trace im
Musashi-CPU-Kern eingebaut, um die echte Registerbelegung beim `Ev$Wait`-
Trap im laufenden Emulator zu messen statt sie aus dem Listing zu raten:

- `third_party/musashi/m68kconf.h`: `M68K_TRAP_HAS_CALLBACK` auf
  `M68K_OPT_ON` (laufzeit-setzbar über `m68k_set_trap_instr_callback`,
  NICHT `M68K_OPT_SPECIFY_HANDLER`). **Wichtig beim nächsten Build:** diese
  Header-Änderung wird von den vendorten Musashi-Objekten (`build/native/
  musashi_m68kcpu.o`, `musashi_m68kops.o`, `musashi_softfloat.o`) NICHT
  automatisch per Header-Dependency neu gebaut — vor dem nächsten Test ggf.
  `rm -f build/native/musashi_m68k*.o build/native/musashi_softfloat.o &&
  make native` prüfen, falls diese drei Dateien seit dem letzten Pull nicht
  neu sind.
- `src/kernel/m68krt.c`, `m68krt_trap_trace_callback()`: aktiviert NUR wenn
  Env-Var `Q9_TRAP_TRACE=<pfad>` gesetzt ist (sonst No-Op, kein Performance-
  Einfluss auf normale Läufe). Liest bei JEDEM `trap #0` das Inline-Wort bei
  `PC+2` (`m68k_read_memory_16`), loggt NUR bei Treffer `callcode==0x53`
  (F$Event) — PC (=`M68K_REG_PPC`, Adresse der trap-Instruktion selbst),
  D0..D3, A0, A1, groß gepuffert (`_IOFBF`, 4 MiB) in die Log-Datei.
  Sicherheitscap `g_trap_trace_cap` (Default 2 Mio. Zeilen).

**Iterationsgeschichte dieser Session (damit niemand dieselben Sackgassen
nochmal durchläuft):**
1. Erster Versuch: JEDEN `trap #0` ungefiltert + zeilengepuffert (`_IOLBF`)
   geloggt → Boot so massiv ausgebremst (ein `write()`-Syscall pro
   Trap0-Zeile, trap#0 ist der Direktaufruf-Trap für ALLE F$/I$-Calls im
   gesamten System), dass es wie ein Absturz/Hänger wirkte. War aber "nur"
   Performance, kein echter Fehler.
2. Zweiter Versuch: Filter auf `D1==4 && D2==0x7fff` direkt im Callback →
   **null Treffer**. Register-Vermutung falsch (siehe Korrektur oben).
3. Dritter Versuch: kompletter Filter-Verzicht, aber groß gepuffert
   (`_IOFBF`, 4 MiB) → Boot lief normal, Log wurde ~280.000 Zeilen groß.
   Analyse zeigte: `D1==4` allein kommt über 4000x vor (viel zu
   unspezifisch, andere Syscalls nutzen D1=4 zufällig für andere Zwecke).
   Versuch, `I$SetStt`-Aufrufe (Code `0x8e`) als Anker zu finden, ergab nur
   5 Treffer im ganzen Log, aber auch dort passte die erwartete
   Registerbelegung nicht sauber — Anlass für die obige Korrektur zum
   Inline-Aufrufcode.
4. Vierter (aktueller) Stand: Filter auf das Inline-Wort `PC+2 == 0x53`
   (echter F$Event-Aufrufcode) statt auf Register-Rateversuche — sollte
   jetzt sauber NUR echte F$Event-Aufrufe treffen (Create/Wait/Link/Delet
   etc., system-weit vermutlich recht selten). **Dieser Stand wurde noch
   nicht getestet** — Build + `make test` liefen sauber durch, aber der
   Testlauf im Emulator wurde vor Abschluss unterbrochen (Nutzer musste los).

## Nächster Schritt bei Wiederaufnahme

1. Sicherstellen, dass der aktuelle Code-Stand gebaut ist:
   ```bash
   cd /Volumes/SSD1TB/projects/Q9
   rm -f build/native/musashi_m68k*.o build/native/musashi_softfloat.o
   make native && make test
   ```
2. Testlauf (braucht `sudo` für vmnet — vom Nutzer selbst auszuführen):
   ```bash
   sudo Q9_TRAP_TRACE=/tmp/telnetdc_trap_traceN.log ./build/native/q9.exe \
     --cb030 /Volumes/SSD1TB/projects/MWOS/OS9/68030/PORTS/Q9/CMDS/BOOTOBJS/ROMBUG/romimage.dev.running.BIN \
     --cf local_images/OS9SYS.claudia-optionb-trace.hda --net vmnet
   ```
   (Neuer Dateiname pro Versuch, da alte Log-Dateien `root` gehören und vom
   Nutzer-Account nicht überschrieben/gelöscht werden können.)
3. Zweites Terminal: `telnet 192.168.200.2`, einloggen (`super`),
   `dir -e /dd/CMDS/BOOTOBJS` bis es hängt, **eine Taste drücken** (löst den
   entscheidenden `Ev$Wait`-Return aus), warten bis fertig, dann im ersten
   Terminal `Strg-]` (flusht den Trace automatisch via `exit(0)`).
4. Log auswerten: `grep callcode=0053 <log>` — die Einträge mit `d1=00000004`
   sind die `Ev$Wait`-Aufrufe; PC-Adresse verrät die Laufzeit-Ladeadresse von
   `telnetdc` (statischer Offset `0xf4c` minus PC ≈ Basisadresse, zum
   Abgleich mit dem statischen Listing unten).
5. Erst danach den eigentlichen 14-Byte-Patch bei `0xf4c` bauen und **nur
   auf einem Klon-Testimage** testen (nie auf `local_images/OS9SYS.hda`
   direkt) — Kandidat weiterhin: kurzer Delay/Poll statt `Ev$Wait`, oder den
   vorhandenen Idle-Alarm-Mechanismus (`FUN_000030b6`) auf das gemeinsame
   Event umbiegen (siehe Fund oben). Erst nach erfolgreichem Klontest, mit
   vorherigem Backup, aufs Produktivimage deployen.

## Testimage

`local_images/OS9SYS.claudia-optionb-trace.hda` — frischer Klon von
`OS9SYS.hda` (erstellt 2026-07-14 abends), hat das Netzwerk-Autostart-Setup
(anders als das ältere, dafür ungeeignete `OS9SYS.claudia-net-test.hda`,
das vor der Netzwerk-Einrichtung erstellt wurde und in seinem `SYS/startup`
keine Netzwerk-Ladebefehle enthält).

## Artefakte in diesem Verzeichnis

- `telnetdc_full_listing.txt` — vollständiges lineares Ghidra-Listing (3416
  Zeilen, Bytes korrekt, s. Korrektur oben — Interpretation der Bytes direkt
  nach `trap #0` braucht das Inline-Aufrufcode-Wissen aus diesem Dokument)
- `telnetdc_decompiled.c` — Ghidra-Decompiler-Ausgabe aller Funktionen
  (Vorsicht: die Trap-Umgebung dekompiliert wegen des Inline-Worts
  unbrauchbar, z.B. `FUN_00003026` als `*in_D1 = 4;` fehlinterpretiert)
- `DecompileAll.java` — Ghidra-Skript für die Decompiler-Ausgabe aller
  Funktionen in eine Datei (Ergänzung zu `FixEntry.java`)
- Relevante Funktionen: `FUN_000009e4` (Setup inkl. Event-Erzeugung +
  SS_SPF/SS_SEvent-Registrierung über `FUN_00002010`/`FUN_00001520`),
  `FUN_00000e5c` (Hauptschleife, enthält den `Ev$Wait`-Aufruf bei `0xf4c`
  sowie den Idle-Alarm-Setup), `FUN_0000118c`/`FUN_000010ae` (die beiden
  nicht-blockierenden Relay-Richtungen, pollen bereits per `SS_Ready`),
  `FUN_00002812`/`FUN_00003026` (Ev$Wait-Wrapper-Kette, `trap #0` bei
  `0x303a`, Inline-Callcode `0x0053` bei `0x303c`, echter Folgebefehl
  `bcs.s +0xc` bei `0x303e`), `FUN_00002a02` (I$SetStt-Wrapper für
  SS_SEvent, `trap #0` bei `0x2a12`), `FUN_00002b08` (I$SetStt-Wrapper für
  SS_SPF, `trap #0` bei `0x2b14`), `FUN_000030b6` (Idle-Alarm-Link-Aufruf),
  `FUN_00003072` (Event-Cleanup, `D1=0` = vermutlich `Ev$Delet`).
