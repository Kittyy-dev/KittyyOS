// alloc.c
#include <stdint.h>
#include <paging.h>

#define PAGE_SIZE 0x1000
#define PAGE_POOL_PAGES 256   // 256 Seiten = 1MB
#define PAGE_POOL_BASE 0x01000000

uint8_t page_pool[PAGE_POOL_PAGES * PAGE_SIZE] __attribute__((aligned(PAGE_SIZE)));
static uint64_t next_free_off = 0;

void* alloc_page(void) {
    if (next_free_off >= PAGE_POOL_PAGES * PAGE_SIZE) {
        return 0; 
    }

    void* p = &page_pool[next_free_off];
    next_free_off += PAGE_SIZE;
    return p;
}
