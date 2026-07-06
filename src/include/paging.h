// paging.h
#pragma once
#include <stdint.h>

#define PAGE_PRESENT  (1ULL << 0)
#define PAGE_RW       (1ULL << 1)
#define PAGE_USER     (1ULL << 2)
      
void* alloc_page();         
void map_page(uint64_t virt, uint64_t phys, uint64_t flags);
void paging_init(void);