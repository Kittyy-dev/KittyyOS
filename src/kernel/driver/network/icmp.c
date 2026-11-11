#include <stdint.h>
#include <net.h>
#include <kprint.h>

static uint16_t ping_sequence = 0;

static uint16_t icmp_checksum(const uint8_t *data, uint16_t len) {
    uint32_t sum = 0;

    for (uint16_t i = 0; i + 1 < len; i += 2) {
        sum += ((uint16_t)data[i] << 8) | data[i + 1];
    }

    if (len & 1) {
        sum += (uint16_t)data[len - 1] << 8;
    }

    while (sum >> 16) {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }

    return (uint16_t)~sum;
}

int icmp_ping(const uint8_t dst_ip[4]) {
    if (!dst_ip) {
        return 0;
    }

    uint8_t packet[16] = {0};

    packet[0] = 8;
    packet[1] = 0;

    packet[4] = 0x4B;
    packet[5] = 0x59;

    uint16_t seq = ++ping_sequence;
    packet[6] = (uint8_t)(seq >> 8);
    packet[7] = (uint8_t)seq;

    packet[8]  = 'K';
    packet[9]  = 'i';
    packet[10] = 't';
    packet[11] = 't';
    packet[12] = 'y';
    packet[13] = 'y';
    packet[14] = 'O';
    packet[15] = 'S';

    uint16_t sum = icmp_checksum(packet, sizeof(packet));
    packet[2] = (uint8_t)(sum >> 8);
    packet[3] = (uint8_t)sum;

    if (!ipv4_send(dst_ip, 1, packet, sizeof(packet))) {
        log_error("ICMP: Failed to send ping\n");
        return 0;
    }

    log_kernel("ICMP: Echo Request sent, seq=%u\n", (unsigned int)seq);

    return 1;
}


void icmp_receive(const uint8_t *packet, uint16_t length, const uint8_t src_ip[4]) {
    if (!packet || !src_ip || length < 8) {
        return;
    }

    if (icmp_checksum(packet, length) != 0) {
        return;
    }

    if (packet[0] == 0 && packet[1] == 0 && packet[4] == 0x4B && packet[5] == 0x59) {

        uint16_t seq = ((uint16_t)packet[6] << 8) | packet[7];

        log_kernel("PING: Reply from %u.%u.%u.%u seq=%u\n", (unsigned int)src_ip[0], (unsigned int)src_ip[1], (unsigned int)src_ip[2], (unsigned int)src_ip[3], (unsigned int)seq);
    }
}