#include <heap.h>
#include "kernel_api.h"
#include <stdint.h>
#include <fat.h>
#include <ports.h>
#include <stdlib.h>
#include <string.h>
#include <vga.h>
#include <colors.h>
#include <ata.h>
#include <cd.h>
#include <disk.h>
#include <tsc.h>
#include <path.h>
#include <tools.h>
#include <kprint.h>
#include <elf.h>

void fat32_ls(void) {
    uint32_t cluster = fat32_get_current_dir();

    while (cluster < 0x0FFFFFF8) {
        uint32_t lba = fat32_cluster_to_lba(cluster);
        uint8_t sector[512];

        for (int s = 0; s < bpb.sectorsPerCluster; s++) {
            read_sectors_abs(lba + s, sector, 1);

            FAT32_DirEntry* e = (FAT32_DirEntry*)sector;

            char lfn[256];
            lfn[0] = 0;

            for (int i = 0; i < 16; i++) {
                // Empty
                if (e[i].name[0] == 0x00) {
                    continue;
                }

                // Deleted
                if (e[i].name[0] == 0xE5) {
                    continue;
                }

                // LFN-Entry
                if (e[i].attr == 0x0F) {
                    FAT32_LFN_Entry* l = (FAT32_LFN_Entry*)&e[i];

                    char temp[14];
                    int p = 0;

                    // Unicode -> ASCII
                    for (int k = 0; k < 5; k++) {
                        uint16_t c = l->name1[k];

                        if (c == 0xFFFF || c == 0x0000) {
                            break;
                        }

                        temp[p++] = (char)c;
                    }
                    
                    for (int k = 0; k < 6; k++) {
                        uint16_t c = l->name2[k];

                        if (c == 0xFFFF || c == 0x0000) {
                            break;
                        }

                        temp[p++] = (char)c;
                    }

                    for (int k = 0; k < 2; k++) {
                        uint16_t c = l->name3[k];
                        if (c == 0xFFFF || c == 0x0000) {
                            break;
                        }
                        temp[p++] = (char)c;
                    }

                    temp[p] = 0;

                    // LFN
                    char new_lfn[256];
                    strcpy(new_lfn, temp);
                    strcat(new_lfn, lfn);
                    strcpy(lfn, new_lfn);

                    continue;
                }

                // Ignore Volume Label
                if (e[i].attr & 0x08) {
                    continue;
                }

                if (lfn[0] != 0) {
                    if (e[i].attr & 0x10) {
                        kprintf("::%s\n", lfn);
                    } else {
                        kprintf("%s\n", lfn);
                    }

                    lfn[0] = 0;
                    continue;
                }

                // No LFN -> 8.3 Name
                char name[13];
                int p = 0;

                for (int k = 0; k < 8; k++) {
                    if (e[i].name[k] == ' ') {
                        break;
                    }

                    name[p++] = e[i].name[k];
                }

                int has_ext = 0;

                for (int k = 8; k < 11; k++) {
                    if (e[i].name[k] != ' ') {
                        has_ext = 1;
                        break;
                    }
                }

                if (has_ext) {
                    name[p++] = '.';

                    for (int k = 8; k < 11; k++) {
                        if (e[i].name[k] == ' ') {
                            break;
                        }

                        name[p++] = e[i].name[k];
                    }
                }

                name[p] = 0;

                if (e[i].attr & 0x10) {
                    kprintf("::%s\n", name);
                } else {
                    kprintf("%s\n", name);
                }
            }
        }

        cluster = get_next_cluster(cluster, fat_table);
    }
}

void fat32_show(const char* path) {
    uint32_t dir_cluster;
    char filename[256];

    if (!fat32_resolve_path(path, &dir_cluster, filename)) {
        kprintf("show: '%s' not found\n", path);
        return;
    }

    FAT32_DirEntry entry;

    if (!fat32_find_entry(dir_cluster, filename, &entry)) {
        kprintf("show: file '%s' not found\n", filename);
        return;
    }

    FileHandle fh;

    fh.startCluster = ((uint32_t)entry.firstClusterHigh << 16) | entry.firstClusterLow;
    fh.size = entry.fileSize;

    uint8_t* buf = malloc(fh.size + 1);
    if (!buf) {
        return;
    }

    int r = read_file(fh, &bpb, fat_table, buf);

    if (r <= 0) {
        free(buf);
        return;
    }

    buf[fh.size] = 0;

    kprintf("%s", buf);

    free(buf);
}

bool kernel_load_module(const char* path) {
    kprintf(WHITE "<" GREEN " KERNEL " WHITE "> " "Loading module %s\n", path);

    LoadedFile file = fat32_load_file(path);

    if (!file.data) {
        kprintf(WHITE "<" RED " ERROR " WHITE "> " "Failed to load module %s!\n", path);
        return false;
    }

    extern KernelAPI kernel_api;

    void *entry_addr = elf_load(file.data);

    if (!entry_addr) {
        kprintf(WHITE "<" RED " ERROR " WHITE "> ELF load failed\n");
        free(file.data);
        return false;
    }

    void (*entry)(KernelAPI*) = (void(*)(KernelAPI*))entry_addr;

    entry(&kernel_api);

    kprintf(WHITE  "<" RED " ERROR " WHITE "> " "Module %s returned\n", path);

    free(file.data);

    return true;
}