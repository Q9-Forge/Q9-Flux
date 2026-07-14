# Option B (telnetdc binär patchen) — Zwischenstand

Sessions 2026-07-14 nachmittags/abends. Ziel: den timeoutlosen `Ev$Wait`-
Aufruf in `telnetdc` (Root-Cause aus ARBEITSPLAN 5.15, SS_SEvent-Lücke)
durch etwas ersetzen, das periodisch aufwacht, ohne auf den funktionslosen
SS_SEvent-Mechanismus angewiesen zu sein.

**Stand:** Statische Analyse abgeschlossen, Live-Trace-Infrastruktur gebaut
und funktionsfähig, aber der entscheidende Testlauf (Registerbelegung am
echten `Ev$Wait`-Trap live einfangen) ist **noch nicht erfolgreich
durchgeführt** — der letzte Anlauf wurde unterbrochen, bevor er zu Ende
lief. Nächster Schritt bei Wiederaufnahme: siehe ganz unten, ein einziger
Testlauf mit dem aktuellen Code-Stand.

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
