#ifndef Q9_LINE_H
#define Q9_LINE_H

#include <stddef.h>
#include <stdint.h>

void q9_line_init_from_env(void);
int q9_line_lookup(uint32_t pc, char *out, size_t out_len);
int q9_line_lookup_module(const char *module, uint32_t module_base,
                          uint32_t pc, char *out, size_t out_len);

#endif
