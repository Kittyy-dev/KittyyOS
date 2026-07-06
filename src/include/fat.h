#pragma once
#include <stdint.h>

// =========================
//   ATA PIO Register
// =========================
#define ATA_DATA        0x1F0
#define ATA_ERROR       0x1F1
#define ATA_SECCOUNT    0x1F2
#define ATA_LBA_LOW     0x1F3
#define ATA_LBA_MID     0x1F4
#define ATA_LBA_HIGH    0x1F5
#define ATA_DRIVE       0x1F6
#define ATA_STATUS      0x1F7
#define ATA_CMD         0x1F7

// =========================
//   ATA Commands
// =========================
#define ATA_CMD_READ    0x20
#define ATA_CMD_WRITE   0x30

// =========================
//   FAT32 Structures
// =========================
#pragma pack(push, 1)

extern uint32_t* fat_table;

typedef struct {
    uint8_t  jmpBoot[3];
    uint8_t  OEMName[8];
    uint16_t bytesPerSector;
    uint8_t  sectorsPerCluster;
    uint16_t reservedSectors;
    uint8_t  numFATs;
    uint16_t rootEntryCount;
    uint16_t totalSectors16;
    uint8_t  media;
    uint16_t FATSize16;
    uint16_t sectorsPerTrack;
    uint16_t numHeads;
    uint32_t hiddenSectors;
    uint32_t totalSectors32;

    uint32_t FATSize32;
    uint16_t extFlags;
    uint16_t FSVersion;
    uint32_t rootCluster;
    uint16_t FSInfo;
    uint16_t backupBootSector;
    uint8_t  reserved[12];
    uint8_t  driveNumber;
    uint8_t  reserved1;
    uint8_t  bootSignature;
    uint32_t volumeID;
    uint8_t  volumeLabel[11];
    uint8_t  fileSystemType[8];
} FAT32_BootSector;

typedef struct __attribute__((packed)) {
    uint8_t  name[11];
    uint8_t  attr;
    uint8_t  ntres;
    uint8_t  crtTimeTenth;
    uint16_t crtTime;
    uint16_t crtDate;
    uint16_t lastAccessDate;
    uint16_t firstClusterHigh;
    uint16_t writeTime;
    uint16_t writeDate;
    uint16_t firstClusterLow;
    uint32_t fileSize;
} FAT32_DirEntry;

typedef char static_assert_FAT32_DirEntry_size[(sizeof(FAT32_DirEntry) == 32) ? 1 : -1];

typedef struct {
    uint8_t order;
    uint16_t name1[5];
    uint8_t attr;
    uint8_t type;
    uint8_t checksum;
    uint16_t name2[6];
    uint16_t zero;
    uint16_t name3[2];
} __attribute__((packed)) FAT32_LFN_Entry;

#pragma pack(pop)

// =========================
//   File Handle
// =========================
typedef struct {
    uint32_t startCluster;
    uint32_t size;
} FileHandle;

// =========================
//   Externe Variablen
// =========================
extern FAT32_BootSector bpb;
extern uint32_t* fat_ram;
extern uint8_t* dir_buf;

// =========================
//   FAT32 API (für Syscalls)
// =========================
void fat32_mount(uint32_t part_lba_start);
void init_fat32(FAT32_BootSector* b);

FileHandle open_file(const char* filename11,
                     FAT32_BootSector* b,
                     uint32_t* fat,
                     uint8_t* dirBuffer);

int read_file(FileHandle fh,
              FAT32_BootSector* b,
              uint32_t* fat,
              uint8_t* out);

int write_file(FileHandle fh,
               FAT32_BootSector* b,
               uint32_t* fat,
               const void* data,
               uint32_t size);

// Low-level sector I/O
void fat32_ls_root(void);
void fat32_ls();

uint32_t fat32_cluster_to_lba(uint32_t cluster);
uint32_t get_next_cluster(uint32_t cluster, uint32_t* fat);
bool fat32_find_entry(uint32_t dir_cluster, const char* name, FAT32_DirEntry* out);
void fat32_get_current_dir_name(char* out);