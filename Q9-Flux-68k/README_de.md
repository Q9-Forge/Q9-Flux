# Q9-Flux 68k

*Englische Version: [README.md](README.md)*

Q9-Flux 68k ist ein 68030-Hardware-Emulator für echtes OS-9/68K und Teil der
[Q9-Forge](https://github.com/Q9-Forge)-Projektfamilie.

Der Emulator verwendet den 68000-Prozessorkern
[Musashi](third_party/musashi/) und bildet die Q9-Board-Hardware nach,
einschließlich RAM/ROM, Remapping, 68681 DUART, CompactFlash-/RBF-Speicher,
QUICC-Ethernet und Echtzeituhr. Ziel ist es, unveränderte OS-9/68K-Software
zu booten und auszuführen.

## Bauen und starten

```sh
make host
./build/<platform>/q9.exe <config.q9>
```

Das vollständige Handbuch steht in
[`docs/HANDBUCH_de.md`](docs/HANDBUCH_de.md). Lokale Emulator-Images,
private Profile und Archive werden absichtlich von Git ignoriert.

## Status

Der 68k-Emulator ist derzeit die ausgereifteste Q9-Flux-Komponente. Weitere
Boards und Gerätemodelle sind geplant.

## Lizenz

Siehe [LICENSE](LICENSE) und die Dokumentation zu den verwendeten
Drittkomponenten.
