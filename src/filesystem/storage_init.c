#include <storage.h>
#include <usb.h>
#include <ahci.h>

int g_use_usb = 0;     
int g_use_ahci = 0;   

void storage_init() {
    if (usb_init() == 0 && usb_has_valid_fat32()) {
        g_use_usb = 1;
        g_use_ahci = 0;
        return;
    }

    if (ahci_init() == 0 && ahci_has_valid_fat32()) {
        g_use_usb = 0;
        g_use_ahci = 1;
        return;
    }

    g_use_usb = 0;
    g_use_ahci = 0;
}
