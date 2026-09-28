#include <stdint.h>
#include <mapping.h>

__attribute__((aligned(4096))) static uint64_t pml4[512];
__attribute__((aligned(4096))) static uint64_t pdpt[512];
__attribute__((aligned(4096))) static uint64_t pd[512];
__attribute__((aligned(4096))) static uint64_t pt[512];

static void map_identity(uint64_t start, uint64_t end) {
    start &= ~0xFFF;
    end   = (end + 0xFFF) & ~0xFFF;

    for (uint64_t addr = start; addr < end; addr += 0x1000) {
        uint64_t idx = (addr >> 12) & 0x1FF;
        pt[idx] = addr | 0x3;
    }
}

/*
void setup_paging(void) {
    for (int i = 0; i < 512; i++) {
        pml4[i] = pdpt[i] = pd[i] = pt[i] = 0;
    }

    pml4[0] = (uint64_t)pdpt | 0x3;
    pdpt[0] = (uint64_t)pd   | 0x3;
    pd[0]   = (uint64_t)pt   | 0x3;

    map_identity(0x00000, 0x200000);

    __asm__ volatile("mov %0, %%cr3" :: "r"(pml4) : "memory");
}
*/

void setup_paging(void) {
    for (int i = 0; i < 512; i++) {
        pml4[i] = pdpt[i] = pd[i] = 0;
    }

    // PML4[0] -> PDPT
    pml4[0] = (uint64_t)pdpt | 0x3;

    // PDPT[0] -> PD
    pdpt[0] = (uint64_t)pd | 0x3;

    // PD: 2MB-Hugepages für z.B. 128 MB
    for (int i = 0; i < 64; i++) {          // 64 * 2MB = 128MB
        uint64_t addr = (uint64_t)i * 0x200000;
        pd[i] = addr | 0x83;                // Present + Write + PS (bit 7)
    }

    __asm__ volatile("mov %0, %%cr3" :: "r"(pml4) : "memory");
}
