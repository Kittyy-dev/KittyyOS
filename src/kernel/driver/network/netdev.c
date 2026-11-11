#include <netdev.h>
#include <kprint.h>
#include <stdint.h>
#include <string.h>
#include <stddef.h>
#include <e1000.h>

static netdev_t *device_list = NULL;

int netdev_register(netdev_t *dev) {
    if (!dev || !dev->name[0] || !dev->send) {
        return -1;
    }

    if (netdev_find(dev->name)) {
        return -1;
    }

    dev->next = NULL;

    if (!device_list) {
        device_list = dev;
    } else {
        netdev_t *last = device_list;

        while (last->next) {
            last = last->next;
        }

        last->next = dev;
    }

    return 0;
}

netdev_t *netdev_find(const char *name) {
    if (!name) {
        return NULL;
    }

    for (netdev_t *dev = device_list; dev; dev = dev->next) {
        if (strcmp(dev->name, name) == 0) {
            return dev;
        }
    }

    return NULL;
}

netdev_t *netdev_first(void) {
    return device_list;
}

int netdev_send(netdev_t *dev, const void *data, uint16_t length) {
    if (!dev || !data || length == 0 || !dev->send) {
        return -1;
    }

    if (!(dev->flags & NETDEV_FLAG_UP)) {
        return -1;
    }

    int result = dev->send(dev, data, length);

    if (result == 0) {
        dev->stats.tx_bytes += length;
        dev->stats.tx_packets++;
    } else {
        dev->stats.tx_errors++;
    }

    return result;
}

void netdev_poll_all(void) {
    for (netdev_t *dev = device_list; dev; dev = dev->next) {
        if (dev->poll && (dev->flags & NETDEV_FLAG_UP)) {
            dev->poll(dev);
        }
    }
}

static int e1000_netdev_send(netdev_t *dev, const void *data, uint16_t length) {
    (void)dev;
    return e1000_send(data, length);
}

static void e1000_netdev_poll(netdev_t *dev) {
    (void)dev;
    e1000_poll_rx();
}