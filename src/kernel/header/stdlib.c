#include <stdint.h>
#include <stdlib.h>

#define HEAP_BASE ((uint8_t*)0x01000000)
#define HEAP_SIZE 0x00800000 

static size_t heap_index = 0;

void* malloc(size_t size) {
    if (heap_index + size > HEAP_SIZE)
        return 0; // out of memory

    void* ptr = HEAP_BASE + heap_index;
    heap_index += size;
    return ptr;
}

void free(void* ptr) {
    // no-op
}
