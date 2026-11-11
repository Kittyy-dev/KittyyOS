#pragma once
#include <stdint.h>
#include <vfs.h>

#define ATA_DATA     0x1F0
#define ATA_SECCOUNT 0x1F2
#define ATA_LBA_LOW  0x1F3
#define ATA_LBA_MID  0x1F4
#define ATA_LBA_HIGH 0x1F5
#define ATA_DRIVE    0x1F6
#define ATA_CMD      0x1F7
#define ATA_CMD_WRITE_DMA 0xCA
#define ATA_CMD_READ  0x20
#define ATA_CMD_WRITE 0x30
#define ATA_CMD_READ_DMA 0xC8

#define BM_CMD      (bmide_base + 0x00)
#define BM_STATUS   (bmide_base + 0x02)
#define BM_PRDT     (bmide_base + 0x04)

void ata_dma_init(void);
int ata_dma_read(uint32_t lba, uint8_t *buffer, uint8_t count);
int ata_dma_write(uint32_t lba, uint16_t count, const void* in);
size_t devhda_write(vfs_node_t *node, const void *buf, size_t len);
size_t devhda_read(vfs_node_t *node, void *buf, size_t len);
int ata_find_boot_disk(void);