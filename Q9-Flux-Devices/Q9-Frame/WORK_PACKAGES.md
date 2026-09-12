# Q9 Frame – Arbeitspakete

## WP01 – Projektstruktur und Integration

- Q9 Frame in den Q9-Flex-Build integrieren
- gemeinsame Konfigurations- und Datentypen anlegen
- README und Dokumentation pflegen

## WP02 – VRAM und Speicherabbild

- VRAM bei `0xFD000000` anbinden
- konfigurierbare Größe mit Default 1 MB
- maximal 16 MB adressierbar
- Dirty-Tracking bei CPU-Schreibzugriffen
- Überlappungsprüfungen mit anderen Speicherbereichen

## WP03 – MC6845-Registermodell

- Register R0–R15 implementieren
- MMIO ab `0xFFFFA000`
- konfigurierbare MC6845-Taktfrequenz
- Registeränderungen als Videoformatänderung erkennen

## WP04 – Monochromer Framebuffer

- 1 Bit pro Pixel
- Bit 7 links
- linearer Stride
- Ausgabe aus VRAM und CRTC-Parametern
- Dirty-Rechtecke bilden und zusammenfassen

## WP05 – Host-Service-Manager

- gemeinsamer nicht-blockierender Service-Loop
- bestehende Terminal-Unterstützung berücksichtigen
- Start, Stop, Logging und Konfiguration der Dienste

## WP06 – Q9-Frame-TCP-Service

- `HELLO`, `VIDEO_INFO`, `FRAME_FULL` und `FRAME_UPDATE`
- Vollframe-Anforderung und Resynchronisierung
- begrenzte Sendewarteschlangen pro Client
- langsame oder getrennte Clients ohne Emulatorstillstand

## WP07 – UDP-Discovery

- Broadcast-Suche im lokalen Netz
- Unicast-Angebot mit TCP-Port und Videoformat
- Discovery abschaltbar und konfigurierbar

## WP08 – Desktop-Testclient

- Handshake und Vollframe
- Dirty-Updates
- Nearest-Neighbor-Skalierung
- Fenster-, Vollbild- und Minimierungsverhalten

## WP09 – Tests und Folgeclients

- VRAM-, CRTC- und Protokolltests
- Tests mit langsamen Clients und Verbindungsabbruch
- später ESP32-Client mit demselben Protokoll
- spätere Farbmodi sowie Tastatur-/Mauseingabe
