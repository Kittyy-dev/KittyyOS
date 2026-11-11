#ifndef DMA_H
#define DMA_H

#include <stdint.h>

struct dma_buf {
    void*    virt;
    uint64_t phys;
    uint32_t len;
};

struct dma_buf dma_alloc(uint32_t size);
void dma_free(struct dma_buf* buf);

#endif
