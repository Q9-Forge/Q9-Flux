# Q9 Frame

Der gemeinsame projektübergreifende Kontext und die verbindlichen Namen
stehen im übergeordneten Q9-Forge-Arbeitsbereich.

Q9 Frame ist das Videosubsystem für Q9 Flex unter dem übergeordneten Projekt Q9 Forge.

Es umfasst zunächst:

- getrennten Video-RAM
- eine MC6845-Registeremulation
- einen gepackten monochromen 1-Bit-Framebuffer
- Dirty-Tracking bei VRAM-Schreibzugriffen
- einen externen TCP-Video-Service
- UDP-Discovery für aktive Emulatoren

Der erste Video-Modus verwendet ein Bit pro Pixel. Bit 7 ist das linke Pixel eines Bytes. Der externe Client erhält denselben gepackten Aufbau wie der interne Ausgabe-Framebuffer.

## Projektstruktur

```text
Q9-Forge/
└── Q9-Frame/
```

Q9 Flex bleibt für CPU, Speicher und Geräteintegration zuständig. Q9 Frame stellt die Video-Funktion und den Host-Service bereit. Desktop- und ESP32-Viewer sind eigenständige Clients.

Weitere Festlegungen stehen in [ARCHITECTURE.md](ARCHITECTURE.md), [PROTOCOL.md](PROTOCOL.md), [MEMORY_MAP.md](MEMORY_MAP.md) und [WORK_PACKAGES.md](WORK_PACKAGES.md).
