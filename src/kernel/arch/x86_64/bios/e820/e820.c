#include <stdint.h>
#include <e820.h>

extern uint16_t e820_count;

uint64_t get_ram_size() {
    uint8_t* ptr = (uint8_t*)0x5000;
    uint64_t total = 0;

    for (int i = 0; i < e820_count; i++) {
        uint64_t base = *(uint64_t*)(ptr + 0);
        uint64_t length = *(uint64_t*)(ptr + 8);
        uint32_t type = *(uint32_t*)(ptr + 16);

        if (type == 1) {
            total += length;
        }

        ptr += 24;
    }

    return total;
}