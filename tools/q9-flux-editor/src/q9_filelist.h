//════════════════════════════════════════════════════════════════════════════════════════════════
// File:   q9_filelist.h                                                                   Ver. 1.00
// Owner:  Claudia
// Desc.:  Verzeichnisinhalt fuer den kommenden Datei-Auswahl-Dialog (Q9FLUX_EDITOR_de.md Abschnitt
//         2) -- Andreas' Wunsch: echte Dateien auflisten, mit Name/Datum/Groesse, gefiltert nach
//         Extension. NUR das direkte Verzeichnis (keine Unterordner-Navigation, kein Rekursion) --
//         der Aufrufer gibt das Verzeichnis fest vor ("ich muss natuerlich vorgeben welches
//         Verzeichnis"), bewusst einfach gehalten. Fester Array (kein malloc, Q9-Grundsatz).
//
// Call:   q9_filelist_t fl;
//         int n = q9_filelist_scan("/pfad/zu/configs", ".q9", &fl);
//         if (n < 0) { /* Verzeichnis nicht lesbar */ }
//         for (i = 0; i < fl.count; i++) { printf("%s %s %s\n", fl.entry[i].name, ...); }
//
// Edition History
//─────────┬──────┬────────────────────────────────────────────────────────────────────────┬──────
// Date    │ Ver. │ Description                                                            │ By
//─────────┼──────┼────────────────────────────────────────────────────────────────────────┬──────
// 26-08-17│ 1.00 │ Erster Wurf -- POSIX (opendir/readdir/stat) implementiert+getestet,      │ Cld
//         │      │ Windows-Zweig (FindFirstFile/FindNextFile) geschrieben, mangels Windows- │
//         │      │ Host hier UNGETESTET                                                     │
//═════════╧══════╧════════════════════════════════════════════════════════════════════════╧══════
#ifndef Q9_FILELIST_H
#define Q9_FILELIST_H

#define Q9_FILELIST_MAX_ENTRIES 256                        /* mehr Config-/Image-Dateien in einem
                                                                Verzeichnis sind unrealistisch --
                                                                zusaetzliche Eintraege werden beim
                                                                Scan stillschweigend abgeschnitten */
#define Q9_FILELIST_NAME_MAX 64
#define Q9_FILELIST_DATE_MAX 16                             /* vorformatiert "DD.MM.YY", kompakt   */
#define Q9_FILELIST_SIZE_MAX 12                             /* vorformatiert "12K"/"1.2M"/... kompakt */

typedef struct {
    char name[Q9_FILELIST_NAME_MAX];
    char date[Q9_FILELIST_DATE_MAX];                        /* bereits fertig zum Anzeigen           */
    char size[Q9_FILELIST_SIZE_MAX];                         /* bereits fertig zum Anzeigen (Kurzform) */
    long size_bytes;                                         /* roh, fuer eine kuenftige Sortierung   */
} q9_fileentry_t;

typedef struct {
    q9_fileentry_t entry[Q9_FILELIST_MAX_ENTRIES];
    int count;
} q9_filelist_t;

//════════════════════════════════════════════════════════════════════════════════════════════════
// Function: q9_filelist_scan
// Desc.:    Listet REGULAERE Dateien (keine Unterverzeichnisse, keine Rekursion) direkt in dir auf,
//           alphabetisch sortiert. ext_filter ist die geforderte Dateiendung OHNE Punkt-Pflicht
//           (z.B. "q9" oder ".q9" -- beides gleichwertig, case-insensitiv verglichen);
//           NULL/""/"*"/"*.*" bedeutet "alle Dateien, kein Filter". Versteckte Dateien (Name
//           beginnt mit '.') werden UEBERSPRUNGEN (typische Konvention, vermeidet .DS_Store,
//           .git usw. in der Liste). Rueckgabe: Anzahl gefundener Eintraege (>=0, auf
//           Q9_FILELIST_MAX_ENTRIES geklemmt), oder -1 wenn dir nicht lesbar ist (out->count wird
//           dann auf 0 gesetzt, kein undefinierter Zustand).
// Call:     int n = q9_filelist_scan("/pfad", ".q9", &fl)
//════════════════════════════════════════════════════════════════════════════════════════════════
int q9_filelist_scan(const char *dir, const char *ext_filter, q9_filelist_t *out);

#endif /* Q9_FILELIST_H */
//────────────────────────────────────────────────────────────────────────────────────────────────
// EOF q9_filelist.h                                                                       Ver. 1.00
//────────────────────────────────────────────────────────────────────────────────────────────────
