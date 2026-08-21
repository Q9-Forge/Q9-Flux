//════════════════════════════════════════════════════════════════════════════════════════════════
// File:   q9board.h                                                                         Ver. 2.30
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
//         5.2a: RAM/ROM/Remap-Speicherlogik. 5.2b: 68681-DUART (nur SRA+THRA/RHRA wirklich
//         aktiv, Rest wird sauber angenommen). 5.2c: Compact-Flash (ATA-PIO-Minimalprotokoll,
//         Backing Store = Host-Datei). 5.2d: Timer/IRQ3 (kooperativ, s. q9_board_poll_timer).
//         Bewusst KEINE Musashi-Abhaengigkeit hier (q9board.c bleibt eigenstaendig testbar) — den
//         eigentlichen `m68k_set_irq()`-Aufruf macht der Aufrufer (m68krt.c/kernel.c), s.
//         q9_board_poll_timer's Rueckgabewert.
//
// Call:   q9_board_t b; q9_board_init(&b, rom, rom_len, ram, ram_len);
//         v = q9_board_read8(&b, addr); q9_board_write8(&b, addr, v); q9_board_reset(&b);
//         q9_board_cf_attach(&b, "local_images/board_cf.img");
//         if (q9_board_poll_timer(&b, q9_hal_ticks_ms())) q9_m68krt_set_irq(3);
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
//═════════╧══════╧════════════════════════════════════════════════════════════════════════╧══════
#ifndef Q9_BOARD_H
#define Q9_BOARD_H

#include <stdint.h>
#include <stdio.h>
#include "devreg.h"                                    /* 5.17: q9_device_t/Vtable, s. devreg.h  */
#include "../devices/cf/cf.h"                          /* 2026-08-20: q9_cf_t/q9_devtype_cf, aus  */
                                                        /* q9board.c/.h hierher verschoben          */

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
#define Q9_BOARD_REMAP_REG_BASE    0xFFFF8000u        /* REMAP-Register: reiner Adress-Trigger  */
#define Q9_BOARD_REMAP_REG_TOP     0xFFFF8FFFu

// ===============================================================================================
// OS-9 Netzwerk Terminal Server Peripherie-Definitionen (Erweiterung Ver. 1.30; 5.10: 8 Kanaele)
// ===============================================================================================
#define MAX_CHANNELS 8
#define MAIN_LISTEN_PORT 2000

/* I/O-Bloecke je Kanal (3 Register: +0 Status, +2 RX-Data, +4 TX-Data), freie Luecke zwischen
   ROM-Spiegelgrenze (bis 0xFFFF_0000 frei, s. Q9_BOARD_ROM_MIRROR_TOP) und REMAP-Register
   (Q9_BOARD_REMAP_REG_BASE ab 0xFFFF_8000) — kollidiert bewusst NICHT mit dem RAM (anders als
   die urspruengliche 0x00FF00xx-Adressierung, die mitten im 16-MByte-RAM lag).
   5.10: OS-9-Geraetenamen sind /x1../x8 (t1.. existiert im MWOS-Port schon anderweitig).

   2026-08-14 (ARBEITSPLAN 5.18-Fortsetzung, Andreas' Entscheidung: eigener 256-Byte-Bereich statt
   Index/Daten-Registerpaar -- "denke das ist erst mal einfacher"): Abstand von 16 auf 256 Byte
   erhoeht, damit jeder Kanal seinen EIGENEN Slot in der I/O-Dispatch-Tabelle bekommt (s. m68krt.c
   g_io_table, ARBEITSPLAN 5.18) -- vorher teilten sich alle acht Kanaele einen einzigen 256-Byte-
   Slot ($FFFF1000-$FFFF10FF), was fuer die Tabelle kein Problem war (EIN gemeinsamer devreg-
   Eintrag fuer alle acht, s.u.), aber keine Trennung auf Registry-Ebene erlaubte. Die eigentliche
   Registerdispatch-Logik (m68krt.c network_read8/write8) vergleicht ausschliesslich gegen
   channels[i].base_addr -- KEINE Annahme ueber den Abstand im Code, daher genuegt hier die reine
   Konstantenaenderung, keine Logikaenderung. NUR die x1..x8-Basisadressen selbst haben sich
   geaendert (x1 z.B. $FFFF1010 -> $FFFF1000) -- die Register-OFFSETS innerhalb eines Kanals
   (+0/+2/+4) sind unveraendert. MUSS mit den entsprechenden Konstanten in der OS-9-seitigen
   systype.d (Q9-Port-Repo, _NETX1_Base.._NETX8_Base/_NETX_Spacing) synchron gehalten werden --
   sonst findet der Treiber die Kanaele nicht mehr. */
#define Q9_BOARD_NET_X1_BASE       0xFFFF1000u
#define Q9_BOARD_NET_X2_BASE       0xFFFF1100u
#define Q9_BOARD_NET_X3_BASE       0xFFFF1200u
#define Q9_BOARD_NET_X4_BASE       0xFFFF1300u
#define Q9_BOARD_NET_X5_BASE       0xFFFF1400u
#define Q9_BOARD_NET_X6_BASE       0xFFFF1500u
#define Q9_BOARD_NET_X7_BASE       0xFFFF1600u
#define Q9_BOARD_NET_X8_BASE       0xFFFF1700u
#define Q9_BOARD_NET_BASE          Q9_BOARD_NET_X1_BASE
#define Q9_BOARD_NET_TOP           0xFFFF17FFu             /* X8_BASE + 0xFF: letzter Kanal-Slot voll erfasst */

typedef struct {
    int client_fd;
    unsigned char rx_data;
    unsigned char tx_data;
    unsigned char status;  // Bit 0 = RX Ready, Bit 1 = TX Empty
    unsigned int base_addr;
    int irq_level;
    int irq_vector;
    unsigned char last_was_cr;  // Telnet-NVT-Normalisierung: LF nach CR verwerfen (5.16)
    unsigned char telnet_state; // IAC-Optionsverhandlung rausfiltern statt an OS-9 durchzureichen
                                 // (0=Daten, 1=nach IAC, 2=nach WILL/WONT/DO/DONT, 3=in SB, 4=in SB nach IAC)
} os9_uart_t;

/* Die Kanaltabelle selbst (channels[]) lebt seit 5.10 in m68krt.c — sie war hier als static im
   Header definiert und haette jeder weiteren einbindenden Uebersetzungseinheit eine eigene,
   unbenutzte Kopie beschert. */

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
   MWOS-Q9-Port), bekommt einen in sich konsistenten Zeitstempel ohne Rollover-Risiko. */
#define Q9_BOARD_RTC_BASE          0xFFFFD000u
#define Q9_BOARD_RTC_TOP           0xFFFFD00Fu

/* 2026-08-20: Compact-Flash-Adress-/Registerkonstanten sind nach src/devices/cf/cf.h umgezogen
   (Hardware-Vereinheitlichung, Pilot "cf") -- via #include oben weiterhin hier sichtbar. */

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

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_board_poll_timer
// Desc.:    5.2d: Kooperative Zeitpruefung (KEIN echter Host-Interrupt, s. docs/BOARD.md) — muss
//           regelmaessig vom Aufrufer aufgerufen werden (dort, wo auch m68k_execute() angestossen
//           wird). Liefert 1 zurueck, wenn seit dem letzten Auslösen >= Q9_BOARD_TIMER_PERIOD_MS
//           vergangen sind UND der Timer per TI_IRQ_ON aktiv ist — der Aufrufer muss dann
//           q9_m68krt_set_irq(3) aufrufen (q9board.c kennt Musashi bewusst nicht). Liefert sonst 0.
// Call:     if (q9_board_poll_timer(&b, q9_hal_ticks_ms())) q9_m68krt_set_irq(3);
//════════════════════════════════════════════════════════════════════════════════════════════════
int q9_board_poll_timer(q9_board_t *b, uint32_t now_ms);

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_board_uart_irq_pending
// Desc.:    5.4: Liefert 1, wenn die DUART laut Interrupt-Maske (IMR) einen Interrupt anfordern
//           wuerde — TxRDYA (Bit 0, bei uns immer sendebereit) oder RxRDYA (Bit 1, Zeichen im
//           Empfangspuffer; pollt dazu die HAL nach). Der Aufrufer legt dann IRQ3 an (dieselbe
//           Leitung wie der 100Hz-Timer — OS-9 verteilt geteilte Level ueber seine IRQ-Polling-
//           Tabelle). Wie beim Timer gilt: kooperativ, der Aufrufer fragt regelmaessig ab.
// Call:     if (q9_board_poll_timer(&b, now) | q9_board_uart_irq_pending(&b)) q9_m68krt_set_irq(3);
//════════════════════════════════════════════════════════════════════════════════════════════════
int q9_board_uart_irq_pending(q9_board_t *b);

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
// 5.17: Geraete-Vtables der bisher hier eingebauten Board-Geraete (s. devreg.h fuer das Konzept).
// dev->state zeigt bei allen dreien auf das q9_board_t-Board selbst (kein separater Zustand noetig
// -- die Register/Puffer bleiben in q9_board_t, nur der DISPATCH wandert aus board_read_byte/
// board_write_byte in die generische Registry). Instanzen werden von m68krt.c angelegt.
//════════════════════════════════════════════════════════════════════════════════════════════════
extern const q9_device_vtable_t q9_devtype_duart68681;   /* 5.17: 68681-DUART                    */
/* q9_devtype_cf: jetzt in cf.h deklariert (s.o., #include), Hardware-Vereinheitlichung 2026-08-20 */
extern const q9_device_vtable_t q9_devtype_timer_irq;      /* 5.17: TI_IRQ_ON/OFF-Adress-Trigger    */
extern const q9_device_vtable_t q9_devtype_rtc72421;        /* 5.17: RTC72421-Echtzeituhr             */

#endif // Q9_BOARD_H

//────────────────────────────────────────────────────────────────────────────────────────────────
// EOF q9board.h                                                                             Ver. 2.20
//────────────────────────────────────────────────────────────────────────────────────────────────
