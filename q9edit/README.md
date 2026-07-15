# q9edit

`q9edit` ist ein kleiner, moderner Vollbild-Texteditor fuer OS-9/68000 im
Q9-CB030-Emulator. Der installierte Programmname ist **`qe`**, damit der
taegliche Aufruf kurz bleibt:

```text
qe datei.c
```

## Ziel

- ANSI/VT100-Vollbilddarstellung auf `TERM=q9`
- Bedienung ueber Konsole und die virtuellen Terminals `/x1` bis `/x8`
- Suche und Speichern
- Syntaxfarben, zuerst fuer C und Assembler
- keine curses-Abhaengigkeit
- schmale OS-9-/termcap-Anpassungsschicht

Die verbindliche Produkt- und Technikbeschreibung steht in
[`docs/PROJEKTZIELE.md`](docs/PROJEKTZIELE.md).

Als Editor-Kern ist eine kontrollierte Portierung von
[Kilo](https://github.com/antirez/kilo) vorgesehen. Kilo ist in C geschrieben,
BSD-2-Clause-lizenziert und benutzt direkt VT100-Sequenzen statt curses.
Upstream-Code und lokale Anpassungen werden getrennt gehalten und zusammen mit
dem upstream Lizenztext aufgenommen.

## Geplante Struktur

```text
q9edit/
|-- README.md
|-- Makefile
|-- src/
|   |-- editor/       portierter Editor-Kern
|   `-- platform/     Terminal-, Datei- und Zeit-Anpassungen
|-- test/             Host- und Emulator-Tests
`-- tools/            Build-/Deployment-Helfer
```

## Namenskonvention

- Projekt und Quellverzeichnis: `q9edit`
- OS-9-Modul und Kommando: `qe`
- Langform `q9edit` kann spaeter optional als zweites Modul oder Shell-Alias
  angeboten werden.

## Entwicklungsweg

1. Auf dem Host bearbeiten und mit der MWOS-Toolchain fuer OS-9/68030 bauen.
   Verbindlicher Zielcompiler ist `xcc` im ANSI-C89-Modus.
2. Nur das Modul `qe` in ein festes Entwicklungsimage unter `/dd/CMDS`
   uebertragen; nie gleichzeitig mit dem Emulator auf das Image schreiben.
3. Im Emulator ueber ein virtuelles Terminal auf Port 2000 testen.
4. Bedienablauf und gespeicherte Datei automatisiert pruefen.

## Erste Meilensteine

1. Terminalprobe: Vollbild, Farben, Pfeiltasten, sichere Wiederherstellung.
2. Kilo-Kern importieren und auf eine Plattform-API umstellen.
3. OS-9-Dateizugriff und CR-Zeilenenden.
4. C-Syntaxfarben und Suche.
5. Automatisches Deployment und Emulator-Rauchtest.

## Dokumentation

- [`docs/PROJEKTZIELE.md`](docs/PROJEKTZIELE.md) — Ziele, Features,
  Technikentscheidungen und Abnahmekriterien
- [`STATUS.md`](STATUS.md) — kompakter, fortsetzbarer Arbeitsstand
- [`docs/SDK_BUILD.md`](docs/SDK_BUILD.md) — verifizierte xcc-/SDK-Buildfakten
- Dieses README — Einstieg, Verzeichnisstruktur und Entwicklungsweg
