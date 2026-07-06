#include <heap.h>

#define HEAP_SIZE (1024 * 1024)

extern char __kernel_end;
static size_t bump = 0;

void* kmalloc(size_t size) {
    void* base = (void*)&__kernel_end;
    void* p = base + bump;
    bump += size;
    return p;
}

void kfree(void* ptr) {
    
}