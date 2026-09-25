# Host-Passthrough-Dateisystem-Manager — Architekturentwurf

*Englische Version: [HOSTFS_MANAGER.md](HOSTFS_MANAGER.md)*

Entstanden am 2026-09-15 aus der Frage, wie man das RBF-Konvertier-
Theater beim Testen umgeht. Ursprünglich als reiner Architekturentwurf
festgehalten, für eine spätere Umsetzungssession nach der
[Musashi→QEMU-Umstellung](../README_de.md) — **Schritt 1 (Musashi) ist
seit 2026-09-25 umgesetzt**: das MMIO-Gerät existiert und ist verifiziert
(`Q9-Flux-68k/src/devices/dhf/`, `make test-dhf`), s. Status-Tabelle in
`Q9-OS/Q9-DHF-68k/STATUS.md`. Offen: das echte OS-9-Treibermodul, das
dieses Gerät tatsächlich anspricht (braucht QCCs `driver`-Aufrufkonvention,
noch nicht gemergt), sowie Schritt 2 (QEMU) weiterhin unangefangen.

## Warum kein RBF/PCF/NFS/FTP/Samba

RBF (das native OS-9-Dateisystem) erfordert eigene Konvertierwerkzeuge,
weil sein On-Disk-Format (LSN/Allocation-Map) kein Host-Tool direkt
versteht. PCF (liest/schreibt FAT) wurde in der Vergangenheit schon
versucht und nicht zum Laufen gebracht. NFS, FTP und Samba wurden
ebenfalls alle schon einmal für Q9-Flux versucht — keins hat
funktioniert (stehen mit "?" markiert in
`.github/ROADMAP.md`/`ROADMAP_de.md`: "FTP-Support, Issue (FTP nutzt
noch den alten TCP-Socket-Pfad)" / "NFS-Support, Issue" /
"Samba-Support, Issue"). Das passt zum bekannten `telnetd`-Hänger-Fund
(fehlendes `SS_SEvent` im SCF-Treiber, dokumentiert in
`Q9-Flux-68k/docs/ARBEITSPLAN_de.md`) — der TCP/Netzwerk-
Anwendungsschicht-Pfad auf dem emulierten OS-9 scheint grundsätzlich
zickig zu sein. **Empfehlung: NFS/FTP/Samba nicht weiterverfolgen.**

## Die Grundidee: ein eigener Manager, kein neues Netzwerkprotokoll

OS-9-Schichtung ist Manager → Driver → Descriptor. RBF und PCF sind
beide Manager (Standard-Dateisystem-API), die intern ein On-Disk-Format
decodieren. Ein neuer Q9-Manager würde dieselbe Standard-Schnittstelle
bedienen, aber ohne eigenes Format-Parsing — er reicht Aufrufe an einen
Driver durch, der sie in ein einfaches Kommandoprotokoll übersetzt
("öffne Pfad X", "lies N Bytes", "liste Verzeichnis").

### Rechtlicher Hinweis

Das Buch *OS-9 Insights, 3rd Edition* (Microware) enthält in Anhang C
("Building a File Manager", ab Seite 519) einen kompletten
PC-DOS-kompatiblen File-Manager im Sourcecode. Dieser trägt einen
expliziten Copyright-Vermerk ("proprietary confidential property of
Microware... Reproduction, publication, or distribution... strictly
prohibited"). Urheberrecht schützt auch Struktur/Aufbau, nicht nur den
wörtlichen Text — ein Umschreiben/Verschleiern des Codes ändert nichts
an der Eigenschaft "abgeleitetes Werk". **Das Buch darf als private
Lernquelle für die ohnehin vom OS-9-Kernel vorgegebene (und offiziell
dokumentierte, nicht proprietäre) Schnittstelle dienen — der eigene
Manager muss komplett neu geschrieben werden, nicht aus dem Buch-Code
abgeleitet.**

## Die 13 Standard-Manager-Einstiegspunkte

Offizielle OS-9-Kernel-Sprungtabelle (reine Schnittstellen-Konvention,
nicht proprietär):

| # | Funktion | Aufgabe |
|---|---|---|
| 1 | `Create` | neue Datei anlegen |
| 2 | `Open` | vorhandenen Pfad öffnen |
| 3 | `MakDir` | neues Verzeichnis anlegen |
| 4 | `ChgDir` | aktuelles Default-Verzeichnis wechseln |
| 5 | `Delete` | Datei/Verzeichnis löschen |
| 6 | `Seek` | Byte-Position im Pfad setzen |
| 7 | `Read` | Bytes lesen |
| 8 | `Write` | Bytes schreiben |
| 9 | `ReadLn` | zeilenweise lesen (bis CR oder Zählergrenze) |
| 10 | `WriteLn` | zeilenweise schreiben |
| 11 | `GetStat` | Status/Zusatzinfo abfragen |
| 12 | `SetStat` | Status/Zusatzinfo setzen |
| 13 | `Close` | Pfad schließen |

Einsprung-Konvention: `a1` zeigt auf den Path Descriptor, `a4` auf den
Process Descriptor, `a5` auf den User-Registersatz, `a6` auf den
System-Global-Bereich.

Für den Passthrough-Manager sind die meisten davon nur dünne
Übersetzer, die den Aufruf + Pfad/Datenzeiger in das GetStat/SetStat-
Kommandoprotokoll zum Driver packen. `GetStat`/`SetStat` selbst werden
der interessanteste Teil.

## Verzeichnis-Listing: synthetische RBF-Records (Option A)

Damit Standard-OS-9-Tools (`dir`, `copy`, `list`, `del`, `makdir`)
unverändert funktionieren, muss der Manager bei `I$Read` auf einen
geöffneten Verzeichnis-Pfad intern per GetStat/SetStat den nächsten
echten Host-Verzeichniseintrag vom Driver holen und daraus einen
synthetischen RBF-ähnlichen Directory-Record bauen. Nach außen sieht
alles wie ein normales Verzeichnis aus; der GetStat-Kanal bleibt
komplett Manager↔Driver-intern.

(Alternative "Option B" — eigene Tools statt Standardbefehle — wurde
erwogen und verworfen, weil Standardkompatibilität das eigentliche Ziel
ist.)

## Mehrere Laufwerke / Host-Pfad-Zuordnung

Der Host-Pfad (wo das jeweilige virtuelle Laufwerk beginnt) steckt als
zusätzliches Descriptor-Feld (Options-Bereich, analog zu
`descriptor`/`descriptorName` in `devschema.h`). Mehrere Laufwerke =
mehrere Descriptor/Driver-Paare mit je eigenem Host-Pfad + eigener
MMIO-Basisadresse — exakt das bestehende `cf`/`nettty`-Muster
(`devreg`/`devdesc`-Registry), kein neuer Mechanismus im
Kernel/Registry nötig.

**Wichtig für Portabilität:** Der Host-Pfad sollte relativ zur
Config-Datei auflösbar sein, analog zu `cfg_resolve_rel()`/
`cfg_relativize()` für ROM-Pfade in `boardcfg.c` — sonst bricht die
Portabilität einer `.q9`-Config auf einem anderen Rechner/Verzeichnis
(ein absoluter Pfad in der Config liefe dort ins Leere).

## Nebenläufigkeit

Nicht vom Kernel/Manager-Rahmen automatisch gelöst — jeder Manager
(auch RBF selbst) muss geteilte Ressourcen selbst schützen. Path
Descriptors sind pro Prozess getrennt (sicher), aber die geteilte
Geräte-Static-Storage (Host-Pfad, MMIO-Kommandokanal) ist es nicht.

**Lösung:** `F$Event`/`Ev$Wait`/`Ev$Signal` als Mutex um den
Host-Roundtrip (dieselbe Primitive wie beim `telnetd`-SS_SEvent-Fund).
Kein Interrupt-Sperren, da der Roundtrip "echte Zeit" braucht — andere
Prozesse sollen währenddessen weiterlaufen können.

Datei-/Record-Locking (zwei Prozesse, dieselbe Datei) ist bewusst
zurückgestellt — selbst die Buch-PCFM hat das nicht.

### Verzeichnisüberwachung als Locking-Ersatz: geprüft und verworfen

Idee war, per FSEvents/inotify/`ReadDirectoryChangesW` einen ganzen
Host-Verzeichnisbaum zu beobachten und darüber einen Mutex zu steuern.
**Grundproblem:** Verzeichnisüberwachung ist reaktiv (meldet
Änderungen, nachdem sie passiert sind, teils zusammengefasst/verzögert)
— ein echter Mutex braucht aber ein Vorher-Versprechen. Kann eine Race
Condition bestenfalls melden, nicht verhindern. Zusätzlich uneinheitlich
zwischen Plattformen: Linux' `inotify` kennt echte
`IN_OPEN`/`IN_CLOSE`-Events, macOS FSEvents und Windows'
Change-Notifications melden im Wesentlichen nur Inhaltsänderungen.

Zwei Probleme sauber getrennt:

1. **Zwei OS-9-Gastprozesse gleichzeitig** — bereits gelöst durch den
   eigenen Driver + `F$Event`-Mutex, kein Host-Mechanismus nötig (beide
   Zugriffe laufen zwangsläufig durch den eigenen Code).
2. **Ein Host-Programm greift gleichzeitig auf dieselbe Datei zu**
   (z. B. ein Texteditor am Entwicklungsrechner) — dasselbe ungelöste
   Problem wie bei VirtualBox Shared Folders/Docker Bind-Mounts/
   Netzwerkfreigaben. Echte OS-Locks (`flock`/`fcntl`/`LockFileEx`)
   sind nur *advisory* — schützen nur gegen andere Programme, die den
   Lock aktiv prüfen. **Entscheidung:** als bekannte, dokumentierte
   Einschränkung akzeptieren, optional advisory Lock setzen solange ein
   Pfad offen ist. Verzeichnisüberwachung höchstens als
   *Diagnose*-Feature ("Datei wurde extern geändert, Cache evtl.
   veraltet"), nicht als Zugriffsschutz.

## Attribut-Handling

### Das "Single"-Bit

OS-9-Dateien haben ein "Single"-Attribut (nur ein gleichzeitiger Open
erlaubt). Host-Dateisysteme kennen kein entsprechendes Konzept — nützt
also nichts direkt. Aber: Die Durchsetzung passiert bei OS-9 ohnehin
nicht magisch im Kernel oder auf Medienebene, sondern im Manager
selbst — auch RBF führt dafür nur eigene Buchführung (offene
Datei-Deskriptoren mit Zähler) und verweigert den zweiten Open bei
gesetztem Single-Bit. Der eigene Manager muss also nur "ist Datei X
offen, ist Single gesetzt?" in derselben (ohnehin schon durch
`F$Event` geschützten) geteilten Gerätestatik mitführen.

### Wo die Attribut-Bits bei echtem RBF stecken

Nicht im Directory-Eintrag selbst — der enthält nur Dateiname + LSN
(Zeiger auf den File-Descriptor(FD)-Sektor). Das Attribut-Byte (inkl.
Single, Read/Write/Execute/Public/Directory) steckt im FD-Sektor,
zusammen mit Owner-ID, Link-Zähler, Größe, Segmentliste und
Zeitstempeln — eine Indirektionsstufe tiefer.

**Praktische Folge:** Da es keine echten LSNs/FD-Sektoren gibt, muss der
eigene Manager pro Host-Datei eine synthetische FD-artige Struktur
mitführen (Attribut-Byte inkl. Single-Flag, Größe, Datum) und über die
passenden GetStat-Aufrufe rausgeben (für Tools wie `attr`/`dsave`, die
bei echtem RBF auch nur über GetStat gehen, nicht roh vom Datenträger
lesen). Das ist dieselbe Datenstruktur wie die Open-Buchführung für die
Single-Bit-Durchsetzung — kein Zusatzaufwand, ein Zustand für zwei
Zwecke.

## Cross-Platform-Anforderung: Mac, Linux, Windows

Q9-Flux zielt ohnehin auf alle drei Plattformen. Nur die Emulator-Seite
ist betroffen — die OS-9-Gast-Seite (Manager + Driver) ist vom
Emulator-Kern unabhängiger 68K-Code.

- **Case-Sensitivität**: OS-9 ist wie APFS und NTFS "case-insensitive,
  aber case-preserving" (`chd cmds` findet ein angelegtes `CMDS`) —
  passt für Mac und Windows von allein. **Nur unter Linux** ist ext4 &
  Co. echt case-sensitiv — dort muss der Driver selbst case-insensitiv
  im Verzeichnis suchen statt sich aufs Host-Dateisystem zu verlassen.
  Bleibt außerdem die Unicode-Normalisierung (macOS NFD vs. OS-9
  simpler Bytevergleich).
- **Windows-spezifisch**: reservierte Namen (`CON`/`PRN`/`AUX`/`NUL`/
  `COM1`–`9`/`LPT1`–`9`) verboten, verbotene Zeichen (`< > : " | ? *`),
  historische 260-Zeichen-`MAX_PATH`-Grenze, und Dateien immer im
  Binärmodus öffnen (sonst übersetzt die Windows-CRT automatisch
  LF↔CRLF und verfälscht Bytes).
- **Verzeichnis-Scan-Logik nicht neu erfinden**: `q9_filelist.c`
  (Q9-Flux-Editor) scannt bereits plattformübergreifend (Mac/Linux/
  Windows) Name/Datum/Größe für den Dateidialog — möglicher
  Startpunkt/Referenz für den Host-Teil des neuen Drivers.

## Weitere Punkte, an die man nicht sofort denkt

- **Pfad-Escape verhindern**: `../../..` darf nicht aus dem
  konfigurierten Host-Wurzelverzeichnis rausführen.
- **Dateinamenlänge**: OS-9-Namen sind klassisch auf ~28/29 Zeichen
  begrenzt — vgl. den ToolShed-29-Zeichen-Bug aus der eigenen
  Projekthistorie.
- **Attribut-Bit-Mapping**: OS-9 Read/Write/Execute/Public/Directory
  vs. macOS-Rechte — das Executable-Bit war mit ToolShed schon einmal
  ein echter Bug (`os9 copy -r` setzt Owner-Execute nicht).
- **Zeitstempel-Umrechnung**: OS-9 gepacktes Datumsformat vs.
  Unix-Timestamps.
- **Fehlercode-Übersetzung**: ENOENT/EACCES/ENOSPC → passende
  OS-9-Fehlercodes (`E$PNNF`, `E$FNA`, …), sonst kriegen Programme
  unsinnige Fehlermeldungen.
- **Host-Filehandle-Leaks bei Prozessabbruch**: Path-Close-Cleanup muss
  den Host-Handle mitschließen.
- **Schreib-Robustheit**: sofort durchschreiben vs. puffern — Parallele
  zum bereits gefixten CF-Write-Bug (256-Byte-Sektoren).
- **Verzeichnis-Listing bei gleichzeitiger Host-seitiger Änderung.**
- **Fehlender/ungültiger Host-Pfad beim Init** → sauberer Attach-Fehler
  statt Absturz.
- **Später relevant, falls Host-Zugriff mal asynchron wird**: Solange
  alles synchron in der CPU-Step-Schleife läuft, kein Thema — bei
  späteren Performance-Optimierungen mit Hintergrund-I/O (um
  Emulation nicht bei langsamer Host-Platten-Operation einfrieren zu
  lassen) bräuchte die Emulator-Seite selbst zusätzliche
  Thread-Sicherheit.

## Wiederverwendbarkeit über mehrere Driver

Derselbe Manager, nur der Driver + das Transportprotokoll ändern sich:

1. **Q9-Flux-Emulator** (zuerst): Driver schreibt Kommandos in
   MMIO-Register (analog zum bestehenden `src/devices/`-Muster: `cf`,
   `quicc`, `rtc72421` usw.). Emulator-seitiges neues Gerät macht
   dahinter echte Host-Aufrufe (`open()`/`read()`/`opendir()`).
2. **CH375/CH376-Chip** (echte Hardware, später): Der Chip hat
   FAT12/16/32-Parsing bereits eingebaut und spricht selbst schon
   datei-basierte Kommandos (nicht nur rohe Sektoren) über
   UART/SPI/parallel — passt strukturell 1:1 zum selben
   Driver-Vertrag. Passend z. B. für das Vinculum-Board (eigene
   68360-Hardware) als einfache USB-Stick-Anbindung ohne eigene
   USB-/FAT-Implementierung.
3. **Teensy 4.1 / ESP32 als Co-Prozessor**: übernehmen FAT-Handling per
   Software-Bibliothek auf einer SD-Karte. Auch hier: nur ein weiterer
   Driver, kein neuer Manager. Denkbar später sogar ein ESP32 mit
   WLAN-Netzwerk-Share dahinter.

**Kernaussage:** Der Manager kennt am Ende nur einen einzigen simplen
Vertrag ("frag den Driver nach Datei X, kriegst Daten zurück"); der
"intelligente Partner" dahinter (Emulator, Chip, Co-Prozessor) macht das
eigentliche Dateisystem-Handling.

## QEMU-Referenzpunkte

Siehe [`QEMU_DEVICE_MODEL_de.md`](QEMU_DEVICE_MODEL_de.md) — insbesondere
`vvfat` und `virtio-9p` als bereits vorhandene, thematisch verwandte
QEMU-Bausteine, die sich vor dem eigenen Protokollentwurf lohnen
anzuschauen (als Inspiration, nicht zum Code-Übernehmen — QEMU ist
GPL-lizenziert).
