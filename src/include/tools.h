#include <stdint.h>
#include <fat.h>
#include <path.h>

void fat32_ls(void);
void fat32_show(const char* path);
bool kernel_load_module(const char* path);