#include <stdint.h>
#include <mem.h>
#include <stdbool.h>
#include <bootinfo.h>
#include <string.h>

#define MAGIC 0x4B545959

extern uint64_t mb;

uint64_t get_ram_size(void) {
    if (!boot_info) {
        return 0;
    }

    uint64_t total = 0;
    uint16_t entries = boot_info->mem_entires;

    if (entries > 64) {
        entries = 64;
    }

    uint8_t* ptr = boot_info->mem;

    for (uint16_t i = 0; i < entries; i++) {
        uint64_t base;
        uint64_t length;
        uint32_t type;

        memcpy(&base, ptr, 8);
        memcpy(&length, ptr + 8, 8);
        memcpy(&type, ptr + 16, 4);

        if (type == 1 && length != 0) {
            if (UINT64_MAX - total < length) {
                return 0;
            }

            total += length;
        }

        ptr += 24;
    }

    return total;
}

uint64_t get_memory_mb(void) {
    return mb;
}