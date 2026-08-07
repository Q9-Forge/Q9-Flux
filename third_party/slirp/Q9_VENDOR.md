# libslirp (+glib2) — Vendor-Notiz

## ✅ GELOEST (2026-08-07, Abend) — hostfwd-Absturz behoben, zwei eigene Bugs, kein libslirp-Bug

Der RESUME-HERE-Plan von der Nacht-Session wurde befolgt und hat zum Ziel gefuehrt.
MSYS2-Toolchain OHNE `pacman-key --init` installiert (Befund bestaetigt: Keyring war
schon gueltig, das war die eigentliche Bremse beim letzten Anlauf) -- diesmal glatt
durchgelaufen. `C:\msys64` ist jetzt dauerhaft mit vollstaendiger `mingw-w64-x86_64-
toolchain` (gcc, gdb, meson, ninja, pkgconf, glib2, git) auf dieser Maschine vorhanden,
der Installationsschritt entfaellt bei einem naechsten Debugging-Anlauf.

libslirp v4.9.3 (identische Version wie vendort) aus dem Quellcode mit `meson setup build
--buildtype=debug && ninja -C build` gebaut, Reproducer dagegen gelinkt, unter
`gdb -batch -ex run -ex bt` laufen lassen -- Absturz sofort mit vollem symbolisiertem
Stacktrace reproduziert (kein Wechsel auf Upstream-Bugreport noetig).

**BUG 1 -- Ursache des urspruenglich hier dokumentierten Absturzes (eigener Konfigfehler,
KEIN libslirp-Bug):** `slirp_register_poll_socket()` (src/slirp.c) ruft
`cb->register_poll_socket()` nur wenn `cfg_version >= 6` ist -- sonst faellt es in den
deprecated `cb->register_poll_fd()`-Zweig, den wir absichtlich NULL gelassen hatten. Mit
`cfg.version = 1` (Q9-Code seit 5.14 "Erster Wurf") griff also immer der NULL-Zeiger-Zweig
-> Crash in `slirp_add_hostfwd()` -> `tcp_listen()` -> `tcpx_listen()` ->
`slirp_register_poll_socket()`. **Fix:** `cfg.version = 6` (SLIRP_CONFIG_VERSION_MAX) in
`src/kernel/slirp_net.c` (Ver. 1.03).

**BUG 2 -- zweiter Absturz, nach Fix 1 per gdb neu aufgedeckt, echter Speicherfehler seit
dem ersten Wurf latent:** `SlirpCb cb;` war eine LOKALE Stack-Variable in
`q9_slirp_start()`. `slirp_new()` speichert davon aber nur den Zeiger
(`slirp->cb = callbacks;`), keine Kopie -- nach Rueckkehr der Funktion dangling. Nie
aufgefallen, weil `slirp_pollfds_poll()` (einziger Dereferenzierer aus dem Hauptloop) nur
bei `g_pollfd_count > 0` laeuft, und ohne hostfwd/aktive Verbindung war das nie der Fall.
Erst der durch Fix 1 aktivierte hostfwd-Listener-Socket machte `g_pollfd_count > 0` schon
beim ersten Poll und liess `slirp->cb->clock_get_ns()` auf eine laengst wiederverwendete
Stack-Adresse springen (Crash-Adresse 0x5fe3b8, kein Modul -- klassisches Dangling-
Pointer-Symptom). **Fix:** `SlirpCb` als `static g_cb` (Datei-Lebensdauer, passt zum
ohnehin Singleton-artigen `g_slirp`) in `src/kernel/slirp_net.c` (Ver. 1.04).

**Verifiziert** (echter `q9.exe`, vendorte Windows-Release-Libs, nicht nur Debug-Build):
kompletter OS-9-Boot bis "8 devices online" mit `net=slirp` + `net_hostfwd = tcp:2323:23`,
`netstat` zeigt Port 2323 im LISTEN-Zustand, eingehende TCP-Verbindung crasht den Emulator
nicht mehr (5+ Sekunden stabil). OS-9s `telnetd` selbst antwortete auf der Testverbindung
noch nicht (kein Banner) -- das ist jetzt ein separates OS-9-Konfigurationsthema (Dienst
evtl. nicht automatisch gestartet), KEIN slirp/hostfwd-Bug mehr. Details/Naechste-Schritte
in context.txt, Session "2026-08-07 (Abend, Claudia)".

Committet + gepusht als 610cca2 (github.com/Q9-Forge/Q9-Flux, main).

---


**Quelle**: MSYS2-Mirror-Pakete (mingw64-Repo), https://mirror.msys2.org/mingw/mingw64/
**Lizenzen**:
- `libslirp` — BSD-3-Clause (Text in `windows/lib/pkgconfig`-Quelle bzw. Upstream
  https://gitlab.freedesktop.org/slirp/libslirp)
- `glib2` — LGPL-2.1-or-later (dynamisch verlinkt, DLL redistributierbar; Quelle:
  https://gitlab.gnome.org/GNOME/glib)
**Vendoriert**: 2026-08-07

## Warum vendort statt System-Paket

`libslirp` ist die Bibliothek, die QEMU/VirtualBox fuer "User-Mode-Networking" nutzen
(--net slirp, s. `src/kernel/slirp_net.c`/quicc.h) — plattformuebergreifend nutzbar,
anders als `vmnet`/`bridge` (macOS-only). Auf Windows gibt es dafuer kein fertiges,
Installer-freies Paket wie das portable w64devkit selbst; `libslirp` braucht `glib2`
als Abhaengigkeit, und die gaengige Windows-Bezugsquelle (MSYS2/pacman) wollten wir
NICHT als Build-Voraussetzung erzwingen (Andreas' Wunsch: "auch ohne Cygwin/MSYS2
laeuft, andere wollen das Projekt auch mal bauen koennen"). Deshalb: einmalig per
Hand aus den MSYS2-Mirror-Paketen extrahiert und hier eingecheckt — w64devkit allein
reicht danach zum Bauen UND Ausfuehren, kein Paketmanager noetig.

## Wieso nur 5 DLLs (nicht die ganze glib2/gcc-libs-Kette)

Die MSYS2-Paketmetadaten deklarieren grosszuegig (glib2 "braucht" laut Paket-Manifest
sogar Python — reine Build-/Introspection-Abhaengigkeit, nicht zur Laufzeit). Per
`objdump -p <dll> | grep "DLL Name"` gegen die TATSAECHLICHEN Symboltabellen geprueft
(nicht gegen die Paket-Manifeste), ergibt sich eine viel kleinere echte Kette:

```
libslirp-0.dll     -> libglib-2.0-0.dll (+ System-DLLs: WS2_32, IPHLPAPI, KERNEL32)
libglib-2.0-0.dll  -> libintl-8.dll, libpcre2-8-0.dll (+ System-DLLs)
libintl-8.dll      -> libiconv-2.dll (+ System-DLLs)
libpcre2-8-0.dll   -> nur System-DLLs
libiconv-2.dll     -> nur System-DLLs
```

Kein `libffi`, kein `zlib`, kein `libwinpthread`, kein `gcc-libs` (libstdc++/libgcc_s) —
diese Pakete werden von den MSYS2-Metadaten als Abhaengigkeit gefuehrt, aber nicht
tatsaechlich von den 5 benoetigten DLLs importiert (nur von gio/gobject/gmodule, die
wir nicht verwenden). Ergebnis: 5 DLLs, ~3.8 MB statt der vollen Paket-Kette.

## Woher genau (Versionen, 2026-08-07)

| Paket | Version | Download |
|---|---|---|
| libslirp | 4.9.3-1 | `mingw-w64-x86_64-libslirp-4.9.3-1-any.pkg.tar.zst` |
| glib2 | 2.88.3-1 | `mingw-w64-x86_64-glib2-2.88.3-1-any.pkg.tar.zst` |
| gettext-runtime (libintl) | 1.0-1 | `mingw-w64-x86_64-gettext-runtime-1.0-1-any.pkg.tar.zst` |
| pcre2 | 10.47-1 | `mingw-w64-x86_64-pcre2-10.47-1-any.pkg.tar.zst` |
| libiconv | 1.19-1 | `mingw-w64-x86_64-libiconv-1.19-1-any.pkg.tar.zst` |

## Was ist hier drin

```
windows/
  include/slirp/       libslirp.h, libslirp-version.h
  include/glib-2.0/     glib.h + glib/*.h (nur der GLib-Kern -- KEIN gio/gobject/
                        gmodule/girepository, s.o. -- die brauchen wir nicht)
  lib/glib-2.0/include/ glibconfig.h (architekturabhaengige generierte Config)
  lib/                  libslirp.dll.a, libglib-2.0.dll.a (Import-Libs zum Linken)
  bin/                  die 5 Laufzeit-DLLs (muessen neben q9.exe liegen, s. Makefile)
```

Compiler-/Linker-Flags (aus den Original-.pc-Dateien der Pakete uebernommen, s.
Makefile `Q9_HAVE_SLIRP`-Block):

```
CFLAGS: -Ithird_party/slirp/windows/include             (slirp_net.c: #include <slirp/libslirp.h>)
        -Ithird_party/slirp/windows/include/glib-2.0
        -Ithird_party/slirp/windows/lib/glib-2.0/include
LIBS:   -Lthird_party/slirp/windows/lib -lslirp -lglib-2.0
```

## macOS/Linux

Dort NICHT vendort — `Q9_HAVE_SLIRP` wird stattdessen ueber System-`libslirp`
aktiviert (`brew install libslirp` auf macOS, `apt install libslirp-dev` auf Debian/
Ubuntu o.ae.), per `pkg-config slirp` gefunden (s. Makefile). Der `src/kernel/
slirp_net.c`-Code selbst ist komplett plattformneutral — nur die Bezugsquelle der
Bibliothek unterscheidet sich.

## Verifiziert (2026-08-07) -- inkl. zwei gefundener Bugs in dieser Kombination

`make native && q9.exe --net slirp ...` mit echtem OS-9-Boot durchgetestet (Windows,
w64devkit). Kompletter Boot bis `8 devices online` funktioniert stabil. Dabei zwei
Absturz-Bugs gefunden und umschifft (beide per `gdb` isoliert: Sprung zu einer
Quasi-NULL-Adresse aus `libslirp-0.dll` heraus):

1. **`slirp_add_hostfwd()` stuerzt zuverlaessig ab** -- reproduziert sowohl im echten
   `q9.exe` als auch in einem minimalen Standalone-Testprogramm OHNE jeden Q9-Code
   (verschiedene `host_addr`-Werte, verschiedene Callback-Belegungen probiert, immer
   derselbe Absturz). **hostfwd ist deshalb vorerst deaktiviert** (`slirp_net.c`,
   `q9_slirp_start()` gibt nur eine Warnung aus und ueberspringt den Aufruf) --
   `net_hostfwd` in der `.q9`-Config wird geparst, aber nicht angewendet.
2. **`slirp_pollfds_poll()` stuerzt ab, wenn 0 Sockets registriert sind** -- NUR im
   echten `q9.exe` reproduzierbar, NICHT in kleinen Standalone-Tests (auch nicht mit
   mehrfachem `WSAStartup()` + vorab geoeffnetem Listener-Socket nachgestellt) --
   sieht nach einem uninitialisierten Speicherzugriff in `libslirp` aus, der sich je
   nach Prozessspeicher-Historie unterschiedlich auswirkt. Workaround: `q9_slirp_poll()`
   ruft `slirp_pollfds_poll()` nur noch auf, wenn `slirp_pollfds_fill_socket()` zuvor
   mindestens 1 Socket gemeldet hat.

**Funktioniert:** die Kernfunktion -- OS-9 bekommt eine emulierte Ethernet-Karte mit
ARP/ICMP/DHCP-freier statischer IP, kann Verbindungen ins echte Internet aufbauen
(ausgehend). **Funktioniert NICHT (vorerst):** Port-Weiterleitung vom Host in den Gast
(`net_hostfwd`, z.B. um von aussen auf OS-9s `telnetd` zuzugreifen).

## Vertiefte Untersuchung des hostfwd-Absturzes (2026-08-07, Nacht-Session)

Weiterverfolgt, weil Andreas' eigentliches Ziel genau das war (Host -> OS-9-telnetd).
Damaliges (falsches) Ergebnis dieser Nacht-Session: "echter Bug, keine Konfigurationssache
-- mit hoher Sicherheit isoliert, aber NICHT gefixt". **Korrektur (2026-08-07 Abend, s.
"GELOEST" ganz oben):** Es WAR eine Konfigurationssache -- `cfg.version = 1` statt 6. Im
Rueckblick erklaert das auch, warum alle Versuche unten wirkungslos blieben: keiner davon
aenderte `cfg.version`, also griff in jedem Versuch weiterhin der falsche (deprecated)
interne Codepfad. Die "ungueltiger Funktionszeiger aus grossem Struct-Offset"-Beobachtung
weiter unten war vermutlich schon ein erster Blick auf denselben dangling-`cb`-Mechanismus
wie BUG 2 oben -- nur ohne Debug-Symbole nicht als solcher erkennbar. Analyse unten bleibt
als historischer Debugging-Pfad stehen, ist aber durch den finalen Fund oben ueberholt.

### Was probiert wurde (alles ohne Erfolg)

- **`slirp_add_hostxfwd()` statt `slirp_add_hostfwd()`** (neuere, generische
  sockaddr-basierte API) -- stuerzt am EXAKT selben Offset in `libslirp-0.dll` ab
  (identischer Low-12-Bit-Adressoffset trotz ASLR-Rebasing zwischen Runs bestaetigt)
  -- beide Funktionen landen im selben internen Codepfad.
- **Andere `libslirp`-Version** (4.8.0 statt 4.9.3, mit passender `.dll.a`) -- laedt
  gar nicht erst (`STATUS_ENTRYPOINT_NOT_FOUND`, 0xc0000139) gegen glib2 2.88.3 --
  ABI-Drift zwischen den unabhaengig aktuellen MSYS2-Paketversionen. Keine anderen
  `libslirp`-Versionen mehr auf dem Mirror verfuegbar (nur 4.8.0..4.9.3).
  Eine passende AELTERE glib2-Version haette zusaetzlich beschafft werden muessen --
  nicht mehr verfolgt (Aufwand explodiert).
- **`register_poll_socket`/`unregister_poll_socket`/`notify`/`init_completed` explizit
  auf No-Op-Stubs statt NULL gesetzt** -- keine Aenderung, keiner dieser Stubs wurde
  je aufgerufen (per eigenem `printf` in den Stubs verifiziert).
- **"Aufwaerm"-Poll-Runden vor `slirp_add_hostfwd()`** (falls Lazy-Init-Problem) --
  keine Aenderung.
- **Echte Windows-Konsole statt Git-Bash-Pty** (PowerShell direkt) -- keine Aenderung,
  exit code weiterhin `0xC0000005` (Access Violation).
- **GLib-Env-Vars** (`G_SLICE=always-malloc`, `G_DEBUG=fatal-warnings`, `NO_COLOR=1`,
  `G_MESSAGES_DEBUG=`) -- keine Aenderung.
- **Quellcode-Nachverfolgung** (github.com/utmapp/libslirp, aktueller Stand):
  `slirp_add_hostfwd` -> `tcp_listen()` -> `socreate()` + `tcp_newtcpcb()` +
  `slirp_socket()`/`bind()`/`listen()`/`setsockopt()`/`getsockname()`. KEINE dieser
  Funktionen ruft im Quellcode einen `slirp->cb->`-Callback auf. Die Disassemblierung
  zeigt trotzdem einen `jmp *%rax`/`call *%rax` durch einen bei Laufzeit ungueltigen
  Funktionszeiger, gelesen aus einer Struktur mit grossem Offset (~0x1770) innerhalb
  eines internen Objekts -- passt eher zu GLibs EIGENER interner Boekhaltung
  (Log-Writer-Dispatch, Typinitialisierung o.ae.) als zu Slirps eigenem Code. Die
  Windows-GLib-Referenzdokumentation erwaehnt eine eigene, von GLib installierte
  Vectored-Exception-Handler-Infrastruktur fuer Windows (`G_VEH_CATCH`) -- ein
  plausibler, aber nicht bewiesener Verdacht: GLibs eigene Windows-Crash-Behandlung
  ist in diesem Build fehlerhaft initialisiert und faengt/verschluckt den eigentlichen
  (vermutlich harmloseren) Fehler falsch ab.
- **Debug-Build aus dem Quellcode selbst** (um mit echten Symbolen einen sauberen
  Stacktrace zu bekommen): MSYS2 wurde dafuer NUR TEMPORAER installiert (nicht als
  Projekt-Abhaengigkeit). Scheiterte an Windows-Automatisierungsproblemen -- `pacman
  -S ... --noconfirm` reagierte trotz des Flags nicht zuverlaessig auf gepipte
  Antworten ueber die Git-Bash<->MSYS2-Prozessgrenze und blieb an interaktiven
  Prompts haengen (Gruppen-Auswahl, Bestaetigung); ein zusaetzlicher, vermutlich
  UNNOETIGER `pacman-key --init`-Versuch (der Keyring hatte bereits gueltigen Inhalt
  aus der Basisinstallation, s. "RESUME HERE" oben) blieb ebenfalls haengen und wurde
  abgebrochen. Nach mehreren Anlaeufen als unverhaeltnismaessiger Zeitaufwand
  abgebrochen, BEVOR der eigentliche Debug-Build ueberhaupt begonnen wurde. `C:\msys64`
  liegt noch auf der Maschine, Basisinstallation funktionsfaehig (Details + konkrete
  naechste Schritte s. "RESUME HERE" ganz oben in dieser Datei).

### Empfohlene naechste Schritte (fuer einen kuenftigen Anlauf)

1. **Sauberen MSYS2-Debug-Build auf einer Maschine mit funktionierendem GPG/Pacman
   versuchen** (evtl. `--disable-download-timeout`, oder `pacman-key --init` mit
   vorhandenem Zufallsgenerator/Maus-Bewegung fuer Entropie, oder komplett OHNE
   Signaturpruefung: `pacman -S --noconfirm --needed <pkgs>` funktioniert oft OHNE
   vorherigen `pacman-key --init`, wenn die Standard-Schluessel schon im Installer
   enthalten sind -- ggf. den `pacman-key --init`-Schritt einfach ueberspringen).
2. **Upstream-Bugreport bei libslirp** (gitlab.freedesktop.org/slirp/libslirp/-/issues)
   mit dem hier gesammelten Befund (Absturz in `slirp_add_hostfwd`/`slirp_add_hostxfwd`
   auf Windows/MinGW, MSYS2-Pakete libslirp 4.9.3-1 + glib2 2.88.3-1, minimaler
   Reproducer verfuegbar) -- die Upstream-Maintainer haben echte Debug-Symbole und
   kennen den Code; das waere vermutlich der schnellste Weg zu einer echten Antwort.
3. **Alternative Bibliothek statt libslirp pruefen** (z.B. `libtapi`/eigener
   Mini-TAP-Treiber, oder ein voellig anderes User-Mode-NAT wie `passt`) -- groesserer
   Schnitt, aber falls libslirp auf Windows grundsaetzlich fragil bleibt, langfristig
   robuster.
4. **Hand-rolled TCP-Relay in Q9 selbst** (kein libslirp-hostfwd, stattdessen eigener
   minimaler TCP-Proxy auf IP/Ethernet-Frame-Ebene ueber `slirp_input()`/`send_packet`)
   -- MACHBAR, aber vom Umfang vergleichbar mit dem, was `tcp_listen()` intern selbst
   tut (Sequenznummern-/ACK-Tracking, Fenster, Retransmission) -- kein Quick-Fix.

**Status:** Kernfunktion (ausgehende Verbindungen) bleibt produktiv nutzbar und
verifiziert stabil. hostfwd bleibt bewusst deaktiviert, bis einer der obigen Schritte
verfolgt wird.
