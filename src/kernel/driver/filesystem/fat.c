#include <stdbool.h>
#include <vfs.h>
#include <heap.h>

#include <stddef.h>
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
#include <kprint.h>

#define ATA_DATA     0x1F0
#define ATA_ERROR    0x1F1
#define ATA_SECCOUNT 0x1F2
#define ATA_LBA_LOW  0x1F3
#define ATA_LBA_MID  0x1F4
#define ATA_LBA_HIGH 0x1F5
#define ATA_DRIVE    0x1F6
#define ATA_STATUS   0x1F7
#define ATA_CMD      0x1F7

#define ATA_CMD_READ  0x20
#define ATA_CMD_WRITE 0x30

static uint8_t fat_dma_buffer[512] __attribute__((aligned(512)));

FAT32_BootSector bpb;

uint32_t* fat_ram = 0;
uint8_t*  dir_buf = 0;

uint32_t* fat_table;

char g_hostname[64] = "KittyyOS";

uint32_t fat32_cluster_to_lba(uint32_t cluster) {
    uint32_t firstDataSector = bpb.reservedSectors + (bpb.numFATs * bpb.FATSize32);

    uint32_t sector = firstDataSector + (cluster - 2) * bpb.sectorsPerCluster;

    return g_part_lba_start + sector;
}

void fat32_to_upper(char* s) {
    while (*s) {
        if (*s >= 'a' && *s <= 'z')
            *s = *s - 32;
        s++;
    }
}

static inline uint32_t first_data_sector(const FAT32_BootSector* b) {
    return b->reservedSectors + (b->numFATs * b->FATSize32);
}

inline uint32_t get_next_cluster(uint32_t cluster, uint32_t* fat) {
    return fat[cluster] & 0x0FFFFFFF;
}

static inline uint32_t find_free_cluster(uint32_t* fat) {
    for (uint32_t c = 2; c < 0x0FFFFFF7; c++) {
        if ((fat[c] & 0x0FFFFFFF) == 0) return c;
    }
    return 0x0FFFFFFF;
}

FileHandle open_file(const char* filename11, FAT32_BootSector* b, uint32_t* fat, uint8_t* dirBuffer) {
    uint32_t cluster = b->rootCluster;
    uint32_t fds = first_data_sector(b);

    while (cluster < 0x0FFFFFF8) {
        uint32_t sector = fds + ((cluster - 2) * b->sectorsPerCluster);

        for (uint32_t i = 0; i < b->sectorsPerCluster; i++) {
            read_sectors(sector + i, dirBuffer + i * b->bytesPerSector);
        }

        uint32_t bytesThisCluster = b->bytesPerSector * b->sectorsPerCluster;
        uint32_t numEntries = bytesThisCluster / sizeof(FAT32_DirEntry);
        FAT32_DirEntry* entries = (FAT32_DirEntry*)dirBuffer;

        for (uint32_t i = 0; i < numEntries; i++) {
            if (entries[i].name[0] == 0x00) break;
            if ((entries[i].attr & 0x0F) == 0x0F) continue;

            if (memcmp(entries[i].name, filename11, 11) == 0) {
                FileHandle fh;
                fh.startCluster =
                    ((uint32_t)entries[i].firstClusterHigh << 16) |
                     entries[i].firstClusterLow;
                fh.size = entries[i].fileSize;
                return fh;
            }
        }

        cluster = get_next_cluster(cluster, fat);
    }

    return (FileHandle){0, 0};
}

int read_file(FileHandle fh, FAT32_BootSector* b, uint32_t* fat, uint8_t* out) {
    uint32_t cluster = fh.startCluster;
    uint8_t*  ptr = out;
    uint32_t  remaining = fh.size;
    uint32_t  fds = first_data_sector(b);

    while (cluster < 0x0FFFFFF8 && remaining > 0) {
        uint32_t sector = fds + ((cluster - 2) * b->sectorsPerCluster);

        for (uint32_t i = 0; i < b->sectorsPerCluster && remaining > 0; i++) {
            read_sectors(sector + i, ptr);
            uint32_t step = b->bytesPerSector;
            ptr += step;
            if (remaining > step) remaining -= step;
            else remaining = 0;
        }

        cluster = get_next_cluster(cluster, fat);
    }

    return fh.size - remaining;
}

int write_file(FileHandle fh, FAT32_BootSector* b, uint32_t* fat, const void* data, uint32_t size) {
    if (!data || size == 0) {
        return 0;
    }

    if (size > fh.size) {
        size = fh.size;
    }

    uint32_t cluster = fh.startCluster;
    uint32_t written = 0;
    uint32_t fds = first_data_sector(b);

    const uint8_t* src = data;

    uint8_t sector_buf[512];

    while (size > 0 && cluster < 0x0FFFFFF8) {
        uint32_t sector = fds + ((cluster - 2) * b->sectorsPerCluster);

        for (uint32_t i = 0; i < b->sectorsPerCluster && size > 0; i++) {
            uint32_t amount = size > 512 ? 512 : size;

            if (amount < 512) {
                read_sectors_abs(sector + i, sector_buf, 1);
                memcpy(sector_buf, src + written, amount);
                write_sectors_abs(sector + i, sector_buf, 1);
            } else {
                write_sectors_abs(sector + i, src + written, 1);
            }

            written += amount;
            size -= amount;
        }

        if (size > 0) {
            break;
        }

        cluster = get_next_cluster(cluster, fat);
    }

    return written;
}

size_t fat32_write(vfs_node_t *node, const void *buf, size_t len) {
    if (!node || !buf) {
        return 0;
    }

    if (node->is_dir) {
        return 0;
    }

    FileHandle fh;
    fh.startCluster = node->cluster;
    fh.size = node->size;

    int written = write_file(fh, &bpb, fat_table, buf, len);

    if (written > 0) {
        node->size = written;
    }

    return written;
}

void init_fat32(FAT32_BootSector* b) {

    uint32_t fat_size_bytes = b->FATSize32 * b->bytesPerSector;
    fat_ram = malloc(fat_size_bytes);
    if (!fat_ram) {
        kprintf(WHITE "<" RED " ERROR " WHITE "> " "Malloc for FAT failed\n");
        return;
    }

    uint32_t fat_start_lba = g_part_lba_start + b->reservedSectors;
    uint32_t remaining = b->FATSize32;
    uint32_t lba = fat_start_lba;
    uint8_t* dst = (uint8_t*)fat_ram;

    const uint32_t CHUNK = 128;

    while (remaining > 0) {
        uint8_t chunk = remaining > 128 ? 128 : remaining;

        read_sectors_abs(lba, dst, chunk);

        lba += chunk;
        dst += chunk * 512;
        remaining -= chunk;
    }

    dir_buf = malloc(b->sectorsPerCluster * b->bytesPerSector);
    if (!dir_buf) {
        kprintf(WHITE "<" RED " ERROR " WHITE "> " "Malloc or Dir Buf failed!\n");
        return;
    }

    fat_table = malloc(bpb.FATSize32 * bpb.bytesPerSector);
    read_sectors_abs(g_part_lba_start + bpb.reservedSectors, (uint8_t*)fat_table, bpb.FATSize32);

    kprintf(WHITE "<" GREEN " KERNEL " WHITE "> " "Fat32 initialized!\n");
}

void fat32_ls_root(void) {
    uint32_t cluster = bpb.rootCluster;
    uint32_t fds = bpb.reservedSectors + (bpb.numFATs * bpb.FATSize32);

    int safety = 0;

    while (cluster < 0x0FFFFFF8 && safety < 32) {
        kprintf("ls: cluster=%x\n", cluster);

        uint32_t rel_sector = fds + ((cluster - 2) * bpb.sectorsPerCluster);
        uint32_t abs_sector = g_part_lba_start + rel_sector;

        read_sectors_abs(abs_sector, dir_buf, (uint8_t)bpb.sectorsPerCluster);

        uint32_t entries = (bpb.bytesPerSector * bpb.sectorsPerCluster) / sizeof(FAT32_DirEntry);
        FAT32_DirEntry* e = (FAT32_DirEntry*)dir_buf;

        for (uint32_t i = 0; i < entries; i++) {
            if (e[i].name[0] == 0x00) break;
            if ((e[i].attr & 0x0F) == 0x0F) continue;
            if (e[i].name[0] == 0xE5) continue;

            char name[13];
            memcpy(name, e[i].name, 8);
            name[8] = '\0';
            for (int j = 7; j >= 0 && name[j] == ' '; j--) name[j] = '\0';

            if (e[i].name[8] != ' ') {
                int len = strlen(name);
                name[len] = '.';
                memcpy(name + len + 1, e[i].name + 8, 3);
                name[len + 4] = '\0';
                for (int j = len + 3; j >= 0 && name[j] == ' '; j--) name[j] = '\0';
            }

            kprintf("%s  0x%x bytes\n", name, e[i].fileSize);
        }

        cluster = get_next_cluster(cluster, fat_ram);
        safety++;
    }

    kprintf("\n");
}

bool fat32_resolve_path(const char* path, uint32_t* out_dir_cluster, char* out_filename) {
    char parts[16][64];
    int count = split_path(path, parts, 16);

    if (count == 0)
        return false;

    uint32_t cluster = (path[0] == '/') ? bpb.rootCluster : fat32_get_current_dir();
    FAT32_DirEntry entry;

    for (int i = 0; i < count; i++) {
        char upper[64];
        strcpy(upper, parts[i]);
        fat32_to_upper(upper);

        if (!fat32_find_entry(cluster, upper, &entry)) {
            return false;
        }

        if (entry.attr & 0x10) {
            cluster = ((uint32_t)entry.firstClusterHigh << 16) | entry.firstClusterLow;
        } else {
            strcpy(out_filename, parts[i]);
            *out_dir_cluster = cluster;
            return true;
        }
    }

    return false;
}

bool fat32_resolve_path_file(const char* path, uint32_t* out_dir_cluster, char* out_filename, FAT32_DirEntry* out_entry, int* out_index) {
    char parts[16][64];
    int count = split_path(path, parts, 16);

    if (count == 0)
        return false;

    uint32_t cluster;

    if (path[0] == '/') {
        cluster = bpb.rootCluster;
    } else {
        cluster = fat32_get_current_dir();
    }

    FAT32_DirEntry entry;

    for (int i = 0; i < count - 1; i++) {
        if (parts[i][0] == 0) {
            continue;
        }

        char upper[64];
        strcpy(upper, parts[i]);
        fat32_to_upper(upper);

        if (!fat32_find_entry(cluster, upper, &entry)) {
            return false;
        }

        if (!(entry.attr & 0x10)) {
            return false;
        }

        cluster = ((uint32_t)entry.firstClusterHigh << 16) | entry.firstClusterLow;
    }

    strcpy(out_filename, parts[count - 1]);

    int idx = fat32_find_entry_index(cluster, out_filename, out_entry);
    if (idx < 0) {
        return false;
    }

    *out_dir_cluster = cluster;
    *out_index = idx;
    return true;
}

bool fat32_resolve_dir(const char* path, uint32_t* out_cluster) {
    char parts[16][64];
    int count = split_path(path, parts, 16);

    if (count == 0)
        return false;

    uint32_t cluster;

    if (path[0] == '/') {
        cluster = bpb.rootCluster;
    } else {
        cluster = fat32_get_current_dir();
    }

    FAT32_DirEntry entry;

    for (int i = 0; i < count; i++) {
        if (parts[i][0] == 0)
            continue;

        char upper[64];
        strcpy(upper, parts[i]);
        fat32_to_upper(upper);

        if (!fat32_find_entry(cluster, upper, &entry)) {
            return false;
        }

        if (!(entry.attr & 0x10)) {
            return false;
        }

        cluster =
            ((uint32_t)entry.firstClusterHigh << 16) |
             entry.firstClusterLow;
    }

    *out_cluster = cluster;
    return true;
}

static uint32_t fat32_max_cluster(void) {
    uint32_t data_sectors = bpb.totalSectors32 - bpb.reservedSectors - (bpb.numFATs * bpb.FATSize32);

    return (data_sectors / bpb.sectorsPerCluster) + 1;
}

static uint32_t fat32_alloc_cluster(void) {
    uint32_t max_cluster = fat32_max_cluster();

    for (uint32_t c = 2; c <= max_cluster; c++) {
        if ((fat_table[c] & 0x0FFFFFFF) == 0) {
            fat_table[c] = 0x0FFFFFFF;
            return c;
        } 
    }

    return 0;
}

static bool fat32_flush_fat(void) {
    if (!fat_table || !bpb.FATSize32) {
        return false;
    }

    uint32_t fat_start = g_part_lba_start + bpb.reservedSectors;

    uint32_t fat_bytes = bpb.FATSize32 * bpb.bytesPerSector;

    for (uint8_t copy = 0; copy < bpb.numFATs; copy++) {
        uint32_t lba = fat_start + copy * bpb.FATSize32;

        const uint8_t *src = (const uint8_t*)fat_table;

        while (fat_bytes > 0) {
            uint8_t count = fat_bytes > (128 * 512) ? 128 : (uint8_t)((fat_start + 511) / 512);

            write_sectors_abs(lba, src, count);

            lba += count;
            src += count * 512;

            uint32_t written = count * 512;

            if (fat_bytes > written) {
                fat_bytes -= written;
            } else {
                fat_bytes = 0;
            }
        }

        fat_bytes = bpb.FATSize32 * bpb.bytesPerSector;
    }

    return true;
}

static bool fat32_update_dir_entry(uint32_t dir_cluster, int index, const FAT32_DirEntry *entry) {
    if (!entry || index < 0) {
        return false;
    }

    uint32_t entries_per_cluster = (bpb.bytesPerSector * bpb.sectorsPerCluster) / sizeof(FAT32_DirEntry);
    uint32_t cluster_index = index / entries_per_cluster;
    uint32_t entry_index = index % entries_per_cluster;
    uint32_t cluster = dir_cluster;

    for (uint32_t i = 0; i < cluster_index; i++) {
        cluster = get_next_cluster(cluster, fat_table);

        if (cluster >= 0x0FFFFFF8) {
            return false;
        }
    }

    uint32_t lba = fat32_cluster_to_lba(cluster);
    uint32_t entries_per_sector = bpb.bytesPerSector / sizeof(FAT32_DirEntry);
    uint32_t sector_index = entry_index / entries_per_sector;
    uint32_t entry_in_sector = entry_index % entries_per_sector;
    uint8_t sector[512];

    read_sectors_abs(lba + sector_index, sector, 1);

    FAT32_DirEntry *entries = (FAT32_DirEntry*)sector;

    entries[entry_in_sector] = *entry;

    write_sectors_abs(lba + sector_index, sector, 1);

    return true;
}

bool fat32_load_hostname() {
    FAT32_DirEntry entry;

    uint32_t root = bpb.rootCluster;

    if (!fat32_find_entry(root, "etc", &entry)) {
        return false;
    }

    uint32_t etc_cluster = ((uint32_t)entry.firstClusterHigh << 16) | entry.firstClusterLow;

    FAT32_DirEntry host_entry;

    if (!fat32_find_entry(etc_cluster, "hostname", &host_entry)) {
        return false;
    }

    FileHandle fh;
    fh.startCluster = ((uint32_t)host_entry.firstClusterHigh << 16) | host_entry.firstClusterLow;
    fh.size = host_entry.fileSize;

    if (fh.size == 0 || fh.size > 63) {
        return false;
    }

    uint8_t* buf = malloc(fh.size + 1);
    if (!buf) {
        return false;
    }

    int r = read_file(fh, &bpb, fat_table, buf);
    if (r <= 0) {
        free(buf);
        return false;
    }

    buf[fh.size] = 0;

    for (int i = 0; i < fh.size; i++) {
        if (buf[i] == '\n' || buf[i] == '\r') {
            buf[i] = 0;
            break;
        }
    }

    strcpy(g_hostname, (char*)buf);
    free(buf);

    return true;
}

bool fat32_find_entry(uint32_t dir_cluster, const char* name, FAT32_DirEntry* out) {
    char target[256];
    strcpy(target, name);
    fat32_to_upper(target);

    while (dir_cluster < 0x0FFFFFF8) {
        uint32_t lba = fat32_cluster_to_lba(dir_cluster);
        uint8_t sector[512];

        for (int s = 0; s < bpb.sectorsPerCluster; s++) {
            read_sectors_abs(lba + s, sector, 1);

            FAT32_DirEntry* e = (FAT32_DirEntry*)sector;

            char lfn[256];
            lfn[0] = 0;

            for (int i = 0; i < 16; i++) {
                if (e[i].name[0] == 0x00) {
                    return false;
                }

                if (e[i].name[0] == 0xE5) {
                    continue;
                }

                if (e[i].attr == 0x0F) {
                    FAT32_LFN_Entry* l = (FAT32_LFN_Entry*)&e[i];

                    char temp[14];
                    int p = 0;

                    for (int k = 0; k < 5; k++) {
                        uint16_t c = l->name1[k];
                        if (c == 0xFFFF || c == 0x0000) break;
                        temp[p++] = (char)c;
                    }
                    for (int k = 0; k < 6; k++) {
                        uint16_t c = l->name2[k];
                        if (c == 0xFFFF || c == 0x0000) break;
                        temp[p++] = (char)c;
                    }
                    for (int k = 0; k < 2; k++) {
                        uint16_t c = l->name3[k];
                        if (c == 0xFFFF || c == 0x0000) break;
                        temp[p++] = (char)c;
                    }

                    temp[p] = 0;

                    char new_lfn[256];
                    strcpy(new_lfn, temp);
                    strcat(new_lfn, lfn);
                    strcpy(lfn, new_lfn);

                    continue;
                }

                if (e[i].attr & 0x08) {
                    continue;
                }

                if (lfn[0] != 0) {
                    char upper_lfn[256];
                    strcpy(upper_lfn, lfn);
                    fat32_to_upper(upper_lfn);

                    if (strcmp(upper_lfn, target) == 0) {
                        *out = e[i];
                        return true;
                    }

                    lfn[0] = 0;
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
                    return true;
                }
            }
        }

        dir_cluster = get_next_cluster(dir_cluster, fat_table);
    }

    return false;
}

uint64_t fat32_partition_size_bytes() {
    return (uint64_t)bpb.totalSectors32 * bpb.bytesPerSector;
}

void fat32_mount(uint32_t fallback_part_lba_start) {
    uint8_t *mbr = fat_dma_buffer;
    read_sectors_abs(0, mbr, 1);

    if (!(mbr[510] == 0x55 && mbr[511] == 0xAA)) {
        kprintf(WHITE "<" RED " ERROR " WHITE "> " "No valid MBR!\n");
        return;
    }

    uint8_t sig0 = mbr[510];
    uint8_t sig1 = mbr[511];

    if (!(sig0 == 0x55 && sig1 == 0xAA)) {
        kprintf(WHITE "<" RED " ERROR " WHITE "> " "No valid MBR, using fallback LBA\n");
        g_part_lba_start = fallback_part_lba_start;
    } else {
        typedef struct {
            uint8_t  status;
            uint8_t  chs_first[3];
            uint8_t  type;
            uint8_t  chs_last[3];
            uint32_t lba_start;
            uint32_t sectors;
        } __attribute__((packed)) MBR_PartitionEntry;

        MBR_PartitionEntry* p = (MBR_PartitionEntry*)&mbr[0x1BE];

        uint32_t found_lba  = 0;
        int      found_idx  = -1;

        for (int i = 0; i < 4; i++) {
            if (p[i].sectors == 0) {
                continue;
            }

            uint8_t bs[512];
            read_sectors_abs(p[i].lba_start, bs, 1);

            if (!(bs[510] == 0x55 && bs[511] == 0xAA)) {
                continue;
            }

            if (memcmp(bs + 0x52, "FAT32", 5) == 0) {
                found_lba = p[i].lba_start;
                found_idx = i;
                break;
            } else {
                kprintf(WHITE "<" RED " ERROR" WHITE "> " "  Part %d: not FAT32 (fs type: '%x')\n", i, bs + 0x52);
            }
        }

        if (found_lba == 0) {
            kprintf(WHITE "<" RED " ERROR " WHITE "> " "No FAT32 partition found via bootsector, using fallback LBA\n");
            g_part_lba_start = fallback_part_lba_start;
        } else {
            g_part_lba_start = found_lba;
        }
    }

    uint8_t bs[512];
    read_sectors(0, bs);

    if (!(bs[510] == 0x55 && bs[511] == 0xAA)) {
        kprintf(WHITE "<" RED " ERROR " WHITE "> " "Invalid FAT32 boot sector signature (after select)\n");
        return;
    }

    FAT32_BootSector* b = (FAT32_BootSector*)bs;
    memcpy(&bpb, b, sizeof(FAT32_BootSector));

    init_fat32(&bpb);
}

LoadedFile fat32_load_file(const char* path) {
    LoadedFile result = {
        .data = NULL,
        .size = 0
    };

    uint32_t dir_cluster;

    char filename[256];

    FAT32_DirEntry entry;

    int index;

    if (!fat32_resolve_path_file(path, &dir_cluster, filename, &entry, &index)) {
        return result;
    }

    uint32_t cluster = ((uint32_t)entry.firstClusterHigh << 16) | entry.firstClusterLow;

    if (cluster == 0) {
        return result;
    }

    uint8_t* buffer = kmalloc(entry.fileSize + 1);

    if (!buffer) {
        return result;
    }

    FileHandle fh;

    fh.startCluster = cluster;
    fh.size = entry.fileSize;


    int read = read_file(fh, &bpb, fat_table, buffer);


    if (read <= 0) {
        free(buffer);
        return result;
    }


    result.data = buffer;
    result.size = entry.fileSize;

    return result;
}

void fat32_umount(void *internal) {
    (void)internal;

    if (fat_ram) {
        free(fat_ram);
        fat_ram = NULL;
    }

    if (fat_table) {
        free(fat_table);
        fat_table = NULL;
    }

    if (dir_buf) {
        free(dir_buf);
        dir_buf = NULL;
    }

    memset(&bpb, 0, sizeof(bpb));

    g_part_lba_start = 0;
}