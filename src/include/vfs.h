#ifndef VFS_H
#define VFS_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#define MAX_FS 8

typedef struct vfs_node {
    char* name;
    bool is_dir;

    struct vfs_node* parent;
    struct vfs_node* children;
    struct vfs_node* next;

    uint32_t cluster;   
    size_t   size;

    struct filesystem* fs;

    void* internal;
} vfs_node_t;

typedef struct filesystem {
    const char* name;

    void* (*mount)(void);
    void (*umount)(void *internal);
    void (*load_children)(vfs_node_t *node);

    vfs_node_t* (*create_file)(vfs_node_t* dir, const char* name);
    vfs_node_t* (*create_dir)(vfs_node_t* dir, const char* name);

    size_t (*read)(vfs_node_t* node, void* buf, size_t len);
    size_t (*write)(vfs_node_t* node, const void* buf, size_t len);

    void (*delete)(vfs_node_t* node);
} filesystem_t;

void vfs_register(filesystem_t* fs);
vfs_node_t* vfs_mount(const char* path, const char* fsname);

vfs_node_t* vfs_create_file(vfs_node_t* dir, const char* name);
vfs_node_t* vfs_create_dir(vfs_node_t* dir, const char* name);

size_t vfs_read(vfs_node_t* node, void* buf, size_t len);
size_t vfs_write(vfs_node_t* node, const void* buf, size_t len);

void vfs_delete(vfs_node_t* node);
vfs_node_t* vfs_mkdir(vfs_node_t* parent, const char* name);
vfs_node_t* vfs_get_root(void);
vfs_node_t* vfs_resolve_path(const char* path);
void vfs_ls(vfs_node_t* dir);
void vfs_umount_all(void);
bool vfs_cd(const char* path);
vfs_node_t* vfs_get_cwd(void);

extern vfs_node_t* vfs_root;

#endif
