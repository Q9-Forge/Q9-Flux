# Lizenzen Dritter

*English version: [THIRD-PARTY-LICENSES.md](THIRD-PARTY-LICENSES.md)*

Der eigene Quellcode von Q9-Flux (dieses Repository, ohne die unten
aufgeführten Pfade) steht unter der MIT-Lizenz — siehe
[LICENSE](LICENSE).

Q9-Flux bindet mehrere Komponenten Dritter ein oder verweist auf sie.
Jede behält ihre **eigene, ursprüngliche Lizenz**, die nicht zwingend MIT
ist. Diese Datei listet jede dieser Komponenten auf, wo sie liegt und
welche Lizenz gilt.

## Eingebundener Quellcode

| Komponente | Ort | Lizenz | Anmerkung |
|---|---|---|---|
| Musashi (68000/68020/68030-CPU-Kern) | `Q9-Flux-68k/third_party/musashi/` | MIT | Copyright Karl Stenerud. Siehe `Q9_VENDOR.md` im selben Verzeichnis für den fixierten Upstream-Commit und die Q9-spezifischen Änderungen. |
| SoftFloat, Release 2b | `Q9-Flux-68k/third_party/musashi/softfloat/` | SoftFloat Release 2b (eigene Lizenz, nicht MIT) | Geschrieben von John R. Hauser / International Computer Science Institute. Permissiv, verlangt aber, dass die vier Absätze unten in jedem abgeleiteten Werk erhalten bleiben. Voller Text unter [SoftFloat Release 2b](#softfloat-release-2b) weiter unten. |
| TinyEMU (RISC-V-Emulator-Bestandteile) | `Q9-Flux-68k/third_party/tinyemu/` | MIT | Copyright Fabrice Bellard. Lizenztext liegt als `MIT-LICENSE.txt` im selben Verzeichnis bei. |

## Eingebundene Windows-Binärdateien (für das slirp-User-Mode-Networking)

Ort: `Q9-Flux-68k/third_party/slirp/windows/`. Das sind unveränderte
Binärdateien aus dem MSYS2-mingw64-Paketspiegel
(https://mirror.msys2.org/mingw/mingw64/); Quellcode dieser Projekte
selbst ist nicht in diesem Repository enthalten, nur die kompilierte
DLL, Importbibliotheken und Header, die zum Bauen und Ausführen dagegen
nötig sind.

| Komponente | Lizenz | Upstream-Quelle |
|---|---|---|
| `libslirp-0.dll` | BSD-3-Clause | https://gitlab.freedesktop.org/slirp/libslirp |
| `libglib-2.0-0.dll` | LGPL-2.1-or-later | https://gitlab.gnome.org/GNOME/glib |
| `libiconv-2.dll` | LGPL-2.1-or-later | https://savannah.gnu.org/projects/libiconv/ |
| `libintl-8.dll` | LGPL-2.1-or-later | Teil von GNU gettext, https://www.gnu.org/software/gettext/ |
| `libpcre2-8-0.dll` | BSD-3-Clause | https://github.com/PCRE2Project/pcre2 |

**Hinweis zur LGPL:** Diese Bibliotheken werden ausschließlich dynamisch
verlinkt (DLL). Q9-Flux verändert sie nicht und verlinkt sie nicht
statisch; ihr zugehöriger Quellcode ist an den obigen Upstream-Adressen
(bzw. den MSYS2-mingw64-Quellpaketen, aus denen sie gebaut werden) frei
verfügbar — damit ist die Quellverfügbarkeitspflicht der LGPL erfüllt,
ohne dass dieses Repository eine eigene Kopie dieses Quellcodes
mitführen müsste.

## Per Git-Submodul referenziert (nicht als Quellcode in diesem Repository eingecheckt)

Ein einfaches `git clone` von Q9-Flux zieht diese Inhalte **nicht**
automatisch; sie erscheinen erst nach `git submodule update --init`, und
ihr Inhalt ist nie Teil der eigenen Git-Historie dieses Repositorys.

| Komponente | Ort | `.gitmodules` | Lizenz | Verwendung |
|---|---|---|---|---|
| Turbo Vision (`tvision`, magiblot-Fork) | `Q9-Flux-68k/third_party/tvision/` | `Q9-Flux-68k/.gitmodules` → https://github.com/magiblot/tvision.git | MIT (Änderungen von magiblot und weiteren Mitwirkenden; der ursprüngliche Borland-Turbo-Vision-Haftungsausschluss bleibt daneben erhalten, siehe `COPYRIGHT` im selben Verzeichnis) | Wird in das Q9-Flux-68k-Editor-/TUI-Tooling einkompiliert. |
| QEMU | `Q9-Flux-x86/third_party/qemu/` | `Q9-Flux-x86/.gitmodules` → https://gitlab.com/qemu-project/qemu.git | GPL-2.0-or-later (einzelne Dateien darin unter LGPL-2.1-or-later oder BSD; siehe QEMUs eigene `COPYING`/`COPYING.LIB`) | Dient als **externer i386-Systememulator-Prozess** für Q9-Flux-x86 (`bin/flux-x86` startet den unveränderten Upstream-`qemu-system-i386` und steuert ihn per QMP/Kommandozeile an). QEMUs eigener Quellcode wird nicht verändert, und kein QEMU-Code wird in Q9-Flux-x86s eigene Binärdateien einkompiliert oder statisch hineingelinkt — das ist der Fall eines Programms, das ein anderes, separates Programm aufruft, kein gemeinsames Werk, daher greift die GPL-2.0 nicht auf Q9-Flux-x86s eigenen Code über. Wer ein kombiniertes Paket weitergibt (z. B. einen Installer mit sowohl Q9-Flux-x86 als auch einem gebauten `qemu-system-i386`), muss für den QEMU-Anteil trotzdem QEMUs eigene GPL-2.0-Weitergabebedingungen einhalten. |

## SoftFloat Release 2b

Der folgende, wörtlich aus
`Q9-Flux-68k/third_party/musashi/softfloat/softfloat.c` übernommene
Hinweis muss laut SoftFloats eigener Lizenz jede beibehaltene oder
abgeleitete Nutzung dieses Codes begleiten:

> This C source file is part of the SoftFloat IEC/IEEE Floating-point
> Arithmetic Package, Release 2b.
>
> Written by John R. Hauser. This work was made possible in part by the
> International Computer Science Institute, located at Suite 600, 1947
> Center Street, Berkeley, California 94704. Funding was partially
> provided by the National Science Foundation under grant MIP-9311980.
> The original version of this code was written as part of a project to
> build a fixed-point vector processor in collaboration with the
> University of California at Berkeley, overseen by Profs. Nelson Morgan
> and John Wawrzynek. More information is available through the Web page
> `http://www.cs.berkeley.edu/~jhauser/arithmetic/SoftFloat.html'.
>
> THIS SOFTWARE IS DISTRIBUTED AS IS, FOR FREE. Although reasonable
> effort has been made to avoid it, THIS SOFTWARE MAY CONTAIN FAULTS THAT
> WILL AT TIMES RESULT IN INCORRECT BEHAVIOR. USE OF THIS SOFTWARE IS
> RESTRICTED TO PERSONS AND ORGANIZATIONS WHO CAN AND WILL TAKE FULL
> RESPONSIBILITY FOR ALL LOSSES, COSTS, OR OTHER PROBLEMS THEY INCUR DUE
> TO THE SOFTWARE, AND WHO FURTHERMORE EFFECTIVELY INDEMNIFY JOHN HAUSER
> AND THE INTERNATIONAL COMPUTER SCIENCE INSTITUTE (possibly via similar
> legal warning) AGAINST ALL LOSSES, COSTS, OR OTHER PROBLEMS INCURRED BY
> THEIR CUSTOMERS AND CLIENTS DUE TO THE SOFTWARE.
>
> Derivative works are acceptable, even for commercial purposes, so long
> as (1) the source code for the derivative work includes prominent
> notice that the work is derivative, and (2) the source code includes
> prominent notice with these four paragraphs for those parts of this
> code that are retained.

Dieser Hinweis bleibt in den eingebundenen `softfloat.c`/`softfloat.h`-
Quelldateien selbst unangetastet erhalten, dort ist die Lizenzbedingung
rechtlich erfüllt; die Kopie oben dient der Übersicht auf
Projektdokumentations-Ebene und der geforderten "prominenten"
Sichtbarkeit.
