# q9edit — Projektziele und technische Festlegungen

Stand: 2026-07-15

## 1. Produktidee

`q9edit` wird ein kleiner, moderner Vollbild-Texteditor fuer OS-9/68000. Er
soll sich auf dem Q9-CB030-System angenehm fuer Quellcode, Skripte und normale
Textdateien einsetzen lassen, ohne eine grosse Unix-Laufzeitumgebung
vorauszusetzen.

Der Projektname lautet **q9edit**. Das installierte OS-9-Modul und der kurze
Kommandoaufruf lauten **`qe`**:

```text
qe datei.c
```

Eine zusaetzliche Langform `q9edit` kann spaeter als Alias oder zweiter
Modulname angeboten werden. `qe` bleibt der verbindliche Hauptname.

## 2. Hauptziele

1. Schneller Start und geringer Speicherbedarf auf einem emulierten oder echten
   68k-System.
2. Vertraute, leicht erlernbare Bedienung ohne Emacs- oder vi-Vorkenntnisse.
3. Zuverlaessiges Bearbeiten und Speichern von OS-9-Textdateien.
4. Syntaxfarben fuer die auf Q9 wichtigsten Programmiersprachen.
5. Betrieb auf der lokalen Konsole und auf den virtuellen Netzwerk-Terminals.
6. Eine kleine, nachvollziehbare Codebasis, die wir selbst warten koennen.
7. Spaetere Ausfuehrbarkeit auf echter CPU32-/MC68EN360-Hardware.

## 3. Verbindlicher Funktionsumfang der ersten brauchbaren Version

### 3.1 Bearbeiten

- Datei oeffnen oder eine neue Datei anlegen
- Zeichen und Zeilen einfuegen und loeschen
- Pfeiltasten, Home, End, Page Up und Page Down
- horizontales und vertikales Scrollen
- Tabulatoren sichtbar und konsistent darstellen
- Datei speichern und unter ihrem bestehenden Namen weiterschreiben
- Warnung vor dem Verlassen einer ungespeicherten Datei

### 3.2 Orientierung

- Statuszeile mit Dateiname, Aenderungszustand und Cursorposition
- sichtbare Rueckmeldung fuer Fehler und Befehle
- inkrementelle Textsuche mit Vorwaerts-/Rueckwaertsnavigation
- gut merkbare Steuerung, zuerst `Ctrl-S` Speichern, `Ctrl-Q` Beenden und
  `Ctrl-F` Suchen

### 3.3 Syntaxfarben

Die erste Version markiert mindestens:

- C-Quellcode und Header (`.c`, `.h`)
- 68k-Assembler nach Festlegung der im MWOS-Baum verwendeten Endungen
- Kommentare, Zeichenketten, Zahlen, Schluesselwoerter und Datentypen

Die Farben werden mit ANSI-Farbsequenzen ausgegeben. Wenn ein Terminal keine
Farben unterstuetzt, muss der Editor vollstaendig monochrom benutzbar bleiben.

### 3.4 OS-9-Textverhalten

- OS-9-Textdateien mit CR-Zeilenenden korrekt lesen und schreiben
- LF- und CRLF-Dateien beim Einlesen erkennen
- beim Speichern standardmaessig das urspruengliche Zeilenformat erhalten;
  neue Dateien verwenden OS-9-CR
- niemals stillschweigend eine Datei zerstoeren: Speichern erfolgt mit
  pruefbaren Fehlerpfaden

## 4. Gewuenschte Ausbaustufen

Nach der ersten stabilen Version sind vorgesehen:

- Undo und Redo
- Zeilennummern, ein- und ausschaltbar
- Gehe-zu-Zeile
- Suchen und Ersetzen
- Auswahl sowie Ausschneiden, Kopieren und Einfuegen innerhalb des Editors
- weitere Syntaxdefinitionen, insbesondere Makefiles und Q9-Konfigurationen
- optionaler zweiter Puffer oder mehrere Dateien
- konfigurierbare Tabulatorbreite
- Hilfeansicht direkt im Editor

Diese Punkte duerfen den kleinen, stabilen Grundeditor nicht unnoetig
verzoegern.

## 5. Bewusst nicht Teil der ersten Version

- keine curses-Abhaengigkeit
- keine grafische Oberflaeche
- kein Mauszwang
- keine Pluginsprache
- kein Language Server Protocol und keine IDE-Funktionen
- kein eingebauter Compiler oder Debugger
- kein Anspruch auf vollstaendige Unicode-Bearbeitung in der ersten Version
- kein gleichzeitiges Bearbeiten sehr grosser Dateien, die den freien
  OS-9-Speicher deutlich ueberschreiten

## 6. Eingesetzte Technik

### 6.1 Editor-Kern: Kilo

Als Ausgangspunkt verwenden wir
[antirez/kilo](https://github.com/antirez/kilo). Kilo ist ein kleiner
Vollbildeditor in C, verwendet direkte VT100-Sequenzen, besitzt bereits Suche
und C/C++-Syntaxfarben und benoetigt keine curses-Bibliothek.

- Sprache: C
- Lizenz: BSD 2-Clause
- Upstream-Code wird mit Lizenztext und Herkunftsnachweis aufgenommen.
- OS-9-Anpassungen werden moeglichst aus dem Editor-Kern herausgehalten.
- Der importierte Stand wird mit genauer Revision dokumentiert.

Kilo ist der Startpunkt, nicht die unveraenderliche Produktgrenze. Fehler,
Speicherverhalten und Bedienung werden fuer q9edit eigenstaendig gepflegt.

### 6.2 Terminalausgabe: Termcap und ANSI/VT100

Q9/OS-9 besitzt derzeit `termcap`, aber keine von uns vorausgesetzte
curses-Portierung. q9edit verwendet deshalb eine kleine Terminalschicht:

- `termcap` fuer Cursorbewegung, Loeschen, Attribute und bekannte
  Tastensequenzen
- ANSI/VT100-Sequenzen als kontrollierter Weg fuer Syntaxfarben auf `TERM=q9`
- OS-9-SCF/GetStat/SetStat fuer Echo, Einzelzeicheneingabe und Terminalzustand
- sichere Wiederherstellung des vorherigen Terminalzustands beim normalen
  Ende und bei behandelbaren Fehlern

Die erste Implementierung darf fuer `TERM=q9` optimiert werden. Terminalcodes
duerfen dennoch nicht quer im Editor-Kern verteilt werden.

### 6.3 Zielsystem und Compiler

- Betriebssystem: Microware OS-9/68000 im Q9-CB030-Emulator
- emulierte CPU: Motorola 68030 ueber Musashi
- spaetere Hardware: CPU32+/MC68EN360
- Zielcompiler: **`xcc` aus dem vorhandenen OS-9-SDK**
- Sprachstandard: **ANSI C89**; der OS-9-Build ist die verbindliche
  Portabilitaetsgrenze
- Compilerweg: vorhandene MWOS-Cross-Toolchain fuer OS-9/68030
- Befehlssatz bleibt konservativ und darf keine fuer CPU32 ungeeigneten
  68030-Sonderfunktionen voraussetzen.

Ein moderner Host-Compiler darf fuer schnelle Tests verwendet werden, aber nur
im C89-Modus mit strengen Warnungen. Code, der nur dort baut, gilt nicht als
fertig.

### 6.4 C89- und OS-9-Portierungsregeln

Der Kilo-Upstream ist klein, setzt aber an einzelnen Stellen neuere C- oder
POSIX-Eigenschaften voraus. Fuer q9edit gelten deshalb:

- Variablen werden am Blockanfang deklariert, nicht im Initialisierungsteil
  einer `for`-Schleife.
- Keine `//`-Kommentare im portablen Zielcode.
- Keine variablen Arrays, `long long`-Abhaengigkeit oder C99-Initialisierer.
- `snprintf()` wird nur benutzt, wenn es in der Zielbibliothek nachweislich
  vorhanden und korrekt ist; sonst erhaelt q9edit eine begrenzte Hilfsfunktion.
- Unix-spezifische APIs wie `termios`, `ioctl(TIOCGWINSZ)`, `ftruncate()` und
  POSIX-Feature-Makros bleiben ausserhalb des Editor-Kerns.
- Verfuegbarkeit und Semantik von `malloc`, `realloc`, `free`, Datei-I/O,
  `ctype` und Zeitfunktionen werden mit kleinen xcc-Proben verifiziert.
- Warnungsfreier xcc-Build und ein lauffaehiges OS-9-Programm-Modul sind fuer
  jeden Meilenstein wichtiger als Komfort im Host-Build.

### 6.5 Entwicklungs- und Testumgebung

- Quellcode und Versionsverwaltung liegen auf dem Host im Q9-Repository.
- Das Ergebnis ist ein OS-9-Programm-Modul namens `qe`.
- Deployment erfolgt nach `/dd/CMDS/qe` in ein festes Entwicklungsimage.
- ToolShed und Emulator duerfen niemals gleichzeitig schreibend auf dasselbe
  Image zugreifen.
- Interaktive Tests laufen bevorzugt ueber `/x1` bis `/x8` auf dem stabilen
  virtuellen Terminalzugang ab Port 2000.
- Das produktive Image wird nicht fuer normale Entwicklungsiterationen
  veraendert; dafuer wird ein Test- oder Klon-Image verwendet.

## 7. Architekturgrenzen

Der Code wird in drei Verantwortungsbereiche geteilt:

```text
Editor-Kern
    |
    +-- Terminal-API
    |     termcap + ANSI + OS-9 SCF
    |
    +-- Datei-API
    |     open/read/write/close + CR/LF-Behandlung
    |
    `-- System-API
          Speicher, Zeit, Fehlertexte und Programmende
```

Der Editor-Kern darf weder `termios` noch Unix-`ioctl` direkt verwenden. Alle
Plattformunterschiede werden in `src/platform/` gekapselt. Dadurch bleibt ein
Host-Build fuer schnelle Tests moeglich.

## 8. Entwicklungsreihenfolge

1. **Terminalprobe:** Bildschirm aufbauen, Farben zeigen, Sondertasten lesen,
   Terminalzustand garantiert restaurieren.
2. **Kilo importieren:** Lizenz und Revision dokumentieren; direkten
   Unix-Zugriff hinter die Plattform-API verschieben.
3. **Host-Testbuild:** Editor-Kern und Syntaxlogik im strengen ANSI-C89-Modus
   schnell ausserhalb des Emulators pruefen.
4. **OS-9-Cross-Build:** mit `xcc` das echte OS-9-Modul `qe` erzeugen.
5. **Deployment automatisieren:** Modul bei gestopptem Emulator nach
   `/dd/CMDS` kopieren und Attribute setzen.
6. **Emulator-Rauchtest:** Start, Eingabe, Bewegung, Suche, Speichern und
   Beenden ueber Port 2000 automatisiert pruefen.
7. **Haertung:** grosse Dateien, lange Zeilen, volle Datentraeger,
   Speicherknappheit und Verbindungsabbruch testen.

## 9. Abnahmekriterien fuer Version 0.1

Version 0.1 gilt als brauchbar, wenn alle folgenden Punkte erfuellt sind:

1. `qe test.c` startet auf der lokalen Konsole und auf `/x1`.
2. Der Editor erkennt Pfeil-, Home-, End- und Bildtasten korrekt.
3. Eine C-Datei wird mit sichtbaren Syntaxfarben dargestellt.
4. Suchen, Aendern und Speichern funktionieren.
5. Eine gespeicherte Datei kann von OS-9-Werkzeugen korrekt gelesen werden.
6. Nach dem Beenden sind Echo, Cursor und Terminalattribute wieder korrekt.
7. Ein fehlgeschlagenes Speichern wird sichtbar gemeldet und verwirft den
   bearbeiteten Puffer nicht.
8. Ein automatisierter Emulator-Rauchtest laeuft reproduzierbar durch.
9. Lizenz, Upstream-Revision und lokale Aenderungen sind dokumentiert.

## 10. Offene Detailentscheidungen

- genaue MWOS-Endungen und Schluesselwortliste fuer 68k-Assembler
- genaue xcc-Schalter, Bibliotheken und Startup-Objekte des vorhandenen
  OS-9/68030-Ports
- termcap-Farb-Erweiterungen gegen direkte ANSI-Farben pro Terminaltyp
- verfuegbare OS-9-GetStat-/SetStat-C-Library-Aufrufe fuer Raw-Modus und
  Terminalgroesse
- atomare oder bestmoeglich sichere Speicherstrategie auf RBF
- Grenzwert fuer maximal zu oeffnende Dateien bei knappem Speicher
- optionaler Langname `q9edit` als Alias oder zweites Modul
