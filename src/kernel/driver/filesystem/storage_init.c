#include <storage.h>
#include <usb.h>
#include <kprint.h>
#include <colors.h>
#include <pci.h>
#include <ahci.h>
#include <devfs.h>
#include <ata.h>

int g_use_usb = 0;
int g_use_ahci = 0;
int g_use_ata = 0;
int port;

void storage_init() {
    // PCI Driver
    kprintf(WHITE "<" GREEN " KERNEL " WHITE "> " "Scanning PCI!\n");
    pci_scan();

    if (pci_ahci_found == 1) {
        kprintf(WHITE "<" GREEN " KERNEL " WHITE "> " "AHCI Controller found!\n");
        ahci_init();
        
        kprintf(WHITE "<" GREEN " KERNEL " WHITE "> " "Selected AHCI: %u:%u.%u\n", pci_ahci_bus, pci_ahci_slot, pci_ahci_func);

        port = ahci_find_port();
        fat32_check_partition(port);

        // devfs_register_device("sda", devsda_read, devsda_write, NULL);

        g_use_ahci = 1;
    }

    if (pci_ide_found == 1) {
        kprintf(WHITE "<" GREEN " KERNEL " WHITE "> " "Using IDE Controller!\n");

        // devfs_register_device("hda", devhda_read, devhda_write, NULL);

        g_use_ata = 1;
    }
}
