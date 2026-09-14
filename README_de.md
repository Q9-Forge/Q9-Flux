# Q9-Flux

*English version: [README.md](README.md)*

Q9-Flux ist die Emulator- und Hardware-Virtualisierungsprojektfamilie für
Q9.

## Komponenten

- `Q9-Flux-68k/` — 68000-/OS-9-Emulator und Board-Modell
- `Q9-Flux-x86/` — Arbeiten am x86-/OS9000-Emulator
- `Q9-Flux-Devices/` — gemeinsame Geräteprojekte wie Q9-Frame

Die architekturabhängige Implementierung bleibt im jeweiligen
Komponentenverzeichnis. Lokale Emulator-Images, private Profile und Archive
werden ignoriert und nicht eingecheckt.

## Lizenz

Der eigene Code von Q9-Flux steht unter der [MIT-Lizenz](LICENSE). Mehrere
eingebundene oder referenzierte Komponenten Dritter (CPU-Emulationskerne,
Netzwerkbibliotheken, QEMU als externer Prozess für Q9-Flux-x86, und
weitere) behalten ihre eigene, ursprüngliche Lizenz — die vollständige
Liste steht in [THIRD-PARTY-LICENSES_de.md](THIRD-PARTY-LICENSES_de.md).
