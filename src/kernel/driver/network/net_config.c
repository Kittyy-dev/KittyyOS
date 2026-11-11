#include <stdint.h>
#include <net_config.h>

static struct ipv4_config config = {
    .ip = {10, 0, 2, 15},
    .netmask = {255, 255, 255, 0},
    .gateway = {10, 0, 2, 2},
    .dns = {10, 0, 2, 3}
};

void net_config_init(void) {

}

void net_config_set(const struct ipv4_config *new_config) {
    if (!new_config) {
        return;
    }

    config = *new_config;
}

const struct ipv4_config *net_config_get(void) {
    return &config;
}