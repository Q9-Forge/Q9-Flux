//════════════════════════════════════════════════════════════════════════════════════════════════
// File:   q9touch.c                                                                       Ver. 1.00
// Owner:  AF
// Desc.:  Beispiel-Tool: legt eine leere Datei an und bestaetigt den Erfolg auf Pfad 1.
//         Arbeitet ausschliesslich ueber libq9, nicht direkt ueber q9_syscall.
//
// Call:   err = q9touch_run("/d0/DATEI.TXT")
//
// Edition History
//─────────┬──────┬────────────────────────────────────────────────────────────────────────┬──────
// Date    │ Ver. │ Description                                                            │ By
//─────────┼──────┼────────────────────────────────────────────────────────────────────────┼──────
// 26-07-04│ 1.00 │ Initiale Version                                                        │ CX
//═════════╧══════╧════════════════════════════════════════════════════════════════════════╧══════

#include <string.h>

#include "../lib/libq9.h"
#include "q9touch.h"

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: write_part
// Desc.:    Schreibt genau len Bytes aus s nach Pfad 1 und meldet E$NotRdy bei Kurzschreibung.
// Call:     err = write_part("touch: ", 7)
//════════════════════════════════════════════════════════════════════════════════════════════════
static int write_part(const char *s, uint32_t len)
{
    uint32_t put = 0;
    int      err;

    err = q9_writln(1, s, len, &put);
    if (err != 0) {
        return err;
    }
    return put == len ? 0 : E_NOTRDY;
}

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: write_confirm
// Desc.:    Schreibt die q9touch-Bestaetigungszeile fuer path nach Pfad 1.
// Call:     err = write_confirm(path)
//════════════════════════════════════════════════════════════════════════════════════════════════
static int write_confirm(const char *path)
{
    int err;

    err = write_part("touch: ", 7u);
    if (err != 0) {
        return err;
    }
    err = write_part(path, (uint32_t)strlen(path));
    if (err != 0) {
        return err;
    }
    return write_part("\n", 1u);
}

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9touch_run
// Desc.:    Siehe q9touch.h. Existierende 8.3-Dateien werden nach E$BPNam nur geoeffnet und
//           geschlossen, damit kein Inhalt gekuerzt wird.
// Call:     err = q9touch_run("/d0/DATEI.TXT")
//════════════════════════════════════════════════════════════════════════════════════════════════
int q9touch_run(const char *path)
{
    uint16_t p;
    int      err;

    err = q9_create(path, Q9_MODE_WRITE, &p);
    if (err == E_BPNAM) {
        /*
         * Geprueftes FAT16-Verhalten: I$Create liefert E$BPNam, wenn der 8.3-Name
         * schon existiert (kein Truncate-Flag im Mode-Byte). Fuer touch reicht dann
         * ein Open/Close des vorhandenen Pfads; der Inhalt bleibt unveraendert.
         */
        err = q9_open(path, Q9_MODE_READ, &p);
    }
    if (err != 0) {
        return err;
    }
    err = q9_close(p);
    if (err != 0) {
        return err;
    }
    return write_confirm(path);
}

//────────────────────────────────────────────────────────────────────────────────────────────────
// EOF q9touch.c                                                                         Ver. 1.00
//────────────────────────────────────────────────────────────────────────────────────────────────
