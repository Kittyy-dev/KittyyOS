// paging.h
#pragma once
#include <stdint.h>

#define PAGE_WRITE    (1ULL << 1)
#define PAGE_PRESENT  (1ULL << 0)
#define PAGE_RW       (1ULL << 1)
#define PAGE_USER     (1ULL << 2)
#define PAGE_NX       (1ULL << 63)

#define PAGE_POOL_BASE 0x01000000

extern uint64_t* pml4;
extern uint8_t page_pool[];

void* alloc_page();         
void map_page(uint64_t virt, uint64_t phys, uint64_t flags);
void paging_init(void);
uint64_t virt_to_phys(uint64_t virt);
void user_map_page(uint64_t virt, uint64_t phys, uint64_t flags);
uint64_t get_pte(uint64_t virt);