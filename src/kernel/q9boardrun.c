//════════════════════════════════════════════════════════════════════════════════════════════════
// File:   q9boardrun.c                                                                      Ver. 2.00
// Owner:  AF
// Desc.:  Implementierung des Board-Boot-Runners, siehe q9boardrun.h.
//
// Edition History
//─────────┬──────┬────────────────────────────────────────────────────────────────────────┬──────
// Date    │ Ver. │ Description                                                            │ By
//─────────┼──────┼────────────────────────────────────────────────────────────────────────┬──────
// 26-07-05│ 1.00 │ 5.3: Erster Boot-Runner                                                 │ CF
// 26-07-05│ 1.10 │ 5.5a: cf_path-Parameter (NULL = Default BOARD_CF_IMAGE)                 │ CF
// 26-07-10│ 1.20 │ 5.7: q9_hal_con_flush() pro Runde -- TX-Ringpuffer-Rest ausliefern,     │ CF
//         │      │ auch ohne neues THRA-Byte im selben Durchlauf                           │
// 26-07-10│ 1.30 │ 5.9: Idle-Drossel -- q9_hal_sleep_ms(1) statt Busy-Loop, wenn die CPU    │ CF
//         │      │ per STOP angehalten ist UND kein IRQ ansteht (OS-9-Leerlauf)             │
// 26-07-13│ 1.40 │ 5.12: net_mode-Parameter -> q9_quicc_net_mode (nat|vmnet)               │ CF
// 26-07-14│ 1.50 │ 5.17: Hauptschleifen-Poll fuer DUART/Timer genericisiert (Geraete-        │ CF
//         │      │ Registry statt hartkodierter Bloecke), QUICC bleibt bis zu seinem eigenen │
//         │      │ 5.17-Schritt explizit verdrahtet                                          │
// 26-07-14│ 1.60 │ 5.17: QUICC ebenfalls umgezogen -- Hauptschleifen-Poll ist jetzt EINE      │ CF
//         │      │ einzige Schleife ueber die Geraete-Registry, keine Sonderfaelle mehr       │
// 26-07-16│ 1.70 │ 5.19: Board-Config (cfg-Parameter) -- mehrere CF-Images (rbf/pcf) auf       │ CF
//         │      │ Onboard-CF (Master/Slave) + RC2014-SC145-Zweitinterface verteilen           │
// 26-08-03│ 1.71 │ 5.24/5.26: MC6845 (crtc) + VRAM-Geraet (fb) nach attach_quicc verdrahtet    │ Ada
// 26-08-03│ 1.72 │ 5.27: Host-Video-Bridge (videobridge) initialisiert + je Hauptschleifen-    │ Ada
//         │      │ Runde gepollt                                                              │
// 26-08-07│ 1.80 │ 5.14: slirp_config/slirp_hostfwd an q9_quicc_net_mode -- net_hostfwd aus    │ AF
//         │      │ der Config wird per q9_parse_hostfwd zerlegt                               │
// 26-08-13│ 1.81 │ 6.5: CPU-Zugriffe (reset/execute/set_irq/is_stopped) auf die neue           │ Cld
//         │      │ q9_cpu_backend_t-Vtable (cpu_backend.h) umgestellt, statt m68krt-Funktionen  │
//         │      │ direkt zu rufen -- Vorbereitung fuer eine zweite Zielarchitektur, reines     │
//         │      │ Refactoring (debug_state/quicc_acks bleiben bewusst direkt, s. dortige       │
//         │      │ Kommentare)                                                                 │
// 26-08-14│ 1.90 │ 5.18-Fortsetzung: q9_cfg_cf_effective_base() (boardcfg.c) statt zweier hier   │ Cld
//         │      │ duplizierter Inline-Berechnungen; neue q9_board_validate_no_overlap() prueft │
//         │      │ NACH allen attach_*-Aufrufen die komplette Registry auf echte Adress-         │
//         │      │ ueberlappungen (useSlot/slot fuehrt erstmals frei waehlbare Adressen ein,     │
//         │      │ die mit fest verdrahteten Geraeten kollidieren koennten)                       │
// 26-08-15│ 2.00 │ Q9FLUX_EDITOR_de.md 4.1: neue q9_board_resolve_cpu() bildet den [board]-Key    │ Cld
//         │      │ "cpu" (boardcfg.h) auf q9_cpu_type_t ab und reicht ihn an q9_m68krt_init()      │
//         │      │ durch -- macht die CPU-Typ-Wahl config-steuerbar statt nur per versteckter      │
//         │      │ Q9_CPU=ec030-Env-Var                                                            │
//═════════╧══════╧════════════════════════════════════════════════════════════════════════╧══════
#include "q9boardrun.h"
#include "q9board.h"
#include "m68krt.h"
#include "../devices/duart68681/duart68681.h"      /* THRA-Mitschrift, s. dort                */
#include "../devices/quicc/quicc.h"
#include "../devices/mc6845/mc6845.h"                  /* 5.24: MC6845-CRT-Controller             */
#include "../devices/framebuf/framebuf.h"              /* 5.26: VRAM-Geraet                       */
#include "../devices/videobridge/videobridge.h"        /* 5.27: Host-Video-Bridge                  */
#include "devreg.h"
#include "boardcfg.h"
#include "../hal/q9_hal.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BOARD_RAM_BYTES   (16u * 1024u * 1024u)       /* 16 MByte SIM-Bestueckung (docs/BOARD.md) */
#define BOARD_ROM_MAX     (512u * 1024u)              /* 29F040-Flash: 512 KByte                  */
#define BOARD_CF_IMAGE    "local_images/board_cf.img" /* Backing-Datei, lazy angelegt (5.2c)      */
#define BOARD_SLICE_CYCLES 20000                       /* CPU-Takte je Runde zwischen Timer-Polls  */
#define Q9_MAX_HOSTFWD    8                             /* 5.14: Obergrenze net_hostfwd-Eintraege   */

/* Statisch statt Host-malloc (Q9-Grundsatz, vgl. Fixed-Heap-Entscheidung 4.9) — native-only,
   im BSS kostet das nichts, solange es unberuehrt bleibt. */
static uint8_t board_ram[BOARD_RAM_BYTES];
static uint8_t board_rom[BOARD_ROM_MAX];
static uint8_t board_vram[Q9_FRAMEBUF_MAX_SIZE];       /* 5.26: VRAM-Backing, s. framebuf.h        */

volatile int q9_dbg_dump_requested = 0;                /* s. q9boardrun.h */

//────────────────────────────────────────────────────────────────────────────────────────────────
// Function: dbg_dump_kernel_globals
// Desc.:    Debug-Sondertaste-Handler (Ctrl-^): liest physischen RAM direkt ueber
//           q9_board_read32, ohne MMU-Uebersetzung -- genau der Speicherbereich, den ein
//           User-State-Debugger (OS-9 "debug") wegen Bus-Error nicht erreicht. Adresse 0
//           enthaelt (falls VBR=0 nach Reset gilt) den System-Global-Zeiger (Q9-OS-RE-Fund:
//           "movec VBR,A6 / movea.l (A6),A6"-Idiom); wirkt plausibel, wird automatisch auch
//           D_ExcJmp (+0x68) und die beiden Syscall-Tabellen (+0x3a4/+0x3a8) mitgedumpt.
//────────────────────────────────────────────────────────────────────────────────────────────────
#define DBG_DUMP_FILE "local_images/q9dbg_dump.txt"

/* Q9-eigener-Kernel-Zusatzdump (2026-08-31, Abschnitt "IOMan-Einbindung"/
   F$SSvc-Debugging): der ORIGINALE Microware-Kernel legt bei physisch @0
   einen ZEIGER auf den System-Global-Bereich ab (s. Kopfkommentar unten),
   unser eigener q9kernel dagegen legt Q9K_GlobBase selbst auf Adresse 0 --
   physisch @0 enthaelt dort direkt Nutzdaten, keinen Zeiger. Der
   bestehende Dump-Pfad ist deshalb fuer unseren Kernel blind (v0 liest
   dort typischerweise 0 oder ein Datenfeld, kein plausibler Zeiger --
   bricht sofort ab). Dieser Zusatzabschnitt dumpt stattdessen direkt die
   uns bekannten, festen Q9K-Adressen (Q9-OS/src/kernel/q9kernel_entry.a
   bzw. q9kernel_moddir.c): Q9K_BootList (Regionsliste, $1000) und die
   aktive Moduldirectory-Kette (Q9K_MODDIR_HEAD_ADDR, $1238) mit Name +
   Headeradresse je Eintrag -- das war der konkrete Anlass: ein
   F$Link("ioman")-Fund, der ueber diese Kette zustande kommt, aber
   weder in Q9K_BootList noch im tatsaechlich gebooteten Image ein
   echtes "ioman"-Modul hat (Verdacht: False-Positive-Treffer im
   RAM-Scan, gleiche Bug-Klasse wie der bereits am 2026-08-21 gefixte
   ROM-Remap-Fund, s. Kopfkommentar Q9K_ModDirPopulateFromBootList).
   Laeuft IMMER zusaetzlich zum bestehenden Original-Kernel-Pfad
   (unabhaengig von v0) -- bei einem tatsaechlich gebooteten
   Original-Kernel enthalten diese Adressen einfach Datenmuell, klar
   erkennbar an unplausiblen Werten. */
#define Q9K_DBG_BOOTLIST_ADDR   0x1000UL
#define Q9K_DBG_MODDIR_HEAD     0x1238UL
#define Q9K_DBG_MODDIR_NEXT_OFF 0x00UL
#define Q9K_DBG_MODDIR_HDR_OFF  0x04UL
#define Q9K_DBG_MH_NAME_OFF     0x0CUL
#define Q9K_DBG_MH_SIZE_OFF     0x04UL

static void dbg_dump_q9kernel_extras(q9_board_t *b, FILE *f)
{
    fprintf(f, "\n--- Q9-eigener-Kernel-Zusatzdump ---\n");

    fprintf(f, "Q9K_BootList @0x%04lx (Regionen Basis/Laenge, bis Basis=0):\n",
            (unsigned long)Q9K_DBG_BOOTLIST_ADDR);
    for (int i = 0; i < 64; i++) {
        uint32_t entryAddr = (uint32_t)Q9K_DBG_BOOTLIST_ADDR + (uint32_t)(i * 8);
        uint32_t base = q9_board_read32(b, entryAddr);
        if (base == 0)
            break;
        uint32_t len = q9_board_read32(b, entryAddr + 4);
        fprintf(f, "  [%2d] Basis=%08x Laenge=%08x (Ende=%08x)\n", i, base, len, base + len);
    }

    fprintf(f, "Moduldirectory-Kette ab Q9K_MODDIR_HEAD_ADDR @0x%04lx:\n",
            (unsigned long)Q9K_DBG_MODDIR_HEAD);
    uint32_t slot = q9_board_read32(b, Q9K_DBG_MODDIR_HEAD);
    int      guard = 0;
    if (slot == 0)
        fprintf(f, "  (leer)\n");
    while (slot != 0 && slot < BOARD_RAM_BYTES && guard < 64) {
        uint32_t hdrAddr = q9_board_read32(b, slot + Q9K_DBG_MODDIR_HDR_OFF);
        char     name[33];
        int      k;

        for (k = 0; k < 32; k++) {
            uint8_t c;
            if (hdrAddr == 0 || hdrAddr >= BOARD_RAM_BYTES) { name[k] = 0; break; }
            uint32_t nameOff = q9_board_read32(b, hdrAddr + Q9K_DBG_MH_NAME_OFF);
            c = q9_board_read8(b, hdrAddr + nameOff + (uint32_t)k);
            if (c == 0) { name[k] = 0; break; }
            name[k] = (char)c;
        }
        name[32] = 0;

        uint32_t modSize = (hdrAddr != 0 && hdrAddr < BOARD_RAM_BYTES)
                                ? q9_board_read32(b, hdrAddr + Q9K_DBG_MH_SIZE_OFF)
                                : 0;
        /* TyLang direkt aus dem MODDIR-Slot (nicht aus dem Modulheader selbst --
           genau der Wert, den Q9K_ModDirLinkByName fuer den Typ-Filter-Vergleich
           heranzieht, s. q9kernel_moddir.c Q9K_MODDIR_TYLANG_OFF=0x08). */
        uint16_t tyLang;
        {
            uint8_t hi = q9_board_read8(b, slot + 0x08UL);
            uint8_t lo = q9_board_read8(b, slot + 0x09UL);
            tyLang = (uint16_t)((hi << 8) | lo);
        }

        fprintf(f, "  Slot=%08x HdrPtr=%08x Groesse=%08x TyLang=%04x Name=\"%s\"\n",
                slot, hdrAddr, modSize, tyLang, name);

        slot = q9_board_read32(b, slot + Q9K_DBG_MODDIR_NEXT_OFF);
        guard++;
    }
    if (guard >= 64)
        fprintf(f, "  (Abbruch nach 64 Eintraegen -- moegliche Ringkette?)\n");

    {   /* Warteschlangen des eigenen Kernels durchlaufen. Die Sentinels und
           Feldoffsets stammen aus q9kernel_sched.c (Ready $1240, Wait $12A0,
           Sleep $12F0; next=+$30, prev=+$34, Zustand=+$1d, ID=+$26-2).
           Zweck: sehen, in WELCHER Schlange ein Prozess wirklich haengt --
           der Zustandsbuchstabe allein sagt das nicht. */
        static const uint32_t sent[3] = { 0x1240u, 0x12A0u, 0x12F0u };
        static const char    *snam[3] = { "Ready", "Wait", "Sleep" };
        uint32_t q;

        for (q = 0u; q < 3u; q++) {
            uint32_t node = q9_board_read32(b, sent[q] + 0x30u);
            uint32_t guard = 0u;

            fprintf(f, "%s-Queue @%04x:", snam[q], (unsigned)sent[q]);
            while (node != sent[q] && guard++ < 16u) {
                if (node == 0u || node >= 0x1000000u) {
                    fprintf(f, " <unplausibel %08x>", (unsigned)node);
                    break;
                }
                fprintf(f, " %08x('%c')", (unsigned)node,
                        (int)q9_board_read8(b, node + 0x1du));
                node = q9_board_read32(b, node + 0x30u);
            }
            if (guard == 0u) {
                fprintf(f, " (leer)");
            }
            fprintf(f, "\n");
        }
    }

    {   /* Mitschrift der F$SSvc-Registrierungen (Q9-OS q9kernel_ssvc.c legt
           sie ab $1700 ab: [0] Anzahl, dann je 12 Byte Code/Routine/
           Tabelleneintrag). Zeigt, WER welchen Slot zuletzt beschreibt. */
        uint32_t n = q9_board_read32(b, 0x1700u);
        uint32_t k;

        fprintf(f, "F$SSvc-Registrierungen: %u\n", (unsigned)n);
        if (n > 40u) { n = 40u; }
        for (k = 0; k < n; k++) {
            uint32_t rec = 0x1704u + k * 12u;
            unsigned code = (unsigned)q9_board_read32(b, rec);
            fprintf(f, "    [%2u] Code $%04x -> %08x   (Eintrag @%08x)\n",
                    (unsigned)k, code,
                    (unsigned)q9_board_read32(b, rec + 4u),
                    (unsigned)q9_board_read32(b, rec + 8u));
        }
    }

    {   /* Wohin zeigen die I$-Slots wirklich? Verlaesslicher als jede
           Disassemblierung von Hand: die Dispatch-Tabellen stehen in
           D_SysDis ($3a4) / D_UsrDis ($3a8), der Eintrag eines Dienstes bei
           Basis + Callcode*4. */
        uint32_t sysdis = q9_board_read32(b, 0x3a4u);
        uint32_t usrdis = q9_board_read32(b, 0x3a8u);
        static const unsigned codes[] = { 0x84u, 0x89u, 0x8au, 0x8bu, 0x8cu };
        unsigned k;

        fprintf(f, "I$-Dispatch-Slots (SysDis @%08x / UsrDis @%08x):\n",
                (unsigned)sysdis, (unsigned)usrdis);
        for (k = 0; k < sizeof codes / sizeof codes[0]; k++) {
            fprintf(f, "    $%02x: sys=%08x  usr=%08x\n", codes[k],
                    (unsigned)q9_board_read32(b, sysdis + codes[k] * 4u),
                    (unsigned)q9_board_read32(b, usrdis + codes[k] * 4u));
        }
    }

    {   /* Frei waehlbare PC-Zaehler (Q9_COUNT_PC), s. m68krt.c */
        uint32_t k;
        if (q9_dbg_cpc_n > 0u) {
            fputs("PC-Zaehler (Q9_COUNT_PC):", f);
            for (k = 0u; k < q9_dbg_cpc_n; k++) {
                fprintf(f, " %08x=%u", (unsigned)q9_dbg_cpc_addr[k],
                        (unsigned)q9_dbg_cpc_hits[k]);
            }
            fputc('\n', f);
        }
    }

    {   /* Diagnose-Scratchzellen des eigenen Kernels ($1600-$1620) -- die
           Syscall-Bruecken legen dort Ein-/Ausgabewerte ab (s.
           Q9K_SEND_SCRATCH_* in q9kernel_procsleep.c). */
        uint32_t z;
        fprintf(f, "Scratchzellen $1600-$1620:");
        for (z = 0x1600u; z <= 0x1620u; z += 4u) {
            fprintf(f, " %08x", (unsigned)q9_board_read32(b, z));
        }
        fprintf(f, "\n");
    }

    {   /* Schreib-Watch (s. m68krt.c). Steht bewusst hier oben: der spaetere
           Zeigerlauf bricht ab, sobald ein Wert unplausibel ist -- alles
           dahinter wurde nie ausgegeben und der Watch sah leer aus, obwohl
           er Treffer hatte. */
        uint32_t z, first, cnt;
        fprintf(f, "\n--- Schreibzugriffe im Watch-Fenster (Q9_WATCH_ADDR/_LEN): %u, Zaehlerstand jetzt #%u ---\n",
                (unsigned)q9_dbg_wv_n, (unsigned)q9_dbg_write_seq_now());
        cnt   = (q9_dbg_wv_n < 64u) ? q9_dbg_wv_n : 64u;
        first = q9_dbg_wv_n - cnt;   /* aeltester noch vorhandener Treffer */
        for (z = first; z < q9_dbg_wv_n; z++) {
            uint32_t i = z % 64u;
            fprintf(f, "    #%-10u pc=%08x -> %08x schrieb %08x (%u Byte)\n",
                    (unsigned)q9_dbg_wv_seq[i],
                    (unsigned)q9_dbg_wv_pc[i], (unsigned)q9_dbg_wv_adr[i],
                    (unsigned)q9_dbg_wv_val[i], (unsigned)q9_dbg_wv_size[i]);
        }
    }

    fprintf(f, "--- Ende Q9-eigener-Kernel-Zusatzdump ---\n\n");
}

static void dbg_dump_kernel_globals(q9_board_t *b)
{
    /* In eine Datei statt nach stderr schreiben: bei groesseren Dumps (Syscall-Tabellen-Scan,
       D_ExcJmp) geht sonst Text im PTY-Puffer verloren, wenn viele Zeilen ohne Interaktion in
       einem Rutsch geschrieben werden (beobachtet 2026-08-02, s. Q9-OS-RE-Sitzung). Eine
       Textdatei ist robust und lässt sich danach in Ruhe komplett lesen. */
    FILE *f = fopen(DBG_DUMP_FILE, "w");
    if (!f) {
        fprintf(stderr, "\n[q9dbg] konnte %s nicht zum Schreiben oeffnen.\n", DBG_DUMP_FILE);
        return;
    }

    dbg_dump_q9kernel_extras(b, f);

    /* Diagnose (2026-09-04): DUART-Interruptzustand und die Polling-Tabelle
       des Q9-eigenen Kernels ($1500, 16 Eintraege a 20 Byte:
       Vektor/Prio/ISR/statisch/Port). Zweck: einen Interrupt-Sturm
       einordnen -- feuert die Quelle dauerhaft, und mit welchem Kontext
       ruft der Dispatcher die ISR auf? */
    {
        uint32_t i;

        fprintf(f, "\n--- DUART/IRQ-Zustand ---\n  IMR=%02x  IVR=%02x\n",
                (unsigned)b->uart_imr, (unsigned)b->uart_ivr);
        /* Steht wirklich etwas im Empfangs-FIFO? Bei IMR-Bit1 (RxRDY) meldet
           die Emulation genau dann Interrupt -- ein voller FIFO, den niemand
           leert, ergibt einen Dauerinterrupt. */
        fprintf(f, "  RX-FIFO: count=%u head=%u tail=%u overflow=%u\n",
                (unsigned)b->uart_rx_count, (unsigned)b->uart_rx_head,
                (unsigned)b->uart_rx_tail, (unsigned)b->uart_rx_overflow);
        fputs("  Polling-Tabelle (belegte Eintraege):\n", f);
        for (i = 0; i < 16u; i++) {
            uint32_t e   = 0x1500u + i * 20u;
            uint32_t vec = q9_board_read32(b, e);

            if (vec != 0) {
                fprintf(f, "    [%2u] Vektor=%-3u ISR=%08x statisch=%08x Port=%08x\n",
                        (unsigned)i, (unsigned)vec,
                        (unsigned)q9_board_read32(b, e + 8u),
                        (unsigned)q9_board_read32(b, e + 12u),
                        (unsigned)q9_board_read32(b, e + 16u));
            }
        }
        {
            extern uint32_t q9_dbg_ackvec[256], q9_dbg_acklevel[8];
            uint32_t k;

            fputs("  Interrupt-Acknowledge -- gelieferte Vektoren:\n", f);
            for (k = 0; k < 256u; k++) {
                if (q9_dbg_ackvec[k]) {
                    fprintf(f, "    Vektor %3u : %u mal\n", (unsigned)k,
                            (unsigned)q9_dbg_ackvec[k]);
                }
            }
            fputs("  ... nach Pegel:\n", f);
            for (k = 0; k < 8u; k++) {
                if (q9_dbg_acklevel[k]) {
                    fprintf(f, "    Level %u : %u mal\n", (unsigned)k,
                            (unsigned)q9_dbg_acklevel[k]);
                }
            }
        }
        fputs("--- Ende DUART/IRQ-Zustand ---\n", f);
    }

    /* Diagnose (2026-09-04): die Systemglobals, an denen der I/O-Weg haengt.
       ACHTUNG bei den Offsets: die Reihenfolge in Q9-OS/src/q9sysglob.a naiv
       durchzuzaehlen ergibt Werte, die um $1C ZU NIEDRIG sind (verifiziert an
       drei Punkten: D_Proc=$4C, D_SysRom=$64, D_SysDis=$3A4). Genau dieser
       Zaehlfehler hat schon einmal dazu gefuehrt, dass $64 fuer D_DevTbl
       gehalten wurde -- es ist D_SysRom. */
    {
        uint32_t devtbl = q9_board_read32(b, 0x80u);   /* D_DevTbl  */
        uint32_t sysrom = q9_board_read32(b, 0x64u);   /* D_SysRom  */
        uint32_t excjmp = q9_board_read32(b, 0x68u);   /* D_ExcJmp  */
        uint32_t i;

        fputs("\n--- Systemglobals (I/O-relevant) ---\n", f);
        fprintf(f, "  D_SysRom($64)=%08x  D_ExcJmp($68)=%08x  D_DevTbl($80)=%08x\n",
                (unsigned)sysrom, (unsigned)excjmp, (unsigned)devtbl);
        if (devtbl != 0 && devtbl < BOARD_RAM_BYTES) {
            fputs("  Geraetetabelle (erste 4 Eintraege a 16 Byte):\n", f);
            for (i = 0; i < 4u; i++) {
                uint32_t e = devtbl + i * 16u;
                fprintf(f, "    [%u] %08x %08x %08x %08x\n", (unsigned)i,
                        (unsigned)q9_board_read32(b, e),
                        (unsigned)q9_board_read32(b, e + 4u),
                        (unsigned)q9_board_read32(b, e + 8u),
                        (unsigned)q9_board_read32(b, e + 12u));
            }
        } else {
            fputs("  D_DevTbl ist LEER oder unplausibel -- I$Attach hat sie nie gefuellt.\n", f);
        }
        fputs("--- Ende Systemglobals ---\n", f);
    }

    /* Diagnose (2026-09-04): Exception-Mitschrift des Q9-eigenen Kernels.
       Q9K_ExcTrap (Q9-OS q9kernel_entry.a) legt bei jeder Exception einen
       festen Satz Felder ab $144020 ab. Sie hier auszugeben erspart es, den
       Kernel selbst mit Diagnose-Ausgaben zu versehen -- was bei diesem Kernel
       nachweislich das Symptom verschiebt (jede Aenderung der Modulgroesse
       verschiebt Ladeadressen und Interrupt-Zeitpunkte). */
    {
        uint32_t sr   = q9_board_read16(b, 0x144020u);
        uint32_t pc   = q9_board_read32(b, 0x144024u);
        uint32_t fmt  = q9_board_read16(b, 0x144028u);
        uint32_t a6   = q9_board_read32(b, 0x14402Cu);
        uint32_t sp   = q9_board_read32(b, 0x144030u);
        uint32_t ret0 = q9_board_read32(b, 0x144034u);
        uint32_t ret1 = q9_board_read32(b, 0x144038u);

        fputs("\n--- Q9K_ExcTrap-Mitschrift (0 = keine Exception aufgetreten) ---\n", f);
        fprintf(f, "  Vektor=%u (Fmt/Vektor-Wort %04x)  SR=%04x\n",
                (unsigned)((fmt & 0x0FFFu) / 4u), (unsigned)fmt, (unsigned)sr);
        fprintf(f, "  PC=%08x  A6=%08x  SP=%08x\n",
                (unsigned)pc, (unsigned)a6, (unsigned)sp);
        fprintf(f, "  Stack darunter: %08x %08x\n", (unsigned)ret0, (unsigned)ret1);
        {   /* Register und Stackauszug -- Q9K_ExcTrap sichert sie laengst
               ($144040 a0/a1, $144048 a2-a4, $144054 d0-d7, $144080 64 Byte
               Stack); sie auch auszugeben erspart einen weiteren Lauf. */
            uint32_t z;

            fprintf(f, "  A0=%08x A1=%08x  A2=%08x A3=%08x A4=%08x\n",
                    (unsigned)q9_board_read32(b, 0x144040u),
                    (unsigned)q9_board_read32(b, 0x144044u),
                    (unsigned)q9_board_read32(b, 0x144048u),
                    (unsigned)q9_board_read32(b, 0x14404Cu),
                    (unsigned)q9_board_read32(b, 0x144050u));
            fputs("  D0-D7:", f);
            for (z = 0u; z < 8u; z++) {
                fprintf(f, " %08x", (unsigned)q9_board_read32(b, 0x144054u + z * 4u));
            }
            fputs("\n  Stack ab SP:", f);
            for (z = 0u; z < 16u; z++) {
                if (z == 8u) {
                    fputs("\n              ", f);
                }
                fprintf(f, " %08x", (unsigned)q9_board_read32(b, 0x144080u + z * 4u));
            }
            fputc('\n', f);
        }
        fputs("--- Ende Exception-Mitschrift ---\n", f);
    }

    /* Diagnose (2026-09-03): was tatsaechlich auf THRA geschrieben wurde, plus
       CPU-Zustand beim selben Zugriff -- s. duart68681.h. Weichen Buswert und
       d0.b voneinander ab, entsteht eine Verstuemmelung erst beim Schreiben;
       sind sie gleich, hat der Gast den Wert schon falsch im Register. */
    {
        uint32_t n = q9_dbg_thra_count;
        uint32_t i;

        if (n > Q9_DBG_THRA_LOG_SIZE) {
            n = Q9_DBG_THRA_LOG_SIZE;
        }
        fprintf(f, "\n--- THRA-Mitschrift: %u Bytes geschrieben (aufgezeichnet: %u) ---\n",
                (unsigned)q9_dbg_thra_count, (unsigned)n);
        for (i = 0; i < n; i++) {
            uint8_t c = q9_dbg_thra_log[i];
            fputc((c >= 0x20u && c < 0x7fu) ? (int)c : '.', f);
        }
        fputs("\n  Abweichungen Buswert/d0 (erste 32):\n", f);
        {
            uint32_t shown = 0;
            for (i = 0; i < n && shown < 32u; i++) {
                if (q9_dbg_thra_log[i] != q9_dbg_thra_d0[i]) {
                    fprintf(f, "    [%3u] bus=%02x d0=%02x pc=%08x\n", (unsigned)i,
                            (unsigned)q9_dbg_thra_log[i], (unsigned)q9_dbg_thra_d0[i],
                            (unsigned)q9_dbg_thra_pc[i]);
                    shown++;
                }
            }
            fprintf(f, "    Abweichungen gesamt unter %u: %u\n", (unsigned)n, (unsigned)shown);
        }
        fputs("--- Ende THRA-Mitschrift ---\n", f);
    }

    /* Diagnose: Instruktionsspur vor der Anomalie (nur mit Q9_TRACE_INSTR=1
       gefuellt, s. m68krt.h). Aeltester Eintrag zuerst. */
    if (q9_dbg_tr_fill != 0u) {
        uint32_t k;

        fprintf(f, "\n--- Instruktionsspur (frozen=%d, %u Eintraege, aeltester zuerst) ---\n",
                q9_dbg_tr_frozen, (unsigned)q9_dbg_tr_fill);
        for (k = 0; k < q9_dbg_tr_fill; k++) {
            uint32_t idx = (q9_dbg_tr_head + Q9_DBG_TR_SIZE - q9_dbg_tr_fill + k) % Q9_DBG_TR_SIZE;
            fprintf(f, "  pc=%08x d0=%08x a0=%08x sp=%08x\n",
                    (unsigned)q9_dbg_tr_pc[idx], (unsigned)q9_dbg_tr_d0[idx],
                    (unsigned)q9_dbg_tr_a0[idx], (unsigned)q9_dbg_tr_sp[idx]);
        }
        fputs("--- Ende Instruktionsspur ---\n", f);

        /* Stackbereich bei jedem Dispatcher-Eintritt, ZUM ZEITPUNKT des
           Eintritts im Hook gesichert (s. m68krt.c). Daran laesst sich die
           Lage des Exception-Frames ablesen: gesucht ist das Format-/Vektor-
           Wort, davor stehen PC und SR des unterbrochenen Codes. */
        if (q9_dbg_ent_n != 0u) {
            uint32_t e, w;

            fprintf(f, "\n--- Stack bei Dispatcher-Eintritt (%u erfasst) ---\n",
                    (unsigned)q9_dbg_ent_n);
            for (e = 0; e < q9_dbg_ent_n; e++) {
                fprintf(f, "  [%2u] sp=%08x:", (unsigned)e, (unsigned)q9_dbg_ent_sp[e]);
                for (w = 0; w < Q9_DBG_ENT_WORDS; w++) {
                    fprintf(f, " %04x", (unsigned)q9_dbg_ent_stk[e][w]);
                }
                fputc(0x0a, f);
            }
            fputs("--- Ende Stack bei Eintritt ---\n", f);
        }

            /* Dasselbe unmittelbar vor dem RTE. Weicht eine Zeile von der
               gleichnamigen Eintritts-Zeile ab, wurde der Frame waehrend des
               Durchlaufs ueberschrieben -- genau das erklaert einen RTE, der nicht
               dorthin springt, wo sein Frame hinzeigt. */
            if (q9_dbg_exi_n != 0u) {
                uint32_t e, w;

                fprintf(f, "\n--- Stack vor dem RTE (%u erfasst) ---\n",
                        (unsigned)q9_dbg_exi_n);
                for (e = 0; e < q9_dbg_exi_n; e++) {
                    fprintf(f, "  [%2u] sp=%08x:", (unsigned)e, (unsigned)q9_dbg_exi_sp[e]);
                    for (w = 0; w < Q9_DBG_ENT_WORDS; w++) {
                        fprintf(f, " %04x", (unsigned)q9_dbg_exi_stk[e][w]);
                    }
                    fputc(0x0a, f);
                }
                fputs("--- Ende Stack vor dem RTE ---\n", f);
            }

                /* Wo schlaegt der Board-Timer zu? Trifft er einen PC INNERHALB des
                   IRQ-Dispatchers, wird dessen halb aufgebauter Stack im
                   Prozessdeskriptor gesichert und spaeter wieder aufgesetzt -- genau
                   das erklaert einen Dispatcher-Ausgang ohne zugehoerigen Eingang. */
                {
                    uint32_t t;

                    fprintf(f, "\n--- Timer-Interrupts: %u gesamt, davon %u im IRQ-Dispatcher ---\n",
                            (unsigned)q9_dbg_tmr_total, (unsigned)q9_dbg_tmr_indisp);
                    for (t = 0; t < q9_dbg_tmr_indisp && t < 8u; t++) {
                        fprintf(f, "    unterbrochener PC: %08x\n", (unsigned)q9_dbg_tmr_pcs[t]);
                    }

    fprintf(f, "--- Weckpfad der sc68681-ISR: Block betreten %u mal, F$Send %u mal ---\n",
            (unsigned)q9_dbg_wake_enter, (unsigned)q9_dbg_wake_send);

    /* Exception-Vektortabelle: welche Slots zeigen in den IRQ-Dispatcher?
       Ein Slot, der NICHT auf dessen Einstieg ($795c) zeigt, sondern
       mitten hinein, erklaert einen Durchlauf ohne einleitendes movem. */
    {
        uint32_t tb = q9_board_read32(b, 0x68u);
        uint32_t v;
        fprintf(f, "\n--- Vektorslots mit Ziel im IRQ-Dispatcher (Tabelle @%08x) ---\n",
                (unsigned)tb);
        if (tb != 0u && tb < BOARD_RAM_BYTES) {
            for (v = 0; v < 256u; v++) {
                uint32_t h = q9_board_read32(b, tb + v * 4u);
                if (h >= 0x7950u && h <= 0x79e0u) {
                    fprintf(f, "    Vektor %3u -> %08x%s\n", (unsigned)v, (unsigned)h,
                            (h == 0x795cu) ? "  (Einstieg, ok)" : "  <== MITTEN HINEIN");
                }
            }
        }
    }
                }
    }

    uint32_t v0 = q9_board_read32(b, 0);

    fprintf(f, "physisch @0x00000000 = %08x (Kandidat System-Global-Zeiger)\n", v0);
    if (v0 == 0 || v0 >= BOARD_RAM_BYTES) {
        fprintf(f, "Wert ausserhalb 0..%uM RAM -- kein plausibler Zeiger, breche ab.\n",
                BOARD_RAM_BYTES / (1024u * 1024u));
        fclose(f);
        fprintf(stderr, "\n[q9dbg] Dump (abgebrochen) geschrieben nach %s\n", DBG_DUMP_FILE);
        return;
    }
    uint32_t excjmp = q9_board_read32(b, v0 + 0x68);
    uint32_t sysdis = q9_board_read32(b, v0 + 0x3a4);
    uint32_t usrdis = q9_board_read32(b, v0 + 0x3a8);

    fprintf(f, "D_ExcJmp   @+0x68  = %08x\n", excjmp);
    fprintf(f, "D_SysDis   @+0x3a4 = %08x\n", sysdis);
    fprintf(f, "D_UsrDis   @+0x3a8 = %08x\n", usrdis);
    fprintf(f, "(0x8e4)    @+0x8e4 = %08x\n", q9_board_read32(b, v0 + 0x8e4));
    fprintf(f, "32 Byte ab System-Global-Basis:\n ");
    for (int i = 0; i < 32; i++) {
        fprintf(f, "%02x ", q9_board_read8(b, v0 + (uint32_t)i));
        if (i == 15) fprintf(f, "\n ");
    }
    fprintf(f, "\n");

    /* Syscall-Tabellen scannen: 256 Eintraege x 4 Byte je Primaerarray (0x000-0x3FF ab sysdis/
       usrdis). Slot 0 wird als Basiswert fuer "nicht registriert/Fehler-Stub" angenommen (laut
       RE-Fund fast alle Eintraege identisch); nur Ausreisser (= tatsaechlich registrierte
       Funktionsnummern) werden gedruckt, sonst waeren 256 Zeilen ueberwiegend Rauschen. */
    for (int t = 0; t < 2; t++) {
        uint32_t base = t == 0 ? sysdis : usrdis;
        uint32_t baseline;
        int      outliers = 0;

        if (base == 0 || base >= BOARD_RAM_BYTES) {
            fprintf(f, "%s unplausibel, ueberspringe Scan.\n", t == 0 ? "D_SysDis" : "D_UsrDis");
            continue;
        }
        baseline = q9_board_read32(b, base);
        fprintf(f, "%s-Primaerarray Scan (Basiswert Slot0=%08x, nur Abweichungen):\n",
                t == 0 ? "D_SysDis" : "D_UsrDis", baseline);
        for (int i = 0; i < 256; i++) {
            uint32_t v = q9_board_read32(b, base + (uint32_t)(i * 4));
            if (v != baseline) {
                fprintf(f, "  Slot %3d (0x%02x) = %08x\n", i, i, v);
                outliers++;
            }
        }
        if (outliers == 0) fprintf(f, "  (keine Abweichungen -- alle Slots = Basiswert)\n");
    }

    /* D_ExcJmp: 256 Eintraege x 10 Byte, Format bereits geklaert (Q9-OS-RE-Sitzung 2026-08-02):
       PEA (vektor*4+8).W ; JMP.L <ziel> -- <ziel> ist die tatsaechliche Dispatcher-Adresse.
       Kernel-Basis wird aus dem Fehler-Stub-Mehrheitswert der Syscall-Tabelle abgeleitet
       (Fehler-Stub liegt bei Modul-Offset 0x1380, s. REVERSE_ENGINEERING.md), damit direkt
       <ziel>-Kernel-Basis mit ausgegeben wird -- das laesst sich sofort gegen die bekannten
       Q9_disp_*-Offsets (0x180/0x452/0x472/0x488/0x5d0/0x888/0x8d0/0xba4) abgleichen. Alle
       Vektoren 2-63 (kompletter dokumentierter Bereich) plus ein paar Stichproben aus dem
       User-Defined-Bereich (64-255). */
    if (excjmp != 0 && excjmp < BOARD_RAM_BYTES) {
        /* Fehler-Stub-Mehrheitswert der Syscall-Tabelle als Kernel-Basis-Referenz (haeufigster
           Wert im D_SysDis-Array, s. Scan oben -- hier zur Einfachheit erneut ermittelt statt
           durchgereicht). */
        uint32_t counts[256];
        uint32_t vals[256];
        int      n = 0;
        for (int i = 0; i < 256; i++) {
            uint32_t v = q9_board_read32(b, sysdis + (uint32_t)(i * 4));
            int      found = 0;
            for (int j = 0; j < n; j++) {
                if (vals[j] == v) { counts[j]++; found = 1; break; }
            }
            if (!found && n < 256) { vals[n] = v; counts[n] = 1; n++; }
        }
        uint32_t stub = vals[0];
        {
            uint32_t best = 0;
            for (int j = 0; j < n; j++) if (counts[j] > best) { best = counts[j]; stub = vals[j]; }
        }
        uint32_t kbase = stub - 0x1380;
        fprintf(f, "Fehler-Stub-Mehrheitswert = %08x -> Kernel-Basis (angenommen -0x1380) = %08x\n",
                stub, kbase);

        /* WICHTIG: D_ExcJmp ist 0-indiziert ab Vektor 2 (Vektoren 0/1 = Reset-SSP/PC laufen nie
           ueber diese Tabelle) -- Array-Index = Vektor-2. Frueherer Off-by-2-Fehler (Vektor direkt
           als Index benutzt) durch Live-Vergleich mit den bekannten Q9_disp_*-Adressen gefunden
           und hier korrigiert (Q9-OS-RE-Sitzung 2026-08-02). */
        fprintf(f, "D_ExcJmp-Eintraege, Format PEA (v).W;JMP.L ziel, entschluesselt:\n");
        for (int vec = 2; vec <= 63; vec++) {
            uint32_t entry = excjmp + (uint32_t)((vec - 2) * 10);
            uint8_t  raw[10];
            for (int i = 0; i < 10; i++) raw[i] = q9_board_read8(b, entry + (uint32_t)i);
            uint32_t pea_val = ((uint32_t)raw[2] << 8) | raw[3];
            uint32_t target  = ((uint32_t)raw[6] << 24) | ((uint32_t)raw[7] << 16)
                              | ((uint32_t)raw[8] << 8) | raw[9];
            long     rel     = (long)target - (long)kbase;
            fprintf(f, "  Vektor %3d: pea=%04x target=%08x target-kbase=%+06ld (0x%lx)\n",
                    vec, pea_val, target, rel, rel);
        }
        fprintf(f, "Stichproben User-Defined-Bereich:\n");
        {
            static const int samples[] = { 64, 90, 128, 180, 200, 255 };
            for (unsigned si = 0; si < sizeof(samples) / sizeof(samples[0]); si++) {
                int      vec   = samples[si];
                uint32_t entry = excjmp + (uint32_t)((vec - 2) * 10);
                uint8_t  raw[10];
                for (int i = 0; i < 10; i++) raw[i] = q9_board_read8(b, entry + (uint32_t)i);
                uint32_t pea_val = ((uint32_t)raw[2] << 8) | raw[3];
                uint32_t target  = ((uint32_t)raw[6] << 24) | ((uint32_t)raw[7] << 16)
                                  | ((uint32_t)raw[8] << 8) | raw[9];
                long     rel     = (long)target - (long)kbase;
                fprintf(f, "  Vektor %3d: pea=%04x target=%08x target-kbase=%+06ld (0x%lx)\n",
                        vec, pea_val, target, rel, rel);
            }
        }
    }

    fclose(f);
    fprintf(stderr, "\n[q9dbg] Dump geschrieben nach %s\n", DBG_DUMP_FILE);
}

//────────────────────────────────────────────────────────────────────────────────────────────────
// Function: q9_streq_ci
// Desc.:    Kleiner Gross-/Kleinschreibungs-unabhaengiger String-Vergleich (kein strcasecmp -- das
//           ist POSIX/BSD, nicht garantiert C99/ueberall verfuegbar) fuer q9_parse_hostfwd.
//────────────────────────────────────────────────────────────────────────────────────────────────
static int q9_streq_ci(const char *a, const char *b)
{
    while (*a && *b) {
        char ca = *a, cb = *b;
        if (ca >= 'A' && ca <= 'Z') ca = (char)(ca - 'A' + 'a');
        if (cb >= 'A' && cb <= 'Z') cb = (char)(cb - 'A' + 'a');
        if (ca != cb) return 0;
        a++; b++;
    }
    return *a == '\0' && *b == '\0';
}

//────────────────────────────────────────────────────────────────────────────────────────────────
// Function: q9_parse_hostfwd
// Desc.:    5.14: net_hostfwd aus der .q9-Config ("tcp:2323:23,udp:5000:5000", s. boardcfg.h) in
//           ein q9_slirp_hostfwd_t-Array zerlegen. Fehlerhafte Eintraege werden einzeln auf stderr
//           gemeldet und uebersprungen statt den ganzen Boot abzubrechen -- ein Tippfehler in EINER
//           Portweiterleitung soll nicht den kompletten Emulator-Start verhindern.
//────────────────────────────────────────────────────────────────────────────────────────────────
static int q9_parse_hostfwd(const char *s, q9_slirp_hostfwd_t *out, int max)
{
    char buf[256];
    char *tok, *saveptr = NULL;
    int   count = 0;

    if (!s || !s[0]) {
        return 0;
    }
    {
        size_t n = strlen(s);
        if (n >= sizeof(buf)) n = sizeof(buf) - 1u;
        memcpy(buf, s, n);
        buf[n] = '\0';
    }
    for (tok = strtok_r(buf, ",", &saveptr); tok && count < max; tok = strtok_r(NULL, ",", &saveptr)) {
        char proto[8];
        unsigned hp = 0, gp = 0;
        if (sscanf(tok, "%7[a-zA-Z]:%u:%u", proto, &hp, &gp) == 3 && hp <= 65535u && gp <= 65535u &&
            (q9_streq_ci(proto, "tcp") || q9_streq_ci(proto, "udp"))) {
            out[count].is_udp     = q9_streq_ci(proto, "udp");
            out[count].host_port  = (uint16_t)hp;
            out[count].guest_port = (uint16_t)gp;
            count++;
        } else {
            fprintf(stderr, "q9board: net_hostfwd-Eintrag ignoriert (Format tcp|udp:hostport:gastport): '%s'\n",
                    tok);
        }
    }
    return count;
}

//────────────────────────────────────────────────────────────────────────────────────────────────
// Function: q9_board_resolve_cpu
// Desc.:    Q9FLUX_EDITOR_de.md 4.1: bildet den rohen [board]-Key "cpu" (boardcfg.h, bereits von
//           boardcfg.c gegen die Whitelist geprueft) auf m68krt.h's q9_cpu_type_t ab. Leerer/nicht
//           gesetzter String -> Q9_CPU_68030 (bisheriger Default, echte Q9-Hardware). Bewusst HIER
//           statt in boardcfg.c: boardcfg.c bleibt schlank und kennt m68krt.h nicht (s. dortiger
//           Kopfkommentar) -- diese Funktion ist der einzige Ort, der beide Seiten kennt.
// Call:     q9_cpu_type_t t = q9_board_resolve_cpu(cfg ? cfg->cpu : "");
//────────────────────────────────────────────────────────────────────────────────────────────────
static q9_cpu_type_t q9_board_resolve_cpu(const char *cpu_str)
{
    if (!cpu_str || !cpu_str[0]) { return Q9_CPU_68030; }
    if (q9_streq_ci(cpu_str, "68000"))   { return Q9_CPU_68000;   }
    if (q9_streq_ci(cpu_str, "68010"))   { return Q9_CPU_68010;   }
    if (q9_streq_ci(cpu_str, "68020"))   { return Q9_CPU_68020;   }
    if (q9_streq_ci(cpu_str, "68ec020")) { return Q9_CPU_68EC020; }
    if (q9_streq_ci(cpu_str, "68ec030")) { return Q9_CPU_68EC030; }
    if (q9_streq_ci(cpu_str, "68040"))   { return Q9_CPU_68040;   }
    if (q9_streq_ci(cpu_str, "68ec040")) { return Q9_CPU_68EC040; }
    if (q9_streq_ci(cpu_str, "68lc040")) { return Q9_CPU_68LC040; }
    return Q9_CPU_68030;                              /* "68030" und jeder unbekannte Rest (kann
                                                          dank boardcfg.c-Whitelist eigentlich nur
                                                          "68030" oder leer sein) */
}

//────────────────────────────────────────────────────────────────────────────────────────────────
// Function: q9_board_validate_no_overlap
// Desc.:    5.18-Fortsetzung (2026-08-14): prueft ALLE registrierten Geraete paarweise auf echte
//           Adressfenster-Ueberlappung (echte Byte-Bereiche, NICHT das 256-Byte-Dispatch-Raster
//           der I/O-Tabelle in m68krt.c -- MC6845 ($FFFFA000-A001) und CLUT ($FFFFA010-A013)
//           TEILEN sich zwar denselben 256-Byte-Slot, ueberlappen sich aber NICHT byteweise und
//           werden hier absichtlich NICHT als Fehler erkannt, s. m68krt.c g_io_ambiguous-
//           Kommentar). War seit ARBEITSPLAN 5.18 (2026-07-14, busmap-Planungsrunde) als Idee
//           vorgesehen ("Ueberlappungs-Validierung... Fehlermeldung mit BEIDEN Instanznamen +
//           Adressen, Abbruch"), aber nie gebaut -- jetzt noetig, weil useSlot/slot (boardcfg.h)
//           erstmals FREI WAEHLBARE Adressen (0-255) einfuehrt, die versehentlich mit einem
//           fest verdrahteten Geraet (RTC, QUICC, MC6845/CLUT, UART, ...) kollidieren koennen --
//           boardcfg.c kann das beim Parsen nicht wissen (Registry existiert da noch nicht),
//           diese Pruefung laeuft daher NACH allen attach_*-Aufrufen, VOR dem ersten CPU-Zyklus.
//           O(n^2) ueber die Registry (n <= Q9_DEVREG_MAX = 24) -- trivial billig, laeuft nur
//           einmal beim Start.
// Call:     if (q9_board_validate_no_overlap() != 0) return 1;
//────────────────────────────────────────────────────────────────────────────────────────────────
static int q9_board_validate_no_overlap(void)
{
    int n = q9_devreg_count();
    for (int i = 0; i < n; i++) {
        q9_device_t *a = q9_devreg_get(i);
        if (!a || a->size == 0) {
            continue;
        }
        uint32_t a_lo = a->base, a_hi = a->base + a->size - 1u;
        for (int j = i + 1; j < n; j++) {
            q9_device_t *b = q9_devreg_get(j);
            uint32_t b_lo, b_hi;
            if (!b || b->size == 0) {
                continue;
            }
            b_lo = b->base;
            b_hi = b->base + b->size - 1u;
            if (a_lo <= b_hi && b_lo <= a_hi) {
                fprintf(stderr,
                        "q9board: Adressueberlappung -- '%s' ($%08X-$%08X) und '%s' ($%08X-$%08X)\n",
                        a->name, a_lo, a_hi, b->name, b_lo, b_hi);
                return -1;
            }
        }
    }
    return 0;
}

int q9_board_boot(const char *rom_path, const char *cf_path, const char *net_mode,
                  const q9_board_cfg_t *cfg)
{
    static q9_board_t board;                           /* eine Instanz, wie Musashi selbst (5.1) */
    static q9_quicc_t quicc;                           /* 5.11: QUICC-Ethernet (SCC1)            */
    static q9_mc6845_t crtc;                           /* 5.24: MC6845-CRT-Controller             */
    static q9_framebuf_t fb;                            /* 5.26: VRAM-Geraet                       */
    static q9_clut_t  clut;                             /* 5.29-Nachtrag: CLUT-Geraet               */
    static q9_videobridge_t videobridge;                /* 5.27: Host-Video-Bridge                  */
    static q9_cf_t    cf_extra[Q9_CFG_MAX_CF];
    static uint32_t   cf_extra_base[Q9_CFG_MAX_CF];
    static int        cf_extra_count;
    q9_vmnet_config_t vmnet_config;
    const q9_vmnet_config_t *vmnet_config_ptr = 0;
    q9_slirp_config_t  slirp_config;
    const q9_slirp_config_t *slirp_config_ptr = 0;
    q9_slirp_hostfwd_t slirp_hostfwd[Q9_MAX_HOSTFWD];
    int                 slirp_hostfwd_count = 0;
    q9_m68krt_t       rt;
    q9_cpu_backend_t  cpu;                              /* 6.5: reset/execute/set_irq/is_stopped   */
    uint32_t          rom_len = 0;
    int               cf2_used   = 0;
    int               onboard_from_cfg = 0;

    /* 5.19: Vorrang klaeren. ROM: CLI schlaegt Config; ohne beides Fehler (kein Default-ROM,
       das echte Boot-ROM ist proprietaer, s. q9boardrun.h). */
    if ((rom_path == NULL || rom_path[0] == '\0') && cfg && cfg->rom_path[0]) {
        rom_path = cfg->rom_path;
    }
    if (rom_path == NULL || rom_path[0] == '\0') {
        fprintf(stderr, "q9board: kein Boot-ROM angegeben (--rom <rom> oder [board] rom= in der Config)\n");
        return 1;
    }

    /* Netz-Backend: CLI schlaegt Config schlaegt Default (nat, in q9_quicc_net_mode). */
    if ((net_mode == NULL || net_mode[0] == '\0') && cfg && cfg->net_mode[0]) {
        net_mode = cfg->net_mode;
    }
    if (cfg) {
        vmnet_config.guest_ip = cfg->vmnet_ip;
        vmnet_config.gateway  = cfg->vmnet_gateway;
        vmnet_config.netmask  = cfg->vmnet_netmask;
        vmnet_config.dhcp_end = cfg->vmnet_dhcp_end;
        vmnet_config_ptr = &vmnet_config;

        /* 5.14: slirp wiederverwendet dieselben vmnet_ip/_gateway/_netmask-Config-Keys (identisches
           Subnetz-Modell, s. boardcfg.h) -- eigener Name (guest_ip statt vmnet_ip) nur, weil
           q9_slirp_config_t bewusst kein vmnet_net.h inkludiert (s. slirp_net.h-Kopfkommentar). */
        slirp_config.guest_ip = cfg->vmnet_ip;
        slirp_config.gateway  = cfg->vmnet_gateway;
        slirp_config.netmask  = cfg->vmnet_netmask;
        slirp_config_ptr    = &slirp_config;
        slirp_hostfwd_count = q9_parse_hostfwd(cfg->net_hostfwd, slirp_hostfwd, Q9_MAX_HOSTFWD);
    }

    if (q9_board_rom_load(rom_path, board_rom, sizeof(board_rom), &rom_len) != Q9_BOARD_OK) {
        fprintf(stderr, "q9board: ROM-Datei '%s' nicht lesbar (fehlt, leer oder > %u KByte)\n",
                rom_path, BOARD_ROM_MAX / 1024u);
        return 1;
    }

    if (cfg && cfg->name[0]) {
        printf("q9board: Board-Config '%s'\r\n", cfg->name);
    }
    printf("q9board: ROM '%s' geladen (%u Byte), %u MByte RAM — Reset.\r\n",
           rom_path, rom_len, BOARD_RAM_BYTES / (1024u * 1024u));

    q9_board_init(&board, board_rom, rom_len, board_ram, sizeof(board_ram));
    cf_extra_count = 0;

    /* 5.19: CF-Images aus der Config verteilen — Onboard-CF (board.cf) und RC2014-Zweitinterface
       (cf2), je Master/Slave. Danach setzt ein explizites --cf immer die Onboard-Master-Einheit
       (CLI schlaegt Config, deckt den bisherigen Ein-Image-Weg ab). */
    if (cfg) {
        for (int i = 0; i < cfg->cf_count; i++) {
            const q9_cfg_cf_t *e = &cfg->cf[i];
            /* 2026-08-14 (5.18-Fortsetzung): q9_cfg_cf_effective_base() beruecksichtigt jetzt
               auch useSlot/slot (boardcfg.h) -- einzige Stelle fuer diese Berechnung, ersetzt
               die bisher hier UND weiter unten (Duplikat-Erkennung) duplizierte Inline-Logik. */
            uint32_t base = q9_cfg_cf_effective_base(e);
            q9_cf_t *iface = 0;
            if (base == Q9_BOARD_CF_BASE) {
                iface = &board.cf;
                onboard_from_cfg = 1;
            } else {
                for (int j = 0; j < cf_extra_count; j++)
                    if (cf_extra_base[j] == base) iface = &cf_extra[j];
                if (!iface && cf_extra_count < Q9_CFG_MAX_CF) {
                    iface = &cf_extra[cf_extra_count];
                    cf_extra_base[cf_extra_count++] = base;
                }
                cf2_used = 1;
            }
            if (!iface) { fprintf(stderr, "q9board: zu viele CF-Bases in Config\n"); return 1; }
            /* Mehrere Descriptoren dürfen dieselbe Hardware und dasselbe Backing-Image
               beschreiben (Partitionen, z.B. e0/e1). Das Interface wird nur einmal bestückt;
               die jeweilige PD_LSNOffs steht im OS-9-Descriptor. */
            int duplicate = 0;
            for (int j = 0; j < i; j++) {
                const q9_cfg_cf_t *p = &cfg->cf[j];
                uint32_t pb = q9_cfg_cf_effective_base(p);
                if (pb == base && p->unit == e->unit && strcmp(p->path, e->path) == 0) {
                    duplicate = 1;
                    break;
                }
            }
            if (!duplicate) {
                q9_cf_attach(iface, e->unit, e->path, e->format);
                q9_cf_set_start_sector(iface, e->unit, e->start_sector);
            }
            printf("q9board: CF %s/%s <- %s (%s)\r\n",
                   e->bus == Q9_CFG_BUS_RC2014 ? "secondary" : "onboard",
                   e->unit ? "slave" : "master", e->path,
                   e->format == Q9_CF_FMT_PCF ? "pcf" :
                   e->format == Q9_CF_FMT_RBF ? "rbf" : "auto");
        }
    }

    /* CLI --cf: Onboard-Master. Ueberschreibt eine etwaige Config-Onboard-Master-Angabe; ohne
       Config UND ohne --cf bleibt der bisherige Default (board_cf.img), damit Andreas' fertige
       Startzeilen unveraendert funktionieren. */
    if (cf_path && cf_path[0]) {
        q9_board_cf_attach(&board, cf_path);
        printf("q9board: CF onboard/master <- %s (auto)\r\n", cf_path);
    } else if (!onboard_from_cfg) {
        q9_board_cf_attach(&board, BOARD_CF_IMAGE);
    }

    if (cfg && cfg->cpu[0]) {
        printf("q9board: CPU <- %s (Config)\r\n", cfg->cpu);
    }

    fflush(stdout);                                    /* Banner raus, bevor der CPU-Loop beginnt */

    q9_m68krt_init(&rt, board_ram, sizeof(board_ram), q9_board_resolve_cpu(cfg ? cfg->cpu : ""));
    q9_m68krt_get_backend(&rt, &cpu);                  /* 6.5: ab hier nur noch ueber die Vtable  */
    q9_m68krt_attach_board(&board);                    /* ab jetzt laeuft ALLES ueber das Board  */
    if (cf2_used) {
        for (int i = 0; i < cf_extra_count; i++)
            q9_m68krt_attach_cf_at(&cf_extra[i], cf_extra_base[i], "cf-secondary");
    }
    q9_quicc_init(&quicc, board_ram, sizeof(board_ram));
    if (q9_quicc_net_mode(&quicc, net_mode, vmnet_config_ptr,
                          slirp_config_ptr, slirp_hostfwd, slirp_hostfwd_count) != 0) {
        return 1;
    }
    q9_m68krt_attach_quicc(&quicc);                    /* 5.11: Ethernet-Fenster $FFFF2000       */
    q9_mc6845_init(&crtc);                              /* 5.24: MC6845, Reset-Zustand = alle 0   */
    q9_m68krt_attach_mc6845(&crtc);                     /* 5.24: CRTC-Fenster $FFFFA000           */
    q9_framebuf_init(&fb, board_vram, sizeof(board_vram), Q9_FRAMEBUF_DEFAULT_SIZE, &crtc);
    q9_m68krt_attach_framebuf(&fb);                     /* 5.26: VRAM-Fenster $FD000000           */
    q9_clut_init(&clut);                                /* 5.29-Nachtrag: CLUT, Identitaets-Graustufe */
    q9_m68krt_attach_clut(&clut);                       /* 5.29-Nachtrag: CLUT-Fenster $FFFFA010  */

    /* 5.18-Fortsetzung: NACH allen attach_*-Aufrufen, VOR dem ersten CPU-Zyklus -- prueft die
       vollstaendige Registry auf echte Adressueberlappungen (s. Funktionskommentar oben). */
    if (q9_board_validate_no_overlap() != 0) {
        return 1;
    }
    {
        /* Optionaler Port-Override (5.29-Diagnose, Andreas' laufender Terminalserver belegt
           sonst 2001/2000) -- Default bleibt unveraendert 2001/2000, analog Q9_NETTTY_PORT. */
        unsigned vb_tcp_port = Q9_VIDEOBRIDGE_TCP_PORT;
        unsigned vb_udp_port = Q9_VIDEOBRIDGE_UDP_PORT;
        const char *vb_tcp_env = getenv("Q9_VIDEOBRIDGE_TCP_PORT");
        const char *vb_udp_env = getenv("Q9_VIDEOBRIDGE_UDP_PORT");
        if (vb_tcp_env && vb_tcp_env[0]) vb_tcp_port = (unsigned)atoi(vb_tcp_env);
        if (vb_udp_env && vb_udp_env[0]) vb_udp_port = (unsigned)atoi(vb_udp_env);
        if (q9_videobridge_init(&videobridge, &fb, &crtc, &clut, vb_tcp_port, vb_udp_port, "Q9Flux") != 0) {
            fprintf(stderr, "q9board: Q9-Frame-Video-Bridge (TCP %u/UDP %u) konnte nicht gestartet werden -- "
                             "Emulation laeuft trotzdem weiter (rein additiver Host-Dienst).\r\n",
                    vb_tcp_port, vb_udp_port);
        }
    }
    cpu.reset(cpu.ctx);                                 /* Reset-Vektoren kommen aus dem ROM      */

    {
        /* Q9_BOARD_DEBUG=1 in der Umgebung: alle ~3s CPU-Zustand auf stderr (PC/SR/IACK-Zaehler
           + Board-Zustand) — das Werkzeug, mit dem der erste OS-9-Boot durchdebuggt wurde. */
        int      dbg         = getenv("Q9_BOARD_DEBUG") != 0;
        uint32_t last_dbg_ms = q9_hal_ticks_ms();

        for (;;) {
            int      irq;
            uint32_t now_ms;

            cpu.execute(cpu.ctx, BOARD_SLICE_CYCLES);
            q9_hal_con_flush();                             /* 5.7: TX-Rest aus vorherigen Runden   */
            if (q9_dbg_dump_requested) {                    /* Debug-Sondertaste, s. q9boardrun.h     */
                q9_dbg_dump_requested = 0;
                dbg_dump_kernel_globals(&board);
            }
            now_ms = q9_hal_ticks_ms();

            /* 5.17: Hauptschleifen-Poll -- die frueher hier hartkodierten Bloecke (DUART/QUICC/
               Timer je einzeln verdrahtet) sind vollstaendig durch EINE Schleife ueber die
               Geraete-Registry ersetzt: erst poll() (falls vorhanden), danach irq_pending()
               unmittelbar im Anschluss (wichtig fuer den Timer -- s. q9board.c timer_dev_poll/
               timer_dev_irq_pending: der Merker gilt nur fuer GENAU diese Runde).
               WICHTIG fuer die Reihenfolge: cpu.set_irq() (6.5: Vtable-Wrapper um
               q9_m68krt_set_irq()) bildet nur EINE kombinierte Leitung nach (kein Bus mit
               unabhaengigen Level-Leitungen, s. m68krt.c-Kommentar bei
               m68krt_reassert_pending_irq) -- der LETZTE Aufruf in dieser Runde gewinnt. Die
               Registrierungsreihenfolge (m68krt.c: DUART 3, Netz-Terminals 4, QUICC 5, Timer 6 --
               s. Kommentare in q9_m68krt_attach_board/attach_quicc) ist deshalb bewusst
               aufsteigend nach IRQ-Level gehalten, damit bei gleichzeitig anstehenden Interrupts
               am Ende dieser Schleife das hoechste Level uebrig bleibt -- genau wie vor 5.17. */
            irq = 0;
            {
                int i, n = q9_devreg_count();
                for (i = 0; i < n; i++) {
                    q9_device_t *d = q9_devreg_get(i);
                    q9_device_poll(d, now_ms);
                    if (q9_device_irq_pending(d)) {
                        cpu.set_irq(cpu.ctx, d->irq_level);
                        irq = 1;
                    }
                }
            }

            q9_videobridge_poll(&videobridge, now_ms);      /* 5.27: nichtblockierend, s. videobridge.h */

            /* 5.9: OS-9 idlet per STOP -- m68k_execute() "verbrennt" dann sofort alle
               angeforderten Takte, ohne etwas zu tun (busy loop, 100% Host-CPU). Ohne anstehenden
               IRQ kann in dieser Zeit nichts passieren, bevor der naechste 10ms-Timer-Tick (oder
               ein DUART-Interrupt) die CPU sowieso weckt -- also kurz schlafen statt sofort
               weiterzudrehen. q9_hal_ticks_ms() bleibt Wanduhr-basiert, die OS-9-Uhr geht also
               nicht falsch. */
            if (!irq && cpu.is_stopped(cpu.ctx)) {
                q9_hal_sleep_ms(1);
            }

            if (dbg && now_ms - last_dbg_ms >= 3000u) {
                uint32_t pc, sr, acks;
                q9_m68krt_debug_state(&pc, &sr, &acks);
                fprintf(stderr, "\n[dbg pc=%08x sr=%04x acks=%u imr=%02x rxfifo=%u rxovf=%u timer=%d]\n",
                        pc, sr, acks, board.uart_imr, board.uart_rx_count,
                        board.uart_rx_overflow, board.timer_active);
                /* 5.15-TCP-Haenger-Diagnose: RXF steigt (ACKs treffen ein), aber qack friert ein
                   -> Level-5-Interrupt wird nicht zugestellt (Emulator). qack steigt weiter, Gast
                   haengt trotzdem -> Bug sitzt im geschlossenen Gast-Treiber. bsy>0 -> RX-Ring lief
                   voll und Frames wurden STILL (ohne sonstiges Log) verworfen. */
                fprintf(stderr, "[quiccdiag rxf=%u bsy=%u txb=%u qack=%u rxfull=%d pending=%d scce=%04x sccm=%04x]\n",
                        quicc.diag_rxf, quicc.diag_bsy, quicc.diag_txb,
                        q9_m68krt_quicc_acks(), q9_quicc_rx_filled(&quicc),
                        q9_quicc_irq_pending(&quicc),
                        q9_quicc_read16(&quicc, 0xFFFF2000u + 0x1610u),
                        q9_quicc_read16(&quicc, 0xFFFF2000u + 0x1614u));
                last_dbg_ms = now_ms;
            }
        }
    }
}

//────────────────────────────────────────────────────────────────────────────────────────────────
// EOF q9boardrun.c                                                                          Ver. 1.90
//────────────────────────────────────────────────────────────────────────────────────────────────
