#pragma once

#include <stdint.h>
#include <stddef.h>

#define NETDEV_NAME_LEN 16
#define NETDEV_MTU      1500

#define NETDEV_FLAG_UP       (1u << 0)
#define NETDEV_FLAG_LINK_UP  (1u << 1)

typedef struct netdev netdev_t;

typedef struct {
    uint64_t rx_bytes;
    uint64_t rx_packets;
    uint64_t rx_errors;
    uint64_t rx_dropped;

    uint64_t tx_bytes;
    uint64_t tx_packets;
    uint64_t tx_errors;
    uint64_t tx_dropped;
} netdev_stats_t;

struct netdev {
    char name[NETDEV_NAME_LEN];

    uint8_t mac[6];
    uint16_t mtu;
    uint32_t flags;

    void *driver_data;

    int (*send)(netdev_t *dev, const void *data, uint16_t length);
    void (*poll)(netdev_t *dev);

    netdev_stats_t stats;
    netdev_t *next;
};

int netdev_register(netdev_t *dev);
netdev_t *netdev_find(const char *name);
netdev_t *netdev_first(void);
int netdev_send(netdev_t *dev, const void *data, uint16_t length);
void netdev_poll_all(void);
void netdev_receive(netdev_t *dev, const void *data, uint16_t length);