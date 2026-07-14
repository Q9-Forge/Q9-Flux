//════════════════════════════════════════════════════════════════════════════════════════════════
// File:   devreg.h                                                                        Ver. 1.00
// Owner:  AF
// Desc.:  5.17: Geraete-Interface + Registry fuer die Board-Hardware im emulierten 68k-Adressraum
//         (CB030-Runner, docs/CB030.md) — NICHT zu verwechseln mit device.c/device.h, das die
//         OS-9-SEITIGEN Pfad-Geraete (/term, /nil, /d0) fuer den wasm3-Kernelpfad modelliert. Hier
//         geht es um Host-emulierte Hardware am 68k-Bus (DUART, CF, Timer, RTC, Netz-Terminals,
//         QUICC-Ethernet), die bisher als sechs hartkodierte if/switch-Ketten an drei Stellen
//         (m68krt.c Speicher-Dispatch, cb030run.c Hauptschleifen-Poll, m68krt.c IRQ-Ack/Reassert)
//         verdrahtet waren (s. ARBEITSPLAN 5.17).
//
//         q9_device_t buendelt Adressfenster (base/size), IRQ-Zuordnung (level/vector, vector=-1
//         heisst Autovektor) und eine Vtable mit den Zugriffsfunktionen. read8/write8 sind Pflicht;
//         read16/32 und write16/32 sind optional (NULL = wird aus den Byte-Zugriffen synthetisiert,
//         big-endian wie der 68k) — die einzige Ausnahme im Bestand ist Compact-Flash, das am
//         Datenregister eigene 16/32-Bit-Pfade braucht (ATA-Doppel-/Vierfach-Byte-Transfer, s.
//         cb030.c) und deshalb eigene read16/32/write16/32-Funktionen einsetzt. poll/irq_pending/
//         reset sind ebenfalls optional (NULL = Geraet braucht das nicht, z.B. CF hat kein IRQ).
//
//         Registry: EIN statisches Array von Geraete-INSTANZEN (q9_devreg_add/get/count), das die
//         drei genannten Stellen statt der alten Ketten durchlaufen. Instanzen werden weiterhin von
//         Hand angelegt (cb030run.c/m68krt.c) — das ist bewusst NICHT die "Typ-Registry": die Typ-
//         Registry (q9_devtype_lookup) bildet Typnamen ("duart68681", "cf", ...) auf ihre Vtable ab
//         und ist eine explizite, statisch kompilierte Tabelle (KEINE Linker-Magie wie
//         __attribute__((constructor)) — portabel, wasm-tauglich, im Projektstil). Sie liegt schon
//         bereit fuer 5.19 (Config-Datei instanziert Geraete ueber den Typnamen); 5.17 selbst nutzt
//         sie noch nicht zur Instanziierung.
//
// Call:   q9_device_t d = {0};
//         d.type = "duart68681"; d.name = "uart0"; d.base = Q9_CB030_UART_BASE;
//         d.size = Q9_CB030_UART_TOP - Q9_CB030_UART_BASE + 1; d.irq_level = 3; d.irq_vector = -1;
//         d.vt = &q9_devtype_duart68681; d.state = &board;
//         q9_devreg_add(d);
//         ... q9_devreg_count(); q9_devreg_get(i); q9_devreg_reset_all();
//
// Edition History
//─────────┬──────┬────────────────────────────────────────────────────────────────────────┬──────
// Date    │ Ver. │ Description                                                            │ By
//─────────┼──────┼────────────────────────────────────────────────────────────────────────┼──────
// 26-07-14│ 1.00 │ 5.17: Erster Wurf — q9_device_t/Vtable, Instanz-Registry, Typ-Registry   │ CF
//═════════╧══════╧════════════════════════════════════════════════════════════════════════╧══════
#ifndef Q9_DEVREG_H
#define Q9_DEVREG_H

#include <stdint.h>

typedef struct q9_device q9_device_t;

typedef uint8_t  (*q9_dev_read8_fn)   (q9_device_t *dev, uint32_t addr);
typedef void     (*q9_dev_write8_fn)  (q9_device_t *dev, uint32_t addr, uint8_t  val);
typedef uint16_t (*q9_dev_read16_fn)  (q9_device_t *dev, uint32_t addr);
typedef void     (*q9_dev_write16_fn) (q9_device_t *dev, uint32_t addr, uint16_t val);
typedef uint32_t (*q9_dev_read32_fn)  (q9_device_t *dev, uint32_t addr);
typedef void     (*q9_dev_write32_fn) (q9_device_t *dev, uint32_t addr, uint32_t val);
typedef void     (*q9_dev_poll_fn)        (q9_device_t *dev, uint32_t now_ms);
typedef int      (*q9_dev_irq_pending_fn) (q9_device_t *dev);
typedef void     (*q9_dev_reset_fn)       (q9_device_t *dev);
typedef int      (*q9_dev_irq_vector_fn)  (q9_device_t *dev);

typedef struct {
    q9_dev_read8_fn        read8;        /* Pflicht                                          */
    q9_dev_write8_fn       write8;       /* Pflicht                                          */
    q9_dev_read16_fn       read16;       /* optional: NULL -> aus read8 synthetisiert (BE)   */
    q9_dev_write16_fn      write16;      /* optional: NULL -> in zwei write8 zerlegt (BE)    */
    q9_dev_read32_fn       read32;       /* optional: NULL -> aus read8 synthetisiert (BE)   */
    q9_dev_write32_fn      write32;      /* optional: NULL -> in vier write8 zerlegt (BE)    */
    q9_dev_poll_fn         poll;         /* optional: einmal je Hauptschleifen-Runde         */
    q9_dev_irq_pending_fn  irq_pending;  /* optional: 1 = Geraet fordert gerade seinen IRQ an */
    q9_dev_reset_fn        reset;        /* optional: 68k-Reset (q9_m68krt_reset)            */
    /* optional: NUR fuer Geraete mit LAUFZEIT-programmiertem Vektor (68681-DUART: der OS-9-
       Treiber schreibt seinen Vektor ins IVR-Register, s. cb030.c uart_ivr) -- NULL bedeutet
       "benutze das statische dev->irq_vector" (QUICC/Netz-Terminals: fester Vektor je Instanz,
       s. devreg.h Kommentar bei irq_vector). */
    q9_dev_irq_vector_fn   irq_vector_fn;
} q9_device_vtable_t;

struct q9_device {
    const char *type;               /* Typname, z.B. "duart68681" (Doku/Diagnose, s. 5.19)   */
    const char *name;                /* Instanzname, z.B. "uart0", "x3", "enet0"              */
    uint32_t    base;                /* erste Adresse des Fensters                            */
    uint32_t    size;                /* Fenstergroesse in Byte (base..base+size-1)            */
    int         irq_level;           /* 0 = kein IRQ (z.B. CF)                                */
    int         irq_vector;          /* -1 = Autovektor, sonst vektorisiert (IACK)            */
    /* 1 = "level-held" -- die IRQ-Leitung bleibt an, bis das Geraet selbst sie wieder freigibt
       (DUART-RX-Puffer, QUICC-SCC-Event, Netz-Terminal-Byte); solche Geraete nimmt der
       IACK-/Reassert-Mechanismus (m68krt.c) in seine Pruefschleife auf. 0 = einmaliger Puls
       je Runde (Timer/IRQ3-Trigger: q9_cb030_poll_timer feuert genau einmal pro faelligem
       Tick und wird bewusst NICHT re-asserted, exakt wie vor 5.17) -- solche Geraete werden
       nur vom Hauptschleifen-Poll (cb030run.c) ueber irq_pending() unmittelbar nach poll()
       abgefragt, s. ARBEITSPLAN 5.17. */
    int         level_held;
    const q9_device_vtable_t *vt;
    void       *state;               /* typspezifischer Zustand, statisch alloziert (kein malloc) */
};

//────────────────────────────────────────────────────────────────────────────────────────────────
// Instanz-Registry: statisches Array, keine malloc (Q9-Grundsatz). Reihenfolge = Registrierungs-
// reihenfolge; die drei Aufrufstellen (Dispatch/Poll/IRQ-Ack) durchlaufen sie in genau dieser
// Reihenfolge -- fuer die IRQ-Prioritaet zwischen gleichzeitig anstehenden Geraeten wichtig
// (s. m68krt.c: bisherige Reihenfolge QUICC vor DUART vor Netz-Terminals bleibt erhalten).
//────────────────────────────────────────────────────────────────────────────────────────────────
#define Q9_DEVREG_MAX 24

void         q9_devreg_clear(void);
int          q9_devreg_add(q9_device_t dev);           /* 0 = ok, -1 = Registry voll           */
int          q9_devreg_count(void);
q9_device_t *q9_devreg_get(int index);                  /* NULL wenn ausserhalb                 */

//────────────────────────────────────────────────────────────────────────────────────────────────
// Generische Zugriffs-Helfer: pruefen Adressbereich, synthetisieren 16/32-Bit aus read8/write8,
// wenn die Vtable dafuer keine eigene Funktion mitbringt (big-endian wie der 68k/m68krt.c).
//────────────────────────────────────────────────────────────────────────────────────────────────
int      q9_device_hit(const q9_device_t *dev, uint32_t addr);

uint8_t  q9_device_read8  (q9_device_t *dev, uint32_t addr);
void     q9_device_write8 (q9_device_t *dev, uint32_t addr, uint8_t  val);
uint16_t q9_device_read16 (q9_device_t *dev, uint32_t addr);
void     q9_device_write16(q9_device_t *dev, uint32_t addr, uint16_t val);
uint32_t q9_device_read32 (q9_device_t *dev, uint32_t addr);
void     q9_device_write32(q9_device_t *dev, uint32_t addr, uint32_t val);

void     q9_device_poll       (q9_device_t *dev, uint32_t now_ms);
int      q9_device_irq_pending(q9_device_t *dev);
void     q9_device_reset      (q9_device_t *dev);

/* Liefert den fuer den IACK-Zyklus zu meldenden Vektor: dev->vt->irq_vector_fn(dev), falls gesetzt
   (laufzeitprogrammierter Vektor), sonst das statische dev->irq_vector (-1 = Autovektor). */
int      q9_device_irq_vector (q9_device_t *dev);

//────────────────────────────────────────────────────────────────────────────────────────────────
// Typ-Registry: explizite, statisch kompilierte Tabelle Typname -> Vtable (bewusst KEINE Linker-
// Magie). Fuer 5.17 nur bereitgestellt/testbar, noch nicht zur Instanziierung genutzt (kommt mit
// der Config-Datei in 5.19). Die Vtables selbst sind in den jeweiligen Geraete-Dateien definiert
// (cb030.c: DUART/CF/Timer/RTC, m68krt.c: nettty, quicc.c: QUICC) und werden hier nur verzeichnet.
//────────────────────────────────────────────────────────────────────────────────────────────────
typedef struct {
    const char                *type;
    const q9_device_vtable_t  *vt;
} q9_device_type_entry_t;

const q9_device_vtable_t *q9_devtype_lookup(const char *type);   /* NULL wenn unbekannt */
int                        q9_devtype_count(void);
const q9_device_type_entry_t *q9_devtype_get(int index);          /* NULL wenn ausserhalb */

#endif /* Q9_DEVREG_H */
//────────────────────────────────────────────────────────────────────────────────────────────────
// EOF devreg.h                                                                            Ver. 1.00
//────────────────────────────────────────────────────────────────────────────────────────────────
