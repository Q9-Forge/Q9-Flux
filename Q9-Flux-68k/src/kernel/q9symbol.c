#include "q9symbol.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define Q9_SYMBOL_MAX 4096u

typedef struct {
    uint32_t offset;
    char name[64];
} q9_symbol_t;

static q9_symbol_t g_symbols[Q9_SYMBOL_MAX];
static size_t g_symbol_count;
static uint32_t g_symbol_base = 0x7100u;
static int g_symbol_loaded;

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
    for (size_t i = 1; i < g_symbol_count; i++) {
        q9_symbol_t value = g_symbols[i];
        size_t j = i;
        while (j > 0 && g_symbols[j - 1u].offset > value.offset) {
            g_symbols[j] = g_symbols[j - 1u];
            j--;
        }
        g_symbols[j] = value;
    }
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
