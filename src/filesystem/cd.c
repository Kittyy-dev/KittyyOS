#include <fat.h>
#include <cd.h>
#include <vga.h>
#include <string.h>

static uint32_t current_dir_cluster = 0;
static uint32_t parent_dir_cluster = 0;

static char current_dir_name[64] = "/";

uint32_t fat32_get_current_dir(void) {
    return current_dir_cluster;
}

void fat32_set_current_dir(uint32_t cluster) {
    current_dir_cluster = cluster;
}

// Header-kompatibel: schreibt den Namen in den Buffer
void fat32_get_current_dir_name(char* out) {
    strcpy(out, current_dir_name);
}

void fat32_cd(const char* name) {
    if (!name || name[0] == 0) {
        vga_print("cd: missing operand\n");
        return;
    }

    vga_printf("CD DEBUG: cur=%d, name='%s'\n", (int)current_dir_cluster, name);

    // stay in same dir
    if (strcmp(name, ".") == 0)
        return;

    // go to parent
    if (strcmp(name, "..") == 0) {
        if (current_dir_cluster != bpb.rootCluster) {
            current_dir_cluster = parent_dir_cluster;
            parent_dir_cluster = bpb.rootCluster;
            strcpy(current_dir_name, "/");   // Name zurücksetzen
        }
        return;
    }

    // find entry
    FAT32_DirEntry entry;
    if (!fat32_find_entry(current_dir_cluster, name, &entry)) {
        vga_printf("cd: %s: No such file or directory\n", name);
        return;
    }

    // must be directory
    if (!(entry.attr & 0x10)) {
        vga_printf("cd: %s: Not a directory\n", name);
        return;
    }

    // compute new cluster
    uint32_t new_cluster = (entry.firstClusterHigh << 16) | entry.firstClusterLow;

    // save parent before switching
    parent_dir_cluster = current_dir_cluster;

    // switch directory
    current_dir_cluster = new_cluster;

    // Ordnername für Prompt speichern
    strcpy(current_dir_name, name);
}
