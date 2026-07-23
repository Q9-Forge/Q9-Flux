# CF Write Test - Anleitung

## Zweck
Testet den Fix für 256-Byte RBF-Images: RAM-Module werden ins CF-Image geschrieben und verifiziert.

## Vorbereitung

1. **Build durchführen:**
   ```bash
   make clean && make native
   ```

2. **Test-Image erstellen:**
   ```bash
   ./test/cf/test_save_modules.sh
   ```
   
   Erstellt: `local_images/Q9-cb030-work-test.hda` (Kopie von Q9-cb030-work-max.hda)

## Test ausführen

### Option A: Automatisch (mit ROM-Image)

```bash
./test/expect/save_ram_modules.exp /pfad/zum/rom-image.bin
```

Das Script:
- Startet den CB030-Emulator mit CF-Tracing
- Wartet auf Shell-Prompt
- Erstellt `/d0/CMDS/BOOTOBJS/RAMTEST`
- Speichert Module: OS9, Init, IOMan, RBF, SCF, etc.
- Listet gespeicherte Dateien auf
- Beendet den Emulator

### Option B: Manuell

```bash
export Q9_CB030_CF_TRACE=1
./build/native/q9.exe --cb030 /pfad/zum/rom.bin --cf local_images/Q9-cb030-work-test.hda
```

Im Emulator:
```
$ setime 26/07/05 20:00:00
$ mdir
$ makdir /d0/CMDS/BOOTOBJS/RAMTEST
$ save OS9 /d0/CMDS/BOOTOBJS/RAMTEST/OS9
$ save Init /d0/CMDS/BOOTOBJS/RAMTEST/Init
$ save IOMan /d0/CMDS/BOOTOBJS/RAMTEST/IOMan
...
$ dir -e /d0/CMDS/BOOTOBJS/RAMTEST
$ [Ctrl-C zum Beenden]
```

## Verifikation

```bash
./test/cf/verify_cf_writes.sh
```

Prüft:
1. Disk-Struktur (dcheck)
2. Existenz des RAMTEST-Verzeichnisses
3. Gespeicherte Module
4. CRC-Integrität jedes Moduls

**Erwartetes Ergebnis:**
- ✓ Disk structure OK
- ✓ Directory exists
- ✓ Alle Module mit "Good CRC"

**Bei Fehler (vor dem Fix):**
- ✗ Disk corrupted
- ✗ Module mit "Bad CRC"
- → Bytes 256-511 gingen verloren!

## Tracing

CF-Trace aktivieren für detaillierte Logs:
```bash
export Q9_CB030_CF_TRACE=1
```

Zeigt:
- `[cf read lba=X first=...]` - Read-Operationen
- `[cf write lba=X first=...]` - Write-Operationen
- `[cf image-sector-size=256]` - Erkannte Sektorgröße

## Der Fix

**Problem:** Bei 256-Byte-Images schrieb `cb030_cf_store_sector()` nur 1 Sektor (256 Bytes),
obwohl der Buffer 2 Sektoren (512 Bytes) enthält.

**Lösung:**
```c
if (img_sec < Q9_CB030_CF_SECTOR_SIZE) {
    fwrite(b->cf_sector, 1, img_sec, b->cf_file);
    fseek(b->cf_file, (long)(b->cf_lba + 1u) * (long)img_sec, SEEK_SET);
    fwrite(b->cf_sector + img_sec, 1, Q9_CB030_CF_SECTOR_SIZE - img_sec, b->cf_file);
}
```

## Dateien

- `test/cf/test_save_modules.sh` - Test-Image vorbereiten
- `test/expect/save_ram_modules.exp` - Automatisierter Test (expect)
- `test/cf/verify_cf_writes.sh` - Verifikation der Resultate
- `BUGFIX_CF_WRITE.md` - Technische Bugfix-Dokumentation

## Troubleshooting

**"ROM-Datei nicht lesbar"**
→ ROM-Pfad korrekt? ROM muss ≤512KB sein

**"Directory not found"**
→ Test wurde nicht ausgeführt, manuell oder per expect-Script starten

**"Bad CRC" bei Modulen**
→ Write-Fehler! Fix prüfen oder Build erneuern
