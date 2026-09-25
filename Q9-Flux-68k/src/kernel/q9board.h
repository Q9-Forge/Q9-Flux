//════════════════════════════════════════════════════════════════════════════════════════════════
// File:   q9board.h                                                                         Ver. 2.50
// Owner:  AF
// Desc.:  Board-Emulation (Schritt 5.2, docs/BOARD.md) — Bootstrap/Validierungs-Zwischenschritt
//         fuer die Musashi-Integration (5.1) mit einem originalen, proprietaeren OS-9-Boot-ROM.
//
//         **Nachgebildete Hardware (einzige bewusste Nennung im Quellbaum, s.u.):** das Q9-Board
//         orientiert sich am CB030 (68030-Bastelrechner aus dem RetroBrew-Computers-Projekt) --
//         daher stammen die Adresslage der Peripherie (DUART, CF, RTC, Remap-Register) und das
//         ROM-Layout. Ueberall sonst heisst es im Code neutral "Board"/"q9board", damit der Name
//         der Fremdhardware nicht durch den gesamten Baum zieht; wer die Registerbelegung mit dem
//         realen Vorbild abgleichen will, findet die Einordnung hier und in docs/BOARD.md.
//
//         5.2a: RAM/ROM/Remap-Speicherlogik -- der einzige Teil, der HEUTE NOCH tatsaechlich hier
//         liegt (board_read_byte/board_write_byte). DUART/CF/RTC/Timer waren urspruenglich
//         ebenfalls hier (5.2b/5.2c/5.6/5.2d), sind aber im Zuge der Hardware-Vereinheitlichung
//         (2026-08-20/21) nach src/devices/duart68681/, src/devices/cf/, src/devices/rtc72421/
//         bzw. src/devices/timer_irq/ umgezogen -- q9_board_t buendelt weiterhin deren
//         Registerzustand als Struct-Member (keine eigene Instanz-Struct wie bei CF, s. dortige
//         Kopfkommentare), aber die Dispatch-/Registerlogik selbst lebt jetzt dort. Bewusst KEINE
//         Musashi-Abhaengigkeit hier (q9board.c bleibt eigenstaendig testbar).
//
// Call:   q9_board_t b; q9_board_init(&b, rom, rom_len, ram, ram_len);
//         v = q9_board_read8(&b, addr); q9_board_write8(&b, addr, v); q9_board_reset(&b);
//         q9_board_cf_attach(&b, "local_images/board_cf.img");
//
// Edition History
//─────────┬──────┬────────────────────────────────────────────────────────────────────────┬──────
// Date    │ Ver. │ Description                                                            │ By
//─────────┼──────┼────────────────────────────────────────────────────────────────────────┼──────
// 26-07-04│ 1.00 │ 5.2a: Erster Grundbaustein — RAM/ROM/Remap-Adress-Dispatch              │ CF
// 26-07-04│ 1.10 │ 5.2b: 68681-DUART — Minimalansatz (nur SRA+THRA/RHRA wirklich aktiv,     │ CF
//         │      │ alle anderen Register werden nur sauber angenommen)                     │
// 26-07-04│ 1.20 │ 5.2c: Compact-Flash — ATA-PIO-Minimalprotokoll (READ/WRITE SECTOR(S)),   │ CF
//         │      │ Backing Store = lazy geoeffnete Host-Datei (Muster wie q9disk.img)       │
// 26-07-04│ 1.30 │ 5.2d: Timer/IRQ3 — kooperativ, Host-Uhrzeit zaehlen statt echtem          │ CF
//         │      │ Host-Interrupt (s. docs/BOARD.md, Begruendung E8)                        │
// 26-07-05│ 1.40 │ 5.3: ROM-Spiegelgrenze korrigiert (bis 0xFEFF_FFFF statt 0x0800_0000,     │ CF
//         │      │ s. docs/BOARD.md Speicherkarte — noetig, weil das echte Boot-ROM vor      │
//         │      │ dem REMAP hoch nach 0xFE00_xxxx springt), I/O auch VOR dem Remap          │
//         │      │ erreichbar, neuer Lade-Helfer q9_board_rom_load                           │
// 26-07-05│ 1.50 │ 5.5a: CF-Multi-Sektor — READ/WRITE SECTOR(S) zaehlen cf_sectcnt jetzt      │ CF
//         │      │ echt durch (0 = 256 Sektoren, ATA-Konvention), neues cf_remaining          │
// 26-07-14│ 1.60 │ 5.10: Netzwerk-Terminals 4 → 8 Kanaele (/x1../x8, $FFFF1010–$FFFF108F,     │ CF
//         │      │ Vektoren 70–77), Kanaltabelle aus dem Header nach m68krt.c verlegt         │
// 26-07-14│ 1.70 │ 5.6: RTC72421 bei $FFFFD000 — Lesen = Host-Uhr (BCD-Nibbles, Latch bei     │ CF
//         │      │ S1-Zugriff), Schreiben ignoriert                                           │
// 26-07-14│ 1.80 │ 5.17: devreg.h eingebunden, q9_devtype_duart68681-Vtable exportiert         │ CF
// 26-07-14│ 1.90 │ 5.17: q9_devtype_cf-Vtable exportiert (zweites umgezogenes Geraet)          │ CF
// 26-07-14│ 1.91 │ 5.17: q9_devtype_timer_irq exportiert, neues Feld timer_irq_pending         │ CF
//         │      │ (drittes umgezogenes Geraet, s. q9board.c)                                    │
// 26-07-14│ 1.92 │ 5.17: q9_devtype_rtc72421 exportiert (viertes/letztes board-internes Geraet)│ CF
// 26-07-16│ 2.00 │ 5.19a: CF-Zustand aus q9_board_t in eigene, mehrfach instanziierbare        │ CF
//         │      │ Struktur q9_cf_t gezogen (Onboard-CF + RC2014-SC145 bei $FFFFC010), zwei    │
//         │      │ Einheiten je Interface (Master/Slave via DEV-Bit in LBA3), Image-Format     │
//         │      │ rbf/pcf aus der Board-Config (s. boardcfg.h) steuert die Sektor-Heuristik   │
// 26-08-06│ 2.10 │ os9_uart_t: neues Feld telnet_state (Doppel-Echo-Bugfix, s. m68krt.c 1.38)  │ AF
// 26-08-14│ 2.20 │ 5.18-Fortsetzung: Q9_BOARD_NET_X1..X8_BASE von 16- auf 256-Byte-Abstand      │ Cld
//         │      │ umgestellt (eigener I/O-Tabellenplatz je Kanal, Andreas' Entscheidung).      │
//         │      │ NUR die Basisadressen -- Register-Offsets/Dispatch-Logik unveraendert. Muss  │
//         │      │ mit systype.d im Q9-Port-Repo synchron bleiben, s. Kommentar bei den Defines │
// 26-08-20│ 2.30 │ Hardware-Vereinheitlichung, Pilot "cf": Compact-Flash-Konstanten/-Structs/     │ Cld
//         │      │ -Vtable nach src/devices/cf/cf.h umgezogen (Andreas' Vorgabe: ein eigenes     │
//         │      │ Sourcefile je Hardware-Typ) -- via #include weiterhin transitiv sichtbar,      │
//         │      │ q9_board_cf_attach bleibt duenner Wrapper hier                                 │
// 26-08-21│ 2.40 │ Hardware-Vereinheitlichung, Folgeschritt: OS-9-Netzwerk-Terminal-Server-        │ Cld
//         │      │ Definitionen (MAX_CHANNELS/os9_uart_t/Q9_BOARD_NET_*) nach                       │
//         │      │ src/devices/nettty/nettty.h umgezogen -- letzter noch fehlender Typ von neun     │
// 26-08-21│ 2.50 │ Hardware-Vereinheitlichung, Andreas' Idee: REMAP-Trigger-Konstanten nach          │ Cld
//         │      │ src/devices/remap/remap.h umgezogen (der Trigger ist jetzt ein eigenes devreg-    │
//         │      │ Geraet)                                                                            │
//═════════╧══════╧════════════════════════════════════════════════════════════════════════╧══════
#ifndef Q9_BOARD_H
#define Q9_BOARD_H

#include <stdint.h>
#include <stdio.h>
#include "devreg.h"                                    /* 5.17: q9_device_t/Vtable, s. devreg.h  */
#include "../devices/cf/cf.h"                          /* 2026-08-20: q9_cf_t/q9_devtype_cf, aus  */
                                                        /* q9board.c/.h hierher verschoben          */
#include "../devices/dhf/q9_dhf.h"                     /* 2026-09-25: q9_dhf_t/q9_devtype_dhf      */

#define Q9_BOARD_OK          0
#define Q9_BOARD_ERR_RAM    -1                       /* RAM fehlt */
#define Q9_BOARD_ERR_ROM    -2                       /* ROM-Datei fehlt/leer/zu gross */

/* Adress-Konstanten aus docs/BOARD.md ("Speicherkarte"/"Emulations-Architektur"). */
#define Q9_BOARD_ROM_MIRROR_TOP    0xFEFFFFFFu        /* Reset-Zustand: ROM gespiegelt bis hier
                                                         (einschliesslich — "bis zum oberen Byte
                                                         des Adressraums", die I/O-Region ab
                                                         0xFFFF_0000 bleibt frei) */
#define Q9_BOARD_ROM_REMAP_BASE    0xFE000000u        /* Remap-Zustand: ROM liegt einmal hier   */
#define Q9_BOARD_ROM_REMAP_TOP     0xFE07FFFFu
/* Q9_BOARD_REMAP_REG_BASE/TOP: 2026-08-21 nach src/devices/remap/remap.h umgezogen
   (Hardware-Vereinheitlichung, der Trigger selbst ist jetzt ein eigenes devreg-Geraet). */

/* 2026-08-21: die OS-9-Netzwerk-Terminal-Server-Definitionen (MAX_CHANNELS/os9_uart_t/
   Q9_BOARD_NET_X1..X8_BASE/channels[]) sind nach src/devices/nettty/nettty.h/.c umgezogen
   (Hardware-Vereinheitlichung) -- s. dort. */

/* 5.2d: Timer/IRQ3 — reine Adress-Trigger, kein Datenwert. */
#define Q9_BOARD_TIRQ_OFF_BASE     0xFFFF9000u
#define Q9_BOARD_TIRQ_OFF_TOP      0xFFFF97FFu
#define Q9_BOARD_TIRQ_ON_BASE      0xFFFF9800u
#define Q9_BOARD_TIRQ_ON_TOP       0xFFFF9FFFu
#define Q9_BOARD_TIMER_PERIOD_MS   10u                /* 100 Hz */

/* 5.6: RTC72421 — Epson-Echtzeituhr, busadressiert, 16 Nibble-Register in 16 Bytes.
   LESEN liefert die Host-Uhr (Register 0..C als BCD-Nibbles, D/E/F Control), SCHREIBEN
   wird komplett ignoriert (die Host-Uhr ist die Wahrheit). Atomaritaet: ein Lesezugriff
   auf Register 0 (S1) frischt den internen Latch aus der Host-Uhr auf, alle weiteren
   Register lesen aus dem Latch — wer S1 zuerst liest (wie der rtclock-Treiber im
   REF-Q9-Port), bekommt einen in sich konsistenten Zeitstempel ohne Rollover-Risiko. */
#define Q9_BOARD_RTC_BASE          0xFFFFD000u
#define Q9_BOARD_RTC_TOP           0xFFFFD00Fu

/* 2026-08-20: Compact-Flash-Adress-/Registerkonstanten sind nach src/devices/cf/cf.h umgezogen
   (Hardware-Vereinheitlichung, Pilot "cf") -- via #include oben weiterhin hier sichtbar. */

/* 2026-09-25: DHF (Direct Host Filesystem, s. src/devices/dhf/q9_dhf.h) -- freie Luecke zwischen
   nettty ($FFFF1000-17FF) und QUICC ($FFFF2000-3FFF, 8K-aligned) einerseits und REMAP ($FFFF8000)
   andererseits: $FFFF4000-7FFF, 16K frei, DHF braucht nur 4K (Q9_DHF_WINDOW_SIZE). */
#define Q9_BOARD_DHF_BASE          0xFFFF4000u
#define Q9_BOARD_DHF_TOP           0xFFFF4FFFu

/* 5.2b: 68681-DUART (docs/BOARD.md, Abschnitt "68681 DUART"). Nur die Adressen, die Aufrufer/
   Selbsttest wirklich brauchen, sind hier exponiert — der Rest des Registersatzes bleibt intern
   in q9board.c (wird nur sauber angenommen, s. Kommentar dort). */
#define Q9_BOARD_UART_BASE   0xFFFFF000u
#define Q9_BOARD_UART_TOP    0xFFFFFFFFu
#define Q9_BOARD_UART_SRA    (Q9_BOARD_UART_BASE + 0x02u)  /* Status A (lesen)                 */
#define Q9_BOARD_UART_THRA   (Q9_BOARD_UART_BASE + 0x06u)  /* Tx-Holding (schreiben) = RHRA-Adr.*/
#define Q9_BOARD_UART_RX_FIFO_SIZE (4u * 1024u * 1024u)

typedef struct q9_board {
    const uint8_t *rom;                               /* Boot-ROM-Inhalt, nur lesend            */
    uint32_t       rom_len;
    uint8_t       *ram;                                /* Emuliertes RAM (Groesse = SIM-Bestueckung) */
    uint32_t       ram_len;
    int            remapped;                           /* 0 = Reset-Zustand, 1 = nach REMAP-Trigger */

    /* 5.2b/5.4: DUART — Empfangs-FIFO (Kanal A = Konsole) + Register-Latches, die der
       OS-9-Treiber (sc68681) zurueckliest, um den Chip zu verifizieren: MR1/MR2 (einziges echtes
       R/W-Register der 68681, mit internem Zeiger) und IVR. S. q9board.c. */
    uint8_t       *uart_rx_fifo;
    uint32_t       uart_rx_fifo_size;
    uint32_t       uart_rx_head;
    uint32_t       uart_rx_tail;
    uint32_t       uart_rx_count;
    uint32_t       uart_rx_overflow;
    int            uart_mr_ptr_a;                      /* 0 = naechster Zugriff MR1A, 1 = MR2A   */
    uint8_t        uart_mr_a[2];
    int            uart_mr_ptr_b;
    uint8_t        uart_mr_b[2];
    uint8_t        uart_ivr;
    uint8_t        uart_imr;                           /* Interrupt-Mask-Latch (Schreiben 0x0A)  */

    /* 5.2c/5.19a: Onboard-Compact-Flash — kompletter Interface-Zustand in q9_cf_t (s.o.), damit
       das RC2014-Zweitinterface dieselbe Emulation als eigene Instanz nutzen kann. */
    q9_cf_t        cf;

    /* 5.2d: Timer/IRQ3 — kooperativ per Host-Uhrzeit, s. q9_board_poll_timer.
       5.6: timer_synced=0 nach TI_IRQ_ON — der erste Poll loest sofort aus und startet
       die Tick-Epoche; danach werden verpasste Perioden einzeln nachgeholt. */
    int            timer_active;
    int            timer_synced;
    uint32_t       timer_last_ms;
    /* 5.17: Transienter "hat der letzte poll() einen Tick ausgeloest"-Merker fuer den Geraete-
       Vtable-Adapter (timer_dev_poll/timer_dev_irq_pending, s. q9board.c) -- NUR fuer die eine
       Runde gueltig, in der q9_device_poll() aufgerufen wurde (Hauptschleife, q9boardrun.c);
       bewusst NICHT level-held (s. devreg.h) und deshalb nicht Teil der IACK-/Reassert-
       Pruefschleife in m68krt.c, exakt wie vor 5.17 (der Timer wird nie erneut angestossen,
       bevor der naechste Tick faellig ist). */
    int            timer_irq_pending;

    /* 5.6: RTC72421 — Latch der 13 Zeit-Register (S1..W) als fertige Nibbles, wird beim
       Lesen von Register 0 (bzw. beim allerersten Zugriff) aus der Host-Uhr befuellt. */
    uint8_t        rtc_regs[13];
    int            rtc_latch_valid;

    /* 2026-09-25: DHF (Direct Host Filesystem) -- kompletter Zustand in q9_dhf_t (s.o.), analog
       "cf" oben: eigener Sourcebaum src/devices/dhf/, statisch eingebettet. */
    q9_dhf_t       dhf;
} q9_board_t;


//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_board_init
// Desc.:    Bindet ROM- und RAM-Puffer an ein Board-Handle, Ausgangszustand = Reset (nicht remapped,
//           Timer aus, keine CF-Datei angehaengt). rom darf NULL/0 sein (reine RAM-Tests ohne
//           Boot-ROM). ram darf nicht NULL sein.
// Call:     err = q9_board_init(&b, rom, rom_len, ram, ram_len)
//════════════════════════════════════════════════════════════════════════════════════════════════
int q9_board_init(q9_board_t *b, const uint8_t *rom, uint32_t rom_len, uint8_t *ram, uint32_t ram_len);

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_board_reset
// Desc.:    Setzt den REMAP-Zustand auf "nicht remapped" zurueck (Reset-Zustand: ROM bei Adresse 0).
//           Ruehrt CF-Anhaengung/Timer-Zustand NICHT an (die ueberleben einen 68k-Reset).
// Call:     q9_board_reset(&b)
//════════════════════════════════════════════════════════════════════════════════════════════════
void q9_board_reset(q9_board_t *b);

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_board_rom_load
// Desc.:    5.3: Laedt eine Boot-ROM-Datei vom Host in den uebergebenen Puffer (fuer q9_board_init).
//           Das echte Microware-Boot-ROM ist proprietaer und liegt NUR lokal vor (Pfad kommt per
//           Kommandozeile, s. q9boardrun.c) — es wird NIE ins Repository eingecheckt. Fehler, wenn
//           die Datei fehlt, leer ist oder groesser als buf_max (Flash ist 512 KByte).
// Call:     err = q9_board_rom_load("boardrom.bin", buf, sizeof(buf), &rom_len)
//════════════════════════════════════════════════════════════════════════════════════════════════
int q9_board_rom_load(const char *path, uint8_t *buf, uint32_t buf_max, uint32_t *out_len);

/* 2026-08-20: q9_cf_attach/q9_cf_set_start_sector sind jetzt in src/devices/cf/cf.h deklariert
   (Hardware-Vereinheitlichung, Pilot "cf") -- via #include oben weiterhin hier sichtbar. */

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_board_cf_attach
// Desc.:    Bisheriger Einzel-Image-Weg (5.2c): Image an die Master-Einheit der Onboard-CF,
//           Format-Autoerkennung — jetzt ein duenner Wrapper um q9_cf_attach (5.19a).
// Call:     q9_board_cf_attach(&b, "local_images/board_cf.img")
//════════════════════════════════════════════════════════════════════════════════════════════════
void q9_board_cf_attach(q9_board_t *b, const char *path);

/* 2026-08-21: q9_board_poll_timer/q9_board_uart_irq_pending sind nach src/devices/timer_irq/
   timer_irq.c bzw. src/devices/duart68681/duart68681.c umgezogen (Hardware-Vereinheitlichung) --
   dort jetzt `static` (kein externer Aufrufer ausser der jeweils eigenen Vtable, s. dortige
   Kommentare). Kein Ersatz noetig -- niemand ausserhalb rief sie je direkt auf. */

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_board_read8/16/32
// Desc.:    Liest ein Byte/Word/Long-Word aus der Board-Adresse 'addr'. I/O (REMAP/Timer/CF/UART,
//           gesamter 0xFFFF_xxxx-Bereich) ist in BEIDEN Zustaenden erreichbar — das Boot-ROM
//           initialisiert die DUART vor dem Remap. Darunter je nach REMAP-Zustand (docs/BOARD.md):
//           Reset = ROM gespiegelt bis Q9_BOARD_ROM_MIRROR_TOP (einschl.); remapped = RAM ab 0,
//           ROM einmalig bei Q9_BOARD_ROM_REMAP_BASE. Ein Zugriff auf den REMAP-Registerbereich
//           schaltet IMMER (unabhaengig vom bisherigen Zustand) auf remapped um. TI_IRQ_ON/OFF-
//           Bereiche sind reine Adress-Trigger (Lesewert 0, s. 5.2d). UART/CF haben echtes (wenn
//           auch minimales) Verhalten, s. q9board.c. 16/32-Bit sind big-endian (68k-Byteorder),
//           wie m68krt.c.
// Call:     v = q9_board_read8(&b, addr)
//════════════════════════════════════════════════════════════════════════════════════════════════
uint8_t  q9_board_read8(q9_board_t *b, uint32_t addr);
uint16_t q9_board_read16(q9_board_t *b, uint32_t addr);
uint32_t q9_board_read32(q9_board_t *b, uint32_t addr);

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_board_write8/16/32
// Desc.:    Schreibt ein Byte/Word/Long-Word auf die Board-Adresse 'addr'. Schreibzugriffe auf ROM
//           (in beiden REMAP-Zustaenden) werden verworfen. Ein Zugriff auf den REMAP- oder
//           TI_IRQ_ON/OFF-Registerbereich schaltet um bzw. (de-)aktiviert den Timer (Wert wird
//           verworfen, reine Adress-Trigger, s. docs/BOARD.md). UART/CF haben echtes Verhalten
//           und sind — wie beim Lesen — auch VOR dem Remap erreichbar.
// Call:     q9_board_write8(&b, addr, val)
//════════════════════════════════════════════════════════════════════════════════════════════════
void q9_board_write8(q9_board_t *b, uint32_t addr, uint8_t val);
void q9_board_write16(q9_board_t *b, uint32_t addr, uint16_t val);
void q9_board_write32(q9_board_t *b, uint32_t addr, uint32_t val);

//════════════════════════════════════════════════════════════════════════════════════════════════
/* 2026-08-21: q9_devtype_duart68681/timer_irq/rtc72421 (wie schon q9_devtype_cf seit 2026-08-20)
   sind jetzt in ihren jeweils eigenen Headern deklariert (src/devices/duart68681/duart68681.h,
   src/devices/timer_irq/timer_irq.h, src/devices/rtc72421/rtc72421.h) -- ANDERS als cf.h binden
   diese q9board.h selbst ein (sie brauchen q9_board_t, s. dortige Kopfkommentare), q9board.h bindet
   NICHT umgekehrt sie ein (vermeidet die Include-Zirkel-Falle) -- ein Aufrufer, der diese Vtables
   braucht (m68krt.c, devdesc.c), included sie deshalb explizit selbst statt sich auf eine
   transitive Weiterreichung durch q9board.h zu verlassen. */

#endif // Q9_BOARD_H

//────────────────────────────────────────────────────────────────────────────────────────────────
// EOF q9board.h                                                                             Ver. 2.20
//────────────────────────────────────────────────────────────────────────────────────────────────
