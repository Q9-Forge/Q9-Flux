#═════════════════════════════════════════════════════════════════════════════════════════════════
# File:   README.md                                                                       Ver. 1.00
# Owner:  AF
# Desc.:  Kurzbeschreibung fuer Q9-Userland: libq9, Beispiel-Tools und nativer Testharness.
#
# Edition History
#─────────┬──────┬─────────────────────────────────────────────────────────────────────────┬──────
# Date    │ Ver. │ Description                                                             │ By
#─────────┼──────┼─────────────────────────────────────────────────────────────────────────┼──────
# 26-07-04│ 1.00 │ Initiale Version                                                        │ CX
# 26-07-04│ 1.01 │ q9dir beschrieben                                                       │ CX
# 26-07-04│ 1.02 │ q9mkdir und q9rm beschrieben                                            │ CX
# 26-07-04│ 1.03 │ q9touch/q9stat und Testuebersicht ergaenzt                              │ CX
# 26-07-04│ 1.04 │ main()-Einsprungpunkte fuer spaetere 68k-Module dokumentiert            │ CX
#═════════╧══════╧═════════════════════════════════════════════════════════════════════════╧══════

# Q9 Userland

Dieses Verzeichnis enthaelt eine kleine C-Bibliothek `libq9`, die die aktuell
implementierten Q9-Syscalls als C-Funktionen kapselt. Die Beispiel-Tools benutzen
nur diese Bibliothek und rufen `q9_syscall()` nicht direkt auf.

## Tools

- `q9cat_run(path)` liest eine Datei komplett und schreibt ihren Inhalt auf Pfad 1.
- `q9copy_run(src, dst)` kopiert eine Datei innerhalb des Q9-Dateisystems.
- `q9dir_run(path)` oeffnet ein FAT16-Verzeichnis ueber `q9_open()` und liest die
  rohen 32-Byte-Directory-Eintraege per `q9_read_exact()`. LFN-Fortsetzungen,
  geloeschte Slots und Volume-Label werden uebersprungen; gueltige Eintraege
  werden als `DIR <Groesse> <Name>` oder `FILE <Groesse> <Name.EXT>` per
  `q9_writln()` auf Pfad 1 ausgegeben.
- `q9mkdir_run(path)` legt ein Verzeichnis ueber `q9_makdir()` an und bestaetigt
  den Erfolg kurz auf Pfad 1.
- `q9rm_run(path)` loescht eine einzelne Datei ueber `q9_delete()` und gibt nur
  bei Erfolg eine kurze Bestaetigung aus.
- `q9touch_run(path)` legt eine leere Datei ueber `q9_create()` an; existiert der
  Pfad bereits, wird er nur geoeffnet und wieder geschlossen.
- `q9stat_run(dir_path, name)` sucht einen 8.3-Namen in einem Directory und gibt
  bei Fund dieselbe eine Zeile wie `q9dir_run()` aus.

Fehlercodes werden von allen Tools unveraendert an den Aufrufer zurueckgegeben.

Dies ist vorbereitende Bibliotheks-/Tool-Logik und noch keine ladbaren
Q9-Module; die offene Einbindung ist in `PROJECT.md` unter O6 beschrieben.

## main()-Einsprungpunkte (Vorbereitung fuer 68k)

Zu jedem Tool gibt es zusaetzlich eine Datei `q9<name>_main.c` mit einer Funktion
`q9<name>_main(int argc, char **argv)`. Diese Funktionen sind duenne
Kommandozeilen-Huellen um die bestehenden `q9<name>_run()`-Funktionen: sie
parsen Positionsargumente, geben bei falscher Anzahl eine kurze Usage auf Pfad 1
aus und behandeln `-h`/`--help`. `q9copy_main()` kennt ausserdem
`-v`/`--verbose`.

Das ist nur Vorbereitung fuer eine spaetere 68k-Kompilierung, voraussichtlich
mit vbcc; diese Toolchain ist aktuell nur auf Andreas' Windows-Rechner
verfuegbar. Q9 hat derzeit noch keinen Mechanismus, echte 68k-Userland-Module
zu laden und auszufuehren. Es werden hier daher keine eigenstaendigen
Host-Kommandos gebaut und es gibt kein echtes `int main()` pro Tool.

Die native Testabdeckung ueber den Harness bleibt der einzige Weg, diese
Struktur jetzt zu verifizieren: der Harness bootet den echten Kernel und ruft
die `q9<name>_main()`-Funktionen mit synthetischen `argc`/`argv`-Arrays auf.

## Test

Aus dem Projekt-Root:

```sh
userland/build.sh
```

Erwarteter Stand: `userland_test: PASS`. Das Skript legt Build-Artefakte nur
unter `userland/build/` ab; das Laufzeit-Image heisst wie bei der nativen HAL
`q9disk.img`.

Der Test baut ein natives Host-Programm, linkt es direkt mit den Kernel-Quellen
und der POSIX-HAL, erzeugt ein minimales FAT16-Image und startet dann
`q9_hal_init()` + `q9_kernel_init()`. Geprueft werden libq9-Basisaufrufe,
Dateiinhalt-Kopie, Directory-Listing, Verzeichnisanlage, Loeschen, Touch einer
Null-Byte-Datei, Stat-Treffer/-Fehlerfall und ausgewaehlte argc/argv-
Einsprungpunkte inklusive Usage, Help und q9copy-Verbose.
