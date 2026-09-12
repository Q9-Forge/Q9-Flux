//════════════════════════════════════════════════════════════════════════════════════════════════
// File:   q9dir.c                                                                         Ver. 1.00
// Owner:  AF
// Desc.:  Beispiel-Tool: listet FAT16-Directory-Eintraege, die der Q9-FAT16-Treiber als rohe
//         32-Byte-Bloecke ueber I$Read liefert. Arbeitet ausschliesslich ueber libq9.
//
// Call:   err = q9dir_run("/d0")
//
// Edition History
//─────────┬──────┬────────────────────────────────────────────────────────────────────────┬──────
// Date    │ Ver. │ Description                                                            │ By
//─────────┼──────┼────────────────────────────────────────────────────────────────────────┼──────
// 26-07-04│ 1.00 │ Initiale Version                                                        │ CX
// 26-07-04│ 1.01 │ Directory-Iteration/-Ausgabe fuer q9stat wiederverwendbar gemacht      │ CX
//═════════╧══════╧════════════════════════════════════════════════════════════════════════╧══════

#include <stdint.h>

#include "../lib/libq9.h"
#include "q9dir.h"

#define Q9DIRENT_SIZE      32u
#define Q9DIRENT_END       0x00u
#define Q9DIRENT_FREE      0xE5u
#define Q9ATTR_VOLUME_ID   0x08u
#define Q9ATTR_DIRECTORY   0x10u
#define Q9ATTR_LONG_NAME   0x0Fu

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: le32
// Desc.:    Liest ein little-endian uint32_t aus einem FAT16-Directory-Feld.
// Call:     value = le32(&de[28])
//════════════════════════════════════════════════════════════════════════════════════════════════
static uint32_t le32(const uint8_t *p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
           ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: trim_len
// Desc.:    Bestimmt die Laenge eines rechts mit Leerzeichen gefuellten FAT16-8.3-Felds.
// Call:     len = trim_len(&de[0], 8)
//════════════════════════════════════════════════════════════════════════════════════════════════
static uint32_t trim_len(const uint8_t *p, uint32_t max)
{
    while (max > 0 && p[max - 1u] == ' ') {
        max--;
    }
    return max;
}

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: append_str
// Desc.:    Haengt den nullterminierten String s an out ab Position pos an und liefert die neue
//           Position. Der Aufrufer garantiert ausreichend Platz.
// Call:     pos = append_str(line, pos, "FILE ")
//════════════════════════════════════════════════════════════════════════════════════════════════
static uint32_t append_str(char *out, uint32_t pos, const char *s)
{
    while (*s) {
        out[pos++] = *s++;
    }
    return pos;
}

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: append_uint
// Desc.:    Haengt v dezimal an out ab Position pos an und liefert die neue Position. Der
//           Aufrufer garantiert ausreichend Platz.
// Call:     pos = append_uint(line, pos, entry->size)
//════════════════════════════════════════════════════════════════════════════════════════════════
static uint32_t append_uint(char *out, uint32_t pos, uint32_t v)
{
    char     tmp[10];
    uint32_t n = 0;

    if (v == 0) {
        out[pos++] = '0';
        return pos;
    }
    while (v > 0) {
        tmp[n++] = (char)('0' + (v % 10u));
        v /= 10u;
    }
    while (n > 0) {
        out[pos++] = tmp[--n];
    }
    return pos;
}

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: format_name
// Desc.:    Formatiert den FAT16-8.3-Namen aus de nach out. Dateien bekommen einen Punkt vor
//           der Extension, Directories nicht; Leerzeichen-Fuellung wird entfernt.
// Call:     len = format_name(de, is_dir, name)
//════════════════════════════════════════════════════════════════════════════════════════════════
static uint32_t format_name(const uint8_t *de, int is_dir, char *out)
{
    uint32_t nlen = trim_len(&de[0], 8u);
    uint32_t elen = trim_len(&de[8], 3u);
    uint32_t pos = 0;

    for (uint32_t i = 0; i < nlen; i++) {
        out[pos++] = (char)de[i];
    }
    if (!is_dir && elen > 0) {
        out[pos++] = '.';
        for (uint32_t i = 0; i < elen; i++) {
            out[pos++] = (char)de[8u + i];
        }
    }
    out[pos] = 0;
    return pos;
}

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: fill_entry
// Desc.:    Uebertraegt einen rohen FAT16-Dirent in q9dir_entry_t: Directory-Bit, Dateigroesse
//           aus Offset 28 und formatierter 8.3-Name.
// Call:     fill_entry(de, &entry)
//════════════════════════════════════════════════════════════════════════════════════════════════
static void fill_entry(const uint8_t *de, q9dir_entry_t *entry)
{
    entry->is_dir = (de[11] & Q9ATTR_DIRECTORY) != 0;
    entry->size = entry->is_dir ? 0u : le32(&de[28]);
    format_name(de, entry->is_dir, entry->name);
}

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9dir_write_entry
// Desc.:    Siehe q9dir.h. Baut eine kurze Ausgabezeile "DIR/FILE size name" und schreibt sie
//           vollstaendig nach Pfad 1.
// Call:     err = q9dir_write_entry(&entry)
//════════════════════════════════════════════════════════════════════════════════════════════════
int q9dir_write_entry(const q9dir_entry_t *entry)
{
    char     line[40];
    uint32_t pos = 0;
    uint32_t put = 0;

    if (!entry) {
        return E_BPADDR;
    }
    pos = append_str(line, pos, entry->is_dir ? "DIR " : "FILE ");
    pos = append_uint(line, pos, entry->size);
    line[pos++] = ' ';
    pos = append_str(line, pos, entry->name);
    line[pos++] = '\n';

    {
        int err = q9_writln(1, line, pos, &put);
        if (err != 0) {
            return err;
        }
    }
    return put == pos ? 0 : E_NOTRDY;
}

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9dir_next
// Desc.:    Siehe q9dir.h. Liest 32-Byte-FAT16-Dirents und ueberspringt geloeschte Slots
//           (0xE5), Long-File-Name-Slots (Attr. 0x0F) und Volume-Labels, weil Q9-Tools nur
//           sichtbare 8.3-Dateien/Directories ausgeben. 0x00 und echtes EOF melden E$EOF.
// Call:     err = q9dir_next(path, &entry)
//════════════════════════════════════════════════════════════════════════════════════════════════
int q9dir_next(uint16_t path, q9dir_entry_t *out_entry)
{
    uint8_t de[Q9DIRENT_SIZE];

    if (!out_entry) {
        return E_BPADDR;
    }
    for (;;) {
        uint32_t got = 0;
        uint8_t  attr;
        int      err;

        err = q9_read_exact(path, de, sizeof(de), &got);
        if (err == E_EOF && got == 0) {
            return E_EOF;
        }
        if (err != 0) {
            return err;
        }
        if (de[0] == Q9DIRENT_END) {
            return E_EOF;
        }
        if (de[0] == Q9DIRENT_FREE) {
            continue;
        }
        attr = de[11];
        if ((attr & Q9ATTR_LONG_NAME) == Q9ATTR_LONG_NAME) {
            continue;
        }
        if (attr & Q9ATTR_VOLUME_ID) {
            continue;
        }
        fill_entry(de, out_entry);
        return 0;
    }
}

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9dir_run
// Desc.:    Siehe q9dir.h. Oeffnet das Directory, schreibt jeden q9dir_next-Treffer und
//           schliesst den Pfad auch bei Abbruch durch Fehler.
// Call:     err = q9dir_run("/d0")
//════════════════════════════════════════════════════════════════════════════════════════════════
int q9dir_run(const char *path)
{
    uint16_t      in;
    q9dir_entry_t entry;
    int           err;

    err = q9_open(path, Q9_MODE_READ, &in);
    if (err != 0) {
        return err;
    }
    for (;;) {
        err = q9dir_next(in, &entry);
        if (err == E_EOF) {
            err = 0;
            break;
        }
        if (err != 0) {
            break;
        }
        err = q9dir_write_entry(&entry);
        if (err != 0) {
            break;
        }
    }
    {
        int cerr = q9_close(in);
        return err != 0 ? err : cerr;
    }
}

//────────────────────────────────────────────────────────────────────────────────────────────────
// EOF q9dir.c                                                                            Ver. 1.00
//────────────────────────────────────────────────────────────────────────────────────────────────
