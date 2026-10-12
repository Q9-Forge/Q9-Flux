#include <stdio.h>
#include <string.h>
#include "../src/kernel/q9symbol.h"
#include "../src/kernel/q9line.h"

static int failures;
static void expect_symbol(const char *label, const char *module, unsigned base,
                          unsigned pc, const char *expected)
{
    char out[128];
    int found = q9_symbolize_module_pc(module, base, pc, out, sizeof(out));
    if (!found || strcmp(out, expected) != 0) {
        fprintf(stderr, "FAIL %s: found=%d '%s', expected '%s'\n",
                label, found, out, expected);
        failures++;
    } else {
        printf("ok   %s: %s\n", label, out);
    }
}

int main(void)
{
    char out[128];
    expect_symbol("exact entry", "sample", 0x2000u, 0x204au, "Init+0x0");
    expect_symbol("next symbol", "SAMPLE", 0x2000u, 0x2076u, "helper+0x6");
    expect_symbol("module relative", "sample", 0x3000u, 0x308du, "Write+0x3");
    if (q9_symbolize_module_pc("missing", 0x2000u, 0x2040u, out, sizeof(out))) {
        fprintf(stderr, "FAIL missing map unexpectedly resolved as %s\n", out);
        failures++;
    }
    if (q9_symbolize_module_pc("../sample", 0x2000u, 0x204au, out, sizeof(out))) {
        fprintf(stderr, "FAIL unsafe module name unexpectedly resolved\n");
        failures++;
    }
    if (!q9_line_lookup_module("SAMPLE", 0x2000u, 0x2050u, out, sizeof(out)) ||
        strcmp(out, "q9_sample.c:10:2") != 0) {
        fprintf(stderr, "FAIL module source line: '%s'\n", out);
        failures++;
    } else {
        printf("ok   module source line: %s\n", out);
    }
    if (!q9_line_lookup_module("sample", 0x2000u, 0x2061u, out, sizeof(out)) ||
        strcmp(out, "q9_sample.c:20:4") != 0) {
        fprintf(stderr, "FAIL next module source line: '%s'\n", out);
        failures++;
    }
    if (q9_line_lookup_module("sample", 0x2000u, 0x2070u, out, sizeof(out))) {
        fprintf(stderr, "FAIL module line map crossed its end marker: '%s'\n", out);
        failures++;
    }
    if (q9_line_lookup_module("../sample", 0x2000u, 0x2050u, out, sizeof(out))) {
        fprintf(stderr, "FAIL unsafe module line map unexpectedly resolved\n");
        failures++;
    }
    if (failures) return 1;
    puts("module symbol map tests passed");
    return 0;
}
