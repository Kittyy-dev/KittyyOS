#include <stdint.h>
#include <e820.h>
#include <stdbool.h>
#include <bootinfo.h>

#define E820_COUNT_ADDR 0x4FFE
#define E820_MAP_ADDR   0x5000

uint64_t get_ram_size() {
    volatile uint16_t *cntp = (volatile uint16_t*)E820_COUNT_ADDR;
    volatile uint8_t  *mapp = (volatile uint8_t*)E820_MAP_ADDR;

    uint16_t e820_count = *cntp;
    uint8_t first_byte  = *mapp;   // Debug


    uint64_t total = 0;
    uint8_t* ptr = (uint8_t*)E820_MAP_ADDR;

    for (uint16_t i = 0; i < e820_count; i++) {
        uint64_t base   = *(uint64_t*)(ptr + 0);
        uint64_t length = *(uint64_t*)(ptr + 8);
        uint32_t type   = *(uint32_t*)(ptr + 16);

        if (type == 1)
            total += length;

        ptr += 24;
    }

    return total;
}

bool is_bios_boot() {
    uint16_t count = *(uint16_t*)E820_COUNT_ADDR;
    return (count > 0);
}

bool is_uefi_boot() {
    volatile uint8_t *boot_type = (volatile uint8_t*)BOOT_TYPE_ADDR;

    return *boot_type == BOOT_TYPE_UEFI;
}