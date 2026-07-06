#pragma once
#include <stdint.h>

// ---------------------------------------------------------
// ATA DMA API
// ---------------------------------------------------------
void ata_dma_init(void);
int  ata_dma_read(uint32_t lba, uint16_t count, void* out);
int  ata_dma_write(uint32_t lba, uint16_t count, const void* in);