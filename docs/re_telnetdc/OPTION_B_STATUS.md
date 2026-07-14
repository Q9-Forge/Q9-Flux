# Option B (telnetdc binär patchen) — Zwischenstand

Session 2026-07-14 nachmittags. Ziel: den timeoutlosen `Ev$Wait`-Aufruf in
`telnetdc` (Root-Cause aus ARBEITSPLAN 5.15, SS_SEvent-Lücke) durch etwas
ersetzen, das periodisch aufwacht, ohne auf den funktionslosen SS_SEvent-
Mechanismus angewiesen zu sein.

## Exakt lokalisiert (verifiziert per Ghidra 12.1.2, `telnetdc.bin` aus
`local_images/OS9SYS.hda`, 14.858 Byte, MD5 zum Zeitpunkt dieser Analyse)

Der blockierende Aufruf sitzt bei **Offset `0x00000f4c`** in `FUN_00000e5c`
(Hauptschleife), 14 Byte:

```
00000f4c  pea (0x7fff).w           ; max = 0x7fff
00000f50  move.l (-0x6786,A6),D0   ; D0 = Event-Handle (gemeinsames tty/net-Event)
00000f54  moveq 0x1,D1             ; min = 1
00000f56  bsr.w FUN_00002812       ; -> FUN_00003026: moveq #4,D1 ; trap #0 (Ev$Wait)
```

`D1=4` = `Ev$Wait`-Unterfunktion von `F$Event`. Bestätigt: **kein Timeout-
Parameter** in diesem Aufruf, siehe `FUN_00003026`.

## Weiterer Fund: vorhandener Alarm-Mechanismus

`telnetdc` besitzt bereits Code für ein `-t <idle-time>`-Feature (Auto-
Logout nach Leerlaufzeit, standardmäßig AUS). Dieser Code (`FUN_000030b6`,
`D1=2` im selben `F$Event`-Trap-Dispatcher wie `Ev$Wait`) benutzt eine
weitere `F$Event`-Unterfunktion — vermutlich `Ev$Link` (zeitgesteuertes
Auto-Signal an ein Event koppeln). Er hängt sein Alarm-Event aber an ein
**separates** Event-Objekt (`-0x67c6,A6`), nicht an das gemeinsame
tty/net-Event (`-0x6786,A6`) — wie das Feuern dort genau behandelt wird
(nur Aufwecken vs. Auto-Logout-Pfad), ist NICHT weiter verfolgt.

## Blocker: Disassemblierung nach `trap #0` fehlausgerichtet

Ghidras lineare Disassemblierung (auch mit `FixEntry.java` + `analyzeAll`)
liest direkt nach jedem `trap #0` Datenmüll als Instruktion, z.B. bei
`FUN_00003026`:

```
0000303a  trap #0x0
0000303c  ori.w #0x650c,(A3)     <- FALSCH, kein echter Code
```

`0x650c` ist als Opcode gelesen `BCS.S +0x0c` (2 Byte) — das klassische
OS-9-Rückgabe-Idiom `trap #0; bcs.s <Fehlerpfad>`. Die 2-Byte-Instruktion
wird als Datenmüll mitgelesen, dadurch können alle Folgeadressen um 2 Byte
verschoben sein. Der Decompiler bestätigt das Problem (`FUN_00003026`
dekompiliert zu offensichtlich falschem Pseudocode: `*in_D1 = 4;`,
Warnung "Bad instruction - Truncating control flow" bei Nachbarfunktionen
mit demselben Muster).

**Vor jedem Byte-Patch muss diese Fehlausrichtung behoben werden** — sonst
Gefahr, versehentlich echte Instruktionsgrenzen zu verschieben und
`telnetdc` beim Patchen zu zerstören.

## Nächster Schritt (nicht mehr Teil dieser Session)

1. In Ghidra die `bcs.s`-Sprungziele nach jedem `trap #0` manuell
   nachdisassemblieren (analog `FixEntry.java`, aber pro Trap-Stelle) ODER
   ein OS-9/68k-fähiges Disassembler-Tool mit korrekter Trap-Idiom-Erkennung
   verwenden (z.B. `objdump` mit `-m m68k`, falls es das Idiom besser
   handhabt, oder ein Ghidra-Sleigh-Callback für `trap #0`, das automatisch
   `bcc`/`bcs` danach korrekt erkennt).
2. Danach den Patch bauen: 14 Byte bei `0xf4c` ersetzen — Kandidat: kurzer
   `F$Sleep`/Delay-Aufruf (Adresse noch zu bestimmen, NICHT direkt nach
   einem `trap #0` kopieren, sondern eine bereits im Kontrollfluss klar
   erkannte Aufrufstelle referenzieren) + Rücksprung in die Poll-Schleife,
   statt `Ev$Wait`.
3. **Nur auf einem Klon-Testimage patchen und mit einer haltbaren
   Telnet-Session testen** (analog `ftpd`-Kontrolltest aus 5.15) — niemals
   direkt auf `local_images/OS9SYS.hda`.
4. Erst nach erfolgreichem Test auf dem Produktivimage deployen (Backup
   vorher, analog `OS9SYS.hda.backup-vor-5.15-netmask24-...`).

## Artefakte in diesem Verzeichnis (Session 2026-07-14 nachmittags)

- `telnetdc_full_listing.txt` — vollständiges lineares Listing (3416 Zeilen,
  Vorsicht: nach jedem `trap #0` potenziell fehlausgerichtet)
- `telnetdc_decompiled.c` — Ghidra-Decompiler-Ausgabe aller Funktionen
- `DecompileAll.java` — neues Ghidra-Skript, das die Decompiler-Ausgabe
  aller Funktionen in eine Datei schreibt (Ergänzung zu `FixEntry.java`)
- Relevante Funktionen: `FUN_000009e4` (Setup inkl. Event-Erzeugung +
  SS_SPF/SS_SEvent-Registrierung), `FUN_00000e5c` (Hauptschleife, enthält
  den `Ev$Wait`-Aufruf bei `0xf4c` sowie den Idle-Alarm-Setup),
  `FUN_0000118c`/`FUN_000010ae` (die beiden nicht-blockierenden Relay-
  Richtungen, pollen bereits per `SS_Ready`), `FUN_00002812`/`FUN_00003026`
  (Ev$Wait-Wrapper-Kette), `FUN_000030b6` (Idle-Alarm-Link-Aufruf),
  `FUN_00003072` (Event-Cleanup, `D1=0` = vermutlich `Ev$Delet`).
