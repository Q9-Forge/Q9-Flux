//════════════════════════════════════════════════════════════════════════════════════════════════
// File:   cf.h                                                                            Ver. 1.00
// Owner:  CF
// Desc.:  Compact-Flash-Interface (ATA-PIO-Minimalprotokoll, docs/BOARD.md Abschnitt "Compact-
//         Flash-Interface"). Backing Store = lazy geoeffnete Host-Datei (Muster wie q9disk.img).
//         Mehrfach instanziierbar (Onboard-CF $FFFFE000 + RC2014-SC145-Zweitinterface $FFFFC010,
//         zwei Einheiten je Interface -- Master/Slave via DEV-Bit in LBA3, Image-Format rbf/pcf aus
//         der Board-Config steuert die Sektor-Heuristik/IDENTIFY, s. boardcfg.h).
//
//         2026-08-20 (Hardware-Vereinheitlichung, Pilot-Migration): aus src/kernel/q9board.c/.h
//         HIERHER verschoben -- Andreas' Vorgabe (2026-08-19), pro Hardware-Typ EIN eigenes
//         Sourcefile zu haben (Vorbild: src/devices/mc6845/quicc/framebuf/clut, "6.6"-Migration
//         2026-08-11). Reines Verschieben, KEINE Verhaltensaenderung -- Registerlogik, ATA-PIO-
//         Protokoll, RBF-Sektorgroessen-Heuristik unveraendert (per Boot-Test verifiziert, s.
//         Q9FLUX_EDITOR_de.md). NEU dabei: q9_devdesc_cf (devdesc.h) buendelt Vtable + die bisher in
//         devschema.c separat gepflegten Feldbeschreibungen an dieser einen Stelle.
//
// Call:   q9_cf_attach(&b.cf, 0, "local_images/board_cf.img", Q9_CF_FMT_AUTO)
//
// Edition History
//─────────┬──────┬────────────────────────────────────────────────────────────────────────┬──────
// Date    │ Ver. │ Description                                                            │ By
//─────────┼──────┼────────────────────────────────────────────────────────────────────────┬──────
// 26-07-04│ 1.xx │ 5.2c: Compact-Flash — ATA-PIO-Minimalprotokoll, urspruenglich Teil von   │ CF
//         │      │ q9board.h/.c (s. dortige Historie bis Ver. 2.20/2.00 fuer die volle       │
//         │      │ Entwicklungsgeschichte: 5.5a Multi-Sektor, 5.17 Geraete-Registry-Umzug,   │
//         │      │ 5.19a Mehrfachinstanziierung ueber q9_cf_t)                               │
// 26-08-20│ 1.00 │ Hardware-Vereinheitlichung, Pilot "cf": aus q9board.c/.h hierher verschoben,│ Cld
//         │      │ neu q9_devdesc_cf (Feldbeschreibung, bisher devschema.c "cf")             │
//═════════╧══════╧════════════════════════════════════════════════════════════════════════╧══════
#ifndef Q9_CF_H
#define Q9_CF_H

#include <stdint.h>
#include <stdio.h>
#include "../../kernel/devreg.h"                          /* q9_device_t/Vtable                     */
#include "../../kernel/devdesc.h"                          /* q9_devdesc_t, s. devdesc.h-Kopfkommentar */

/* 5.2c: Compact-Flash-Interface (docs/BOARD.md, Abschnitt "Compact-Flash-Interface"). */
#define Q9_BOARD_CF_BASE           0xFFFFE000u
#define Q9_BOARD_CF_TOP            0xFFFFE0FFu
/* 5.19a: Zweites CF-Interface — SC145-CF-Kartenleser im RC2014-Erweiterungsslot, Adresse aus
   dem REF-Q9-Port (systype.d: RC2014_CF_Base = Slot-Basis $FFFFC000 + Offset $10; Descriptoren
   e0 = Master / f0 = Slave, beide DrvNum-gesteuert ueber das DEV-Bit in LBA3: $E0/$F0). Fenster
   sind die 8 ATA-Register $FFFFC010–$FFFFC017. Wird NUR registriert, wenn die Board-Config
   (boardcfg.h) dort Images anhaengt — ohne Config existiert das Fenster nicht (Board wie bisher). */
#define Q9_BOARD_CF2_BASE          0xFFFFC010u
#define Q9_BOARD_CF2_TOP           0xFFFFC017u
#define Q9_BOARD_CF_CMD_READ       0x20u              /* READ SECTOR(S)  */
#define Q9_BOARD_CF_CMD_WRITE      0x30u              /* WRITE SECTOR(S) */
#define Q9_BOARD_CF_CMD_SETFEAT    0xEFu              /* SET FEATURES (8-Bit-Mode etc.) */
#define Q9_BOARD_CF_STAT_BSY       0x80u
#define Q9_BOARD_CF_STAT_DRQ       0x08u
#define Q9_BOARD_CF_STAT_RDY       0x40u
#define Q9_BOARD_CF_STAT_ERR       0x01u
#define Q9_BOARD_CF_SECTOR_SIZE    512u

/* 5.19a: Image-Format eines angehaengten CF-Images (aus der Board-Config, s. boardcfg.h) —
   steuert NUR die Host-seitige Sektorgroessen-Erkennung und die IDENTIFY-Sektorzahl, nicht das
   ATA-Protokoll selbst:
   AUTO/RBF = bisherige RBF-Heuristik (256-Byte-LSNs alter OS-9-Images erkennen, DD_TOT aus LSN0);
   PCF      = FAT12/16-Image: immer 512-Byte-Sektoren, keine RBF-Heuristik (die FAT-Bootsektor-
              Bytes wuerden sonst als LSN0 fehlgedeutet), IDENTIFY-Sektorzahl aus der Dateigroesse. */
#define Q9_CF_FMT_AUTO 0
#define Q9_CF_FMT_RBF  1
#define Q9_CF_FMT_PCF  2

/* 5.19a: Eine CF-EINHEIT (Master oder Slave) — Backing-Datei + erkannte Sektorgroesse. */
typedef struct {
    const char *path;                                  /* NULL = Einheit nicht bestueckt          */
    FILE       *file;                                  /* lazy geoeffnet (Muster wie q9disk.img)  */
    uint32_t    image_sector_size;                     /* 0 = noch unerkannt; 256/512             */
    uint32_t    start_sector;                          /* Host-LBA, auf den Gast-LBA 0 abgebildet wird */
    int         format;                                /* Q9_CF_FMT_*                             */
} q9_cf_unit_t;

/* 5.19a: Ein CF-INTERFACE (ATA-Registersatz + Sektorpuffer), mehrfach instanziierbar (Onboard-CF
   $FFFFE000 + RC2014-SC145 $FFFFC010). Beide Einheiten teilen sich Registersatz und Puffer wie
   bei echtem ATA — welche Einheit ein Kommando bedient, entscheidet das DEV-Bit (Bit 4) in LBA3
   ($E0 = Master, $F0 = Slave, exakt die Werte der e0/f0-Descriptoren im REF-Q9-Port). */
typedef struct {
    q9_cf_unit_t unit[2];                              /* [0] = Master, [1] = Slave              */
    uint32_t     lba;
    uint8_t      lba3;                                 /* LBA bits 27..24 + DEV/LBA-Flags        */
    uint8_t      sectcnt;
    uint8_t      status;
    uint8_t      sector[Q9_BOARD_CF_SECTOR_SIZE];
    uint32_t     pos;                                  /* Index in sector, 0..SECTOR_SIZE        */
    uint32_t     transfer_size;                        /* 256 fuer alte RBF-Daten, IDENTIFY 512  */
    int          write_pending;                        /* 1 waehrend WRITE-SECTOR-Datenphase     */
    uint32_t     remaining;                            /* 5.5a: ausstehende Sektoren im Kommando */
} q9_cf_t;

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_cf_attach
// Desc.:    5.19a: Haengt ein Backing-Image an EINE Einheit (0 = Master, 1 = Slave) eines
//           CF-Interfaces — die Datei selbst wird lazy beim ersten Kommando geoeffnet/angelegt
//           (Muster wie die native HAL bei q9disk.img). path muss die gesamte Lebensdauer von c
//           ueberleben (wird nur als Zeiger gehalten, nicht kopiert). format = Q9_CF_FMT_*
//           (s.o.). Setzt den Interface-Registersatz zurueck (wie ein Kartenwechsel).
// Call:     q9_cf_attach(&b.cf, 0, "local_images/board_cf.img", Q9_CF_FMT_AUTO)
//════════════════════════════════════════════════════════════════════════════════════════════════
void q9_cf_attach(q9_cf_t *c, int unit, const char *path, int format);
void q9_cf_set_start_sector(q9_cf_t *c, int unit, uint32_t start_sector);

extern const q9_device_vtable_t q9_devtype_cf;            /* 5.17: eigene 16/32-Bit-Pfade            */
extern const q9_devdesc_t       q9_devdesc_cf;             /* 2026-08-20: Vtable+Schema vereint       */

#endif /* Q9_CF_H */
//────────────────────────────────────────────────────────────────────────────────────────────────
// EOF cf.h                                                                                Ver. 1.00
//────────────────────────────────────────────────────────────────────────────────────────────────
