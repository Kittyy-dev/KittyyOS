#ifndef FAT12_H
#define FAT12_H

#include <stdint.h>

#define FAT12_SECTOR_SIZE 512
#define FAT12_MAX_FILENAME 11

typedef struct __attribute__((packed)) {
    uint8_t  jmp[3];
    char     oem[8];

    uint16_t bytes_per_sector;
    uint8_t  sectors_per_cluster;
    uint16_t reserved_sectors;

    uint8_t  fat_count;
    uint16_t root_entry_count;
    uint16_t total_sectors_16;

    uint8_t  media_type;
    uint16_t sectors_per_fat;

    uint16_t sectors_per_track;
    uint16_t head_count;
    uint32_t hidden_sectors;
    uint32_t total_sectors_32;

    uint8_t  rest[474];
} FAT12_BPB;

typedef struct __attribute__((packed)) {
    char filename[8];
    char extension[3];

    uint8_t attributes;
    uint8_t reserved;
    uint8_t creation_time_tenths;

    uint16_t creation_time;
    uint16_t creation_date;
    uint16_t access_date;

    uint16_t cluster_high;

    uint16_t modification_time;
    uint16_t modification_date;

    uint16_t cluster_low;
    uint32_t file_size;
} FAT12_DirEntry;

int fat12_init(void);

int fat12_read_file(const char* filename, uint8_t* buffer, uint32_t* size);

#endif