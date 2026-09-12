//════════════════════════════════════════════════════════════════════════════════════════════════
// File:   clint.h                                                                        Ver. 1.00
// Owner:  Claudia
// Desc.:  CLINT (Core-Local Interruptor) -- die Timer-/Software-Interruptquelle jeder RISC-V-
//         Maschine, so weit wie ein Gast fuer periodische Interrupts braucht. Registerlage und
//         Adressfenster nach der QEMU-"virt"-Maschine (Basis 0x02000000, Groesse 0x10000), aus
//         denselben Gruenden wie bei uart16550.h: das ist die De-facto-Standardmaschine der
//         RISC-V-Welt, und wer sie nachbildet, kann deren Bildersammlung benutzen und gegen
//         qemu-system-riscv gegenpruefen.
//
//         BEWUSST ENDIAN-NEUTRAL und ohne Bindung an src/kernel/devreg.h -- gleiche Begruendung
//         wie bei uart16550.h (dessen Vtable synthetisiert 16/32-Bit big-endian, fuer RISC-V
//         falsch herum; die Verallgemeinerung ist Schritt 6.7).
//
//         Registerfenster (nur ein Hart, wie unser Board):
//           +0x0000  msip       4 Byte, Bit 0 = Software-Interrupt anfordern (MIP_MSIP)
//           +0x4000  mtimecmp   8 Byte (zwei 32-Bit-Zugriffe -- der Kern kennt keine 64-Bit-MMIO,
//                                s. iomem.h DEVIO_SIZE64-Kommentar dort), low zuerst dann high
//           +0xbff8  mtime      8 Byte, nur LESEND (Schreiben wird angenommen und verworfen --
//                                echte Hardware erlaubt es, ein Gast braucht es fuer diesen
//                                Bring-up-Schritt nicht)
//
//         WICHTIGE VEREINFACHUNG, bewusst und hier dokumentiert: mtime laeuft NICHT nach realer
//         Wanduhrzeit, sondern nach AUSGEFUEHRTEN ZYKLEN (q9_clint_advance() wird vom Board pro
//         interp()-Abschnitt aufgerufen). Das macht Prueflaeufe deterministisch und
//         host-unabhaengig -- keine Zeitschwankungen zwischen macOS/Windows/Linux oder je nach
//         Rechnerlast. Fuer eine Emulation ohne echte Sekunden ist "Fortschritt pro Zyklus" eine
//         zulaessige und uebliche Vereinfachung; ein Gast, der aus mtime eine WANDUHRZEIT ableiten
//         will (statt nur "Zeit vergeht monoton"), wuerde dadurch falsche Werte sehen -- fuer den
//         Bring-up-Schritt hier ist das ohne Belang.
//
//         ZWEI FALLEN bei der Anbindung (nicht im Geraet selbst, sondern beim Aufrufer -- beide
//         beim Bau von test/rvboard_timer.c live gefunden, nicht nur befuerchtet):
//
//         1. riscv_cpu_interp() setzt EINMAL gesetzte power_down_flag (durch "wfi") nur zurueck,
//            wenn riscv_cpu_set_mip() aufgerufen wird -- ein blosses Veraendern des mip-Registers
//            "von aussen" gibt es nicht. Der Board-Treiber muss also nach jedem
//            q9_clint_advance() pruefen und bei Bedarf aktiv riscv_cpu_set_mip(cpu, MIP_MTIP)
//            rufen -- sonst haengt ein "wfi" fuer immer, obwohl die Zeit laengst abgelaufen ist.
//
//         2. Die GEGENRICHTUNG braucht denselben Abgleich, aber SOFORT statt periodisch: auf
//            echter Hardware ist mip.MTIP ein KOMBINATORISCHES Signal (mtime >= mtimecmp,
//            jederzeit aktuell); in diesem Kern dagegen ein Zwischenspeicher, den nur
//            set_mip()/reset_mip() aendern. Gleicht der Board-Treiber nur PERIODISCH ab (einmal
//            pro Abschnitt der Hauptschleife), bleibt MTIP nach einem Trap-Handler-Durchlauf bis
//            zum naechsten Abschnitt gesetzt -- der Kern prueft mip&mie aber vor JEDER Instruktion
//            neu und feuert den Handler in der Zwischenzeit wiederholt: ein Interrupt-Sturm.
//            Gemessen beim ersten Testlauf: 33 statt der erwarteten 5 Interrupts. Der Fix ist NICHT
//            ein kleinerer Abschnitt (verschiebt das Problem nur), sondern ein Abgleich SOFORT nach
//            jedem Schreibzugriff auf mtimecmp -- also im write-Rueckruf des Geraets selbst, dort
//            wo test/rvboard_timer.c auf den Kern zugreifen kann (dieses Geraet selbst kennt ihn
//            bewusst nicht, s. Kopf).
//════════════════════════════════════════════════════════════════════════════════════════════════
#ifndef Q9_CLINT_H
#define Q9_CLINT_H

#include <stdint.h>

#define Q9_CLINT_SIZE       0x10000u
#define Q9_CLINT_OFF_MSIP   0x0000u
#define Q9_CLINT_OFF_MTIMECMP 0x4000u
#define Q9_CLINT_OFF_MTIME  0xbff8u

typedef struct {
    uint64_t mtime;
    uint64_t mtimecmp;
    uint32_t msip;
} q9_clint_t;

void     q9_clint_init(q9_clint_t *c);
uint32_t q9_clint_read32 (q9_clint_t *c, uint32_t offset);
void     q9_clint_write32(q9_clint_t *c, uint32_t offset, uint32_t val);

/* Vom Board pro interp()-Abschnitt aufgerufen -- treibt mtime voran (s. Kopf: zyklenbasiert). */
void     q9_clint_advance(q9_clint_t *c, uint64_t cycles);

/* mtime >= mtimecmp? Der Aufrufer muss darauf riscv_cpu_set_mip()/reset_mip() folgen lassen,
   s. Falle im Kopf -- dieses Geraet ruft den Kern nicht selbst auf (kennt ihn nicht). */
int      q9_clint_timer_pending(const q9_clint_t *c);

#endif /* Q9_CLINT_H */
// EOF clint.h                                                                             Ver. 1.00
