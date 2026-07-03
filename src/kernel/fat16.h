//════════════════════════════════════════════════════════════════════════════════════════════════
// File:   fat16.h                                                                        Ver. 1.00
// Owner:  AF
// Desc.:  Q9 FAT16-File-Manager, lesend (Phase 3.3). Nur Superfloppy-Images (Boot-Sektor bei
//         LBA 0 des Block-Device, KEINE Partitionstabelle) — Interop-Test: am Mac mit
//         `newfs_msdos -F 16` formatiert und befuellt, aus Q9 heraus gelesen (ARBEITSPLAN.md).
//         Layout aus oeffentlich dokumentierter FAT16-Spezifikation (Microsoft "FAT: General
//         Overview of On-Disk Format") nachgebaut — KEIN Copyright-Code uebernommen. Als
//         Design-Referenz fuer den File-Manager-Schnitt diente "OS-9 Insights" (Dibble, nur
//         Konzept gelesen, nicht abgeschrieben).
//
//         Boot-Pipeline: q9_fat16_mount() liest+prueft den Boot-Sektor (BPB) und merkt sich
//         FAT-Start/Root-Dir-Start/Datenregion-Start/ClusterSize als statischen Zustand (kein
//         malloc). Danach reicht q9_fat16_fm (q9_fm_t, vfs.h) als File-Manager an ein q9_dev_t.
//
// Call:   if (q9_fat16_mount() == 0) q9_dev_set_fm(d0, &q9_fat16_fm);
//
// Edition History
//─────────┬──────┬────────────────────────────────────────────────────────────────────────┬──────
// Date    │ Ver. │ Description                                                            │ By
//─────────┼──────┼────────────────────────────────────────────────────────────────────────┼──────
// 26-07-04│ 1.00 │ 3.3: Initiale Version — Boot-Sektor/Root-Dir/Cluster-Ketten lesend,     │ CF
//         │      │ 8.3 + LFN-Namen, q9_fat16_fm (open/read/seek)                          │
//═════════╧══════╧════════════════════════════════════════════════════════════════════════╧══════
#ifndef Q9_FAT16_H
#define Q9_FAT16_H

#include <stdint.h>
#include "vfs.h"

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_fat16_mount
// Desc.:    Liest den Boot-Sektor (LBA 0) über die HAL und prüft ihn auf ein plausibles FAT16-
//           Superfloppy-Layout (BytesPerSector == Q9_BLK_SIZE, SectorsPerCluster Zweierpotenz,
//           FATSize16 != 0, RootEntryCount != 0, Boot-Signatur $55AA). Bei Erfolg wird die
//           interne Geometrie (FAT-Start/Root-Dir/Datenregion/ClusterSize) gemerkt.
//           Rückgabe 0 = erkannt und gemountet, sonst E$NotRdy (kein/kein plausibles FAT16-Image).
// Call:     err = q9_fat16_mount()
//════════════════════════════════════════════════════════════════════════════════════════════════
int q9_fat16_mount(void);

//╔══════════════════════════════════════════════════════════════════════════════════════════════╗
//║ FILE-MANAGER (q9_fm_t, vfs.h) — an ein q9_dev_t haengen via q9_dev_set_fm()                   ║
//╚══════════════════════════════════════════════════════════════════════════════════════════════╝
extern const q9_fm_t q9_fat16_fm;

#endif // Q9_FAT16_H

//────────────────────────────────────────────────────────────────────────────────────────────────
// EOF fat16.h                                                                            Ver. 1.00
//────────────────────────────────────────────────────────────────────────────────────────────────
