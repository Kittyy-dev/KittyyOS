#include <stdint.h>
#include <net.h>
#include <net_config.h>

static uint16_t be16(const uint8_t *p) {
    return ((uint16_t)p[0] << 8) | p[1];
}

static void put16(uint8_t *p, uint16_t value) {
    p[0] = (uint8_t)(value >> 8);
    p[1] = (uint8_t)value;
}

static uint32_t checksum_add(uint32_t sum, const uint8_t *data, uint16_t length) {
    uint16_t i = 0;

    for (; i + 1 < length; i += 2) {
        sum += ((uint16_t)data[i] << 8) | data[i + 1];
    }

    if (i < length) {
        sum += (uint16_t)data[i] << 8;
    }

    return sum;
}

static uint16_t checksum_finish(uint32_t sum) {
    while (sum >> 16) {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }

    return (uint16_t)~sum;
}

int udp_send(const uint8_t dst_ip[4], uint16_t src_port, uint16_t dst_port, const uint8_t *data, uint16_t length) {
    if (!dst_ip || (!data && length) || length > UDP_MAX_DATA) {
        return 0;
    }

    const struct ipv4_config *cfg = net_config_get();

    if (!cfg) {
        return 0;
    }

    uint8_t packet[1480] = {0};
    uint16_t udp_len = (uint16_t)(8 + length);

    put16(&packet[0], src_port);
    put16(&packet[2], dst_port);
    put16(&packet[4], udp_len);

    for (uint16_t i = 0; i < length; i++) {
        packet[8 + i] = data[i];
    }

    uint32_t sum = 0;

    sum = checksum_add(sum, cfg->ip, 4);
    sum = checksum_add(sum, dst_ip, 4);
    sum += IP_PROTO_UDP;
    sum += udp_len;
    sum = checksum_add(sum, packet, udp_len);

    uint16_t check = checksum_finish(sum);

    put16(&packet[6], check ? check : 0xFFFF);

    return ipv4_send(dst_ip, IP_PROTO_UDP, packet, udp_len);
}

void udp_receive(const uint8_t *packet, uint16_t length, const uint8_t src_ip[4], const uint8_t dst_ip[4]) {
    if (!packet || !src_ip || !dst_ip || length < 8) {
        return;
    }

    uint16_t src_port = be16(&packet[0]);
    uint16_t dst_port = be16(&packet[2]);
    uint16_t udp_len = be16(&packet[4]);

    if (udp_len < 8 || udp_len > length) {
        return;
    }

    if (be16(&packet[6]) != 0) {
        uint32_t sum = 0;

        sum = checksum_add(sum, src_ip, 4);
        sum = checksum_add(sum, dst_ip, 4);
        sum += IP_PROTO_UDP;
        sum += udp_len;
        sum = checksum_add(sum, packet, udp_len);

        if (checksum_finish(sum) != 0) {
            return;
        }
    }

    if (src_port == 53) {
        dns_receive(&packet[8], udp_len - 8, src_ip, src_port, dst_port);
    }
}
