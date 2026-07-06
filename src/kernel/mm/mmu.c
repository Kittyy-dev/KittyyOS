#include <paging.h>
#include <stdint.h>
#include <colors.h>
#include <vga.h>

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

    for (uint64_t addr = 0; addr < (0x01000000 + 0x0800000); addr += 0x1000) { // 24 MB
        map_page(addr, addr, PAGE_RW);
    }

    // AHCI
    for (uint64_t addr = 0x91500000; addr < 0x91700000; addr += 0x1000) {
        map_page(addr, addr, PAGE_RW);
    }

    // XHCI
    /*
    for (uint64_t addr = g_xhci_mmio; addr < g_xhci_mmio + 0x10000; addr += 0x1000) {
        map_page(addr, addr, PAGE_RW);
    } */

    vga_printf(WHITE "<" YELLOW " INFO " WHITE "> " "PML4[0]=%x PD[0]=%x PDPT[0]=%x\n\n", pml4[0], pd[0], pdpt[0]);
    // vga_printf("PAGE POOL at %x\n", (uint64_t)page_pool);

    // CR3 auf PML4 setzen
    asm volatile("mov %0, %%cr3" :: "r"(pml4));
}
