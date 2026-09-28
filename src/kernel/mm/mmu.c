#include <paging.h>
#include <stdint.h>
#include <colors.h>
#include <vga.h>
#include <kprint.h>

uint64_t* pml4;
uint64_t* pdpt;
uint64_t* pd;

extern uint8_t page_pool[];

void map_page(uint64_t virt, uint64_t phys, uint64_t flags) {
    uint64_t pml4_i = (virt >> 39) & 0x1FF;
    uint64_t pdpt_i = (virt >> 30) & 0x1FF;
    uint64_t pd_i   = (virt >> 21) & 0x1FF;
    uint64_t pt_i   = (virt >> 12) & 0x1FF;

    if (!(pml4[pml4_i] & PAGE_PRESENT)) {
        uint64_t* new_pdpt = alloc_page();
        pml4[pml4_i] = ((uint64_t)new_pdpt) | PAGE_PRESENT | PAGE_RW;
        for (int i = 0; i < 512; i++) new_pdpt[i] = 0;
    }
    uint64_t* pdpt = (uint64_t*)(pml4[pml4_i] & ~0xFFFULL);

    if (!(pdpt[pdpt_i] & PAGE_PRESENT)) {
        uint64_t* new_pd = alloc_page();
        pdpt[pdpt_i] = ((uint64_t)new_pd) | PAGE_PRESENT | PAGE_RW;
        for (int i = 0; i < 512; i++) new_pd[i] = 0;
    }
    uint64_t* pd = (uint64_t*)(pdpt[pdpt_i] & ~0xFFFULL);

    if (!(pd[pd_i] & PAGE_PRESENT)) {
        uint64_t* new_pt = alloc_page();
        pd[pd_i] = ((uint64_t)new_pt) | PAGE_PRESENT | PAGE_RW;
        for (int i = 0; i < 512; i++) new_pt[i] = 0;
    }
    uint64_t* pt = (uint64_t*)(pd[pd_i] & ~0xFFFULL);

    pt[pt_i] = (phys & ~0xFFFULL) | flags | PAGE_PRESENT;
}

void map_page_internal(uint64_t virt, uint64_t phys, uint64_t flags) {
    uint64_t pml4_i = (virt >> 39) & 0x1FF;
    uint64_t pdpt_i = (virt >> 30) & 0x1FF;
    uint64_t pd_i   = (virt >> 21) & 0x1FF;
    uint64_t pt_i   = (virt >> 12) & 0x1FF;

    if (!(pml4[pml4_i] & PAGE_PRESENT)) {
        uint64_t *new_pdpt = alloc_page();

        if (!new_pdpt) {
            return;
        }

        for (int i = 0; i < 512; i++) {
            new_pdpt[i] = 0;
        }

        pml4[pml4_i] = ((uint64_t)new_pdpt) | PAGE_PRESENT | PAGE_RW;
    }

    uint64_t *pdpt = (uint64_t*)(pml4[pml4_i] & ~0xFFFULL);

    if (!(pdpt[pdpt_i] & PAGE_PRESENT)) {
        uint64_t *new_pd = alloc_page();

        if (!new_pd) {
            return;
        }

        for (int i = 0; i < 512; i++) {
            new_pd[i] = 0;
        }

        pdpt[pdpt_i] = ((uint64_t)new_pd) | PAGE_PRESENT | PAGE_RW;
    }

    uint64_t *pd = (uint64_t *)(pdpt[pdpt_i] & ~0xFFFULL);

    if (!(pd[pd_i] & PAGE_PRESENT)) {
        uint64_t *new_pt = alloc_page();

        if (!new_pt) {
            return;
        }

        for (int i = 0; i < 512; i++) {
            new_pt[i] = 0;
        }

        pd[pd_i] = ((uint64_t)new_pt) | PAGE_PRESENT | PAGE_RW;
    }

    uint64_t *pt = (uint64_t*)(pd[pd_i] & ~0xFFFULL);

    pt[pt_i] = (phys & ~0xFFFULL) | flags | PAGE_PRESENT;

    asm volatile("invlpg (%0)" :: "r"(virt) : "memory");
}

void user_map_page(uint64_t virt, uint64_t phys, uint64_t flags) {
    map_page_internal(virt, phys, flags | PAGE_USER);
}

uint64_t get_pte(uint64_t virt) {
    uint64_t pml4_i = (virt >> 39) & 0x1FF;
    uint64_t pdpt_i = (virt >> 30) & 0x1FF;
    uint64_t pd_i   = (virt >> 21) & 0x1FF;
    uint64_t pt_i   = (virt >> 12) & 0x1FF;

    if (!(pml4[pml4_i] & PAGE_PRESENT)) {
        return 0;
    }

    uint64_t *pdpt = (uint64_t*)(pml4[pml4_i] & ~0xFFFULL);

    if (!(pdpt[pdpt_i] & PAGE_PRESENT)) {
        return 0;
    }

    uint64_t *pd = (uint64_t*)(pdpt[pdpt_i] & ~0xFFFULL);

    if (!(pd[pd_i] & PAGE_PRESENT)) {
        return 0;
    }

    uint64_t *pt = (uint64_t*)(pd[pd_i] & ~0xFFFULL);

    return pt[pt_i];
}

uint64_t virt_to_phys(uint64_t virt) {
    uint64_t pml4_index = (virt >> 39) & 0x1FF;
    uint64_t pdpt_index = (virt >> 30) & 0x1FF;
    uint64_t pd_index   = (virt >> 21) & 0x1FF;
    uint64_t pt_index   = (virt >> 12) & 0x1FF;
    uint64_t offset     = virt & 0xFFF;

    uint64_t pml4e = pml4[pml4_index];
    if (!(pml4e & PAGE_PRESENT))
        return 0;

    uint64_t *pdpt = (uint64_t *)(pml4e & ~0xFFFULL);

    uint64_t pdpte = pdpt[pdpt_index];
    if (!(pdpte & PAGE_PRESENT))
        return 0;

    uint64_t *pd = (uint64_t *)(pdpte & ~0xFFFULL);

    uint64_t pde = pd[pd_index];
    if (!(pde & PAGE_PRESENT))
        return 0;

    uint64_t *pt = (uint64_t *)(pde & ~0xFFFULL);

    uint64_t pte = pt[pt_index];
    if (!(pte & PAGE_PRESENT))
        return 0;

    return (pte & ~0xFFFULL) + offset;
}

void paging_init(void) {
    pml4 = alloc_page();
    pdpt = alloc_page();
    pd = alloc_page();

    for (int i = 0; i < 512; i++) {
        pml4[i] = 0;
        pdpt[i] = 0;
        pd[i] = 0;
    }

    pml4[0] = (uint64_t)pdpt | PAGE_PRESENT | PAGE_RW;
    pdpt[0] = (uint64_t)pd | PAGE_PRESENT | PAGE_RW;

    // General

    for (uint64_t addr = 0; addr < 0x01900000; addr += 0x1000) { // 24 MB
        map_page(addr, addr, PAGE_RW);
    }

    // CR3 auf PML4 setzen
    asm volatile("mov %0, %%cr3" :: "r"(pml4));
}