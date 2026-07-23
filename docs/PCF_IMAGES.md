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
