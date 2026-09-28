#include <vfs.h>
#include <ramfs.h>

static void* ramfs_mount() {
    return (vfs_node_t*)ramfs_root;
}

static vfs_node_t* ramfs_create_file(vfs_node_t* dir, const char* name) {
    return (vfs_node_t*)ramfs_create((ramfs_node_t*)dir, name, false);
}

static vfs_node_t* ramfs_create_dir(vfs_node_t* dir, const char* name) {
    return (vfs_node_t*)ramfs_create((ramfs_node_t*)dir, name, true);
}

static size_t ramfs_read_file(vfs_node_t* node, void* buf, size_t len) {
    return ramfs_read((ramfs_node_t*)node, buf, len);
}

static size_t ramfs_write_file(vfs_node_t* node, const void* buf, size_t len) {
    return ramfs_write((ramfs_node_t*)node, buf, len);
}

static void ramfs_delete_node(vfs_node_t* node) {
    ramfs_delete((ramfs_node_t*)node);
}

filesystem_t ramfs_fs = {
    .name = "ramfs",
    .mount = ramfs_mount,
    .create_file = ramfs_create_file,
    .create_dir = ramfs_create_dir,
    .read = ramfs_read_file,
    .write = ramfs_write_file,
    .delete = ramfs_delete_node,
};
