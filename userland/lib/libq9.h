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

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_attach
// Desc.:    Haengt das Geraet "name" im angegebenen Mode an und liefert bei Erfolg den
//           Device-Zeiger ueber *out_device. Rueckgabe: 0 = ok, sonst Q9-Fehlercode.
// Call:     err = q9_attach("d0", Q9_MODE_READ, &dev)
//════════════════════════════════════════════════════════════════════════════════════════════════
int q9_attach(const char *name, uint8_t mode, void **out_device);

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_detach
// Desc.:    Loest ein zuvor per q9_attach erhaltenes Geraet wieder. device darf nicht NULL sein;
//           Rueckgabe: 0 = ok, sonst Q9-Fehlercode.
// Call:     err = q9_detach(dev)
//════════════════════════════════════════════════════════════════════════════════════════════════
int q9_detach(void *device);

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_open
// Desc.:    Oeffnet "path" im angegebenen Mode und liefert die Pfadnummer ueber *out_path.
//           Rueckgabe: 0 = ok, sonst Q9-Fehlercode.
// Call:     err = q9_open("/d0/DATEI.TXT", Q9_MODE_READ, &path)
//════════════════════════════════════════════════════════════════════════════════════════════════
int q9_open(const char *path, uint8_t mode, uint16_t *out_path);

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_create
// Desc.:    Legt "path" im angegebenen Mode an und liefert die neue Pfadnummer ueber *out_path.
//           Rueckgabe: 0 = ok, sonst Q9-Fehlercode.
// Call:     err = q9_create("/d0/NEU.TXT", Q9_MODE_WRITE, &path)
//════════════════════════════════════════════════════════════════════════════════════════════════
int q9_create(const char *path, uint8_t mode, uint16_t *out_path);

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_close
// Desc.:    Schliesst die Pfadnummer "path". Rueckgabe: 0 = ok, sonst Q9-Fehlercode.
// Call:     err = q9_close(path)
//════════════════════════════════════════════════════════════════════════════════════════════════
int q9_close(uint16_t path);

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_dup
// Desc.:    Dupliziert die Pfadnummer "path" und liefert die Kopie ueber *out_path.
//           Rueckgabe: 0 = ok, sonst Q9-Fehlercode.
// Call:     err = q9_dup(path, &copy)
//════════════════════════════════════════════════════════════════════════════════════════════════
int q9_dup(uint16_t path, uint16_t *out_path);

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_read
// Desc.:    Liest bis zu maxlen Bytes von "path" nach buf; *out_len bekommt optional die
//           tatsaechlich gelesene Laenge. Rueckgabe: 0 = ok, sonst Q9-Fehlercode.
// Call:     err = q9_read(path, buf, sizeof(buf), &got)
//════════════════════════════════════════════════════════════════════════════════════════════════
int q9_read(uint16_t path, void *buf, uint32_t maxlen, uint32_t *out_len);

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_read_exact
// Desc.:    Liest genau len Bytes von "path" nach buf, sofern genug Daten vorhanden sind;
//           *out_len bekommt optional den Fortschritt. E$EOF bei zu kurzem Input.
// Call:     err = q9_read_exact(path, block, sizeof(block), &got)
//════════════════════════════════════════════════════════════════════════════════════════════════
int q9_read_exact(uint16_t path, void *buf, uint32_t len, uint32_t *out_len);

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_write
// Desc.:    Schreibt len Bytes aus buf nach "path"; *out_len bekommt optional die tatsaechlich
//           geschriebene Laenge. Rueckgabe: 0 = ok, sonst Q9-Fehlercode.
// Call:     err = q9_write(path, buf, len, &put)
//════════════════════════════════════════════════════════════════════════════════════════════════
int q9_write(uint16_t path, const void *buf, uint32_t len, uint32_t *out_len);

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_readln
// Desc.:    Liest eine Zeile bzw. bis zu maxlen Bytes von "path" nach buf; *out_len bekommt
//           optional die Laenge. Rueckgabe: 0 = ok, sonst Q9-Fehlercode.
// Call:     err = q9_readln(path, line, sizeof(line), &got)
//════════════════════════════════════════════════════════════════════════════════════════════════
int q9_readln(uint16_t path, void *buf, uint32_t maxlen, uint32_t *out_len);

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_writln
// Desc.:    Schreibt len Bytes als Zeilenausgabe nach "path"; *out_len bekommt optional die
//           tatsaechliche Laenge. Rueckgabe: 0 = ok, sonst Q9-Fehlercode.
// Call:     err = q9_writln(1, line, len, &put)
//════════════════════════════════════════════════════════════════════════════════════════════════
int q9_writln(uint16_t path, const void *buf, uint32_t len, uint32_t *out_len);

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_seek
// Desc.:    Setzt die Position des Pfads "path" auf Byte-Offset pos. Rueckgabe: 0 = ok,
//           sonst Q9-Fehlercode.
// Call:     err = q9_seek(path, 0)
//════════════════════════════════════════════════════════════════════════════════════════════════
int q9_seek(uint16_t path, uint32_t pos);

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_delete
// Desc.:    Loescht die Datei oder den Pfad "path". Rueckgabe: 0 = ok, sonst Q9-Fehlercode.
// Call:     err = q9_delete("/d0/ALT.TXT")
//════════════════════════════════════════════════════════════════════════════════════════════════
int q9_delete(const char *path);

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_makdir
// Desc.:    Legt das Verzeichnis "path" an. Rueckgabe: 0 = ok, sonst Q9-Fehlercode.
// Call:     err = q9_makdir("/d0/NEUDIR")
//════════════════════════════════════════════════════════════════════════════════════════════════
int q9_makdir(const char *path);

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_chgdir
// Desc.:    Wechselt das aktuelle Directory auf "path". Rueckgabe: 0 = ok, sonst Q9-Fehlercode.
// Call:     err = q9_chgdir("/d0/NEUDIR")
//════════════════════════════════════════════════════════════════════════════════════════════════
int q9_chgdir(const char *path);

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_getstt
// Desc.:    Fuehrt I$GetStt fuer "path" und Status-Code "code" aus; regs transportiert die
//           syscall-Register hinein und hinaus. Rueckgabe: 0 = ok, sonst Q9-Fehlercode.
// Call:     err = q9_getstt(path, code, &regs)
//════════════════════════════════════════════════════════════════════════════════════════════════
int q9_getstt(uint16_t path, uint16_t code, q9_regs_t *regs);

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_setstt
// Desc.:    Fuehrt I$SetStt fuer "path" und Status-Code "code" aus; regs transportiert die
//           syscall-Register hinein und hinaus. Rueckgabe: 0 = ok, sonst Q9-Fehlercode.
// Call:     err = q9_setstt(path, code, &regs)
//════════════════════════════════════════════════════════════════════════════════════════════════
int q9_setstt(uint16_t path, uint16_t code, q9_regs_t *regs);

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_prsnam
// Desc.:    Parst das naechste Namenselement aus pathlist. out_name enthaelt Start, naechsten
//           Zeiger, Laenge und Delimiter. Rueckgabe: 0 = ok, sonst Q9-Fehlercode.
// Call:     err = q9_prsnam("/d0/DATEI.TXT", &name)
//════════════════════════════════════════════════════════════════════════════════════════════════
int q9_prsnam(const char *pathlist, q9_name_parse_t *out_name);

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_cmpnam
// Desc.:    Vergleicht a und b ueber len Zeichen nach Q9-Namensregeln. Rueckgabe: 0 = gleich,
//           sonst Q9-Fehlercode/ungleich.
// Call:     err = q9_cmpnam("D0", "d0", 2)
//════════════════════════════════════════════════════════════════════════════════════════════════
int q9_cmpnam(const char *a, const char *b, uint16_t len);

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_id
// Desc.:    Liefert Prozess-ID und User-ID ueber *out_pid und *out_uid. Rueckgabe: 0 = ok,
//           sonst Q9-Fehlercode.
// Call:     err = q9_id(&pid, &uid)
//════════════════════════════════════════════════════════════════════════════════════════════════
int q9_id(uint16_t *out_pid, uint32_t *out_uid);

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_link
// Desc.:    Sucht ein Modul nach name/type/lang und linkt es. Optional liefert die Funktion
//           Header, Directory-Eintrag und Revision. Rueckgabe: 0 = ok, sonst Q9-Fehlercode.
// Call:     err = q9_link("term", type, lang, &hdr, &ent, &rev)
//════════════════════════════════════════════════════════════════════════════════════════════════
int q9_link(const char *name, uint8_t type, uint8_t lang,
            void **out_header, void **out_entry, uint8_t *out_rev);

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_unlink
// Desc.:    Loest einen zuvor gelinkten Modul-Header. header darf nicht NULL sein; Rueckgabe:
//           0 = ok, sonst Q9-Fehlercode.
// Call:     err = q9_unlink(hdr)
//════════════════════════════════════════════════════════════════════════════════════════════════
int q9_unlink(void *header);

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_load
// Desc.:    Laedt ein Modul aus "path" und linkt es. Optional liefert die Funktion Header,
//           Directory-Eintrag und Revision. Rueckgabe: 0 = ok, sonst Q9-Fehlercode.
// Call:     err = q9_load("/d0/MOD", &hdr, &ent, &rev)
//════════════════════════════════════════════════════════════════════════════════════════════════
int q9_load(const char *path, void **out_header, void **out_entry, uint8_t *out_rev);

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_time
// Desc.:    Liest die aktuelle Q9-Zeit nach *out_time inklusive Datum, Uhrzeit, Wochentag und
//           Boot-Ticks. Rueckgabe: 0 = ok, sonst Q9-Fehlercode.
// Call:     err = q9_time(&time)
//════════════════════════════════════════════════════════════════════════════════════════════════
int q9_time(q9_time_t *out_time);

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_stime
// Desc.:    Setzt die Q9-Zeit aus *time; Datum und Uhrzeit werden in syscall-Register gepackt.
//           Rueckgabe: 0 = ok, sonst Q9-Fehlercode.
// Call:     err = q9_stime(&time)
//════════════════════════════════════════════════════════════════════════════════════════════════
int q9_stime(const q9_time_t *time);

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_exit
// Desc.:    Beendet den aktuellen Q9-Prozess mit status (derzeit Kernel-Stub fuer Proto-Prozess).
//           Rueckgabe: 0 = ok, sonst Q9-Fehlercode.
// Call:     err = q9_exit(0)
//════════════════════════════════════════════════════════════════════════════════════════════════
int q9_exit(uint16_t status);

#endif // Q9_USERLAND_LIBQ9_H

//────────────────────────────────────────────────────────────────────────────────────────────────
// EOF libq9.h                                                                            Ver. 1.00
//────────────────────────────────────────────────────────────────────────────────────────────────
