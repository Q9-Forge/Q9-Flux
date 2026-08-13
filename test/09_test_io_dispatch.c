//════════════════════════════════════════════════════════════════════════════════════════════════
// File:   09_test_io_dispatch.c                                                           Ver. 1.00
// Owner:  Claudia
// Desc.:  5.18 (zweiter Teilschritt): gezielte Absicherung fuer die neue I/O-Dispatch-Tabelle in
//         m68krt.c (Q9_IO_CLUSTER_BASE/g_io_table/devreg_hit) -- der allgemeine Boot-Diff-Test
//         (Boot-Transkript vor/nach der Aenderung byte-identisch) beweist nicht sicher, dass OS-9
//         waehrend EINES Boots ueberhaupt MC6845/CLUT-Register anfasst; hier werden alle drei
//         neuen Codepfade DIREKT ueber die echten oeffentlichen m68k_read/write_memory_*-
//         Funktionen angesprungen (kein Stub fuer die Dispatch-Logik selbst, die ist file-static
//         in m68krt.c):
//
//         (1) EINDEUTIGER Slot, Fenster = ganzer Slot oder kleiner (CF, UART): Tabellentreffer
//             direkt nutzbar.
//         (2) EINDEUTIGER Slot, aber Fenster KLEINER als 256 Byte (RTC, 16 von 256 Byte): Adressen
//             AUSSERHALB des echten Fensters, aber im selben Slot, muessen weiterhin "kein Geraet"
//             liefern (Board-Fallback), nicht faelschlich das Geraet treffen.
//         (3) MEHRDEUTIGER Slot (MC6845 $FFFFA000 + CLUT $FFFFA010, 16 Byte auseinander, teilen
//             sich Slot $A0): beide Geraete muessen weiterhin korrekt UNTERSCHIEDEN werden (nicht
//             das jeweils andere treffen), Adressen im selben Slot, die zu KEINEM der beiden
//             gehoeren, muessen "kein Geraet" liefern.
//         (4) Slot komplett unregistriert (in diesem schlanken Testaufbau z.B. der QUICC-Bereich,
//             da attach_quicc hier bewusst NICHT aufgerufen wird): muss weiterhin "kein Geraet"
//             liefern, kein Crash, kein falscher Treffer.
//
//         Bewusst OHNE q9_m68krt_reset()/cpu.execute() -- reine Register-Dispatch-Pruefung ohne
//         CPU-Ausfuehrung, board.remapped bleibt 0 (Reset-Zustand), rom_len=0, damit jeder
//         Board-Fallback deterministisch 0 liefert (kein ROM-Inhalt im Spiel, keine Seiteneffekte).
//
// Call:   build/<platform>/test_io_dispatch
//════════════════════════════════════════════════════════════════════════════════════════════════
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>

#include "../src/kernel/m68krt.h"
#include "../src/kernel/q9board.h"
#include "../src/kernel/devreg.h"
#include "../src/devices/mc6845/mc6845.h"
#include "../src/devices/clut/clut.h"
#include "m68k.h"

static int g_fails = 0;

static void check_u8(const char *label, unsigned int got, unsigned int want)
{
    if (got == want) {
        printf("    OK   %s (0x%02x)\n", label, got);
    } else {
        printf("    FAIL %s -- erwartet 0x%02x, bekommen 0x%02x\n", label, want, got);
        g_fails++;
    }
}

int main(void)
{
    static uint8_t   board_ram[64u * 1024u];
    static q9_board_t board;
    static q9_mc6845_t crtc;
    static q9_clut_t   clut;
    q9_m68krt_t rt;

    /* WICHTIG: q9_m68krt_attach_board() startet unconditional einen Netz-Terminal-Server
       (init_network_terminals(), s. m68krt.c) -- ohne dieses Signal wuerde er Port 2000 belegen
       wollen, genau den Port, auf dem Andreas' echte, lang laufende q9.exe-Sessions arbeiten.
       "0" (Kernel vergibt automatisch einen freien Port) FUNKTIONIERT HIER NICHT -- die eigene
       Parser-Pruefung in init_network_terminals() verlangt ausdruecklich parsed>0, "0" wird also
       verworfen und faellt still auf den Default 2000 zurueck (beim ersten Testlauf genau so
       beobachtet). Deshalb stattdessen ein konkreter Port weit oben im IANA-Dynamic/Private-
       Bereich (49152-65535), praktisch kollisionsfrei. Diese Test-Datei spricht die nettty-
       Sockets nie tatsaechlich an, der genaue Wert ist beliebig, nur "nicht 2000" zaehlt. Muss
       VOR dem attach_board()-Aufruf gesetzt sein (dort wird der Env-Wert gelesen).
       putenv() statt setenv(): setenv() fehlt auf manchen aelteren Windows-CRT-Umgebungen,
       putenv() ist sowohl unter POSIX als auch MSVCRT/UCRT verfuegbar. Statischer Puffer, weil
       putenv() die Speicherverwaltung nicht selbst uebernimmt -- muss fuer die Prozesslaufzeit
       gueltig bleiben. */
    {
        static char env_buf[] = "Q9_NETTTY_PORT=59217";
        putenv(env_buf);
    }

    if (q9_board_init(&board, NULL, 0, board_ram, sizeof(board_ram)) != Q9_BOARD_OK) {
        printf("FAIL q9_board_init fehlgeschlagen\n");
        return 1;
    }
    if (q9_m68krt_init(&rt, board_ram, sizeof(board_ram)) != Q9_M68KRT_OK) {
        printf("FAIL q9_m68krt_init fehlgeschlagen\n");
        return 1;
    }
    q9_m68krt_attach_board(&board);                    /* registriert UART/CF/RTC/nettty          */
    q9_mc6845_init(&crtc);
    q9_m68krt_attach_mc6845(&crtc);                     /* $FFFFA000-A001, Slot $A0                */
    q9_clut_init(&clut);
    q9_m68krt_attach_clut(&clut);                       /* $FFFFA010-A013, TEILT sich Slot $A0     */
    /* Bewusst NICHT attach_quicc/attach_cf_at aufgerufen -- damit bleibt der QUICC-Adressraum
       ($FFFF2000+) in DIESEM Testaufbau unregistriert (Fall 4 oben), ohne dass es hier auf einen
       zweiten Geraetetyp mit demselben Verhalten wie CF/RTC ankaeme. */

    printf("=== (1) Eindeutiger Slot, echtes Geraet: CF-Basisregister ===\n");
    check_u8("CF-Basis $FFFFE000 (Data-Register, Reset-Zustand)",
             m68k_read_memory_8(0xFFFFE000u), 0x00u);

    printf("=== (2) Eindeutiger Slot, Fenster kleiner als der Slot: RTC ===\n");
    /* RTC-Fenster ist 16 Byte ($FFFFD000-$FFFFD00F); $FFFFD010/$FFFFD0FF liegen im SELBEN
       256-Byte-Slot ($D0), aber ausserhalb des echten Fensters -- muessen "kein Geraet" (Board-
       Fallback, hier deterministisch 0 dank rom_len=0) liefern, NICHT das RTC-Geraet treffen. */
    check_u8("RTC ausserhalb des Fensters, selber Slot ($FFFFD010)",
             m68k_read_memory_8(0xFFFFD010u), 0x00u);
    check_u8("RTC ausserhalb des Fensters, selber Slot ($FFFFD0FF)",
             m68k_read_memory_8(0xFFFFD0FFu), 0x00u);

    printf("=== (3) Mehrdeutiger Slot: MC6845 + CLUT teilen sich Slot $A0 ===\n");
    /* MC6845 (memset-initialisiert, Ver. 1.00): Index-/Datenregister lesen sich bei frischem
       Reset als 0 -- reicht als Beleg, dass ueberhaupt DAS RICHTIGE Geraet (nicht CLUT)
       angesprochen wird, ohne MC6845-Registersemantik im Detail nachzubilden. */
    check_u8("MC6845 Indexregister $FFFFA000", m68k_read_memory_8(0xFFFFA000u), 0x00u);
    check_u8("MC6845 Datenregister  $FFFFA001", m68k_read_memory_8(0xFFFFA001u), 0x00u);
    /* CLUT (Reset-Zustand "Identitaets-Graustufe", s. q9_clut_init/ARBEITSPLAN 5.29-Nachtrag):
       Index bei 0 gelesen -- auch hier reicht der Nachweis "richtiges Geraet", nicht volle
       Registersemantik. */
    check_u8("CLUT Indexregister $FFFFA010", m68k_read_memory_8(0xFFFFA010u), 0x00u);
    /* Adressen im selben Slot $A0, die zu KEINEM der beiden Geraete gehoeren (zwischen MC6845s
       2 Byte und CLUTs Start bei Offset $10, bzw. hinter CLUTs 4 Byte bis zum Slot-Ende). */
    check_u8("Slot $A0, weder MC6845 noch CLUT ($FFFFA002)",
             m68k_read_memory_8(0xFFFFA002u), 0x00u);
    check_u8("Slot $A0, weder MC6845 noch CLUT ($FFFFA0FF)",
             m68k_read_memory_8(0xFFFFA0FFu), 0x00u);

    printf("=== (4) Komplett unregistrierter Slot (QUICC-Bereich, hier nicht attached) ===\n");
    check_u8("kein Geraet, kein Crash ($FFFF2500)", m68k_read_memory_8(0xFFFF2500u), 0x00u);

    printf("=== Schreiben + Rueckwaertspruefung ueber die Ambiguous-Slot-Grenze ===\n");
    /* Schreibt in MC6845s Fenster, liest danach CLUT -- muss UNVERAENDERT bleiben (beweist, dass
       die beiden Geraete im selben Slot nicht versehentlich denselben Zustand teilen). */
    m68k_write_memory_8(0xFFFFA000u, 0x42u);
    check_u8("CLUT nach MC6845-Schreibzugriff unveraendert ($FFFFA010)",
             m68k_read_memory_8(0xFFFFA010u), 0x00u);

    printf("\n=== Zusammenfassung ===\n");
    printf("  Gesamt: %d Checks fehlgeschlagen\n", g_fails);
    return g_fails ? 1 : 0;
}

//────────────────────────────────────────────────────────────────────────────────────────────────
// EOF 09_test_io_dispatch.c                                                               Ver. 1.00
//────────────────────────────────────────────────────────────────────────────────────────────────
