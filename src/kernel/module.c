//════════════════════════════════════════════════════════════════════════════════════════════════
// File:   module.c                                                                        Ver. 1.40
// Owner:  AF
// Desc.:  CRC32-Routine + ROM-Image-Suche/Validierung + Modul-Directory + F$Link/F$UnLink/F$Load-
//         Unterbau fuer Q9-Module (Header-Struct steht in module.h). Directory ist ein statisches
//         Array (kein malloc im Kernel, wie devtab/pathtab in device.c). Der Load-Puffer-Pool
//         (3.5) folgt demselben Muster: statisches Array fester Groesse statt malloc.
//
// Call:   crc = q9_crc32(data, len); hdr = q9_mod_scan_first(rom, romlen);
//         err = q9_mod_validate(rom, romlen, hdr); err = q9_mod_register(hdr);
//         err = q9_mod_link(name, type, lang, &hdr); err = q9_mod_unlink(hdr)
//         err = q9_mod_load(pathlist, &hdr)
//
// Edition History
//─────────┬──────┬────────────────────────────────────────────────────────────────────────┬──────
// Date    │ Ver. │ Description                                                            │ By
//─────────┼──────┼────────────────────────────────────────────────────────────────────────┼──────
// 26-07-03│ 1.00 │ 2.1: q9_crc32 (bitweise, CRC-32/ISO-HDLC)                              │ CF
// 26-07-03│ 1.10 │ 2.3a: q9_mod_scan_first/next — Sync-Suche, Sprung um ModuleSize        │ CF
// 26-07-03│ 1.20 │ 2.3b: q9_mod_validate — Groesse/Nameoffset/CRC32                       │ CF
// 26-07-03│ 1.30 │ 2.3c+d: Modul-Directory (register/find), F$Link/UnLink-Unterbau        │ CF
// 26-07-04│ 1.40 │ 3.5: q9_mod_load (F$Load) — statischer Load-Puffer-Pool, Directory-    │ CF
//         │      │ Eintraege merken sich jetzt ihre Puffer-Herkunft (loadbuf-Feld) fuer    │
//         │      │ die automatische Freigabe bei Link-Count 0 (anders als ROM-Module).     │
//         │      │ q9_mod_validate prueft jetzt zusaetzlich die Sync-Bytes selbst (auf dem │
//         │      │ ROM-Scan-Pfad redundant, aber Pflicht fuer F$Load ohne Scan-Vorstufe)   │
//═════════╧══════╧════════════════════════════════════════════════════════════════════════╧══════

#include <stddef.h>

#include "device.h"
#include "module.h"
#include "name.h"
#include "syscall.h"
#include "vfs.h"

//────────────────────────────────────────────────────────────────────────────────────────────────
// Function: crc32_byte
// Desc.:    Ein CRC-32/ISO-HDLC-Schritt auf dem laufenden (noch nicht invertierten) Zustand.
//           Gemeinsamer Unterbau fuer q9_crc32 und die Modul-CRC in q9_mod_validate (dort muss
//           das CRC-Feld selbst als 0 gerechnet werden, ohne das Modul zu kopieren).
//────────────────────────────────────────────────────────────────────────────────────────────────
static uint32_t crc32_byte(uint32_t crc, uint8_t b)
{
    crc ^= b;
    for (int bit = 0; bit < 8; bit++) {
        uint32_t mask = -(crc & 1u);
        crc = (crc >> 1) ^ (0xEDB88320u & mask);
    }
    return crc;
}

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_crc32
// Desc.:    Siehe module.h. Bitweise CRC-32/ISO-HDLC ueber data[0..len).
// Call:     crc = q9_crc32(data, len)
//════════════════════════════════════════════════════════════════════════════════════════════════
uint32_t q9_crc32(const uint8_t *data, uint32_t len)
{
    uint32_t crc = 0xFFFFFFFFu;

    for (uint32_t i = 0; i < len; i++) {
        crc = crc32_byte(crc, data[i]);
    }
    return crc ^ 0xFFFFFFFFu;
}

//────────────────────────────────────────────────────────────────────────────────────────────────
// Function: sync_ok
// Desc.:    Prueft, ob bei rom[off] ein voller Header Platz hat und dort die Sync-Bytes stehen.
//────────────────────────────────────────────────────────────────────────────────────────────────
static int sync_ok(const uint8_t *rom, uint32_t romlen, uint32_t off)
{
    if (off > romlen - Q9_MOD_HDRSIZE) {               /* romlen >= Q9_MOD_HDRSIZE vorausgesetzt  */
        return 0;
    }
    return rom[off] == Q9_MOD_SYNC0 && rom[off + 1] == Q9_MOD_SYNC1;
}

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_mod_scan_first
// Desc.:    Siehe module.h. Byteweise Suche nach den Sync-Bytes, Groesse/CRC folgt in 2.3b.
// Call:     hdr = q9_mod_scan_first(rom, romlen)
//════════════════════════════════════════════════════════════════════════════════════════════════
const q9_modhdr_t *q9_mod_scan_first(const uint8_t *rom, uint32_t romlen)
{
    if (romlen < Q9_MOD_HDRSIZE) {
        return 0;
    }
    for (uint32_t off = 0; off <= romlen - Q9_MOD_HDRSIZE; off++) {
        if (sync_ok(rom, romlen, off)) {
            return (const q9_modhdr_t *)(rom + off);
        }
    }
    return 0;
}

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_mod_scan_next
// Desc.:    Siehe module.h. Springt exakt um ModuleSize weiter (kein erneutes Durchsuchen) —
//           OS-9-Vorbild: Module liegen im ROM-Image lueckenlos hintereinander.
// Call:     next = q9_mod_scan_next(rom, romlen, hdr)
//════════════════════════════════════════════════════════════════════════════════════════════════
const q9_modhdr_t *q9_mod_scan_next(const uint8_t *rom, uint32_t romlen, const q9_modhdr_t *cur)
{
    uint32_t curoff  = (uint32_t)((const uint8_t *)cur - rom);
    uint32_t nextoff = curoff + cur->modsize;

    if (cur->modsize < Q9_MOD_HDRSIZE || nextoff < curoff) {  /* Groessen-Plausibilitaet: 2.3b;   */
        return 0;                                             /* hier nur Schutz vor Endlosschleife/Overflow */
    }
    if (romlen < Q9_MOD_HDRSIZE || nextoff > romlen - Q9_MOD_HDRSIZE) {
        return 0;
    }
    return sync_ok(rom, romlen, nextoff) ? (const q9_modhdr_t *)(rom + nextoff) : 0;
}

//────────────────────────────────────────────────────────────────────────────────────────────────
// Function: mod_crc
// Desc.:    CRC32 ueber rom[modoff..modoff+modsize), aber die 4 Bytes des CRC-Feldes selbst
//           werden als 0 gerechnet (wie im Header dokumentiert) — ohne das Modul zu kopieren,
//           daher byteweise ueber crc32_byte statt q9_crc32(data, len).
//────────────────────────────────────────────────────────────────────────────────────────────────
static uint32_t mod_crc(const uint8_t *rom, uint32_t modoff, uint32_t modsize)
{
    uint32_t crcoff = modoff + (uint32_t)offsetof(q9_modhdr_t, crc32);
    uint32_t crc    = 0xFFFFFFFFu;

    for (uint32_t i = 0; i < modsize; i++) {
        uint32_t off = modoff + i;
        uint8_t  b   = (off >= crcoff && off < crcoff + 4) ? 0 : rom[off];
        crc = crc32_byte(crc, b);
    }
    return crc ^ 0xFFFFFFFFu;
}

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_mod_validate
// Desc.:    Siehe module.h. Struktur-Checks (billig) vor der vollen CRC32 (teuer).
// Call:     err = q9_mod_validate(rom, romlen, hdr)
//════════════════════════════════════════════════════════════════════════════════════════════════
int q9_mod_validate(const uint8_t *rom, uint32_t romlen, const q9_modhdr_t *hdr)
{
    uint32_t off = (uint32_t)((const uint8_t *)hdr - rom);

    if (hdr->sync[0] != Q9_MOD_SYNC0 || hdr->sync[1] != Q9_MOD_SYNC1) {
        return E_BMHP;                                    /* redundant auf dem ROM-Scan-Pfad (der   */
    }                                                      /* Sync schon prueft), aber Pflicht fuer   */
                                                            /* F$Load (3.5) — dort gibt es keine       */
                                                            /* vorherige Sync-Suche, die ganze Datei   */
                                                            /* IST das Modul.                          */
    if (hdr->hdrsize != Q9_MOD_HDRSIZE || hdr->modsize < Q9_MOD_HDRSIZE) {
        return E_BMHP;
    }
    if (hdr->modsize > romlen - off) {                  /* off <= romlen-HDRSIZE laut Scan-Aufruf */
        return E_BMHP;
    }
    if (hdr->nameoff >= hdr->modsize) {                 /* Name muss im Modul selbst liegen        */
        return E_BMHP;
    }
    if (mod_crc(rom, off, hdr->modsize) != hdr->crc32) {
        return E_BMCRC;
    }
    return 0;
}

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_mod_name
// Desc.:    Siehe module.h.
// Call:     name = q9_mod_name(hdr)
//════════════════════════════════════════════════════════════════════════════════════════════════
const char *q9_mod_name(const q9_modhdr_t *hdr)
{
    return (const char *)hdr + hdr->nameoff;
}

//╔══════════════════════════════════════════════════════════════════════════════════════════════╗
//║ MODUL-DIRECTORY (2.3c: Bekanntmachen) — statisches Array, kein malloc (wie devtab/pathtab)    ║
//╚══════════════════════════════════════════════════════════════════════════════════════════════╝

#define Q9_MOD_MAXDIR 8                                    /* Directory-Groesse                   */

typedef struct {
    const q9_modhdr_t *hdr;                                /* NULL = Slot frei                    */
    uint16_t            link;                               /* Link-Count (F$Link/F$UnLink)        */
    int8_t              loadbuf;                            /* 3.5: -1 = kein Load-Puffer (ROM/    */
                                                              /*      eingebaut, bleibt bei Link=0   */
                                                              /*      registriert), sonst Index in   */
                                                              /*      loadbuf[] (wird bei Link=0     */
                                                              /*      automatisch freigegeben)       */
} moddir_entry_t;

static moddir_entry_t moddir[Q9_MOD_MAXDIR];

//╔══════════════════════════════════════════════════════════════════════════════════════════════╗
//║ LOAD-PUFFER-POOL (3.5: F$Load) — statisch, kein malloc, siehe module.h                       ║
//╚══════════════════════════════════════════════════════════════════════════════════════════════╝

static uint8_t loadbuf[Q9_MOD_LOADBUF_COUNT][Q9_MOD_LOADBUF_SIZE];
static uint8_t loadbuf_used[Q9_MOD_LOADBUF_COUNT];         /* 0 = frei                            */

static uint32_t str_len(const char *s)
{
    uint32_t n = 0;
    while (s[n]) {
        n++;
    }
    return n;
}

//────────────────────────────────────────────────────────────────────────────────────────────────
// Function: name_eq
// Desc.:    Voller (nicht praefixweiser) case-insensitiver Namensvergleich zweier C-Strings.
//────────────────────────────────────────────────────────────────────────────────────────────────
static int name_eq(const char *a, const char *b)
{
    uint32_t la = str_len(a);
    return str_len(b) == la && q9_name_cmp(a, la, b) == 0;
}

//────────────────────────────────────────────────────────────────────────────────────────────────
// Function: mod_register_ex
// Desc.:    Gemeinsamer Unterbau von q9_mod_register (ROM/eingebaute Module, loadbuf_idx = -1) und
//           q9_mod_load (3.5: aus Datei geladene Module, loadbuf_idx = Load-Puffer-Index). Wird ein
//           bestehender Directory-Eintrag durch eine hoehere Revision verdraengt und war DESSEN
//           Speicher ein Load-Puffer mit Link-Count 0 (niemand haelt mehr einen Zeiger darauf), wird
//           dieser Puffer sofort freigegeben; steht sein Link-Count noch > 0, bleibt der alte Puffer
//           belegt, bis er ganz reguaer per F$UnLink auf 0 faellt (q9_mod_unlink) — kein Modul wird
//           unter der Hand aus dem Speicher gerissen, waehrend noch jemand darauf verlinkt ist.
//────────────────────────────────────────────────────────────────────────────────────────────────
static int mod_register_ex(const q9_modhdr_t *hdr, int8_t loadbuf_idx)
{
    int free_slot = -1;

    for (int i = 0; i < Q9_MOD_MAXDIR; i++) {
        if (!moddir[i].hdr) {
            if (free_slot < 0) {
                free_slot = i;
            }
            continue;
        }
        if (moddir[i].hdr->type == hdr->type && name_eq(q9_mod_name(moddir[i].hdr), q9_mod_name(hdr))) {
            if (hdr->rev > moddir[i].hdr->rev) {           /* hoehere Revision gewinnt             */
                if (moddir[i].loadbuf >= 0 && moddir[i].link == 0) {
                    loadbuf_used[moddir[i].loadbuf] = 0;
                }
                moddir[i].hdr     = hdr;
                moddir[i].loadbuf = loadbuf_idx;
            }
            return 0;                                      /* Gleichstand/niedriger: stiller No-Op */
        }
    }
    if (free_slot < 0) {
        return E_DIRFUL;
    }
    moddir[free_slot].hdr     = hdr;
    moddir[free_slot].link    = 0;
    moddir[free_slot].loadbuf = loadbuf_idx;
    return 0;
}

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_mod_register
// Desc.:    Siehe module.h. Namenskollisions-/Revision-Regel wie OS-9 (docs/MODULES.md).
// Call:     err = q9_mod_register(hdr)
//════════════════════════════════════════════════════════════════════════════════════════════════
int q9_mod_register(const q9_modhdr_t *hdr)
{
    return mod_register_ex(hdr, -1);
}

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_mod_find
// Desc.:    Siehe module.h. type/lang == 0 wirkt als Platzhalter ("beliebig").
// Call:     hdr = q9_mod_find(name, type, lang)
//════════════════════════════════════════════════════════════════════════════════════════════════
const q9_modhdr_t *q9_mod_find(const char *name, uint8_t type, uint8_t lang)
{
    for (int i = 0; i < Q9_MOD_MAXDIR; i++) {
        const q9_modhdr_t *h = moddir[i].hdr;
        if (h && name_eq(q9_mod_name(h), name) &&
            (type == 0 || h->type == type) && (lang == 0 || h->lang == lang)) {
            return h;
        }
    }
    return 0;
}

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_mod_link
// Desc.:    Siehe module.h.
// Call:     err = q9_mod_link(name, type, lang, &hdr)
//════════════════════════════════════════════════════════════════════════════════════════════════
int q9_mod_link(const char *name, uint8_t type, uint8_t lang, const q9_modhdr_t **out)
{
    for (int i = 0; i < Q9_MOD_MAXDIR; i++) {
        const q9_modhdr_t *h = moddir[i].hdr;
        if (h && name_eq(q9_mod_name(h), name) &&
            (type == 0 || h->type == type) && (lang == 0 || h->lang == lang)) {
            moddir[i].link++;
            *out = h;
            return 0;
        }
    }
    return E_MNF;
}

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_mod_unlink
// Desc.:    Siehe module.h.
// Call:     err = q9_mod_unlink(hdr)
//════════════════════════════════════════════════════════════════════════════════════════════════
int q9_mod_unlink(const q9_modhdr_t *hdr)
{
    for (int i = 0; i < Q9_MOD_MAXDIR; i++) {
        if (moddir[i].hdr == hdr) {
            if (moddir[i].link > 0) {
                moddir[i].link--;
            }
            if (moddir[i].link == 0 && moddir[i].loadbuf >= 0) {
                /* 3.5: aus einem F$Load-Puffer geladenes Modul — anders als ein ROM-Modul (das    */
                /* bei Link-Count 0 registriert bleibt, es kostet ja keinen Speicher) wird es hier */
                /* sofort komplett entfernt, weil der kleine Load-Puffer-Pool sonst nach wenigen    */
                /* Load/Unlink-Zyklen erschoepft waere. Kein Ghost/Sticky-Attribut ($40) ausge-     */
                /* wertet (Ideenspeicher, falls ein Modul trotz Link-Count 0 resident bleiben soll).*/
                loadbuf_used[moddir[i].loadbuf] = 0;
                moddir[i].hdr     = 0;
                moddir[i].loadbuf = -1;
            }
            return 0;
        }
    }
    return E_MNF;
}

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_mod_load
// Desc.:    Siehe module.h.
// Call:     err = q9_mod_load("/d0/HELLO.MOD", &hdr)
//════════════════════════════════════════════════════════════════════════════════════════════════
int q9_mod_load(const char *pathlist, const q9_modhdr_t **out)
{
    int                 bufidx = -1;
    int                 path;
    uint32_t            got = 0;
    int                 err;
    q9_path_t          *p;
    const q9_modhdr_t  *hdr;

    if (!pathlist || !out) {
        return E_BPADDR;
    }
    for (int i = 0; i < Q9_MOD_LOADBUF_COUNT; i++) {
        if (!loadbuf_used[i]) {
            bufidx = i;
            break;
        }
    }
    if (bufidx < 0) {
        return E_NORAM;                                  /* alle Load-Puffer belegt                */
    }

    path = q9_vfs_open(pathlist, Q9_MODE_READ);
    if (path < 0) {
        return -path;
    }
    p = q9_path_get((uint32_t)path);

    /* Datei komplett in den Load-Puffer lesen — File-Manager-Op bevorzugt (FAT16 & Co.), sonst
       wie sc_read (syscall.c) auf die rohe Treiber-Op zurueckfallen. */
    for (;;) {
        uint32_t n = Q9_MOD_LOADBUF_SIZE - got;
        if (n == 0) {
            q9_path_close((uint32_t)path);
            return E_NORAM;                              /* Datei zu gross fuer den Load-Puffer    */
        }
        if (p->dev->fm && p->dev->fm->read) {
            err = p->dev->fm->read(p->dev, p, loadbuf[bufidx] + got, &n);
        } else {
            err = p->dev->drv->read(p->dev, loadbuf[bufidx] + got, &n);
        }
        if (err == E_EOF) {
            break;
        }
        if (err != 0) {
            q9_path_close((uint32_t)path);
            return err;
        }
        if (n == 0) {
            break;
        }
        got += n;
    }
    q9_path_close((uint32_t)path);

    if (got < Q9_MOD_HDRSIZE) {
        return E_BMHP;                                    /* zu kurz fuer einen Modul-Header         */
    }
    hdr = (const q9_modhdr_t *)loadbuf[bufidx];
    err = q9_mod_validate(loadbuf[bufidx], got, hdr);
    if (err != 0) {
        return err;
    }

    loadbuf_used[bufidx] = 1;                             /* reservieren, bevor die Directory den    */
    err = mod_register_ex(hdr, (int8_t)bufidx);           /* Puffer-Index uebernimmt                 */
    if (err != 0) {
        loadbuf_used[bufidx] = 0;
        return err;
    }
    if (q9_mod_find(q9_mod_name(hdr), hdr->type, hdr->lang) != hdr) {
        /* Namenskollision: ein bestehendes Modul mit gleicher/hoeherer Revision hat gewonnen
           (mod_register_ex, s.o.) — unser frisch gefuellter Puffer wird nicht gebraucht. */
        loadbuf_used[bufidx] = 0;
    }
    return q9_mod_link(q9_mod_name(hdr), hdr->type, hdr->lang, out);
}

//────────────────────────────────────────────────────────────────────────────────────────────────
// EOF module.c                                                                            Ver. 1.40
//────────────────────────────────────────────────────────────────────────────────────────────────
