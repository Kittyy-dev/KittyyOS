#include <stdint.h>
#include <net.h>
#include <kprint.h>

void ethernet_receive(const uint8_t *packet, uint16_t length) {
    if (!packet || length < 14) {
        return;
    }

    uint16_t ethertype = ((uint16_t)packet[12] << 8) | (uint16_t)packet[13];

    switch (ethertype) {
        case ETH_TYPE_ARP: {
            arp_receive(packet, length);
            break;
        }

        case ETH_TYPE_IPV4: {
            ipv4_receive(&packet[14], length - 14);
            break;
        }

        default: {
            break;
        }
    }
}
