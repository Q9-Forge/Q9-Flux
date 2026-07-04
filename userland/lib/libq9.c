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

static int check_ptr(const void *p)
{
    return p ? 0 : E_BPADDR;
}

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

int q9_detach(void *device)
{
    q9_regs_t r = {0};

    if (!device) {
        return E_BPADDR;
    }
    r.a[2] = device;
    return q9_syscall(I_DETACH, &r);
}

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

int q9_close(uint16_t path)
{
    q9_regs_t r = {0};

    r.d[0] = path;
    return q9_syscall(I_CLOSE, &r);
}

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

int q9_read(uint16_t path, void *buf, uint32_t maxlen, uint32_t *out_len)
{
    return io_rw(I_READ, path, buf, maxlen, out_len);
}

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

int q9_write(uint16_t path, const void *buf, uint32_t len, uint32_t *out_len)
{
    return io_rw(I_WRITE, path, buf, len, out_len);
}

int q9_readln(uint16_t path, void *buf, uint32_t maxlen, uint32_t *out_len)
{
    return io_rw(I_READLN, path, buf, maxlen, out_len);
}

int q9_writln(uint16_t path, const void *buf, uint32_t len, uint32_t *out_len)
{
    return io_rw(I_WRITLN, path, buf, len, out_len);
}

int q9_seek(uint16_t path, uint32_t pos)
{
    q9_regs_t r = {0};

    r.d[0] = path;
    r.d[1] = pos;
    return q9_syscall(I_SEEK, &r);
}

static int path_only(uint16_t func, const char *path)
{
    q9_regs_t r = {0};

    if (!path) {
        return E_BPADDR;
    }
    r.a[0] = (void *)path;
    return q9_syscall(func, &r);
}

int q9_delete(const char *path)
{
    return path_only(I_DELETE, path);
}

int q9_makdir(const char *path)
{
    return path_only(I_MAKDIR, path);
}

int q9_chgdir(const char *path)
{
    return path_only(I_CHGDIR, path);
}

static int stat_call(uint16_t func, uint16_t path, uint16_t code, q9_regs_t *regs)
{
    if (!regs) {
        return E_BPADDR;
    }
    regs->d[0] = path;
    regs->d[1] = code;
    return q9_syscall(func, regs);
}

int q9_getstt(uint16_t path, uint16_t code, q9_regs_t *regs)
{
    return stat_call(I_GETSTT, path, code, regs);
}

int q9_setstt(uint16_t path, uint16_t code, q9_regs_t *regs)
{
    return stat_call(I_SETSTT, path, code, regs);
}

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

int q9_unlink(void *header)
{
    q9_regs_t r = {0};

    if (!header) {
        return E_BPADDR;
    }
    r.a[1] = header;
    return q9_syscall(F_UNLINK, &r);
}

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
