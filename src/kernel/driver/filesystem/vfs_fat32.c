#include <disk.h>
#include <heap.h>
#include <vfs.h>
#include <fat.h>
#include <string.h>
#include <stdint.h>
#include <pmm.h>
#include <stdlib.h>
#include <fat.h>

extern FAT32_BootSector bpb;
extern uint32_t* fat_table;

vfs_node_t* fat32_make_node(vfs_node_t* parent, const char* name, bool is_dir, uint32_t cluster, uint32_t size) {
    vfs_node_t* node = kmalloc(sizeof(vfs_node_t));

    if (!node) {
        return NULL;
    }

    memset(node, 0, sizeof(vfs_node_t));

    node->name = strdup(name);

    if (!node->name) {
        free(node);
        return NULL;
    }

    node->is_dir = is_dir;
    node->parent = parent;
    node->fs = &fat32_fs;
    node->cluster = cluster;
    node->size = size;

    node->next = parent->children;
    parent->children = node;

    return node;
}

void fat32_populate_directory(vfs_node_t* parent, uint32_t cluster) {
    uint32_t fds = bpb.reservedSectors + (bpb.numFATs * bpb.FATSize32);

    int safety = 0;

    while (cluster < 0x0FFFFFF8 && safety < 32) {

        uint32_t rel_sector = fds + ((cluster - 2) * bpb.sectorsPerCluster);

        uint32_t abs_sector = g_part_lba_start + rel_sector;

        read_sectors_abs(abs_sector, dir_buf, (uint8_t)bpb.sectorsPerCluster);

        uint32_t entries = (bpb.bytesPerSector * bpb.sectorsPerCluster) / sizeof(FAT32_DirEntry);

        FAT32_DirEntry* e = (FAT32_DirEntry*)dir_buf;

        char lfn[256];

        lfn[0] = '\0';

        for (uint32_t i = 0; i < entries; i++) {
            if (e[i].name[0] == 0x00) {
                break;
            }

            if (e[i].name[0] == 0xE5) {
                lfn[0] = '\0';
                continue;
            }

            if ((e[i].attr & 0x0F) == 0x0F) {

                FAT32_LFN_Entry* l = (FAT32_LFN_Entry*)&e[i];

                char temp[14];
                int p = 0;

                for (int k = 0; k < 5; k++) {
                    uint16_t c = l->name1[k];

                    if (c == 0x0000 || c == 0xFFFF) {
                        break;
                    }

                    temp[p++] = (char)c;
                }

                for (int k = 0; k < 6; k++) {
                    uint16_t c = l->name2[k];

                    if (c == 0x0000 || c == 0xFFFF) {
                        break;
                    }

                    temp[p++] = (char)c;
                }

                for (int k = 0; k < 2; k++) {
                    uint16_t c = l->name3[k];

                    if (c == 0x0000 || c == 0xFFFF) {
                        break;
                    }

                    temp[p++] = (char)c;
                }

                temp[p] = '\0';

                char new_lfn[256];

                strcpy(new_lfn, temp);
                strcat(new_lfn, lfn);
                strcpy(lfn, new_lfn);

                continue;
            }

            if (e[i].attr & 0x08) {
                lfn[0] = '\0';
                continue;
            }

            char name[256];

            if (lfn[0] != '\0') {
                strcpy(name, lfn);

            } else {
                int p = 0;

                for (int j = 0; j < 8; j++) {
                    if (e[i].name[j] == ' ') {
                        break;
                    }

                    name[p++] = e[i].name[j];
                }

                if (e[i].name[8] != ' ') {

                    name[p++] = '.';

                    for (int j = 8; j < 11; j++) {
                        if (e[i].name[j] == ' ') {
                            break;
                        }

                        name[p++] = e[i].name[j];
                    }
                }

                name[p] = '\0';
            }

            if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0) {
                lfn[0] = '\0';
                continue;
            }

            uint32_t entry_cluster = ((uint32_t)e[i].firstClusterHigh << 16) | e[i].firstClusterLow;

            bool is_dir = (e[i].attr & 0x10) != 0;

            fat32_make_node(parent, name, is_dir, entry_cluster, e[i].fileSize);
            
            lfn[0] = '\0';
        }

        cluster = get_next_cluster(cluster, fat_table);

        safety++;
    }
}

static void* fat32_mount_root(void) {
    vfs_node_t* root = kmalloc(sizeof(vfs_node_t));

    if (!root) {
        return NULL;
    }

    memset(root, 0, sizeof(vfs_node_t));

    root->name = strdup("fat32");
    root->is_dir = true;
    root->fs = &fat32_fs;
    root->cluster = bpb.rootCluster;

    fat32_populate_directory(root, bpb.rootCluster);

    return root;
}

static vfs_node_t* fat32_create_file(vfs_node_t* dir, const char* name) {
    return NULL;
}

static vfs_node_t* fat32_create_dir(vfs_node_t* dir, const char* name) {
    return NULL;
}

static size_t fat32_read_file(vfs_node_t* node, void* buf, size_t len) {
    FileHandle fh;
    fh.startCluster = node->cluster;
    fh.size         = node->size;

    return read_file(fh, &bpb, fat_table, buf);
}

static size_t fat32_write_file(vfs_node_t* node, const void* buf, size_t len) {
    FileHandle fh;
    fh.startCluster = node->cluster;
    fh.size = node->size;

    return write_file(fh, &bpb, fat_table, buf, len);
}

static void fat32_delete_node(vfs_node_t* node) {
}

filesystem_t fat32_fs = {
    .name = "fat32",
    .mount = fat32_mount_root,
    .umount = fat32_umount,
    .create_file = fat32_create_file,
    .create_dir = fat32_create_dir,
    .read = fat32_read_file,
    .write = fat32_write_file,
    .delete = fat32_delete_node,
};
