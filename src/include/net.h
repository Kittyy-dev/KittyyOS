#pragma once

#include <stdint.h>

#define NET_IP0 10
#define NET_IP1 0
#define NET_IP2 2
#define NET_IP3 15

#define NET_GATEWAY_IP0 10
#define NET_GATEWAY_IP1 0
#define NET_GATEWAY_IP2 2
#define NET_GATEWAY_IP3 2

#define ETH_TYPE_IPV4 0x0800
#define ETH_TYPE_ARP  0x0806

#define IP_PROTO_ICMP 1
#define IP_PROTO_UDP 17

#define UDP_MAX_DATA 1472

typedef struct {
    uint64_t rx_bytes;
    uint64_t rx_packets;
    uint64_t rx_errors;
    uint64_t rx_dropped;

    uint64_t tx_bytes;
    uint64_t tx_packets;
    uint64_t tx_errors;
    uint64_t tx_dropped;
} net_stats_t;

void net_stats_get(net_stats_t *out);
void ethernet_receive(const uint8_t *packet, uint16_t length);
void arp_request(const uint8_t target_ip[4]);
void arp_init(void);
void arp_receive(const uint8_t *packet, uint16_t length);
void ipv4_receive(const uint8_t *packet, uint16_t length);
int ipv4_send(const uint8_t dst_ip[4], uint8_t protocol, const uint8_t *payload, uint16_t payload_len);
void icmp_receive(const uint8_t *packet, uint16_t length, const uint8_t src_ip[4]);
const uint8_t *arp_get_gateway_mac(void);
int arp_gateway_ready(void);
const uint8_t *arp_get_gateway_mac(void);
int icmp_ping(const uint8_t dst_ip[4]);
int udp_send(const uint8_t dst_ip[4], uint16_t src_port, uint16_t dst_port, const uint8_t *data, uint16_t length);
void udp_receive(const uint8_t *packet, uint16_t length, const uint8_t src_ip[4], const uint8_t dst_ip[4]);
int dns_resolve(const char *hostname);
void dns_receive(const uint8_t *data, uint16_t length, const uint8_t src_ip[4], uint16_t src_port, uint16_t dst_port);
int dns_get_result(uint8_t ip[4]);