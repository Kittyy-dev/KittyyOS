#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <vfs.h>
#include <kprint.h>
#include <colors.h>
#include <stdlib.h>
#include <string.h>
#include <devfs.h>
#include <keyboard.h>
#include <tty.h>
#include <devfs.h>
#include <ahci.h>
#include <storage.h>
#include <ata.h>


// static vfs_node_t* devfs_root = NULL;

static devfs_state_t devfs_state;

static void *devfs_mount(void) {
    return &devfs_state;
}

static void devfs_load_children(vfs_node_t *node) {
    (void)node;
}

static vfs_node_t *devfs_create_file(vfs_node_t* dir, const char* name) {
    if (!dir || !dir->is_dir || !name) {
        return NULL;
    }

    vfs_node_t *node = malloc(sizeof(vfs_node_t));

    if (!node) {
        return NULL;
    }

    memset(node, 0, sizeof(vfs_node_t));

    node->name = strdup(name);

    if (!node->name) {
        free(node);
        return NULL;
    }

    node->is_dir = false;
    node->parent = dir;
    node->fs = &devfs;

    node->next = dir->children;
    dir->children = node;

    return node;
}

static vfs_node_t *devfs_create_dir(vfs_node_t *dir, const char *name) {
    if (!dir || !dir->is_dir || !name) {
        return NULL;
    }

    vfs_node_t *node = malloc(sizeof(vfs_node_t));

    if (!node) {
        return NULL;
    }

    memset(node, 0, sizeof(vfs_node_t));

    node->name = strdup(name);

    if (!node->name) {
        free(node);
        return NULL;
    }

    node->is_dir = true;
    node->parent = dir;
    node->fs = &devfs;

    node->next = dir->children;
    dir->children = node;

    return node;
}

static size_t devfs_read(vfs_node_t *node, void *buf, size_t len) {
    if (!node || !node->internal) {
        return 0;
    }

    devfs_device_t *device = (devfs_device_t*)node->internal;

    if (!device->read) {
        return 0;
    }

    return device->read(node, buf, len);
}

static size_t devfs_write(vfs_node_t *node, const void *buf, size_t len) {
    if (!node || !node->internal) {
        return 0;
    }

    devfs_device_t *device = (devfs_device_t*)node->internal;

    if (!device->write) {
        return 0;
    }

    return device->write(node, buf, len);
}

static void devfs_delete(vfs_node_t *node) {
    if (!node) {
        return;
    }


}

static size_t devnull_read(vfs_node_t *node, void *buf, size_t len) {
    (void)node;
    (void)buf;
    (void)len;

    return 0;
}

static size_t devnull_write(vfs_node_t *node, const void *buf, size_t len) {
    (void)node;
    (void)buf;

    return len;
}

static size_t devzero_read(vfs_node_t *node, void *buf, size_t len) {
    (void)node;

    if (!buf) {
        return 0;
    }

    memset(buf, 0, len);
    return len;
}

static size_t devzero_write(vfs_node_t *node, const void *buf, size_t len) {
    (void)node;
    (void)buf;

    return len;
}

vfs_node_t *devfs_register_device(const char *name, size_t (*read)(vfs_node_t *node, void *buf, size_t len), size_t (*write)(vfs_node_t *node, const void *buf, size_t len), void *private_data) {
    if (!name) {
        return NULL;
    }

    vfs_node_t *root = vfs_resolve_path("/dev");

    if (!root) {
        return NULL;
    }

    if (root->fs != &devfs) {
        return NULL;
    }

    vfs_node_t *node = devfs_create_file(root, name);

    if (!node) {
        return NULL;
    }

    devfs_device_t *device = malloc(sizeof(devfs_device_t));

    if (!device) {
        free(node->name);
        free(node);
        return NULL;
    }

    device->read = read;
    device->write = write;

    node->internal = device;

    return node;
}

void devfs_init(void) {
    memset(&devfs_state, 0, sizeof(devfs_state));

    devfs_register_device("null", devnull_read, devnull_write, NULL);
    devfs_register_device("zero", devzero_read, devzero_write, NULL);

    // devfs_register_device("sda", devsda_read, devsda_write, NULL);

    tty_init();

    if (g_use_ahci == 1) {
        devfs_register_device("sda", devsda_read, devsda_write, NULL);
    }

    if (g_use_ata == 1) {
        devfs_register_device("hda", devhda_read, devhda_write, NULL);
    }

    // devfs_register_device("tty0", tty_read, tty_write, &ttys[0]);
    // devfs_register_device("tty", tty_read, tty_write);
}

static void devfs_umount(void *internal) {
    (void)internal;

    memset(&devfs_state, 0, sizeof(devfs_state));
}

filesystem_t devfs = {
    .name = "devfs",

    .mount = devfs_mount,
    .umount = devfs_umount,
    .load_children = devfs_load_children,

    .create_file = devfs_create_file,
    .create_dir = devfs_create_dir,

    .read = devfs_read,
    .write = devfs_write,

    .delete = devfs_delete
};