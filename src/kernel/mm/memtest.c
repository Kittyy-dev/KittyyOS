#include <stdint.h>
#include <stdbool.h>
#include <paging.h>
#include <pmm.h>
#include <kprint.h>
#include <panic.h>

void test_hhdm(void) {
    uint64_t phys = 0x100000;

    volatile uint64_t *virt = (volatile uint64_t*)(HHDM_BASE + phys);

    uint64_t old = *virt;

    *virt = 0x123456789ABCDEF0ULL;

    if (*virt == 0x123456789ABCDEF0ULL) {
        log_kernel("HHDM: Ok!\n");
    } else {
        panic("HHDM: Failed!");
    }

    *virt = old;
}

void test_pmm(void) {
    uint64_t phys = pmm_alloc_page();

    if (!phys) {
        panic("PMM: Allocation Failed!");
        return;
    }

    log_kernel("PMM: Allocated: %llx\n", (unsigned long long)phys);

    volatile uint64_t *virt = (volatile uint64_t*)(HHDM_BASE + phys);

    *virt = 0xDEADBEEFCAFEBABEULL;

    if (*virt == 0xDEADBEEFCAFEBABEULL) {
        log_kernel("PMM: Read/Write Ok!\n");
    }

    pmm_free_page(phys);

    log_kernel("PMM: Free Ok!\n");
}