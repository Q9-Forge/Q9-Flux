# Q9 Frame – Protokollentwurf

## Transport

- Video: binäres TCP
- Discovery: UDP-Broadcast, Antwort bevorzugt per Unicast
- Mehrbytewerte: Big Endian
- TCP-Übertragung: nicht-blockierend und mit begrenzter Client-Warteschlange

## Nachrichtenfluss

```text
Client → HELLO
Server → VIDEO_INFO
Server → FRAME_FULL
Server → FRAME_UPDATE ...
```

Ein Client kann jederzeit `REQUEST_FULL_FRAME` senden. Bei einer Änderung von Geometrie, Stride, Bitmodus oder Farben folgen erneut `VIDEO_INFO` und `FRAME_FULL`.

## Nachrichtentypen

- `HELLO`: Protokollversion und Client-Fähigkeiten
- `VIDEO_INFO`: sichtbare Auflösung, Stride, Bits pro Pixel, Bitreihenfolge, Farben und Formatversion
- `FRAME_FULL`: kompletter sichtbarer, gepackter Framebuffer
- `FRAME_UPDATE`: ein oder mehrere geänderte byte-ausgerichtete Rechtecke
- `REQUEST_FULL_FRAME`: erneute Vollsynchronisierung
- `PING` / `PONG`: Verbindungsprüfung
- `ERROR`: protocol- oder formatbezogener Fehler

Ein möglicher feste Nachrichtenkopf:

```text
Magic       4 Bytes   "Q9VF"
Version     2 Bytes
Type        2 Bytes
Length      4 Bytes
Sequence    4 Bytes
```

## Bilddaten

Der Startmodus ist `mono1`:

- 1 Bit pro Pixel
- Bit 7 ist das linke Pixel
- `stride = ceil(width / 8)`
- Daten werden zeilenweise übertragen
- X-Bereiche werden auf vollständige Byte-Spalten aufgerundet

Ein Update beschreibt deshalb bevorzugt:

```text
x_byte_start
byte_count
y_start
row_count
byte_count × row_count Bytes
```

## Discovery

Der Client sendet `Q9_VIDEO_DISCOVER` als UDP-Broadcast. Der Emulator antwortet mit Name, Protokollversion, TCP-Port, verfügbaren Modi sowie aktueller sichtbarer Auflösung. Discovery ist konfigurierbar und abschaltbar.

Tastatur- und Mausnachrichten bleiben für eine spätere Protokollerweiterung reserviert.
