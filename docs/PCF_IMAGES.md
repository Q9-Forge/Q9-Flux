# PCF-/FAT-Images

`tools/q9fat.c` ist der portable C-Generator für FAT12/16-Images. Er kann
entweder eine Superfloppy oder ein Image mit integriertem MBR erzeugen:

```sh
make q9fat
build/tools/q9fat local_images/q9-pcf-mbr.img \
    --mb 64 --spc 4 --label Q9DATA --mbr --start-sector 64
```

Im MBR-Modus liegt der Aufbau so:

```text
LBA 0       MBR mit FAT16/CHS-Partition (Typ 0x06)
LBA 64      FAT-Bootsektor
LBA 65...   FAT, Root-Directory und Daten
```

Der ältere OS-9-Befehl `partdgen` akzeptiert den modernen MBR-Typ FAT16/LBA
(`0x0E`) nicht zuverlässig. `q9fat` verwendet deshalb den kompatiblen Typ
`0x06` mit gültigen CHS-Grenzwerten. Damit erkennt `partdgen -l d@` die
Partition ab Sektor 64 und analysiert anschließend FAT16.

Der FAT-BPB enthält `HiddenSectors=64`. Das passt zum Q9-PCF-Descriptor,
der den Partitionsoffset über `PD_LSNOffs` an den CF-Treiber weitergibt.
Der Generator schreibt keine vorhandenen Images um; neue Images sollten
zunächst unter einem neuen Namen erzeugt und mit `dir`, `free` sowie einem
Schreib-/Lesetest im Emulator geprüft werden.

Ein späterer OS-9-Port kann denselben Layout-Kern auf einem bereits
angeschlossenen Blockgerät verwenden. Er kann damit MBR/FAT auf `/d0` oder
`/e0` schreiben, aber keine neue Host-Datei im `local_images/`-Verzeichnis
anlegen — diese Datei sieht nur der Emulator-Host.

Für einen Whole-Disk-Descriptor `d` kann die klassische PCF-Geometrie direkt
aus der Imagegröße abgeleitet werden:

```sh
python3 tools/patch_pcf_descriptor.py local_images/d0 local_images/d \
    --name d --base 0xFFFFE100 --lsn 0 \
    --image local_images/q9-d0-pcf-mbr-test.img --driver cfidef
```

## Wichtige Erkenntnis: `@`-Rohzugriff braucht trotzdem einen passenden Header

Das `@`-Suffix (z.B. `partdgen -l d@`, `dump /c0@`) öffnet ein PCF-/RBF-Gerät
im rohen Sektor-Modus statt die normale Verzeichnisebene zu mounten — aber
es umgeht **nicht** die Plausibilitätsprüfung des jeweiligen File-Managers.
Getestet (2026-07-24): `dump /c0@` liefert den echten rohen LSN-0-Sektor
(RBF-Volume-Header, Volume-Name "OS9SYS" sichtbar), weil dort tatsächlich
ein gültiger RBF-Header liegt. Derselbe Zugriff auf ein Whole-Disk-Device,
dessen LSN 0 einen **MBR** statt eines RBF/PCF-Headers enthält, scheitert
weiterhin am Open — unabhängig davon, ob der Descriptor als `pcf` oder
`RBF` deklariert ist, und unabhängig von der Sektorgeometrie im Descriptor
(beides wurde einzeln getestet und ausgeschlossen).

**Konsequenz**: Ein MBR lässt sich mit dieser File-Manager-Kombination
*nicht* zuverlässig von innerhalb OS-9 lesen (weder `mbr` noch `partdgen`
noch `dump`, mit oder ohne `@`). Der praktikable Weg ist daher, `mbr` **auf
dem Host** direkt gegen die `.img`-Datei laufen zu lassen (siehe unten) und
den passenden Descriptor fertig gebaut auf das Image zu kopieren, statt die
Erkennung live in OS-9 zu versuchen.

## Host-seitiger Workflow (Descriptor bauen, prüfen, deployen)

### 1. Partitions-/Filesystem-Layout ermitteln

`os9/mbr/mbr.c` ist bewusst reines ANSI-C und lässt sich auch nativ auf dem
Host kompilieren (nützlich zum schnellen Prüfen, bevor überhaupt ein
OS-9-Descriptor gebaut wird):

```sh
cd os9/mbr
clang -Wall -Wextra -std=c89 -pedantic -o /tmp/mbr_hosttest mbr.c
/tmp/mbr_hosttest local_images/<image>.img
```

Zeigt MBR-Partitionen (Start/Größe/Typ) bzw. direktes FAT/RBF ab LSN 0 —
rein informativ, erzeugt noch keinen OS-9-Modul-Descriptor.

### 2a. PCF-Descriptor (FAT-Partition) — siehe Beispiel oben

`patch_pcf_descriptor.py` verlangt ein PCF-typisiertes Template
(`FileMgr=pcf`, akzeptiert `pcd0`/`d0`-Namen) und patcht Name, Base, LSN
und Geometrie hinein.

### 2b. RBF-Descriptor (Whole-Disk, kein FAT) — kein fertiges Skript

Für einen reinen RBF-Whole-Disk-Descriptor gibt es kein Tool (das
PCF-Skript akzeptiert nur `FileMgr=pcf`-Vorlagen). Stattdessen einen
bewährten, funktionierenden RBF-Descriptor (z. B. `c0`) als Rohbyte-Vorlage
nehmen und nur Name/Base/DevDrv patchen — alle Felder liegen an denselben
universellen Offsets wie bei PCF (`M$Name`-Zeiger bei `$0E`, `FileMgr` bei
`$38`, `DevDrv` bei `$3A`, `DevCon` bei `$3C`, `Base` bei `$30`), nur die
tatsächliche String-Position im Modul unterscheidet sich je nach Vorlage.
Anschließend Header-Parität und CRC neu berechnen (siehe `repair_module()`
in `mbr.c` als Referenzimplementierung).

### 3. Verifizieren vor dem Deploy

- Header-Parität: XOR aller 16-Bit-Wörter von Offset `0` bis `$2F`
  (**inklusive** des Paritätsfeldes selbst) muss `0xFFFF` ergeben.
- CRC: OS-9-CRC-24 über alle Bytes außer den letzten 3, Ergebnis in die
  letzten 3 Bytes schreiben.
- `ident <datei>` auf dem Image gibt "Good CRC"/"Good parity" aus, wenn
  beides stimmt.

### 4. Deployen (Emulator muss dafür gestoppt sein!)

Gleichzeitiges Schreiben von Host-Toolshed und laufendem Emulator auf
dasselbe Image beschädigt es — siehe `docs/OS9SYS_BOOT.md`.

```sh
os9 copy -r <neue-datei> local_images/OS9SYS.hda,CMDS/BOOTOBJS/<name>
os9 attr -e -pe local_images/OS9SYS.hda,CMDS/BOOTOBJS/<name>
```

### 5. Im laufenden OS-9 laden

```
attr -e -pe /dd/CMDS/BOOTOBJS/<treiber>
load /dd/CMDS/BOOTOBJS/<treiber>
attr -e -pe /dd/CMDS/BOOTOBJS/<descriptor>
load /dd/CMDS/BOOTOBJS/<descriptor>
iniz /<name>
```

Alle geladenen Module gehen bei jedem Emulator-Neustart verloren (nur im
RAM) — ein Prozedurfile mit der kompletten Ladesequenz spart hier viel Zeit.
