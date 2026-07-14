//════════════════════════════════════════════════════════════════════════════════════════════════
// File:   cb030.h                                                                         Ver. 1.90
// Owner:  AF
// Desc.:  CB030-Board-Emulation (Schritt 5.2, docs/CB030.md) — Bootstrap/Validierungs-Zwischenschritt
//         fuer die Musashi-Integration (5.1) mit dem originalen, proprietaeren Microware-OS-9-Boot-
//         ROM. 5.2a: RAM/ROM/Remap-Speicherlogik. 5.2b: 68681-DUART (nur SRA+THRA/RHRA wirklich
//         aktiv, Rest wird sauber angenommen). 5.2c: Compact-Flash (ATA-PIO-Minimalprotokoll,
//         Backing Store = Host-Datei). 5.2d: Timer/IRQ3 (kooperativ, s. q9_cb030_poll_timer).
//         Bewusst KEINE Musashi-Abhaengigkeit hier (cb030.c bleibt eigenstaendig testbar) — den
//         eigentlichen `m68k_set_irq()`-Aufruf macht der Aufrufer (m68krt.c/kernel.c), s.
//         q9_cb030_poll_timer's Rueckgabewert.
//
// Call:   q9_cb030_t b; q9_cb030_init(&b, rom, rom_len, ram, ram_len);
//         v = q9_cb030_read8(&b, addr); q9_cb030_write8(&b, addr, v); q9_cb030_reset(&b);
//         q9_cb030_cf_attach(&b, "cb030_cf.img");
//         if (q9_cb030_poll_timer(&b, q9_hal_ticks_ms())) q9_m68krt_set_irq(3);
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
//         │      │ Host-Interrupt (s. docs/CB030.md, Begruendung E8)                        │
// 26-07-05│ 1.40 │ 5.3: ROM-Spiegelgrenze korrigiert (bis 0xFEFF_FFFF statt 0x0800_0000,     │ CF
//         │      │ s. docs/CB030.md Speicherkarte — noetig, weil das echte Boot-ROM vor      │
//         │      │ dem REMAP hoch nach 0xFE00_xxxx springt), I/O auch VOR dem Remap          │
//         │      │ erreichbar, neuer Lade-Helfer q9_cb030_rom_load                           │
// 26-07-05│ 1.50 │ 5.5a: CF-Multi-Sektor — READ/WRITE SECTOR(S) zaehlen cf_sectcnt jetzt      │ CF
//         │      │ echt durch (0 = 256 Sektoren, ATA-Konvention), neues cf_remaining          │
// 26-07-14│ 1.60 │ 5.10: Netzwerk-Terminals 4 → 8 Kanaele (/x1../x8, $FFFF1010–$FFFF108F,     │ CF
//         │      │ Vektoren 70–77), Kanaltabelle aus dem Header nach m68krt.c verlegt         │
// 26-07-14│ 1.70 │ 5.6: RTC72421 bei $FFFFD000 — Lesen = Host-Uhr (BCD-Nibbles, Latch bei     │ CF
//         │      │ S1-Zugriff), Schreiben ignoriert                                           │
// 26-07-14│ 1.80 │ 5.17: devreg.h eingebunden, q9_devtype_duart68681-Vtable exportiert         │ CF
// 26-07-14│ 1.90 │ 5.17: q9_devtype_cf-Vtable exportiert (zweites umgezogenes Geraet)          │ CF
//═════════╧══════╧════════════════════════════════════════════════════════════════════════╧══════
#ifndef Q9_CB030_H
#define Q9_CB030_H

#include <stdint.h>
#include <stdio.h>
#include "devreg.h"                                    /* 5.17: q9_device_t/Vtable, s. devreg.h  */

#define Q9_CB030_OK          0
#define Q9_CB030_ERR_RAM    -1                       /* RAM fehlt */
#define Q9_CB030_ERR_ROM    -2                       /* ROM-Datei fehlt/leer/zu gross */

/* Adress-Konstanten aus docs/CB030.md ("Speicherkarte"/"Emulations-Architektur"). */
#define Q9_CB030_ROM_MIRROR_TOP    0xFEFFFFFFu        /* Reset-Zustand: ROM gespiegelt bis hier
                                                         (einschliesslich — "bis zum oberen Byte
                                                         des Adressraums", die I/O-Region ab
                                                         0xFFFF_0000 bleibt frei) */
#define Q9_CB030_ROM_REMAP_BASE    0xFE000000u        /* Remap-Zustand: ROM liegt einmal hier   */
#define Q9_CB030_ROM_REMAP_TOP     0xFE07FFFFu
#define Q9_CB030_REMAP_REG_BASE    0xFFFF8000u        /* REMAP-Register: reiner Adress-Trigger  */
#define Q9_CB030_REMAP_REG_TOP     0xFFFF8FFFu

// ===============================================================================================
// OS-9 Netzwerk Terminal Server Peripherie-Definitionen (Erweiterung Ver. 1.30; 5.10: 8 Kanaele)
// ===============================================================================================
#define MAX_CHANNELS 8
#define MAIN_LISTEN_PORT 2000

/* I/O-Bloecke je Kanal (3 Register: +0 Status, +2 RX-Data, +4 TX-Data), freie Luecke zwischen
   ROM-Spiegelgrenze (bis 0xFFFF_0000 frei, s. Q9_CB030_ROM_MIRROR_TOP) und REMAP-Register
   (Q9_CB030_REMAP_REG_BASE ab 0xFFFF_8000) — kollidiert bewusst NICHT mit dem RAM (anders als
   die urspruengliche 0x00FF00xx-Adressierung, die mitten im 16-MByte-RAM lag).
   5.10: OS-9-Geraetenamen sind /x1../x8 (t1.. existiert im MWOS-Port schon anderweitig). */
#define Q9_CB030_NET_X1_BASE       0xFFFF1010u
#define Q9_CB030_NET_X2_BASE       0xFFFF1020u
#define Q9_CB030_NET_X3_BASE       0xFFFF1030u
#define Q9_CB030_NET_X4_BASE       0xFFFF1040u
#define Q9_CB030_NET_X5_BASE       0xFFFF1050u
#define Q9_CB030_NET_X6_BASE       0xFFFF1060u
#define Q9_CB030_NET_X7_BASE       0xFFFF1070u
#define Q9_CB030_NET_X8_BASE       0xFFFF1080u
#define Q9_CB030_NET_BASE          Q9_CB030_NET_X1_BASE
#define Q9_CB030_NET_TOP           0xFFFF108Fu

typedef struct {
    int client_fd;
    unsigned char rx_data;
    unsigned char tx_data;
    unsigned char status;  // Bit 0 = RX Ready, Bit 1 = TX Empty
    unsigned int base_addr;
    int irq_level;
    int irq_vector;
    unsigned char last_was_cr;  // Telnet-NVT-Normalisierung: LF nach CR verwerfen (5.16)
} os9_uart_t;

/* Die Kanaltabelle selbst (channels[]) lebt seit 5.10 in m68krt.c — sie war hier als static im
   Header definiert und haette jeder weiteren einbindenden Uebersetzungseinheit eine eigene,
   unbenutzte Kopie beschert. */

/* 5.2d: Timer/IRQ3 — reine Adress-Trigger, kein Datenwert. */
#define Q9_CB030_TIRQ_OFF_BASE     0xFFFF9000u
#define Q9_CB030_TIRQ_OFF_TOP      0xFFFF97FFu
#define Q9_CB030_TIRQ_ON_BASE      0xFFFF9800u
#define Q9_CB030_TIRQ_ON_TOP       0xFFFF9FFFu
#define Q9_CB030_TIMER_PERIOD_MS   10u                /* 100 Hz */

/* 5.6: RTC72421 — Epson-Echtzeituhr, busadressiert, 16 Nibble-Register in 16 Bytes.
   LESEN liefert die Host-Uhr (Register 0..C als BCD-Nibbles, D/E/F Control), SCHREIBEN
   wird komplett ignoriert (die Host-Uhr ist die Wahrheit). Atomaritaet: ein Lesezugriff
   auf Register 0 (S1) frischt den internen Latch aus der Host-Uhr auf, alle weiteren
   Register lesen aus dem Latch — wer S1 zuerst liest (wie der rtclock-Treiber im
   MWOS-Q9-Port), bekommt einen in sich konsistenten Zeitstempel ohne Rollover-Risiko. */
#define Q9_CB030_RTC_BASE          0xFFFFD000u
#define Q9_CB030_RTC_TOP           0xFFFFD00Fu

/* 5.2c: Compact-Flash-Interface (docs/CB030.md, Abschnitt "Compact-Flash-Interface"). */
#define Q9_CB030_CF_BASE           0xFFFFE000u
#define Q9_CB030_CF_TOP            0xFFFFE0FFu
#define Q9_CB030_CF_CMD_READ       0x20u              /* READ SECTOR(S)  */
#define Q9_CB030_CF_CMD_WRITE      0x30u              /* WRITE SECTOR(S) */
#define Q9_CB030_CF_CMD_SETFEAT    0xEFu              /* SET FEATURES (8-Bit-Mode etc.) */
#define Q9_CB030_CF_STAT_BSY       0x80u
#define Q9_CB030_CF_STAT_DRQ       0x08u
#define Q9_CB030_CF_STAT_RDY       0x40u
#define Q9_CB030_CF_STAT_ERR       0x01u
#define Q9_CB030_CF_SECTOR_SIZE    512u

/* 5.2b: 68681-DUART (docs/CB030.md, Abschnitt "68681 DUART"). Nur die Adressen, die Aufrufer/
   Selbsttest wirklich brauchen, sind hier exponiert — der Rest des Registersatzes bleibt intern
   in cb030.c (wird nur sauber angenommen, s. Kommentar dort). */
#define Q9_CB030_UART_BASE   0xFFFFF000u
#define Q9_CB030_UART_TOP    0xFFFFFFFFu
#define Q9_CB030_UART_SRA    (Q9_CB030_UART_BASE + 0x02u)  /* Status A (lesen)                 */
#define Q9_CB030_UART_THRA   (Q9_CB030_UART_BASE + 0x06u)  /* Tx-Holding (schreiben) = RHRA-Adr.*/
#define Q9_CB030_UART_RX_FIFO_SIZE (4u * 1024u * 1024u)

typedef struct q9_cb030 {
    const uint8_t *rom;                               /* Boot-ROM-Inhalt, nur lesend            */
    uint32_t       rom_len;
    uint8_t       *ram;                                /* Emuliertes RAM (Groesse = SIM-Bestueckung) */
    uint32_t       ram_len;
    int            remapped;                           /* 0 = Reset-Zustand, 1 = nach REMAP-Trigger */

    /* 5.2b/5.4: DUART — Empfangs-FIFO (Kanal A = Konsole) + Register-Latches, die der
       OS-9-Treiber (sc68681) zurueckliest, um den Chip zu verifizieren: MR1/MR2 (einziges echtes
       R/W-Register der 68681, mit internem Zeiger) und IVR. S. cb030.c. */
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

    /* 5.2c: Compact-Flash — Backing Store lazy geoeffnet (Muster wie q9disk.img). */
    const char    *cf_path;
    FILE          *cf_file;
    uint32_t       cf_image_sector_size;                 /* 256 fuer alte RBF-Images, sonst 512 */
    uint32_t       cf_lba;
    uint8_t        cf_lba3;                              /* LBA bits 27..24 + DEV/LBA flags      */
    uint8_t        cf_sectcnt;
    uint8_t        cf_status;
    uint8_t        cf_sector[Q9_CB030_CF_SECTOR_SIZE];
    uint32_t       cf_pos;                              /* Index in cf_sector, 0..SECTOR_SIZE   */
    uint32_t       cf_transfer_size;                    /* 256 fuer alte RBF-Daten, IDENTIFY 512 */
    int            cf_write_pending;                     /* 1 waehrend WRITE-SECTOR-Datenphase   */
    uint32_t       cf_remaining;                          /* 5.5a: noch ausstehende Sektoren im  */
                                                           /* laufenden Kommando (cf_sectcnt==0   */
                                                           /* bedeutet 256, ATA-Konvention)        */

    /* 5.2d: Timer/IRQ3 — kooperativ per Host-Uhrzeit, s. q9_cb030_poll_timer.
       5.6: timer_synced=0 nach TI_IRQ_ON — der erste Poll loest sofort aus und startet
       die Tick-Epoche; danach werden verpasste Perioden einzeln nachgeholt. */
    int            timer_active;
    int            timer_synced;
    uint32_t       timer_last_ms;

    /* 5.6: RTC72421 — Latch der 13 Zeit-Register (S1..W) als fertige Nibbles, wird beim
       Lesen von Register 0 (bzw. beim allerersten Zugriff) aus der Host-Uhr befuellt. */
    uint8_t        rtc_regs[13];
    int            rtc_latch_valid;
} q9_cb030_t;


//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_cb030_init
// Desc.:    Bindet ROM- und RAM-Puffer an ein Board-Handle, Ausgangszustand = Reset (nicht remapped,
//           Timer aus, keine CF-Datei angehaengt). rom darf NULL/0 sein (reine RAM-Tests ohne
//           Boot-ROM). ram darf nicht NULL sein.
// Call:     err = q9_cb030_init(&b, rom, rom_len, ram, ram_len)
//════════════════════════════════════════════════════════════════════════════════════════════════
int q9_cb030_init(q9_cb030_t *b, const uint8_t *rom, uint32_t rom_len, uint8_t *ram, uint32_t ram_len);

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_cb030_reset
// Desc.:    Setzt den REMAP-Zustand auf "nicht remapped" zurueck (Reset-Zustand: ROM bei Adresse 0).
//           Ruehrt CF-Anhaengung/Timer-Zustand NICHT an (die ueberleben einen 68k-Reset).
// Call:     q9_cb030_reset(&b)
//════════════════════════════════════════════════════════════════════════════════════════════════
void q9_cb030_reset(q9_cb030_t *b);

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_cb030_rom_load
// Desc.:    5.3: Laedt eine Boot-ROM-Datei vom Host in den uebergebenen Puffer (fuer q9_cb030_init).
//           Das echte Microware-Boot-ROM ist proprietaer und liegt NUR lokal vor (Pfad kommt per
//           Kommandozeile, s. cb030run.c) — es wird NIE ins Repository eingecheckt. Fehler, wenn
//           die Datei fehlt, leer ist oder groesser als buf_max (Flash ist 512 KByte).
// Call:     err = q9_cb030_rom_load("cb030rom.bin", buf, sizeof(buf), &rom_len)
//════════════════════════════════════════════════════════════════════════════════════════════════
int q9_cb030_rom_load(const char *path, uint8_t *buf, uint32_t buf_max, uint32_t *out_len);

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_cb030_cf_attach
// Desc.:    Merkt sich den Dateipfad fuer die Compact-Flash-Karte (5.2c) — die Datei selbst wird
//           lazy beim ersten Kommando geoeffnet/angelegt (Muster wie die native HAL bei q9disk.img,
//           s. hal_native.c/hal_posix.c). path muss die gesamte Lebensdauer von b ueberleben
//           (wird nur als Zeiger gehalten, nicht kopiert).
// Call:     q9_cb030_cf_attach(&b, "cb030_cf.img")
//════════════════════════════════════════════════════════════════════════════════════════════════
void q9_cb030_cf_attach(q9_cb030_t *b, const char *path);

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_cb030_poll_timer
// Desc.:    5.2d: Kooperative Zeitpruefung (KEIN echter Host-Interrupt, s. docs/CB030.md) — muss
//           regelmaessig vom Aufrufer aufgerufen werden (dort, wo auch m68k_execute() angestossen
//           wird). Liefert 1 zurueck, wenn seit dem letzten Auslösen >= Q9_CB030_TIMER_PERIOD_MS
//           vergangen sind UND der Timer per TI_IRQ_ON aktiv ist — der Aufrufer muss dann
//           q9_m68krt_set_irq(3) aufrufen (cb030.c kennt Musashi bewusst nicht). Liefert sonst 0.
// Call:     if (q9_cb030_poll_timer(&b, q9_hal_ticks_ms())) q9_m68krt_set_irq(3);
//════════════════════════════════════════════════════════════════════════════════════════════════
int q9_cb030_poll_timer(q9_cb030_t *b, uint32_t now_ms);

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_cb030_uart_irq_pending
// Desc.:    5.4: Liefert 1, wenn die DUART laut Interrupt-Maske (IMR) einen Interrupt anfordern
//           wuerde — TxRDYA (Bit 0, bei uns immer sendebereit) oder RxRDYA (Bit 1, Zeichen im
//           Empfangspuffer; pollt dazu die HAL nach). Der Aufrufer legt dann IRQ3 an (dieselbe
//           Leitung wie der 100Hz-Timer — OS-9 verteilt geteilte Level ueber seine IRQ-Polling-
//           Tabelle). Wie beim Timer gilt: kooperativ, der Aufrufer fragt regelmaessig ab.
// Call:     if (q9_cb030_poll_timer(&b, now) | q9_cb030_uart_irq_pending(&b)) q9_m68krt_set_irq(3);
//════════════════════════════════════════════════════════════════════════════════════════════════
int q9_cb030_uart_irq_pending(q9_cb030_t *b);

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_cb030_read8/16/32
// Desc.:    Liest ein Byte/Word/Long-Word aus der Board-Adresse 'addr'. I/O (REMAP/Timer/CF/UART,
//           gesamter 0xFFFF_xxxx-Bereich) ist in BEIDEN Zustaenden erreichbar — das Boot-ROM
//           initialisiert die DUART vor dem Remap. Darunter je nach REMAP-Zustand (docs/CB030.md):
//           Reset = ROM gespiegelt bis Q9_CB030_ROM_MIRROR_TOP (einschl.); remapped = RAM ab 0,
//           ROM einmalig bei Q9_CB030_ROM_REMAP_BASE. Ein Zugriff auf den REMAP-Registerbereich
//           schaltet IMMER (unabhaengig vom bisherigen Zustand) auf remapped um. TI_IRQ_ON/OFF-
//           Bereiche sind reine Adress-Trigger (Lesewert 0, s. 5.2d). UART/CF haben echtes (wenn
//           auch minimales) Verhalten, s. cb030.c. 16/32-Bit sind big-endian (68k-Byteorder),
//           wie m68krt.c.
// Call:     v = q9_cb030_read8(&b, addr)
//════════════════════════════════════════════════════════════════════════════════════════════════
uint8_t  q9_cb030_read8(q9_cb030_t *b, uint32_t addr);
uint16_t q9_cb030_read16(q9_cb030_t *b, uint32_t addr);
uint32_t q9_cb030_read32(q9_cb030_t *b, uint32_t addr);

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_cb030_write8/16/32
// Desc.:    Schreibt ein Byte/Word/Long-Word auf die Board-Adresse 'addr'. Schreibzugriffe auf ROM
//           (in beiden REMAP-Zustaenden) werden verworfen. Ein Zugriff auf den REMAP- oder
//           TI_IRQ_ON/OFF-Registerbereich schaltet um bzw. (de-)aktiviert den Timer (Wert wird
//           verworfen, reine Adress-Trigger, s. docs/CB030.md). UART/CF haben echtes Verhalten
//           und sind — wie beim Lesen — auch VOR dem Remap erreichbar.
// Call:     q9_cb030_write8(&b, addr, val)
//════════════════════════════════════════════════════════════════════════════════════════════════
void q9_cb030_write8(q9_cb030_t *b, uint32_t addr, uint8_t val);
void q9_cb030_write16(q9_cb030_t *b, uint32_t addr, uint16_t val);
void q9_cb030_write32(q9_cb030_t *b, uint32_t addr, uint32_t val);

//════════════════════════════════════════════════════════════════════════════════════════════════
// 5.17: Geraete-Vtables der bisher hier eingebauten Board-Geraete (s. devreg.h fuer das Konzept).
// dev->state zeigt bei allen dreien auf das q9_cb030_t-Board selbst (kein separater Zustand noetig
// -- die Register/Puffer bleiben in q9_cb030_t, nur der DISPATCH wandert aus cb030_read_byte/
// cb030_write_byte in die generische Registry). Instanzen werden von m68krt.c angelegt.
//════════════════════════════════════════════════════════════════════════════════════════════════
extern const q9_device_vtable_t q9_devtype_duart68681;   /* 5.17: 68681-DUART                    */
extern const q9_device_vtable_t q9_devtype_cf;            /* 5.17: Compact-Flash (eigene 16/32-Bit-Pfade) */

#endif // Q9_CB030_H

//────────────────────────────────────────────────────────────────────────────────────────────────
// EOF cb030.h                                                                             Ver. 1.90
//────────────────────────────────────────────────────────────────────────────────────────────────
