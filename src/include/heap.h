#ifndef HEAP_H
#define HEAP_H

#include <stdint.h>
#include <stddef.h>

#define HEAP_START 0x01000000
#define HEAP_SIZE (16 * 1024 * 1024)
#define HEAP_END (HEAP_START + HEAP_SIZE)
#define HEAP_BASE ((uint8_t*)0x01000000)

void heap_init(void);

void *kmalloc(uint32_t size);

void *kcalloc(uint32_t count, uint32_t size);

uint32_t heap_free(void);
uint32_t heap_size(void);
uint32_t heap_used(void);

#endif