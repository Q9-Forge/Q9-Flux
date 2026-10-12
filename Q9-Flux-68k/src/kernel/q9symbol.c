#include "q9symbol.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define Q9_SYMBOL_MAX 4096u
#define Q9_MODULE_MAP_CACHE 16u

typedef struct {
    uint32_t offset;
    char name[64];
} q9_symbol_t;

static q9_symbol_t g_symbols[Q9_SYMBOL_MAX];
static size_t g_symbol_count;
static uint32_t g_symbol_base = 0x7100u;
static int g_symbol_loaded;

typedef struct {
    char module[33];
    q9_symbol_t *symbols;
    size_t count;
} q9_module_map_t;

static q9_module_map_t g_module_maps[Q9_MODULE_MAP_CACHE];
static size_t g_module_map_count;

static void sort_symbols(q9_symbol_t *symbols, size_t count)
{
    size_t i;
    for (i = 1; i < count; i++) {
        q9_symbol_t value = symbols[i];
        size_t j = i;
        while (j > 0 && symbols[j - 1u].offset > value.offset) {
            symbols[j] = symbols[j - 1u];
            j--;
        }
        symbols[j] = value;
    }
}

static int safe_module_name(const char *module)
{
    const unsigned char *p = (const unsigned char *)module;
    size_t n = 0;
    if (!p || !*p) return 0;
    while (*p) {
        if (!((*p >= 'a' && *p <= 'z') || (*p >= 'A' && *p <= 'Z') ||
              (*p >= '0' && *p <= '9') || *p == '_' || *p == '-' || *p == '.'))
            return 0;
        p++;
        if (++n > 32u) return 0;
    }
    return 1;
}

static int load_map_file(q9_module_map_t *map, const char *path)
{
    char line[512];
    FILE *f = fopen(path, "r");
    if (!f) return 0;
    while (fgets(line, sizeof(line), f) && map->count < Q9_SYMBOL_MAX) {
        char *part = line;
        char *next;
        do {
            char name[64], kind[8];
            unsigned long offset;
            next = strchr(part, '|');
            if (next) *next = 0;
            if (sscanf(part, " %63s %7s %lx", name, kind, &offset) == 3 &&
                strcmp(kind, "COD") == 0) {
                strncpy(map->symbols[map->count].name, name,
                        sizeof(map->symbols[map->count].name) - 1u);
                map->symbols[map->count].offset = (uint32_t)offset;
                map->count++;
            }
            part = next ? next + 1 : NULL;
        } while (part && map->count < Q9_SYMBOL_MAX);
    }
    fclose(f);
    return map->count != 0;
}

/* Microware l68 -g symbol modules: OS-9 module header, then a versioned
 * symbol block, fixed-size records, and a NUL-terminated name pool.  The
 * record fields are value:u32, kind:u16, name_offset:u32 (module absolute).
 * Code symbols have kind 0x0004, 0x1004, or 0x2004 in the shipped samples;
 * the low nibble is the code class. */
static int load_stb_file(q9_module_map_t *map, const char *path)
{
    unsigned char *bytes = NULL;
    long file_size;
    uint32_t module_size, symtab, records, count, records_end;
    FILE *f = fopen(path, "rb");
    size_t i;
    int ok = 0;
    if (!f) return 0;
    if (fseek(f, 0, SEEK_END) != 0 || (file_size = ftell(f)) < 0 ||
        fseek(f, 0, SEEK_SET) != 0 || file_size < 0x30L ||
        (unsigned long)file_size > 0xffffffffUL) goto done;
    bytes = (unsigned char *)malloc((size_t)file_size);
    if (!bytes || fread(bytes, 1, (size_t)file_size, f) != (size_t)file_size)
        goto done;
    if (bytes[0] != 0x4a || bytes[1] != 0xfc) goto done;
    module_size = ((uint32_t)bytes[4] << 24) | ((uint32_t)bytes[5] << 16) |
                  ((uint32_t)bytes[6] << 8) | bytes[7];
    symtab = ((uint32_t)bytes[0x1c] << 24) | ((uint32_t)bytes[0x1d] << 16) |
             ((uint32_t)bytes[0x1e] << 8) | bytes[0x1f];
    if (module_size > (uint32_t)file_size || module_size < 0x30u ||
        bytes[0x12] != 0x04u || symtab < 0x30u || symtab > module_size ||
        module_size - symtab < 14u || bytes[symtab] != 1u) goto done;
    records = ((uint32_t)bytes[symtab + 8u] << 8) | bytes[symtab + 9u];
    count = ((uint32_t)bytes[symtab + 12u] << 8) | bytes[symtab + 13u];
    if (records < symtab + 14u || records > module_size ||
        count > Q9_SYMBOL_MAX || count > (module_size - records) / 10u)
        goto done;
    records_end = records + count * 10u;
    for (i = 0; i < count; i++) {
        uint32_t off = records + (uint32_t)i * 10u;
        uint32_t value = ((uint32_t)bytes[off] << 24) |
                         ((uint32_t)bytes[off + 1u] << 16) |
                         ((uint32_t)bytes[off + 2u] << 8) | bytes[off + 3u];
        uint16_t kind = (uint16_t)(((uint16_t)bytes[off + 4u] << 8) |
                                   bytes[off + 5u]);
        uint32_t name_off = ((uint32_t)bytes[off + 6u] << 24) |
                            ((uint32_t)bytes[off + 7u] << 16) |
                            ((uint32_t)bytes[off + 8u] << 8) | bytes[off + 9u];
        size_t len;
        size_t name_remaining;
        const unsigned char *name;
        if ((kind & 0x000fu) != 4u) continue;
        if (name_off < records_end || name_off >= module_size) goto done;
        name = bytes + name_off;
        name_remaining = (size_t)(module_size - name_off);
        for (len = 0; len < name_remaining && name[len] != 0; len++) {
            if (name[len] < 0x21u || name[len] > 0x7eu || len >= 63u) break;
        }
        if (len >= name_remaining || name[len] != 0 || len == 0u || len >= 63u)
            goto done;
        memcpy(map->symbols[map->count].name, name, len);
        map->symbols[map->count].name[len] = 0;
        map->symbols[map->count].offset = value;
        map->count++;
    }
    ok = map->count != 0;
done:
    free(bytes);
    fclose(f);
    if (!ok) map->count = 0;
    return ok;
}

static q9_module_map_t *load_module_map(const char *module)
{
    const char *dir = getenv("Q9_SYMBOL_DIR");
    q9_module_map_t *map;
    char path[512], key[33];
    size_t i;

    if (!dir || !*dir || !safe_module_name(module)) return NULL;
    for (i = 0; module[i] && i < sizeof(key) - 1u; i++)
        key[i] = (char)tolower((unsigned char)module[i]);
    key[i] = 0;
    for (i = 0; i < g_module_map_count; i++)
        if (strcmp(g_module_maps[i].module, key) == 0)
            return g_module_maps[i].symbols ? &g_module_maps[i] : NULL;
    if (g_module_map_count >= Q9_MODULE_MAP_CACHE) return NULL;
    map = &g_module_maps[g_module_map_count];
    memset(map, 0, sizeof(*map));
    memcpy(map->module, key, strlen(key) + 1u);
    map->symbols = (q9_symbol_t *)calloc(Q9_SYMBOL_MAX, sizeof(*map->symbols));
    if (!map->symbols) return NULL;
    if (snprintf(path, sizeof(path), "%s/%s.map", dir, key) < (int)sizeof(path))
        load_map_file(map, path);
    if (map->count == 0) {
        if (snprintf(path, sizeof(path), "%s/%s.stb", dir, key) < (int)sizeof(path))
            load_stb_file(map, path);
    }
    if (map->count == 0) {
        free(map->symbols);
        memset(map, 0, sizeof(*map));
        map->symbols = NULL;
        g_module_map_count++; /* negative cache: don't hit the filesystem per PC */
        return NULL;
    }
    sort_symbols(map->symbols, map->count);
    g_module_map_count++;
    return map;
}

void q9_symbol_init_from_env(void)
{
    const char *path = getenv("Q9_SYMBOL_MAP");
    const char *base = getenv("Q9_SYMBOL_BASE");
    FILE *f;
    char line[256];

    if (g_symbol_loaded)
        return;
    g_symbol_loaded = 1;
    if (base && *base)
        g_symbol_base = (uint32_t)strtoul(base, NULL, 0);
    if (!path || !*path)
        return;
    f = fopen(path, "r");
    if (!f)
        return;
    while (fgets(line, sizeof(line), f) && g_symbol_count < Q9_SYMBOL_MAX) {
        char name[64], kind[8];
        unsigned long offset;
        if (sscanf(line, " %63s %7s %lx", name, kind, &offset) != 3 ||
            strcmp(kind, "COD") != 0)
            continue;
        strncpy(g_symbols[g_symbol_count].name, name, sizeof(name) - 1u);
        g_symbols[g_symbol_count].name[sizeof(name) - 1u] = 0;
        g_symbols[g_symbol_count].offset = (uint32_t)offset;
        g_symbol_count++;
    }
    fclose(f);
    sort_symbols(g_symbols, g_symbol_count);
}

int q9_symbolize_pc(uint32_t pc, char *out, size_t out_len)
{
    size_t lo = 0, hi = g_symbol_count;
    const q9_symbol_t *best = NULL;
    uint32_t offset;
    if (!out || out_len == 0) return 0;
    out[0] = 0;
    if (!g_symbol_loaded) q9_symbol_init_from_env();
    if (g_symbol_count == 0 || pc < g_symbol_base) return 0;
    offset = pc - g_symbol_base;
    while (lo < hi) {
        size_t mid = lo + (hi - lo) / 2u;
        if (g_symbols[mid].offset <= offset) {
            best = &g_symbols[mid];
            lo = mid + 1u;
        } else {
            hi = mid;
        }
    }
    if (!best) return 0;
    snprintf(out, out_len, "%s+0x%x", best->name,
             (unsigned)(offset - best->offset));
    return 1;
}

int q9_symbolize_module_pc(const char *module, uint32_t module_base,
                           uint32_t pc, char *out, size_t out_len)
{
    q9_module_map_t *map;
    const q9_symbol_t *best = NULL;
    size_t lo = 0, hi;
    uint32_t offset;
    if (!out || out_len == 0) return 0;
    out[0] = 0;
    if (pc < module_base) return 0;
    map = load_module_map(module);
    if (!map) return 0;
    offset = pc - module_base;
    hi = map->count;
    while (lo < hi) {
        size_t mid = lo + (hi - lo) / 2u;
        if (map->symbols[mid].offset <= offset) {
            best = &map->symbols[mid];
            lo = mid + 1u;
        } else {
            hi = mid;
        }
    }
    if (!best) return 0;
    snprintf(out, out_len, "%s+0x%x", best->name,
             (unsigned)(offset - best->offset));
    return 1;
}
