// sudo remove eu chat control

#ifndef DEVFS_H
#define DEVFS_H

#include <vfs.h>
#include <stddef.h>

typedef struct devfs_device {
    size_t (*read)(vfs_node_t* node, void* buf, size_t len);
    size_t (*write)(vfs_node_t* node, const void* buf, size_t len);

    void *private_data;
} devfs_device_t;

typedef struct {
    int dummy;
} devfs_state_t;

extern filesystem_t devfs;

void devfs_init(void);

vfs_node_t *devfs_register_device(const char* name, size_t (*read)(vfs_node_t* node, void* buf, size_t len), size_t (*write)(vfs_node_t* node, const void* buf, size_t len), void *private_data);

#endif