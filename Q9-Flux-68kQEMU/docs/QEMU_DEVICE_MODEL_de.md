# QEMUs Geräte-Modell

*Englische Version: [QEMU_DEVICE_MODEL.md](QEMU_DEVICE_MODEL.md)*

Kurzer Überblick über QEMUs Bausteine, als Orientierung für die geplante
Umstellung von Musashi auf QEMU.

## QOM (QEMU Object Model)

QEMUs eigenes, in C nachgebautes Klassensystem (kein C++). Jeder
Gerätetyp ist ein `TypeInfo`-Struct mit `class_init`-/`instance_init`-
Funktionen, einem Elterntyp (Vererbung) und Struct-Größen für Instanz
und Klasse. Virtuelle Methoden laufen über Funktionszeiger-Vtables in
der Klassen-Struct — im Prinzip C++-Vererbung von Hand nachgebaut.

## qdev (Device Model)

Auf QOM aufgesetzt, speziell für Hardware-Geräte. Geräte erben von
`DeviceState`, deklarieren **Properties** (konfigurierbare Parameter —
hier würde z. B. ein Host-Pfad für den geplanten
[Hostfs-Manager](HOSTFS_MANAGER_de.md) reinpassen) und haben eine
`realize()`-Funktion, die läuft, sobald alle Properties gesetzt sind
(im Prinzip der eigentliche Konstruktor).

## MemoryRegion-API

Der eigentliche Kern für MMIO-Zugriffe: Ein Gerät legt eine
`MemoryRegion` an, gibt ihr Read/Write-Callback-Funktionen
(`MemoryRegionOps`, nehmen Adresse + Größe entgegen) und die Region wird
dann in den Adressraum des Gastsystems eingehängt. Greift die CPU auf
eine Adresse in diesem Bereich zu, ruft QEMU automatisch den
registrierten Callback auf.

Funktional dasselbe Prinzip wie das aktuelle Musashi-Trap-and-Emulate
(`src/devices/` + `devreg`/`devdesc`-Registry), nur in ein formalisiertes
API gegossen statt handgeschrieben.

## Board-/Machine-Datei

`hw/<arch>/<board>.c` instanziiert und verdrahtet alle Geräte für ein
konkretes Board — entspricht der Rolle, die bei uns heute
`q9board.c`/`boardcfg.c` spielen.

## Bereits installierte m68k-Maschinen (Stand 2026-09-15, QEMU 11.1.1)

```
an5206               Arnewsh 5206
mcf5208evb           MCF5208EVB (Standard)
next-cube            NeXT Cube
q800                 Macintosh Quadra 800
virt                 QEMU M68K Virtual Machine
```

Keine davon entspricht dem eigenen CB030-/Vinculum-Zielboard — ein
eigener QEMU-Maschinentyp wird nötig sein, analog zu `q9board.c` im
Musashi-Zweig.

## Zwei vorhandene QEMU-Features als Referenzpunkte

Bevor der Hostfs-Manager sein eigenes Protokoll entwirft, lohnt sich ein
Blick auf zwei bereits vorhandene, thematisch sehr nahe QEMU-Bausteine —
nicht zum Code-Übernehmen (QEMU ist GPL-lizenziert), sondern um
frühzeitig zu sehen, was die eigene Planung eventuell übersieht:

- **`vvfat`** — ein QEMU-Blockgerät, das ein Host-Verzeichnis als
  virtuelles FAT-Image präsentiert, rein in Software, ohne echte
  Image-Datei. Konzeptionell verwandt, nur auf FAT-Ebene statt einem
  eigenen Protokoll.
- **`virtio-9p`** — QEMUs Standardmechanismus für Host-Ordner-Freigaben
  in "echten" VMs: ein Gast-seitiger 9P-Protokoll-Treiber spricht über
  eine virtio-Queue mit dem Host, der die eigentlichen
  Open/Read/Write/Readdir-Aufrufe macht. Im Kern genau das eigene
  Manager/Treiber-Konzept — schon fertig durchdacht und im Feld bewährt.
