#include <stdint.h>
#include <paging.h>
#include <pmm.h>   
#include <dma.h>

struct dma_buf dma_alloc(uint32_t size) {
    struct dma_buf buf = {0};

    uint32_t pages = (size + 4095) / 4096;
    uint64_t phys = pmm_alloc_contiguous(pages);

    if (!phys)
        return buf;

    for (uint32_t i = 0; i < pages; i++)
        map_page(phys + i * 4096, phys + i * 4096, PAGE_RW);

    buf.phys = phys;
    buf.virt = (void*)(uintptr_t)phys; // Identity-Mapping
    buf.len  = pages * 4096;

    return buf;
}

void dma_free(struct dma_buf* buf) {
    if (!buf || !buf->phys || !buf->len)
        return;

    uint32_t page_size = 0x1000;
    uint32_t pages = buf->len / page_size;

    for (uint32_t i = 0; i < pages; i++) {
        uint64_t phys = buf->phys + (uint64_t)i * page_size;
        pmm_free_page(phys);
    }

    buf->phys = 0;
    buf->virt = 0;
    buf->len  = 0;
}
