#include <stdint.h>
#include <stddef.h>
#include <net.h>
#include <net_config.h>
#include <kprint.h>

static uint16_t read_be16(const uint8_t *p) {
    return ((uint16_t)p[0] << 8) | p[1];
}

static uint16_t ipv4_checksum(const uint8_t *header, uint16_t length) {
    uint32_t sum = 0;

    for (uint16_t i = 0; i < length; i += 2) {
        sum += ((uint16_t)header[i] << 8) | header[i + 1];
    }

    while (sum >> 16) {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }

    return (uint16_t)~sum;
}

void ipv4_receive(const uint8_t *packet, uint16_t length) {
    if (!packet || length < 20) {
        return;
    }

    uint8_t version = packet[0] >> 4;
    uint8_t ihl = packet[0] & 0x0F;

    if (version != 4 || ihl < 5) {
        return;
    }

    uint16_t header_length = (uint16_t)ihl * 4;

    if (header_length > length) {
        return;
    }

    uint16_t total_length = read_be16(&packet[2]);

    if (total_length < header_length ||
        total_length > length) {
        return;
    }

    if (ipv4_checksum(packet, header_length) != 0) {
        return;
    }

    const uint8_t *src = &packet[12];
    const uint8_t *dst = &packet[16];

    const struct ipv4_config *cfg = net_config_get();

    if (dst[0] != cfg->ip[0] || dst[1] != cfg->ip[1] || dst[2] != cfg->ip[2] || dst[3] != cfg->ip[3]) {
        return;
    }

    uint8_t protocol = packet[9];

    log_kernel( "IPv4: %u.%u.%u.%u -> %u.%u.%u.%u Proto: %u Len: %u\n", (unsigned int)src[0], (unsigned int)src[1], (unsigned int)src[2], (unsigned int)src[3], (unsigned int)dst[0], (unsigned int)dst[1], (unsigned int)dst[2], (unsigned int)dst[3], (unsigned int)protocol, (unsigned int)total_length);

    uint16_t fragment = ((uint16_t)packet[6] << 8) | packet[7];

    if (fragment & 0x3FFF) {
        return;
    }

    if (protocol == IP_PROTO_ICMP) {
        icmp_receive(&packet[header_length], total_length - header_length, src);
    } else if (protocol == IP_PROTO_UDP) {
        udp_receive(&packet[header_length], total_length - header_length, src, dst);
    }
}
