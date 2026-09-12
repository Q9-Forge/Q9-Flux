//════════════════════════════════════════════════════════════════════════════════════════════════
// File:   plic.h                                                                          Ver. 1.00
// Owner:  Claudia
// Desc.:  PLIC (Platform-Level Interrupt Controller) -- verteilt externe Geraete-Interrupts
//         (hier: UART-RX) auf die CPU. Registerlage nach der QEMU-"virt"-Maschine, aus
//         Sekundaerquelle bestaetigt gegen den tatsaechlichen NuttX-Quelltext
//         (arch/risc-v/src/qemu-rv/hardware/qemu_rv_plic.h), nicht nur aus der Spezifikation
//         hergeleitet -- Basisadresse 0x0c000000, Registerlage wie unten.
//
//         BEWUSST ENDIAN-NEUTRAL und ohne Bindung an src/kernel/devreg.h -- gleiche Begruendung
//         wie bei clint.h/uart16550.h (Schritt 6.7 verallgemeinert das devreg-Schema erst spaeter).
//
//         Registerfenster (relativ zur Basisadresse, wie sie cpu_register_device() bekommt):
//           +0x000000 + 4*N   PRIORITY[N]     Prioritaet der Quelle N (0 = "nie", Quelle 0 unbenutzt)
//           +0x001000 + 4*w   PENDING[w]      lesend: welche Quellen (32 je Wort) gerade anliegen
//           +0x002000 + 0x80*ctx + 4*w   ENABLE[ctx][w]   welche Quellen fuer Kontext ctx freigegeben sind
//           +0x200000 + 0x1000*ctx       THRESHOLD[ctx]   nur Quellen MIT hoeherer Prioritaet zaehlen
//           +0x200004 + 0x1000*ctx       CLAIM/COMPLETE[ctx]  lesen = naechste Quelle uebernehmen,
//                                          schreiben derselben Nummer = abschliessen
//
//         ZWEI KONTEXTE vorgesehen (0 = Machine-Mode Hart 0, 1 = Supervisor-Mode Hart 0) --
//         Kontext 0 reicht fuer ein M-Mode-only-Gastsystem (z.B. NuttX "nsh"); Kontext 1 liegt
//         bereit fuer den Tag, an dem ein Gast mit S-Mode-MMU (NuttX "knsh32", s. PTE_A/PTE_D-
//         Vorarbeit) External-Interrupts im Supervisor-Modus braucht -- dieselbe Kontext-Distanz
//         (0x80 Byte fuer ENABLE, 0x1000 Byte fuer THRESHOLD/CLAIM) wie in der echten Hardware,
//         kein Nachbau bei Bedarf noetig.
//
//         PEGELGESTEUERT (level-triggered), wie eine echte 16550 am PLIC: eine Quelle bleibt
//         "anliegend", bis das GERAET selbst sie zuruecknimmt (hier: q9_plic_set_level(N, 0)),
//         nicht schon durch das Claim/Complete-Protokoll allein. Liest der Gast eine Quelle per
//         CLAIM, OHNE die Ursache beim Geraet zu beheben (bei UART: das wartende Zeichen
//         tatsaechlich abzuholen), bleibt die Quelle anspruchsberechtigt und wird beim naechsten
//         CLAIM sofort wieder geliefert -- genau das von der Spezifikation verlangte Verhalten,
//         kein Fehler dieses Nachbaus.
//
//         DIE SET-MIP-FALLE AUS clint.h GILT HIER GENAUSO: dieses Geraet kennt die CPU nicht
//         (bewusst, gleiche Trennung wie beim CLINT). Der Aufrufer muss nach JEDER Aenderung, die
//         q9_plic_context_pending() beeinflussen kann -- ein MMIO-Schreibzugriff auf dieses
//         Geraet UND jeder q9_plic_set_level()-Aufruf eines angeschlossenen Geraets --
//         riscv_cpu_set_mip(cpu, MIP_MEIP)/reset_mip() SOFORT nachziehen, nicht nur periodisch.
//         Sonst wiederholt sich der beim CLINT-Timer gefundene Interrupt-Sturm hier fuer externe
//         Interrupts.
//════════════════════════════════════════════════════════════════════════════════════════════════
#ifndef Q9_PLIC_H
#define Q9_PLIC_H

#include <stdint.h>

#define Q9_PLIC_SIZE          0x400000u   /* registriertes Adressfenster, deckt beide Kontexte */
#define Q9_PLIC_NUM_SOURCES   64          /* Index 0 unbenutzt, 1..63 verfuegbar (UART liegt bei 37) */
#define Q9_PLIC_NUM_CONTEXTS  2           /* 0 = M-Mode Hart0, 1 = S-Mode Hart0 */

typedef struct {
    uint32_t priority[Q9_PLIC_NUM_SOURCES];
    uint8_t  level[Q9_PLIC_NUM_SOURCES];        /* 0/1, vom Geraet gesetzt */
    uint32_t enable[Q9_PLIC_NUM_CONTEXTS][2];   /* je 2 Woerter = 64 Quellenbits */
    uint32_t threshold[Q9_PLIC_NUM_CONTEXTS];
    int      claimed[Q9_PLIC_NUM_CONTEXTS];     /* 0 = nichts uebernommen, sonst Quellennummer */
} q9_plic_t;

void     q9_plic_init(q9_plic_t *p);
uint32_t q9_plic_read32 (q9_plic_t *p, uint32_t offset);
void     q9_plic_write32(q9_plic_t *p, uint32_t offset, uint32_t val);

/* Vom angeschlossenen Geraet aufgerufen, wenn sich sein Anforderungszustand aendert
   (level=1: fordert an: level=0: zurueckgenommen). Nicht vom Wirt periodisch, sondern SOFORT
   bei jeder Zustandsaenderung -- siehe die Sturm-Warnung im Kopf. */
void     q9_plic_set_level(q9_plic_t *p, unsigned source, int level);

/* 1, wenn Kontext ctx gerade einen externen Interrupt sehen sollte (irgendeine freigegebene,
   ausreichend priorisierte, noch nicht uebernommene Quelle liegt an). Der Aufrufer bildet daraus
   riscv_cpu_set_mip()/reset_mip() mit MIP_MEIP (Kontext 0) bzw. MIP_SEIP (Kontext 1). */
int      q9_plic_context_pending(const q9_plic_t *p, unsigned ctx);

#endif /* Q9_PLIC_H */
// EOF plic.h                                                                               Ver. 1.00
