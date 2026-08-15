//════════════════════════════════════════════════════════════════════════════════════════════════
// File:   boardcfg.h                                                                      Ver. 1.50
// Owner:  AF
// Desc.:  5.19: Board-Konfigurationsdatei fuer den Q9-Emulator (INI-artig, C99-Parser ohne
//         Fremdbibliothek, s. docs/HWCONFIG.md Abschnitt 3). Erster Positionsparameter der
//         Kommandozeile (ohne "-") gibt die Config-Datei an; fehlt die Extension, wird ".q9"
//         angenommen. Die Datei beschreibt ROM, Netz-Backend und — neu — MEHRERE CF-Images
//         (Typ rbf oder pcf/FAT12/16), verteilt auf Onboard-CF (Master/Slave) und das
//         RC2014-SC145-Zweitinterface.
//
//         Bewusst schlank gehalten: die Datei fuellt nur eine q9_board_cfg_t-Struktur, die der
//         Boot-Runner (q9boardrun.c) auswertet. Die bestehenden CLI-Optionen (--rom/--cf/--net)
//         bleiben und ueberschreiben die Config-Werte (s. q9boardrun/main).
//
// Call:   q9_board_cfg_t cfg; q9_board_cfg_default(&cfg);
//         if (q9_board_cfg_load(&cfg, "mysystem.q9", errbuf, sizeof(errbuf)) != 0) { ... }
//
// Edition History
//─────────┬──────┬────────────────────────────────────────────────────────────────────────┬──────
// Date    │ Ver. │ Description                                                            │ By
//─────────┼──────┼────────────────────────────────────────────────────────────────────────┼──────
// 26-07-23│ 1.10 │ 5.19: vmnet_ip/gateway/netmask/dhcp_end in [board]                    │ AF
// 26-08-07│ 1.20 │ 5.14: net_hostfwd in [board] -- Host->Gast-Portweiterleitung fuer     │ AF
//         │      │ net=slirp (vmnet_ip/_gateway/_netmask wiederverwendet, s. dortige      │
//         │      │ Kommentare)                                                            │
// 26-08-14│ 1.30 │ q9_cfg_cf_t.descriptor (String) ersetzt durch has_descriptor (Bool) +   │ Cld
//         │      │ descriptor_name (String) -- projektweit einheitliche descriptor-        │
//         │      │ Bedeutung (s. devschema.c), alle betroffenen .q9-Dateien mitmigriert    │
// 26-08-14│ 1.40 │ q9_cfg_cf_t.use_slot/slot -- devschema.c useSlot/slot jetzt tatsaechlich  │ Cld
//         │      │ wirksam (nicht mehr nur Schema-Beschreibung), s. q9boardrun.c            │
// 26-08-15│ 1.50 │ Q9FLUX_EDITOR_de.md 4.1: [board]-Key "cpu" -- CPU-Typ-Auswahl, bisher nur │ Cld
//         │      │ per Q9_CPU=ec030-Env-Var versteckt (s. m68krt.h q9_cpu_type_t)            │
//═════════╧══════╧════════════════════════════════════════════════════════════════════════╧══════
#ifndef Q9_BOARDCFG_H
#define Q9_BOARDCFG_H

#include <stdint.h>

#define Q9_CFG_PATH_MAX 512
#define Q9_CFG_MAX_CF   4

/* CF-Bus: an welches der beiden emulierten CF-Interfaces das Image geht. */
#define Q9_CFG_BUS_ONBOARD 0                 /* $FFFFE000, Descriptoren c0..c3 (DrvNum 0/Master)   */
#define Q9_CFG_BUS_RC2014  1                 /* $FFFFC010, Descriptoren e0 (Master)/f0 (Slave)     */

typedef struct {
    char path[Q9_CFG_PATH_MAX];              /* absoluter/relativer Image-Pfad (bereits aufgeloest) */
    /* 2026-08-14: "descriptor" ist projektweit (auch bei kuenftigen Nicht-CF-Geraeten, s.
       devschema.c "memory") ein Bool -- braucht dieses Geraet ueberhaupt einen OS-9-Descriptor
       (bei CF Default yes). Der bisherige Descriptor-NAME-String (fuer den ROM-Generator, s.
       ARBEITSPLAN 5.20, noch nicht gebaut) heisst .q9-seitig jetzt "descriptorName" (Andreas'
       camelCase-Vorgabe fuer neue Keys) und ist nur relevant, wenn has_descriptor gesetzt ist --
       ersetzt das alte, gleichnamige Feld "descriptor" (String). Der C-Struct-Feldname bleibt
       bewusst snake_case (descriptor_name), passend zum sonstigen C-Stil dieser Codebasis --
       beide Namensraeume (Datei-Syntax/C-Implementierung) duerfen auseinanderlaufen. */
    int  has_descriptor;                     /* .q9-Key "descriptor" = yes/no, Default yes           */
    char descriptor_name[Q9_CFG_PATH_MAX];   /* .q9-Key "descriptorName" (ersetzt altes "descriptor") */
    int  bus;                                /* Q9_CFG_BUS_*                                        */
    int  unit;                               /* 0 = Master, 1 = Slave                              */
    int  format;                             /* Q9_CF_FMT_* (q9board.h): AUTO/RBF/PCF                 */
    uint32_t base;                           /* ATA-Base; 0 = Standard-Base anhand bus             */
    /* 2026-08-14 (ARBEITSPLAN 5.18-Fortsetzung, devschema.c useSlot/slot jetzt wirksam): Wahl
       zwischen einem automatisch zugeteilten I/O-Tabellenplatz (256-Byte-Raster ab $FFFF0000,
       s. m68krt.c g_io_table) und der freien "base" oben. use_slot=1 GEWINNT ueber "base" --
       q9boardrun.c berechnet die Adresse dann aus slot statt base zu lesen. slot=-1 bedeutet
       "nicht gesetzt" (Sentinel, s. boardcfg.c cfg_parse_u32 arbeitet mit uint32_t, daher eigenes
       int-Feld mit -1-Sentinel statt 0 -- 0 waere ein GUELTIGER Slot und liesse sich nicht von
       "vergessen" unterscheiden). use_slot=1 UND slot==-1 ist ein Parse-Fehler (s. boardcfg.c). */
    int      use_slot;                       /* .q9-Key "useSlot" = yes/no, Default no                */
    int      slot;                           /* .q9-Key "slot" = 0-255, -1 = nicht gesetzt            */
    uint32_t start_sector;                   /* Host-Startsektor fuer Gast-LBA 0 (Default 0)       */
    uint32_t length_sectors;                 /* logische Partitionslaenge fuer Descriptor/Pruefung  */
    uint32_t descriptor_lsn;                 /* PD_LSNOffs im OS-9-Descriptor                   */
} q9_cfg_cf_t;

typedef struct {
    char        name[64];                    /* [board] name = ... (Banner/Diagnose)               */
    char        rom_path[Q9_CFG_PATH_MAX];   /* [board] rom  = ... (leer = per CLI/Default)        */
    char        net_mode[32];                /* [board] net  = nat|vmnet|bridge:<if> (leer = nat)  */
    char        vmnet_ip[32];                /* [board] vmnet_ip = Gast-IP (OS-9-seitig statisch)  */
    char        vmnet_gateway[32];           /* [board] vmnet_gateway = vmnet Shared-Gateway      */
    char        vmnet_netmask[32];           /* [board] vmnet_netmask = Netzmaske                */
    char        vmnet_dhcp_end[32];          /* [board] vmnet_dhcp_end = DHCP-Bereichsende       */
    char        net_hostfwd[256];            /* [board] net_hostfwd = tcp:2323:23,tcp:2000:2000 --
                                                 5.14 (slirp): Host-Port -> Gast-Port, kommasepariert,
                                                 gilt fuer net=slirp UND (wiederverwendet) vmnet_ip/
                                                 _gateway/_netmask fuer slirp's Subnetz-Konfiguration */
    char        cpu[16];                     /* [board] cpu = 68030|68000|68010|68020|68ec020|
                                                 68ec030|68040|68ec040|68lc040 (leer = 68030-Default,
                                                 s. m68krt.h q9_cpu_type_t) -- Q9FLUX_EDITOR_de.md 4.1 */
    q9_cfg_cf_t cf[Q9_CFG_MAX_CF];
    int         cf_count;
} q9_board_cfg_t;

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_board_cfg_default
// Desc.:    Setzt die Struktur auf "leere" Defaults (kein ROM, kein CF, net leer). So bleibt der
//           Weg ohne Config-Datei exakt das heutige Verhalten (CLI-Argumente steuern alles).
//════════════════════════════════════════════════════════════════════════════════════════════════
void q9_board_cfg_default(q9_board_cfg_t *cfg);

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_board_cfg_resolve_path
// Desc.:    Haengt bei fehlender Extension ".q9" an den uebergebenen Config-Namen an und schreibt
//           das Ergebnis nach out (max out_max Byte). Enthaelt der Basisname bereits einen Punkt
//           (im letzten Pfadsegment), bleibt er unveraendert.
// Call:     q9_board_cfg_resolve_path("mysys", buf, sizeof(buf))  ->  "mysys.q9"
//════════════════════════════════════════════════════════════════════════════════════════════════
void q9_board_cfg_resolve_path(const char *arg, char *out, unsigned out_max);

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_board_cfg_load
// Desc.:    Liest die Config-Datei und fuellt cfg. Image-/ROM-Pfade werden relativ zur CONFIG-DATEI
//           aufgeloest (nicht zum CWD, s. docs/HWCONFIG.md). Gibt 0 bei Erfolg zurueck, sonst -1
//           und eine erklaerende Meldung (mit Zeilennummer) in err (max err_max Byte).
// Call:     if (q9_board_cfg_load(&cfg, path, err, sizeof(err)) != 0) fprintf(stderr, "%s\n", err);
//════════════════════════════════════════════════════════════════════════════════════════════════
int q9_board_cfg_load(q9_board_cfg_t *cfg, const char *cfg_path, char *err, unsigned err_max);

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_cfg_cf_effective_base
// Desc.:    Berechnet die tatsaechliche ATA-Basisadresse fuer einen CF-Abschnitt -- EINZIGE Stelle
//           fuer diese Regel (2026-08-14 aus drei duplizierten Inline-Berechnungen zusammengezogen,
//           boardcfg.c-Nachvalidierung + zweimal q9boardrun.c): use_slot=1 GEWINNT ($FFFF0000 +
//           slot*256), sonst base (falls != 0), sonst der Bus-Standard (Onboard $FFFFE000,
//           RC2014 $FFFFC010). Ruft NICHT cf->use_slot/slot auf Gueltigkeit ab (das ist
//           q9_board_cfg_load()s Aufgabe bei der Nachvalidierung) -- bei use_slot=1 UND slot==-1
//           (eigentlich ein Config-Fehler, der load() schon abgefangen haben sollte) wird slot als
//           0 behandelt, rein defensiv, kein Crash.
// Call:     uint32_t addr = q9_cfg_cf_effective_base(&cfg.cf[i]);
//════════════════════════════════════════════════════════════════════════════════════════════════
uint32_t q9_cfg_cf_effective_base(const q9_cfg_cf_t *cf);

#endif /* Q9_BOARDCFG_H */
//────────────────────────────────────────────────────────────────────────────────────────────────
// EOF boardcfg.h                                                                          Ver. 1.50
//────────────────────────────────────────────────────────────────────────────────────────────────
