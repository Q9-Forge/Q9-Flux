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

static q9_module_map_t *load_module_map(const char *module)
{
    const char *dir = getenv("Q9_SYMBOL_DIR");
    q9_module_map_t *map;
    char path[512], line[512], key[33];
    FILE *f;
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
    if (snprintf(path, sizeof(path), "%s/%s.map", dir, key) >= (int)sizeof(path))
        return NULL;
    f = fopen(path, "r");
    if (!f) {
        g_module_map_count++; /* negative cache: don't hit the filesystem per PC */
        return NULL;
    }
    map->symbols = (q9_symbol_t *)calloc(Q9_SYMBOL_MAX, sizeof(*map->symbols));
    if (!map->symbols) { fclose(f); memset(map, 0, sizeof(*map)); return NULL; }
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
    if (map->count == 0) {
        free(map->symbols);
        memset(map, 0, sizeof(*map));
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
