/*
 * mbr.c - MBR- und FAT-Partitionsinspektor fuer OS-9/68K.
 *
 * Aufruf:
 *     mbr <device-or-image> [<pcf-templ>] [<rbf-templ>]
 *     mbr -?                                    (Hilfe, OS-9-ueblich)
 *
 * Das Programm liest den ersten 512-Byte-Sektor, zeigt die vier MBR-
 * Partitionseintraege und - sofern vorhanden - den FAT-Bootsektor an.
 * Die ermittelten Werte sind zugleich die Eingabedaten fuer die PCF-/RBF-
 * Descriptor-Erzeugung, die als naechster Schritt in diese Datei kommt.
 *
 * GPT (GUID Partition Table) wird erkannt (Signatur "EFI PART" auf LSN 1),
 * aber bewusst NICHT unterstuetzt - es wird nur eine entsprechende Meldung
 * ausgegeben, mit der Empfehlung, ein MBR-partitioniertes Laufwerk/Abbild
 * zu verwenden.
 *
 * Bewusst nur ANSI-C und OS-9-kompatible Standard-I/O-Funktionen: derselbe
 * Quelltext kann zuerst im Emulator mit /dd/CMDS/mbr und spaeter auf echter
 * OS-9-Hardware verwendet werden.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SECTOR_SIZE       512L
#define MBR_PART_OFFSET   446
#define MBR_PART_COUNT    4
#define MBR_PART_SIZE     16
#define MODULE_MAX        256
#define CRC_SIZE          3

static unsigned char module[MODULE_MAX];

static unsigned long u16le(const unsigned char *p)
{
    return (unsigned long)p[0] | ((unsigned long)p[1] << 8);
}

static unsigned long u32le(const unsigned char *p)
{
    return (unsigned long)p[0]
         | ((unsigned long)p[1] << 8)
         | ((unsigned long)p[2] << 16)
         | ((unsigned long)p[3] << 24);
}

static unsigned long u24be(const unsigned char *p)
{
    return ((unsigned long)p[0] << 16)
         | ((unsigned long)p[1] << 8)
         | (unsigned long)p[2];
}

static unsigned long u16be(const unsigned char *p)
{
    return ((unsigned long)p[0] << 8) | (unsigned long)p[1];
}

static unsigned long u32be(const unsigned char *p)
{
    return ((unsigned long)p[0] << 24)
         | ((unsigned long)p[1] << 16)
         | ((unsigned long)p[2] << 8)
         | (unsigned long)p[3];
}

static void put16be(unsigned char *p, unsigned long v)
{
    p[0] = (unsigned char)(v >> 8);
    p[1] = (unsigned char)v;
}

static void put32be(unsigned char *p, unsigned long v)
{
    p[0] = (unsigned char)(v >> 24);
    p[1] = (unsigned char)(v >> 16);
    p[2] = (unsigned char)(v >> 8);
    p[3] = (unsigned char)v;
}

static int module_string(unsigned long offset, char *out, unsigned max)
{
    unsigned i = 0;

    if (offset >= MODULE_MAX || max == 0) {
        return 0;
    }
    while (offset < MODULE_MAX && i + 1 < max && module[offset] != 0) {
        out[i++] = (char)(module[offset++] & 0x7F);
    }
    out[i] = '\0';
    return offset < MODULE_MAX && module[offset] == 0;
}

static void os9_crc(const unsigned char *data, unsigned long length,
                    unsigned char *out)
{
    unsigned char crc[3] = { 0xFF, 0xFF, 0xFF };
    unsigned long i;

    for (i = 0; i < length; i++) {
        unsigned int a = (unsigned int)(data[i] ^ crc[0]);
        crc[0] = crc[1];
        crc[1] = crc[2];
        crc[1] ^= (unsigned char)(a >> 7);
        crc[2] = (unsigned char)(a << 1);
        crc[1] ^= (unsigned char)(a >> 2);
        crc[2] ^= (unsigned char)(a << 6);
        /* Keep the intermediate value wider here.  OS-9's CRC algorithm
           truncates only after the three XOR/shift steps, not after each
           individual assignment. */
        a ^= a << 1;
        a ^= a << 2;
        a ^= a << 4;
        a &= 0xFFU;
        if (a & 0x80) {
            crc[0] ^= 0x80;
            crc[2] ^= 0x21;
        }
    }
    out[0] = (unsigned char)(crc[0] ^ 0xFF);
    out[1] = (unsigned char)(crc[1] ^ 0xFF);
    out[2] = (unsigned char)(crc[2] ^ 0xFF);
}

static void repair_module(void)
{
    unsigned long size = u32be(module + 4);
    unsigned long i;
    unsigned long parity = 0;
    unsigned char crc[CRC_SIZE];

    /* M$Parity itself is at $2E and must not participate in its calculation. */
    for (i = 0; i < 0x2E; i += 2) {
        parity ^= ((unsigned long)module[i] << 8) | module[i + 1];
    }
    put16be(module + 0x2E, 0xFFFFUL ^ parity);
    os9_crc(module, size - CRC_SIZE, crc);
    module[size - 3] = crc[0];
    module[size - 2] = crc[1];
    module[size - 1] = crc[2];
}

static int load_module(FILE *f, unsigned long *size)
{
    long n;

    if (fseek(f, 0L, SEEK_END) != 0) return 0;
    n = ftell(f);
    if (n < 51 || n > MODULE_MAX || fseek(f, 0L, SEEK_SET) != 0) return 0;
    if (fread(module, 1, (unsigned)n, f) != (unsigned)n) return 0;
    if (module[0] != 0x4A || module[1] != 0xFC || u32be(module + 4) != (unsigned long)n) return 0;
    *size = (unsigned long)n;
    return 1;
}

static int make_descriptor(const char *template_path, const char *output_path,
                           const char *name, int want_pcf, unsigned long lsn,
                           unsigned long sectors, unsigned long sector_size)
{
    FILE *in, *out;
    unsigned long size = 0, name_off, fm_off, cylinders = 0;
    unsigned long control;
    char fm[16];

    in = fopen(template_path, "rb");
    if (!in || !load_module(in, &size)) {
        if (in) fclose(in);
        printf("  Descriptor: Vorlage '%s' nicht lesbar oder kein OS-9-Modul\n", template_path);
        return 0;
    }
    fclose(in);

    fm_off = u16be(module + 0x38);
    if (!module_string(fm_off, fm, sizeof(fm)) ||
        (want_pcf && strcmp(fm, "pcf") != 0) ||
        (!want_pcf && strcmp(fm, "rbf") != 0)) {
        printf("  Descriptor: Vorlage '%s' hat falschen File Manager (%s)\n",
               template_path, fm);
        return 0;
    }

    /* OS-9's module name offset is the 16-bit word at $0E.  The long at
       $0C spans the owner field and the name offset, so reading it as a
       32-bit value would point at the module start and corrupt the header
       before the CRC is repaired. */
    name_off = u16be(module + 0x0E);
    if (name_off >= size || strlen(name) > 31) return 0;
    /* Name field in the supplied descriptor templates is long enough for d0..d3. */
    strcpy((char *)(module + name_off), name);

    /* PCF can derive the real medium geometry from the FAT BPB at the
       partition offset.  Do not derive fixed CHS values from the partition
       length: the backing disk can contain trailing sectors outside the
       partition (the Q9 2-GB image does), which otherwise yields E$BTYP.
       RBF descriptors still use the classic fixed geometry. */
    if (want_pcf) {
        module[0x4B] = 0x80;             /* hard disk */
        module[0x4C] = 0x00;             /* single density */
        put16be(module + 0x4E, 0);       /* auto-size */
        module[0x50] = 0;
        put16be(module + 0x52, 0);
        put16be(module + 0x54, 0);
    } else {
        cylinders = (sectors + 255UL * 63UL - 1UL) / (255UL * 63UL);
        if (cylinders == 0) cylinders = 1;
        if (cylinders > 65535UL) return 0;
        put16be(module + 0x4E, cylinders);
        module[0x50] = 255;
        put16be(module + 0x52, 63);
        put16be(module + 0x54, 63);
    }
    if (sector_size > 65535UL) return 0;
    put16be(module + 0x5C, sector_size);
    put16be(module + 0x6C, want_pcf ? 0 : cylinders);
    put32be(module + 0x68, lsn);
    /* $78 is the File Manager name in the descriptor template ("pcf" or
       "rbf"), not a total-byte field.  Do not overwrite it: the generated
       descriptor would load but fail later with E$BTYP (249). */

    /* Format disabled + multi-sector I/O; preserve other template bits. */
    control = u16be(module + 0x5E);
    control |= 0x0003UL;
    if (want_pcf) control |= 0x0008UL;   /* AutoEnabl */
    put16be(module + 0x5E, control);
    repair_module();

    out = fopen(output_path, "wb");
    if (!out || fwrite(module, 1, (unsigned)size, out) != (unsigned)size) {
        if (out) fclose(out);
        return 0;
    }
    fclose(out);
    printf("  Descriptor erzeugt: %s (%s, LSNOffs=%lu, %lu Bytes)\n",
           output_path, want_pcf ? "PCF" : "RBF", lsn, sectors * sector_size);
    return 1;
}

static int known_partition_type(unsigned char type)
{
    switch (type) {
    case 0x01: case 0x04: case 0x05: case 0x06:
    case 0x0B: case 0x0C: case 0x0E: case 0x0F:
    case 0x11: case 0x14: case 0x16:
    case 0x1B: case 0x1C: case 0x1E:
    case 0x82: case 0x83: case 0x85: case 0x8E:
    case 0xA5: case 0xEE: case 0xEF:
        return 1;
    default:
        return 0;
    }
}

static int looks_like_mbr(const unsigned char *sector)
{
    int i;

    if (sector[510] != 0x55 || sector[511] != 0xAA) {
        return 0;
    }
    for (i = 0; i < MBR_PART_COUNT; i++) {
        const unsigned char *p = sector + MBR_PART_OFFSET + i * MBR_PART_SIZE;
        unsigned long start = u32le(p + 8);
        unsigned long count = u32le(p + 12);
        if (known_partition_type(p[4]) && start != 0 && count != 0) {
            return 1;
        }
    }
    return 0;
}

static int looks_like_fat(const unsigned char *b)
{
    unsigned long bytes = u16le(b + 11);
    unsigned long spc = b[13];
    unsigned long total = u16le(b + 19);
    unsigned long fatsz = u16le(b + 22);
    unsigned long clusters;
    unsigned long root = u16le(b + 17);
    unsigned long root_sectors;

    if (b[510] != 0x55 || b[511] != 0xAA || bytes != 512 ||
        spc == 0 || spc > 128 || (spc & (spc - 1UL)) != 0 ||
        b[16] == 0 || root == 0 || fatsz == 0) {
        return 0;
    }
    if (total == 0) {
        total = u32le(b + 32);
    }
    root_sectors = (root * 32UL + bytes - 1UL) / bytes;
    if (total <= 1UL + (unsigned long)b[16] * fatsz + root_sectors) {
        return 0;
    }
    clusters = (total - 1UL - (unsigned long)b[16] * fatsz - root_sectors) / spc;
    return clusters >= 4085UL && clusters < 65525UL;
}

static int looks_like_rbf(const unsigned char *b)
{
    unsigned long total = u24be(b + 0);
    unsigned long track_sectors = b[3];
    unsigned long map_bytes = u16be(b + 4);
    unsigned long sectors_bit = u16be(b + 6);
    unsigned long root_fd = u24be(b + 8);
    unsigned long spt = u16be(b + 17);
    unsigned long lsn_size = u16be(b + 104);

    if (total == 0 || track_sectors == 0 || map_bytes == 0 ||
        sectors_bit == 0 || root_fd == 0 || root_fd >= total ||
        spt == 0 || spt > 1024 || (lsn_size != 256 && lsn_size != 512)) {
        return 0;
    }
    return 1;
}

static void print_rbf_info(const unsigned char *b)
{
    unsigned long total = u24be(b + 0);
    unsigned long track_sectors = b[3];
    unsigned long map_bytes = u16be(b + 4);
    unsigned long sectors_bit = u16be(b + 6);
    unsigned long root_fd = u24be(b + 8);
    unsigned long spt = u16be(b + 17);
    unsigned long lsn_size = u16be(b + 104);
    char name[33];
    int i;

    for (i = 0; i < 32; i++) {
        unsigned char c = b[31 + i];
        name[i] = (c >= 0x20 && c < 0x7F) ? (char)c : '.';
    }
    name[32] = '\0';

    printf("RBF direkt ab LSN 0\n");
    printf("  DD_TOT=%lu, DD_TKS=%lu, DD_MAP=%lu, DD_BIT=%lu\n",
           total, track_sectors, map_bytes, sectors_bit);
    printf("  DD_DIR=%lu, DD_SPT=%lu, DD_LSNSize=%lu, Name='%s'\n",
           root_fd, spt, lsn_size, name);
    printf("  Descriptor: RBF, LSNOffs=0, SSize=%lu, Sektoren=%lu\n",
           lsn_size, total);
}

static int read_sector(FILE *f, unsigned long lsn, unsigned char *buf)
{
    long offset = (long)(lsn * (unsigned long)SECTOR_SIZE);

    if (fseek(f, offset, SEEK_SET) != 0) {
        return 0;
    }
    return fread(buf, 1, (unsigned)SECTOR_SIZE, f) == (unsigned)SECTOR_SIZE;
}

/* GPT (GUID Partition Table) erkennen: massgeblich ist die Signatur
   "EFI PART" am Anfang von LSN 1 (dem GPT-Header) - unabhaengig davon,
   ob LSN 0 zufaellig als klassisches MBR durchgeht (der "Protective MBR"
   einer GPT-Platte hat oft einen Eintrag vom Typ $EE, der in
   known_partition_type() als bekannt gilt). Dieser Check muss daher VOR
   looks_like_mbr() laufen, sonst wuerde die MBR-Partitionsschleife den
   GPT-Header versehentlich als Bootsektor einer Partition $EE lesen. */
static int looks_like_gpt(FILE *f)
{
    unsigned char sector[SECTOR_SIZE];

    if (!read_sector(f, 1, sector)) {
        return 0;
    }
    return memcmp(sector, "EFI PART", 8) == 0;
}

static void print_fat_info(const unsigned char *b, unsigned long lsn,
                           unsigned long part_sectors)
{
    unsigned long bytes, spc, reserved, fats, root_entries;
    unsigned long total, fat_sectors, hidden;
    unsigned long root_sectors, data_sectors, clusters;
    const char *kind = "unbekannt";

    if (b[510] != 0x55 || b[511] != 0xAA) {
        printf("      Bootsektor: keine Signatur 55 AA\n");
        return;
    }

    bytes        = u16le(b + 11);
    spc          = b[13];
    reserved     = u16le(b + 14);
    fats         = b[16];
    root_entries = u16le(b + 17);
    total        = u16le(b + 19);
    if (total == 0) {
        total = u32le(b + 32);
    }
    fat_sectors = u16le(b + 22);
    hidden      = u32le(b + 28);

    if (bytes == 0 || spc == 0 || fats == 0) {
        printf("      Bootsektor: unvollstaendige BPB\n");
        return;
    }

    root_sectors = ((root_entries * 32UL) + bytes - 1UL) / bytes;
    if (total > reserved + fats * fat_sectors + root_sectors) {
        data_sectors = total - reserved - fats * fat_sectors - root_sectors;
        clusters = data_sectors / spc;
        if (clusters < 4085UL) {
            kind = "FAT12";
        } else if (clusters < 65525UL) {
            kind = "FAT16";
        } else {
            kind = "FAT32/zu gross";
        }
    } else {
        data_sectors = 0;
        clusters = 0;
    }

    printf("      Bootsektor LSN: %lu, Typ: %s\n", lsn, kind);
    printf("      Bytes/Sektor: %lu, Sektoren/Cluster: %lu\n", bytes, spc);
    printf("      Gesamtsektoren BPB: %lu, Partition: %lu\n", total, part_sectors);
    printf("      Hidden/Start: %lu, Reserved: %lu, FATs: %lu, FAT-Sektoren: %lu\n",
           hidden, reserved, fats, fat_sectors);
    printf("      Datencluster: %lu\n", clusters);
    printf("      Descriptor: LSNOffs=%lu, SSize=%lu\n", lsn, bytes);
}

static void print_partition(FILE *f, int number, const unsigned char *p)
{
    unsigned long start = u32le(p + 8);
    unsigned long count = u32le(p + 12);
    unsigned long end;
    unsigned char boot = p[0];
    unsigned char type = p[4];
    unsigned char boot_sector[SECTOR_SIZE];

    if (type == 0 || count == 0) {
        printf("  Partition %d: leer\n", number);
        return;
    }

    end = start + count - 1UL;
    printf("  Partition %d: Typ=$%02X, %s, Start=%lu, Ende=%lu, Sektoren=%lu\n",
           number, type, boot == 0x80 ? "aktiv" : "inaktiv", start, end, count);

    if (read_sector(f, start, boot_sector)) {
        print_fat_info(boot_sector, start, count);
    } else {
        printf("      Bootsektor nicht lesbar\n");
    }
}

static void print_help(void)
{
    printf("Syntax:   mbr <device-or-image> [<pcf-templ>] [<rbf-templ>]\n");
    printf("Function: MBR-/FAT-/RBF-Partitionen erkennen und daraus PCF-/RBF-\n");
    printf("          Geraetedescriptoren erzeugen\n");
    printf("Options:\n");
    printf("     <device-or-image>  Geraet oder Abbilddatei, die untersucht wird\n");
    printf("     <pcf-templ>        PCF-Descriptor-Vorlage fuer erkannte FAT-Partitionen\n");
    printf("     <rbf-templ>        RBF-Descriptor-Vorlage fuer erkannte RBF-Partitionen\n");
    printf("                        (ohne Angabe wird <pcf-templ> auch dafuer verwendet)\n");
    printf("     -?                 diese Hilfe anzeigen\n");
}

int main(int argc, char **argv)
{
    FILE *f;
    unsigned char mbr[SECTOR_SIZE];
    const char *pcf_template = 0;
    const char *rbf_template = 0;
    char name[8];
    int i;

    if (argc == 2 && strcmp(argv[1], "-?") == 0) {
        print_help();
        return 0;
    }

    if (argc < 2 || argc > 4) {
        fprintf(stderr, "Aufruf: mbr <device-or-image> [pcf-template] [rbf-template]\n");
        fprintf(stderr, "        mbr -?   (Hilfe)\n");
        return 2;
    }
    if (argc >= 3) pcf_template = argv[2];
    if (argc >= 4) rbf_template = argv[3];
    if (!rbf_template) rbf_template = pcf_template;

    f = fopen(argv[1], "rb");
    if (!f) {
        fprintf(stderr, "mbr: kann '%s' nicht oeffnen\n", argv[1]);
        return 1;
    }

    if (!read_sector(f, 0, mbr)) {
        fprintf(stderr, "mbr: LSN 0 nicht lesbar\n");
        fclose(f);
        return 1;
    }

    if (looks_like_gpt(f)) {
        printf("GPT (GUID Partition Table) erkannt.\n");
        printf("GPT wird von diesem Werkzeug derzeit nicht unterstuetzt.\n");
        printf("Bitte ein Laufwerk/Abbild mit klassischer MBR-Partitionierung verwenden.\n");
        fclose(f);
        return 4;
    }

    if (!looks_like_mbr(mbr)) {
        if (looks_like_fat(mbr)) {
            printf("FAT direkt ab LSN 0 (kein MBR)\n");
            print_fat_info(mbr, 0, 0);
            if (pcf_template) {
                make_descriptor(pcf_template, "d0", "d0", 1,
                                0, u32le(mbr + 32), u16le(mbr + 11));
            }
            fclose(f);
            return 0;
        }
        if (looks_like_rbf(mbr)) {
            print_rbf_info(mbr);
            if (rbf_template) {
                make_descriptor(rbf_template, "d0", "d0", 0,
                                0, u24be(mbr), u16be(mbr + 104));
            }
            fclose(f);
            return 0;
        }
        printf("Kein plausibles MBR-, FAT- oder RBF-Format erkannt.\n");
        fclose(f);
        return 3;
    }

    printf("MBR: %s\n", argv[1]);
    printf("  Signatur: 55 AA (gueltig), Partitionstabelle plausibel\n");
    for (i = 0; i < MBR_PART_COUNT; i++) {
        print_partition(f, i, mbr + MBR_PART_OFFSET + i * MBR_PART_SIZE);
        {
            const unsigned char *p = mbr + MBR_PART_OFFSET + i * MBR_PART_SIZE;
            unsigned long start = u32le(p + 8);
            unsigned long count = u32le(p + 12);
            unsigned char boot[SECTOR_SIZE];
            if (p[4] != 0 && count != 0 && read_sector(f, start, boot)) {
                sprintf(name, "d%d", i);
                if (looks_like_fat(boot)) {
                    if (pcf_template) {
                        make_descriptor(pcf_template, name, name, 1,
                                        start, count, u16le(boot + 11));
                    }
                } else if (looks_like_rbf(boot)) {
                    if (rbf_template) {
                        make_descriptor(rbf_template, name, name, 0,
                                        start, count, u16be(boot + 104));
                    }
                }
            }
        }
    }

    fclose(f);
    return 0;
}
