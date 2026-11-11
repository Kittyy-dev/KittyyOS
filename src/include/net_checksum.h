#ifndef NET_CHECKSUM_H
#define NET_CHECKSUM_H

#include <stdint.h>
#include <stddef.h>

uint16_t net_checksum(const void *data, size_t lenght);

#endif