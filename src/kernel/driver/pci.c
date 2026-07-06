#include <pci.h>
#include <ports.h>
#include <stdint.h>

#define PCI_CONFIG_ADDRESS 0xCF8
#define PCI_CONFIG_DATA    0xCFC

int pci_xhci_found = 0;
uint8_t pci_xhci_bus = 0;
uint8_t pci_xhci_slot = 0;
uint8_t pci_xhci_func = 0;

static uint32_t pci_make_address(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset) {
    return (uint32_t)(
        (1U << 31) |
        ((uint32_t)bus  << 16) |
        ((uint32_t)slot << 11) |
        ((uint32_t)func << 8)  |
        (offset & 0xFC)
    );
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

void pci_scan_function(uint8_t bus, uint8_t slot, uint8_t func) {

    uint8_t class_code = pci_read_byte(bus, slot, func, 0x0B);
    uint8_t subclass   = pci_read_byte(bus, slot, func, 0x0A);
    uint8_t prog_if    = pci_read_byte(bus, slot, func, 0x09);

    if (class_code == 0x0C && subclass == 0x03 && prog_if == 0x30) {
        pci_xhci_found = 1;
        pci_xhci_bus = bus;
        pci_xhci_slot = slot;
        pci_xhci_func = func;
    }
}

void pci_scan_slot(uint8_t bus, uint8_t slot) {

    uint16_t vendor = pci_read_word(bus, slot, 0, 0x00);
    if (vendor == 0xFFFF) return;

    pci_scan_function(bus, slot, 0);

    uint8_t header = pci_read_byte(bus, slot, 0, 0x0E);
    if (header & 0x80) {
        for (int func = 1; func < 8; func++) {
            vendor = pci_read_word(bus, slot, func, 0x00);
            if (vendor != 0xFFFF)
                pci_scan_function(bus, slot, func);
        }
    }
}

void pci_scan_bus(uint8_t bus) {
    for (int slot = 0; slot < 32; slot++) {
        pci_scan_slot(bus, slot);
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
