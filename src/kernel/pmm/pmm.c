#include <stdint.h>
#include <string.h>
#include <pmm.h>

#define PAGE_SIZE 4096
#define MAX_PAGES (1024 * 1024)   

static uint8_t pmm_bitmap[MAX_PAGES / 8];
static uint64_t total_pages;

static inline void bitmap_set(uint64_t idx) {
    pmm_bitmap[idx >> 3] |= (1 << (idx & 7));
}

static inline void bitmap_clear(uint64_t idx) {
    pmm_bitmap[idx >> 3] &= ~(1 << (idx & 7));
}

static inline int bitmap_test(uint64_t idx) {
    return pmm_bitmap[idx >> 3] & (1 << (idx & 7));
}

void pmm_init(uint64_t mem_size) {
    total_pages = mem_size / PAGE_SIZE;
    memset(pmm_bitmap, 0, sizeof(pmm_bitmap));

    bitmap_set(0);
}

uint64_t pmm_alloc_page(void) {
    for (uint64_t i = 1; i < total_pages; i++) {
        if (!bitmap_test(i)) {
            bitmap_set(i);
            return i * PAGE_SIZE;
        }
    }
    return 0; 
}

void pmm_free_page(uint64_t phys) {
    uint64_t idx = phys / PAGE_SIZE;
    bitmap_clear(idx);
}

uint64_t pmm_alloc_contiguous(uint32_t pages) {
    for (uint64_t i = 1; i < total_pages - pages; i++) {
        int ok = 1;
        for (uint32_t p = 0; p < pages; p++) {
            if (bitmap_test(i + p)) {
                ok = 0;
                break;
            }
        }
        if (ok) {
            for (uint32_t p = 0; p < pages; p++)
                bitmap_set(i + p);
            return i * PAGE_SIZE;
        }
    }
    return 0;
}
