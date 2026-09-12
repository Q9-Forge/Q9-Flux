#═════════════════════════════════════════════════════════════════════════════════════════════════
# File:   HWCONFIG.md                                                                     Ver. 1.00
# Owner:  AF
# Desc.:  Design-Referenz fuer die konfigurierbaren Hardware-Module des Q9-Emulators
#         (ARBEITSPLAN 5.17-5.21). Ergebnis der Planungsrunde Andreas + Claudia am 2026-07-14
#         (abends): Geraete-Interface, Bus-Dispatch, Board-Konfigurationsdatei,
#         Descriptor-Generierung, Plattform-Strategie. Dieses Dokument ist die Arbeitsgrundlage
#         fuer die Umsetzungs-Sessions — hier steht das WARUM zu jedem Design-Entscheid.
#
# Edition History
#─────────┬──────┬─────────────────────────────────────────────────────────────────────────┬──────
# Date    │ Ver. │ Description                                                             │ By
#─────────┼──────┼─────────────────────────────────────────────────────────────────────────┼──────
# 26-07-14│ 1.00 │ Initiale Version aus der Planungsrunde (Chat-Session 14.7. abends)      │ CF
#═════════╧══════╧═════════════════════════════════════════════════════════════════════════╧══════

# Konfigurierbare Hardware-Module — Design-Referenz (5.17–5.21)

Bezug: [ARBEITSPLAN.md](../.claude/ARBEITSPLAN.md) Schritte 5.17–5.21.

## Die vier Ziele (Andreas, 2026-07-14)

1. **Modulare Hardware:** Jedes emulierte Gerät einzeln erstellen und per
   Konfiguration ins Board aufnehmen können — ohne Zwang, alle zu verwenden.
2. **Konfigurierbare Descriptoren:** Adresse, Name, Vektor, Level usw. an EINER
   Stelle pflegen — für Emulator UND MWOS-Port (OS-9-Descriptoren).
3. **Maximale Geschwindigkeit:** möglichst wenige Adressabfragen pro Zugriff.
4. **Alle drei großen OS:** macOS, Windows, Linux.

Kernerkenntnis der Runde: Ziele 1 und 3 widersprechen sich NICHT, weil die
Konfiguration nur die *Initialisierung* betrifft. Der Dispatch wird beim Start
aus den tatsächlich instanziierten Geräten gebaut — ein nicht konfiguriertes
Gerät kostet exakt null Laufzeit.

## Ist-Zustand (vor dem Umbau, Stand 5.16)

Geräte fest verdrahtet an DREI Stellen: Speicher-Dispatch (`m68krt.c`
Musashi-Hooks → Netz-Terminals → QUICC → `q9board.c`-if/else-Kette:
REMAP → 2× Timer-Trigger → RTC → CF → UART → *erst dann* RAM), Hauptschleifen-Poll
(`q9boardrun.c`), IRQ-Ack (`m68krt_board_int_ack` + `m68krt_reassert_pending_irq`).
Jeder RAM-Zugriff läuft durch bis zu ~10 Bereichsabfragen, 16/32-Bit-Zugriffe
rufen den Byte-Pfad 2×/4× auf. Opcode-Fetches (16 Bit) sind der heißeste Pfad.

## 1. Geräte-Interface (5.17)

```c
typedef struct q9_device {
    const char *type;                  /* Modulname: "nettty", "quicc", "cf", ... */
    char        name[16];              /* Instanzname aus der Config              */
    uint32_t    base, size;            /* Adressfenster (size meldet das MODUL!)  */
    int         irq_level, irq_vector;
    void       *state;                 /* opaker Instanz-Zustand                  */

    uint8_t  (*read8)(struct q9_device *d, uint32_t off);
    void     (*write8)(struct q9_device *d, uint32_t off, uint8_t v);
    /* read16/32 + write16/32 optional: NULL = aus Byte-Zugriffen synthetisiert.
       CF braucht eigene (16/32-Bit-Datenregister-Protokoll)! */
    void     (*poll)(struct q9_device *d, uint32_t now_ms);
    int      (*irq_pending)(struct q9_device *d);
    void     (*reset)(struct q9_device *d);
} q9_device_t;
```

- Zugriffe kommen als **Offset** an (`addr - base`) — das Modul dekodiert seine
  Register selbst (wie heute schon `network_read8` intern: +0 Status, +2 RX, +4 TX).
- **Die Fensterlänge meldet das Modul**, nicht die Config (Hardware-Eigenschaft
  des Typs: RTC=16 Bytes, nettty=channels×$10, …). Vermeidet die Fehlerklasse
  "Config sagt 16, Modul dekodiert 32". Nur bei `ram`/`rom` ist `size` echte
  Konfiguration.
- Die bestehenden sechs Geräte (68681-DUART, CF, Timer-Trigger, RTC72421,
  nettty-Terminals, QUICC) ziehen EINZELN nacheinander um, Verhalten unverändert,
  je Gerät ein Commit + Boot-Test.
- **Adress-Trigger (REMAP, TI_IRQ_ON/OFF) werden Mini-Geräte** mit Fenster, deren
  read8/write8 nur den Seiteneffekt auslöst — keine Sonderbehandlung im Dispatch.

### Einbindung: statisch, explizite Registry (Entscheidung)

```c
/* device_registry.c — die einzige Stelle, die alle Typen kennt */
const q9_devtype_t *q9_devtypes[] = {
    &q9_dev_sc68681, &q9_dev_cf, &q9_dev_rtc72421,
    &q9_dev_nettty, &q9_dev_quicc, &q9_dev_ticker, NULL
};
```

**Alle Gerätetypen sind immer einkompiliert** (kostet nur KB); die Config
entscheidet nur über die INSTANZIIERUNG. Bewusst KEIN dlopen/LoadLibrary
(drei ABI-Pfade, wasm-untauglich, kein Nutzen bei Modulen dieser Größe) und
KEINE Linker-Magie (`__attribute__((constructor))`, Custom-Sections) — die
explizite Tabelle ist portabel über gcc/clang/w64devkit/emcc und man sieht auf
einen Blick, was im Binary steckt. Plattformspezifisches bleibt per
`#ifdef`/Makefile IM Modul (Q9_HAVE_VMNET-Muster); nicht verfügbare Backends
lehnt die Config-Validierung mit klarer Meldung ab.

## 2. Bus-Dispatch (5.18)

```c
typedef struct {
    uint32_t     base, top;   /* Fenster [base..top]                          */
    uint8_t     *mem;         /* != NULL: RAM/ROM — direkter Speicherzugriff  */
    q9_device_t *dev;         /* sonst: Geraet — vtable mit Offset            */
} q9_range_t;

typedef struct {
    uint8_t   *ram;           /* Schnellpfad: Haupt-RAM ab Adresse 0          */
    uint32_t   ram_len;
    q9_range_t r[Q9_MAX_RANGES];   /* sortiert; Suchreihenfolge s.u.          */
    int        n;
} q9_busmap_t;
```

**Aufbau beim Start (einmalig, unkritisch):**
1. Sammeln — Geräte melden Fenster aus dem Konstruktor, `ram`/`rom`-Abschnitte
   liefern Speicherzeiger; `ram0` (Bereich ab 0) wird als Schnellpfad herausgezogen.
2. Sortieren (Insertion-Sort reicht).
3. Überlappungs-Validierung benachbarter Einträge → Fehlermeldung mit BEIDEN
   Instanznamen + Adressen, Abbruch.
4. **ZWEI komplette busmaps bauen**: Reset-Zustand (ROM-Spiegel statt RAM) und
   Remap-Zustand (RAM ab 0). Der REMAP-Trigger tauscht nur noch einen globalen
   Zeiger (`g_bus = &map_remapped;`) — kein Zustands-Check mehr im heißen Pfad.

**Zugriff (der heiße Pfad):**
- `addr < ram_len` zuerst — >99 % aller Zugriffe inkl. Opcode-Fetches:
  1 Vergleich, direkter Zugriff. 16/32-Bit lesen/schreiben direkt big-endian
  (heute: 2×/4× durch die komplette Byte-Kette — größter Einzelgewinn).
- Danach Suche im Bereichs-Array. `mem`-Treffer (colored RAM, weitere ROMs):
  direkter Zugriff ohne Funktionsaufruf, auch 16/32-Bit direkt. `dev`-Treffer:
  vtable mit Offset. Kein Treffer: liest 0 / verwirft (wie heute).

**Suchstrategie (Andreas' Einwand, wichtig fürs Verständnis):** Primär
**lineare Abfrage, nach Zugriffshäufigkeit sortiert** — heißestes Gerät zuerst
(= 1 Vergleich für den häufigsten I/O-Fall, vermutlich das DUART-Statusregister).
Begründung: Bei ~8 Einträgen schlägt die Sprungvorhersage-/Cache-Freundlichkeit
des sequenziellen Durchlaufs die theoretisch weniger Vergleiche der Binärsuche
(datenabhängige Halbierungs-Branches ≈ 15–20 Takte je Fehlvorhersage; das ganze
Array sind 2 Cachezeilen, der Prefetcher liebt den Linear-Scan). Die
**Binärsuche** (lo/hi-Halbierung über Indizes — Andreas' ADR_MIN/ADR_MAX-Prinzip)
wird als Rückfalloption MIT implementiert und im A/B-Test gemessen; relevant
erst ab deutlich mehr Einträgen (~32+). Die Sortierreihenfolge des Linear-Scans
kommt aus den Benchmark-Zählern, nicht aus Bauchgefühl.

**Benchmark (Teil von 5.18):** emulierte Zyklen/Wandzeit + Zugriffszähler je
Eintrag; dokumentiert Vorher/Nachher und Linear-vs-Binär.

## 3. Board-Konfigurationsdatei (5.19)

INI-artig, C99-Parser ohne Fremdbibliothek. Referenz-Beispiel (Stand der
Planungsrunde — namentliche Keys können sich in der Umsetzung noch schärfen):

```ini
; q9board.cfg — Board-Konfiguration Q9-Emulator
; Kommentare mit ';' oder '#', Adressen hex mit 0x, Groessen mit K/M-Suffix

[board]
name = Q9-Board

[ram0]                     ; Haupt-RAM ab 0 = Dispatch-Schnellpfad
type = ram
base = 0x00000000
size = 16M

[ram1]                     ; weiterer Bereich ("colored RAM") — beliebig viele
type      = ram
base      = 0x08000000
size      = 4M
preload   = ramdisk.img    ; optional: Inhalt beim Start einspielen
os9_color = 1              ; nur fuer den Descriptor-/init-Generator (5.20)

[rom0]
type    = rom
base    = 0xFE000000
size    = 512K
preload = romimage.dev.running.BIN
mirror_at_reset = yes      ; Board-Eigenart: ROM-Spiegel im Reset-Zustand

[remap]
type = remap_trigger       ; Adress-Trigger als Mini-Geraet
base = 0xFFFF8000

[uart0]
type  = sc68681
base  = 0xFFFE0000
level = 3                  ; Vektor liefert die DUART selbst per IVR

[timer]
type   = ticker
base   = 0xFFFF9000        ; TI_IRQ_OFF/ON-Fenster
level  = 6
period = 10ms              ; 100 Hz

[rtc0]
type = rtc72421
base = 0xFFFFD000

[cf0]
type  = cf
base  = 0xFFFFE000
image = local_images/OS9SYS.hda

[xterm]
type     = nettty
base     = 0xFFFF1000     ; 2026-08-14: 256 Byte je Kanal (eigener I/O-Tabellenplatz), s. q9board.h
level    = 4
vector   = 70              ; Kanal n bekommt vector+n (70..77)
channels = 8
port     = 2000            ; TCP-Port am Host
os9_names  = x1..x8        ; fuer den Generator (5.20)
os9_driver = nettty
os9_fm     = scf

[eth0]
type    = quicc
base    = 0xFFFF2000
level   = 5
vector  = 254
backend = vmnet            ; nat | vmnet | bridge:<ifname>
mac     = 00:73:39:33:36:30
```

**Regeln:**
- **`[abschnitt]` = frei wählbarer INSTANZname, `type=` = MODULname.** So gehen
  mehrere Instanzen desselben Typs (`[xterm2] type=nettty port=2001 ...`).
- **`size` NUR bei `ram`/`rom`** — Geräte melden ihre Fensterlänge selbst.
- `type` + `base` immer Pflicht; `level`/`vector` nur bei IRQ-Geräten;
  typspezifische Parameter haben Defaults im Modul.
- **`os9_*`-Keys** transportieren die rein OS-9-seitigen Angaben für den
  Generator (5.20) in DERSELBEN Datei; der Emulator ignoriert sie. So bleibt
  eine Datei die einzige Quelle der Wahrheit für beide Welten.
- Pfade in der Datei relativ zur **Config-Datei** (nicht zum CWD).
- `--config <datei>`; ohne Angabe `./q9board.cfg`; existiert auch das nicht:
  eingebaute Default-Config = **heutiges Board byte-genau** (Adressen/Vektoren/
  Level identisch — bestehende Images, Descriptoren, startup laufen unverändert).
- Prioritätsreihenfolge: eingebaute Defaults → Config-Datei → CLI
  (bestehende Optionen `--rom`/`--cf`/`--net` bleiben als überschreibende
  Kurzformen für schnelle Testläufe).
- **Validierung beim Start** (lieber hart sterben als OS-9 mysteriös nicht
  booten lassen): Fensterüberlappung, Vektor-/Level-Kollisionen, base mitten
  im RAM, unbekannter type, auf der Plattform nicht verfügbares backend —
  Fehlermeldung mit Zeilennummer und Instanznamen.

## 4. Descriptor-Generator MWOS-Seite (5.20)

Tool (Python, wie der idbgen-Weg aus 5.14) liest DIESELBE Board-Config und
erzeugt die OS-9-Seite: `systype.d`-Fragmente + Descriptor-Quellen
(`x1.a`-Muster) für den MWOS-Q9-Port; gebaut wie gehabt über die
Wine-Toolchain (os9make). Adresse/Vektor/Level stehen damit nur noch an einer
Stelle. Ausbaustufe (separat zu entscheiden): Descriptor-Module direkt binär
erzeugen inkl. CRC (Modul-Format-Parser aus 5.15 existiert) — spart den
Wine-Roundtrip, kostet einen eigenen Binär-Generator in der Pflege.
Deployment ins Image bleibt wie gehabt (ToolShed + `attr -e -pe` + startup).

**Offene Fragen für die nächste Planungsrunde (Stand 2026-07-14, mit Andreas
zu klären, BEVOR 5.20 auf 🟢 geht):**
1. Schnitt der Generierung: lose `systype.d`-FRAGMENTE zum manuellen Einfügen,
   oder verwaltet der Generator einen markierten Block in Andreas' systype.d
   (`; === Q9-GENERATED BEGIN/END ===`) — wiederholbar ohne Handarbeit, fasst
   aber die bestehende Datei an?
2. Auch das `init`-Modul? Mit `os9_color` wäre es konsequent, auch die
   MemList-Einträge zu generieren — aber größerer Eingriff (init steckt im
   Bootfile/ROM, s. ROM-Rebuild-Drift unter "Geparkt").
3. Wann läuft der Generator? Tendenz: Handaufruf (`tools/q9desc.py
   q9board.cfg`) + `make`-Warnung bei "Config neuer als Generat" — KEIN
   Automatismus, der ungefragt ins MWOS-Verzeichnis schreibt.

## 5. Plattform-Strategie (5.21)

- **Winsock2-Shim** für Windows (WSAStartup/closesocket/ioctlsocket) — betrifft
  nettty-Server + Mini-NAT.
- **Netz-Backend-Abstufung:** Mini-NAT (reine Sockets) = portabler Default auf
  allen drei OS; vmnet/BPF bleiben macOS-Extras; TAP (Linux) / npcap (Windows)
  als spätere eigene Vorschläge. Ergebnis: /x1-8-Terminals + TCP/IP via
  Mini-NAT überall; echtes LAN-Bridging zunächst nur macOS.
- `make native` + `make test` auf allen drei OS grün (w64devkit-Pfad + conio-HAL
  existieren seit 0.1/0.4/1.10). Windows-Test auf Andreas' Desktop AF-PC.

## Reihenfolge und Absicherung

5.17 (Interface + Registry + Umzug, größtes Risiko zuerst) → 5.18 (Dispatch +
Benchmark) → 5.19 (Config) → 5.20 (Generator) → 5.21 (Plattformen). Nach jedem
Schritt: `make test` + Boot-Test gegen Klon-Image mit vollem Login über
Port 2000 (Vorgehen wie beim 5.16-CRLF-Fix). Empfohlener Einstieg in 5.17:
RTC72421 als kleinstes, rein lesendes Gerät zuerst umziehen.

#═════════════════════════════════════════════════════════════════════════════════════════════════
# EOF HWCONFIG.md
#═════════════════════════════════════════════════════════════════════════════════════════════════
