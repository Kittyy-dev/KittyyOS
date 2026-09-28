#include "fat.h"
#include <vfs.h>
#include <stddef.h>
#include <stdint.h>
#include <ports.h>
#include <vga.h>
#include <ata.h>
#include <string.h>
#include <kprint.h>
#include <colors.h>
#include <tsc.h>

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

extern int port;
extern int g_use_ahci;
extern int g_use_ata;

uint32_t g_part_lba_start = 0;

static uint32_t bmide_base = 0;

typedef struct {
    uint32_t base;
    uint16_t count;
    uint16_t flags;
} __attribute__((packed)) PRDEntry;

static PRDEntry prdt __attribute__((aligned(4)));
static uint8_t dma_buf[128 * 512] __attribute__((aligned(4)));

static uint32_t pci_read32(uint8_t bus, uint8_t dev, uint8_t func, uint8_t offset) {
    uint32_t addr = 0x80000000 | ((uint32_t)bus  << 16) | ((uint32_t)dev  << 11) | ((uint32_t)func << 8) | (offset & 0xFC);

    outl(0xCF8, addr);
    return inl(0xCFC);
}

void ata_dma_init(void) {
    uint32_t bar4 = pci_read32(0, 1, 1, 0x20);
    bmide_base = bar4 & ~0x3;
}

static inline uint32_t virt_to_phys(void* v) {
    return (uint64_t)v;
}

static inline void ata_wait_ready(void) {
    while (inb(ATA_CMD) & 0x80) { }
}

static inline int ata_wait_ready_timeout(uint32_t timeout_ms) {
    uint64_t start = rdtsc();

    while (inb(ATA_STATUS) & 0x80) {
        if (tsc_timeout(timeout_ms, start)) {
            return -1;
        }
    }

    return 0;
}

static inline int ata_wait_drq_timeout(uint32_t timeout_ms) {
    uint64_t start = rdtsc();

    while (1) {
        uint8_t st = inb(ATA_STATUS);

        if (st & 0x08) {
            return 0;
        }

        if (st & 0x01) {
            return -2;
        }

        if (tsc_timeout(timeout_ms, start)) {
            return -1;
        }
    }
}

int ata_dma_read(uint32_t lba, uint8_t *buffer, uint8_t count) {
    if (ata_wait_ready_timeout(10000) < 0) {
        kprintf(WHITE "<" RED " ERROR " WHITE "> " "ATA BSY timeout before read!\n");
        return -1;
    }

    outb(ATA_SECCOUNT, count);
    outb(ATA_LBA_LOW,  (uint8_t)(lba));
    outb(ATA_LBA_MID,  (uint8_t)(lba >> 8));
    outb(ATA_LBA_HIGH, (uint8_t)(lba >> 16));
    outb(ATA_DRIVE,    0xE0 | ((lba >> 24) & 0x0F));
    outb(ATA_CMD, ATA_CMD_READ);

    for (uint8_t s = 0; s < count; s++) {
        if (ata_wait_ready_timeout(150000) < 0) {
            kprintf(WHITE "<" RED " ERROR " WHITE "> " "ATA BSY timeout during read!\n");
            return -1;
        }

        if (ata_wait_drq_timeout(15000) < 0) {
            kprintf(WHITE "<" RED " ERROR " WHITE "> " "ATA DRQ timeout during read!\n");
            return -1;
        }

        for (int i = 0; i < 256; i++) {
            uint16_t data = inw(ATA_DATA);

            buffer[s * 512 + i * 2] = data & 0xFF;
            buffer[s * 512 + i * 2 + 1] = data >> 8;
        }
    }

    return 0;
}

int ata_dma_write(uint32_t lba, uint16_t count, const void* in) {
    if (count == 0 || count > 128 || bmide_base == 0) {
        return -1;
    }

    memcpy(dma_buf, in, count * 512);

    prdt.base  = virt_to_phys(dma_buf);
    prdt.count = (count * 512) - 1;
    prdt.flags = 0x8000;

    outl(BM_PRDT, virt_to_phys(&prdt));

    uint8_t st = inb(BM_STATUS);
    outb(BM_STATUS, st | 0x06);

    ata_wait_ready();

    outb(ATA_SECCOUNT, (uint8_t)count);
    outb(ATA_LBA_LOW,  (uint8_t)(lba));
    outb(ATA_LBA_MID,  (uint8_t)(lba >> 8));
    outb(ATA_LBA_HIGH, (uint8_t)(lba >> 16));
    outb(ATA_DRIVE,    0xE0 | ((lba >> 24) & 0x0F));

    outb(BM_CMD, 0x00);
    outb(BM_CMD, 0x01);

    outb(ATA_CMD, ATA_CMD_WRITE_DMA);

    uint32_t timeout = 0;
    while (1) {
        st = inb(BM_STATUS);
        if (st & 0x04) {
            break;
        }
        if (++timeout > 20000000) {
            kprintf(WHITE "<" RED " ERROR " WHITE "> " "DMA timeout!\n");
            outb(BM_CMD, 0x00);
            return -1;
        }
    }

    outb(BM_CMD, 0x00);

    if (st & 0x02) {
        return -1;
    }

    return 0;
}

size_t devhda_read(vfs_node_t *node, void *buf, size_t len) {
    (void)node;

    if (!buf || len == 0) {
        return 0;
    }

    if (len % 512 != 0) {
        return 0;
    }

    uint32_t sectors = len / 512;
    size_t done = 0;

    while (sectors > 0) {
        uint16_t count = sectors > 128 ? 128 : sectors;

        if (ata_dma_read(done / 512, (uint8_t*)buf + done, count) != 0) {
            break;
        }

        done += (size_t)count * 512;
        sectors -= count;
    }

    return done;
}

size_t devhda_write(vfs_node_t *node, const void *buf, size_t len) {
    (void)node;

    if (!buf || len == 0) {
        return 0;
    }

    if (len % 512 != 0) {
        return 0;
    }

    uint32_t sectors = len / 512;
    size_t done = 0;

    while (sectors > 0) {
        uint16_t count = sectors > 128 ? 128 : sectors;

        if (ata_dma_write(done / 512, count, (const uint8_t*)buf + done) != 0) {
            break;
        }

        done += (size_t)count * 512;
        sectors -= count;
    }

    return done;
}