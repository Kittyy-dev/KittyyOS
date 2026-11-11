#include <e1000.h>
#include <paging.h>
#include <pci.h>
#include <ports.h>
#include <stdint.h>
#include <kprint.h>
#include <colors.h>

#define PCI_CONFIG_ADDRESS 0xCF8
#define PCI_CONFIG_DATA    0xCFC

int pci_ahci_found = 0;
uint8_t pci_ahci_bus = 0;
uint8_t pci_ahci_slot = 0;
uint8_t pci_ahci_func = 0;

int pci_nvme_found = 0;
uint8_t pci_nvme_bus = 0;
uint8_t pci_nvme_slot = 0;
uint8_t pci_nvme_func = 0;

int pci_network_found = 0;
uint8_t pci_network_bus = 0;
uint8_t pci_network_slot = 0;
uint8_t pci_network_func = 0;

uint16_t pci_network_vendor = 0;
uint16_t pci_network_device = 0;
uint8_t pci_network_subclass = 0;

int pci_xhci_found = 0;
uint8_t pci_xhci_bus = 0;
uint8_t pci_xhci_slot = 0;
uint8_t pci_xhci_func = 0;

int pci_ide_found = 0;

static uint32_t pci_make_address(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset) {
    return (uint32_t)((1U << 31) | ((uint32_t)bus  << 16) | ((uint32_t)slot << 11) | ((uint32_t)func << 8)  | (offset & 0xFC));
}

void pci_write_dword(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset, uint32_t value) {
    outl(PCI_CONFIG_ADDRESS, pci_make_address(bus, slot, func, offset));
    outl(PCI_CONFIG_DATA, value);
}

void pci_write_word(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset, uint16_t value) {
    uint32_t old = pci_read_dword(bus, slot, func, offset);
    uint32_t shift = (offset & 2) * 8;
    uint32_t mask = 0xFFFF << shift;
    uint32_t newval = (old & ~mask) | ((uint32_t)value << shift);
    pci_write_dword(bus, slot, func, offset, newval);
}

uint32_t pci_read_dword(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset) {
    outl(PCI_CONFIG_ADDRESS, pci_make_address(bus, slot, func, offset));
    return inl(PCI_CONFIG_DATA);
}

uint16_t pci_read_word(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset) {
    uint32_t value = pci_read_dword(bus, slot, func, offset);
    return (value >> ((offset & 2) * 8)) & 0xFFFF;
}

uint8_t pci_read_byte(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset) {
    uint32_t value = pci_read_dword(bus, slot, func, offset);
    return (value >> ((offset & 3) * 8)) & 0xFF;
}

uint8_t pci_header_type() {
    return pci_read_byte(0, 0, 0, 0x0E);
}

void pci_enable_mmio_busmaster(uint8_t bus, uint8_t slot, uint8_t func) {
    uint16_t command = pci_read_word(bus, slot, func, 0x04);

    command |= (1u << 1); // Memory Space Enable
    command |= (1u << 2); // Bus Master Enable

    pci_write_word(bus, slot, func, 0x04, command);
}

void pci_scan_function(uint8_t bus, uint8_t slot, uint8_t func) {
    uint16_t vendor = pci_read_word(bus, slot, func, 0x00);

    if (vendor == 0xFFFF) {
        return;
    }

    uint8_t class_code = pci_read_byte(bus, slot, func, 0x0B);
    uint8_t subclass   = pci_read_byte(bus, slot, func, 0x0A);
    uint8_t prog_if    = pci_read_byte(bus, slot, func, 0x09);

    // AHCI
    if (class_code == 0x01 && subclass == 0x06 && prog_if == 0x01) {
        log_kernel("AHCI Controller found at %u:%u.%u\n", bus, slot, func);

        if (!pci_ahci_found) {
            pci_ahci_found = 1;
            pci_ahci_bus = bus;
            pci_ahci_slot = slot;
            pci_ahci_func = func;
        }
    }

    // XHCI
    if (class_code == 0x0C && subclass == 0x03 && prog_if == 0x30) {
        if (!pci_xhci_found) {
            pci_xhci_found = 1;

            pci_xhci_bus = bus;
            pci_xhci_slot = slot;
            pci_xhci_func = func;
        }

        log_kernel("xHCI Controller found at %u:%u.%u\n");
    }

    // NVMe
    if (class_code == 0x01 && subclass == 0x08 && prog_if == 0x02) {
        log_kernel("NVMe Controller found at %u:%u.%u\n", bus, slot, func);

        if (!pci_nvme_found) {
            pci_nvme_found = 1;
            pci_nvme_bus = bus;
            pci_nvme_slot = slot;
            pci_nvme_func = func;
        }
    }

    // IDE/ATA
    if (class_code == 0x01 && subclass == 0x01) {
        pci_ide_found = 1;

        log_kernel("IDE Controller found at %u:%u.%u\n", bus, slot, func);
    }


    // Network Controller
    if (class_code == 0x02) {
        uint16_t device_id = pci_read_word(bus, slot, func, 0x02);
        uint8_t revision = pci_read_byte(bus, slot, func, 0x08);

        log_kernel("NIC %02x:%02x.%u Vendor: %04x Device: %04x Rev: %02x Prog_if: %02x\n", (unsigned int)bus, (unsigned int)slot, (unsigned int)func, (unsigned int)vendor, (unsigned int)device_id, (unsigned int)revision, (unsigned int)prog_if);

        if (subclass == 0x00) {
            log_kernel("Network Type: Ethernet\n");
        } else if (subclass == 0x80) {
            log_kernel("Network Type: Other network controller\n");
        } else {
            log_kernel("Network Subclass: %02x\n", (unsigned int)subclass);
        }

        if (!pci_network_found) {
            pci_network_found = 1;
            pci_network_bus = bus;
            pci_network_slot = slot;
            pci_network_func = func;
            pci_network_vendor = vendor;
            pci_network_device = device_id;
            pci_network_subclass = subclass;
        }

        if (vendor == 0x8086 && device_id == 0x100E) {
            log_kernel("PCI: Supported Intel 82540EM detected!\n");

            pci_network_bus = bus;
            pci_network_slot = slot;
            pci_network_func = func;
            pci_network_vendor = vendor;
            pci_network_device = device_id;
            pci_network_subclass = subclass;

            pci_network_found = 1;
            
        } else {
            log_kernel("PCI: No driver for this NIC yet\n");
        }
    }

}

void pci_scan_slot(uint8_t bus, uint8_t slot) {
    uint16_t vendor = pci_read_word(bus, slot, 0, 0x00);
    if (vendor == 0xFFFF) {
        return;
    }

    pci_scan_function(bus, slot, 0);

    uint8_t header = pci_read_byte(bus, slot, 0, 0x0E);
    if (header & 0x80) {
        for (int func = 1; func < 8; func++) {
            vendor = pci_read_word(bus, slot, func, 0x00);
            if (vendor != 0xFFFF) {
                pci_scan_function(bus, slot, func);
            }
        }
    }
}

void pci_scan_bus(uint8_t bus) {
    for (int slot = 0; slot < 32; slot++) {
        pci_scan_slot(bus, slot);
    }
}

void pci_scan(void) {
    pci_ide_found = 0;
    pci_ahci_found = 0;
    pci_nvme_found = 0;
    pci_network_found = 0;

    for (uint16_t bus = 0; bus < 256; bus++) {
        for (uint8_t slot = 0; slot < 32; slot++) {
            for (uint8_t func = 0; func < 8; func++) {
                pci_scan_function(bus, slot, func);
            }
        }
    }

    if (!pci_network_found) {
        log_kernel("No PCI network controller found!\n");
    }
}

void pci_enable_msi(uint8_t bus, uint8_t slot, uint8_t func, uint8_t vector) {
    uint8_t cap_ptr = pci_read_byte(bus, slot, func, 0x34);

    while (cap_ptr) {
        uint8_t cap_id = pci_read_byte(bus, slot, func, cap_ptr);

        if (cap_id == 0x05) {
            uint16_t control = pci_read_word(bus, slot, func, cap_ptr + 2);

            uint32_t msg_addr = 0xFEE00000;   // LAPIC
            uint16_t msg_data = vector;       // Interrupt vector

            pci_write_dword(bus, slot, func, cap_ptr + 4, msg_addr);
            pci_write_word(bus, slot, func, cap_ptr + 8, msg_data);

            pci_write_word(bus, slot, func, cap_ptr + 2, control | 1);
            return;
        }

        cap_ptr = pci_read_byte(bus, slot, func, cap_ptr + 1);
    }
}

uint64_t pci_get_bar5(uint8_t bus, uint8_t slot, uint8_t func) {
    uint32_t low = pci_read_dword(bus, slot, func, 0x24);

    // I/O Bar
    if (low & 1) {
        return 0;
    }

    uint64_t bar = low & ~0xFULL;

    if (((low >> 1) & 0x3) == 0x2) {
        uint32_t high = pci_read_dword(bus, slot, func, 0x28);
        bar |= (uint64_t)high << 32;
    }

    return bar;
}

uint64_t pci_get_bar0(uint8_t bus, uint8_t slot, uint8_t func) {
    uint32_t low = pci_read_dword(bus, slot, func, 0x10);

    if (low & 1) {
        return 0; // I/O BAR
    }

    uint64_t bar = low & 0xFFFFFFF0;

    // 64-bit memory BAR
    if (((low >> 1) & 0x3) == 0x2) {
        uint32_t high = pci_read_dword(bus, slot, func, 0x14);

        bar |= (uint64_t)high << 32;
    }

    return bar;
}