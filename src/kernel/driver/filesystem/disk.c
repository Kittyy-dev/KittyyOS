#include <stdint.h>
#include <string.h>
#include <ports.h>
#include <disk.h>
#include <ata.h>
#include <storage.h>
#include <usb.h>
#include <vga.h>
#include <colors.h>
#include <tsc.h>
#include <kprint.h>
#include <ahci.h>

#define ATA_STATUS 0x1F7

extern int port;

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

void read_sectors_abs(uint32_t lba, uint8_t* buffer, uint8_t count) {
    if (g_use_ahci == 1) {
        while (count > 0) {
            uint8_t n = count > 16 ? 16 : count;

            int ret = ahci_read(port, lba, n, buffer);

            if (ret != 0) {
                kprintf(WHITE "<" RED " ERROR " WHITE "> " "AHCI read failed!: %d LBA: %u Count: %u Port: %u\n", ret, lba, n, port);
                return;
            }

            lba += n;
            buffer += n * 512;
            count -= n;
        }

        return;
    }

    if (g_use_ata == 1) {
        if (ata_dma_read(lba, buffer, count) != 0) {
            kprintf(WHITE "<" RED " ERROR " WHITE "> " "ATA read failed! LBA: %u Count: %u\n", lba, count);
            return;
        }

        return;
    }

    kprintf(WHITE "<" RED " ERROR " WHITE "> " "Read failed no controller or driver found!\n");

    return;
}

void read_sectors(uint32_t rel_lba, uint8_t* buffer) {
    read_sectors_abs(g_part_lba_start + rel_lba, buffer, 1);
}

void write_sectors_abs(uint32_t lba, const uint8_t *buffer, uint8_t count) {
    if (g_use_ahci == 1) {
        while (count > 0) {
            uint8_t n = count > 16 ? 16 : count;

            int ret = ahci_write(port, lba, n, buffer);

            if (ret != 0) {
                kprintf(WHITE "<" RED " ERROR " WHITE "> " "AHCI write failed! LBA: %u Count: %u Port: %u\n", lba, n, port);
                return;
            }

            lba += n;
            buffer += n * 512;
            count -= n;
        }

        return;
    }

    if (g_use_ata == 1) {
        if (ata_dma_write(lba, count, buffer) != 0) {
            kprintf(WHITE "<" RED " ERROR " WHITE "> " "ATA DMA write failed! LBA: %u Count: %u\n", lba, count);
            return;
        }

        return;
    }

    kprintf(WHITE "<" RED " ERROR " WHITE "> " "Write failed: no controller or driver found!\n");
}

void write_sectors(uint32_t rel_lba, uint8_t* buffer) {
    write_sectors_abs(g_part_lba_start + rel_lba, buffer, 1);
}