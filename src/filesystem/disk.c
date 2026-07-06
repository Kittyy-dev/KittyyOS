#include <stdint.h>
#include <string.h>
#include <ports.h>
#include <disk.h>
#include <ata.h>
#include <ahci.h>
#include <storage.h>
#include <usb.h>

#define ATA_DATA     0x1F0
#define ATA_ERROR    0x1F1
#define ATA_SECCOUNT 0x1F2
#define ATA_LBA_LOW  0x1F3
#define ATA_LBA_MID  0x1F4
#define ATA_LBA_HIGH 0x1F5
#define ATA_DRIVE    0x1F6
#define ATA_STATUS   0x1F7
#define ATA_CMD      0x1F7

#define ATA_CMD_READ  0x20
#define ATA_CMD_WRITE 0x30

uint32_t g_part_lba_start = 0;

static inline void ata_wait_ready(void) {
    while (inb(ATA_STATUS) & 0x80) { }
}

void read_sectors_abs(uint32_t lba, uint8_t* buffer, uint8_t count) {

    if (g_use_usb) { // g_use_ahci
        // AHCI
        usb_read_lba(lba, count, buffer);
        return;
    }
    
    if (g_use_ahci) {
        ahci_read_lba(lba, count, buffer);
        return;
    }

    // IDE‑PIO fallback
    ata_wait_ready();
    outb(ATA_SECCOUNT, count);
    outb(ATA_LBA_LOW,  (uint8_t)(lba));
    outb(ATA_LBA_MID,  (uint8_t)(lba >> 8));
    outb(ATA_LBA_HIGH, (uint8_t)(lba >> 16));
    outb(ATA_DRIVE,    0xE0 | ((lba >> 24) & 0x0F));
    outb(ATA_CMD, ATA_CMD_READ);

    for (uint8_t s = 0; s < count; s++) {
        ata_wait_ready();
        while (!(inb(ATA_STATUS) & 0x08)) {}

        for (int i = 0; i < 256; i++) {
            uint16_t data = inw(ATA_DATA);
            buffer[s * 512 + i * 2]     = data & 0xFF;
            buffer[s * 512 + i * 2 + 1] = data >> 8;
        }
    }
}

void read_sectors(uint32_t rel_lba, uint8_t* buffer) {
    read_sectors_abs(g_part_lba_start + rel_lba, buffer, 1);
}

void write_sectors(uint32_t rel_lba, const uint8_t* buffer) {
    uint32_t lba = g_part_lba_start + rel_lba;

    // Erst DMA versuchen
    if (ata_dma_write(lba, 1, buffer) == 0)
        return;

    // PIO
    ata_wait_ready();

    outb(ATA_SECCOUNT, 1);
    outb(ATA_LBA_LOW,  (uint8_t)(lba));
    outb(ATA_LBA_MID,  (uint8_t)(lba >> 8));
    outb(ATA_LBA_HIGH, (uint8_t)(lba >> 16));
    outb(ATA_DRIVE,    0xE0 | ((lba >> 24) & 0x0F));
    outb(ATA_CMD, ATA_CMD_WRITE);

    ata_wait_ready();
    while (!(inb(ATA_STATUS) & 0x08)) {}

    for (int i = 0; i < 256; i++) {
        uint16_t data = (uint16_t)buffer[i * 2] |
                        ((uint16_t)buffer[i * 2 + 1] << 8);
        outw(ATA_DATA, data);
    }
}