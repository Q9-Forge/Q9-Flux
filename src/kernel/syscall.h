//════════════════════════════════════════════════════════════════════════════════════════════════
// File:   syscall.h                                                                       Ver. 1.30
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
#define F_ID      0x0c                                 /* F$ID:     Return Process ID            */
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

//╔══════════════════════════════════════════════════════════════════════════════════════════════╗
//║ ERROR CODES (= OS-9, MWOS errno.h)                                                           ║
//╚══════════════════════════════════════════════════════════════════════════════════════════════╝

#define E_PTHFUL  0xc8                                 /* path table full                        */
#define E_BPNUM   0xc9                                 /* bad path number                        */
#define E_BMODE   0xcb                                 /* bad access mode                        */
#define E_UNKSVC  0xd0                                 /* unknown service request                */
#define E_BPADDR  0xd2                                 /* bad parameter address                  */
#define E_EOF     0xd3                                 /* end of file                            */
#define E_BPNAM   0xd7                                 /* bad path name                          */
#define E_MNF     0xdd                                 /* module not found (unbekanntes Gerät)   */
#define E_PARAM   0xe1                                 /* bad parameter                          */
#define E_DIFF    0xe2                                 /* names differ (F$CmpNam) — VORLÄUFIG:   */
                                                       /*   OS-9 setzt nur Carry; Nummer beim    */
                                                       /*   MWOS-Abgleich prüfen (M:\MWOS)       */
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

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_proc_halted
// Desc.:    1 = Proto-Prozess wurde per F$Exit beendet (Übergangslösung bis Phase 4).
// Call:     if (q9_proc_halted()) ...
//════════════════════════════════════════════════════════════════════════════════════════════════
int q9_proc_halted(void);

#endif // Q9_SYSCALL_H

//────────────────────────────────────────────────────────────────────────────────────────────────
// EOF syscall.h                                                                           Ver. 1.30
//────────────────────────────────────────────────────────────────────────────────────────────────
