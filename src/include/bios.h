// bios.h
#pragma once
#include <stdint.h>
#include <paging.h>

struct RSDPDescriptor20 {
    char signature[8];
    uint8_t checksum;
    char oem_id[6];
    uint8_t revision;
    uint32_t rsdt_address;

    uint32_t length;
    uint64_t xsdt_address;
    uint8_t extended_checksum;
    uint8_t reserved[3];
} __attribute__((packed));

struct XSDT {
    char signature[4];
    uint32_t length;
    uint8_t revision;
    uint8_t checksum;
    char oem_id[6];
    char oem_table_id[8];
    uint32_t oem_revision;
    uint32_t creator_id;
    uint32_t creator_revision;
    uint64_t entries[];
} __attribute__((packed));

struct FADT {
    char signature[4];
    uint32_t length;
    uint8_t unneeded1[76];
    uint32_t pm1a_control_block;
    uint32_t pm1b_control_block;
    uint8_t unneeded2[112];
} __attribute__((packed));

struct RSDT {
    char signature[4];
    uint32_t length;
    uint8_t revision;
    uint8_t checksum;
    char oem_id[6];
    char oem_table_id[8];
    uint32_t oem_revision;
    uint32_t creator_id;
    uint32_t creator_revision;
    uint32_t entries[];
} __attribute__((packed));

// ACPI-Funktionen
void* find_rsdp(void);
struct FADT* get_fadt(void);
void acpi_shutdown(void);
void acpi_map_bios_region(void);