#ifndef BOOTINFO_H
#define BOOTINFO_H

#include <stdint.h>

#define MAX_MEMORY_ENTRIES 128
#define BOOT_TYPE_ADDR 0x4FFC
#define BOOT_TYPE_BIOS 1
#define BOOT_TYPE_UEFI 2

typedef struct {
    uint32_t type;
    uint64_t physical_state;
    uint64_t number_of_pages;
    uint64_t attribute;
} BootMemoryEntry;

typedef struct {
    uint32_t video_mode;
    uint32_t width;
    uint32_t height;
    uint32_t pitch;
    uint64_t framebuffer;
    uint32_t bpp;

    uint32_t memory_entry_count;

    BootMemoryEntry memory_map[MAX_MEMORY_ENTRIES];
} BootInfo;

#endif