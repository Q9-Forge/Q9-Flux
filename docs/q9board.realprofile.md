# Q9 reales CF-Testprofil (5.19b)

Die lokalen Testartefakte liegen unter `local_images/` (proprietäre OS-9-Images und
Descriptoren werden nicht ins Repository eingecheckt).

| Gerät | Base | Format | Image | Descriptor | Host-Layout |
|---|---:|---|---|---|---|
| `c0` | `0xFFFFE000` | RBF | `OS9SYS.hda` | `c0` | Bootimage ab Sektor 0, Länge `0x800000` |
| `d0` | `0xFFFFE100` | PCF | `q9-d0-pcf-2gb.img` | `d0` | FAT16-Partition ab Hostsektor 64, Nutzlänge `0x3FF800` (2047 MiB) |
| `e0` | `0xFFFFE200` | PCF | `q9-e-shared-2gb.img` | `e0-pcf` | FAT-Partition ab Hostsektor 64, Länge `0x200000` |
| `e1` | `0xFFFFE200` | RBF | `q9-e-shared-2gb.img` | `e1` | RBF-Partition ab Hostsektor `0x200040` (1 GiB + 64), Nutzlänge `0x1FFFC0` |

`e0` und `e1` sind zwei Partition-Descriptoren derselben Hardware und desselben
Backings; sie sind nicht als Master/Slave modelliert. Die `PD_LSNOffs`-Werte der
Descriptoren sind entsprechend gesetzt: 0 für `c0`, 64 für `d0`/`e0` und `0x200040`
für `e1`. Die Config ist
`docs/q9board.realprofile.q9`. `start_sector=0` bleibt dort absichtlich stehen, weil
der OS-9-Descriptor den Partitionsoffset bereits an den Treiber weitergibt.

Zusätzlich liegen im Testimage die Format-Descriptoren `d0_fmt`, `e0_fmt` und `e1_fmt`.
`d0_fmt`/`e0_fmt` basieren auf dem PCF-Template `pcd0` (File Manager `pcf`),
`e1_fmt` auf dem RBF-Template `c0_fmt`. In allen drei ist `FmtDsabl` gelöscht;
Base, Start-LSN und Partitionslänge bleiben zum jeweiligen Daten-Descriptor identisch.

Die PCF-Descriptoren `d0` und `e0` verwenden die Q9-Anpassung **Hard + feste Geometrie**
und den separaten Testtreiber `cfidef` (Edition 43; der ROM-Treiber `cfide`
bleibt unverändert).
Das SDK-`pcd0` ist ursprünglich ein 1,44-MB-Disketten-Descriptor (`pcdos380`);
unverändert führt er bei den 2-GB-FAT16-Images zu `E$BTyp`. Die Descriptoren verwenden
jetzt 512 Byte, 63 Sektoren/Track und 255 Köpfe (d0: 261, e0: 131 Zylinder), passend
zur FAT-Geometrie. `dump /d0@` bleibt ein Rohzugriff und
zeigt deshalb weiterhin den 64-Sektoren-Vorspann; der reguläre Zugriff ist `dir /d0`.

Zusätzlich enthielt die MVME147-Vorlage im separaten `DevCon`-Feld noch `scsi147`.
Für Q9 muss dieses Feld ebenfalls `cfide` referenzieren; andernfalls lädt der
Descriptor zwar, aber `iniz d0` endet mit `E$MNF`.

Das zukünftige Profil reserviert `CF_BASE`, `CF_BASE+$100` und `CF_BASE+$200` für
drei CF-Fenster. `e0/e1` teilen sich das dritte Fenster als zwei Partitionen. Ein
späteres Master/Slave-Flag ist davon unabhängig und gehört zum separaten Hardware-Workitem.

## Format-/Offset-Teststand (2026-07-17)

Der ROM-Treiber `cfide` ignorierte beim ATA-Lese-/Schreibzugriff zunächst
`PD_LSNOffs`. Dadurch konnte ein Formatlauf zwar scheinbar erfolgreich enden,
schrieb bei `e1` aber am Image-Sektor 0 statt bei `0x200040`; bei `d0/e0` wurde
Sektor 0 statt des FAT-Beginns bei Hostsektor 64 gelesen (`E$BTyp`).

`tools/cfq9_format.a` enthält jetzt den Offset-Fix für `CF_Read` und `CF_Write`
und weiterhin den virtuellen `SS_Reset`/`SS_WTrk`-Erfolgspfad für
Format-Descriptoren. Das daraus gebaute `cfidef` ist Edition 43, CRC/parity-gültig
und liegt im Testimage unter `CMDS/BOOTOBJS/cfidef`. Nach dem Fix lesen sowohl
`dir /d0` als auch `dir /e0` die vorbereiteten FAT16-Testinhalte korrekt.

Der erste fehlerhafte `e1`-Formatlauf beschädigte den FAT-Anfang des gemeinsamen
Images. Der erste Bereich wurde aus `q9-e0-pcf-1gb.img` wiederhergestellt; vor einem
erneuten Formatlauf muss der neue `cfidef` geladen sein. `format /e1_fmt` ist der
RBF-Weg. `pcformat -f=3` verlangt dagegen formal ein physisches Fixed-Disk-Format;
`-np` ist mit Formatnummer 3 nicht zulässig. Für PCF/FAT bleiben die vorbereiteten
Images der verlässliche Ausgangspunkt.

`dcheck` gilt nur für RBF (`e1`). Für PCF/FAT werden `dir`, `free` sowie ein
Schreib-/Lese-Roundtrip verwendet. `dsave` kopiert rekursiv zuverlässig nach RBF;
bei PCF/FAT können lange Namen und OS-9-Attribute nicht vollständig übernommen
werden. Einzeldateien wie `AUTOEXEC.NEW` lassen sich auf `d0`/`e0` kopieren.
