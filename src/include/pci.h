#ifndef PCI_H
#define PCI_H

#include <stdint.h>

uint8_t  pci_read_byte (uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset);
uint16_t pci_read_word (uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset);
uint32_t pci_read_dword(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset);

void pci_write_dword(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset, uint32_t value);
void pci_write_word(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset, uint16_t value);

void pci_enable_msi(uint8_t bus, uint8_t slot, uint8_t func, uint8_t vector);

uint8_t pci_header_type();
void pci_scan_bus(uint8_t bus);
void pci_scan_slot(uint8_t bus, uint8_t slot);
void pci_scan_function(uint8_t bus, uint8_t slot, uint8_t func);

extern int pci_xhci_found;
extern uint8_t pci_xhci_bus;
extern uint8_t pci_xhci_slot;
extern uint8_t pci_xhci_func;

#endif
