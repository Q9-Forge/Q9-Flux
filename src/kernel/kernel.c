//════════════════════════════════════════════════════════════════════════════════════════════════
// File:   kernel.c                                                                        Ver. 2.80
// Owner:  AF
// Desc.:  Q9-Kernel, Phase 1: Boot + Zeilen-REPL, komplett über die eigene Syscall-Schicht
//         (I$ReadLn/I$WritLn — Dogfooding der OS-9-kompatiblen ABI, siehe docs/SYSCALLS.md).
//
// Call:   q9_kernel_init(); danach zyklisch q9_kernel_step();
//
// Edition History
//─────────┬──────┬────────────────────────────────────────────────────────────────────────┬──────
// Date    │ Ver. │ Description                                                            │ By
//─────────┼──────┼────────────────────────────────────────────────────────────────────────┼──────
// 26-07-02│ 1.00 │ Initiale Version: Banner, Echo-Loop                                    │ CF
// 26-07-03│ 1.10 │ Syscall-Schicht: REPL über I$ReadLn/I$WritLn, F$Exit, Selbsttest       │ CF
// 26-07-03│ 1.20 │ 1.3: q9_dev_init() beim Boot, Selbsttests für Device-Modell            │ CF
// 26-07-03│ 1.30 │ 1.4: Selbsttests I$Dup/I$Close                                         │ CF
// 26-07-03│ 1.40 │ 1.5: Selbsttests F$PrsNam/F$CmpNam                                     │ CF
// 26-07-03│ 1.50 │ 1.6: Selbsttests I$Attach/I$Detach                                     │ CF
// 26-07-03│ 1.60 │ 1.7: /nil im Banner + Selbsttest                                       │ CF
// 26-07-03│ 1.70 │ 1.8: Selbsttests I$GetStt/I$SetStt                                     │ CF
// 26-07-03│ 1.80 │ 1.9: Selbsttests F$STime/F$Time                                        │ CF
// 26-07-03│ 1.90 │ Bugfix: F$CmpNam-Selbsttest nutzt jetzt E_DIFFER($A5)                  │ CF
// 26-07-03│ 2.00 │ Bugfix: F$STime/F$Time-Selbsttest an d0=Zeit/d1=Datum angepasst        │ CF
// 26-07-03│ 2.10 │ 2.1: Selbsttest q9_crc32 (Referenzwert "123456789")                    │ CF
// 26-07-03│ 2.20 │ 2.3a: Selbsttests q9_mod_scan_first/next                              │ CF
// 26-07-03│ 2.30 │ 2.3b-d: Selbsttests Validierung/Directory/F$Link/F$UnLink             │ CF
// 26-07-03│ 2.40 │ 3.1: /d0 im Banner + Selbsttests SS.BlkRd/SS.BlkWr                    │ CF
// 26-07-04│ 2.50 │ 3.2: Selbsttests I$Open/I$ChgDir (VFS-Pfad-Routing, Test-File-Manager)│ CF
// 26-07-04│ 2.60 │ 3.3: Selbsttests FAT16 (8.3/LFN-Datei, Unterverzeichnis, I$Seek);     │ CF
//         │      │ Checks nur aktiv, wenn q9disk.img beim Boot als FAT16 erkannt wurde   │
// 26-07-04│ 2.70 │ 3.4: Selbsttests FAT16 schreibend (I$Create/I$Write/I$MakDir/         │ CF
//         │      │ I$Delete); I$Create/MakDir/Delete-Test ohne fm jetzt gegen /term      │
//         │      │ statt Geruest-Erwartung (echte Semantik ab 3.4)                       │
// 26-07-04│ 2.80 │ 3.5: Selbsttests F$Load (Modul per I$Create/I$Write nach /d0          │ CF
//         │      │ geschrieben, F$Load laedt+validiert+registriert, F$Link findet es     │
//         │      │ danach ueber den Namen, F$UnLink baut beide Referenzen sauber ab,      │
//         │      │ fehlende Datei -> E$PNNF, kaputter Sync -> E$BMHP); Testdateien werden  │
//         │      │ danach per I$Delete wieder entfernt (haelt den FAT16-Cluster-Zustand    │
//         │      │ fuer die 3.4-Nachvalidierung in test/06 unveraendert)                  │
//═════════╧══════╧════════════════════════════════════════════════════════════════════════╧══════

#include "../hal/q9_hal.h"
#include "device.h"
#include "module.h"
#include "syscall.h"
#include "vfs.h"
#include "kernel.h"

//╔══════════════════════════════════════════════════════════════════════════════════════════════╗
//║ INTERNAL HELPERS (thin wrappers over the syscall layer)                                      ║
//╚══════════════════════════════════════════════════════════════════════════════════════════════╝

static uint32_t str_len(const char *s)
{
    uint32_t n = 0;
    while (s[n]) {
        n++;
    }
    return n;
}

//────────────────────────────────────────────────────────────────────────────────────────────────
// Function: kputs
// Desc.:    Gibt einen String über I$Write auf Pfad 1 (stdout) aus.
// Call:     kputs("text\n")
//────────────────────────────────────────────────────────────────────────────────────────────────
static void kputs(const char *s)
{
    q9_regs_t r = {0};

    r.d[0] = 1;                                        /* path 1 = stdout                        */
    r.d[1] = str_len(s);
    r.a[0] = (void *)s;
    q9_syscall(I_WRITE, &r);
}

//╔══════════════════════════════════════════════════════════════════════════════════════════════╗
//║ KERNEL API                                                                                   ║
//╚══════════════════════════════════════════════════════════════════════════════════════════════╝

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_kernel_init
// Desc.:    Kernel-Initialisierung, gibt das Boot-Banner aus (über die Syscall-Schicht).
// Call:     q9_kernel_init()
//════════════════════════════════════════════════════════════════════════════════════════════════
void q9_kernel_init(void)
{
    q9_dev_init();                                     /* device model first — kputs needs it    */

    kputs("\n");
    kputs("  ═══════════════════════════════════════\n");
    kputs("   Q9 v" Q9_VERSION " alpha\n");
    kputs("   modulares Mini-OS in OS-9-Tradition\n");
    kputs("  ═══════════════════════════════════════\n");
    kputs("  target: ");
    kputs(q9_hal_target());
    kputs("\n  syscalls: OS-9-ABI aktiv (docs/SYSCALLS.md)\n");
    kputs("  devices:  /term (Pfade 0/1/2), /nil, /d0\n\n");
    kputs("  Phase 1: REPL. Eingabe wird zurückgegeben, 'exit' beendet.\n\n");
    kputs("Q9> ");
}

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_kernel_step
// Desc.:    Ein Kernel-Tick: I$ReadLn pollen; komplette Zeile -> Antwort + neuer Prompt.
//           "exit" ruft F$Exit (Proto-Prozess hält an).
// Call:     q9_kernel_step()
//════════════════════════════════════════════════════════════════════════════════════════════════
void q9_kernel_step(void)
{
    q9_regs_t r = {0};
    uint8_t   line[80];
    uint32_t  n;

    if (q9_proc_halted()) {
        return;
    }

    r.d[0] = 0;                                        /* path 0 = stdin                         */
    r.d[1] = sizeof(line) - 1;
    r.a[0] = line;
    if (q9_syscall(I_READLN, &r) != 0) {
        return;                                        /* E$NotRdy: line not complete yet        */
    }

    n = r.d[1];
    if (n > 0 && line[n - 1] == '\r') {                /* strip CR for comparing                 */
        n--;
    }
    line[n] = 0;

    if (n == 4 && line[0] == 'e' && line[1] == 'x' && line[2] == 'i' && line[3] == 't') {
        q9_regs_t ex = {0};
        kputs("Prozess beendet (F$Exit). Bis bald!\n");
        q9_syscall(F_EXIT, &ex);
        return;
    }

    if (n > 0) {
        kputs("echo: ");
        kputs((const char *)line);
        kputs("\n");
    }
    kputs("Q9> ");
}

//────────────────────────────────────────────────────────────────────────────────────────────────
// Function: build_module
// Desc.:    Baut ein minimales, gueltiges Q9-Modul (Header + nullterminierter Name direkt danach)
//           in buf und rechnet die CRC32 passend (Feld erst 0, dann q9_crc32 ueber alles).
//           Nur fuer den Selbsttest — buf muss mindestens Q9_MOD_HDRSIZE + strlen(name) + 1 Bytes
//           gross sein.
//────────────────────────────────────────────────────────────────────────────────────────────────
static void build_module(uint8_t *buf, const char *name, uint8_t type, uint8_t lang, uint8_t rev)
{
    q9_modhdr_t *h       = (q9_modhdr_t *)buf;
    uint32_t     namelen = str_len(name);
    uint32_t     modsize = Q9_MOD_HDRSIZE + namelen + 1;

    h->sync[0]  = Q9_MOD_SYNC0;
    h->sync[1]  = Q9_MOD_SYNC1;
    h->hdrsize  = Q9_MOD_HDRSIZE;
    h->modsize  = modsize;
    h->nameoff  = Q9_MOD_HDRSIZE;
    h->type     = type;
    h->lang     = lang;
    h->attr     = 0;
    h->rev      = rev;
    h->execoff  = Q9_MOD_HDRSIZE;                   /* Dummy-Einsprung: zeigt auf den Namen    */
    h->datasize = 0;
    h->crc32    = 0;
    for (uint32_t i = 0; i <= namelen; i++) {
        buf[Q9_MOD_HDRSIZE + i] = (uint8_t)name[i];
    }
    h->crc32 = q9_crc32(buf, modsize);
}

//────────────────────────────────────────────────────────────────────────────────────────────────
// Function: test_fm_open
// Desc.:    Minimaler Test-File-Manager (3.2-Selbsttest): "open" akzeptiert nur den Rest-Pfad
//           "datei", legt dessen Laenge im Datei-Kontext (fmctx) ab. Alles andere -> E$PNNF.
//           Beweist NUR das Routing der VFS-Schicht (device.h/vfs.c) — kein echtes Dateisystem
//           (das kommt in 3.3/3.4 als FAT16-Manager).
//────────────────────────────────────────────────────────────────────────────────────────────────
static int test_fm_open(q9_dev_t *dev, q9_path_t *p, const char *restpath, uint32_t len, uint8_t mode)
{
    (void)dev; (void)mode;
    if (len != 5 || restpath[0] != 'd' || restpath[1] != 'a' || restpath[2] != 't' ||
        restpath[3] != 'e' || restpath[4] != 'i') {
        return E_PNNF;
    }
    p->fmctx[0] = (uint8_t)len;                        /* Beweis: Kontext liegt im Pfad, nicht im */
    return 0;                                          /*   Geraet — pro offenem Pfad eigener Stand */
}

static const q9_fm_t test_fm = { "testfm", test_fm_open, 0, 0, 0, 0, 0, 0 };

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_kernel_selftest
// Desc.:    Syscall-Selbsttest: prüft Erfolgs- und Fehlerpfade der Phase-1-ABI.
//           Rückgabe 0 = alle Checks ok, sonst Anzahl Fehler.
// Call:     fails = q9_kernel_selftest()
//════════════════════════════════════════════════════════════════════════════════════════════════
int q9_kernel_selftest(void)
{
    int fails = 0;

    struct {
        const char *name;
        int         ok;
    } checks[64];
    int nchecks = 0;

    {   /* I$WritLn on stdout succeeds and reports the byte count */
        q9_regs_t r = {0};
        static const char msg[] = "selftest: I$WritLn ok\r";
        r.d[0] = 1;
        r.d[1] = sizeof(msg) - 1;
        r.a[0] = (void *)msg;
        int err = q9_syscall(I_WRITLN, &r);
        checks[nchecks].name = "I$WritLn stdout";
        checks[nchecks++].ok = (err == 0 && r.d[1] == sizeof(msg) - 1);
    }
    {   /* bad path number is rejected */
        q9_regs_t r = {0};
        uint8_t   b = 'x';
        r.d[0] = 7;
        r.d[1] = 1;
        r.a[0] = &b;
        checks[nchecks].name = "I$Write Pfad 7 -> E$BPNum";
        checks[nchecks++].ok = (q9_syscall(I_WRITE, &r) == E_BPNUM);
    }
    {   /* unknown function code is rejected */
        q9_regs_t r = {0};
        checks[nchecks].name = "Func $7F -> E$UnkSvc";
        checks[nchecks++].ok = (q9_syscall(0x7f, &r) == E_UNKSVC);
    }
    {   /* read without pending input reports not-ready (phase 1 semantics) */
        q9_regs_t r = {0};
        uint8_t   buf[8];
        r.d[0] = 0;
        r.d[1] = sizeof(buf);
        r.a[0] = buf;
        checks[nchecks].name = "I$Read leer -> E$NotRdy";
        checks[nchecks++].ok = (q9_syscall(I_READ, &r) == E_NOTRDY);
    }
    {   /* F$ID returns the proto process */
        q9_regs_t r = {0};
        checks[nchecks].name = "F$ID -> PID 1";
        checks[nchecks++].ok = (q9_syscall(F_ID, &r) == 0 && r.d[0] == 1);
    }
    {   /* F$Time delivers uptime */
        q9_regs_t r = {0};
        checks[nchecks].name = "F$Time ok";
        checks[nchecks++].ok = (q9_syscall(F_TIME, &r) == 0);
    }
    {   /* standard paths 0/1/2 sit on the /term device */
        q9_dev_t *term = q9_dev_find("term");
        int ok = (term != 0);
        for (uint32_t i = 0; i < 3; i++) {
            q9_path_t *p = q9_path_get(i);
            ok = ok && p && p->dev == term && p->mode == Q9_MODE_UPDATE;
        }
        checks[nchecks].name = "Pfade 0/1/2 -> /term";
        checks[nchecks++].ok = ok;
    }
    {   /* write on a read-only path is rejected with E$BMode */
        q9_regs_t r = {0};
        uint8_t   b = 'x';
        int path = q9_path_open("term", Q9_MODE_READ);
        r.d[0] = (uint32_t)path;
        r.d[1] = 1;
        r.a[0] = &b;
        checks[nchecks].name = "I$Write auf Lesepfad -> E$BMode";
        checks[nchecks++].ok = (path == 3 && q9_syscall(I_WRITE, &r) == E_BMODE);
    }
    {   /* closing frees the path: same write now yields E$BPNum */
        q9_regs_t r = {0};
        uint8_t   b = 'x';
        r.d[0] = 3;
        r.d[1] = 1;
        r.a[0] = &b;
        checks[nchecks].name = "q9_path_close -> Pfad 3 wieder E$BPNum";
        checks[nchecks++].ok = (q9_path_close(3) == 0 && q9_syscall(I_WRITE, &r) == E_BPNUM);
    }
    {   /* I$Dup clones stdout to the lowest free path, I$Close frees it again */
        q9_regs_t r = {0};
        static const char msg[] = "selftest: I$Dup ok\r";
        int ok;
        r.d[0] = 1;
        ok = (q9_syscall(I_DUP, &r) == 0 && r.d[0] == 3);
        r.d[1] = sizeof(msg) - 1;
        r.a[0] = (void *)msg;
        ok = ok && (q9_syscall(I_WRITLN, &r) == 0);     /* write via the duplicate               */
        checks[nchecks].name = "I$Dup Pfad 1 -> 3, schreibbar";
        checks[nchecks++].ok = ok;

        q9_regs_t c = {0};
        c.d[0] = 3;
        ok = (q9_syscall(I_CLOSE, &c) == 0);
        ok = ok && (q9_syscall(I_WRITLN, &r) == E_BPNUM);
        c.d[0] = 3;
        ok = ok && (q9_syscall(I_CLOSE, &c) == E_BPNUM);
        checks[nchecks].name = "I$Close gibt Duplikat frei";
        checks[nchecks++].ok = ok;
    }
    {   /* F$PrsNam splits "/term/xyz" into name "term" + delimiter '/' */
        q9_regs_t r = {0};
        static const char pl[] = "/term/xyz";
        r.a[0] = (void *)pl;
        int ok = (q9_syscall(F_PRSNAM, &r) == 0 &&
                  r.d[1] == 4 && (const char *)r.a[0] == pl + 1 &&
                  (const char *)r.a[1] == pl + 5 && r.d[0] == '/');
        r.a[0] = r.a[1];                               /* chain: parse the next element         */
        ok = ok && (q9_syscall(F_PRSNAM, &r) == 0 && r.d[1] == 3);
        checks[nchecks].name = "F$PrsNam '/term/xyz' -> term, xyz";
        checks[nchecks++].ok = ok;
    }
    {   /* empty pathlist element is rejected */
        q9_regs_t r = {0};
        r.a[0] = (void *)"//x";
        checks[nchecks].name = "F$PrsNam '//x' -> E$BPNam";
        checks[nchecks++].ok = (q9_syscall(F_PRSNAM, &r) == E_BPNAM);
    }
    {   /* F$CmpNam: case-insensitive match, mismatch -> E$Diff */
        q9_regs_t r = {0};
        r.a[0] = (void *)"TERM";
        r.a[1] = (void *)"term";
        r.d[1] = 4;
        int ok = (q9_syscall(F_CMPNAM, &r) == 0);
        r.a[1] = (void *)"trem";
        ok = ok && (q9_syscall(F_CMPNAM, &r) == E_DIFFER);
        checks[nchecks].name = "F$CmpNam TERM=term, TERM!=trem";
        checks[nchecks++].ok = ok;
    }
    {   /* I$Attach finds /term by pathlist name and bumps the link count */
        q9_regs_t  r = {0};
        q9_dev_t  *term  = q9_dev_find("term");
        uint8_t    links = term ? term->links : 0;
        r.a[0] = (void *)"/TERM";                      /* case-insensitive, leading slash        */
        int ok = (q9_syscall(I_ATTACH, &r) == 0 &&
                  (q9_dev_t *)r.a[2] == term && term->links == links + 1);
        checks[nchecks].name = "I$Attach /TERM -> Geraet term";
        checks[nchecks++].ok = ok;

        q9_regs_t d = {0};
        d.a[2] = r.a[2];
        ok = (q9_syscall(I_DETACH, &d) == 0 && term && term->links == links);
        d.a[2] = (void *)&d;                           /* not a device table entry               */
        ok = ok && (q9_syscall(I_DETACH, &d) == E_PARAM);
        checks[nchecks].name = "I$Detach gibt frei, Fremdzeiger -> E$Param";
        checks[nchecks++].ok = ok;
    }
    {   /* unknown device name is rejected */
        q9_regs_t r = {0};
        r.a[0] = (void *)"/disk0";
        checks[nchecks].name = "I$Attach /disk0 -> E$MNF";
        checks[nchecks++].ok = (q9_syscall(I_ATTACH, &r) == E_MNF);
    }
    {   /* second device /nil: write discards, read reports EOF */
        q9_regs_t r = {0};
        uint8_t   buf[4];
        int path = q9_path_open("/nil", Q9_MODE_UPDATE);
        int ok   = (path == 3);
        r.d[0] = (uint32_t)path;
        r.d[1] = sizeof(buf);
        r.a[0] = buf;
        ok = ok && (q9_syscall(I_WRITE, &r) == 0 && r.d[1] == sizeof(buf));
        r.d[1] = sizeof(buf);
        ok = ok && (q9_syscall(I_READ, &r) == E_EOF);
        ok = ok && (q9_path_close(3) == 0);
        checks[nchecks].name = "/nil: Write verwirft, Read -> E$EOF";
        checks[nchecks++].ok = ok;
    }
    {   /* I$GetStt: SS.Ready without input -> E$NotRdy, SS.EOF on /term -> ok */
        q9_regs_t r = {0};
        r.d[0] = 0;                                    /* stdin                                  */
        r.d[1] = SS_READY;
        int ok = (q9_syscall(I_GETSTT, &r) == E_NOTRDY);
        r.d[1] = SS_EOF;
        ok = ok && (q9_syscall(I_GETSTT, &r) == 0);
        r.d[1] = 0x7f;                                 /* unsupported SS code                    */
        ok = ok && (q9_syscall(I_GETSTT, &r) == E_UNKSVC);
        checks[nchecks].name = "I$GetStt SS.Ready/SS.EOF auf /term";
        checks[nchecks++].ok = ok;
    }
    {   /* I$GetStt SS.EOF on /nil reports E$EOF; I$SetStt has no codes yet */
        q9_regs_t r = {0};
        int path = q9_path_open("/nil", Q9_MODE_READ);
        r.d[0] = (uint32_t)path;
        r.d[1] = SS_EOF;
        int ok = (path == 3 && q9_syscall(I_GETSTT, &r) == E_EOF);
        ok = ok && (q9_syscall(I_SETSTT, &r) == E_UNKSVC);
        ok = ok && (q9_path_close(3) == 0);
        checks[nchecks].name = "I$GetStt /nil SS.EOF -> E$EOF, SetStt -> E$UnkSvc";
        checks[nchecks++].ok = ok;
    }
    {   /* F$STime/F$Time roundtrip: set 2026-07-03 20:15:00, expect it back (Friday) */
        q9_regs_t s = {0};
        q9_regs_t t = {0};
        s.d[0] = (20u << 16) | (15u << 8) | 0u;        /* Zeit: 20:15:00 (d0, MWOS-verifiziert)  */
        s.d[1] = (2026u << 16) | (7u << 8) | 3u;       /* Datum: 2026-07-03 (d1)                 */
        int ok = (q9_syscall(F_STIME, &s) == 0);
        ok = ok && (q9_syscall(F_TIME, &t) == 0);
        ok = ok && (t.d[1] == s.d[1]);                 /* gleiches Datum                         */
        ok = ok && ((t.d[0] >> 8) == (s.d[0] >> 8));   /* gleiche Stunde+Minute                  */
        ok = ok && (t.d[2] == 5);                      /* 2026-07-03 ist ein Freitag             */
        checks[nchecks].name = "F$STime/F$Time Roundtrip + Wochentag";
        checks[nchecks++].ok = ok;

        q9_regs_t b = {0};
        b.d[1] = (2026u << 16) | (13u << 8) | 3u;      /* Monat 13 im Datum (jetzt d1)           */
        checks[nchecks].name = "F$STime Monat 13 -> E$Param";
        checks[nchecks++].ok = (q9_syscall(F_STIME, &b) == E_PARAM);
    }
    {   /* q9_crc32: Standard-Referenzwert fuer CRC-32/ISO-HDLC ueber "123456789" */
        static const uint8_t ref[] = "123456789";
        checks[nchecks].name = "q9_crc32 Referenzwert (123456789 -> $CBF43926)";
        checks[nchecks++].ok = (q9_crc32(ref, sizeof(ref) - 1) == 0xCBF43926u);
    }
    {   /* q9_mod_scan_first/next: zwei Module lueckenlos im ROM-Image, Sprung um ModuleSize */
        static uint8_t rom[2 * Q9_MOD_HDRSIZE];
        q9_modhdr_t   *h1 = (q9_modhdr_t *)rom;
        q9_modhdr_t   *h2 = (q9_modhdr_t *)(rom + Q9_MOD_HDRSIZE);

        h1->sync[0] = Q9_MOD_SYNC0;
        h1->sync[1] = Q9_MOD_SYNC1;
        h1->modsize = Q9_MOD_HDRSIZE;                  /* zweites Modul folgt direkt              */
        h2->sync[0] = Q9_MOD_SYNC0;
        h2->sync[1] = Q9_MOD_SYNC1;
        h2->modsize = Q9_MOD_HDRSIZE;                  /* Sprung fuehrt aus dem Blob heraus        */

        const q9_modhdr_t *first = q9_mod_scan_first(rom, sizeof(rom));
        int ok = (first == h1);
        if (ok) {
            const q9_modhdr_t *second = q9_mod_scan_next(rom, sizeof(rom), first);
            ok = ok && (second == h2);
            ok = ok && second && (q9_mod_scan_next(rom, sizeof(rom), second) == 0);
        }
        checks[nchecks].name = "q9_mod_scan_first/next: 2 Module, Ende erkannt";
        checks[nchecks++].ok = ok;
    }
    {   /* q9_mod_scan_first: kein Sync im Blob -> NULL */
        static uint8_t rom[Q9_MOD_HDRSIZE] = {0};
        checks[nchecks].name = "q9_mod_scan_first ohne Sync -> NULL";
        checks[nchecks++].ok = (q9_mod_scan_first(rom, sizeof(rom)) == 0);
    }
    {   /* 2.3b: q9_mod_validate — gueltiges Modul (Groesse/Nameoffset/CRC ok) */
        static uint8_t buf[64];
        build_module(buf, "vmod", Q9_MOD_DRIVR, Q9_MOD_M68K, 1);
        checks[nchecks].name = "q9_mod_validate: gueltiges Modul -> 0";
        checks[nchecks++].ok = (q9_mod_validate(buf, sizeof(buf), (const q9_modhdr_t *)buf) == 0);
    }
    {   /* 2.3b: kaputte ModuleSize (kleiner als Header) -> E$BMHP */
        static uint8_t buf[64];
        build_module(buf, "vmod", Q9_MOD_DRIVR, Q9_MOD_M68K, 1);
        ((q9_modhdr_t *)buf)->modsize = Q9_MOD_HDRSIZE - 1;
        checks[nchecks].name = "q9_mod_validate: ModuleSize < Header -> E$BMHP";
        checks[nchecks++].ok = (q9_mod_validate(buf, sizeof(buf), (const q9_modhdr_t *)buf) == E_BMHP);
    }
    {   /* 2.3b: NameOffset zeigt aus dem Modul heraus -> E$BMHP */
        static uint8_t buf[64];
        build_module(buf, "vmod", Q9_MOD_DRIVR, Q9_MOD_M68K, 1);
        ((q9_modhdr_t *)buf)->nameoff = ((q9_modhdr_t *)buf)->modsize;
        checks[nchecks].name = "q9_mod_validate: NameOffset ausserhalb -> E$BMHP";
        checks[nchecks++].ok = (q9_mod_validate(buf, sizeof(buf), (const q9_modhdr_t *)buf) == E_BMHP);
    }
    {   /* 2.3b: Datenbyte nach dem Header kaputt -> CRC-Mismatch -> E$BMCRC */
        static uint8_t buf[64];
        build_module(buf, "vmod", Q9_MOD_DRIVR, Q9_MOD_M68K, 1);
        buf[Q9_MOD_HDRSIZE] ^= 0xff;                    /* erstes Namensbyte kippen                */
        checks[nchecks].name = "q9_mod_validate: kaputtes Byte -> E$BMCRC";
        checks[nchecks++].ok = (q9_mod_validate(buf, sizeof(buf), (const q9_modhdr_t *)buf) == E_BMCRC);
    }
    {   /* 2.3c: q9_mod_register + q9_mod_find — Basisfall, danach Revision-Tie-Break */
        static uint8_t bufA[64];
        static uint8_t bufB[64];
        build_module(bufA, "regmod", Q9_MOD_PRGRM, Q9_MOD_M68K, 1);
        build_module(bufB, "regmod", Q9_MOD_PRGRM, Q9_MOD_M68K, 2);   /* hoehere Revision           */

        int ok = (q9_mod_register((const q9_modhdr_t *)bufA) == 0);
        ok = ok && (q9_mod_find("regmod", Q9_MOD_PRGRM, 0) == (const q9_modhdr_t *)bufA);
        ok = ok && (q9_mod_register((const q9_modhdr_t *)bufB) == 0);
        ok = ok && (q9_mod_find("regmod", Q9_MOD_PRGRM, 0) == (const q9_modhdr_t *)bufB);
        checks[nchecks].name = "q9_mod_register/find: hoehere Revision gewinnt";
        checks[nchecks++].ok = ok;

        build_module(bufA, "regmod", Q9_MOD_PRGRM, Q9_MOD_M68K, 2);   /* Gleichstand: A neu gebaut  */
        ok = (q9_mod_register((const q9_modhdr_t *)bufA) == 0);
        ok = ok && (q9_mod_find("regmod", Q9_MOD_PRGRM, 0) == (const q9_modhdr_t *)bufB);
        checks[nchecks].name = "q9_mod_register: Gleichstand -> etabliertes Modul bleibt";
        checks[nchecks++].ok = ok;
    }
    {   /* 2.3d: F$Link/F$UnLink ueber die Directory (regmod ist seit dem vorigen Block bekannt) */
        q9_regs_t r = {0};
        static const char name[] = "regmod";
        r.a[0] = (void *)name;
        r.d[1] = Q9_MOD_PRGRM;
        r.d[2] = 0;                                      /* Language: beliebig                     */
        int ok = (q9_syscall(F_LINK, &r) == 0);
        ok = ok && (r.a[1] != 0) && (r.d[0] == 2);        /* Revision 2 (gewann den Tie-Break oben)  */
        checks[nchecks].name = "F$Link: regmod gefunden, Revision im d0";
        checks[nchecks++].ok = ok;

        q9_regs_t u = {0};
        u.a[1] = r.a[1];
        checks[nchecks].name = "F$UnLink: bekanntes Modul -> 0";
        checks[nchecks++].ok = (q9_syscall(F_UNLINK, &u) == 0);

        q9_regs_t miss = {0};
        static const char noname[] = "keinmodul";
        miss.a[0] = (void *)noname;
        checks[nchecks].name = "F$Link: unbekannter Name -> E$MNF";
        checks[nchecks++].ok = (q9_syscall(F_LINK, &miss) == E_MNF);
    }
    {   /* 3.1: /d0 Block-Device — SS.BlkWr/SS.BlkRd-Roundtrip ueber die HAL (q9disk.img).        */
        /*      LBA 1 wird dafuer benutzt und danach WIEDERHERGESTELLT (3.3: /d0 kann inzwischen  */
        /*      ein echtes FAT16-Image sein, dessen FAT typischerweise bei LBA 1 beginnt — dieser */
        /*      Test darf sie nicht dauerhaft mit Testmuster ueberschreiben). */
        static uint8_t wbuf[Q9_BLK_SIZE];
        static uint8_t rbuf[Q9_BLK_SIZE];
        static uint8_t origbuf[Q9_BLK_SIZE];
        q9_regs_t r = {0};
        int path = q9_path_open("/d0", Q9_MODE_UPDATE);
        int ok = (path == 3);
        int had_orig;

        r.d[0] = (uint32_t)path;
        r.d[1] = SS_BLKRD;
        r.d[2] = 1;                                     /* LBA 1 vorher sichern                   */
        r.a[0] = origbuf;
        had_orig = ok && (q9_syscall(I_GETSTT, &r) == 0);

        for (uint32_t i = 0; i < sizeof(wbuf); i++) {
            wbuf[i] = (uint8_t)(i * 7 + 3);
        }
        r.d[1] = SS_BLKWR;
        r.a[0] = wbuf;
        ok = ok && (q9_syscall(I_SETSTT, &r) == 0);

        r.d[1] = SS_BLKRD;
        r.a[0] = rbuf;
        ok = ok && (q9_syscall(I_GETSTT, &r) == 0);
        for (uint32_t i = 0; ok && i < sizeof(wbuf); i++) {
            ok = ok && (rbuf[i] == wbuf[i]);
        }
        checks[nchecks].name = "/d0: SS.BlkWr/SS.BlkRd Roundtrip (LBA 1)";
        checks[nchecks++].ok = ok;

        if (had_orig) {
            r.d[1] = SS_BLKWR;
            r.a[0] = origbuf;
            q9_syscall(I_SETSTT, &r);                    /* LBA 1 wiederherstellen                 */
        }

        r.d[1] = SS_BLKWR;
        r.a[0] = 0;
        ok = (q9_syscall(I_SETSTT, &r) == E_PARAM);
        r.d[1] = SS_READY;                               /* unbekannt fuer /d0                     */
        ok = ok && (q9_syscall(I_GETSTT, &r) == E_UNKSVC);
        ok = ok && (q9_path_close((uint32_t)path) == 0);
        checks[nchecks].name = "/d0: NULL-Puffer -> E$Param, unbek. SS-Code -> E$UnkSvc";
        checks[nchecks++].ok = ok;
    }
    {   /* 3.2: I$Open auf ein Geraet OHNE File-Manager — Rest-Pfad muss leer sein (Rueckwaerts-  */
        /*      Kompatibilitaet zu Phase 1/2: /term/xyz -> E$PNNF, /term allein -> normaler Pfad) */
        q9_regs_t r = {0};
        r.d[0] = Q9_MODE_UPDATE;
        r.a[0] = (void *)"/term/xyz";
        int ok = (q9_syscall(I_OPEN, &r) == E_PNNF);

        q9_regs_t r2 = {0};
        r2.d[0] = Q9_MODE_UPDATE;
        r2.a[0] = (void *)"/term";
        ok = ok && (q9_syscall(I_OPEN, &r2) == 0) && (r2.d[0] == 3);
        ok = ok && (q9_path_close(3) == 0);
        checks[nchecks].name = "I$Open: /term/xyz -> E$PNNF, /term -> Pfad 3";
        checks[nchecks++].ok = ok;
    }
    {   /* 3.2: I$Open auf ein Geraet MIT File-Manager — Rest-Pfad wird durchgereicht */
        q9_dev_t     *d0 = q9_dev_find("d0");
        const q9_fm_t *saved_fm = d0 ? d0->fm : 0;      /* 3.3 kann hier bereits FAT16 haengen —   */
        int ok = (d0 != 0);                             /*   nach dem Test wiederherstellen         */
        if (ok) {
            q9_dev_set_fm(d0, &test_fm);
        }
        q9_regs_t r = {0};
        r.d[0] = Q9_MODE_READ;
        r.a[0] = (void *)"/d0/datei";
        ok = ok && (q9_syscall(I_OPEN, &r) == 0) && (r.d[0] == 3);
        ok = ok && (q9_path_get(3) != 0) && (q9_path_get(3)->fmctx[0] == 5);
        ok = ok && (q9_path_close(3) == 0);

        q9_regs_t bad = {0};
        bad.d[0] = Q9_MODE_READ;
        bad.a[0] = (void *)"/d0/unbekannt";
        ok = ok && (q9_syscall(I_OPEN, &bad) == E_PNNF);
        if (d0) {
            q9_dev_set_fm(d0, saved_fm);                /* urspruenglichen File-Manager zurueck     */
        }
        checks[nchecks].name = "I$Open: /d0/datei ueber Test-File-Manager, /d0/unbekannt -> E$PNNF";
        checks[nchecks++].ok = ok;
    }
    {   /* 3.2: I$ChgDir setzt das globale Arbeitsverzeichnis, relative I$Open loest dagegen auf */
        q9_dev_t     *d0 = q9_dev_find("d0");
        const q9_fm_t *saved_fm = d0 ? d0->fm : 0;
        if (d0) {
            q9_dev_set_fm(d0, &test_fm);                /* Test-File-Manager fuer den relativen Fall */
        }

        q9_regs_t c = {0};
        c.a[0] = (void *)"/d0";
        int ok = (d0 != 0) && (q9_syscall(I_CHGDIR, &c) == 0);
        {
            const char *cwd = q9_vfs_cwd();
            ok = ok && cwd[0] == '/' && cwd[1] == 'd' && cwd[2] == '0' && cwd[3] == 0;
        }

        q9_regs_t r = {0};
        r.d[0] = Q9_MODE_READ;
        r.a[0] = (void *)"datei";                       /* relativ: kein fuehrender '/'            */
        ok = ok && (q9_syscall(I_OPEN, &r) == 0) && (r.d[0] == 3);
        ok = ok && (q9_path_close(3) == 0);

        q9_regs_t back = {0};
        back.a[0] = (void *)"/";
        ok = ok && (q9_syscall(I_CHGDIR, &back) == 0);
        if (d0) {
            q9_dev_set_fm(d0, saved_fm);                 /* urspruenglichen File-Manager zurueck     */
        }
        checks[nchecks].name = "I$ChgDir /d0 + relatives I$Open 'datei' -> Pfad 3";
        checks[nchecks++].ok = ok;
    }
    {   /* 3.4: I$Create/I$MakDir/I$Delete auf ein Geraet OHNE File-Manager (/term) -> E$UnkSvc
           (kein Dateisystem hinter dem Geraet, also kann es diese Operationen nicht anbieten) */
        q9_regs_t r = {0};
        r.d[0] = Q9_MODE_WRITE;                          /* gueltiger Modus, sonst E$BMode zuerst   */
        r.a[0] = (void *)"/term/neu.txt";
        int ok = (q9_syscall(I_CREATE, &r) == E_UNKSVC);
        q9_regs_t m = {0};
        m.a[0] = (void *)"/term/neudir";
        ok = ok && (q9_syscall(I_MAKDIR, &m) == E_UNKSVC);
        q9_regs_t d = {0};
        d.a[0] = (void *)"/term/xyz";
        ok = ok && (q9_syscall(I_DELETE, &d) == E_UNKSVC);
        checks[nchecks].name = "I$Create/I$MakDir/I$Delete ohne File-Manager -> E$UnkSvc";
        checks[nchecks++].ok = ok;
    }
    {   /* 3.3: FAT16 — nur relevant, wenn q9disk.img beim Boot als FAT16 erkannt wurde (von      */
        /*      test/06_test_fat16.py per Python vorbereitet; sonst hat /d0 kein fm, Checks       */
        /*      werden dann still uebersprungen — kein FEHLER, das Image ist z.B. bei Test 04/05  */
        /*      absichtlich kein FAT16). */
        q9_dev_t *d0 = q9_dev_find("d0");
        if (d0 && d0->fm) {
            q9_regs_t r = {0};
            uint8_t   buf[64];
            int       ok;

            r.d[0] = Q9_MODE_READ;
            r.a[0] = (void *)"/d0/HELLO.TXT";             /* 8.3-Name, per Python-Skript angelegt */
            ok = (q9_syscall(I_OPEN, &r) == 0);
            int path83 = ok ? (int)r.d[0] : -1;
            if (ok) {
                q9_regs_t rr = {0};
                rr.d[0] = (uint32_t)path83;
                rr.d[1] = sizeof(buf);
                rr.a[0] = buf;
                ok = ok && (q9_syscall(I_READ, &rr) == 0) && (rr.d[1] == 12);
                ok = ok && (buf[0] == 'H' && buf[1] == 'a' && buf[11] == '9');  /* "Hallo Q9!!!9" */
                ok = ok && (q9_path_close((uint32_t)path83) == 0);
            }
            checks[nchecks].name = "FAT16: 8.3-Datei /d0/HELLO.TXT lesen (Inhalt+Groesse)";
            checks[nchecks++].ok = ok;

            r = (q9_regs_t){0};
            r.d[0] = Q9_MODE_READ;
            r.a[0] = (void *)"/d0/This is a very long filename.txt";  /* erzwingt LFN-Eintraege   */
            ok = (q9_syscall(I_OPEN, &r) == 0);
            int pathlfn = ok ? (int)r.d[0] : -1;
            if (ok) {
                q9_regs_t rr = {0};
                rr.d[0] = (uint32_t)pathlfn;
                rr.d[1] = sizeof(buf);
                rr.a[0] = buf;
                ok = ok && (q9_syscall(I_READ, &rr) == 0) && (rr.d[1] == 9);
                ok = ok && (buf[0] == 'L' && buf[8] == '!');            /* "Langname!" */
                ok = ok && (q9_path_close((uint32_t)pathlfn) == 0);
            }
            checks[nchecks].name = "FAT16: LFN-Datei (langer Name) lesen";
            checks[nchecks++].ok = ok;

            r = (q9_regs_t){0};
            r.d[0] = Q9_MODE_READ;
            r.a[0] = (void *)"/d0/SUBDIR/NESTED.TXT";      /* Unterverzeichnis + Datei darin       */
            ok = (q9_syscall(I_OPEN, &r) == 0);
            int pathsub = ok ? (int)r.d[0] : -1;
            if (ok) {
                q9_regs_t rr = {0};
                rr.d[0] = (uint32_t)pathsub;
                rr.d[1] = sizeof(buf);
                rr.a[0] = buf;
                ok = ok && (q9_syscall(I_READ, &rr) == 0) && (rr.d[1] > 0);
                ok = ok && (q9_path_close((uint32_t)pathsub) == 0);
            }
            checks[nchecks].name = "FAT16: Datei in Unterverzeichnis lesen";
            checks[nchecks++].ok = ok;

            r = (q9_regs_t){0};
            r.d[0] = Q9_MODE_READ;
            r.a[0] = (void *)"/d0/HELLO.TXT";
            ok = (q9_syscall(I_OPEN, &r) == 0);
            int pathseek = ok ? (int)r.d[0] : -1;
            if (ok) {
                q9_regs_t sk = {0};
                q9_regs_t rr = {0};
                sk.d[0] = (uint32_t)pathseek;
                sk.d[1] = 6;                               /* I$Seek auf Position 6                */
                ok = ok && (q9_syscall(I_SEEK, &sk) == 0);
                rr.d[0] = (uint32_t)pathseek;
                rr.d[1] = sizeof(buf);
                rr.a[0] = buf;
                ok = ok && (q9_syscall(I_READ, &rr) == 0) && (rr.d[1] == 6);
                ok = ok && (buf[0] == 'Q');                 /* "Hallo Q9!!!9"[6..] == "Q9!!!9"       */
                ok = ok && (q9_path_close((uint32_t)pathseek) == 0);
            }
            checks[nchecks].name = "FAT16: I$Seek + I$Read ab Position 6";
            checks[nchecks++].ok = ok;

            r = (q9_regs_t){0};
            r.d[0] = Q9_MODE_READ;
            r.a[0] = (void *)"/d0/NICHTDA.TXT";
            checks[nchecks].name = "FAT16: unbekannte Datei -> E$PNNF";
            checks[nchecks++].ok = (q9_syscall(I_OPEN, &r) == E_PNNF);

            /* 3.4: FAT16 schreibend — I$Create, I$Write (ueber Cluster-Grenzen, um Allozieren zu   */
            /* pruefen), I$Open erneut lesend, I$MakDir, I$Delete. */
            {
                q9_regs_t cr = {0};
                cr.d[0] = Q9_MODE_WRITE;
                cr.a[0] = (void *)"/d0/NEU.TXT";
                ok = (q9_syscall(I_CREATE, &cr) == 0);
                int pathnew = ok ? (int)cr.d[0] : -1;
                if (ok) {
                    static const uint8_t payload[] = "Von Q9 geschrieben!";
                    q9_regs_t wr = {0};
                    wr.d[0] = (uint32_t)pathnew;
                    wr.d[1] = sizeof(payload) - 1;
                    wr.a[0] = (void *)payload;
                    ok = ok && (q9_syscall(I_WRITE, &wr) == 0) && (wr.d[1] == sizeof(payload) - 1);
                    ok = ok && (q9_path_close((uint32_t)pathnew) == 0);
                }
                checks[nchecks].name = "FAT16: I$Create + I$Write /d0/NEU.TXT";
                checks[nchecks++].ok = ok;

                r = (q9_regs_t){0};
                r.d[0] = Q9_MODE_READ;
                r.a[0] = (void *)"/d0/NEU.TXT";
                ok = (q9_syscall(I_OPEN, &r) == 0);
                int pathread = ok ? (int)r.d[0] : -1;
                if (ok) {
                    q9_regs_t rr = {0};
                    rr.d[0] = (uint32_t)pathread;
                    rr.d[1] = sizeof(buf);
                    rr.a[0] = buf;
                    ok = ok && (q9_syscall(I_READ, &rr) == 0) && (rr.d[1] == 19);
                    ok = ok && (buf[0] == 'V' && buf[18] == '!');
                    ok = ok && (q9_path_close((uint32_t)pathread) == 0);
                }
                checks[nchecks].name = "FAT16: neu geschriebene Datei zurueckgelesen";
                checks[nchecks++].ok = ok;

                {
                    q9_regs_t dup = {0};
                    dup.d[0] = Q9_MODE_WRITE;
                    dup.a[0] = (void *)"/d0/NEU.TXT";       /* existierender Name -> E$BPNam         */
                    checks[nchecks].name = "FAT16: I$Create auf existierenden Namen -> E$BPNam";
                    checks[nchecks++].ok = (q9_syscall(I_CREATE, &dup) == E_BPNAM);
                }

                {
                    q9_regs_t bad = {0};
                    bad.d[0] = Q9_MODE_WRITE;
                    bad.a[0] = (void *)"/d0/zulangerdateiname.txt"; /* kein gueltiger 8.3-Name */
                    checks[nchecks].name = "FAT16: I$Create mit LFN-pflichtigem Namen -> E$BPNam";
                    checks[nchecks++].ok = (q9_syscall(I_CREATE, &bad) == E_BPNAM);
                }

                {
                    /* NEUDIR bleibt bewusst bestehen (keine I$Delete-Aufraeumung hier) — die       */
                    /* Python-Nachvalidierung in test/06_test_fat16.py (post_validate) prueft nach   */
                    /* dem Selbsttest-Lauf explizit, dass NEUDIR als Verzeichnis mit korrekten       */
                    /* "."/".."-Eintraegen im Root steht (das ist der praktikable Ersatz fuer ein    */
                    /* echtes "am Mac mounten", ARBEITSPLAN.md 3.4). Das Image wird von test/06 vor  */
                    /* jedem Lauf komplett neu erzeugt (Python "wb"), daher keine Notwendigkeit,      */
                    /* NEUDIR danach wieder zu loeschen. */
                    q9_regs_t md = {0};
                    md.a[0] = (void *)"/d0/NEUDIR";
                    ok = (q9_syscall(I_MAKDIR, &md) == 0);
                    q9_regs_t r2 = {0};
                    r2.d[0] = Q9_MODE_READ;
                    r2.a[0] = (void *)"/d0/NEUDIR";
                    ok = ok && (q9_syscall(I_OPEN, &r2) == 0);
                    ok = ok && (q9_path_close(r2.d[0]) == 0);
                    checks[nchecks].name = "FAT16: I$MakDir /d0/NEUDIR + I$Open darauf";
                    checks[nchecks++].ok = ok;
                }

                /* 3.5: F$Load — Modul aus einer echten Datei (statt nur ROM-Image) laden, validieren,
                   registrieren. Q9 schreibt sich das Testmodul selbst per I$Create/I$Write auf /d0
                   (Nagelprobe Phase 2 + 3 zusammen: derselbe FAT16-Schreibpfad wie oben, diesmal mit
                   echtem Modul-Byte-Inhalt statt Text). Bewusst VOR dem I$Delete von /d0/NEU.TXT
                   platziert (statt danach): dir_alloc_slot (fat16.c) vergibt neue Directory-Slots
                   zuerst an bereits geloeschte/freie Eintraege — liefe dieser Block NACH dem
                   NEU.TXT-Delete, wuerde I$Create(/d0/LOADMOD.BIN) genau dessen frisch freigewordenen
                   Slot (und Cluster 6) wiederverwenden und damit den DIRENT_FREE-Marker ueberschreiben,
                   den die Python-Nachvalidierung in test/06_test_fat16.py (post_validate) anschliessend
                   erwartet. Aufraeumen per I$Delete am Ende dieses Blocks stellt den Zustand vor dem
                   Block wieder her (eigene Slots/Cluster wieder frei), sodass der NEU.TXT-Test danach
                   unveraendert funktioniert. */
                {
                    static uint8_t modbuf[128];
                    build_module(modbuf, "loadmod", Q9_MOD_PRGRM, Q9_MOD_M68K, 1);
                    uint32_t modlen = ((const q9_modhdr_t *)modbuf)->modsize;

                    q9_regs_t cr = {0};
                    cr.d[0] = Q9_MODE_WRITE;
                    cr.a[0] = (void *)"/d0/LOADMOD.BIN";
                    ok = (q9_syscall(I_CREATE, &cr) == 0);
                    int pathmod = ok ? (int)cr.d[0] : -1;
                    if (ok) {
                        q9_regs_t wr = {0};
                        wr.d[0] = (uint32_t)pathmod;
                        wr.d[1] = modlen;
                        wr.a[0] = (void *)modbuf;
                        ok = ok && (q9_syscall(I_WRITE, &wr) == 0) && (wr.d[1] == modlen);
                        ok = ok && (q9_path_close((uint32_t)pathmod) == 0);
                    }
                    checks[nchecks].name = "F$Load: Testmodul nach /d0/LOADMOD.BIN geschrieben";
                    checks[nchecks++].ok = ok;

                    q9_regs_t ld = {0};
                    ld.a[0] = (void *)"/d0/LOADMOD.BIN";
                    ok = (q9_syscall(F_LOAD, &ld) == 0);
                    ok = ok && (ld.a[1] != 0) && (ld.d[0] == 1);      /* Revision 1                */
                    ok = ok && (((const q9_modhdr_t *)ld.a[1])->execoff == Q9_MOD_HDRSIZE);
                    ok = ok && ((const uint8_t *)ld.a[2] == (const uint8_t *)ld.a[1] + Q9_MOD_HDRSIZE);
                    checks[nchecks].name = "F$Load: /d0/LOADMOD.BIN geladen, validiert, registriert";
                    checks[nchecks++].ok = ok;

                    /* Nagelprobe: das per F$Load registrierte Modul ist jetzt ganz normal per
                       F$Link ueber seinen Namen ansprechbar — genau wie ein ROM-Modul. */
                    q9_regs_t lk = {0};
                    static const char loadmodname[] = "loadmod";
                    lk.a[0] = (void *)loadmodname;
                    lk.d[1] = Q9_MOD_PRGRM;
                    ok = (q9_syscall(F_LINK, &lk) == 0);
                    ok = ok && (lk.a[1] == ld.a[1]) && (lk.d[0] == 1);
                    checks[nchecks].name = "F$Load + F$Link: dasselbe Modul ueber den Namen erreichbar";
                    checks[nchecks++].ok = ok;

                    q9_regs_t un1 = {0};
                    un1.a[1] = ld.a[1];
                    ok = (q9_syscall(F_UNLINK, &un1) == 0);           /* F$Load-Referenz senken     */
                    q9_regs_t un2 = {0};
                    un2.a[1] = lk.a[1];
                    ok = ok && (q9_syscall(F_UNLINK, &un2) == 0);     /* F$Link-Referenz senken     */
                    checks[nchecks].name = "F$UnLink: beide Referenzen auf loadmod sauber abgebaut";
                    checks[nchecks++].ok = ok;

                    q9_regs_t missing = {0};
                    missing.a[0] = (void *)"/d0/NICHTDA.MOD";
                    checks[nchecks].name = "F$Load: fehlende Datei -> E$PNNF";
                    checks[nchecks++].ok = (q9_syscall(F_LOAD, &missing) == E_PNNF);

                    /* Ungueltiges Modul (kaputte Sync-Bytes) -> E$BMHP, kein Directory-Eintrag */
                    {
                        static uint8_t badbuf[64];
                        build_module(badbuf, "badmod", Q9_MOD_PRGRM, Q9_MOD_M68K, 1);
                        badbuf[0] = 0x00;                              /* Sync kaputt -> Header ungueltig */
                        uint32_t badlen = ((const q9_modhdr_t *)badbuf)->modsize;

                        q9_regs_t bcr = {0};
                        bcr.d[0] = Q9_MODE_WRITE;
                        bcr.a[0] = (void *)"/d0/BADMOD.BIN";
                        int bok = (q9_syscall(I_CREATE, &bcr) == 0);
                        int pathbad = bok ? (int)bcr.d[0] : -1;
                        if (bok) {
                            q9_regs_t bwr = {0};
                            bwr.d[0] = (uint32_t)pathbad;
                            bwr.d[1] = badlen;
                            bwr.a[0] = (void *)badbuf;
                            bok = bok && (q9_syscall(I_WRITE, &bwr) == 0);
                            bok = bok && (q9_path_close((uint32_t)pathbad) == 0);
                        }
                        q9_regs_t bld = {0};
                        bld.a[0] = (void *)"/d0/BADMOD.BIN";
                        bok = bok && (q9_syscall(F_LOAD, &bld) == E_BMHP);
                        checks[nchecks].name = "F$Load: kaputter Sync -> E$BMHP, kein Directory-Eintrag";
                        checks[nchecks++].ok = bok;

                        q9_regs_t bdel = {0};
                        bdel.a[0] = (void *)"/d0/BADMOD.BIN";
                        q9_syscall(I_DELETE, &bdel);           /* Cluster/Directory-Slot wieder frei  */
                    }

                    /* Aufraeumen: /d0/LOADMOD.BIN wieder loeschen, damit dieser Block denselben
                       FAT16-Zustand (freie Cluster/Directory-Slots) hinterlaesst wie er ihn vorfand —
                       die Python-Nachvalidierung in test/06_test_fat16.py prueft feste Cluster-Nummern
                       fuer NEU.TXT/NEUDIR und wuerde sonst durch die zusaetzlichen Dateien hier
                       verfaelscht. */
                    q9_regs_t delmod = {0};
                    delmod.a[0] = (void *)"/d0/LOADMOD.BIN";
                    q9_syscall(I_DELETE, &delmod);
                }

                {
                    q9_regs_t del = {0};
                    del.a[0] = (void *)"/d0/NEU.TXT";
                    ok = (q9_syscall(I_DELETE, &del) == 0);
                    q9_regs_t r2 = {0};
                    r2.d[0] = Q9_MODE_READ;
                    r2.a[0] = (void *)"/d0/NEU.TXT";
                    ok = ok && (q9_syscall(I_OPEN, &r2) == E_PNNF);   /* wirklich weg */
                    checks[nchecks].name = "FAT16: I$Delete /d0/NEU.TXT, danach E$PNNF";
                    checks[nchecks++].ok = ok;
                }
            }
        }
    }

    for (int i = 0; i < nchecks; i++) {
        kputs(checks[i].ok ? "  [ok] " : "  [FEHLER] ");
        kputs(checks[i].name);
        kputs("\n");
        if (!checks[i].ok) {
            fails++;
        }
    }
    kputs(fails == 0 ? "SYSCALL TEST PASS\n" : "SYSCALL TEST FAIL\n");
    return fails;
}

//────────────────────────────────────────────────────────────────────────────────────────────────
// EOF kernel.c                                                                            Ver. 2.70
//────────────────────────────────────────────────────────────────────────────────────────────────
