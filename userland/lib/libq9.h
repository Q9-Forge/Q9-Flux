//════════════════════════════════════════════════════════════════════════════════════════════════
// File:   libq9.h                                                                         Ver. 1.00
// Owner:  AF
// Desc.:  Kleine C-Userland-Bibliothek fuer Q9. Kapselt die implementierten Syscalls als
//         freundliche Funktionen, damit Programme nicht direkt q9_regs_t/q9_syscall benutzen.
//
// Call:   #include "libq9.h"
//
// Edition History
//─────────┬──────┬────────────────────────────────────────────────────────────────────────┬──────
// Date    │ Ver. │ Description                                                            │ By
//─────────┼──────┼────────────────────────────────────────────────────────────────────────┼──────
// 26-07-04│ 1.00 │ Initiale Version: Wrapper fuer fertige I$/F$-Syscalls                  │ CX
// 26-07-04│ 1.01 │ q9_read_exact fuer feste Blockgroessen ergaenzt                       │ CX
//═════════╧══════╧════════════════════════════════════════════════════════════════════════╧══════
#ifndef Q9_USERLAND_LIBQ9_H
#define Q9_USERLAND_LIBQ9_H

#include <stdint.h>
#include "../../src/kernel/device.h"
#include "../../src/kernel/syscall.h"

typedef struct q9_time {
    uint16_t year;
    uint8_t  month;
    uint8_t  day;
    uint8_t  hour;
    uint8_t  min;
    uint8_t  sec;
    uint8_t  weekday;                                  /* 0 = Sonntag                            */
    uint32_t ticks_ms;                                 /* Millisekunden seit Boot                */
} q9_time_t;

typedef struct q9_name_parse {
    const char *name;
    const char *next;
    uint16_t    len;
    uint8_t     delim;
} q9_name_parse_t;

int q9_attach(const char *name, uint8_t mode, void **out_device);
int q9_detach(void *device);

int q9_open(const char *path, uint8_t mode, uint16_t *out_path);
int q9_create(const char *path, uint8_t mode, uint16_t *out_path);
int q9_close(uint16_t path);
int q9_dup(uint16_t path, uint16_t *out_path);

int q9_read(uint16_t path, void *buf, uint32_t maxlen, uint32_t *out_len);
int q9_read_exact(uint16_t path, void *buf, uint32_t len, uint32_t *out_len);
int q9_write(uint16_t path, const void *buf, uint32_t len, uint32_t *out_len);
int q9_readln(uint16_t path, void *buf, uint32_t maxlen, uint32_t *out_len);
int q9_writln(uint16_t path, const void *buf, uint32_t len, uint32_t *out_len);
int q9_seek(uint16_t path, uint32_t pos);

int q9_delete(const char *path);
int q9_makdir(const char *path);
int q9_chgdir(const char *path);

int q9_getstt(uint16_t path, uint16_t code, q9_regs_t *regs);
int q9_setstt(uint16_t path, uint16_t code, q9_regs_t *regs);

int q9_prsnam(const char *pathlist, q9_name_parse_t *out_name);
int q9_cmpnam(const char *a, const char *b, uint16_t len);
int q9_id(uint16_t *out_pid, uint32_t *out_uid);

int q9_link(const char *name, uint8_t type, uint8_t lang,
            void **out_header, void **out_entry, uint8_t *out_rev);
int q9_unlink(void *header);
int q9_load(const char *path, void **out_header, void **out_entry, uint8_t *out_rev);
int q9_time(q9_time_t *out_time);
int q9_stime(const q9_time_t *time);
int q9_exit(uint16_t status);

#endif // Q9_USERLAND_LIBQ9_H

//────────────────────────────────────────────────────────────────────────────────────────────────
// EOF libq9.h                                                                            Ver. 1.00
//────────────────────────────────────────────────────────────────────────────────────────────────
