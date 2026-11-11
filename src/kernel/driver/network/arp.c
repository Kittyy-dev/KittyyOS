#include <stdint.h>
#include <stddef.h>
#include <kprint.h>
#include <e1000.h>
#include <net.h>
#include <net_config.h>

static uint8_t gateway_mac[6];
static int gateway_mac_valid = 0;

static void copy_bytes(uint8_t *dst, const uint8_t *src, size_t n) {
    for (size_t i = 0; i < n; i++) {
        dst[i] = src[i];
    }
}

static int bytes_equal(const uint8_t *a, const uint8_t *b, size_t n) {
    for (size_t i = 0; i < n; i++) {
        if (a[i] != b[i]) {
            return 0;
        }
    }

    return 1;
}

static const uint8_t *arp_get_our_mac(void) {
    return e1000_get_mac();
}

static const uint8_t *arp_get_our_ip(void) {
    const struct ipv4_config *cfg = net_config_get();

    return cfg->ip;
}

static int is_local_ip(const uint8_t ip[4]) {
    const uint8_t *our_ip = arp_get_our_ip();

    return bytes_equal(ip, our_ip, 4);
}

void arp_request(const uint8_t target_ip[4]) {
    if (!target_ip) {
        return;
    }

    const uint8_t *our_mac = arp_get_our_mac();
    const uint8_t *our_ip = arp_get_our_ip();

    if (!our_mac || !our_ip) {
        log_error("ARP: MAC or IP unavailable\n");
        return;
    }

    uint8_t frame[ETH_ARP_FRAME_LEN] = {0};

    for (int i = 0; i < 6; i++) {
        frame[i] = 0xFF;
    }

    copy_bytes(&frame[6], our_mac, 6);

    frame[12] = 0x08;
    frame[13] = 0x06;

    uint8_t *arp = &frame[ETH_HEADER_LEN];

    arp[0] = 0x00;
    arp[1] = ARP_HTYPE_ETHERNET;

    arp[2] = 0x08;
    arp[3] = 0x00;

    arp[4] = 6;
    arp[5] = 4;

    arp[6] = 0x00;
    arp[7] = ARP_REQUEST;

    copy_bytes(&arp[8], our_mac, 6);
    copy_bytes(&arp[14], our_ip, 4);

    copy_bytes(&arp[24], target_ip, 4);

    if (e1000_send(frame, sizeof(frame))) {
        log_kernel("ARP: Request for %u.%u.%u.%u sent\n", (unsigned int)target_ip[0], (unsigned int)target_ip[1], (unsigned int)target_ip[2], (unsigned int)target_ip[3]
        );
    } else {
        log_error("ARP: TX queue busy\n");
    }
}

void arp_receive(const uint8_t *packet, uint16_t length) {
    if (!packet || length < ETH_HEADER_LEN) {
        return;
    }

    uint16_t ethertype = ((uint16_t)packet[12] << 8) | packet[13];

    if (ethertype != ETH_TYPE_ARP) {
        return;
    }

    if (length < ETH_HEADER_LEN + ARP_HEADER_LEN) {
        return;
    }

    const uint8_t *arp = &packet[ETH_HEADER_LEN];

    uint16_t htype = ((uint16_t)arp[0] << 8) | arp[1];
    uint16_t ptype = ((uint16_t)arp[2] << 8) | arp[3];
    uint16_t oper = ((uint16_t)arp[6] << 8) | arp[7];

    if (htype != ARP_HTYPE_ETHERNET || ptype != ARP_PTYPE_IPV4 || arp[4] != 6 || arp[5] != 4) {
        return;
    }

    const uint8_t *sender_mac = &arp[8];
    const uint8_t *sender_ip  = &arp[14];
    const uint8_t *target_mac = &arp[18];
    const uint8_t *target_ip  = &arp[24];

    const uint8_t *our_mac = arp_get_our_mac();
    const uint8_t *our_ip = arp_get_our_ip();

    if (!our_mac || !our_ip) {
        return;
    }

    const struct ipv4_config *cfg = net_config_get();

    if (oper == ARP_REPLY && is_local_ip(target_ip)) {
        log_kernel("ARP: Reply from %u.%u.%u.%u\n", (unsigned int)sender_ip[0], (unsigned int)sender_ip[1], (unsigned int)sender_ip[2], (unsigned int)sender_ip[3]);

        if (bytes_equal(sender_ip, cfg->gateway, 4)) {
            copy_bytes(gateway_mac, sender_mac, 6);
            gateway_mac_valid = 1;

            log_kernel("ARP: Gateway MAC %02x:%02x:%02x:%02x:%02x:%02x\n", (unsigned int)gateway_mac[0], (unsigned int)gateway_mac[1], (unsigned int)gateway_mac[2], (unsigned int)gateway_mac[3], (unsigned int)gateway_mac[4], (unsigned int)gateway_mac[5]);
        }

        return;
    }

    if (oper == ARP_REQUEST && is_local_ip(target_ip)) {
        uint8_t reply[ETH_ARP_FRAME_LEN] = {0};

        copy_bytes(&reply[0], sender_mac, 6);
        copy_bytes(&reply[6], our_mac, 6);

        reply[12] = 0x08;
        reply[13] = 0x06;

        uint8_t *r = &reply[ETH_HEADER_LEN];

        r[0] = 0x00;
        r[1] = ARP_HTYPE_ETHERNET;

        r[2] = 0x08;
        r[3] = 0x00;

        r[4] = 6;
        r[5] = 4;

        r[6] = 0x00;
        r[7] = ARP_REPLY;

        copy_bytes(&r[8], our_mac, 6);
        copy_bytes(&r[14], our_ip, 4);

        copy_bytes(&r[18], sender_mac, 6);
        copy_bytes(&r[24], sender_ip, 4);

        if (!e1000_send(reply, sizeof(reply))) {
            log_error("ARP: Failed to send reply\n");
        } else {
            log_kernel("ARP: Reply sent\n");
        }

        (void)target_mac;
    }
}

void arp_init(void) {
    gateway_mac_valid = 0;

    const struct ipv4_config *cfg = net_config_get();

    log_kernel("ARP: Local IP %u.%u.%u.%u\n", (unsigned int)cfg->ip[0], (unsigned int)cfg->ip[1], (unsigned int)cfg->ip[2], (unsigned int)cfg->ip[3]);

    log_kernel("ARP: Gateway %u.%u.%u.%u\n", (unsigned int)cfg->gateway[0], (unsigned int)cfg->gateway[1], (unsigned int)cfg->gateway[2], (unsigned int)cfg->gateway[3]);
}

int arp_gateway_ready(void) {
    return gateway_mac_valid;
}

const uint8_t *arp_get_gateway_mac(void) {
    return gateway_mac_valid ? gateway_mac : 0;
}
