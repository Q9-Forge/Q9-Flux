# Q9 Frame – Speicher- und Registerabbild

## Q9-Adressraum

```text
FD000000–FDFFFFFF   Video-RAM, maximal 16 MB
FE000000–FEFFFFFF   ROM-Bereich
FF000000–FFFFFFFF   reservierter I/O-Bereich
```

Die anfängliche VRAM-Größe ist konfigurierbar und beträgt standardmäßig 1 MB. Der Bereich ist vom normalen Q9-RAM getrennt. Eine Speicherverwaltung muss Überschneidungen mit RAM, ROM und I/O ablehnen.

## MC6845

Der MC6845 wird standardmäßig abgebildet ab:

```text
0xFFFFA000
```

Die Registeradressierung folgt dem klassischen Index-/Daten-Prinzip. Der genaue Zugriff wird bei der Integration in den bestehenden Q9-I/O-Dispatcher festgelegt.

Die Register R0–R15 bestimmen in der vereinfachten Emulation unter anderem:

- Zeichen- und Zeilenparameter
- sichtbare Breite und Höhe
- Raster-/Zeilenaufbau
- Display-Startadresse

Die initiale Implementierung verwendet Startadresse 0 innerhalb des VRAM. Eine CRTC-Adresse ist dabei ein VRAM-Offset, keine absolute CPU-Adresse.

## Buffer-Trennung

- VRAM und CPU-Zugriffe: emulierter Q9-Speicher
- Renderer- und Snapshot-Buffer: Host-Speicher
- Client-Sendewarteschlangen: Host-Speicher
- TCP-Socketpuffer: Betriebssystem
