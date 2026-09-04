#define _GNU_SOURCE
//════════════════════════════════════════════════════════════════════════════════════════════════
// File:   m68krt.c                                                                        Ver. 1.70
// Owner:  AF
// Desc.:  Implementierung des Musashi-Wrappers, siehe m68krt.h. Definiert die sechs Speicherzugriffs-
//         Funktionen, die Musashi vom Host verlangt (m68k_read/write_memory_8/16/32 — deklariert in
//         m68k.h, aber nicht implementiert, s. third_party/musashi/m68kconf.h M68K_SEPARATE_READS
//         OFF: read_immediate/read_pcrelative fallen intern auf dieselben sechs Funktionen zurueck,
//         separate Varianten werden fuer Q9 nicht gebraucht).
//
// Edition History
//─────────┬──────┬────────────────────────────────────────────────────────────────────────┬──────
// Date    │ Ver. │ Description                                                            │ By
//─────────┼──────┼────────────────────────────────────────────────────────────────────────┼──────
// 26-07-04│ 1.00 │ 5.1: Erster Grundbaustein                                               │ CF
// 26-07-04│ 1.10 │ 5.2d: q9_m68krt_set_irq (Wrapper um m68k_set_irq())                     │ CF
// 26-07-05│ 1.20 │ 5.3: q9_m68krt_attach_board — Speicher-Hooks koennen wahlweise ueber     │ CF
//         │      │ den Board-Adress-Dispatch laufen (inkl. Autovector-Int-Ack)             │
// 26-07-14│ 1.30 │ 5.10: Netzwerk-Terminals 4 → 8 (/x1../x8), Kanaltabelle aus q9board.h      │ CF
//         │      │ hierher, network_irq_resync gegen verlorene Interrupts bei >1 Kanal      │
// 26-07-14│ 1.31 │ 5.17: Geraete-Registry (devreg.h) eingebunden -- 68681-DUART als erstes   │ CF
//         │      │ Geraet umgezogen (Dispatch + IACK/Reassert pruefen jetzt zuerst die       │
//         │      │ Registry); Netz-Terminals/QUICC/CF/Timer/RTC folgen einzeln               │
// 26-07-14│ 1.32 │ 5.17: Compact-Flash umgezogen (q9_devtype_cf, kein IRQ)                   │ CF
// 26-07-14│ 1.33 │ 5.17: Timer/IRQ3-Adress-Trigger umgezogen (q9_devtype_timer_irq)          │ CF
// 26-07-14│ 1.34 │ 5.17: RTC72421 umgezogen (q9_devtype_rtc72421) -- alle vier board-internen│ CF
//         │      │ Geraete jetzt in der Registry, nur noch nettty/QUICC hartkodiert           │
// 26-07-14│ 1.35 │ 5.17: Netz-Terminals /x1../x8 umgezogen (q9_devtype_nettty, EIN Geraet    │ CF
//         │      │ fuer alle acht Kanaele) -- channels[]-Fallback-Loops in IACK/Reassert       │
//         │      │ entfernt, nur noch QUICC hartkodiert                                        │
// 26-07-14│ 1.36 │ 5.17: QUICC umgezogen (q9_devtype_quicc, letztes von sechs Geraeten) --    │ CF
//         │      │ alle g_quicc-Sonderpruefungen in den sechs m68k_read/write_memory_*-        │
//         │      │ Funktionen sowie in IACK/Reassert entfernt; Timer/IRQ3 wird jetzt erst in   │
//         │      │ attach_quicc (nach QUICC) registriert, damit die Registrierungsreihenfolge  │
//         │      │ ueberall aufsteigend nach Level bleibt (3,4,5,6) -- s. dortige Kommentare    │
// 26-08-06│ 1.37 │ Nativer Windows-Build: init/update_network_terminals auf q9_sockcompat.h    │ AF
//         │      │ umgestellt (Winsock2 statt BSD-Sockets), write()/read() auf Socket-Fds durch │
//         │      │ send()/recv() ersetzt (auf Windows funktionieren CRT-read/write nicht auf    │
//         │      │ SOCKET-Handles)                                                              │
// 26-08-06│ 1.38 │ Doppel-Echo-Bugfix (Andreas' Report): Server verhandelte bisher gar kein     │ AF
//         │      │ Telnet -- IAC WILL ECHO/SUPPRESS-GA bei Connect + telnet_filter_byte()       │
//         │      │ filtert die IAC-Antwortsequenzen des Clients aus dem RX-Bytestrom            │
// 26-08-07│ 1.39 │ Ctrl-Q (konfigurierbar, Q9_NET_DISCONNECT_CTRL) trennt nur die eigene       │ AF
//         │      │ Telnet-Verbindung sauber -- Andreas' Wunsch nach einem Pendant zum lokalen   │
//         │      │ Ctrl-Q-Host-Escape (hal_windows.c/hal_posix.c), aber mit Kanal- statt          │
//         │      │ Prozess-Reichweite                                                           │
// 26-08-13│ 1.40 │ 6.5: q9_m68krt_get_backend -- befuellt eine q9_cpu_backend_t (cpu_backend.h) │ Cld
//         │      │ mit duennen Wrappern um reset/execute/set_irq/is_stopped, damit             │
//         │      │ q9boardrun.c nur noch ueber die Vtable auf die 68k-CPU zugreift              │
// 26-08-13│ 1.41 │ 5.18 (erster Teilschritt): RAM-Fast-Path (ram_fast_hit) in allen sechs       │ Cld
//         │      │ m68k_read/write_memory_*-Funktionen -- faengt RAM-Zugriffe im remapped-      │
//         │      │ Zustand VOR der Geraete-Registry-Suche (devreg_hit, bisher O(n) bei JEDEM    │
//         │      │ Zugriff) ab. Verifiziert per Boot-Gegenprobe: Boot-Transkript mit/ohne den    │
//         │      │ Fast-Path byte-identisch (git stash der Aenderung, neu gebaut, diff)          │
// 26-08-15│ 1.42 │ Q9FLUX_EDITOR_de.md 4.1: q9_m68krt_init bekommt echten cpu-Parameter          │ Cld
//         │      │ (q9_cpu_type_t) statt der bisher versteckten Q9_CPU=ec030-Env-Var-Abfrage --  │
//         │      │ Env-Var bleibt als Diagnose-Override ERHALTEN, greift aber nur noch, wenn      │
//         │      │ der Aufrufer den Default (Q9_CPU_68030) uebergeben hat (kein stiller           │
//         │      │ Ueberschreiber einer expliziten Wahl)                                          │
// 26-08-20│ 1.50 │ Hardware-Vereinheitlichung, Pilot "cf": q9_device_t.use_table (devreg.h) --      │ Cld
//         │      │ io_table_build() traegt Geraete mit use_table==0 als "ambiguous" ein (erzwingt   │
//         │      │ linearen Scan-Fallback statt automatischer Cluster-Zugehoerigkeit); alle zehn     │
//         │      │ q9_devreg_add()-Aufrufstellen explizit gesetzt (neun =1, framebuf =0 da unterhalb │
//         │      │ des Clusters)                                                                     │
// 26-08-21│ 1.60 │ Hardware-Vereinheitlichung, Folgeschritt: duart68681/rtc72421/timer_irq nach       │ Cld
//         │      │ eigene Dateien verschoben (waren zuvor in q9board.c) -- explizite Includes hier,    │
//         │      │ da q9board.h ihre Vtables nicht mehr transitiv re-exportiert (s. dortiger           │
//         │      │ Kommentar)                                                                          │
// 26-08-21│ 1.70 │ Hardware-Vereinheitlichung, letzter Typ: nettty (Netz-Terminals) komplett nach        │ Cld
//         │      │ src/devices/nettty/ verschoben. Andreas' Vorgabe "jedes Geraet einzeln, mit eigenem   │
//         │      │ Descriptor, eigener Adresse, im Array" -- ACHT separate devreg-Eintraege statt einem  │
//         │      │ gemeinsamen (q9_nettty_attach()), kein irq_vector_fn mehr noetig. Musashi-Entkopplung │
//         │      │ per Funktionszeiger-Hook (q9_nettty_set_irq_hook), hier mit q9_m68krt_set_irq          │
//         │      │ verdrahtet -- damit ist devdesc.c's Registry jetzt vollstaendig (alle neun Typen)      │
//═════════╧══════╧════════════════════════════════════════════════════════════════════════╧══════
#include "m68krt.h"
#include "q9board.h"
#include "../devices/quicc/quicc.h"
#include "../devices/duart68681/duart68681.h"          /* 2026-08-21: q9_devtype_duart68681,      */
                                                        /* aus q9board.h ausgelagert                */
#include "../devices/rtc72421/rtc72421.h"               /* 2026-08-21: q9_devtype_rtc72421          */
#include "../devices/timer_irq/timer_irq.h"             /* 2026-08-21: q9_devtype_timer_irq         */
#include "../devices/remap/remap.h"                       /* 2026-08-21: q9_devtype_remap             */
#include "../devices/nettty/nettty.h"                    /* 2026-08-21: q9_devtype_nettty, aus       */
                                                          /* q9board.h/m68krt.c ausgelagert           */
#include "devreg.h"
#include "m68k.h"
#include "q9_sockcompat.h"    /* Windows-Build: Windows/Winsock-Portabilitaet fuer die Netz-Terminals */
#include <ctype.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>


/* Musashi haelt seinen CPU-Zustand in eigenen globalen Variablen und ruft m68k_read/write_memory_*
   ohne Kontext-Zeiger auf (s. m68krt.h) — deshalb muessen der aktive RAM-Block bzw. das aktive
   Board hier ebenfalls global liegen, statt im q9_m68krt_t-Handle. Nur EIN q9_m68krt_init()
   gleichzeitig aktiv. Ist g_board gesetzt (q9_m68krt_attach_board, 5.3), laufen ALLE Zugriffe
   ueber den Board-Adress-Dispatch (RAM/ROM/Remap/UART/CF/Timer); sonst nackter RAM-Block (5.1). */
static uint8_t     *g_ram;
static uint32_t     g_ram_len;
static q9_board_t  *g_board;
static q9_quicc_t  *g_quicc;                          /* 5.11: QUICC-Ethernet, optional (attach) */

/* 2026-08-21: die komplette Netzwerk-Terminal-Server-Implementierung (channels[]/network_
   irq_resync/Telnet-Filterung/init_network_terminals/update_network_terminals/network_read8/
   write8/nettty_dev_* / q9_devtype_nettty) ist nach src/devices/nettty/nettty.c umgezogen
   (Hardware-Vereinheitlichung) -- s. dort, inkl. der Musashi-Entkopplung per Funktionszeiger-Hook
   (q9_nettty_set_irq_hook), die nettty von einem direkten m68k_set_irq()-Aufruf befreit. */

/* 5.18 (zweiter Teilschritt, 2026-08-14): direkt indizierte Tabelle fuer den festen I/O-Cluster
   $FFFF0000-$FFFFFFFF (64 KByte) -- ALLE heutigen Geraete ausser dem Framebuffer ($FD000000,
   ausserhalb dieses Bereichs) liegen hier. Statt bei jedem Zugriff linear ueber die Registry zu
   suchen (devreg_hit()s alter Weg, unten als Fallback erhalten), zeigt table[(addr-$FFFF0000)>>8]
   direkt auf das zustaendige Geraet -- ein Tabellenzugriff (256 Eintraege à 1 Zeiger = 2 KByte)
   statt einer Schleife ueber bis zu Q9_DEVREG_MAX Eintraege.

   MEHRERE GERAETE IM SELBEN 256-BYTE-SLOT (heute: MC6845 $FFFFA000 + CLUT $FFFFA010, nur 16 Byte
   auseinander, s. ARBEITSPLAN 5.18-Nachtrag 2026-08-13): fuer genau diesen Fall kann ein
   Tabelleneintrag nicht eindeutig sein. Absichtlich NICHT geraten oder eine Adresskarten-
   Migration erzwungen (das Index/Daten-Registermuster fuer sowas ist eine groessere, eigene
   Entscheidung, s. ARBEITSPLAN) -- solche Slots werden mit g_io_ambiguous markiert und fallen
   exakt auf den alten linearen Scan zurueck. Funktional bit-identisch zu vorher: schneller nur
   dort, wo ein Slot eindeutig ist, korrekt ueberall.
   KLEINERES FENSTER ALS DER SLOT (z.B. RTC 16 von 256 Byte, CF2 8 von 256 Byte): der Tabellen-
   Eintrag zeigt trotzdem auf das (einzige) Geraet dieses Slots; q9_device_hit() prueft danach
   weiterhin das ECHTE Fenster -- Adressen ausserhalb liefern wie bisher "kein Geraet" (Board-
   Fallback), nur ohne den unnoetigen Scan ueber alle anderen Geraete davor.
   NICHT REGISTRIERTE BEREICHE (z.B. Luecken zwischen Geraete-Fenstern innerhalb des Clusters):
   Tabelleneintrag NULL, Cluster-Abdeckung ist vollstaendig -> sofort "kein Geraet" statt jeder
   Schleife. 2026-08-21: das REMAP-Register ($FFFF8000-$FFFF8FFF, vormals das Beispiel hier) ist
   jetzt selbst ein registriertes devreg-Geraet (src/devices/remap/remap.c) und faellt deshalb
   nicht mehr in diesen Fall.

   Aufbau EINMALIG, lazy beim ersten Zugriff (nicht bei jedem q9_devreg_add -- die Registry fuellt
   sich erst ueber mehrere q9_m68krt_attach_*-Aufrufe in q9boardrun.c, ein fruehzeitiger Aufbau
   wuerde spaeter hinzukommende Geraete verpassen). g_io_table_built wird in q9_m68krt_init()
   zusammen mit q9_devreg_clear() zurueckgesetzt -- sonst bliebe bei einem zweiten Boot im selben
   Prozess (z.B. Tests) die Tabelle des VORHERIGEN Laufs stehen. */
#define Q9_IO_CLUSTER_BASE  0xFFFF0000u
#define Q9_IO_CLUSTER_SLOTS 256u                          /* 0x10000 Byte / 256 Byte je Slot        */
#define Q9_IO_SLOT_SHIFT    8u

static q9_device_t *g_io_table[Q9_IO_CLUSTER_SLOTS];
/* Sentinel fuer "mehrere Geraete teilen sich diesen Slot" -- kann nie ein echter Geraete-Zeiger
   sein (die kommen alle aus dem statischen Registry-Array in devreg.c, niemals aus Adresse 1). */
static q9_device_t *const g_io_ambiguous = (q9_device_t *)(uintptr_t)1;
static int g_io_table_built;

static void io_table_build(void)
{
    int i, n;

    memset(g_io_table, 0, sizeof(g_io_table));
    n = q9_devreg_count();
    for (i = 0; i < n; i++) {
        q9_device_t *d = q9_devreg_get(i);
        uint32_t lo, hi, slot, end;

        if (!d || d->size == 0) {
            continue;
        }
        lo = d->base;
        hi = d->base + d->size - 1u;                      /* size>0 geprueft, kein Unterlauf         */
        if (hi < lo) {
            continue;                                      /* Ueberlauf -- fehlerhaftes Fenster, ignorieren (sollte nie eintreten) */
        }
        if (hi < Q9_IO_CLUSTER_BASE) {
            continue;                                      /* komplett unterhalb des Clusters (z.B. Framebuffer) */
        }
        if (lo < Q9_IO_CLUSTER_BASE) {
            lo = Q9_IO_CLUSTER_BASE;                        /* defensiv: heutige Geraete ueberschneiden die Cluster-Grenze nie */
        }
        slot = (lo - Q9_IO_CLUSTER_BASE) >> Q9_IO_SLOT_SHIFT;
        end  = (hi - Q9_IO_CLUSTER_BASE) >> Q9_IO_SLOT_SHIFT;  /* hi <= 0xFFFFFFFF, passt immer in 0..255 */
        for (; slot <= end; slot++) {
            /* 2026-08-20: d->use_table==0 heisst "dieses Geraet bewusst NICHT eintragen" (s. devreg.h) --
               NICHT einfach ueberspringen (das liesse den Slot faelschlich NULL/"kein Geraet", falls
               kein anderes Geraet ihn beansprucht), sondern wie einen Kollisionsfall behandeln: der
               Sentinel erzwingt zuverlaessig den linearen Scan-Fallback unten in devreg_hit(), der das
               Geraet ganz normal ueber die Registry findet -- nur eben ohne den Tabellen-Vorteil. */
            if (!d->use_table) {
                g_io_table[slot] = g_io_ambiguous;
            } else if (g_io_table[slot] == NULL) {
                g_io_table[slot] = d;
            } else if (g_io_table[slot] != d) {
                g_io_table[slot] = g_io_ambiguous;
            }
        }
    }
    g_io_table_built = 1;
}

/* 5.17: Geraete-Registry -- Geraete, die bereits umgezogen sind (s. devreg.h/q9board.h), werden HIER
   vor dem alten Board-Fallback geprueft; noch nicht migrierte Geraete (Netz-Terminals, QUICC, sowie
   innerhalb von q9_board_read8/write8: CF/Timer/RTC) bleiben bis zu ihrem eigenen 5.17-Schritt in
   den bisherigen, direkt danebenstehenden Pruefungen bzw. im Board-Fallback. */
static q9_device_t *devreg_hit(uint32_t address)
{
    if (address >= Q9_IO_CLUSTER_BASE) {
        q9_device_t *fast;
        if (!g_io_table_built) {
            io_table_build();
        }
        fast = g_io_table[(address - Q9_IO_CLUSTER_BASE) >> Q9_IO_SLOT_SHIFT];
        if (fast != g_io_ambiguous) {
            /* fast==NULL: Tabelle deckt den GESAMTEN Cluster ab -- kein Eintrag heisst wirklich
               kein Geraet (kein Fall-through auf den Scan noetig). fast!=NULL: q9_device_hit()
               prueft weiterhin das echte, moeglicherweise kleinere Fenster (s.o. RTC/CF2). */
            return (fast && q9_device_hit(fast, address)) ? fast : NULL;
        }
        /* ambiguous: faellt bewusst durch auf den unveraenderten linearen Scan unten. */
    }
    {
        int i, n = q9_devreg_count();
        for (i = 0; i < n; i++) {
            q9_device_t *d = q9_devreg_get(i);
            if (q9_device_hit(d, address)) {
                return d;
            }
        }
        return NULL;
    }
}

/* 5.18: RAM-Fast-Path -- die weit ueberwiegende Mehrheit aller Zugriffe (praktisch jeder
   Opcode-Fetch, dazu der meiste Datenverkehr) geht an ganz normales RAM, nachdem OS-9 einmal
   den REMAP-Trigger ausgeloest hat. Bisher lief JEDER dieser Zugriffe trotzdem erst durch die
   komplette Geraete-Registry-Suche (devreg_hit, O(n) ueber alle registrierten Geraete) UND durch
   den Board-Fallback, bevor ueberhaupt RAM prueft wurde. Diese Funktion faengt den haeufigsten
   Fall VORHER mit einer einzigen billigen Pruefung ab.
   WICHTIG fuer Korrektheit (Boot-kritisch, s. q9board.c board_read_byte): nur anwenden, wenn
   g_board->remapped WIRKLICH gesetzt ist -- im Reset-Zustand (vor dem REMAP-Trigger) liegt bei
   denselben Adressen der ROM-Spiegel, nicht RAM (b->rom[addr % b->rom_len]). Ohne diese Pruefung
   wuerde der Fast-Path im Reset-Zustand falsches/uninitialisiertes RAM statt des Boot-ROMs
   liefern -- der Emulator wuerde nicht mehr booten. Kein Ueberschneidungsrisiko mit Geraeten:
   alle heutigen Geraetefenster liegen weit oberhalb von RAM (niedrigstes $FFFF1000, seit
   2026-08-14 256-Byte-Netz-Terminal-Slots, s. q9board.h -- bzw. Framebuffer $FD000000/ROM-Remap
   $FE000000, RAM nur 16 MByte ab 0, s. BOARD_RAM_BYTES). */
static inline int ram_fast_hit(uint32_t address, uint32_t span)
{
    /* address < ram_len zuerst geprueft, DANACH die Subtraktion ram_len - address (garantiert
       kein Unterlauf) statt address + span zu bilden -- eine direkte address+span-Addition
       koennte fuer address nahe UINT32_MAX ueberlaufen (address=0xFFFFFFFF, span=1 wird zu 0,
       0 < ram_len waere faelschlich wahr) und wuerde dann eine Adresse ausserhalb des RAM
       faelschlich als Treffer werten. */
    return g_board && g_board->remapped && address < g_board->ram_len &&
           span < (g_board->ram_len - address);
}

/* s. m68krt.h -- Instruktions-Ringpuffer mit Freeze-on-Anomaly. */
uint32_t q9_dbg_tr_pc[Q9_DBG_TR_SIZE];
uint32_t q9_dbg_tr_d0[Q9_DBG_TR_SIZE];
uint32_t q9_dbg_tr_a0[Q9_DBG_TR_SIZE];
uint32_t q9_dbg_tr_sp[Q9_DBG_TR_SIZE];

/* Stackbereich ZUM ZEITPUNKT jedes Dispatcher-Eintritts. Die Lage des
   Exception-Frames wird damit ABGELESEN statt angenommen -- zwei Versuche,
   sie zu erraten (sp+2 bzw. sp+62), lieferten beide Unsinn. */
uint32_t q9_dbg_ent_sp[Q9_DBG_ENT_MAX];
uint16_t q9_dbg_ent_stk[Q9_DBG_ENT_MAX][Q9_DBG_ENT_WORDS];
uint32_t q9_dbg_ent_n = 0u;
uint32_t q9_dbg_exi_sp[Q9_DBG_ENT_MAX];
uint16_t q9_dbg_exi_stk[Q9_DBG_ENT_MAX][Q9_DBG_ENT_WORDS];
uint32_t q9_dbg_exi_n = 0u;
uint32_t q9_dbg_tmr_total = 0u;
uint32_t q9_dbg_tmr_indisp = 0u;
uint32_t q9_dbg_tmr_pcs[8];
uint32_t q9_dbg_wake_enter = 0u;
uint32_t q9_dbg_wake_send  = 0u;
uint32_t q9_dbg_tr_head   = 0u;
uint32_t q9_dbg_tr_fill   = 0u;
int      q9_dbg_tr_frozen = 0;

static void q9_dbg_instr_hook(unsigned int pc)
{
    if (q9_dbg_tr_frozen) {
        return;
    }
    /* Zweiter Freeze-Ausloeser (2026-09-04): Sprung ins Leere. Unterhalb von
       $1000 liegt in diesem System ausschliesslich der Systemglobal-Bereich,
       dort steht niemals Code -- ein PC dort ist immer die Folge eines
       Sprungziels aus einer leeren Tabelle o. ae. Der Eintrag wird noch
       geschrieben, DANN eingefroren: so ist der Fehlsprung selbst die letzte
       Zeile der Spur und alles davor bleibt erhalten. */
    q9_dbg_tr_pc[q9_dbg_tr_head] = (uint32_t)pc;
    q9_dbg_tr_d0[q9_dbg_tr_head] = (uint32_t)m68k_get_reg(NULL, M68K_REG_D0);
    q9_dbg_tr_a0[q9_dbg_tr_head] = (uint32_t)m68k_get_reg(NULL, M68K_REG_A0);
    q9_dbg_tr_sp[q9_dbg_tr_head] = (uint32_t)m68k_get_reg(NULL, M68K_REG_SP);
    /* Beim Eintritt in Q9K_IRQDispatch den Exception-Frame gleich MITLESEN.
       Ihn erst im Ctrl-^-Dump zu lesen ist wertlos: der Dump kommt Sekunden
       spaeter, der Stackinhalt ist dann laengst ein anderer (real erlebt --
       alle Eintraege sahen identisch aus, weil derselbe aktuelle Speicher
       gelesen wurde). Der gemeldete SP ist der Stand NACH dem einleitenden
       "movem.l d0-d7/a0-a6,-(sp)" (60 Byte), der Frame liegt also bei +60:
       SR, dann PC, dann das Format-/Vektor-Wort. */
    if (pc == 0x795cu && q9_dbg_ent_n < Q9_DBG_ENT_MAX) {
        uint32_t fsp = (uint32_t)m68k_get_reg(NULL, M68K_REG_SP);
        uint32_t w;

        q9_dbg_ent_sp[q9_dbg_ent_n] = fsp;
        for (w = 0; w < Q9_DBG_ENT_WORDS; w++) {
            q9_dbg_ent_stk[q9_dbg_ent_n][w] = (uint16_t)m68k_read_memory_16(fsp + w * 2u);
        }
        q9_dbg_ent_n++;
    }
    /* Weckpfad der sc68681-ISR (Adressen fuer den aktuellen Build):
       $ca30 = "move.w $8(a2),d0" (Prozess-ID holen), $ca3e = F$Send-Trampolin. */
    if (pc == 0x0ca30u) { q9_dbg_wake_enter++; }
    if (pc == 0x0ca3eu) { q9_dbg_wake_send++; }
    if (pc == 0x75a2u) {                                    /* Q9K_TimerIRQHandler */
        uint32_t tsp  = (uint32_t)m68k_get_reg(NULL, M68K_REG_SP);
        uint32_t tfpc = m68k_read_memory_32(tsp + 2u);

        q9_dbg_tmr_total++;
        if (tfpc >= 0x795cu && tfpc <= 0x79deu) {           /* mitten im IRQ-Dispatcher */
            if (q9_dbg_tmr_indisp < 8u) {
                q9_dbg_tmr_pcs[q9_dbg_tmr_indisp] = tfpc;
            }
            q9_dbg_tmr_indisp++;
        }
    }
    if (pc == 0x79deu && q9_dbg_exi_n < Q9_DBG_ENT_MAX) {   /* unmittelbar vor dem RTE */
        uint32_t fsp = (uint32_t)m68k_get_reg(NULL, M68K_REG_SP);
        uint32_t w;

        q9_dbg_exi_sp[q9_dbg_exi_n] = fsp;
        for (w = 0; w < Q9_DBG_ENT_WORDS; w++) {
            q9_dbg_exi_stk[q9_dbg_exi_n][w] = (uint16_t)m68k_read_memory_16(fsp + w * 2u);
        }
        q9_dbg_exi_n++;
    }
    q9_dbg_tr_head = (q9_dbg_tr_head + 1u) % Q9_DBG_TR_SIZE;
    if (q9_dbg_tr_fill < Q9_DBG_TR_SIZE) {
        q9_dbg_tr_fill++;
    }
    if (pc < 0x1000u) {
        q9_dbg_tr_frozen = 1;
    }
}

void q9_dbg_instr_trace_init(void)
{
    const char *env = getenv("Q9_TRACE_INSTR");

    if (env && env[0] == '1') {
        m68k_set_instr_hook_callback(q9_dbg_instr_hook);
    }
}

void q9_dbg_instr_trace_note_tx(unsigned char val)
{
    if (val >= 0x80u) {                            /* nicht-ASCII = die gesuchte Anomalie */
        q9_dbg_tr_frozen = 1;
    }
}

unsigned int m68k_read_memory_8(unsigned int address)
{
    q9_device_t *dev;

    if (ram_fast_hit(address, 0)) {
        return g_board->ram[address];
    }
    if ((dev = devreg_hit(address)) != NULL) {
        return q9_device_read8(dev, (uint32_t)address);
    }
    if (g_board) {
        return q9_board_read8(g_board, (uint32_t)address);
    }
    return (address < g_ram_len) ? g_ram[address] : 0;
}

unsigned int m68k_read_memory_16(unsigned int address)
{
    q9_device_t *dev;

    if (ram_fast_hit(address, 1)) {
        return ((unsigned int)g_board->ram[address] << 8) | g_board->ram[address + 1];
    }
    if ((dev = devreg_hit(address)) != NULL) {
        return q9_device_read16(dev, (uint32_t)address);
    }
    if (g_board) {
        return q9_board_read16(g_board, (uint32_t)address);
    }
    if (address + 1 >= g_ram_len) {
        return 0;
    }
    return ((unsigned int)g_ram[address] << 8) | g_ram[address + 1];
}

unsigned int m68k_read_memory_32(unsigned int address)
{
    q9_device_t *dev;

    if (ram_fast_hit(address, 3)) {
        return ((unsigned int)g_board->ram[address]     << 24) |
               ((unsigned int)g_board->ram[address + 1] << 16) |
               ((unsigned int)g_board->ram[address + 2] <<  8) |
                (unsigned int)g_board->ram[address + 3];
    }
    if ((dev = devreg_hit(address)) != NULL) {
        return q9_device_read32(dev, (uint32_t)address);
    }
    if (g_board) {
        return q9_board_read32(g_board, (uint32_t)address);
    }
    if (address + 3 >= g_ram_len) {
        return 0;
    }
    return ((unsigned int)g_ram[address]     << 24) | ((unsigned int)g_ram[address + 1] << 16) |
           ((unsigned int)g_ram[address + 2] <<  8) |  (unsigned int)g_ram[address + 3];
}

/* Diagnose (2026-09-04): Schreib-Watch auf einen einzelnen Vektorslot.
   Vektor 27 zeigte 50 Byte zu tief in den IRQ-Dispatcher hinein, waehrend
   alle anderen korrekt auf dessen Einstieg zeigen -- gesucht ist, WER den
   Wert dorthin schreibt. */
uint32_t q9_dbg_wv_n = 0u;
uint32_t q9_dbg_wv_pc[8];
uint32_t q9_dbg_wv_val[8];
uint32_t q9_dbg_wv_size[8];
static uint32_t g_dbg_watch_addr = 0u;

static void q9_dbg_watch(unsigned int address, unsigned int value, unsigned int size)
{
    /* Beobachtete Adresse -- ueber die Umgebung setzbar, damit der Watch ohne
       Neuuebersetzung auf ein anderes Feld gelegt werden kann. */
    if (g_dbg_watch_addr == 0u) {
        const char *e = getenv("Q9_WATCH_ADDR");
        g_dbg_watch_addr = e ? (uint32_t)strtoul(e, 0, 0) : 0xFFFFFFFFu;
    }
    if (address <= g_dbg_watch_addr && address + size > g_dbg_watch_addr && q9_dbg_wv_n < 8u) {
        q9_dbg_wv_pc[q9_dbg_wv_n]   = (uint32_t)m68k_get_reg(NULL, M68K_REG_PPC);
        q9_dbg_wv_val[q9_dbg_wv_n]  = (uint32_t)value;
        q9_dbg_wv_size[q9_dbg_wv_n] = size;
        q9_dbg_wv_n++;
    }
}

void m68k_write_memory_8(unsigned int address, unsigned int value)
{
    q9_device_t *dev;

    if (ram_fast_hit(address, 0)) {
        g_board->ram[address] = (uint8_t)value;
        return;
    }
    if ((dev = devreg_hit(address)) != NULL) {
        q9_device_write8(dev, (uint32_t)address, (uint8_t)value);
        return;
    }
    if (g_board) {
        q9_board_write8(g_board, (uint32_t)address, (uint8_t)value);
        return;
    }
    if (address < g_ram_len) {
        g_ram[address] = (uint8_t)value;
    }
}

void m68k_write_memory_16(unsigned int address, unsigned int value)
{
    q9_device_t *dev;

    q9_dbg_watch(address, value, 2u);

    if (ram_fast_hit(address, 1)) {
        g_board->ram[address]     = (uint8_t)(value >> 8);
        g_board->ram[address + 1] = (uint8_t)value;
        return;
    }
    if ((dev = devreg_hit(address)) != NULL) {
        q9_device_write16(dev, (uint32_t)address, (uint16_t)value);
        return;
    }
    if (g_board) {
        q9_board_write16(g_board, (uint32_t)address, (uint16_t)value);
        return;
    }
    if (address + 1 >= g_ram_len) {
        return;
    }
    g_ram[address]     = (uint8_t)(value >> 8);
    g_ram[address + 1] = (uint8_t)value;
}

void m68k_write_memory_32(unsigned int address, unsigned int value)
{
    q9_device_t *dev;

    q9_dbg_watch(address, value, 4u);

    if (ram_fast_hit(address, 3)) {
        g_board->ram[address]     = (uint8_t)(value >> 24);
        g_board->ram[address + 1] = (uint8_t)(value >> 16);
        g_board->ram[address + 2] = (uint8_t)(value >> 8);
        g_board->ram[address + 3] = (uint8_t)value;
        return;
    }
    if ((dev = devreg_hit(address)) != NULL) {
        q9_device_write32(dev, (uint32_t)address, (uint32_t)value);
        return;
    }
    if (g_board) {
        q9_board_write32(g_board, (uint32_t)address, (uint32_t)value);
        return;
    }
    if (address + 3 >= g_ram_len) {
        return;
    }
    g_ram[address]     = (uint8_t)(value >> 24);
    g_ram[address + 1] = (uint8_t)(value >> 16);
    g_ram[address + 2] = (uint8_t)(value >>  8);
    g_ram[address + 3] = (uint8_t)value;
}

//────────────────────────────────────────────────────────────────────────────────────────────────
// Function: m68krt_board_int_ack
// Desc.:    5.3/5.4: Interrupt-Acknowledge im Board-Betrieb — die IRQ-Leitung wird beim Annehmen
//           des Interrupts wieder losgelassen (Puls-Verhalten, sonst wuerde der level-gehaltene
//           IRQ3 die CPU endlos erneut unterbrechen). Vektor-Auswahl wie beim echten Board:
//           Fordert die DUART gerade einen Interrupt an, legt sie ihren IVR-Inhalt auf den Bus
//           (vektorisierter 68681-IACK-Zyklus — der OS-9-Treiber sc68681 registriert seinen
//           Handler auf genau diesem Vektor, z.B. 0x50); sonst Autovector (Timer, Vektor 27 =
//           Autovektor Level 3).
//────────────────────────────────────────────────────────────────────────────────────────────────
static uint32_t g_ack_count;                          /* Diagnose: wie oft wurde IACK durchlaufen */
uint32_t q9_dbg_ackvec[256];                          /* Diagnose: Vektoren beim IACK, s. u. */
uint32_t q9_dbg_acklevel[8];                          /* Diagnose: Pegel beim IACK           */
static uint32_t g_quicc_ack_count;                    /* 5.15-Diagnose: davon QUICC (Level 5)     */

/* 5.15-Befund: echte Hardware haelt pro Geraet eine EIGENE IRQ-Leitung; quittiert die CPU
   das Level eines Geraets, senken NUR dessen eigene Leitung, alle anderen gleichzeitig
   anliegenden Anforderungen bleiben unberuehrt bestehen und der Prioritaets-Encoder praesentiert
   der CPU sofort wieder das naechsthoehere noch anstehende Level. Dieser Emulator bildet aber
   nur EINEN kombinierten `m68k_set_irq()`-Wert nach (kein Bus mit unabhaengigen Leitungen) -
   das blanke `m68k_set_irq(0)` unten wirft deshalb bislang ALLE gleichzeitig anstehenden
   Anforderungen weg, nicht nur die des gerade quittierten Geraets, und ueberlaesst die
   Wiederherstellung der naechsten Hauptschleifen-Runde (ganze BOARD_SLICE_CYCLES spaeter).
   Traf ein SCC1-TXB-Event (Level 5) wiederholt mit dem 100Hz-Timer (Level 6, wird zuletzt
   gesetzt und ueberschreibt daher bewusst 3/5) zusammen, ging die QUICC-Anforderung dadurch
   in einer Weise "verloren", die kein reines Hardware-Aequivalent hat - vermutlicher Ausloeser
   des TCP-Haengers bei sptcp/telnetd nach ~15 Segmenten (der geschlossene Microware-Treiber
   zaehlt vermutlich ausstehende TXB-Bestaetigungen und verliert bei einer verschmolzenen/
   verzoegerten Zustellung die Spur). Fix: nach dem Zuruecksetzen sofort das naechsthoechste
   NOCH anstehende Level neu anlegen, statt bis zur naechsten Runde zu warten - macht die
   Ack-Behandlung analog zum Prioritaets-Encoder echter Hardware selbstheilend. */
/* 5.17: liefert das erste registrierte "level-held" Geraet (s. devreg.h), das GERADE einen IRQ
   anfordert -- Timer/IRQ3-Trigger sind bewusst NICHT level-held (level_held=0) und bleiben daher
   wie vor 5.17 aus dieser Pruefung aussen vor (einmaliger Puls je Runde, s. q9boardrun.c). */
static q9_device_t *devreg_pending_level_held(int filter_level)
{
    int i, n = q9_devreg_count();
    for (i = 0; i < n; i++) {
        q9_device_t *d = q9_devreg_get(i);
        if (!d->level_held) {
            continue;
        }
        if (filter_level >= 0 && d->irq_level != filter_level) {
            continue;
        }
        if (q9_device_irq_pending(d)) {
            return d;
        }
    }
    return NULL;
}

static void m68krt_reassert_pending_irq(void)
{
    q9_device_t *dev;

    if ((dev = devreg_pending_level_held(-1)) != NULL) {
        m68k_set_irq((unsigned int)dev->irq_level);
        return;
    }
}

static int m68krt_board_int_ack(int int_level)
{
    int vector = M68K_INT_ACK_AUTOVECTOR;
    q9_device_t *dev;

    g_ack_count++;
    if (int_level == Q9_QUICC_IRQ_LEVEL) {
        g_quicc_ack_count++;                           /* 5.15-Diagnose: QUICC-ISR wurde zugestellt */
    }
    m68k_set_irq(0);
    if ((dev = devreg_pending_level_held(int_level)) != NULL) {
        int v = q9_device_irq_vector(dev);
        vector = (v >= 0) ? v : M68K_INT_ACK_AUTOVECTOR;
    }
    /* sonst: Autovektor (z.B. Level 6/Timer, das laut devreg-Eintrag level_held=0 hat und     */
    /* deshalb nie hier landet, s. q9board.c timer_dev_*-Kommentar).                             */

    m68krt_reassert_pending_irq();                     /* 5.15: sofort statt erst naechste Runde  */
    /* Diagnose (2026-09-04): welchen Vektor bekommt die CPU wirklich? Ein
       Geraet ohne gesetztes IVR liefert hier 0 -- die CPU vektorisiert dann
       ueber Slot 0 (Reset-SP), was nie gewollt ist. */
    if (vector >= 0 && vector < 256) {
        q9_dbg_ackvec[vector]++;
    }
    q9_dbg_acklevel[int_level & 7]++;
    return vector;
}


/* 5.15/Option B (2026-07-14): optionaler Trap#0-Trace fuer die telnetdc-Ev$Wait-Untersuchung
   (docs/re_telnetdc/OPTION_B_STATUS.md) -- nur aktiv, wenn die Env-Var Q9_TRAP_TRACE gesetzt ist,
   sonst No-Op (kein Effekt auf normale Laeufe/Performance).
   WICHTIG: trap #0 ist der Direktaufruf-Trap fuer ALLE F$/I$-Syscalls im GESAMTEN System (jeder
   Prozess, staendig). Testlauf 1 (2026-07-14) mit ungefiltertem, zeilengepuffertem Log bremste
   den Boot so massiv aus, dass er wie haengengeblieben wirkte -- Ursache war das Zeilenpuffer-
   Flushing (ein write()-Syscall pro Trap0), nicht das Tracing an sich. Testlauf 2 mit einem
   Register-Filter (D1==4, D2==0x7fff) ergab NULL Treffer -- die vermutete Registerbelegung war
   falsch. Deshalb jetzt: KEIN Filter mehr (alles loggen), aber gross gepuffert (_IOFBF, 4 MiB)
   statt zeilenweise geflusht -- vermeidet den Testlauf-1-Bremseffekt, ohne auf Verdacht zu
   filtern. Loggt PC (= Adresse der trap-Instruktion, ueber M68K_REG_PPC) + D0..D3/A0/A1;
   zusaetzlich ein Cap (Q9_TRAP_TRACE_CAP, Default 2 Mio. Zeilen) als Sicherheitsnetz gegen
   unbegrenztes Log-Wachstum bei einer sehr langen Session. */
static FILE   *g_trap_trace_fp  = NULL;
static long    g_trap_trace_cap = 2000000;
static long    g_trap_trace_n   = 0;
static int      g_trap_trace_all = 0;
static uint32_t g_watch_pc  = 0;
static uint32_t g_watch_pc2 = 0;
static int      g_telnetdc_base_known  = 0;
static int      g_watch_manual         = 0;   /* per Q9_WATCH_PC/2 vorgegeben -- Autoerkennung bleibt aus */
static uint32_t g_watch_candidate_base   = 0;
static int      g_watch_candidate_streak = 0;
static uint32_t g_event_return_pc = 0;    /* pkdvr-Diagnose (2026-07-15): naechste Instruktion nach
   einem geloggten F$Event-Trap -- einzelner globaler Slot genuegt, da der Emulator nur einen
   CPU-Kern hat und die Rueckkehr-Instruktion garantiert die naechste ausgefuehrte ist, bevor
   irgendein anderer Trap dazwischenkommen kann. */
static uint32_t g_syscall_return_pc = 0;  /* gezielter Rueckgabetrace fuer I$Open/I$Attach */
static uint16_t g_syscall_return_code = 0;
static uint32_t g_syscall_return_a0 = 0;

/* 2026-08-10 (Claude, Modul-Klassifizierung Kernel/IOMan/SysCache/SSM): live pruefen, welches
   Modul einen F$/I$-Aufruf tatsaechlich bearbeitet -- als Gegenprobe zu den dokumentierten
   Tabellen D-1..D-4 im Technical Reference Manual. Adressbereiche stammen aus einem echten
   "mdir -e" auf genau diesem Boot-Image (mdir zeigt die Basisadressen NACH Relokation, die je
   Boot/ROM identisch sind, solange sich die Modulreihenfolge im Bootvorgang nicht aendert):
     kernel   7100-e03c   ioman  e03c-f658
     syscache f7a6-f93c   ssm    f93c-100b0
   (init f658-f7a6 liegt dazwischen, ist aber keines der vier Faelle und wird als "other" gezaehlt.)
   Methode: bei JEDEM trap#0 (wenn Q9_TRAP_TRACE_ALL) wird ein Klassifizierungs-Fenster geoeffnet
   (Rueckkehr-PC = pc+4, wie auch sonst im Trace verwendet); der bereits vorhandene Instruction-
   Hook markiert dann jede besuchte Adresse per Bitmaske, bis die Rueckkehr-Adresse erreicht wird
   -- dann wird das Ergebnis als eine Zeile geloggt. Vereinfachung: verschachtelte Traps (ein
   Syscall-Handler ruft selbst wieder trap#0) wuerden das Fenster ueberschreiben -- fuer diese
   Untersuchung akzeptiert, da OS-9-Syscall-Handler laut Doku nicht rekursiv ueber trap#0 arbeiten. */
#define Q9_CLASSIFY_KERNEL_LO   0x00007100u
#define Q9_CLASSIFY_KERNEL_HI   0x0000e03cu
#define Q9_CLASSIFY_IOMAN_LO    0x0000e03cu
#define Q9_CLASSIFY_IOMAN_HI    0x0000f658u
#define Q9_CLASSIFY_SYSCACHE_LO 0x0000f7a6u
#define Q9_CLASSIFY_SYSCACHE_HI 0x0000f93cu
#define Q9_CLASSIFY_SSM_LO      0x0000f93cu
#define Q9_CLASSIFY_SSM_HI      0x000100b0u
#define Q9_CLASSIFY_BIT_KERNEL   0x01
#define Q9_CLASSIFY_BIT_IOMAN    0x02
#define Q9_CLASSIFY_BIT_SYSCACHE 0x04
#define Q9_CLASSIFY_BIT_SSM      0x08
#define Q9_CLASSIFY_BIT_OTHER    0x10
static int      g_classify_active     = 0;
static uint32_t g_classify_return_pc  = 0;
static uint32_t g_classify_call_pc    = 0;
static uint16_t g_classify_callcode   = 0;
static unsigned g_classify_seen_mask  = 0;

/* Testlauf 3 (2026-07-14) zeigte: D0/D1 allein liefern keine brauchbare Filterung -- Grund
   (Ghidra-Nachanalyse): der eigentliche OS-9-Aufrufcode (F$Event = 0x53) steckt NICHT in einem
   Register, sondern klassisch OS-9-typisch als INLINE-DATENWORT direkt hinter der trap#0-
   Instruktion im Code (Kernel liest es beim Rueckkehren und ueberspringt es). Deshalb jetzt
   gezielt genau dieses Wort mitlesen (m68k_read_disassembler_16, seiteneffektfrei) und nur bei
   Treffer F$Event (0x53) loggen -- das ist system-weit selten genug fuer ein sauberes Signal. */
/* Testlauf 7 (2026-07-15): die Ladeadresse von telnetdc variiert JE BOOT (nicht stabil, wie
   zunaechst per manuellem "l telnetdc"/.r7-Test angenommen) -- ein vorab von aussen uebergebenes
   Q9_WATCH_PC passt daher oft nicht mehr. Deshalb jetzt Autoerkennung: der ERSTE F$Sleep-Aufruf
   (callcode=0x0a) im telnetdc-Adressraum verraet die Basis (PC - 0xf4e, statischer Offset unseres
   eigenen Patches bei telnetdc-Offset 0xf4c/0xf4e), daraus werden die beiden GetStt-Watchpoints
   (Erfolg 0x32a8, Fehler 0x32b2) automatisch abgeleitet -- kein manuelles Ausrechnen/Uebergeben
   mehr noetig, funktioniert bootuebergreifend automatisch. */

/* pkdvr-Diagnose (2026-07-15): Modulname am Zeiger `addr` OS-9-typisch lesen (letztes Zeichen
   hat Bit 7 gesetzt, kein NUL-Terminator im Speicher) -- fuer F$Link/F$Load-Namen (A0). */
static void m68krt_read_os9_name(uint32_t addr, char *out, int outsz)
{
    int i = 0;
    while (i < outsz - 1) {
        uint8_t b = (uint8_t)m68k_read_memory_8(addr + i);
        char c = (char)(b & 0x7F);
        if (c == 0) {
            break;
        }
        out[i++] = c;
        if (b & 0x80) {
            break;
        }
    }
    out[i] = '\0';
}

static int m68krt_trap_trace_callback(int trap)
{
    if (trap == 0 && g_trap_trace_fp && g_trap_trace_n < g_trap_trace_cap) {
        uint32_t pc = m68k_get_reg(NULL, M68K_REG_PPC);
        uint32_t callcode = m68k_read_memory_16(pc + 2);
        /* pkdvr-Diagnose (2026-07-15): F$Link(0x00)/F$Load(0x01) zusaetzlich zu F$Event mit-
           verfolgen -- Ziel: klaeren, ob "pkdvr" mehrfach separat gelinkt/geladen wird (Verdacht
           nach der Basis-Analyse: mehrere gleichzeitig aktive Kopien im Speicher, s.
           docs/re_telnetdc/OPTION_B_STATUS.md). A0 = Modulname bei beiden Aufrufen. */
        if (callcode == 0x00 || callcode == 0x01) {
            char name[16];
            m68krt_read_os9_name(m68k_get_reg(NULL, M68K_REG_A0), name, sizeof(name));
            /* 2026-08-10 (Claude, dynamic-load-Untersuchung Kernel/IOMan): Namensfilter fuer
               "modulecall" aufgeweitet -- vorher nur smb/soc/spf-Praefix (pkdvr-Diagnose-Kontext),
               jetzt JEDER Modulname, sobald Q9_TRAP_TRACE_ALL gesetzt ist. Alte pk*-"linkname"-
               Zeile (inkl. g_event_return_pc-Seiteneffekt fuer die pkdvr-Untersuchung) bleibt
               unveraendert erhalten. Zusaetzlich fflush() an beiden Stellen, weil der 4-MiB-
               Puffer sonst nie voll wird und die Datei bis zum (hier nicht vorhandenen) sauberen
               Prozessende leer bleibt. */
            if (g_trap_trace_all) {
                fprintf(g_trap_trace_fp, "modulecall pc=%08x callcode=%04x name=%s a0=%08x\n",
                        pc, callcode, name, m68k_get_reg(NULL, M68K_REG_A0));
                fflush(g_trap_trace_fp);
                g_trap_trace_n++;
            }
            if (name[0] == 'p' && name[1] == 'k') {
                fprintf(g_trap_trace_fp, "linkname pc=%08x callcode=%04x name=%s\n", pc, callcode, name);
                fflush(g_trap_trace_fp);
                g_trap_trace_n++;
                g_event_return_pc = pc + 4;    /* Rueckgabe (D0/D1/A0-A2) im Watch-Callback mitloggen */
            }
            return 0;
        }
        if (g_trap_trace_all || callcode == 0x53 || callcode == 0x0a || callcode == 0x8d) {
            if (g_trap_trace_all) {
                /* Klassifizierungs-Fenster fuer DIESEN Aufruf oeffnen -- Auswertung/Log erfolgt
                   im Instruction-Hook, sobald die Rueckkehradresse pc+4 erreicht wird. */
                g_classify_active    = 1;
                g_classify_return_pc = pc + 4;
                g_classify_call_pc   = pc;
                g_classify_callcode  = (uint16_t)callcode;
                g_classify_seen_mask = 0;
            }
            if (g_trap_trace_all &&
                (callcode == 0x80 || callcode == 0x83 || callcode == 0x84 ||
                 callcode == 0x86 || callcode == 0x87)) {
                char path[96];
                m68krt_read_os9_name(m68k_get_reg(NULL, M68K_REG_A0), path, sizeof(path));
                fprintf(g_trap_trace_fp, "path callcode=%04x a0=%08x path=%s\n",
                        callcode, m68k_get_reg(NULL, M68K_REG_A0), path);
                if (callcode == 0x80 || callcode == 0x84) {
                    g_syscall_return_pc = pc + 4;
                    g_syscall_return_code = (uint16_t)callcode;
                    g_syscall_return_a0 = m68k_get_reg(NULL, M68K_REG_A0);
                }
            }
            fprintf(g_trap_trace_fp,
                    "trap0 pc=%08x callcode=%04x d0=%08x d1=%08x d2=%08x d3=%08x "
                    "a0=%08x a1=%08x\n",
                    pc, callcode,
                    m68k_get_reg(NULL, M68K_REG_D0), m68k_get_reg(NULL, M68K_REG_D1),
                    m68k_get_reg(NULL, M68K_REG_D2), m68k_get_reg(NULL, M68K_REG_D3),
                    m68k_get_reg(NULL, M68K_REG_A0), m68k_get_reg(NULL, M68K_REG_A1));
            g_trap_trace_n++;
            if (callcode == 0x53) {
                /* pkdvr-Diagnose (2026-07-15): Rueckgabewert (D0=Fehlercode, D1=Ergebnis/
                   gelesener Event-Wert) mitschneiden -- noetig, um z.B. Ev$Read-Werte oder
                   ob ein Ev$Wait ueberhaupt zurueckkehrt, im Log zu sehen. trap#0 + Inline-
                   Wort sind zusammen 4 Byte lang, die Rueckkehradresse ist also PC+4. */
                g_event_return_pc = pc + 4;
            }
            /* Testlauf 10 zeigte: ein simples "erste Instanz gewinnt" verriegelt sich auf eine
               fruehe, irrelevante Instanz (z.B. vom Login-Prozess), waehrend die eigentlich
               relevante (dominante, dauerhaft aktive) Instanz nie erfasst wird. Deshalb jetzt
               "3x hintereinander dieselbe Basis" als Kriterium -- fegt kurzlebige Strays weg,
               folgt aber zuverlaessig der aktuell aktiven Schleife (die F$Sleep sehr oft in
               Folge aufruft), auch wenn diese erst spaeter im Boot/Verbindungsverlauf startet. */
            if (callcode == 0x0a && !g_watch_manual) {
                uint32_t base = pc - 0xf4e;
                if (base == g_watch_candidate_base) {
                    g_watch_candidate_streak++;
                } else {
                    g_watch_candidate_base   = base;
                    g_watch_candidate_streak = 1;
                }
                if (g_watch_candidate_streak == 3 && g_watch_pc != base + 0x32a8) {
                    g_watch_pc  = base + 0x32a8;
                    g_watch_pc2 = base + 0x32b2;
                    g_telnetdc_base_known = 1;
                    fprintf(g_trap_trace_fp,
                            "auto-base telnetdc=%08x watch1=%08x watch2=%08x\n",
                            base, g_watch_pc, g_watch_pc2);
                }
            }
        }
    }
    return 0;                                         /* nicht behandelt -- normale Exception laeuft weiter */
}

/* Testlauf 4 (2026-07-15): der Trap#0-Callback feuert nur VOR dem Trap (Eingaberegister), nicht
   beim Rueckkehren -- der Rueckgabewert von I$GetStt/SS_Ready (in D1, s. Disassemblierung des
   Wrapers bei telnetdc-Offset 0x32a8) blieb dadurch unsichtbar. Deshalb zusaetzlich ein gezielter
   PC-Watchpoint ueber M68K_INSTRUCTION_HOOK: feuert vor JEDER Instruktion (teuer, aber der
   Vergleich selbst ist trivial und es wird nur bei echtem Treffer geloggt/geflusht -- anders als
   der frueher verworfene ungefilterte Trap-Log-Versuch, der pro Zeile schrieb). Adresse kommt aus
   Q9_WATCH_PC (Hex, ohne 0x-Praefix), da die Ladeadresse je Boot variiert. */

static void m68krt_watch_pc_callback(unsigned int pc)
{
    if (g_classify_active) {
        if (pc == g_classify_return_pc) {
            if (g_trap_trace_fp && g_trap_trace_n < g_trap_trace_cap) {
                fprintf(g_trap_trace_fp,
                        "classify pc=%08x callcode=%04x kernel=%d ioman=%d syscache=%d ssm=%d other=%d\n",
                        g_classify_call_pc, g_classify_callcode,
                        (g_classify_seen_mask & Q9_CLASSIFY_BIT_KERNEL)   ? 1 : 0,
                        (g_classify_seen_mask & Q9_CLASSIFY_BIT_IOMAN)    ? 1 : 0,
                        (g_classify_seen_mask & Q9_CLASSIFY_BIT_SYSCACHE) ? 1 : 0,
                        (g_classify_seen_mask & Q9_CLASSIFY_BIT_SSM)     ? 1 : 0,
                        (g_classify_seen_mask & Q9_CLASSIFY_BIT_OTHER)   ? 1 : 0);
                g_trap_trace_n++;
            }
            g_classify_active = 0;
        } else {
            if (pc >= Q9_CLASSIFY_KERNEL_LO && pc < Q9_CLASSIFY_KERNEL_HI) {
                g_classify_seen_mask |= Q9_CLASSIFY_BIT_KERNEL;
            } else if (pc >= Q9_CLASSIFY_IOMAN_LO && pc < Q9_CLASSIFY_IOMAN_HI) {
                g_classify_seen_mask |= Q9_CLASSIFY_BIT_IOMAN;
            } else if (pc >= Q9_CLASSIFY_SYSCACHE_LO && pc < Q9_CLASSIFY_SYSCACHE_HI) {
                g_classify_seen_mask |= Q9_CLASSIFY_BIT_SYSCACHE;
            } else if (pc >= Q9_CLASSIFY_SSM_LO && pc < Q9_CLASSIFY_SSM_HI) {
                g_classify_seen_mask |= Q9_CLASSIFY_BIT_SSM;
            } else {
                g_classify_seen_mask |= Q9_CLASSIFY_BIT_OTHER;
            }
        }
    }
    if (g_syscall_return_pc && pc == g_syscall_return_pc && g_trap_trace_fp &&
        g_trap_trace_n < g_trap_trace_cap) {
        fprintf(g_trap_trace_fp,
                "syscallret pc=%08x callcode=%04x a0=%08x d0=%08x d1=%08x a2=%08x\n",
                pc, g_syscall_return_code, g_syscall_return_a0,
                m68k_get_reg(NULL, M68K_REG_D0), m68k_get_reg(NULL, M68K_REG_D1),
                m68k_get_reg(NULL, M68K_REG_A2));
        g_trap_trace_n++;
        g_syscall_return_pc = 0;
    }
    if (g_trap_trace_fp && g_trap_trace_n < g_trap_trace_cap &&
        ((g_watch_pc && pc == g_watch_pc) || (g_watch_pc2 && pc == g_watch_pc2))) {
        fprintf(g_trap_trace_fp,
                "watch pc=%08x d0=%08x d1=%08x d2=%08x d3=%08x a0=%08x a1=%08x\n",
                pc,
                m68k_get_reg(NULL, M68K_REG_D0), m68k_get_reg(NULL, M68K_REG_D1),
                m68k_get_reg(NULL, M68K_REG_D2), m68k_get_reg(NULL, M68K_REG_D3),
                m68k_get_reg(NULL, M68K_REG_A0), m68k_get_reg(NULL, M68K_REG_A1));
        g_trap_trace_n++;
    }
    /* pkdvr-Diagnose (2026-07-15): Rueckkehrpunkt eines zuvor geloggten F$Event-Traps erreicht --
       D0/D1 jetzt sind der Rueckgabewert (D0=0 Erfolg/sonst Fehlercode, D1=Ergebnis bei
       Ev$Read/Ev$Wait-Erfolg). Sofort konsumieren (auf 0 setzen), damit kein spaeterer,
       zufaelliger Treffer derselben Adresse (z.B. Schleifenrunde) faelschlich mitgeloggt wird. */
    if (g_event_return_pc && pc == g_event_return_pc && g_trap_trace_fp &&
        g_trap_trace_n < g_trap_trace_cap) {
        /* A0-A2 mitgeloggt fuer die F$Link/F$Load-Diagnose (2026-07-15): bei Erfolg liefert
           F$Link/F$Load die Modulbasis typischerweise in A2 -- damit laesst sich jeder Link-
           Aufruf direkt der resultierenden Ladeadresse zuordnen. */
        fprintf(g_trap_trace_fp,
                "eventret pc=%08x d0=%08x d1=%08x a0=%08x a1=%08x a2=%08x\n",
                pc, m68k_get_reg(NULL, M68K_REG_D0), m68k_get_reg(NULL, M68K_REG_D1),
                m68k_get_reg(NULL, M68K_REG_A0), m68k_get_reg(NULL, M68K_REG_A1),
                m68k_get_reg(NULL, M68K_REG_A2));
        g_trap_trace_n++;
        g_event_return_pc = 0;
    }
}

int q9_m68krt_init(q9_m68krt_t *rt, uint8_t *ram, uint32_t ram_len, q9_cpu_type_t cpu)
{
    unsigned musashi_type;

    if (!ram || ram_len < 8) {
        return Q9_M68KRT_ERR_RAM;
    }

    memset(rt, 0, sizeof(*rt));
    rt->ram     = ram;
    rt->ram_len = ram_len;
    g_ram       = ram;
    g_ram_len   = ram_len;
    g_board     = 0;                                  /* RAM-Modus, bis attach_board (5.3) folgt */
    q9_devreg_clear();                                /* 5.17: frische Geraete-Registry je Boot  */
    g_io_table_built = 0;                             /* 5.18: I/O-Tabelle muss neu aufgebaut werden,
                                                          sonst blieben Zeiger eines VORHERIGEN Laufs
                                                          stehen (z.B. bei mehreren Boots im selben
                                                          Prozess/Test) */

    /* Q9FLUX_EDITOR_de.md 4.1: cpu ist jetzt ein echter Aufrufparameter statt einer versteckten
       Env-Var-Abfrage. Q9_CPU_68030 bleibt der Default (echte Q9-Hardware, Entscheidung E12). */
    switch (cpu) {
        case Q9_CPU_68000:   musashi_type = M68K_CPU_TYPE_68000;   break;
        case Q9_CPU_68010:   musashi_type = M68K_CPU_TYPE_68010;   break;
        case Q9_CPU_68EC020: musashi_type = M68K_CPU_TYPE_68EC020; break;
        case Q9_CPU_68020:   musashi_type = M68K_CPU_TYPE_68020;   break;
        case Q9_CPU_68EC030: musashi_type = M68K_CPU_TYPE_68EC030; break;
        case Q9_CPU_68EC040: musashi_type = M68K_CPU_TYPE_68EC040; break;
        case Q9_CPU_68LC040: musashi_type = M68K_CPU_TYPE_68LC040; break;
        case Q9_CPU_68040:   musashi_type = M68K_CPU_TYPE_68040;   break;
        case Q9_CPU_68030:
        default:
            musashi_type = M68K_CPU_TYPE_68030;
            break;
    }
    /* Diagnose-Override bleibt erhalten (5.x-Vergleichslaeufe, deaktiviert Musashis PMMU, um
       einen OS-9/ftpdc-Fehler von einem PMMU-Emulationsfehler zu trennen) -- greift aber NUR noch,
       wenn der Aufrufer den Default uebergeben hat, damit eine explizite cpu-Wahl (z.B. aus der
       .q9-Config) nicht still von einer alten Diagnose-Env-Var ueberschrieben wird. */
    if (cpu == Q9_CPU_68030 && getenv("Q9_CPU") && strcmp(getenv("Q9_CPU"), "ec030") == 0) {
        musashi_type = M68K_CPU_TYPE_68EC030;
    }
    m68k_set_cpu_type(musashi_type);
    m68k_init();
    q9_dbg_instr_trace_init();                          /* Diagnose, s. m68krt.h */
    m68k_set_int_ack_callback(0);

    {
        const char *trace_path = getenv("Q9_TRAP_TRACE");
        if (trace_path && !g_trap_trace_fp) {
            g_trap_trace_all = getenv("Q9_TRAP_TRACE_ALL") != NULL;
            g_trap_trace_fp = fopen(trace_path, "w");
            if (g_trap_trace_fp) {
                setvbuf(g_trap_trace_fp, NULL, _IOFBF, 4 * 1024 * 1024);
                m68k_set_trap_instr_callback(m68krt_trap_trace_callback);
            }
        }
        const char *watch_pc_str  = getenv("Q9_WATCH_PC");
        const char *watch_pc2_str = getenv("Q9_WATCH_PC2");
        if (watch_pc_str || watch_pc2_str) {
            if (watch_pc_str)  g_watch_pc  = (uint32_t)strtoul(watch_pc_str, NULL, 16);
            if (watch_pc2_str) g_watch_pc2 = (uint32_t)strtoul(watch_pc2_str, NULL, 16);
            g_watch_manual = 1;                     /* manuell vorgegeben -- Autoerkennung bleibt komplett aus */
        }
        if (g_trap_trace_fp) {
            m68k_set_instr_hook_callback(m68krt_watch_pc_callback);   /* auch ohne Vorgabe: Autoerkennung */
        }
    }

    // === NEU: Netzwerk-Server beim Start hochfahren ===
    q9_sock_startup();          /* Windows-Build: WSAStartup unter Windows, no-op auf POSIX */
    q9_nettty_set_irq_hook(q9_m68krt_set_irq);   /* 2026-08-21: Musashi-Entkopplung, s. nettty.c */
    q9_nettty_init();

    return Q9_M68KRT_OK;
}



void q9_m68krt_attach_board(q9_board_t *board)
{
    g_board = board;
    m68k_set_int_ack_callback(board ? m68krt_board_int_ack : 0);

    /* 5.17: Geraete in die Registry eintragen -- DUART/CF/Timer/RTC (board-intern) + Netz-
       Terminals (unabhaengig vom Board, aber nur beim echten Boot gebraucht, s.u.). Nur QUICC
       bleibt noch hartkodiert (eigener 5.17-Schritt). WICHTIG: die Netz-Terminals (Level 4)
       werden bewusst NACH der DUART (Level 3) registriert -- q9boardrun.c's erster Poll-Durchgang
       (Level < 5, filtert Timer/Level 6 per Levelvergleich weg, s. dort) durchlaeuft die Registry
       in dieser Reihenfolge, und q9_m68krt_set_irq() bildet nur EINE kombinierte Leitung nach
       (letzter Aufruf gewinnt) -- die Level-3-vor-Level-4-Reihenfolge muss darum erhalten bleiben. */
    if (board) {
        q9_device_t d;
        memset(&d, 0, sizeof(d));
        d.type       = "duart68681";
        d.name       = "uart0";
        d.base       = Q9_BOARD_UART_BASE;
        d.size       = Q9_BOARD_UART_TOP - Q9_BOARD_UART_BASE + 1u;
        d.irq_level  = 3;
        d.irq_vector = -1;                            /* dynamisch, s. irq_vector_fn            */
        d.level_held = 1;
        d.use_table  = 1;                             /* liegt im Fast-Table-Cluster, s. devreg.h */
        d.vt         = &q9_devtype_duart68681;
        d.state      = board;
        q9_devreg_add(d);

        /* 5.17: Compact-Flash, zweites umgezogenes Geraet -- kein IRQ (level_held bleibt 0,
           irq_vector -1/unbenutzt: q9_devtype_cf setzt keinen irq_pending). 5.19a: state zeigt
           auf das q9_cf_t-Interface im Board (nicht mehr aufs Board selbst) -- dieselbe Vtable
           bedient auch das RC2014-Zweitinterface, das q9boardrun.c bei Bedarf registriert. */
        memset(&d, 0, sizeof(d));
        d.type       = "cf";
        d.name       = "cf0";
        d.base       = Q9_BOARD_CF_BASE;
        d.size       = Q9_BOARD_CF_TOP - Q9_BOARD_CF_BASE + 1u;
        d.irq_level  = 0;
        d.irq_vector = -1;
        d.level_held = 0;
        d.use_table  = 1;                             /* liegt im Fast-Table-Cluster, s. devreg.h */
        d.vt         = &q9_devtype_cf;
        d.state      = &board->cf;
        q9_devreg_add(d);

        /* 5.17: RTC72421, viertes board-internes Geraet -- kein IRQ. (Timer/IRQ3 wird bewusst
           NICHT hier, sondern erst in q9_m68krt_attach_quicc registriert -- s. dort, Grund ist
           Registrierungsreihenfolge/IRQ-Prioritaet, Level 6 muss NACH QUICC/Level 5 kommen.) */
        memset(&d, 0, sizeof(d));
        d.type       = "rtc72421";
        d.name       = "rtc0";
        d.base       = Q9_BOARD_RTC_BASE;
        d.size       = Q9_BOARD_RTC_TOP - Q9_BOARD_RTC_BASE + 1u;
        d.irq_level  = 0;
        d.irq_vector = -1;
        d.level_held = 0;
        d.use_table  = 1;                             /* liegt im Fast-Table-Cluster, s. devreg.h */
        d.vt         = &q9_devtype_rtc72421;
        d.state      = board;
        q9_devreg_add(d);

        /* 2026-08-21 (Hardware-Vereinheitlichung, Andreas' Idee): der REMAP-Trigger selbst -- kein
           IRQ, Registrierungsreihenfolge daher egal (kein IRQ-Prioritaetskonflikt moeglich). state
           zeigt wie bei duart68681/rtc72421 auf das ganze q9_board_t. */
        memset(&d, 0, sizeof(d));
        d.type       = "remap";
        d.name       = "remap0";
        d.base       = Q9_BOARD_REMAP_REG_BASE;
        d.size       = Q9_BOARD_REMAP_REG_TOP - Q9_BOARD_REMAP_REG_BASE + 1u;
        d.irq_level  = 0;
        d.irq_vector = -1;
        d.level_held = 0;
        d.use_table  = 1;                             /* liegt im Fast-Table-Cluster, s. devreg.h */
        d.vt         = &q9_devtype_remap;
        d.state      = board;
        q9_devreg_add(d);

        /* 5.17: Netz-Terminals, fuenftes umgezogenes Geraet. 2026-08-21 (Hardware-Vereinheitlichung,
           Andreas' Vorgabe "jedes Geraet einzeln, mit eigenem Descriptor, eigener Adresse, im
           Array"): ACHT separate devreg-Eintraege statt einem gemeinsamen -- q9_nettty_attach()
           (nettty.c) registriert sie alle, je einer pro Kanal mit eigenem dev->state (der eigene
           "Descriptor") und eigener, bereits seit 2026-08-14 vorhandener Adresse. Aufgerufen NACH
           DUART/CF/RTC (Level 3), damit die Registrierungsreihenfolge weiterhin aufsteigend nach
           Level bleibt (3 vor 4) -- wichtig fuer die "letzter Aufruf gewinnt"-Semantik von
           q9_m68krt_set_irq(), s. Kommentar dort. */
        q9_nettty_attach();
    }
}

void q9_m68krt_attach_quicc(q9_quicc_t *quicc)
{
    g_quicc = quicc;                                  /* 5.11: ab jetzt dekodiert das QUICC-     */
                                                       /* Fenster $FFFF2000-$FFFF3FFF             */

    /* 5.17: QUICC, sechstes und letztes umgezogenes Geraet -- fester Vektor 254 (kein
       irq_vector_fn noetig, anders als DUART/nettty). Registriert NACH den Netz-Terminals
       (Level 4 vor Level 5) -- s. q9_devtype_quicc-Kommentar in quicc.c und die Reihenfolge-
       Erklaerung in q9_m68krt_attach_board.
       Timer/IRQ3 (Level 6, viertes 5.17-Geraet inhaltlich, aber ERST HIER registriert statt in
       attach_board) folgt bewusst GANZ ZULETZT: Level 6 muss in der Registrierungsreihenfolge
       NACH QUICC/Level 5 stehen, damit q9boardrun.c's Poll-Schleife (Level < 5 vor QUICC, Level
       >= 5 danach) und m68krt_board_int_ack/reassert_pending_irq (durchlaufen die GESAMTE
       Registry in Registrierungsreihenfolge) am Ende bei gleichzeitig anstehenden Interrupts
       konsistent das hoechste Level uebrig lassen -- exakt das Verhalten, das vor 5.17 durch die
       hartkodierte Aufrufreihenfolge (DUART 3, QUICC 5, Timer 6) in q9boardrun.c sichergestellt war. */
    if (quicc && g_board) {
        q9_device_t d;
        memset(&d, 0, sizeof(d));
        d.type       = "quicc";
        d.name       = "enet0";
        d.base       = Q9_QUICC_BASE;
        d.size       = Q9_QUICC_TOP - Q9_QUICC_BASE + 1u;
        d.irq_level  = Q9_QUICC_IRQ_LEVEL;
        d.irq_vector = Q9_QUICC_IRQ_VECTOR;            /* fest, s. q9_devtype_quicc-Kommentar    */
        d.level_held = 1;
        d.use_table  = 1;                             /* liegt im Fast-Table-Cluster, s. devreg.h */
        d.vt         = &q9_devtype_quicc;
        d.state      = quicc;
        q9_devreg_add(d);

        memset(&d, 0, sizeof(d));
        d.type       = "timer_irq";
        d.name       = "tirq0";
        d.base       = Q9_BOARD_TIRQ_OFF_BASE;
        d.size       = Q9_BOARD_TIRQ_ON_TOP - Q9_BOARD_TIRQ_OFF_BASE + 1u;
        d.irq_level  = 6;
        d.irq_vector = -1;
        d.level_held = 0;
        d.use_table  = 1;                             /* liegt im Fast-Table-Cluster, s. devreg.h */
        d.vt         = &q9_devtype_timer_irq;
        d.state      = g_board;
        q9_devreg_add(d);
    }
}

void q9_m68krt_attach_cf2(q9_cf_t *cf2)
{
    /* 5.19a: RC2014-SC145-Zweitinterface — kein IRQ (level_held 0), Reihenfolge damit egal
       (q9_devtype_cf setzt nie irq_pending, s. q9board.c). Eigene Basisadresse: cf_dev_* rechnen
       den ATA-Registeroffset ueber dev->base aus, dieselbe Vtable bedient beide Interfaces. */
    if (cf2) {
        q9_m68krt_attach_cf_at(cf2, Q9_BOARD_CF2_BASE, "cf1");
    }
}

void q9_m68krt_attach_cf_at(q9_cf_t *cf, uint32_t base, const char *name)
{
    if (cf) {
        q9_device_t d;
        memset(&d, 0, sizeof(d));
        d.type       = "cf";
        d.name       = name ? name : "cf";
        d.base       = base;
        d.size       = Q9_BOARD_CF2_TOP - Q9_BOARD_CF2_BASE + 1u;
        d.irq_level  = 0;
        d.irq_vector = -1;
        d.level_held = 0;
        d.use_table  = 1;                             /* liegt im Fast-Table-Cluster, s. devreg.h */
        d.vt         = &q9_devtype_cf;
        d.state      = cf;
        q9_devreg_add(d);
    }
}

//────────────────────────────────────────────────────────────────────────────────────────────────
// Function: q9_m68krt_attach_mc6845
// Desc.:    5.24: MC6845-CRT-Controller in die Registry eintragen -- kein IRQ (level_held bleibt 0,
//           s. mc6845.h). Reihenfolge egal (kein IRQ-Prioritaetskonflikt moeglich).
//────────────────────────────────────────────────────────────────────────────────────────────────
void q9_m68krt_attach_mc6845(q9_mc6845_t *crtc)
{
    if (crtc) {
        q9_device_t d;
        memset(&d, 0, sizeof(d));
        d.type       = "mc6845";
        d.name       = "crtc0";
        d.base       = Q9_MC6845_BASE;
        d.size       = Q9_MC6845_TOP - Q9_MC6845_BASE + 1u;
        d.irq_level  = 0;
        d.irq_vector = -1;
        d.level_held = 0;
        d.use_table  = 1;                             /* liegt im Fast-Table-Cluster, s. devreg.h */
        d.vt         = &q9_devtype_mc6845;
        d.state      = crtc;
        q9_devreg_add(d);
    }
}

//────────────────────────────────────────────────────────────────────────────────────────────────
// Function: q9_m68krt_attach_framebuf
// Desc.:    5.26: VRAM-Geraet in die Registry eintragen -- kein IRQ. Fenstergroesse kommt aus
//           fb->size (q9_framebuf_size), also erst NACH q9_framebuf_init() aufrufbar.
//────────────────────────────────────────────────────────────────────────────────────────────────
void q9_m68krt_attach_framebuf(q9_framebuf_t *fb)
{
    if (fb) {
        q9_device_t d;
        memset(&d, 0, sizeof(d));
        d.type       = "framebuf";
        d.name       = "vram0";
        d.base       = Q9_FRAMEBUF_BASE;
        d.size       = q9_framebuf_size(fb);
        d.irq_level  = 0;
        d.irq_vector = -1;
        d.level_held = 0;
        d.use_table  = 0;                             /* liegt UNTERHALB des Fast-Table-Clusters   */
                                                        /* ($FD000000 < $FFFF0000) -- Tabelle greift */
                                                        /* hier ohnehin nie, s. devreg.h/m68krt.c    */
        d.vt         = &q9_devtype_framebuf;
        d.state      = fb;
        q9_devreg_add(d);
    }
}

//────────────────────────────────────────────────────────────────────────────────────────────────
// Function: q9_m68krt_attach_clut
// Desc.:    5.29-Nachtrag: CLUT-Geraet in die Registry eintragen -- kein IRQ.
//────────────────────────────────────────────────────────────────────────────────────────────────
void q9_m68krt_attach_clut(q9_clut_t *clut)
{
    if (clut) {
        q9_device_t d;
        memset(&d, 0, sizeof(d));
        d.type       = "clut";
        d.name       = "clut0";
        d.base       = Q9_CLUT_BASE;
        d.size       = Q9_CLUT_TOP - Q9_CLUT_BASE + 1u;
        d.irq_level  = 0;
        d.irq_vector = -1;
        d.level_held = 0;
        d.use_table  = 1;                             /* liegt im Fast-Table-Cluster, s. devreg.h */
        d.vt         = &q9_devtype_clut;
        d.state      = clut;
        q9_devreg_add(d);
    }
}

void q9_m68krt_reset(q9_m68krt_t *rt)
{
    (void)rt;
    m68k_pulse_reset();
}

int q9_m68krt_execute(q9_m68krt_t *rt, int cycles)
{
    (void)rt;
    q9_nettty_poll();
    return m68k_execute(cycles);
}

uint32_t q9_m68krt_get_d(q9_m68krt_t *rt, int n)
{
    (void)rt;
    return m68k_get_reg(NULL, (m68k_register_t)(M68K_REG_D0 + n));
}

void q9_m68krt_free(q9_m68krt_t *rt)
{
    q9_nettty_shutdown();       /* 2026-08-21: schliesst alle Kanal-/Server-Sockets, s. nettty.c */
    q9_sock_cleanup();          /* Windows-Build: WSACleanup unter Windows, no-op auf POSIX */

    g_ram     = NULL;
    g_ram_len = 0;
    g_board   = NULL;
    g_quicc   = NULL;
    m68k_set_int_ack_callback(0);
    memset(rt, 0, sizeof(*rt));
}

void q9_m68krt_set_irq(int level)
{
    m68k_set_irq((unsigned int)level);
}

void q9_m68krt_debug_state(uint32_t *pc, uint32_t *sr, uint32_t *acks)
{
    *pc   = m68k_get_reg(NULL, M68K_REG_PC);
    *sr   = m68k_get_reg(NULL, M68K_REG_SR);
    *acks = g_ack_count;
}

uint32_t q9_m68krt_quicc_acks(void)
{
    return g_quicc_ack_count;
}

int q9_m68krt_is_stopped(void)
{
    return m68k_is_stopped();
}

/* 6.5: Wrapper mit der generischen q9_cpu_backend_t-Signatur -- reichen nur an die vorhandenen
   q9_m68krt_*-Funktionen durch. ctx wird dabei ueberwiegend NICHT gebraucht (Musashi ist ein
   Singleton mit eigenen Globals, s. Typkommentar in m68krt.h), ist aber Teil der Vtable-Signatur,
   damit ein kuenftiges Nicht-Singleton-Backend (z.B. TinyEMU/RISC-V, s.
   third_party/tinyemu/Q9_VENDOR.md) ohne Vtable-Aenderung moeglich bleibt. */
static void cpu_backend_reset(void *ctx)
{
    q9_m68krt_reset((q9_m68krt_t *)ctx);
}

static int cpu_backend_execute(void *ctx, int cycles)
{
    return q9_m68krt_execute((q9_m68krt_t *)ctx, cycles);
}

static void cpu_backend_set_irq(void *ctx, int level)
{
    (void)ctx;
    q9_m68krt_set_irq(level);
}

static int cpu_backend_is_stopped(void *ctx)
{
    (void)ctx;
    return q9_m68krt_is_stopped();
}

void q9_m68krt_get_backend(q9_m68krt_t *rt, q9_cpu_backend_t *backend)
{
    backend->reset      = cpu_backend_reset;
    backend->execute    = cpu_backend_execute;
    backend->set_irq    = cpu_backend_set_irq;
    backend->is_stopped = cpu_backend_is_stopped;
    backend->ctx        = rt;
}

//────────────────────────────────────────────────────────────────────────────────────────────────
// EOF m68krt.c                                                                            Ver. 1.40
//────────────────────────────────────────────────────────────────────────────────────────────────
