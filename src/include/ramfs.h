#ifndef RAMFS_H
#define RAMFS_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <vfs.h>

typedef struct ramfs_node {
    char* name;
    bool is_dir;

    struct ramfs_node* parent;
    struct ramfs_node* children;
    struct ramfs_node* next;

    uint8_t* data;
    size_t size;
} ramfs_node_t;

void ramfs_init();
ramfs_node_t* ramfs_create(ramfs_node_t* dir, const char* name, bool is_dir);
size_t ramfs_write(ramfs_node_t* file, const void* buf, size_t len);
size_t ramfs_read(ramfs_node_t* file, void* buf, size_t len);
void ramfs_delete(ramfs_node_t* node);

extern filesystem_t ramfs_fs;

extern ramfs_node_t* ramfs_root;

#endif
