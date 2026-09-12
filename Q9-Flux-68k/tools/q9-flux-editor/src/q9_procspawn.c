//════════════════════════════════════════════════════════════════════════════════════════════════
// File:   q9_procspawn.c                                                                  Ver. 1.00
// Owner:  Claudia
// Desc.:  Implementierung, siehe q9_procspawn.h.
//
// Edition History
//─────────┬──────┬────────────────────────────────────────────────────────────────────────┬──────
// Date    │ Ver. │ Description                                                            │ By
//─────────┼──────┼────────────────────────────────────────────────────────────────────────┬──────
// 26-08-16│ 1.00 │ Erster Wurf                                                              │ Cld
//═════════╧══════╧════════════════════════════════════════════════════════════════════════╧══════
#include "q9_procspawn.h"

#ifdef _WIN32

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <stdio.h>
#include <string.h>

int q9_procspawn_run(const char *path, char *const argv[])
{
    STARTUPINFOA si;
    PROCESS_INFORMATION pi;
    char cmdline[4096];
    unsigned pos = 0;
    int i;
    DWORD exit_code;

    /* Windows kennt kein argv-Array fuer CreateProcess -- die Kommandozeile muss als EIN String
       zusammengesetzt werden. Jedes Argument in Anfuehrungszeichen -- fuer unsere bekannten
       Aufrufe (Programmpfad + Config-Dateiname) ausreichend; KEIN vollstaendiges Windows-
       Kommandozeilen-Escaping fuer Sonderzeichen wie eingebettete Anfuehrungszeichen. */
    cmdline[0] = '\0';
    for (i = 0; argv[i] != NULL; i++) {
        int n = snprintf(cmdline + pos, sizeof(cmdline) - pos, "%s\"%s\"",
                          i > 0 ? " " : "", argv[i]);
        if (n < 0 || (unsigned)n >= sizeof(cmdline) - pos) {
            return -1;                                  /* Kommandozeile zu lang fuer den Puffer */
        }
        pos += (unsigned)n;
    }

    memset(&si, 0, sizeof(si));
    si.cb = sizeof(si);
    memset(&pi, 0, sizeof(pi));

    if (!CreateProcessA(path, cmdline, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
        return -1;
    }
    WaitForSingleObject(pi.hProcess, INFINITE);
    if (!GetExitCodeProcess(pi.hProcess, &exit_code)) {
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        return -1;
    }
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    return (int)exit_code;
}

#else /* POSIX */

#include <unistd.h>
#include <sys/wait.h>

int q9_procspawn_run(const char *path, char *const argv[])
{
    pid_t pid;
    int status;

    pid = fork();
    if (pid < 0) {
        return -1;
    }
    if (pid == 0) {
        /* Kind: execv ersetzt das Prozessabbild komplett -- kehrt nur bei Fehler zurueck.
           _exit() statt exit(): keine doppelte stdio-Pufferleerung mit dem Elternprozess (der
           Elternprozess koennte selbst noch ungeleerte Puffer haben, fork() dupliziert die). */
        execv(path, argv);
        _exit(127);                                     /* Shell-Konvention: "Kommando nicht gefunden" */
    }
    /* Elternprozess: blockierend warten, bis das Kind fertig ist. */
    if (waitpid(pid, &status, 0) < 0) {
        return -1;
    }
    if (WIFEXITED(status)) {
        return WEXITSTATUS(status);
    }
    return -2;                                           /* per Signal beendet, kein regulaerer Exit */
}

#endif /* _WIN32 */

//────────────────────────────────────────────────────────────────────────────────────────────────
// EOF q9_procspawn.c                                                                      Ver. 1.00
//────────────────────────────────────────────────────────────────────────────────────────────────
