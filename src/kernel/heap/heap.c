#include <stdint.h>
#include <heap.h>
#include <stdint.h>
#include <stdlib.h>

static uint8_t *heap_current = (uint8_t*)HEAP_START;

uint32_t heap_used(void) {
    return (uint32_t)(heap_current - (uint8_t*)HEAP_START);
}

uint32_t heap_free(void) {
    return (uint32_t)(HEAP_END - (uintptr_t)heap_current);
}

void heap_init(void) {
    heap_current = (uint8_t*)HEAP_START;
}

void *kmalloc(uint32_t size) {
    if (size == 0) {
        return 0;
    }

    if ((uintptr_t)heap_current + size > HEAP_END) {
        return 0;
    }

    void *ptr = heap_current;
    heap_current += size;

    return ptr;
}

void *kcalloc(uint32_t count, uint32_t size) {
    uint32_t total = count * size;

    uint8_t *ptr = (uint8_t*)kmalloc(total);

    if (ptr == 0) {
        return 0;
    }

    for (uint32_t i = 0; i < total; i++) {
        ptr[i] = 0;
    }

    return ptr;
}

uint32_t heap_size(void) {
    return HEAP_SIZE;
}

static size_t heap_index = 0;

void* malloc(size_t size) {
    if (heap_index + size > HEAP_SIZE)
        return 0;

    void* ptr = HEAP_BASE + heap_index;
    heap_index += size;
    return ptr;
}

void free(void* ptr) {
    uintptr_t p = (uintptr_t)ptr;
    uintptr_t base = (uintptr_t)HEAP_BASE;

    if (p >= base && p < base + HEAP_SIZE) {
        heap_index = p - base;   // Rollback
    }
}
