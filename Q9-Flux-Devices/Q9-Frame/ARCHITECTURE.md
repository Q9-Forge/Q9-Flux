# Q9 Frame – Architektur

## Komponenten

```text
Q9 Flex Emulator
├── Video Device
│   ├── VRAM
│   ├── MC6845-Register
│   └── Dirty-Tracking
├── Video Renderer
│   └── gepackter Ausgabe-Framebuffer
└── Host-Service-Manager
    ├── Terminal-Service, TCP 2000
    ├── Video-Service, eigener TCP-Port
    └── UDP-Discovery
```

Das Video-Device bleibt vom Netzwerk unabhängig. Ein CPU-Schreibzugriff markiert nur den betroffenen Bereich. Der Host-Service verarbeitet Änderungen im normalen Emulator-Loop und darf CPU-Emulation oder Buszugriffe nicht durch blockierende Netzwerkoperationen anhalten.

## Zeitbasis

Der MC6845-Takt ist ein konfigurierbarer Hardwareparameter. Eine zyklusgenaue CRTC-Timingsimulation ist zunächst nicht vorgesehen; Geometrie und Ausgabe werden aus den Registern R0–R15 abgeleitet.

Die Netzwerk-/Anzeigeaktualisierung verwendet eine unabhängige Host-Zeitbasis. Der Startwert für `update_hz` ist 30 Hz. Ohne Dirty-Bereiche wird kein Bild übertragen.

## Clientzustand

Der Framebuffer wird nur einmal erzeugt. Jeder Client besitzt lediglich eigenen Sitzungszustand:

- Protokollversion und Fähigkeiten
- zuletzt bestätigte Format- und Frame-Version
- ausstehende Updates
- begrenzte Sendewarteschlange

Ein neuer oder wieder verbundener Client erhält Format und vollständigen Frame. Bei langsamen Clients dürfen veraltete Updates zusammengefasst oder verworfen werden; anschließend wird ein vollständiger Frame angeboten.

## Videomodi

Der Server bestimmt über `VIDEO_INFO` allein, in welchem Modus der Client zeichnet (`bpp` + `mode`, siehe `Q9VideoMode` in `framebuffer.h`). Die Anzeige passt sich vollständig dem Server an, keine Client-seitige Annahme.

| bpp | Modus(e) | Farben | CLUT |
|---|---|---|---|
| 1 | `INDEXED1` | 2 | ja, 2 Einträge |
| 2 | `INDEXED2` | 4 | ja, 4 Einträge |
| 4 | `INDEXED4` | 16 | ja, 16 Einträge |
| 8 | `INDEXED8` | 256 | ja, 256 Einträge (Graustufen ist nur eine bestimmte Palettenbelegung, kein eigener Modus) |
| 16 | `RGB565` | direkt | nein |
| 16 | `RGB555I` | direkt + globales Intensity-Bit | nein |
| 24 | `RGB888` | direkt | nein |

Bei 16 Bit lohnen sich zwei Varianten, weil sich 16 Bit nicht glatt auf 3 Farbkanäle aufteilen lässt (16/3 = 5,33): `RGB565` nutzt das übrige Bit als zusätzliches Grün-Bit (Standard), `RGB555I` lässt es als globalen Helligkeits-Multiplikator für alle drei Kanäle übrig (historisch z.B. Amiga). Bei 24 Bit (8-8-8) bleibt kein Bit übrig, daher keine Variante nötig.

Die CLUT ist unabhängig vom `bpp` einheitlich als Tabelle mit 256 Einträgen à 24 Bit definiert; niedrigere bpp-Modi nutzen nur die ersten `2^bpp` Einträge.

## Skalierung

Die Ausgabegröße des Emulators bleibt unverändert. Desktop-Clients skalieren lokal mit Nearest Neighbor, bevorzugt ganzzahlig und unter Beibehaltung des Seitenverhältnisses. Der ESP32-Client kann Dirty-Rechtecke zeilenweise auf sein Display übertragen.
