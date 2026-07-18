# OS-9 `pks`- und `telnetdc`-Fix fuer die Telnet-Port-23-Blockade

## Ziel

Der Fix betrifft den OS-9-Telnetpfad auf Port 23 (`telnetd`/`pks`), nicht
Q9s separaten Port-2000-Pfad (`/x1`..`/x8`, `nettty`).

Im Root-Modulmerge `/dd/netmods` wird das Modul `pks` gepatcht:

| Feld | Wert |
|---|---:|
| Merge-Offset des Moduls | `0x2714` |
| Modulgrösse | `124` Bytes |
| `PD_PAU`-Offset im Modul | `0x4f` |
| vorher | `0x01` |
| nachher | `0x00` |

`PD_PAU=0` deaktiviert die Seitenpause des SCF-Descriptors. Das ist eine
dauerhafte Descriptoränderung im Image; `pkdvr`, `pkman`, `telnetdc` und Q9s
Host-Treiber `nettty` werden dabei nicht ersetzt.

## Zweiter Fix: `telnetdc`

Auch `telnetdc` enthält einen echten Kompatibilitätsfehler im Zusammenspiel
mit dem verwendeten pks/SCF-Pfad. Bei `telnetdc` lag der timeoutlose
`Ev$Wait` bei Modul-Offset `0x0f4c`; dort wurde ein `SS_SEvent` erwartet, das
der Pseudo-Terminalpfad nicht signalisierte. Der 14-Byte-Patch ersetzt diesen
unbegrenzten Wartepunkt durch eine kurze `F$Sleep`-Poll-Schleife.

Das betrifft `/dd/CMDS/telnetdc`, nicht `/dd/netmods`. Im aktuellen
Testimage wurde die `telnetdc`-Edition von `218` auf `219` erhöht. Der
`telnetdc`-Patch ist ein echter Kompatibilitätsfix, aber kein vollständiger
Ersatz für den Treiber.

Nach jeder Änderung werden OS-9-CRC und Header-Parität neu erzeugt und
geprüft. Die getestete, klar benannte Image-Kopie liegt lokal als
`local_images/OS9SYS.pks___mit_patch_fuer_telnet_Blockade.hda` vor. Sie wird
nicht ins Repository eingecheckt, weil OS-9-Images gross/proprietär sind.

## Edition

Im klar benannten Testimage wurde die Edition des `pks`-Moduls von `6` auf
`7` und die Edition von `telnetdc` von `218` auf `219` erhöht. Beide Module
melden danach `Good CRC` und `Good parity`.

## Beobachtungen

- Port 23 zeigt nach dem Boot `tmode ... nopause`.
- Mehrere gleichzeitige `super`-Sessions laufen schnell durch.
- Ein `afoe`-Prozess mit Priorität `64` läuft gegenüber `super` mit Priorität
  `128` deutlich langsamer und holt nach dem Ende der höher priorisierten
  Prozesse auf. Das ist ein Scheduler-/Prioritätseffekt.
- Port 2000 war nicht der betroffene Pfad.

## Werkzeuge

- `tools/patch_pks_descriptor.py` – patcht `PD_PAU` und regeneriert CRC.
- `tools/build_pks_nopause_image.py` – erzeugt einen APFS-Klon und deployt den
  Patch nach `/dd/netmods`.
- `tools/set_pks_edition.py` – setzt die OS-9-Edition und regeneriert Parität
  und CRC.
- `tools/set_module_edition.py` – generisches Editions-Update für OS-9-Module.
- `test_pks_nopause.exp` – automatisierter Gasttest gegen den Patch-Klon.
