#ifndef PMM_H
#define PMM_H

#include <stdint.h>
#include <stddef.h>

#define PAGE_SIZE 4096
#define MAX_PAGES (1024 * 1024)   

void     pmm_init(uint64_t mem_size);
uint64_t pmm_alloc_page(void);
void     pmm_free_page(uint64_t phys);
uint64_t pmm_alloc_contiguous(uint32_t pages);
void pmm_free(void* ptr);
void* pmm_alloc(size_t size);
uint64_t pmm_alloc_pages(uint64_t count);

#endif
