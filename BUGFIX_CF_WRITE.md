# Bugfix: CF Write für 256-Byte RBF-Images

## Problem

Bei 256-Byte RBF-Images (OS-9 Standard-Format) kam es zu Datenkorruption beim Schreiben:
- **Read** funktionierte korrekt: `cb030_cf_load_write_buffer()` liest ZWEI 256-Byte-Sektoren in den 512-Byte-Buffer
- **Write** war defekt: `cb030_cf_store_sector()` schrieb nur EINEN 256-Byte-Sektor zurück
- → Bytes 256-511 gingen verloren, führte zu inkonsistentem Dateisystem

## Root Cause

Die Implementierung hatte eine Read/Write-Asymmetrie:
```c
// READ: Liest 2 Sektoren à 256 Bytes
static void cb030_cf_load_write_buffer(q9_cb030_t *b) {
    if (img_sec < Q9_CB030_CF_SECTOR_SIZE) {
        // Sektor 1: Bytes 0-255
        fread(b->cf_sector, 1, img_sec, b->cf_file);
        // Sektor 2: Bytes 256-511
        fread(b->cf_sector + img_sec, 1, Q9_CB030_CF_SECTOR_SIZE - img_sec, b->cf_file);
    }
}

// WRITE: Schrieb nur 1 Sektor (0-255)!
static void cb030_cf_store_sector(q9_cb030_t *b) {
    fwrite(b->cf_sector, 1, img_sec, b->cf_file);  // FEHLER: nur erster Teil
}
```

## Fix

`cb030_cf_store_sector()` schreibt jetzt bei 256-Byte-Images ebenfalls ZWEI Sektoren:

```c
static void cb030_cf_store_sector(q9_cb030_t *b) {
    if (cb030_cf_ensure_open(b)) {
        uint32_t img_sec = b->cf_image_sector_size ? b->cf_image_sector_size : Q9_CB030_CF_SECTOR_SIZE;
        
        /* For 256-byte images: write TWO sectors (matching the read behavior) */
        if (img_sec < Q9_CB030_CF_SECTOR_SIZE) {
            fwrite(b->cf_sector, 1, img_sec, b->cf_file);
            fseek(b->cf_file, (long)(b->cf_lba + 1u) * (long)img_sec, SEEK_SET);
            fwrite(b->cf_sector + img_sec, 1, Q9_CB030_CF_SECTOR_SIZE - img_sec, b->cf_file);
        } else {
            fwrite(b->cf_sector, 1, written < img_sec ? written : img_sec, b->cf_file);
        }
        fflush(b->cf_file);
    }
}
```

## Testing

Test-Image erstellen:
```bash
./test/cf/test_cf_write.sh
```

Mit CF-Tracing testen:
```bash
export Q9_CB030_CF_TRACE=1
./build/native/q9.exe --cb030 local_images/HD5-skip2-work.hda --cf local_images/test-cf-write-256.hda
```

## Affected Files

- `src/kernel/cb030.c`: Fix in `cb030_cf_store_sector()`
- `src/kernel/cb030.h`: Feld `cf_image_sector_size` dokumentiert

## Related

- Image-Creation: `tools/build_q9_work_image.py` erstellt 256-Byte RBF-Images
- Format-Spezifikation: OS-9 RBF (Random Block File Manager) mit LSN=256
