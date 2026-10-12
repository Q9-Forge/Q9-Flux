#include "q9line.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define Q9_LINE_MAX 8192u
#define Q9_MODULE_LINE_CACHE 16u

typedef struct {
    uint32_t address;
    unsigned line;
    unsigned column;
    char source[128];
} q9_line_t;

static q9_line_t g_lines[Q9_LINE_MAX];
static size_t g_line_count;
static int g_line_loaded;

typedef struct {
    char module[33];
    q9_line_t *lines;
    size_t count;
    size_t capacity;
} q9_module_line_map_t;

static q9_module_line_map_t g_module_lines[Q9_MODULE_LINE_CACHE];
static size_t g_module_line_count;

static int line_safe_module_name(const char *module)
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

static void sort_lines(q9_line_t *lines, size_t count)
{
    size_t i;
    for (i = 1; i < count; i++) {
        q9_line_t value = lines[i];
        size_t j = i;
        while (j > 0 && lines[j - 1u].address > value.address) {
            lines[j] = lines[j - 1u];
            j--;
        }
        lines[j] = value;
    }
}

static q9_module_line_map_t *load_module_lines(const char *module)
{
    const char *dir = getenv("Q9_LINE_DIR");
    q9_module_line_map_t *map;
    char key[33], path[512], text[256];
    FILE *f;
    size_t i;
    if (!dir || !*dir || !line_safe_module_name(module)) return NULL;
    for (i = 0; module[i] && i < sizeof(key) - 1u; i++)
        key[i] = (char)tolower((unsigned char)module[i]);
    key[i] = 0;
    for (i = 0; i < g_module_line_count; i++)
        if (strcmp(g_module_lines[i].module, key) == 0)
            return g_module_lines[i].lines ? &g_module_lines[i] : NULL;
    if (g_module_line_count >= Q9_MODULE_LINE_CACHE) return NULL;
    map = &g_module_lines[g_module_line_count];
    memset(map, 0, sizeof(*map));
    memcpy(map->module, key, strlen(key) + 1u);
    if (snprintf(path, sizeof(path), "%s/%s.lines", dir, key) >= (int)sizeof(path))
        return NULL;
    f = fopen(path, "r");
    if (!f) { g_module_line_count++; return NULL; }
    while (fgets(text, sizeof(text), f)) {
        q9_line_t value;
        unsigned long address;
        if (sscanf(text, " 0x%lx %127[^:]:%u:%u", &address, value.source,
                   &value.line, &value.column) != 4)
            continue;
        if (map->count == map->capacity) {
            size_t next = map->capacity ? map->capacity * 2u : 128u;
            q9_line_t *grown;
            if (next > Q9_LINE_MAX) next = Q9_LINE_MAX;
            if (map->count >= next) break;
            grown = (q9_line_t *)realloc(map->lines, next * sizeof(*map->lines));
            if (!grown) break;
            map->lines = grown;
            map->capacity = next;
        }
        value.address = (uint32_t)address;
        map->lines[map->count++] = value;
    }
    fclose(f);
    if (map->count == 0) {
        free(map->lines);
        memset(map, 0, sizeof(*map));
        return NULL;
    }
    sort_lines(map->lines, map->count);
    g_module_line_count++;
    return map;
}

void q9_line_init_from_env(void)
{
    const char *path = getenv("Q9_LINE_MAP");
    FILE *f;
    char text[256];
    if (g_line_loaded) return;
    g_line_loaded = 1;
    if (!path || !*path || !(f = fopen(path, "r"))) return;
    while (fgets(text, sizeof(text), f) && g_line_count < Q9_LINE_MAX) {
        unsigned long address;
        q9_line_t *entry = &g_lines[g_line_count];
        if (sscanf(text, " 0x%lx %127[^:]:%u:%u", &address, entry->source,
                   &entry->line, &entry->column) != 4) continue;
        entry->address = (uint32_t)address;
        g_line_count++;
    }
    fclose(f);
    sort_lines(g_lines, g_line_count);
}

int q9_line_lookup(uint32_t pc, char *out, size_t out_len)
{
    size_t lo = 0, hi = g_line_count;
    const q9_line_t *best = NULL;
    if (!out || out_len == 0) return 0;
    out[0] = 0;
    if (!g_line_loaded) q9_line_init_from_env();
    while (lo < hi) {
        size_t mid = lo + (hi - lo) / 2u;
        if (g_lines[mid].address <= pc) {
            best = &g_lines[mid];
            lo = mid + 1u;
        } else hi = mid;
    }
    if (!best || best->source[0] == '!') return 0;
    snprintf(out, out_len, "%s:%u:%u", best->source, best->line, best->column);
    return 1;
}


int q9_line_lookup_module(const char *module, uint32_t module_base,
                          uint32_t pc, char *out, size_t out_len)
{
    q9_module_line_map_t *map;
    const q9_line_t *best = NULL;
    size_t lo = 0, hi;
    uint32_t offset;
    if (!out || out_len == 0) return 0;
    out[0] = 0;
    if (pc < module_base) return 0;
    map = load_module_lines(module);
    if (!map) return 0;
    offset = pc - module_base;
    hi = map->count;
    while (lo < hi) {
        size_t mid = lo + (hi - lo) / 2u;
        if (map->lines[mid].address <= offset) {
            best = &map->lines[mid];
            lo = mid + 1u;
        } else {
            hi = mid;
        }
    }
    if (!best || best->source[0] == '!') return 0;
    snprintf(out, out_len, "%s:%u:%u", best->source, best->line, best->column);
    return 1;
}
