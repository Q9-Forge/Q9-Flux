//════════════════════════════════════════════════════════════════════════════════════════════════
// File:   syscall.h                                                                       Ver. 2.10
// Owner:  AF
// Desc.:  Q9 Syscall-Schnittstelle. Funktionsnummern und Fehlercodes sind identisch zu OS-9
//         (Quelle: MWOS DEFS/funcs.h + errno.h). Parameter im virtuellen 68k-Registersatz.
//         ABI-Spezifikation: docs/SYSCALLS.md
//
// Call:   q9_regs_t r = {0}; r.d[0] = 1; ...; err = q9_syscall(I_WRITLN, &r);
//
// Edition History
//─────────┬──────┬────────────────────────────────────────────────────────────────────────┬──────
// Date    │ Ver. │ Description                                                            │ By
//─────────┼──────┼────────────────────────────────────────────────────────────────────────┼──────
// 26-07-03│ 1.00 │ Initiale Version: Registersatz, F$/I$-Nummern, Fehlercodes             │ CF
// 26-07-03│ 1.10 │ 1.5: E$BPNam + E$Diff (vorläufig) ergänzt                             │ CF
// 26-07-03│ 1.20 │ 1.6: E$MNF ergänzt                                                     │ CF
// 26-07-03│ 1.30 │ 1.8: SS.*-Statuscodes ergänzt                                          │ CF
// 26-07-03│ 1.40 │ Bugfix: E$Diff($E2) kollidierte mit E$NoChld -> E_DIFFER($A5)         │ CF
// 26-07-03│ 1.50 │ 2.3b-d: E$BMHP/E$BMCRC/E$DirFul/E$ModBsy ergänzt (MWOS-verifiziert)   │ CF
// 26-07-03│ 1.60 │ 3.1: SS.BlkRd/SS.BlkWr ergänzt (Roh-Blockzugriff /d0)                 │ CF
// 26-07-04│ 1.70 │ 3.2: E$PNNF ergänzt (VFS: Pfad-Routing/I$Open/I$ChgDir)               │ CF
// 26-07-04│ 1.80 │ 3.4: I$Write jetzt echt (FAT16-Routing); E$DirFul auch fuer volle     │ CF
//         │      │ FAT16-Directorys wiederverwendet (kein neuer Code noetig, MWOS-Wert)   │
// 26-07-04│ 1.90 │ 3.5: E$NoRAM ergaenzt (F$Load: Modul-Puffer-Pool voll/Datei zu gross) │ CF
// 26-07-04│ 2.00 │ 4.2: E$IPrcID/E$NoChld/E$PrcFul/E$NEMod ergaenzt (F$Fork/F$Wait/       │ CF
//         │      │ F$Chain, MWOS-verifiziert — E$NoChld($E2) bestaetigt die 1.40-Notiz)   │
// 26-07-04│ 2.10 │ 4.4: F_SSPD ($0B) + F_SPRIOR ($0D) ergaenzt (Prozess suspendieren/     │ CF
//         │      │ Prioritaetsfeld setzen)                                                │
//═════════╧══════╧════════════════════════════════════════════════════════════════════════╧══════
#ifndef Q9_SYSCALL_H
#define Q9_SYSCALL_H

#include <stdint.h>

//╔══════════════════════════════════════════════════════════════════════════════════════════════╗
//║ VIRTUAL REGISTER SET (canonical 68k layout)                                                  ║
//╚══════════════════════════════════════════════════════════════════════════════════════════════╝

typedef struct q9_regs {
    uint32_t d[8];                                     /* d0..d7 data registers                  */
    void    *a[8];                                     /* a0..a7 address registers               */
} q9_regs_t;

//╔══════════════════════════════════════════════════════════════════════════════════════════════╗
//║ FUNCTION CODES (= OS-9, MWOS funcs.h)                                                        ║
//╚══════════════════════════════════════════════════════════════════════════════════════════════╝

#define F_LINK    0x00                                 /* F$Link:   Link to Module               */
#define F_LOAD    0x01                                 /* F$Load:   Load Module from File        */
#define F_UNLINK  0x02                                 /* F$UnLink: Unlink Module                */
#define F_FORK    0x03                                 /* F$Fork:   Start New Process            */
#define F_WAIT    0x04                                 /* F$Wait:   Wait for Child Process       */
#define F_CHAIN   0x05                                 /* F$Chain:  Chain Process to New Module  */
#define F_EXIT    0x06                                 /* F$Exit:   Terminate Process            */
#define F_SEND    0x08                                 /* F$Send:   Send Signal to Process       */
#define F_SLEEP   0x0a                                 /* F$Sleep:  Suspend Process              */
#define F_SSPD    0x0b                                 /* F$SSpd:   Suspend Process              */
#define F_ID      0x0c                                 /* F$ID:     Return Process ID            */
#define F_SPRIOR  0x0d                                 /* F$SPrior: Set Process Priority         */
#define F_PRSNAM  0x10                                 /* F$PrsNam: Parse Pathlist Name          */
#define F_CMPNAM  0x11                                 /* F$CmpNam: Compare Two Names            */
#define F_TIME    0x15                                 /* F$Time:   Get Current Time             */
#define F_STIME   0x16                                 /* F$STime:  Set Current Time             */
#define F_CRC     0x17                                 /* F$CRC:    Generate CRC                 */

#define I_ATTACH  0x80                                 /* I$Attach: Attach I/O Device            */
#define I_DETACH  0x81                                 /* I$Detach: Detach I/O Device            */
#define I_DUP     0x82                                 /* I$Dup:    Duplicate Path               */
#define I_CREATE  0x83                                 /* I$Create: Create New File              */
#define I_OPEN    0x84                                 /* I$Open:   Open Existing File           */
#define I_MAKDIR  0x85                                 /* I$MakDir: Make Directory File          */
#define I_CHGDIR  0x86                                 /* I$ChgDir: Change Default Directory     */
#define I_DELETE  0x87                                 /* I$Delete: Delete File                  */
#define I_SEEK    0x88                                 /* I$Seek:   Change Current Position      */
#define I_READ    0x89                                 /* I$Read:   Read Data                    */
#define I_WRITE   0x8a                                 /* I$Write:  Write Data                   */
#define I_READLN  0x8b                                 /* I$ReadLn: Read Line of ASCII Data      */
#define I_WRITLN  0x8c                                 /* I$WritLn: Write Line of ASCII Data     */
#define I_GETSTT  0x8d                                 /* I$GetStt: Get Path Status              */
#define I_SETSTT  0x8e                                 /* I$SetStt: Set Path Status              */
#define I_CLOSE   0x8f                                 /* I$Close:  Close Path                   */

//╔══════════════════════════════════════════════════════════════════════════════════════════════╗
//║ STATUS CODES für I$GetStt/I$SetStt (= OS-9 SS.*; Nummern beim MWOS-Abgleich prüfen)          ║
//╚══════════════════════════════════════════════════════════════════════════════════════════════╝

#define SS_OPT    0x00                                 /* SS.Opt:   path options (später)        */
#define SS_READY  0x01                                 /* SS.Ready: data ready?                  */
#define SS_SIZE   0x02                                 /* SS.Size:  file size (später, VFS)      */
#define SS_EOF    0x06                                 /* SS.EOF:   test for end of file         */
#define SS_BLKRD  0x14                                 /* SS.BlkRd: raw block read (3.1, /d0)    */
#define SS_BLKWR  0x15                                 /* SS.BlkWr: raw block write (3.1, /d0)   */

//╔══════════════════════════════════════════════════════════════════════════════════════════════╗
//║ ERROR CODES (= OS-9, MWOS errno.h)                                                           ║
//╚══════════════════════════════════════════════════════════════════════════════════════════════╝

#define E_DIFFER  0xa5                                 /* names differ (F$CmpNam)                 */
#define E_PTHFUL  0xc8                                 /* path table full                        */
#define E_BPNUM   0xc9                                 /* bad path number                        */
#define E_BMODE   0xcb                                 /* bad access mode                        */
#define E_BMID    0xcd                                 /* bad module id (2.3b: hdrsize/typo)     */
#define E_DIRFUL  0xce                                 /* module directory full (2.3c)           */
#define E_UNKSVC  0xd0                                 /* unknown service request                */
#define E_MODBSY  0xd1                                 /* module busy, still linked (2.3d)       */
#define E_BPADDR  0xd2                                 /* bad parameter address                  */
#define E_EOF     0xd3                                 /* end of file                            */
#define E_BPNAM   0xd7                                 /* bad path name                          */
#define E_PNNF    0xd8                                 /* path name not found (3.2, VFS)         */
#define E_MNF     0xdd                                 /* module not found (unbekanntes Gerät)   */
#define E_IPRCID  0xe0                                 /* illegal process ID (4.2: F$Wait/F$Send  */
                                                        /*   auf unbekannte/fremde PID)             */
#define E_PARAM   0xe1                                 /* bad parameter                          */
#define E_NOCHLD  0xe2                                 /* no children (4.2: F$Wait ohne Kinder)   */
#define E_PRCFUL  0xe5                                 /* too many active processes (4.2: F$Fork, */
                                                        /*   Prozesstabelle voll)                  */
#define E_BMCRC   0xe8                                 /* bad module CRC (2.3b)                  */
#define E_NEMOD   0xea                                 /* non-executable module (4.2: F$Fork/     */
                                                        /*   F$Chain auf ein Nicht-Q9_MOD_NATIVE-  */
                                                        /*   Modul — noch keine 68k/WASM-Runtime)  */
#define E_BMHP    0xec                                 /* bad module header (2.3b, keine echte   */
                                                        /*   Parity-Pruefung — Q9 macht das nicht) */
#define E_NORAM   0xed                                 /* no RAM available (3.5: Load-Puffer-    */
                                                        /*   Pool voll/Datei zu gross)             */
#define E_NOTRDY  0xf6                                 /* device not ready                       */

//╔══════════════════════════════════════════════════════════════════════════════════════════════╗
//║ API                                                                                          ║
//╚══════════════════════════════════════════════════════════════════════════════════════════════╝

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_syscall
// Desc.:    Zentraler Syscall-Dispatcher. Parameter/Ergebnisse im virtuellen Registersatz.
//           Rückgabe 0 = Erfolg, sonst OS-9-Fehlercode (entspricht Carry + d1.w/B).
// Call:     err = q9_syscall(I_WRITLN, &regs)
//════════════════════════════════════════════════════════════════════════════════════════════════
int q9_syscall(uint16_t func, q9_regs_t *regs);

#endif // Q9_SYSCALL_H

//────────────────────────────────────────────────────────────────────────────────────────────────
// EOF syscall.h                                                                           Ver. 2.10
//────────────────────────────────────────────────────────────────────────────────────────────────
