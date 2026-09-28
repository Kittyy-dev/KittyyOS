#include <vfs.h>
#include <string.h>
#include <stdlib.h>
#include <colors.h>
#include <kprint.h>
#include <fat.h>


extern filesystem_t fat32_fs;

static filesystem_t* filesystems[MAX_FS];
static int fs_count = 0;

vfs_node_t* vfs_root = NULL;

void vfs_register(filesystem_t* fs) {
    if (fs_count < MAX_FS) {
        filesystems[fs_count++] = fs;
    }
}

static vfs_node_t* vfs_find_child(vfs_node_t* dir, const char* name) {
    if (!dir) {
        return NULL;
    }

    vfs_node_t* child = dir->children;

    while (child) {
        if (strcmp(child->name, name) == 0) {
            return child;
        }

        child = child->next;
    }

    return NULL;
}

static void vfs_load_children(vfs_node_t* node) {
    if (!node) {
        return;
    }

    if (!node->is_dir) {
        return;
    }

    if (node->fs != &fat32_fs) {
        return;
    }

    if (node->children != NULL) {
        return;
    }

    fat32_populate_directory(node, node->cluster);
}

vfs_node_t* vfs_resolve_path(const char* path) {
    if (!path || path[0] != '/') {
        return NULL;
    }

    if (strcmp(path, "/") == 0) {
        return vfs_root;
    }

    char* copy = strdup(path);

    if (!copy) {
        return NULL;
    }

    vfs_node_t* current = vfs_root;

    char* token = strtok(copy, "/");

    while (token) {
        vfs_load_children(current);

        current = vfs_find_child(current, token);

        if (!current) {
            free(copy);
            return NULL;
        }

        token = strtok(NULL, "/");
    }

    free(copy);
    vfs_load_children(current);

    return current;
}

vfs_node_t* vfs_mount(const char* path, const char* fsname) {
    filesystem_t* fs = NULL;

    for (int i = 0; i < fs_count; i++) {
        if (strcmp(filesystems[i]->name, fsname) == 0) {
            fs = filesystems[i];
            break;
        }
    }

    if (!fs) {
        return NULL;
    }

    if (strcmp(path, "/") == 0) {
        vfs_root = fs->mount();

        if (!vfs_root) {
            return NULL;
        }

        return vfs_root;
    }

    vfs_node_t* node = vfs_resolve_path(path);

    if (!node) {
        return NULL;
    }

    vfs_node_t *mounted = fs->mount();

    if (!mounted) {
        return NULL;
    }

    node->fs = fs;
    node->internal = mounted;

    return node;
}

vfs_node_t* vfs_create_file(vfs_node_t* dir, const char* name) {
    return dir->fs->create_file(dir, name);
}

vfs_node_t* vfs_create_dir(vfs_node_t* dir, const char* name) {
    return dir->fs->create_dir(dir, name);
}

size_t vfs_read(vfs_node_t* node, void* buf, size_t len) {
    return node->fs->read(node, buf, len);
}

size_t vfs_write(vfs_node_t* node, const void* buf, size_t len) {
    return node->fs->write(node, buf, len);
}

void vfs_delete(vfs_node_t* node) {
    node->fs->delete(node);
}

vfs_node_t* vfs_mkdir(vfs_node_t* parent, const char* name) {
    vfs_node_t* node = malloc(sizeof(vfs_node_t));
    memset(node, 0, sizeof(vfs_node_t));

    node->name = strdup(name);
    node->is_dir = true;
    node->parent = parent;
    node->next = parent->children;
    parent->children = node;

    return node;
}

vfs_node_t* vfs_get_root(void) {
    return vfs_root;
}

void vfs_ls(vfs_node_t* dir) {
    if (!dir || !dir->is_dir) {
        return;
    }

    vfs_node_t* child = dir->children;

    while (child) {
        if (child->is_dir) {
            kprintf("%s/\n", child->name);
        } else {
            kprintf("%s\n", child->name);
        }

        child = child->next;
    }
}

static void vfs_umount_node(vfs_node_t *node) {
    if (!node) {
        return;
    }

    vfs_node_t *child = node->children;

    while (child) {
        vfs_node_t *next = child->next;

        vfs_umount_node(child);

        child = next;
    }

    if (node->fs && node->internal && node->fs->umount) {
        node->fs->umount(node->internal);
        node->internal = NULL;
    }

    node->fs = NULL;
}

void vfs_umount_all(void) {
    if (!vfs_root) {
        return;
    }

    vfs_umount_node(vfs_root);

    vfs_root = NULL;
}

static vfs_node_t* vfs_cwd = NULL;

vfs_node_t* vfs_get_cwd(void) {
    if (!vfs_cwd) {
        vfs_cwd = vfs_root;
    }

    return vfs_cwd;
}

bool vfs_cd(const char* path) {
    if (!path || !path[0]) {
        return false;
    }

    vfs_node_t* current;

    if (path[0] == '/') {
        current = vfs_root;
    } else {
        current = vfs_get_cwd();
    }

    char* copy = strdup(path);
    if (!copy) {
        return false;
    }

    char* token = strtok(copy, "/");

    while (token) {
        if (strcmp(token, ".") == 0) {
            // aktuelles Verzeichnis
        }
        else if (strcmp(token, "..") == 0) {
            if (current->parent) {
                current = current->parent;
            }
        }
        else {
            vfs_load_children(current);

            vfs_node_t* child = vfs_find_child(current, token);

            if (!child || !child->is_dir) {
                free(copy);
                return false;
            }

            current = child;
        }

        token = strtok(NULL, "/");
    }

    free(copy);

    vfs_cwd = current;
    return true;
}