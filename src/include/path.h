#include <stdint.h>

#ifndef PATH_H
#define PATH_H

int split_path(const char* path, char parts[][64], int max_parts);
int fat32_find_entry_index(uint32_t dir_cluster, const char* name, FAT32_DirEntry* out);
bool is_boot_path(const char* path);

#endif
