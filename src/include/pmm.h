#ifndef PMM_H
#define PMM_H

#include <stdint.h>

void     pmm_init(uint64_t mem_size);
uint64_t pmm_alloc_page(void);
void     pmm_free_page(uint64_t phys);
uint64_t pmm_alloc_contiguous(uint32_t pages);

#endif
