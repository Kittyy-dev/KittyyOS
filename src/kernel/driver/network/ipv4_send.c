#include <stdint.h>
#include <stddef.h>
#include <net.h>
#include <net_config.h>
#include <e1000.h>
#include <kprint.h>

static uint16_t ipv4_id = 1;

static void copy_bytes(uint8_t *dst, const uint8_t *src, uint16_t count) {
    for (uint16_t i = 0; i < count; i++) {
        dst[i] = src[i];
    }
}

static uint16_t checksum(const uint8_t *data, uint16_t len) {
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

int ipv4_send(const uint8_t dst_ip[4], uint8_t protocol, const uint8_t *payload, uint16_t payload_len) {
    if (!dst_ip || (!payload && payload_len)) {
        return 0;
    }

    if (payload_len > 1480) {
        return 0;
    }

    const struct ipv4_config *cfg = net_config_get();
    const uint8_t *our_mac = e1000_get_mac();

    if (!cfg || !our_mac) {
        return 0;
    }

    const uint8_t *dst_mac = arp_get_gateway_mac();

    if (!dst_mac) {
        log_error("IPv4: Gateway MAC unknown\n");
        return 0;
    }

    uint8_t frame[1514] = {0};

    copy_bytes(&frame[0], dst_mac, 6);
    copy_bytes(&frame[6], our_mac, 6);

    frame[12] = 0x08;
    frame[13] = 0x00;

    uint8_t *ip = &frame[14];
    uint16_t total_len = (uint16_t)(20 + payload_len);

    ip[0] = 0x45;
    ip[1] = 0;
    ip[2] = (uint8_t)(total_len >> 8);
    ip[3] = (uint8_t)total_len;

    uint16_t id = ipv4_id++;
    ip[4] = (uint8_t)(id >> 8);
    ip[5] = (uint8_t)id;

    ip[6] = 0x40;
    ip[7] = 0;

    ip[8] = 64;
    ip[9] = protocol;

    copy_bytes(&ip[12], cfg->ip, 4);
    copy_bytes(&ip[16], dst_ip, 4);

    uint16_t sum = checksum(ip, 20);
    ip[10] = (uint8_t)(sum >> 8);
    ip[11] = (uint8_t)sum;

    copy_bytes(&ip[20], payload, payload_len);

    uint16_t frame_len = (uint16_t)(14 + total_len);

    if (frame_len < 60) {
        frame_len = 60;
    }

    return e1000_send(frame, frame_len);
}

