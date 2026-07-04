//════════════════════════════════════════════════════════════════════════════════════════════════
// File:   libq9.c                                                                         Ver. 1.00
// Owner:  AF
// Desc.:  Implementierung der kleinen Q9-Userland-Bibliothek. Alle Funktionen geben 0 oder den
//         positiven Q9-Fehlercode aus syscall.h zurueck; keine eigenen Fehlernummern.
//
// Call:   err = q9_open("/d0/DATEI.TXT", Q9_MODE_READ, &path)
//
// Edition History
//─────────┬──────┬────────────────────────────────────────────────────────────────────────┬──────
// Date    │ Ver. │ Description                                                            │ By
//─────────┼──────┼────────────────────────────────────────────────────────────────────────┼──────
// 26-07-04│ 1.00 │ Initiale Version: duenne Wrapper ueber q9_syscall                       │ CX
// 26-07-04│ 1.01 │ q9_read_exact fuer feste Blockgroessen ergaenzt                         │ CX
//═════════╧══════╧════════════════════════════════════════════════════════════════════════╧══════

#include "libq9.h"

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: check_ptr
// Desc.:    Interner Nullpointer-Check fuer Pflichtzeiger; liefert 0 oder E$BPAddr.
// Call:     err = check_ptr(ptr)
//════════════════════════════════════════════════════════════════════════════════════════════════
static int check_ptr(const void *p)
{
    return p ? 0 : E_BPADDR;
}

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_attach
// Desc.:    Wrapper fuer I$Attach; siehe libq9.h. Setzt Mode/Name in Register und liest A2 aus.
// Call:     err = q9_attach("d0", Q9_MODE_READ, &dev)
//════════════════════════════════════════════════════════════════════════════════════════════════
int q9_attach(const char *name, uint8_t mode, void **out_device)
{
    q9_regs_t r = {0};
    int       err;

    if (!name || !out_device) {
        return E_BPADDR;
    }
    r.d[0] = mode;
    r.a[0] = (void *)name;
    err = q9_syscall(I_ATTACH, &r);
    if (err == 0) {
        *out_device = r.a[2];
    }
    return err;
}

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_detach
// Desc.:    Wrapper fuer I$Detach; siehe libq9.h. Erwartet den Device-Zeiger in A2.
// Call:     err = q9_detach(dev)
//════════════════════════════════════════════════════════════════════════════════════════════════
int q9_detach(void *device)
{
    q9_regs_t r = {0};

    if (!device) {
        return E_BPADDR;
    }
    r.a[2] = device;
    return q9_syscall(I_DETACH, &r);
}

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_open
// Desc.:    Wrapper fuer I$Open; siehe libq9.h. Liefert die Pfadnummer aus D0.
// Call:     err = q9_open("/d0/DATEI.TXT", Q9_MODE_READ, &path)
//════════════════════════════════════════════════════════════════════════════════════════════════
int q9_open(const char *path, uint8_t mode, uint16_t *out_path)
{
    q9_regs_t r = {0};
    int       err;

    if (!path || !out_path) {
        return E_BPADDR;
    }
    r.d[0] = mode;
    r.a[0] = (void *)path;
    err = q9_syscall(I_OPEN, &r);
    if (err == 0) {
        *out_path = (uint16_t)r.d[0];
    }
    return err;
}

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_create
// Desc.:    Wrapper fuer I$Create; siehe libq9.h. Liefert die neue Pfadnummer aus D0.
// Call:     err = q9_create("/d0/NEU.TXT", Q9_MODE_WRITE, &path)
//════════════════════════════════════════════════════════════════════════════════════════════════
int q9_create(const char *path, uint8_t mode, uint16_t *out_path)
{
    q9_regs_t r = {0};
    int       err;

    if (!path || !out_path) {
        return E_BPADDR;
    }
    r.d[0] = mode;
    r.a[0] = (void *)path;
    err = q9_syscall(I_CREATE, &r);
    if (err == 0) {
        *out_path = (uint16_t)r.d[0];
    }
    return err;
}

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_close
// Desc.:    Wrapper fuer I$Close; siehe libq9.h. Uebergibt die Pfadnummer in D0.
// Call:     err = q9_close(path)
//════════════════════════════════════════════════════════════════════════════════════════════════
int q9_close(uint16_t path)
{
    q9_regs_t r = {0};

    r.d[0] = path;
    return q9_syscall(I_CLOSE, &r);
}

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_dup
// Desc.:    Wrapper fuer I$Dup; siehe libq9.h. Liefert die duplizierte Pfadnummer aus D0.
// Call:     err = q9_dup(path, &copy)
//════════════════════════════════════════════════════════════════════════════════════════════════
int q9_dup(uint16_t path, uint16_t *out_path)
{
    q9_regs_t r = {0};
    int       err;

    if (!out_path) {
        return E_BPADDR;
    }
    r.d[0] = path;
    err = q9_syscall(I_DUP, &r);
    if (err == 0) {
        *out_path = (uint16_t)r.d[0];
    }
    return err;
}

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: io_rw
// Desc.:    Gemeinsamer Unterbau fuer I$Read/I$Write/I$ReadLn/I$WritLn; func waehlt den Syscall,
//           path/buf/len gehen in D0/A0/D1, out_len bekommt optional D1 zurueck.
// Call:     err = io_rw(I_READ, path, buf, len, &done)
//════════════════════════════════════════════════════════════════════════════════════════════════
static int io_rw(uint16_t func, uint16_t path, const void *buf, uint32_t len, uint32_t *out_len)
{
    q9_regs_t r = {0};
    int       err;

    if (!buf) {
        return E_BPADDR;
    }
    r.d[0] = path;
    r.d[1] = len;
    r.a[0] = (void *)buf;
    err = q9_syscall(func, &r);
    if (err == 0 && out_len) {
        *out_len = r.d[1];
    }
    return err;
}

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_read
// Desc.:    Wrapper fuer I$Read ueber io_rw; siehe libq9.h.
// Call:     err = q9_read(path, buf, sizeof(buf), &got)
//════════════════════════════════════════════════════════════════════════════════════════════════
int q9_read(uint16_t path, void *buf, uint32_t maxlen, uint32_t *out_len)
{
    return io_rw(I_READ, path, buf, maxlen, out_len);
}

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_read_exact
// Desc.:    Wiederholt q9_read(), bis len Bytes erreicht sind oder EOF/Fehler kommt; out_len
//           meldet auch im Fehlerfall die bereits gelesenen Bytes.
// Call:     err = q9_read_exact(path, block, sizeof(block), &got)
//════════════════════════════════════════════════════════════════════════════════════════════════
int q9_read_exact(uint16_t path, void *buf, uint32_t len, uint32_t *out_len)
{
    uint8_t  *p = (uint8_t *)buf;
    uint32_t  total = 0;
    int       err;

    if (!buf) {
        return E_BPADDR;
    }
    while (total < len) {
        uint32_t got = 0;

        err = q9_read(path, &p[total], len - total, &got);
        if (err != 0) {
            if (out_len) {
                *out_len = total;
            }
            return err;
        }
        if (got == 0) {
            break;
        }
        total += got;
    }
    if (out_len) {
        *out_len = total;
    }
    return total == len ? 0 : E_EOF;
}

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_write
// Desc.:    Wrapper fuer I$Write ueber io_rw; siehe libq9.h.
// Call:     err = q9_write(path, buf, len, &put)
//════════════════════════════════════════════════════════════════════════════════════════════════
int q9_write(uint16_t path, const void *buf, uint32_t len, uint32_t *out_len)
{
    return io_rw(I_WRITE, path, buf, len, out_len);
}

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_readln
// Desc.:    Wrapper fuer I$ReadLn ueber io_rw; siehe libq9.h.
// Call:     err = q9_readln(path, line, sizeof(line), &got)
//════════════════════════════════════════════════════════════════════════════════════════════════
int q9_readln(uint16_t path, void *buf, uint32_t maxlen, uint32_t *out_len)
{
    return io_rw(I_READLN, path, buf, maxlen, out_len);
}

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_writln
// Desc.:    Wrapper fuer I$WritLn ueber io_rw; siehe libq9.h.
// Call:     err = q9_writln(1, line, len, &put)
//════════════════════════════════════════════════════════════════════════════════════════════════
int q9_writln(uint16_t path, const void *buf, uint32_t len, uint32_t *out_len)
{
    return io_rw(I_WRITLN, path, buf, len, out_len);
}

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_seek
// Desc.:    Wrapper fuer I$Seek; siehe libq9.h. Uebergibt Pfadnummer und absolute Position.
// Call:     err = q9_seek(path, 0)
//════════════════════════════════════════════════════════════════════════════════════════════════
int q9_seek(uint16_t path, uint32_t pos)
{
    q9_regs_t r = {0};

    r.d[0] = path;
    r.d[1] = pos;
    return q9_syscall(I_SEEK, &r);
}

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: path_only
// Desc.:    Gemeinsamer Unterbau fuer Pfad-Syscalls mit nur einem Pfadnamen in A0.
// Call:     err = path_only(I_DELETE, path)
//════════════════════════════════════════════════════════════════════════════════════════════════
static int path_only(uint16_t func, const char *path)
{
    q9_regs_t r = {0};

    if (!path) {
        return E_BPADDR;
    }
    r.a[0] = (void *)path;
    return q9_syscall(func, &r);
}

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_delete
// Desc.:    Wrapper fuer I$Delete ueber path_only; siehe libq9.h.
// Call:     err = q9_delete("/d0/ALT.TXT")
//════════════════════════════════════════════════════════════════════════════════════════════════
int q9_delete(const char *path)
{
    return path_only(I_DELETE, path);
}

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_makdir
// Desc.:    Wrapper fuer I$MakDir ueber path_only; siehe libq9.h.
// Call:     err = q9_makdir("/d0/NEUDIR")
//════════════════════════════════════════════════════════════════════════════════════════════════
int q9_makdir(const char *path)
{
    return path_only(I_MAKDIR, path);
}

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_chgdir
// Desc.:    Wrapper fuer I$ChgDir ueber path_only; siehe libq9.h.
// Call:     err = q9_chgdir("/d0/NEUDIR")
//════════════════════════════════════════════════════════════════════════════════════════════════
int q9_chgdir(const char *path)
{
    return path_only(I_CHGDIR, path);
}

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: stat_call
// Desc.:    Gemeinsamer Unterbau fuer I$GetStt/I$SetStt; setzt Pfad und Code in regs vor dem
//           Syscall, weitere Register bleiben Aufrufer-gesteuert.
// Call:     err = stat_call(I_GETSTT, path, code, &regs)
//════════════════════════════════════════════════════════════════════════════════════════════════
static int stat_call(uint16_t func, uint16_t path, uint16_t code, q9_regs_t *regs)
{
    if (!regs) {
        return E_BPADDR;
    }
    regs->d[0] = path;
    regs->d[1] = code;
    return q9_syscall(func, regs);
}

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_getstt
// Desc.:    Wrapper fuer I$GetStt ueber stat_call; siehe libq9.h.
// Call:     err = q9_getstt(path, code, &regs)
//════════════════════════════════════════════════════════════════════════════════════════════════
int q9_getstt(uint16_t path, uint16_t code, q9_regs_t *regs)
{
    return stat_call(I_GETSTT, path, code, regs);
}

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_setstt
// Desc.:    Wrapper fuer I$SetStt ueber stat_call; siehe libq9.h.
// Call:     err = q9_setstt(path, code, &regs)
//════════════════════════════════════════════════════════════════════════════════════════════════
int q9_setstt(uint16_t path, uint16_t code, q9_regs_t *regs)
{
    return stat_call(I_SETSTT, path, code, regs);
}

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_prsnam
// Desc.:    Wrapper fuer F$PrsNam; siehe libq9.h. Kopiert A0/A1/D1/D0 in q9_name_parse_t.
// Call:     err = q9_prsnam("/d0/DATEI.TXT", &name)
//════════════════════════════════════════════════════════════════════════════════════════════════
int q9_prsnam(const char *pathlist, q9_name_parse_t *out_name)
{
    q9_regs_t r = {0};
    int       err;

    if (!pathlist || !out_name) {
        return E_BPADDR;
    }
    r.a[0] = (void *)pathlist;
    err = q9_syscall(F_PRSNAM, &r);
    if (err == 0) {
        out_name->name  = (const char *)r.a[0];
        out_name->next  = (const char *)r.a[1];
        out_name->len   = (uint16_t)r.d[1];
        out_name->delim = (uint8_t)r.d[0];
    }
    return err;
}

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_cmpnam
// Desc.:    Wrapper fuer F$CmpNam; siehe libq9.h. Vergleicht A0/A1 ueber D1 Zeichen.
// Call:     err = q9_cmpnam("D0", "d0", 2)
//════════════════════════════════════════════════════════════════════════════════════════════════
int q9_cmpnam(const char *a, const char *b, uint16_t len)
{
    q9_regs_t r = {0};

    if (!a || !b) {
        return E_BPADDR;
    }
    r.a[0] = (void *)a;
    r.a[1] = (void *)b;
    r.d[1] = len;
    return q9_syscall(F_CMPNAM, &r);
}

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_id
// Desc.:    Wrapper fuer F$ID; siehe libq9.h. Liefert Prozess-ID aus D0 und User-ID aus D1.
// Call:     err = q9_id(&pid, &uid)
//════════════════════════════════════════════════════════════════════════════════════════════════
int q9_id(uint16_t *out_pid, uint32_t *out_uid)
{
    q9_regs_t r = {0};
    int       err;

    if (!out_pid || !out_uid) {
        return E_BPADDR;
    }
    err = q9_syscall(F_ID, &r);
    if (err == 0) {
        *out_pid = (uint16_t)r.d[0];
        *out_uid = r.d[1];
    }
    return err;
}

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_link
// Desc.:    Wrapper fuer F$Link; siehe libq9.h. Optionale Out-Parameter werden nur bei Erfolg
//           geschrieben.
// Call:     err = q9_link("term", type, lang, &hdr, &ent, &rev)
//════════════════════════════════════════════════════════════════════════════════════════════════
int q9_link(const char *name, uint8_t type, uint8_t lang,
            void **out_header, void **out_entry, uint8_t *out_rev)
{
    q9_regs_t r = {0};
    int       err;

    if (!name) {
        return E_BPADDR;
    }
    r.a[0] = (void *)name;
    r.d[1] = type;
    r.d[2] = lang;
    err = q9_syscall(F_LINK, &r);
    if (err == 0) {
        if (out_header) {
            *out_header = r.a[1];
        }
        if (out_entry) {
            *out_entry = r.a[2];
        }
        if (out_rev) {
            *out_rev = (uint8_t)r.d[0];
        }
    }
    return err;
}

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_unlink
// Desc.:    Wrapper fuer F$UnLink; siehe libq9.h. Uebergibt den Modul-Header in A1.
// Call:     err = q9_unlink(hdr)
//════════════════════════════════════════════════════════════════════════════════════════════════
int q9_unlink(void *header)
{
    q9_regs_t r = {0};

    if (!header) {
        return E_BPADDR;
    }
    r.a[1] = header;
    return q9_syscall(F_UNLINK, &r);
}

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_load
// Desc.:    Wrapper fuer F$Load; siehe libq9.h. Optionale Out-Parameter werden nur bei Erfolg
//           geschrieben.
// Call:     err = q9_load("/d0/MOD", &hdr, &ent, &rev)
//════════════════════════════════════════════════════════════════════════════════════════════════
int q9_load(const char *path, void **out_header, void **out_entry, uint8_t *out_rev)
{
    q9_regs_t r = {0};
    int       err;

    if (!path) {
        return E_BPADDR;
    }
    r.a[0] = (void *)path;
    err = q9_syscall(F_LOAD, &r);
    if (err == 0) {
        if (out_header) {
            *out_header = r.a[1];
        }
        if (out_entry) {
            *out_entry = r.a[2];
        }
        if (out_rev) {
            *out_rev = (uint8_t)r.d[0];
        }
    }
    return err;
}

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_time
// Desc.:    Wrapper fuer F$Time; siehe libq9.h. Entpackt Zeit/Datum aus D0/D1 und Ticks aus D3.
// Call:     err = q9_time(&time)
//════════════════════════════════════════════════════════════════════════════════════════════════
int q9_time(q9_time_t *out_time)
{
    q9_regs_t r = {0};
    int       err;

    if (check_ptr(out_time) != 0) {
        return E_BPADDR;
    }
    err = q9_syscall(F_TIME, &r);
    if (err == 0) {
        out_time->hour     = (uint8_t)(r.d[0] >> 16);
        out_time->min      = (uint8_t)(r.d[0] >> 8);
        out_time->sec      = (uint8_t)r.d[0];
        out_time->year     = (uint16_t)(r.d[1] >> 16);
        out_time->month    = (uint8_t)(r.d[1] >> 8);
        out_time->day      = (uint8_t)r.d[1];
        out_time->weekday  = (uint8_t)r.d[2];
        out_time->ticks_ms = r.d[3];
    }
    return err;
}

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_stime
// Desc.:    Wrapper fuer F$STime; siehe libq9.h. Packt Uhrzeit in D0 und Datum in D1.
// Call:     err = q9_stime(&time)
//════════════════════════════════════════════════════════════════════════════════════════════════
int q9_stime(const q9_time_t *time)
{
    q9_regs_t r = {0};

    if (check_ptr(time) != 0) {
        return E_BPADDR;
    }
    r.d[0] = ((uint32_t)time->hour << 16) | ((uint32_t)time->min << 8) | time->sec;
    r.d[1] = ((uint32_t)time->year << 16) | ((uint32_t)time->month << 8) | time->day;
    return q9_syscall(F_STIME, &r);
}

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_exit
// Desc.:    Wrapper fuer F$Exit; siehe libq9.h. Der Kernel behandelt das aktuell als Stub.
// Call:     err = q9_exit(0)
//════════════════════════════════════════════════════════════════════════════════════════════════
int q9_exit(uint16_t status)
{
    q9_regs_t r = {0};

    /* F$Exit ist im Kernel aktuell nur ein Phase-1/4-Uebergangsstub:
       er setzt "proto process halted"; echte Prozess-Semantik kommt spaeter. */
    r.d[1] = status;
    return q9_syscall(F_EXIT, &r);
}

//────────────────────────────────────────────────────────────────────────────────────────────────
// EOF libq9.c                                                                            Ver. 1.00
//────────────────────────────────────────────────────────────────────────────────────────────────
