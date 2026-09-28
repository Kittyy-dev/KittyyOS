#include <stdint.h>
#include <fat.h>
#include <path.h>
#include <string.h>
#include <disk.h>

int split_path(const char* path, char parts[][64], int max_parts) {
    int count = 0;
    int p = 0;

    for (int i = 0; path[i] != 0; i++) {
        if (path[i] == '/') {
            if (p > 0) {
                parts[count][p] = 0;
                count++;
                p = 0;
                if (count >= max_parts) {
                    return count;
                }
            }
        } else {
            parts[count][p++] = path[i];
            if (p >= 63) {
                parts[count][63] = 0;
                count++;
                p = 0;
            }
        }
    }

    if (p > 0) {
        parts[count][p] = 0;
        count++;
    }

    return count;
}

int fat32_find_entry_index(uint32_t dir_cluster, const char* name, FAT32_DirEntry* out) {
    char target[256];

    strcpy(target, name);

    fat32_to_upper(target);

    uint32_t lba = fat32_cluster_to_lba(dir_cluster);
    uint8_t sector[512];

    for (int s = 0; s < bpb.sectorsPerCluster; s++) {
        read_sectors_abs(lba + s, sector, 1);

        FAT32_DirEntry* e = (FAT32_DirEntry*)sector;

        char lfn[256];

        lfn[0] = 0;

        for (int i = 0; i < 16; i++) {
            if (e[i].name[0] == 0x00) {
                return -1;
            }

            if (e[i].name[0] == 0xE5) {
                continue;
            }

            if (e[i].attr == 0x0F) {
                continue;
            }

            char shortname[13];
            int p = 0;

            for (int k = 0; k < 8; k++) {
                if (e[i].name[k] == ' ') {
                    break;
                }

                shortname[p++] = e[i].name[k];
            }

            int has_ext = 0;

            for (int k = 8; k < 11; k++) {
                if (e[i].name[k] != ' ') {
                    has_ext = 1;
                    break;
                }
            }

            if (has_ext) {
                shortname[p++] = '.';

                for (int k = 8; k < 11; k++) {
                    if (e[i].name[k] == ' ') {
                        break;
                    }
                    shortname[p++] = e[i].name[k];
                }
            }

            shortname[p] = 0;

            fat32_to_upper(shortname);

            if (strcmp(shortname, target) == 0) {
                *out = e[i];
                
                return i;   
            }
        }
    }

    return -1;
}

bool is_boot_path(const char* path) {
    return strncmp(path, "/boot/", 6) == 0;
}