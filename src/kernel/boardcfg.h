//════════════════════════════════════════════════════════════════════════════════════════════════
// File:   boardcfg.h                                                                      Ver. 1.00
// Owner:  AF
// Desc.:  5.19: Board-Konfigurationsdatei fuer den Q9-Emulator (INI-artig, C99-Parser ohne
//         Fremdbibliothek, s. docs/HWCONFIG.md Abschnitt 3). Erster Positionsparameter der
//         Kommandozeile (ohne "-") gibt die Config-Datei an; fehlt die Extension, wird ".q9"
//         angenommen. Die Datei beschreibt ROM, Netz-Backend und — neu — MEHRERE CF-Images
//         (Typ rbf oder pcf/FAT12/16), verteilt auf Onboard-CF (Master/Slave) und das
//         RC2014-SC145-Zweitinterface.
//
//         Bewusst schlank gehalten: die Datei fuellt nur eine q9_board_cfg_t-Struktur, die der
//         Boot-Runner (cb030run.c) auswertet. Die bestehenden CLI-Optionen (--cb030/--cf/--net)
//         bleiben und ueberschreiben die Config-Werte (s. cb030run/main).
//
// Call:   q9_board_cfg_t cfg; q9_board_cfg_default(&cfg);
//         if (q9_board_cfg_load(&cfg, "mysystem.q9", errbuf, sizeof(errbuf)) != 0) { ... }
//
// Edition History
//─────────┬──────┬────────────────────────────────────────────────────────────────────────┬──────
// Date    │ Ver. │ Description                                                            │ By
//─────────┼──────┼────────────────────────────────────────────────────────────────────────┼──────
// 26-07-16│ 1.00 │ 5.19: Erster Wurf — [board] rom/net + [cfN] image/type/bus/unit          │ CF
//═════════╧══════╧════════════════════════════════════════════════════════════════════════╧══════
#ifndef Q9_BOARDCFG_H
#define Q9_BOARDCFG_H

#define Q9_CFG_PATH_MAX 512
#define Q9_CFG_MAX_CF   4                    /* Onboard M/S + RC2014 M/S = maximal 4 Einheiten     */

/* CF-Bus: an welches der beiden emulierten CF-Interfaces das Image geht. */
#define Q9_CFG_BUS_ONBOARD 0                 /* $FFFFE000, Descriptoren c0..c3 (DrvNum 0/Master)   */
#define Q9_CFG_BUS_RC2014  1                 /* $FFFFC010, Descriptoren e0 (Master)/f0 (Slave)     */

typedef struct {
    char path[Q9_CFG_PATH_MAX];              /* absoluter/relativer Image-Pfad (bereits aufgeloest) */
    int  bus;                                /* Q9_CFG_BUS_*                                        */
    int  unit;                               /* 0 = Master, 1 = Slave                              */
    int  format;                             /* Q9_CF_FMT_* (cb030.h): AUTO/RBF/PCF                 */
} q9_cfg_cf_t;

typedef struct {
    char        name[64];                    /* [board] name = ... (Banner/Diagnose)               */
    char        rom_path[Q9_CFG_PATH_MAX];   /* [board] rom  = ... (leer = per CLI/Default)        */
    char        net_mode[32];                /* [board] net  = nat|vmnet|bridge:<if> (leer = nat)  */
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

#endif /* Q9_BOARDCFG_H */
//────────────────────────────────────────────────────────────────────────────────────────────────
// EOF boardcfg.h                                                                          Ver. 1.00
//────────────────────────────────────────────────────────────────────────────────────────────────
