#include <stdint.h>
#include <ahci.h>
#include <kprint.h>
#include <colors.h>

int fat32_check_partition(uint8_t port) {
    uint8_t sector[512];

    // LBA 0 = MBR
    int ret = ahci_read(port, 0, 1, sector);

    if (ret != 0) {
        kprintf(WHITE "<" RED " ERROR " WHITE "> " "Could not read MBR!\n");

        return -1;
    }

    // MBR Signature
    if (sector[510] != 0x55 || sector[511] != 0xAA) {
        kprintf(WHITE "<" RED " ERROR " WHITE "> " "Invalid MBR signature!\n");

        return -2;
    }

    kprintf(WHITE "<" GREEN " KERNEL " WHITE "> " "Valid MBR found!\n");

    // Partition table starts at 0x1BE
    for (int i = 0; i < 4; i++) {
        uint32_t offset = 0x1BE + (i * 16);

        uint8_t type = sector[offset + 4];

        uint32_t lba_start = (uint32_t)sector[offset + 8] | ((uint32_t)sector[offset + 9] << 8) | ((uint32_t)sector[offset + 10] << 16) | ((uint32_t)sector[offset + 11] << 24);
        uint32_t sector_count = (uint32_t)sector[offset + 12] | ((uint32_t)sector[offset + 13] << 8) | ((uint32_t)sector[offset + 14] << 16) | ((uint32_t)sector[offset + 15] << 24);

        if (type == 0x00) {
            continue;
        }

        kprintf(WHITE "<" GREEN " KERNEL " WHITE "> " "Partition %u: Type: %02x Start: %u Size: %u\n", i + 1, type, lba_start, sector_count);

        // Fat32
        if (type == 0x0B || type == 0x0C) {
            kprintf(WHITE "<" GREEN " KERNEL " WHITE "> " "FAT32 partition found: %u\n", i + 1);

            return (int)lba_start;
        }
    }

    kprintf(WHITE "<" RED " ERROR " WHITE "> " "No FAT32 partition found!\n");

    return -3;
}