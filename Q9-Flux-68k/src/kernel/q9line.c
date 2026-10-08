#include "q9line.h"

#include <stdio.h>
#include <stdlib.h>

#define Q9_LINE_MAX 8192u

typedef struct {
    uint32_t address;
    unsigned line;
    unsigned column;
    char source[128];
} q9_line_t;

static q9_line_t g_lines[Q9_LINE_MAX];
static size_t g_line_count;
static int g_line_loaded;

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
    for (size_t i = 1; i < g_line_count; i++) {
        q9_line_t value = g_lines[i];
        size_t j = i;
        while (j > 0 && g_lines[j - 1u].address > value.address) {
            g_lines[j] = g_lines[j - 1u];
            j--;
        }
        g_lines[j] = value;
    }
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
    if (!best) return 0;
    snprintf(out, out_len, "%s:%u:%u", best->source, best->line, best->column);
    return 1;
}
