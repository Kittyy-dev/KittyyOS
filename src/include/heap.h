#ifndef HEAP_H
#define HEAP_H

#include <stdint.h>
#include <stddef.h>
#include <paging.h>

#define HEAP_PHYS  0x02000000ULL
#define HEAP_SIZE  (16ULL * 1024 * 1024)
#define HEAP_START (HHDM_BASE + HEAP_PHYS)
#define HEAP_END   (HEAP_START + HEAP_SIZE)
#define HEAP_BASE  ((uint8_t *)HEAP_START)
#define HEAP_PAGES (HEAP_INITIAL_SIZE / PAGE_SIZE)
#define HEAP_INITIAL_SIZE (16ULL * 1024 * 1024)
#define HEAP_ALIGN 16

typedef struct heap_block {
    size_t size;
    bool free;
    struct heap_block *next;
} heap_block_t;

void heap_init(void);

void *kmalloc(size_t size);
void *kcalloc(size_t count, size_t size);
void kfree(void *ptr);

size_t heap_free(void);
size_t heap_size(void);
size_t heap_used(void);

#endif