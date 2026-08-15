//════════════════════════════════════════════════════════════════════════════════════════════════
// File:   m68krt.h                                                                        Ver. 1.31
// Owner:  AF
// Desc.:  Schmaler Q9-Wrapper um die eingebettete Musashi-68000-Emulation (third_party/musashi,
//         Entscheidung E12 in PROJECT.md). Native-Build-only — Grundbaustein fuer Phase 5 (Prozesse
//         mit echtem 68k-Maschinencode, vgl. den wasm3-Zweig aus Phase 4/wasmrt.h). Schritt 5.1:
//         nur Laden + Ausfuehren in emuliertem RAM + Register lesen, KEINE Scheduler-/
//         Syscall-Bridge-Entscheidungen (die kommen erst mit der Detailplanung von Phase 5).
//
//         Musashi haelt seinen kompletten CPU-Zustand in eigenen globalen Variablen (kein Kontext-
//         Zeiger in m68k_read/write_memory_*, anders als wasm3s IM3Runtime-Handles) — es kann daher
//         je Prozess immer nur EINE Musashi-Instanz aktiv sein. q9_m68krt_t ist deshalb bewusst kein
//         eigenstaendiges, mehrfach instanzierbares Handle wie q9_wasmrt_t, sondern nur eine duenne
//         Buchhaltungs-Struktur (RAM-Zeiger+Groesse) neben den Musashi-eigenen Globals.
//
// Call:   q9_m68krt_t rt; q9_m68krt_init(&rt, ram, sizeof(ram));
//         q9_m68krt_reset(&rt); q9_m68krt_execute(&rt, 100);
//         d0 = q9_m68krt_get_d(&rt, 0); q9_m68krt_free(&rt);
//
// Edition History
//─────────┬──────┬────────────────────────────────────────────────────────────────────────┬──────
// Date    │ Ver. │ Description                                                            │ By
//─────────┼──────┼────────────────────────────────────────────────────────────────────────┼──────
// 26-07-04│ 1.00 │ 5.1: Erster Grundbaustein — RAM anbinden, Reset+Execute, D0-D7 lesen    │ CF
// 26-07-04│ 1.10 │ 5.2d: q9_m68krt_set_irq — duenner Wrapper um m68k_set_irq() fuer         │ CF
//         │      │ q9board.c's Timer/IRQ3-Polling                                            │
// 26-07-05│ 1.20 │ 5.3: q9_m68krt_attach_board — Speicherzugriffe wahlweise ueber den       │ CF
//         │      │ Board-Adress-Dispatch (q9board.h) statt nacktem RAM-Block                 │
// 26-07-10│ 1.30 │ 5.9: q9_m68krt_is_stopped — Wrapper um Musashis m68k_is_stopped()        │ CF
//         │      │ (Vendor-Patch) fuer die Idle-Drossel                              │
// 26-08-03│ 1.21 │ 5.26: q9_m68krt_attach_framebuf (framebuf.h, VRAM-Geraet)               │ Ada
// 26-08-13│ 1.31 │ 6.5: q9_m68krt_get_backend -- befuellt cpu_backend.h's q9_cpu_backend_t   │ Cld
//         │      │ mit reset/execute/set_irq/is_stopped-Wrappern                           │
// 26-08-15│ 1.32 │ Q9FLUX_EDITOR_de.md 4.1: q9_cpu_type_t + neuer cpu-Parameter fuer         │ Cld
//         │      │ q9_m68krt_init -- macht die bisher per Q9_CPU=ec030-Env-Var versteckte    │
//         │      │ CPU-Typ-Wahl zu einem echten, konfigurierbaren Aufrufparameter (Musashis   │
//         │      │ M68K_CPU_TYPE_*-Enum bleibt intern in m68krt.c, wie bei allen anderen       │
//         │      │ Wrappern hier -- kein Musashi-Header-Leck nach aussen)                     │
//═════════╧══════╧════════════════════════════════════════════════════════════════════════╧══════
#ifndef Q9_M68KRT_H
#define Q9_M68KRT_H

#include <stdint.h>
#include "q9board.h"
#include "cpu_backend.h"                                /* 6.5: q9_cpu_backend_t                   */
#include "../devices/mc6845/mc6845.h"                  /* 5.24: q9_mc6845_t                       */
#include "../devices/framebuf/framebuf.h"              /* 5.26: q9_framebuf_t                     */
#include "../devices/clut/clut.h"                      /* 5.29-Nachtrag: q9_clut_t                 */

#define Q9_M68KRT_OK        0
#define Q9_M68KRT_ERR_RAM  -1                       /* RAM fehlt oder zu klein fuer Reset-Vektoren */

/* Buchhaltung neben Musashis eigenen Globals (s.o.) — kein Handle im Sinne von q9_wasmrt_t. */
typedef struct q9_m68krt {
    uint8_t  *ram;                                   /* Emuliertes RAM, big-endian (68k-Byteorder) */
    uint32_t  ram_len;
} q9_m68krt_t;

/* Q9FLUX_EDITOR_de.md 4.1: Q9-seitige Sicht auf Musashis M68K_CPU_TYPE_*-Enum (third_party/musashi/
   m68k.h) -- absichtlich eine eigene, kleinere Aufzaehlung statt den Musashi-Header hier
   einzubinden (m68krt.h bindet third_party/musashi bewusst NIRGENDS ein, s. Typkommentar oben).
   Q9_CPU_68030 = 0 ist der Default (= echte Q9-Hardware, Entscheidung E12) -- ein vergessenes/
   nicht gesetztes Feld verhaelt sich damit automatisch richtig (memset-Nullwert). SCC68070 aus
   Musashis Enum bewusst NICHT aufgenommen (kein 680x0, andere CPU-Familie, fuer Q9 irrelevant). */
typedef enum {
    Q9_CPU_68030 = 0,                                 /* Default -- echte Q9-Hardware              */
    Q9_CPU_68000,
    Q9_CPU_68010,
    Q9_CPU_68EC020,
    Q9_CPU_68020,
    Q9_CPU_68EC030,
    Q9_CPU_68EC040,
    Q9_CPU_68LC040,
    Q9_CPU_68040
} q9_cpu_type_t;

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_m68krt_init
// Desc.:    Bindet einen RAM-Block als Speicher der Musashi-Instanz an (m68k_read/write_memory_*
//           in m68krt.c greifen darauf zu) und legt den CPU-Typ fest. cpu = Q9_CPU_68030 fuer den
//           bisherigen Default (echte Q9-Hardware, Entscheidung E12); andere Werte sind bewusst
//           erlaubt, auch wenn nicht jede Wahl das echte OS-9-Image erfolgreich bootet (OS-9/68030
//           erwartet eine PMMU, die z.B. 68000/68010 gar nicht haben) -- keine neue Einschraenkung,
//           s. Q9FLUX_EDITOR_de.md 4.1. ram_len muss mindestens 8 Byte sein (Reset-Vektoren: SP bei
//           Adresse 0, PC bei Adresse 4, je 4 Byte big-endian) — der Aufrufer traegt die Vektoren +
//           das Programm selbst ein, BEVOR q9_m68krt_reset() aufgerufen wird.
// Call:     err = q9_m68krt_init(&rt, ram, sizeof(ram), Q9_CPU_68030)
//════════════════════════════════════════════════════════════════════════════════════════════════
int q9_m68krt_init(q9_m68krt_t *rt, uint8_t *ram, uint32_t ram_len, q9_cpu_type_t cpu);

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_m68krt_reset
// Desc.:    Liest die Reset-Vektoren aus dem RAM (Adresse 0 = initialer SP, Adresse 4 = initialer
//           PC) und setzt die CPU auf diesen Zustand zurueck (m68k_pulse_reset()).
// Call:     q9_m68krt_reset(&rt)
//════════════════════════════════════════════════════════════════════════════════════════════════
void q9_m68krt_reset(q9_m68krt_t *rt);

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_m68krt_execute
// Desc.:    Fuehrt bis zu 'cycles' CPU-Takte aus (m68k_execute()). Liefert die tatsaechlich
//           verbrauchten Takte zurueck (kann wegen unvollstaendiger letzter Instruktion leicht
//           abweichen).
// Call:     spent = q9_m68krt_execute(&rt, 100)
//════════════════════════════════════════════════════════════════════════════════════════════════
int q9_m68krt_execute(q9_m68krt_t *rt, int cycles);

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_m68krt_get_d
// Desc.:    Liefert den Inhalt von Datenregister Dn (n = 0..7) — fuer den Rauchtest (5.1) reicht
//           das; PC/SR/Ax folgen erst, wenn die Syscall-Bridge sie tatsaechlich braucht.
// Call:     v = q9_m68krt_get_d(&rt, 0)
//════════════════════════════════════════════════════════════════════════════════════════════════
uint32_t q9_m68krt_get_d(q9_m68krt_t *rt, int n);

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_m68krt_free
// Desc.:    Loest die RAM-Bindung wieder (Musashis eigene Globals bleiben bestehen, s.o. — ein
//           erneutes q9_m68krt_init() mit neuem RAM ist trotzdem sauber moeglich, weil jeder
//           Aufruf m68k_init()+m68k_set_cpu_type() neu ausfuehrt).
// Call:     q9_m68krt_free(&rt)
//════════════════════════════════════════════════════════════════════════════════════════════════
void q9_m68krt_free(q9_m68krt_t *rt);

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_m68krt_set_irq
// Desc.:    5.2d: Duenner Wrapper um Musashis m68k_set_irq(level) — die eigentliche Interrupt-
//           Mechanik (PC+SR auf den Supervisor-Stack, Vektor holen, springen) macht Musashi
//           vollstaendig selbst; dieser Wrapper existiert nur, damit q9board.c (das Musashi bewusst
//           nicht kennt, s. q9board.h) nicht direkt gegen third_party/musashi linken muss. level = 0
//           loescht die Interrupt-Anforderung wieder (Musashi-Konvention).
// Call:     q9_m68krt_set_irq(3)
//════════════════════════════════════════════════════════════════════════════════════════════════
void q9_m68krt_set_irq(int level);

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_m68krt_attach_board
// Desc.:    5.3: Schaltet die sechs Musashi-Speicher-Hooks auf den Board-Adress-Dispatch um —
//           ALLE CPU-Zugriffe (auch das Holen der Reset-Vektoren durch q9_m68krt_reset) laufen
//           dann ueber q9_board_read/write8/16/32 (RAM/ROM/Remap/UART/CF/Timer) statt ueber den
//           nackten RAM-Block aus q9_m68krt_init. Setzt ausserdem den Interrupt-Acknowledge-
//           Callback auf Autovector-Betrieb mit Puls-Verhalten (IRQ-Leitung wird beim Annehmen
//           losgelassen, s. m68krt.c). board = NULL schaltet zurueck in den RAM-Modus (5.1);
//           q9_m68krt_init/free setzen ebenfalls auf RAM-Modus zurueck. Aufruf NACH
//           q9_m68krt_init und VOR q9_m68krt_reset.
// Call:     q9_m68krt_attach_board(&board);  ...  q9_m68krt_attach_board(0);
//════════════════════════════════════════════════════════════════════════════════════════════════
void q9_m68krt_attach_board(q9_board_t *board);

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_m68krt_attach_quicc
// Desc.:    5.11: Haengt die QUICC-Ethernet-Emulation (quicc.h) in den Adress-Dispatch ein —
//           Zugriffe auf das Fenster $FFFF2000-$FFFF3FFF gehen dann an q9_quicc_read/write*,
//           und der Interrupt-Acknowledge liefert fuer Level 5 den QUICC-Vektor (254), solange
//           der QUICC einen Interrupt anfordert. quicc = NULL haengt das Fenster wieder aus;
//           q9_m68krt_free setzt ebenfalls zurueck. Aufruf NACH q9_m68krt_attach_board.
// Call:     q9_m68krt_attach_quicc(&quicc);  ...  q9_m68krt_attach_quicc(0);
//════════════════════════════════════════════════════════════════════════════════════════════════
struct q9_quicc;
void q9_m68krt_attach_quicc(struct q9_quicc *quicc);

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_m68krt_attach_mc6845
// Desc.:    5.24: Haengt die MC6845-CRT-Controller-Emulation (mc6845.h) in die Geraete-Registry ein
//           -- Fenster $FFFFA000-$FFFFA001 (Index/Datenregister). Kein IRQ. Aufruf NACH
//           q9_m68krt_attach_board (analog QUICC/CF2). crtc muss die gesamte Laufzeit ueberleben.
// Call:     q9_m68krt_attach_mc6845(&crtc);
//════════════════════════════════════════════════════════════════════════════════════════════════
void q9_m68krt_attach_mc6845(q9_mc6845_t *crtc);

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_m68krt_attach_framebuf
// Desc.:    5.26: Haengt das VRAM-Geraet (framebuf.h) in die Geraete-Registry ein -- Fenster ab
//           Q9_FRAMEBUF_BASE, Groesse kommt aus fb->size (q9_framebuf_size), also NACH
//           q9_framebuf_init() aufrufen. Kein IRQ. fb muss die gesamte Laufzeit ueberleben.
// Call:     q9_m68krt_attach_framebuf(&fb);
//════════════════════════════════════════════════════════════════════════════════════════════════
void q9_m68krt_attach_framebuf(q9_framebuf_t *fb);

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_m68krt_attach_clut
// Desc.:    5.29-Nachtrag: Haengt das CLUT-Geraet (clut.h) in die Geraete-Registry ein -- Fenster
//           $FFFFA010-$FFFFA013 (Index + R/G/B). Kein IRQ. Aufruf NACH q9_m68krt_attach_board.
//           clut muss die gesamte Laufzeit ueberleben.
// Call:     q9_m68krt_attach_clut(&clut);
//════════════════════════════════════════════════════════════════════════════════════════════════
void q9_m68krt_attach_clut(q9_clut_t *clut);

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_m68krt_attach_cf2
// Desc.:    5.19a: Registriert ein ZWEITES Compact-Flash-Interface (RC2014-SC145-Kartenleser bei
//           Q9_BOARD_CF2_BASE, Descriptoren e0/f0 im MWOS-Q9-Port) in der Geraete-Registry. Nutzt
//           dieselbe q9_devtype_cf-Vtable wie die Onboard-CF, nur mit eigener q9_cf_t-Instanz und
//           eigener Basisadresse. Kein IRQ (wie die Onboard-CF). Nur aufrufen, wenn die Board-
//           Config dort Images anhaengt — ohne Aufruf existiert das Fenster nicht (Board wie 5.17).
//           cf2 muss die gesamte Lauf­zeit ueberleben (wird nur als Zeiger gehalten).
// Call:     q9_m68krt_attach_cf2(&cf2);
//════════════════════════════════════════════════════════════════════════════════════════════════
void q9_m68krt_attach_cf2(q9_cf_t *cf2);
void q9_m68krt_attach_cf_at(q9_cf_t *cf, uint32_t base, const char *name);

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_m68krt_debug_state
// Desc.:    Diagnose (5.4): aktueller PC + SR der emulierten CPU und Anzahl der bisher
//           durchlaufenen Interrupt-Acknowledge-Zyklen — fuer die Boot-Fehlersuche im Runner
//           (wo haengt die CPU, kommt der Interrupt ueberhaupt an, wie steht die IRQ-Maske).
// Call:     q9_m68krt_debug_state(&pc, &sr, &acks)
//════════════════════════════════════════════════════════════════════════════════════════════════
void q9_m68krt_debug_state(uint32_t *pc, uint32_t *sr, uint32_t *acks);

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_m68krt_quicc_acks
// Desc.:    5.15-Diagnose (TCP-Haenger): Anzahl der bisher tatsaechlich an die CPU zugestellten
//           QUICC-Interrupts (Level 5). Waechst dieser Zaehler waehrend eines Haengers NICHT
//           weiter, obwohl die QUICC-RXF-Zaehler (quicc.c diag_rxf) steigen, wird der Hardware-
//           Interrupt nicht zugestellt -> Emulator-Bug. Waechst er weiter -> ISR laeuft, der
//           Stillstand sitzt im (geschlossenen) Gast-Treiber.
//════════════════════════════════════════════════════════════════════════════════════════════════
uint32_t q9_m68krt_quicc_acks(void);

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_m68krt_is_stopped
// Desc.:    5.9: Duenner Wrapper um Musashis m68k_is_stopped() (Vendor-Patch, s.
//           third_party/musashi/Q9_VENDOR.md) — 1 wenn die CPU per STOP-Instruktion angehalten
//           ist (z.B. OS-9s Idle-Loop), sonst 0. Grundlage fuer die Host-Idle-Drossel im
//           Board-Runner (q9boardrun.c): wenn gestoppt UND kein IRQ anliegt, kann der Host
//           kurz schlafen statt den Slice sofort wieder "leer" zu verbrennen.
// Call:     if (q9_m68krt_is_stopped()) ...
//════════════════════════════════════════════════════════════════════════════════════════════════
int q9_m68krt_is_stopped(void);

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_m68krt_get_backend
// Desc.:    6.5: Befuellt eine q9_cpu_backend_t (cpu_backend.h) mit Wrapper-Funktionen um
//           q9_m68krt_reset/execute/set_irq/is_stopped -- ab jetzt der einzige Weg, wie
//           q9boardrun.c die 68k-CPU antreibt (statt die vier Funktionen einzeln beim Namen zu
//           kennen). backend->ctx zeigt auf rt; die Wrapper selbst reichen rt grossteils gar nicht
//           weiter, weil Musashi ohnehin ein Singleton mit eigenen Globals ist (s. Typkommentar
//           oben) -- das ist reine Anpassung an die generische Vtable-Signatur, kein neuer
//           Zustand. rt muss die gesamte Laufzeit des Backends ueberleben.
// Call:     q9_cpu_backend_t cpu; q9_m68krt_get_backend(&rt, &cpu);
//════════════════════════════════════════════════════════════════════════════════════════════════
void q9_m68krt_get_backend(q9_m68krt_t *rt, q9_cpu_backend_t *backend);

#endif // Q9_M68KRT_H

//────────────────────────────────────────────────────────────────────────────────────────────────
// EOF m68krt.h                                                                            Ver. 1.21
//────────────────────────────────────────────────────────────────────────────────────────────────
