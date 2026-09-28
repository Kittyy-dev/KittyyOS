#pragma once
#include <stdint.h>
#include <vfs.h>

void ata_dma_init(void);
int ata_dma_read(uint32_t lba, uint8_t *buffer, uint8_t count);
int ata_dma_write(uint32_t lba, uint16_t count, const void* in);
size_t devhda_write(vfs_node_t *node, const void *buf, size_t len);
size_t devhda_read(vfs_node_t *node, void *buf, size_t len);