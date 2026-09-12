# Q9 Frame Remote Client

Dieser Client verbindet sich per TCP mit einem Q9 Frame Emulator, empfängt Framebufferdaten und stellt sie mit SDL dar.

## Features
- UDP-Discovery und manuelle IP/Port-Eingabe
- Protokoll gemäß Q9 Frame-Spezifikation
- 1-Bit-Framebuffer-Darstellung
- Live-Updates (FRAME_UPDATE)
- Portabel (getestet auf macOS, Linux)

## Testmodus
Ein Dummy-Server (tests/dummy_server.cpp) simuliert einen Emulator für Entwicklung und Tests.

## Kompilieren
```sh
make
```

## Starten
```sh
./q9frame_client
```

## Testserver starten
```sh
g++ tests/dummy_server.cpp -o dummy_server
./dummy_server
```
