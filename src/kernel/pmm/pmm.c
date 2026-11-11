#include <bootinfo.h>
#include <stdint.h>
#include <string.h>
#include <pmm.h>
#include <stddef.h>
#include <stdbool.h>
#include <kprint.h>
#include <paging.h>

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

static uint64_t pmm_alloc_internal(uint64_t count) {
    if (count == 0 || count > total_pages) {
        return 0;
    } 

    for (uint64_t i = 1; i <= total_pages - count; i++) {
        bool free = true;

        for (uint64_t j = 0; j < count; j++) {
            if (bitmap_test(i + j)) {
                free = false;
                break;
            }
        }

        if (!free) {
            continue;
        }

        for (uint64_t j = 0; j < count; j++) {
            bitmap_set(i + j);
        }

        return i * PAGE_SIZE;
    }

    return 0;
}

static void pmm_reserve_region(uint64_t base, uint64_t length) {
    if (length == 0) {
        return;
    }

    uint64_t start = base / PAGE_SIZE;
    uint64_t end = (base + length + PAGE_SIZE - 1) / PAGE_SIZE;

    if (end > total_pages) {
        end = total_pages;
    }

    for (uint64_t i = start; i < end; i++) {
        bitmap_set(i);
    }
}

static void pmm_free_region(uint64_t base, uint64_t length) {
    if (!length || base + length < base) {
        return;
    }

    uint64_t end_addr = base + length;

    uint64_t start = (base + PAGE_SIZE - 1) / PAGE_SIZE;
    uint64_t end = end_addr / PAGE_SIZE;

    if (start >= total_pages) {
        return;
    }

    if (end > total_pages) {
        end = total_pages;
    }

    for (uint64_t i = start; i < end; i++) {
        bitmap_clear(i);
    }
}

void pmm_init(uint64_t mem_size) {
    (void)mem_size;

    uint64_t max_addr = 0;

    if (!boot_info) {
        return;
    }

    uint16_t entries = boot_info->mem_entires;

    if (entries > 64) {
        entries = 64;
    }

    uint8_t* ptr = boot_info->mem;

    for (uint16_t i = 0; i < entries; i++) {
        uint64_t base;
        uint64_t length;

        memcpy(&base, ptr, 8);
        memcpy(&length, ptr + 8, 8);

        if (length && base + length >= base && base + length > max_addr) {
            max_addr = base + length;
        }

        ptr += 24;
    }

    total_pages = max_addr / PAGE_SIZE;

    if (total_pages > MAX_PAGES) {
        total_pages = MAX_PAGES;
    }

    memset(pmm_bitmap, 0xFF, sizeof(pmm_bitmap));

    ptr = boot_info->mem;

    for (uint16_t i = 0; i < entries; i++) {
        uint64_t base;
        uint64_t length;
        uint32_t type;

        memcpy(&base, ptr, 8);
        memcpy(&length, ptr + 8, 8);
        memcpy(&type, ptr + 16, 4);

        if (type == 1) {
            pmm_free_region(base, length);
        }

        ptr += 24;
    }

    bitmap_set(0);

    for (uint64_t i = 3; i <= 8; i++) {
        if (i < total_pages) {
            bitmap_set(i);
        }
    }

    uint64_t free_pages = 0;
    uint64_t used_pages = 0;

    for (uint64_t i = 0; i < total_pages; i++) {
        if (bitmap_test(i)) {
            used_pages++;
        } else {
            free_pages++;
        }
    }

    uint64_t kernel_start = (uint64_t)__kernel_phys_start;
    uint64_t kernel_end = (uint64_t)__kernel_phys_end;

    pmm_reserve_region(kernel_start, kernel_end - kernel_start);
    pmm_reserve_region(0, 0x100000);

    log_kernel("PMM: Total Pages: %llu\n", (unsigned long long)total_pages);
    log_kernel("PMM: Free Pages: %llu\n", (unsigned long long)free_pages);
    log_kernel("PMM: Used Pages: %llu\n", (unsigned long long)used_pages);
}

uint64_t pmm_alloc_page(void) {
    return pmm_alloc_internal(1);
}

void pmm_free_page(uint64_t phys) {
    if (phys == 0 || phys % PAGE_SIZE != 0) {
        return;
    }

    uint64_t idx = phys / PAGE_SIZE;

    if (idx >= total_pages) {
        return;
    }

    bitmap_clear(idx);
}

uint64_t pmm_alloc_contiguous(uint32_t pages) {
    return pmm_alloc_internal(pages);
}

void* pmm_alloc(size_t size) {
    if (size == 0) {
        return NULL;
    }

    uint64_t pages = (size + PAGE_SIZE - 1) / PAGE_SIZE;
    uint64_t phys = pmm_alloc_pages(pages);

    return phys ? (void *)(HHDM_BASE + phys) : NULL;
}

void pmm_free(void* ptr) {
    if (!ptr) {
        return;
    }

    uint64_t phys = (uint64_t)ptr;
    pmm_free_page(phys);
}

uint64_t pmm_alloc_pages(uint64_t count) {
    return pmm_alloc_internal(count);
}