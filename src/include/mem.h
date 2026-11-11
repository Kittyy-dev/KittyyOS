#ifndef E820_H
#define E820_H

#include <stdint.h>
#include <stdbool.h>

uint64_t get_ram_size(void);
uint64_t get_memory_mb(void);

#endif