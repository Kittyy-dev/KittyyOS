#ifndef CD_H
#define CD_H

#include <stdint.h>

void fat32_cd(const char* name);
uint32_t fat32_get_current_dir(void);
void fat32_set_current_dir(uint32_t cluster);

#endif