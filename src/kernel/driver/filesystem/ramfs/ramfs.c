#include <stdint.h>
#include <ramfs.h>
#include <pmm.h>
#include <string.h>
#include <stddef.h>

ramfs_node_t* ramfs_root;

void ramfs_init() {
    ramfs_root = pmm_alloc(sizeof(ramfs_node_t));
    memset(ramfs_root, 0, sizeof(ramfs_node_t));
    ramfs_root->name = strdup("ramfs");
    ramfs_root->is_dir = true;
}

ramfs_node_t* ramfs_create(ramfs_node_t* dir, const char* name, bool is_dir) {
    ramfs_node_t* n = pmm_alloc(sizeof(ramfs_node_t));
    memset(n, 0, sizeof(ramfs_node_t));

    n->name = strdup(name);
    n->is_dir = is_dir;
    n->parent = dir;

    n->next = dir->children;
    dir->children = n;

    return n;
}

size_t ramfs_write(ramfs_node_t* file, const void* buf, size_t len) {
    if (file->is_dir) {
        return 0;
    }

    uint8_t* newbuf = pmm_alloc(len);
    memcpy(newbuf, buf, len);

    if (file->data) {
        pmm_free(file->data);
    }

    file->data = newbuf;
    file->size = len;

    return len;
}

size_t ramfs_read(ramfs_node_t* file, void* buf, size_t len) {
    if (file->is_dir) {
        return 0;
    }

    if (len > file->size) {
        len = file->size;
    }

    memcpy(buf, file->data, len);
    return len;
}

void ramfs_delete(ramfs_node_t* node) {
    ramfs_node_t** p = &node->parent->children;

    while (*p && *p != node) {
        p = &(*p)->next;
    }

    if (*p) {
        *p = node->next;
    }

    if (node->data) {
        pmm_free(node->data);
    }

    pmm_free(node->name);
    pmm_free(node);
}