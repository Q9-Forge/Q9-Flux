#ifndef Q9_SYMBOL_H
#define Q9_SYMBOL_H

#include <stddef.h>
#include <stdint.h>

void q9_symbol_init_from_env(void);
int q9_symbolize_pc(uint32_t pc, char *out, size_t out_len);

#endif
