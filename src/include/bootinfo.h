#ifndef BOOTINFO_H
#define BOOTINFO_H

#include <stdint.h>

#define MAX_MEMORY_ENTRIES 128
#define BOOT_TYPE_ADDR 0x4FFC
#define BOOT_TYPE_BIOS 1
#define BOOT_TYPE_UEFI 2
#define BOOT_INFO_ADDR 0x3000

/*
typedef struct {
    uint32_t type;
    uint64_t physical_state;
    uint64_t number_of_pages;
    uint64_t attribute;
} BootMemoryEntry; */

typedef struct __attribute__((packed)) {
    uint32_t magic;
    uint32_t version;
    uint32_t video_mode;
    uint32_t screen_width;
    uint32_t screen_height;
    uint32_t bpp;
    uint32_t framebuffer;
    uint16_t mem_entires;

    uint8_t reserved[2];

    uint8_t fb_ctrl[0x200];
    uint8_t fb_mode[0x100];
    char fb_oem[0x100];

    uint8_t mem[64 * 24];
    uint8_t drive;

    uint64_t boot_partition_lba;

    uint32_t mbr_signature;
} BootInfo;

extern BootInfo *boot_info;

void boot_info_relocate(void);

#endif