#ifndef PCI_H
#define PCI_H

#include <stdint.h>
#include <efi.h>

#define PCI_CONFIG_ADDRESS 0xCF8
#define PCI_CONFIG_DATA 0xCFC

uint8_t  pci_read_byte (uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset);
uint16_t pci_read_word (uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset);
uint32_t pci_read_dword(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset);

void pci_write_dword(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset, uint32_t value);
void pci_write_word(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset, uint16_t value);

void pci_enable_msi(uint8_t bus, uint8_t slot, uint8_t func, uint8_t vector);

uint8_t pci_header_type();
void pci_scan(EFI_SYSTEM_TABLE *SystemTable);
void pci_scan_bus(uint8_t bus, EFI_SYSTEM_TABLE *SystemTable);
void pci_scan_slot(uint8_t bus, uint8_t slot, EFI_SYSTEM_TABLE *SystemTable);
void pci_scan_function(uint8_t bus, uint8_t slot, uint8_t func, EFI_SYSTEM_TABLE *SystemTable);
uint64_t pci_get_bar5(uint8_t bus, uint8_t slot, uint8_t func);

extern int pci_ahci_found;
extern uint8_t pci_ahci_bus;
extern uint8_t pci_ahci_slot;
extern uint8_t pci_ahci_func;

extern int pci_nvme_found;
extern uint8_t pci_nvme_bus;
extern uint8_t pci_nvme_slot;
extern uint8_t pci_nvme_func;

extern int pci_ide_found;

#endif
