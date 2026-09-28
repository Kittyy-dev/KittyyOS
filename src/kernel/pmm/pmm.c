#include <stdint.h>
#include <string.h>
#include <pmm.h>
#include <stddef.h>
#include <stdbool.h>

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
        if (i >= 3 && i <= 8) {
            continue;
        }

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

void* pmm_alloc(size_t size) {
    uint64_t phys = pmm_alloc_page();

    if (!phys) {
        return NULL;
    }

    return (void*)phys;
}

void pmm_free(void* ptr) {
    if (!ptr) {
        return;
    }

    uint64_t phys = (uint64_t)ptr;
    pmm_free_page(phys);
}

uint64_t pmm_alloc_pages(uint64_t count) {
    if (count == 0) {
        return 0;
    }

    for (uint64_t i = 1; i <= total_pages - count; i++) {
        bool free = true;

        for (uint64_t j = 0; j < count; j++) {
            uint64_t page = i + j;

            if (page >= 3 && page <= 8) {
                free = false;
                break;
            }

            if (bitmap_test(page)) {
                free = false;
                break;
            }
        }

        if (!free) {
            continue;
        }

        for (uint64_t j = 0; j < count; j++) {
            bitmap_test(i + j);
        }

        return i * PAGE_SIZE;
    }

    return 0;
}