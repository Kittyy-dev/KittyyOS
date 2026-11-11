#include <net_checksum.h>
#include <stddef.h>
#include <stdint.h>

uint16_t net_checksum(const void *data, size_t lenght) {
    const uint8_t *p = (const uint8_t*)data;
    uint32_t sum = 0;

    while (lenght >= 2) {
        sum += ((uint16_t)p[0] << 8) | p[1];
        p += 2;
        lenght -= 2;
    }

    if (lenght != 0) {
        sum += (uint16_t)p[0] << 8;
    }

    while (sum >> 16) {
        sum = (sum & 0xFFFFU) + (sum >> 16);
    }

    return (uint16_t)~sum;
}
