# Musashi-PMMU-Fix: Table-B-Fault als Bus-Error statt Emulator-Abbruch

## Problem

Ein Table-B-Deskriptor mit Mode 0 (ungueltig, Uebersetzungsfehler) fuehrte
bisher zu `fatalerror()` in `pmmu_translate_addr()` (`m68kmmu.h`) — der
gesamte Q9-Emulatorprozess wurde beendet. Echte 68030-Hardware liefert einen
solchen PMMU-Fault dagegen als Bus-Error an den Gast (OS-9) aus; dessen
Exception-Handler bekommt so die Chance, den Fehler zu behandeln oder zu
melden, statt dass Q9 komplett stirbt.

Zusaetzlich enthielt `m68ki_exception_bus_error()` (`m68kcpu.h`) einen
eigenstaendigen Bug: Der Stack-Frame fuer den Bus-Error war fest auf das
kurze Format-8-Layout (`m68ki_stack_frame_1000`) verdrahtet, das **nur fuer
68010** gueltig ist. Auf 68020/68030 erwartet der Gast das lange
Format-B-Layout; mit dem falschen Frame konnte aus einem einzelnen
PMMU-Fault eine Reset-Schleife werden, weil der Exception-Handler ein
falsches Stack-Layout vorfand.

## Fix

| Datei | Aenderung |
|---|---|
| `third_party/musashi/m68kcpu.h` | `m68ki_exception_bus_error()`: fuer `CPU_TYPE_IS_020_PLUS(CPU_TYPE)` wird `m68ki_stack_frame_1011()` (Format B) statt `m68ki_stack_frame_1000()` (Format 8) verwendet |
| `third_party/musashi/m68kcpu.h` | Neue `q9_mmu_access_*`-Variablen + `m68ki_set_mmu_access()`, in allen sechs `m68ki_read/write_*_fc()`-Varianten aufgerufen — haelt Adresse/Function-Code/Richtung/Groesse des aktuellen Speicherzugriffs fest |
| `third_party/musashi/m68kmmu.h` | Table-B `case 0` in `pmmu_translate_addr()`: statt `fatalerror()` wird jetzt `m68ki_exception_bus_error()` aufgerufen; zusaetzliche Debug-Ausgabe (TC/SRP/CRP-Root-Pointer, fehlerhafte Instruktion PPC/PC/IR, Zugriffsadresse/-richtung/-groesse) |
| `third_party/musashi/m68kmmu.h` | Table-C `case 0`: **unveraendert**, ruft weiterhin `fatalerror()` — nur die gleiche zusaetzliche Debug-Ausgabe wurde ergaenzt. Nicht mitgezogen, weil Table-C-Faults in der Praxis noch nicht beobachtet/getestet wurden (Stand 2026-07-31, offen fuer Nachzug) |

Commit: `245a613053b7e4acf063117113d8f9e1c394411b`
Vorheriger, unveraenderter Stand: `245a613~1` (`9ade754`, "Musashi:
Register-/Opcode-Dump vor PMMU-Table-B-Fatalerror")

## Originalzustand wiederherstellen

Der Originalcode ist nicht geloescht, sondern per Git jederzeit vollstaendig
abrufbar. Kompletten Fix rueckgaengig machen:

```sh
git revert 245a613
```

Nur zum Ansehen des Originalcodes, ohne etwas zu aendern:

```sh
git show 245a613~1:third_party/musashi/m68kcpu.h
git show 245a613~1:third_party/musashi/m68kmmu.h
```

Die beiden betroffenen Stellen im Original, zur schnellen Referenz ohne
Git-Aufruf:

**`m68kcpu.h`, `m68ki_exception_bus_error()` (vorher):**

```c
static inline void m68ki_exception_bus_error(void)
{
	/* ... */
	uint sr = m68ki_init_exception();

	/* Note: This is implemented for 68010 only! */
	m68ki_stack_frame_1000(REG_PPC, sr, EXCEPTION_BUS_ERROR);

	m68ki_jump_vector(EXCEPTION_BUS_ERROR);
	/* ... */
}
```

**`m68kmmu.h`, Table-B `case 0` in `pmmu_translate_addr()` (vorher):**

```c
case 0:	// invalid, should cause MMU exception
	fprintf(stderr, "680x0 PMMU DEBUG: D0-D7 %08x %08x %08x %08x %08x %08x %08x %08x\n",
		REG_D[0], REG_D[1], REG_D[2], REG_D[3], REG_D[4], REG_D[5], REG_D[6], REG_D[7]);
	fprintf(stderr, "680x0 PMMU DEBUG: A0-A7 %08x %08x %08x %08x %08x %08x %08x %08x\n",
		REG_A[0], REG_A[1], REG_A[2], REG_A[3], REG_A[4], REG_A[5], REG_A[6], REG_A[7]);
	fprintf(stderr, "680x0 PMMU DEBUG: USP %08x SR %04x\n", REG_USP, m68ki_get_sr());
	{
		int dbgi;
		fprintf(stderr, "680x0 PMMU DEBUG: bytes at PC-8..PC+15:");
		for (dbgi = -8; dbgi < 16; dbgi++)
		{
			fprintf(stderr, "%s%02x", (dbgi == 0) ? " |" : " ", m68k_read_memory_8(REG_PC + dbgi));
		}
		fprintf(stderr, "\n");
	}
	fatalerror("680x0 PMMU: Unhandled Table B mode %d (addr_in %08x PC %x)\n", tbmode, addr_in, REG_PC);
	break;
```

## Offen

- Table-C-Fault (siehe oben) noch nicht auf denselben Bus-Error-Pfad
  umgestellt.
- Live-Verifikation im vollen Q9-Testlauf steht noch aus (der "mildere
  PMMU-Fehler (A0=1)" aus dem QCC-Vollport-Status war der Ausloeser fuer
  diesen Fix).
