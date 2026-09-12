/*
 * File:   q9fat.c
 * Desc.:  Kleiner portabler FAT12/16-Image-Generator fuer Q9-PCF-Images.
 *         Optionaler MBR bei LBA 0, FAT-Partition ab --start-sector.
 *
 * Call:   q9fat <image> [--mb N] [--spc N] [--label NAME]
 *                    [--start-sector N] [--mbr]
 *
 * Der Kern ist bewusst ohne Q9-/Host-Abhaengigkeiten gehalten, damit er spaeter
 * in einen Emulator- oder Image-Service-Pfad uebernommen werden kann.
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if !defined(_WIN32)
#include <sys/types.h>
#endif

#define SECTOR 512u
#define RESERVED 1u
#define FATS 2u
#define ROOT_ENTRIES 512u
#define MBR_PARTITION 446u

static void put16le(uint8_t *p, uint16_t v)
{
    p[0] = (uint8_t)v; p[1] = (uint8_t)(v >> 8);
}

static void put32le(uint8_t *p, uint32_t v)
{
    p[0] = (uint8_t)v; p[1] = (uint8_t)(v >> 8);
    p[2] = (uint8_t)(v >> 16); p[3] = (uint8_t)(v >> 24);
}

static int parse_u32(const char *s, uint32_t *out)
{
    char *end;
    unsigned long v = strtoul(s, &end, 0);
    if (end == s || *end != '\0' || v > 0xFFFFFFFFul) return 0;
    *out = (uint32_t)v;
    return 1;
}

static void fat_set(uint8_t *fat, int fat16, uint32_t idx, uint32_t value)
{
    if (fat16) {
        put16le(fat + idx * 2u, (uint16_t)value);
    } else {
        uint32_t off = idx + idx / 2u;
        uint16_t cur = (uint16_t)(fat[off] | ((uint16_t)fat[off + 1u] << 8));
        if (idx & 1u) cur = (uint16_t)((cur & 0x000Fu) | ((value & 0x0FFFu) << 4));
        else          cur = (uint16_t)((cur & 0xF000u) | (value & 0x0FFFu));
        put16le(fat + off, cur);
    }
}

static void name83(uint8_t *dst, const char *name)
{
    const char *dot = strchr(name, '.');
    size_t base_len = dot ? (size_t)(dot - name) : strlen(name);
    size_t ext_len = dot ? strlen(dot + 1) : 0;
    size_t i;
    memset(dst, ' ', 11);
    if (base_len > 8) base_len = 8;
    if (ext_len > 3) ext_len = 3;
    for (i = 0; i < base_len; i++) {
        char c = name[i];
        dst[i] = (uint8_t)(c >= 'a' && c <= 'z' ? c - 'a' + 'A' : c);
    }
    for (i = 0; i < ext_len; i++) {
        char c = dot[1 + i];
        dst[8 + i] = (uint8_t)(c >= 'a' && c <= 'z' ? c - 'a' + 'A' : c);
    }
}

static void dirent(uint8_t *e, const char *name, uint8_t attr, uint16_t cluster, uint32_t size)
{
    memset(e, 0, 32);
    name83(e, name);
    e[0x0B] = attr;
    put16le(e + 0x16, (uint16_t)(12u << 11));
    put16le(e + 0x18, (uint16_t)(((2026u - 1980u) << 9) | (7u << 5) | 22u));
    put16le(e + 0x1A, cluster);
    put32le(e + 0x1C, size);
}

static void boot_sector(uint8_t *b, uint32_t total, uint32_t spc,
                        uint32_t fatsz, int fat16, uint32_t hidden,
                        const char *label)
{
    memset(b, 0, SECTOR);
    b[0] = 0xEB; b[1] = 0x3C; b[2] = 0x90;
    memcpy(b + 3, "Q9CFAT  ", 8);
    put16le(b + 0x0B, SECTOR);
    b[0x0D] = (uint8_t)spc;
    put16le(b + 0x0E, RESERVED);
    b[0x10] = FATS;
    put16le(b + 0x11, ROOT_ENTRIES);
    if (total < 0x10000u) put16le(b + 0x13, (uint16_t)total);
    put32le(b + 0x20, total >= 0x10000u ? total : 0u);
    b[0x15] = 0xF8;
    put16le(b + 0x16, (uint16_t)fatsz);
    put16le(b + 0x18, 63);
    put16le(b + 0x1A, 255);
    put32le(b + 0x1C, hidden);
    b[0x24] = 0x80;
    b[0x26] = 0x29;
    put32le(b + 0x27, 0x51394642u);
    memset(b + 0x2B, ' ', 11);
    if (label) {
        size_t n = strlen(label); if (n > 11) n = 11;
        memcpy(b + 0x2B, label, n);
    }
    memcpy(b + 0x36, fat16 ? "FAT16   " : "FAT12   ", 8);
    put16le(b + 0x1FE, 0xAA55);
}

static void mbr_sector(uint8_t *mbr, uint32_t start, uint32_t count)
{
    uint8_t *p;
    uint32_t end = start + count - 1u;
    uint32_t start_cyl = start / (255u * 63u);
    uint32_t start_rem = start % (255u * 63u);
    uint32_t end_cyl = end / (255u * 63u);
    uint32_t end_rem = end % (255u * 63u);
    uint32_t start_head = start_rem / 63u;
    uint32_t start_sec = (start_rem % 63u) + 1u;
    uint32_t end_head = end_rem / 63u;
    uint32_t end_sec = (end_rem % 63u) + 1u;

    if (start_cyl > 1023u) start_cyl = 1023u;
    if (end_cyl > 1023u) end_cyl = 1023u;
    memset(mbr, 0, SECTOR);
    p = mbr + MBR_PARTITION;
    p[0] = 0x80;                         /* active */
    p[1] = (uint8_t)start_head;
    p[2] = (uint8_t)(start_sec | ((start_cyl >> 2) & 0xC0u));
    p[3] = (uint8_t)start_cyl;
    p[4] = 0x06;                         /* FAT16, old PC-DOS CHS type */
    p[5] = (uint8_t)end_head;
    p[6] = (uint8_t)(end_sec | ((end_cyl >> 2) & 0xC0u));
    p[7] = (uint8_t)end_cyl;
    put32le(p + 8, start);
    put32le(p + 12, count);
    put16le(mbr + 0x1FE, 0xAA55);
}

static int seek64(FILE *f, uint64_t off)
{
#if defined(_WIN32)
    return _fseeki64(f, (__int64)off, SEEK_SET);
#else
    return fseeko(f, (off_t)off, SEEK_SET);
#endif
}

static int write_zeros(FILE *f, uint64_t bytes)
{
    uint8_t zero[SECTOR] = {0};
    while (bytes >= SECTOR) {
        if (fwrite(zero, 1, SECTOR, f) != SECTOR) return 0;
        bytes -= SECTOR;
    }
    return bytes == 0 || fwrite(zero, 1, (size_t)bytes, f) == bytes;
}

static void usage(const char *argv0)
{
    fprintf(stderr, "Usage: %s <image> [--mb N] [--spc N] [--label NAME] [--start-sector N] [--mbr]\n", argv0);
}

int main(int argc, char **argv)
{
    const char *out = 0, *label = "Q9DATA";
    uint32_t mb = 16, spc = 4, start = 0, total, root_secs, fat_secs = 1;
    uint32_t data_secs, clusters, need_bytes, need_fat;
    uint8_t *fat, *root, *cluster, sector[SECTOR], mbr[SECTOR];
    int fat16, with_mbr = 0, i;
    FILE *f;

    if (argc < 2) { usage(argv[0]); return 2; }
    out = argv[1];
    for (i = 2; i < argc; i++) {
        uint32_t v;
        if (!strcmp(argv[i], "--mbr")) { with_mbr = 1; continue; }
        if (i + 1 >= argc) { usage(argv[0]); return 2; }
        if (!strcmp(argv[i], "--mb")) { if (!parse_u32(argv[++i], &mb)) return 2; }
        else if (!strcmp(argv[i], "--spc")) { if (!parse_u32(argv[++i], &spc)) return 2; }
        else if (!strcmp(argv[i], "--start-sector")) { if (!parse_u32(argv[++i], &start)) return 2; }
        else if (!strcmp(argv[i], "--label")) { label = argv[++i]; }
        else { usage(argv[0]); return 2; }
        (void)v;
    }
    if (!mb || !spc || spc > 128 || (spc & (spc - 1u))) {
        fprintf(stderr, "q9fat: --mb > 0 und --spc als Zweierpotenz bis 128 erwartet\n"); return 2;
    }
    if (with_mbr && !start) start = 64;
    total = (mb * 1024u * 1024u) / SECTOR;
    root_secs = ROOT_ENTRIES * 32u / SECTOR;
    for (i = 0; i < 64; i++) {
        data_secs = total - RESERVED - 2u * fat_secs - root_secs;
        clusters = data_secs / spc;
        fat16 = clusters >= 4085u;
        need_bytes = ((clusters + 2u) * (fat16 ? 16u : 12u) + 7u) / 8u + 1u;
        need_fat = (need_bytes + SECTOR - 1u) / SECTOR;
        if (need_fat <= fat_secs) break;
        fat_secs = need_fat;
    }
    data_secs = total - RESERVED - 2u * fat_secs - root_secs;
    clusters = data_secs / spc;
    fat16 = clusters >= 4085u;
    if (fat16 && clusters > 65524u) {
        fprintf(stderr, "q9fat: FAT16-Grenze ueberschritten; --spc erhoehen\n"); return 1;
    }
    if (start > 0xFFFFFFFFu - total) { fprintf(stderr, "q9fat: Image zu gross\n"); return 1; }

    fat = (uint8_t *)calloc(fat_secs, SECTOR);
    root = (uint8_t *)calloc(root_secs, SECTOR);
    cluster = (uint8_t *)calloc(spc, SECTOR);
    if (!fat || !root || !cluster) { fprintf(stderr, "q9fat: Speicherfehler\n"); return 1; }
    fat_set(fat, fat16, 0, fat16 ? 0xFFF8u : 0xFF8u);
    fat_set(fat, fat16, 1, fat16 ? 0xFFFFu : 0xFFFu);
    fat_set(fat, fat16, 2, fat16 ? 0xFFFFu : 0xFFFu);
    fat_set(fat, fat16, 3, fat16 ? 0xFFFFu : 0xFFFu);
    fat_set(fat, fat16, 4, fat16 ? 0xFFFFu : 0xFFFu);
    dirent(root, "README.TXT", 0x20, 2, 44);
    dirent(root + 32, "Q9DIR", 0x10, 3, 0);
    dirent(cluster, ".", 0x10, 3, 0);
    dirent(cluster + 32, "..", 0x10, 0, 0);
    dirent(cluster + 64, "INFO.TXT", 0x20, 4, 34);
    memcpy(cluster + spc * SECTOR - 44, "Q9 FAT image generated by q9fat (C).\r\n", 40);

    f = fopen(out, "wb");
    if (!f) { perror(out); return 1; }
    if (with_mbr) {
        mbr_sector(mbr, start, total);
        if (fwrite(mbr, 1, SECTOR, f) != SECTOR) return 1;
        if (start > 1 && !write_zeros(f, (uint64_t)(start - 1u) * SECTOR)) return 1;
    } else if (start && !write_zeros(f, (uint64_t)start * SECTOR)) return 1;
    boot_sector(sector, total, spc, fat_secs, fat16, start, label);
    if (fwrite(sector, 1, SECTOR, f) != SECTOR) return 1;
    for (i = 0; i < 2; i++) if (fwrite(fat, 1, fat_secs * SECTOR, f) != fat_secs * SECTOR) return 1;
    if (fwrite(root, 1, root_secs * SECTOR, f) != root_secs * SECTOR) return 1;
    if (fwrite(cluster, 1, spc * SECTOR, f) != spc * SECTOR) return 1;
    if (seek64(f, (uint64_t)(start + total) * SECTOR - 1u) != 0 || fputc(0, f) == EOF) return 1;
    fclose(f);
    free(fat); free(root); free(cluster);
    printf("%s: FAT%d, %u MB, start=%u, %s\n", out, fat16 ? 16 : 12, mb,
           start, with_mbr ? "MBR" : "Superfloppy");
    return 0;
}
