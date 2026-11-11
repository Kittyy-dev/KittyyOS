#pragma once

#include <stdint.h>

#define ETH_HEADER_LEN 14
#define ARP_HEADER_LEN 28
#define ETH_ARP_FRAME_LEN 60

#define ETH_TYPE_ARP 0x0806
#define ARP_HTYPE_ETHERNET 1
#define ARP_PTYPE_IPV4 0x0800

#define ARP_REQUEST 1
#define ARP_REPLY   2

struct ipv4_config {
    uint8_t ip[4];
    uint8_t netmask[4];
    uint8_t gateway[4];
    uint8_t dns[4];
};

void net_config_init(void);
void net_config_set(const struct ipv4_config *new_config);
const struct ipv4_config *net_config_get(void);