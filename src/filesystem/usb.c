#include <stdint.h>
#include <string.h>
#include <usb.h>
#include <xhci.h>
#include <vga.h>
#include <colors.h>
#include <storage.h>

extern uint8_t g_msc_bulk_in_ep;
extern uint8_t g_msc_bulk_out_ep;

int usb_init() {
    int r = xhci_init();

    if (r == 0) {
        vga_printf(WHITE "<" GREEN " KERNEL " WHITE "> " "USB XHCI initialized.\n");

        if (usb_has_valid_fat32()) {
            g_use_usb = 1;
            g_use_ahci = 0;
            return 0;
        }

        vga_printf(WHITE "<" RED " ERROR " WHITE "> " "Missing Fat32 on USB Device\n");
        return -1;
    } 
    else {
        vga_printf(WHITE "<" RED " ERROR " WHITE "> " "USB XHCI init failed.\n");
    }
    return -1;
}

int usb_has_valid_fat32() {

    // Activate ports
    xhci_enable_ports();

    int port = xhci_find_device_port();
    if (port < 0) {
        vga_printf(WHITE "<" RED " ERROR " WHITE "> No USB device detected on XHCI port.\n");
        return 0;
    }

    vga_printf(WHITE "<" GREEN " KERNEL " WHITE "> USB device detected on port %d.\n", port);

    uint8_t dev_desc_test[64];
    int r = xhci_get_device_descriptor(port, dev_desc_test, sizeof(dev_desc_test));
    vga_printf(WHITE "<" GREEN " KERNEL " WHITE "> devdesc ret=%d len=%d\n", r, dev_desc_test[0]);

    if (r < 0 || dev_desc_test[0] < 18) {
        vga_printf(WHITE "<" RED " ERROR " WHITE "> Device descriptor invalid.\n");
        return 0;
    }

    // Read device descriptor
    uint8_t dev_desc[64];
    if (xhci_get_device_descriptor(port, dev_desc, sizeof(dev_desc)) < 0) {
        vga_printf(WHITE "<" RED " ERROR " WHITE "> Failed to read device descriptor.\n");
        return 0;
    }

    // Check: Mass Storage?
    /*
    if (dev_desc[4] != 0x08) {
        vga_printf(WHITE "<" RED " ERROR " WHITE "> Device is not Mass Storage (Class=%x).\n", dev_desc[4]);
    } */

    vga_printf(WHITE "<" GREEN " KERNEL " WHITE "> Mass Storage device detected.\n");

    // Initialize endpoints
    if (xhci_init_mass_storage(port) < 0) {
        vga_printf(WHITE "<" RED " ERROR " WHITE "> Failed to initialize Bulk endpoints.\n");
        return 0;
    }

    uint8_t test[512];
    int res = usb_read_lba(0, 1, test);
    vga_printf(WHITE "<" GREEN " DEBUG " WHITE "> " "USB Read LBA(0): %d\n", res);
    for (int i = 0; i < 16; i++) {
        vga_printf("%x ", test[i]);
    }
    vga_printf("\n");

    vga_printf(WHITE "<" GREEN " KERNEL " WHITE "> Bulk endpoints set (IN=%x OUT=%x).\n",
               g_msc_bulk_in_ep, g_msc_bulk_out_ep);

    // FAT32 detection

    uint8_t mbr[512];
    if (usb_read_lba(0, 1, mbr) < 0) {
        vga_printf(WHITE "<" RED " ERROR " WHITE "> Failed to read MBR.\n");
        return 0;
    }

    // Boot signature
    if (mbr[510] != 0x55 || mbr[511] != 0xAA) {
        vga_printf(WHITE "<" RED " ERROR " WHITE "> Invalid MBR signature.\n");
        return 0;
    }

    // Partition entry 0
    // uint32_t start_lba = *(uint32_t*)&mbr[0x1BE + 8];
    uint32_t start_lba = *(uint32_t*)&mbr[0x1DE + 8];
    vga_printf(WHITE "<" YELLOW " INFO " WHITE "> " "Partition 2 starts at LBA %u\n", start_lba);

    vga_printf(WHITE "<" GREEN " KERNEL " WHITE "> Partition starts at LBA %x.\n", start_lba);

    uint8_t bs[512];
    if (usb_read_lba(start_lba, 1, bs) < 0) {
        vga_printf(WHITE "<" RED " KERNEL " WHITE "> Failed to read partition boot sector.\n");
        return 0;
    }

    uint32_t p2_lba = *(uint32_t*)&mbr[0x1DE + 8];
    uint32_t p2_size = *(uint32_t*)&mbr[0x1DE + 12];


    uint16_t fat16_size = *(uint16_t*)&bs[0x16];
    uint32_t fat32_size = *(uint32_t*)&bs[0x24];

    if (fat16_size == 0 && fat32_size != 0) {
        vga_printf(WHITE "<" GREEN " KERNEL " WHITE "> FAT32 detected at LBA %x.\n", start_lba);
        return 1;
    }

    vga_printf("MBR partition LBA = %u\n", start_lba);
    vga_printf("P2 LBA=%u SIZE=%u\n", p2_lba, p2_size);
    vga_printf(WHITE "<" RED " ERROR " WHITE "> No FAT32 structure found.\n");
    return 0;
}

int usb_read_lba(uint32_t lba, uint8_t count, uint8_t* buffer) {

    int res = xhci_scsi_read10(lba, count, buffer);
    if (res < 0) {
        memset(buffer, 0, count * 512);
        return -1;
    }

    return 0;
}
