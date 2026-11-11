#include <paging.h>
#include <stdint.h>

uint64_t *pml4;

static inline uint64_t kvirt_to_phys(void *p) {
    return (uint64_t)p - KERNEL_OFFSET;
}

static inline uint64_t *table_virt(uint64_t phys) {
    return (uint64_t*)(phys + KERNEL_OFFSET);
}

static inline void *phys_to_virt(uint64_t phys) {
    return (void*)(phys + HHDM_BASE);
}

static uint64_t *new_table(uint64_t *phys_out) {
    uint64_t *t = alloc_page();
    
    if (!t) {
        return 0;
    }

    for (int i = 0; i < 512; i++) {
        t[i] = 0;
    }

    *phys_out = kvirt_to_phys(t);

    return t;
}

static uint64_t *next_level(uint64_t *table, uint64_t idx, uint64_t flags) {
    uint64_t e = table[idx];

    if (!(e & PAGE_PRESENT)) {
        uint64_t phys;

        if (!new_table(&phys)) {
            return 0;
        }

        e = phys | PAGE_PRESENT | PAGE_RW | (flags & PAGE_USER);
        table[idx] = e;
    } else {
        if (e & PAGE_HUGE) {
            return 0;
        }

        if ((flags & PAGE_USER) && !(e & PAGE_USER)) {
            table[idx] = e | PAGE_USER;
        }
    }

    return table_virt(table[idx] & ADDR_MASK);
}

void map_page_internal(uint64_t virt, uint64_t phys, uint64_t flags) {
    uint64_t pml4_i = (virt >> 39) & 0x1FF;
    uint64_t pdpt_i = (virt >> 30) & 0x1FF;
    uint64_t pd_i = (virt >> 21) & 0x1FF;
    uint64_t pt_i = (virt >> 12) & 0x1FF;
    
    uint64_t *pdpt = next_level(pml4, pml4_i, flags);

    if (!pdpt) {
        return;
    }

    uint64_t *pd = next_level(pdpt, pdpt_i, flags);

    if (!pd) {
        return;
    }

    uint64_t *pt = next_level(pd, pd_i, flags);

    if (!pt) {
        return;
    }

    pt[pt_i] = (phys & ADDR_MASK) | flags | PAGE_PRESENT;
    
    asm volatile("invlpg (%0)" :: "r"(virt) : "memory");
}

void map_page(uint64_t virt, uint64_t phys, uint64_t flags) {
    map_page_internal(virt, phys, flags);
}

void user_map_page(uint64_t virt, uint64_t phys, uint64_t flags) {
    map_page_internal(virt, phys, flags | PAGE_USER);
}

static void map_huge_2m(uint64_t virt, uint64_t phys, uint64_t flags) {
    uint64_t *pdpt = next_level(pml4, (virt >> 39) & 0x1FF, flags);

    if (!pdpt) {
        return;
    }

    uint64_t *pd = next_level(pdpt, (virt >> 30) & 0x1FF, flags);

    if (!pd) {
        return;
    }

    pd[(virt >> 21) & 0x1FF] = (phys & ADDR_MASK) | flags | PAGE_PRESENT | PAGE_HUGE;
}

uint64_t get_pte(uint64_t virt) {
    uint64_t* pdpt = (uint64_t*)0;
    uint64_t e = pml4[(virt >> 39) & 0x1FF];

    if (!(e & PAGE_PRESENT)) {
        return 0;
    }

    pdpt = table_virt(e & ADDR_MASK);
    e = pdpt[(virt >> 30) & 0x1FF];
    
    if (!(e & PAGE_PRESENT)) {
        return 0;
    }

    uint64_t* pd = table_virt(e & ADDR_MASK);
    e = pd[(virt >> 21) & 0x1FF];

    if (!(e & PAGE_PRESENT)) {
        return 0;
    }

    if (e & PAGE_HUGE) {
        return e;
    }

    uint64_t* pt = table_virt(e & ADDR_MASK);
    return pt[(virt >> 12) & 0x1FF];
}

uint64_t virt_to_phys(uint64_t virt) {
    uint64_t e = get_pte(virt);

    if (!(e & PAGE_PRESENT)) {
        return 0;
    }

    if (e & PAGE_HUGE) {
        return (e & ADDR_MASK & ~0x1FFFFFULL) + (virt & 0x1FFFFFULL);
    }

    return (e & ADDR_MASK) + (virt & 0xFFF);
}

void paging_init(void) {
    uint64_t pml4_phys;

    pml4 = new_table(&pml4_phys);

    if (!pml4) {
        for (;;) {
            asm volatile("hlt");
        }
    }

    map_huge_2m(0x00000000, 0x00000000, PAGE_RW);

    for (uint64_t p = 0; p < 0x40000000ULL; p += 0x200000) {
        map_huge_2m(HHDM_BASE + p, p, PAGE_RW);
    }

    uint64_t kernel_end_phys = (uint64_t)__kernel_end - KERNEL_OFFSET;

    for (uint64_t p = KERNEL_LMA; p < kernel_end_phys; p += 0x1000) {
        map_page(p + KERNEL_OFFSET, p, PAGE_RW);
    }

    asm volatile("mov %0, %%cr3" :: "r"(pml4_phys) : "memory");
}