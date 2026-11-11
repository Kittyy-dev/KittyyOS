// paging.h
#pragma once
#include <stdint.h>

#define PAGE_WRITE    (1ULL << 1)
#define PAGE_PRESENT  (1ULL << 0)
#define PAGE_RW       (1ULL << 1)
#define PAGE_USER     (1ULL << 2)
#define PAGE_NX       (1ULL << 63)
#define PAGE_PWT (1ULL << 3)  // Page-level Write-Through
#define PAGE_PCD (1ULL << 4)  // Page-level Cache Disable

#define USER_CODE_ADDR  0x400000
#define USER_STACK_ADDR 0x800000
#define USER_STACK_TOP  (USER_STACK_ADDR + PAGE_SIZE)

#define PAGE_POOL_BASE 0x01000000

#define KERNEL_LMA    0x00100000ULL
#define KERNEL_OFFSET 0xFFFFFFFF7FF00000ULL
#define HHDM_BASE     0xFFFF800000000000ULL
#define PAGE_HUGE   (1ULL << 7)
#define ADDR_MASK   0x000FFFFFFFFFF000ULL

extern uint64_t* pml4;
extern uint8_t page_pool[];
extern char __kernel_end[];

void* alloc_page();         
void map_page(uint64_t virt, uint64_t phys, uint64_t flags);
void paging_init(void);
uint64_t virt_to_phys(uint64_t virt);
void user_map_page(uint64_t virt, uint64_t phys, uint64_t flags);
uint64_t get_pte(uint64_t virt);