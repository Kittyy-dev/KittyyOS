#include <fat.h>
#include <cd.h>
#include <vga.h>
#include <string.h>
#include <kprint.h>

static uint32_t current_dir_cluster = 0;
static uint32_t parent_dir_cluster = 0;

static char current_dir_name[64] = "/";

uint32_t fat32_get_current_dir(void) {
    return current_dir_cluster;
}

void fat32_set_current_dir(uint32_t cluster) {
    current_dir_cluster = cluster;
}

void fat32_get_current_dir_name(char* out) {
    strcpy(out, current_dir_name);
}

void fat32_cd(const char* path) {
    if (!path || path[0] == 0) {
        kprintf("cd: missing operand\n");
        return;
    }

    if (strcmp(path, ".") == 0)
        return;

    if (strcmp(path, "..") == 0) {
        current_dir_cluster = parent_dir_cluster;
        strcpy(current_dir_name, "/");
        return;
    }

    uint32_t new_cluster;

    if (!fat32_resolve_dir(path, &new_cluster)) {
        kprintf("cd: %s: No such directory\n", path);
        return;
    }

    parent_dir_cluster = current_dir_cluster;
    current_dir_cluster = new_cluster;

    char parts[16][64];
    int count = split_path(path, parts, 16);
    strcpy(current_dir_name, parts[count - 1]);
}
