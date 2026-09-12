# Recherche: OS-9000/x86-Compiler und ELF-Konvertierung

Stand: 2026-09-08. Ziel ist eine belastbare Toolchain für 32-Bit-OS-9000/x86-Module.

## Ergebnis

Einen frei verfügbaren, aktuellen OS-9000/x86-Compiler mit vollständiger Toolchain konnte ich nicht finden. Historisch gab es genau dafür Microwares OS-9000/386-Entwicklungsumgebung mit Ultra C, Assembler, Linker und Debugger. Die Produktunterlagen belegen die Existenz, aber keine frei zugängliche Downloadquelle.

Der wichtigste Fund ist ein echter `i386os9k`-BFD-Backend im historischen GNU-binutils-2.10-Zweig. Er erkennt OS-9000/i386-Module und beschreibt den OS-9000-Header (`include/os9k.h`). In der gefundenen Version sind die Schreibpfade jedoch mit `#if 0` deaktiviert. Das ist eine sehr gute Formatbasis, aber kein fertiger ELF→OS-9000-Konverter.

## Originale Microware-Toolchain

- Microware beschreibt OS-9000 als portable C-basierte Variante für Intel 386+ einschließlich x86-Unterstützung: [OS-9000 FAQ](https://www.microware.com/index.php/support?faqid=2&view=faq)
- Historische OS-9000/386-Werbung nennt eine integrierte C-Compiler-, Debugger- und Entwicklungsumgebung auf 80386-PCs: [Microware OS-9000/386-Anzeige](https://colorcomputerarchive.com/repo/Documents/Ads/Microware/Microware%20Ads%20Collection.pdf)
- Die Produktliste nennt ein `PC Development Pak` für X86 mit Ultra C; das Run-Time-Pak enthält keine Entwicklungstools: [OS-9 Family Produktübersicht](https://os9projects.com/CD_Archive/PRODUCTS/MICROWARE/OS9Prices_0395.pdf)
- Die Ultra-C-Dokumentation bestätigt die 80x86-Familie und den Microware-spezifischen ABI-/Bibliotheksumfang: [Ultra C Library Reference](https://manualzilla.com/doc/5746197/ultra-c-library-reference)

Bewertung: Die Originaltoolchain ist historisch eindeutig belegt, aber eine aktuelle, öffentlich zugängliche Binärdistribution für OS-9000/x86 wurde nicht gefunden. Archive enthalten vor allem Dokumentation.

## Historischer GNU-binutils-Support

Im öffentlichen Fuchsia-Mirror des GNU-binutils-2.10-Zweigs existieren:

- `include/os9k.h` mit `MODSYNC 0x4afc` für 80386, `MODREV 1`, `CRCCON 0x800fe3` und dem 80-Byte-Common-Header: [os9k.h](https://fuchsia.googlesource.com/third_party/binutils-gdb/+/refs/heads/upstream/binutils-2_10-branch/include/os9k.h)
- `bfd/i386os9k.c` als BFD-Backend für OS-9000-i386: [i386os9k.c](https://fuchsia.googlesource.com/third_party/binutils-gdb/+/refs/heads/upstream/binutils-2_10-branch/bfd/i386os9k.c)
- `bfd/config.bfd` mit dem Target-Triple `i[3456]86-*-os9k`.

Der Header enthält Offsets für Name, Usage-Kommentar, Symboltabelle, Entry, Exception, Datenbedarf, Stack, initialisierte Daten sowie Initialisierungs-/Terminierungsroutinen. Das ist deutlich mehr als ein normaler ELF-Header.

Bewertung: Dieser Code ist die beste gefundene Referenz für Q9-Forge. Er müsste auf moderne BFD-/binutils-APIs portiert und um einen validierten OS-9000-Schreibpfad ergänzt werden.

## Moderne x86-ELF-Ausgangstoolchains

- GCC unterstützt i686 als Zielarchitektur und dokumentiert Cross-Compiler mit Binutils, Sysroot, Headers und Startobjekten: [GCC Configure](https://gcc.gnu.org/install/configure.html), [GCC Cross-Compiler-Build](https://gcc.gnu.org/install/build.html)
- Clang unterstützt stabile 32-/64-Bit-X86-Codegenerierung und explizite Target-Triples wie `i386-unknown-elf`; ohne korrektes Target kann versehentlich Host-Code entstehen: [Clang Cross-Compilation](https://clang.llvm.org/docs/CrossCompilation.html), [Clang User Manual](https://clang.llvm.org/docs/UsersManual.html)

Bewertung: `i686-elf-gcc` oder Clang `--target=i386-unknown-elf` sind geeignete Codegeneratoren für freestanding 32-Bit-x86-Code. Sie liefern jedoch weder OS-9000-ABI, Systembibliotheken, Startup-Code noch das native Modul-/Relocationformat.

## Bestehende OS-9-Projekte und Konverter

- ToolShed enthält Compiler-/Assembler- und RBF-Werkzeuge, ist aber auf OS-9/6809 bzw. NitrOS-9 ausgerichtet und kein OS-9000/x86-Toolchain: [ToolShed](https://github.com/nitros9project/toolshed)
- LWTOOLS kann OS-9-Module für 6809 erzeugen, nicht OS-9000/i386: [LWTOOLS OS9-Linking Notes](https://www.lwtools.ca/manual/x1019.html)
- Die allgemeine OS-9-Dokumentation bestätigt Header, Entry, BSS-/Datenbedarf und CRC-Konzept, bezieht sich aber überwiegend auf 6809/68K: [OS-9 System Programmer's Guide](https://www.roug.org/retrocomputing/os/os9/os9sysprog.html)

Einen öffentlich auffindbaren fertigen ELF32/i386→OS-9000-Konverter konnte ich nicht verifizieren.

## Technische Schlussfolgerung

Die realistische Route ist:

```text
C-Quelltext -> i386-ELF-Compiler -> ELF32-Relocatable Object
            -> Q9-Forge OS-9000/x86 Packer -> OS-9000-Modul
```

Der Packer muss ELF32 little-endian/i386, Sections, Symbole und Relocations lesen, zunächst eine kleine geprüfte Relocation-Teilmenge unterstützen, den OS-9000/x86-Header gemäß `os9k.h` erzeugen, Entry-/Stack-/Datenfelder setzen, Imports/Relocations nach OS-9000-Konvention abbilden und die CRC `0x800fe3` berechnen. Ein Linux-/ELF-Programm kann nicht unverändert als OS-9000-Modul starten, weil OS-9000-Startup, ABI, Systemaufrufe und Relocationstruktur fehlen.

## Empfehlung für Q9-Forge

1. `i386os9k.c` und `os9k.h` als Forschungsreferenz sichern, nicht ungeprüft in aktuelle Binutils integrieren.
2. Einen unabhängigen ELF32-Parser/Packer schreiben, zunächst nur für ein handcodiertes Testprogramm ohne externe Symbole.
3. Die OS-9000/x86-Relocationsemantik anhand echter Module aus dem Bootimage reverse-engineeren.
4. Erst danach GCC/Clang als C-Frontend anbinden.
5. Parallel nach einer vollständigen Microware Ultra-C/PC-Development-Pak-Installation in Archiven suchen.

## Grenzen

Die Recherche belegt die historische Toolchain und den historischen GNU-BFD-Backend, aber keinen aktuell legal verfügbaren OS-9000/x86-Binärcompiler und keinen fertigen ELF-Konverter. Die genaue OS-9000/x86-Relocationstabelle und Startup-/C-ABI-Konventionen müssen aus Originalmodulen und/oder Originaldokumentation rekonstruiert werden.
