//════════════════════════════════════════════════════════════════════════════════════════════════
// File:   14_test_dhf_driver.c                                                            Ver. 1.00
// Owner:  Claude
// Desc.:  Ende-zu-Ende-Test fuer das ECHTE 68k-Treibermodul (Q9-OS/Q9-DHF-68k/driver/dhfdrv_68k.a,
//         per psect/qr68k/ql68k gebaut, s. dortige STATUS.md). Anders als test/13_test_dhf.c (das
//         die C-Vtable q9_devtype_dhf DIREKT anspricht, ohne 68k-CPU) laedt dieser Test das
//         assemblierte Modul dhfdrv_68k.mod in Gast-RAM und fuehrt seine Init/Read/Write/GetStat/
//         SetStat-Einspruenge ueber die echte Musashi-CPU aus (m68k_execute), genau wie es OS-9s
//         IOMan beim Aufruf ueber die Sprungtabelle taete -- inklusive der echten MMIO-Bus-
//         Weiterleitung (q9_m68krt_attach_board -> q9_devtype_dhf -> dhf_emu_device.c ->
//         Host-Dateisystem). Beweist damit, dass die Registerkonvention (a1 = Zeiger auf einen
//         28-Byte dhf_shared-Block im Gast-RAM, Rueckgabe ueber Carry+d1.w) tatsaechlich
//         funktioniert, nicht nur, dass der Modulkopf wohlgeformt ist (das leistete bereits
//         "os9 ident"/"os9 dump" beim Bau).
//
//         Sprungtabelle: Read/Write/GetStat/SetStat zeigen laut Quelltext (dhfdrv_68k.a) alle auf
//         DENSELBEN Code -- der Treiber selbst zerlegt keine Kommandos, er kopiert nur den
//         kompletten dhf_shared-Block roh in die MMIO-Registerbank, schreibt das command-Byte
//         (loest im Geraet die eigentliche Verarbeitung aus) und kopiert das Ergebnis zurueck. Der
//         Test nutzt deshalb durchgaengig den "Write"-Einsprung fuer ALLE DHF_CMD_*-Kommandos
//         (CREATE/OPEN/READ/CLOSE/...) -- das ist keine Testvereinfachung, sondern spiegelt exakt
//         die Absicht aus der Session ("der Treiber ist recht einfach, der Simulator zerlegt").
//         Eine explizite Pruefung bestaetigt zusaetzlich, dass alle vier Einspruenge wirklich auf
//         dieselbe Adresse zeigen (Regressionsschutz gegen versehentliche Entkopplung).
//
//         Modul-Header-Format: die 7-Wort-Sprungtabelle wird NICHT an einem festen Byte-Offset
//         erwartet, sondern ueber das Standard-OS-9-Kopffeld an Byte-Offset $32 (Big-Endian,
//         16 Bit) gefunden -- das ist derselbe Mechanismus, den IOMan bei einem echten Modul
//         verwendet, robust gegen kuenftige Kopfgroessenaenderungen im Assembler/Linker.
//
// Call:   make test-dhf-driver   (baut das Modul per qr68k/ql68k frisch, dann diesen Test)
//         Setzt voraus, dass Q9-OS als Geschwister-Repo neben Q9-Forge/Q9-Flux ausgecheckt ist
//         (../../Q9-OS/Q9-DHF-68k/driver/dhfdrv_68k.mod) -- wie bei jeder Mehr-Repo-Arbeit in
//         Q9-Forge (s. Makefile-Kommentarkopf).
//
// Edition History
//─────────┬──────┬────────────────────────────────────────────────────────────────────────┬──────
// Date    │ Ver. │ Description                                                            │ By
//─────────┼──────┼────────────────────────────────────────────────────────────────────────┬──────
// 26-09-25│ 1.00 │ Initiale Version                                                        │ Cld
//═════════╧══════╧════════════════════════════════════════════════════════════════════════╧══════
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stddef.h>
#include <unistd.h>

#include "../src/kernel/m68krt.h"
#include "../src/kernel/q9board.h"
#include "../src/devices/dhf/q9_dhf.h"
#include "../src/devices/dhf/dhf_proto.h"
#include "../src/devices/dhf/dhf_shared.h"
#include "m68k.h"

#define MODULE_PATH   "../../Q9-OS/Q9-DHF-68k/driver/dhfdrv_68k.mod"

#define MOD_LOAD_ADDR    0x00001000u
#define SHARED_ADDR      0x00002000u   /* a1-Zeiger: 28-Byte dhf_shared-Block, s. dhf_shared.h  */
#define PATH_ADDR        0x00002100u
#define PATH2_ADDR       0x00002140u
#define DATA_ADDR        0x00002200u
#define DATA2_ADDR       0x00002300u
#define STACK_TOP        0x0000F000u
#define TRAMPOLINE_ADDR  0x00001F00u   /* stop #$2700 -- Rueckkehrpunkt nach dem Modul-rts       */
#define ENTRY_OFF_FIELD  0x00000032u   /* Standard-OS-9-Kopffeld: 16-Bit-Offset zur Sprungtabelle */

enum { E_INIT = 0, E_READ, E_WRITE, E_GETSTAT, E_SETSTAT, E_TERM, E_RESERVED, E_COUNT };

static int      g_fails = 0;
static uint32_t g_entry[E_COUNT];
static q9_m68krt_t g_rt;

static void check(const char *label, int ok)
{
    printf("%-58s %s\n", label, ok ? "OK" : "FAIL");
    if (!ok) g_fails++;
}

static void put_str(uint32_t addr, const char *s)
{
    size_t i, n = strlen(s) + 1;
    for (i = 0; i < n; i++) m68k_write_memory_8((uint32_t)(addr + i), (uint8_t)s[i]);
}

static void sh_set8(uint32_t off, uint8_t v)   { m68k_write_memory_8(SHARED_ADDR + off, v); }
static void sh_set32(uint32_t off, uint32_t v) { m68k_write_memory_32(SHARED_ADDR + off, v); }
static uint32_t sh_get32(uint32_t off)         { return m68k_read_memory_32(SHARED_ADDR + off); }

/* Ruft einen der sechs Einspruenge des Moduls ueber die echte Musashi-CPU auf. a1 zeigt auf den
   vorbereiteten dhf_shared-Block; a2/a4/a6 (statischer Speicher/Prozessdeskriptor/Systemglobal)
   bleiben 0 -- der Treiber fasst sie laut Quelltext nicht an. Laeuft bis das Modul per rts zur
   Trampolin-Adresse zurueckkehrt und dort "stop" ausfuehrt (erkennbar an q9_m68krt_is_stopped()),
   oder bis das Zyklenbudget ausgeht (dann FAIL -- Absturz/Endlosschleife im Modul). */
static int call_entry(int idx, uint32_t *out_d0, uint8_t *out_carry, uint8_t *out_d1)
{
    uint32_t sp;
    int budget;
    /* m68k_pulse_reset() (s. q9_m68krt_reset) bringt die CPU zwischen den Aufrufen in einen
       sauberen, definierten Zustand (Prefetch/interne Zustandsmaschine) -- RAM/board.remapped
       bleiben davon unberuehrt (q9_m68krt_reset ruft nur m68k_pulse_reset(), s. m68krt.c). */
    q9_m68krt_reset(&g_rt);
    sp = STACK_TOP - 4u;
    m68k_write_memory_32(sp, TRAMPOLINE_ADDR);
    m68k_set_reg(M68K_REG_A7, sp);
    m68k_set_reg(M68K_REG_A1, SHARED_ADDR);
    m68k_set_reg(M68K_REG_A2, 0);
    m68k_set_reg(M68K_REG_A4, 0);
    m68k_set_reg(M68K_REG_A6, 0);
    m68k_set_reg(M68K_REG_SR, 0x2700u);           /* Supervisor, IPL7, alle Flags geloescht        */
    m68k_set_reg(M68K_REG_PC, MOD_LOAD_ADDR + g_entry[idx]);

    /* Trampolin ist "bra.s *" (Selbstschleife, s.u.), KEIN "stop" -- stop wuerde das komplette SR
       (samt Carry, dem Rueckgabekanal fuer Fehler!) mit seinem Sofortoperanden ueberschreiben und
       so genau das Ergebnis zerstoeren, das dieser Test lesen will. Grosszuegiges, aber festes
       Zyklenbudget statt is_stopped()-Polling; die winzige Treiberroutine (ein paar Dutzend
       Instruktionen, laengste Schleife 24 Byte-Kopien) braucht davon nur einen Bruchteil. */
    budget = 4000;
    while (budget > 0) {
        budget -= q9_m68krt_execute(&g_rt, 200);
    }
    if (m68k_get_reg(NULL, M68K_REG_PC) != TRAMPOLINE_ADDR) {
        return 0;                       /* nicht zur Trampolin-Adresse zurueckgekehrt -- Absturz/Crash */
    }
    if (out_d0)    *out_d0    = m68k_get_reg(NULL, M68K_REG_D0);
    if (out_carry) *out_carry = (uint8_t)(m68k_get_reg(NULL, M68K_REG_SR) & 1u);
    if (out_d1)    *out_d1    = (uint8_t)(m68k_get_reg(NULL, M68K_REG_D1) & 0xFFu);
    return 1;
}

/* Fuehrt EIN DHF-Kommando komplett durch: fuellt den dhf_shared-Block (Header-Bytes bleiben bei 0,
   nur command+seq/a0/a1/d0/d1/d2 werden gesetzt, wie es ein Manager taete), ruft den Treiber ueber
   den "Write"-Einsprung auf (s. Kopfkommentar) und liefert d0/carry/d1 zurueck. */
static int run_cmd(uint8_t cmd, uint32_t a0, uint32_t a1, uint32_t d0, uint32_t d1, uint32_t d2,
                    uint32_t *out_d0, uint8_t *out_carry, uint8_t *out_d1)
{
    sh_set8(offsetof(struct dhf_shared, command), cmd);
    sh_set32(offsetof(struct dhf_shared, a0), a0);
    sh_set32(offsetof(struct dhf_shared, a1), a1);
    sh_set32(offsetof(struct dhf_shared, d0), d0);
    sh_set32(offsetof(struct dhf_shared, d1), d1);
    sh_set32(offsetof(struct dhf_shared, d2), d2);
    return call_entry(E_WRITE, out_d0, out_carry, out_d1);
}

int main(void)
{
    static uint8_t board_ram[256u * 1024u];
    static q9_board_t board;
    uint8_t modbuf[1024];
    size_t modlen;
    uint32_t entry_table_off;
    int i;

    FILE *f = fopen(MODULE_PATH, "rb");
    if (!f) {
        fprintf(stderr, "FAIL Modul nicht gefunden: %s\n"
                        "     (Q9-OS muss als Geschwister-Repo neben Q9-Flux ausgecheckt sein)\n",
                MODULE_PATH);
        return 2;
    }
    modlen = fread(modbuf, 1, sizeof(modbuf), f);
    fclose(f);
    printf("Modul geladen: %s (%zu Byte)\n\n", MODULE_PATH, modlen);
    if (modlen == 0 || modlen >= sizeof(modbuf)) {
        fprintf(stderr, "FAIL unplausible Modulgroesse (%zu Byte)\n", modlen);
        return 2;
    }

    if (q9_board_init(&board, NULL, 0, board_ram, sizeof(board_ram)) != Q9_BOARD_OK) {
        fprintf(stderr, "FAIL q9_board_init fehlgeschlagen\n");
        return 2;
    }
    if (q9_m68krt_init(&g_rt, board_ram, sizeof(board_ram), Q9_CPU_68030) != Q9_M68KRT_OK) {
        fprintf(stderr, "FAIL q9_m68krt_init fehlgeschlagen\n");
        return 2;
    }
    /* registriert u.a. dhf0 an Q9_BOARD_DHF_BASE, vorerst mit dem hartkodierten echten
       OS9SYS-Basepath (s. m68krt.c) -- sofort danach auf ein wegwerfbares Testverzeichnis
       umgebogen, damit dieser Test NICHT im echten Q9-Images-Abbild herumschreibt. */
    q9_m68krt_attach_board(&board);
    q9_m68krt_reset(&g_rt);
    /* Reset-Zustand (docs/BOARD.md): unterhalb $FFFF0000 ist erst ROM gespiegelt sichtbar, RAM-
       Schreibzugriffe werden board_write_byte zufolge kommentarlos verworfen, bis der REMAP-
       Trigger ausgeloest wurde. Dieser Test hat kein Boot-ROM (rom_len=0) und keinen 68k-
       Bootvorgang, der den Trigger selbst ansprechen wuerde -- den Zustand direkt setzen. */
    board.remapped = 1;

    char tmpl[] = "/tmp/q9dhfdrv_test_XXXXXX";
    char *tmpdir = mkdtemp(tmpl);
    if (!tmpdir) { fprintf(stderr, "FAIL mkdtemp\n"); return 2; }
    q9_dhf_init(&board.dhf, tmpdir, board.ram, board.ram_len);
    printf("DHF-Testverzeichnis: %s\n\n", tmpdir);

    for (i = 0; i < (int)modlen; i++) {
        m68k_write_memory_8((uint32_t)(MOD_LOAD_ADDR + i), modbuf[i]);
    }
    entry_table_off = m68k_read_memory_16(MOD_LOAD_ADDR + ENTRY_OFF_FIELD);
    check("Sprungtabellen-Offset im Modulkopf plausibel",
          entry_table_off > 0 && entry_table_off + 14u <= modlen);
    for (i = 0; i < E_COUNT; i++) {
        g_entry[i] = m68k_read_memory_16(MOD_LOAD_ADDR + entry_table_off + (uint32_t)(i * 2));
    }
    printf("Sprungtabelle: Init=$%04x Read=$%04x Write=$%04x GetStat=$%04x SetStat=$%04x Term=$%04x\n\n",
           g_entry[E_INIT], g_entry[E_READ], g_entry[E_WRITE], g_entry[E_GETSTAT],
           g_entry[E_SETSTAT], g_entry[E_TERM]);
    check("Read/Write/GetStat/SetStat zeigen auf denselben Code (Treiber-Design, s. Kopfkommentar)",
          g_entry[E_READ] == g_entry[E_WRITE] && g_entry[E_WRITE] == g_entry[E_GETSTAT] &&
          g_entry[E_GETSTAT] == g_entry[E_SETSTAT]);
    check("Init und Term zeigen auf denselben (leeren) Code", g_entry[E_INIT] == g_entry[E_TERM]);

    m68k_write_memory_16(TRAMPOLINE_ADDR, 0x60FEu);        /* bra.s * -- Selbstschleife, Flags unberuehrt */

    /* --- Init ueber die echte CPU aufrufen (leerer Rumpf, muss sauber mit geloeschtem Carry
       zurueckkehren) --- */
    {
        uint32_t d0; uint8_t carry, d1;
        int ret = call_entry(E_INIT, &d0, &carry, &d1);
        check("Init kehrt zurueck (kein Timeout/Absturz)", ret);
        check("Init: Carry geloescht", carry == 0);
    }

    /* --- CREATE hello.txt, WRITE "Hello DHF!", CLOSE --- */
    put_str(PATH_ADDR, "hello.txt");
    uint32_t d0; uint8_t carry, d1;
    int ret = run_cmd(DHF_CMD_CREATE, PATH_ADDR, 0, 0, 0644u, DHF_MODE_WRITE, &d0, &carry, &d1);
    check("CREATE hello.txt kehrt zurueck", ret);
    check("CREATE hello.txt: kein Fehler (Carry geloescht)", carry == 0);
    uint32_t handle = sh_get32(offsetof(struct dhf_shared, d0));

    const char *payload = "Hello DHF!";
    size_t plen = strlen(payload);
    for (i = 0; i < (int)plen; i++) m68k_write_memory_8((uint32_t)(DATA_ADDR + i), (uint8_t)payload[i]);
    ret = run_cmd(DHF_CMD_WRITE, 0, DATA_ADDR, handle, (uint32_t)plen, 0, &d0, &carry, &d1);
    check("WRITE 10 Byte kehrt zurueck", ret);
    check("WRITE: kein Fehler, D0 == geschriebene Byteanzahl", carry == 0 && d0 == plen);

    ret = run_cmd(DHF_CMD_CLOSE, 0, 0, handle, 0, 0, &d0, &carry, &d1);
    check("CLOSE (nach WRITE): kein Fehler", ret && carry == 0);

    /* --- OPEN (lesend), READ zurueck, Inhalt vergleichen --- */
    put_str(PATH_ADDR, "hello.txt");
    ret = run_cmd(DHF_CMD_OPEN, PATH_ADDR, 0, 0, 0, DHF_MODE_READ, &d0, &carry, &d1);
    check("OPEN hello.txt: kein Fehler", ret && carry == 0);
    uint32_t handle2 = sh_get32(offsetof(struct dhf_shared, d0));

    ret = run_cmd(DHF_CMD_READ, 0, DATA2_ADDR, handle2, 64, 0, &d0, &carry, &d1);
    check("READ: kein Fehler, genau 10 Byte", ret && carry == 0 && d0 == plen);
    {
        char got[16];
        for (i = 0; i < (int)plen; i++) got[i] = (char)m68k_read_memory_8((uint32_t)(DATA2_ADDR + i));
        got[plen] = 0;
        check("READ-Inhalt == geschriebener Inhalt (\"Hello DHF!\")", strncmp(got, payload, plen) == 0);
    }
    ret = run_cmd(DHF_CMD_CLOSE, 0, 0, handle2, 0, 0, &d0, &carry, &d1);
    check("CLOSE (nach READ): kein Fehler", ret && carry == 0);

    /* --- Fehlerpfad: Pfadflucht (Basepath-Confinement) muss ueber Carry+d1 sichtbar werden --- */
    put_str(PATH2_ADDR, "../escape.txt");
    ret = run_cmd(DHF_CMD_CREATE, PATH2_ADDR, 0, 0, 0644u, DHF_MODE_WRITE, &d0, &carry, &d1);
    check("CREATE mit \"..\" kehrt zurueck", ret);
    check("CREATE mit \"..\": Carry GESETZT (Fehler wie erwartet)", carry == 1);
    check("CREATE mit \"..\": d1 traegt den DHF-Fehlercode (!= 0)", d1 != 0);

    /* --- DELETE hello.txt aufraeumen, dann Term --- */
    put_str(PATH_ADDR, "hello.txt");
    ret = run_cmd(DHF_CMD_DELETE, PATH_ADDR, 0, 0, 0, 0, &d0, &carry, &d1);
    check("DELETE hello.txt: kein Fehler", ret && carry == 0);
    ret = call_entry(E_TERM, &d0, &carry, &d1);
    check("Term kehrt zurueck, Carry geloescht", ret && carry == 0);

    printf("\n%s\n", g_fails == 0 ? "ALLE TESTS BESTANDEN" : "TESTS FEHLGESCHLAGEN");

    char rmcmd[600];
    snprintf(rmcmd, sizeof(rmcmd), "rm -rf '%s'", tmpdir);
    if (system(rmcmd) != 0) { /* best effort */ }

    return g_fails == 0 ? 0 : 1;
}
//────────────────────────────────────────────────────────────────────────────────────────────────
// EOF 14_test_dhf_driver.c                                                                Ver. 1.00
//────────────────────────────────────────────────────────────────────────────────────────────────
