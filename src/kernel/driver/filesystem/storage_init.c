#include <bootinfo.h>
#include <stdint.h>
#include <storage.h>
#include <usb.h>
#include <kprint.h>
#include <colors.h>
#include <pci.h>
#include <ahci.h>
#include <devfs.h>
#include <ata.h>
#include <fat.h>

int g_use_usb = 0;
int g_use_ahci = 0;
int g_use_ata = 0;
int port;

int mbr_matches_boot_disk(const uint8_t* mbr) {
    if (mbr[510] != 0x55 || mbr[511] != 0xAA) {
        return 0;
    }

    uint32_t signature = (uint32_t)mbr[0x1B8] | ((uint32_t)mbr[0x1B9] << 8) | ((uint32_t)mbr[0x1BA] << 16) | ((uint32_t)mbr[0x1BB] << 24);
    uint32_t partition_lba = (uint32_t)mbr[0x1BE + 8] | ((uint32_t)mbr[0x1BE + 9] << 8) | ((uint32_t)mbr[0x1BE + 10] << 16) | ((uint32_t)mbr[0x1BE + 11] << 24);

    if (signature != boot_info->mbr_signature) {
        return 0;
    }

    return 1;
}

void storage_init() {
    // PCI Driver
    log_kernel("Scanning PCI!\n");
    pci_scan();

    if (pci_ahci_found == 1) {
        log_kernel("AHCI Controller found!\n");
        ahci_init();
        
        log_kernel("Selected AHCI: %u:%u.%u\n", pci_ahci_bus, pci_ahci_slot, pci_ahci_func);

        port = ahci_find_port();
        
        if (port >= 0) {
            log_kernel("Boot disk found on AHCI port %d!\n", port);

            g_use_ahci = 1;
        } else {
            log_error("Boot disk not found on AHCI!\n");
        }

        // devfs_register_device("sda", devsda_read, devsda_write, NULL);
    }

    if (pci_xhci_found == 1) {
        log_kernel("Selected xHCI: %u:%u.%u\n", pci_xhci_bus, pci_xhci_slot, pci_xhci_func);
    }

    if (pci_ide_found == 1) {
        log_kernel("Using IDE Controller!\n");
        ata_dma_init();

        if (ata_find_boot_disk()) {
            log_kernel("Boot disk found on ATA!\n");
        } else {
            log_error("No boot disk found on ATA!\n");
        }

        // devfs_register_device("hda", devhda_read, devhda_write, NULL);

        g_use_ata = 1;
    }
}
