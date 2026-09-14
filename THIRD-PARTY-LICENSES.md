# Third-Party Licenses

*German version: [THIRD-PARTY-LICENSES_de.md](THIRD-PARTY-LICENSES_de.md)*

Q9-Flux's own source code (this repository, excluding the paths listed
below) is licensed under the MIT License — see [LICENSE](LICENSE).

Q9-Flux bundles or references several third-party components. Each keeps
its **own, original license**, which is not necessarily MIT. This file
lists every such component, where it lives, and what license applies.

## Bundled source code

| Component | Location | License | Notes |
|---|---|---|---|
| Musashi (68000/68020/68030 CPU core) | `Q9-Flux-68k/third_party/musashi/` | MIT | Copyright Karl Stenerud. See `Q9_VENDOR.md` in that directory for the pinned upstream commit and the Q9-specific modifications. |
| SoftFloat, Release 2b | `Q9-Flux-68k/third_party/musashi/softfloat/` | SoftFloat Release 2b (own license, not MIT) | Written by John R. Hauser / International Computer Science Institute. Permissive, but requires the four paragraphs below to be retained in any derivative work. See full text under [SoftFloat Release 2b](#softfloat-release-2b) below. |
| TinyEMU (RISC-V emulator components) | `Q9-Flux-68k/third_party/tinyemu/` | MIT | Copyright Fabrice Bellard. License text bundled as `MIT-LICENSE.txt` in that directory. |

## Bundled Windows binaries (used by the slirp user-mode networking backend)

Location: `Q9-Flux-68k/third_party/slirp/windows/`. These are unmodified
binaries obtained from the MSYS2 mingw64 package mirror
(https://mirror.msys2.org/mingw/mingw64/); no source code from these
projects is included in this repository, only the compiled DLL, import
libraries, and headers needed to build and run against them.

| Component | License | Upstream source |
|---|---|---|
| `libslirp-0.dll` | BSD-3-Clause | https://gitlab.freedesktop.org/slirp/libslirp |
| `libglib-2.0-0.dll` | LGPL-2.1-or-later | https://gitlab.gnome.org/GNOME/glib |
| `libiconv-2.dll` | LGPL-2.1-or-later | https://savannah.gnu.org/projects/libiconv/ |
| `libintl-8.dll` | LGPL-2.1-or-later | part of GNU gettext, https://www.gnu.org/software/gettext/ |
| `libpcre2-8-0.dll` | BSD-3-Clause | https://github.com/PCRE2Project/pcre2 |

**LGPL note:** these libraries are used exclusively via dynamic linking
(DLL). Q9-Flux does not modify or statically link them, and their
corresponding source is freely available at the upstream locations above
(or the MSYS2 mingw64 source packages that build them), satisfying the
LGPL's source-availability requirement without this repository having to
carry a private copy of that source.

## Referenced via git submodule (not vendored as source in this repository)

A plain `git clone` of Q9-Flux does **not** pull these in; they only
appear after `git submodule update --init`, and their content is never
part of this repository's own git history.

| Component | Location | `.gitmodules` | License | How it is used |
|---|---|---|---|---|
| Turbo Vision (`tvision`, magiblot fork) | `Q9-Flux-68k/third_party/tvision/` | `Q9-Flux-68k/.gitmodules` → https://github.com/magiblot/tvision.git | MIT (modifications by magiblot and contributors; original Borland Turbo Vision disclaimer retained alongside, see `COPYRIGHT` in that directory) | Compiled into the Q9-Flux-68k editor/TUI tooling. |
| QEMU | `Q9-Flux-x86/third_party/qemu/` | `Q9-Flux-x86/.gitmodules` → https://gitlab.com/qemu-project/qemu.git | GPL-2.0-or-later (with some individual files under LGPL-2.1-or-later or BSD; see QEMU's own `COPYING`/`COPYING.LIB`) | Used as the **external i386 system emulator process** for Q9-Flux-x86 (`bin/flux-x86` launches the unmodified upstream `qemu-system-i386` and drives it via QMP/the command line). QEMU's own source is not modified, and no QEMU code is compiled into or statically linked with Q9-Flux-x86's own binaries — this is a case of one program invoking another, separate program, not a combined work, so GPL-2.0 does not extend to Q9-Flux-x86's own code. Anyone who builds and redistributes a combined package (e.g. a bundled installer containing both Q9-Flux-x86 and a built `qemu-system-i386`) must still meet QEMU's own GPL-2.0 redistribution terms for the QEMU part. |

## SoftFloat Release 2b

The following notice, reproduced verbatim from
`Q9-Flux-68k/third_party/musashi/softfloat/softfloat.c`, is required by
SoftFloat's own license to accompany any retained or derivative use of
that code:

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

This notice remains intact in the vendored `softfloat.c`/`softfloat.h`
source files themselves, which is where the license terms are legally
satisfied; the copy above is provided for convenience and for the
"prominent notice" requirement at the project-documentation level.
