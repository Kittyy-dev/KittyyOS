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

FAT32_BootSector bpb;

uint32_t* fat_ram = 0;
uint8_t*  dir_buf = 0;

uint32_t* fat_table;

uint32_t fat32_cluster_to_lba(uint32_t cluster) {
    uint32_t firstDataSector =
        bpb.reservedSectors +
        (bpb.numFATs * bpb.FATSize32);

    uint32_t sector =
        firstDataSector +
        (cluster - 2) * bpb.sectorsPerCluster;

    return g_part_lba_start + sector;
}

// Gibt den Namen des aktuellen Verzeichnisses zurück.
// Root → "/"

void fat32_to_upper(char* s) {
    while (*s) {
        if (*s >= 'a' && *s <= 'z')
            *s = *s - 32;   // ASCII: a→A, b→B, ...
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

FileHandle open_file(const char* filename11,
                     FAT32_BootSector* b,
                     uint32_t* fat,
                     uint8_t* dirBuffer)
{
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

int read_file(FileHandle fh,
              FAT32_BootSector* b,
              uint32_t* fat,
              uint8_t* out)
{
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

int write_file(FileHandle fh,
               FAT32_BootSector* b,
               uint32_t* fat,
               const void* data,
               uint32_t size)
{
    uint32_t cluster = fh.startCluster;
    uint32_t written = 0;
    uint32_t fds = first_data_sector(b);
    const uint8_t* buf = (const uint8_t*)data;

    while (size > 0 && cluster < 0x0FFFFFF8) {
        uint32_t sector = fds + ((cluster - 2) * b->sectorsPerCluster);

        for (uint32_t i = 0; i < b->sectorsPerCluster && size > 0; i++) {
            write_sectors(sector + i, buf + written);
            uint32_t step = b->bytesPerSector;
            written += step;
            if (size > step) size -= step;
            else size = 0;
        }

        if (size > 0) {
            uint32_t newCluster = find_free_cluster(fat);
            if (newCluster >= 0x0FFFFFF8) break;

            fat[cluster]    = newCluster;
            fat[newCluster] = 0x0FFFFFFF;
            cluster = newCluster;
        }
    }

    return written;
}

void init_fat32(FAT32_BootSector* b) {
    vga_print(WHITE "<" YELLOW " INFO " WHITE "> " "Fat32: start\n");

    uint32_t fat_size_bytes = b->FATSize32 * b->bytesPerSector;
    fat_ram = malloc(fat_size_bytes);
    if (!fat_ram) {
        vga_print(WHITE "<" RED " KERNEL " WHITE "> " "malloc for FAT failed\n");
        return;
    }

    uint32_t fat_start_lba = g_part_lba_start + b->reservedSectors;
    uint32_t remaining = b->FATSize32;
    uint32_t lba = fat_start_lba;
    uint8_t* dst = (uint8_t*)fat_ram;

    const uint32_t CHUNK = 128;

    vga_printf(WHITE "<" YELLOW " INFO " WHITE "> " "FAT load: %x sectors\n", remaining);

    while (remaining > 0) {
        uint32_t chunk = remaining > CHUNK ? CHUNK : remaining;

        int r = ata_dma_read(lba, (uint16_t)chunk, dst);
        if (r != 0) {
            vga_print(WHITE "<" RED " ERROR " WHITE "> " "FAT DMA failed! fallback PIO\n");
            read_sectors_abs(lba, dst, (uint8_t)chunk);
        }

        lba       += chunk;
        dst       += chunk * b->bytesPerSector;
        remaining -= chunk;
    }

    dir_buf = malloc(b->sectorsPerCluster * b->bytesPerSector);
    if (!dir_buf) {
        vga_print(WHITE "<" RED " ERROR " WHITE "> " "Malloc or Dir Buf failed!\n");
        return;
    }

    fat_table = malloc(bpb.FATSize32 * bpb.bytesPerSector);
    read_sectors_abs(g_part_lba_start + bpb.reservedSectors, (uint8_t*)fat_table, bpb.FATSize32);

    vga_print("\n" WHITE "<" GREEN " KERNEL " WHITE "> " "Fat32: done\n\n");
}

void fat32_ls_root(void) {
    uint32_t cluster = bpb.rootCluster;
    uint32_t fds = bpb.reservedSectors + (bpb.numFATs * bpb.FATSize32);

    int safety = 0;

    while (cluster < 0x0FFFFFF8 && safety < 32) {
        vga_printf("ls: cluster=%x\n", cluster);

        uint32_t rel_sector = fds + ((cluster - 2) * bpb.sectorsPerCluster);
        uint32_t abs_sector = g_part_lba_start + rel_sector;

        read_sectors_abs(abs_sector, dir_buf, (uint8_t)bpb.sectorsPerCluster);

        uint32_t entries =
            (bpb.bytesPerSector * bpb.sectorsPerCluster) / sizeof(FAT32_DirEntry);
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

            vga_printf("%s  0x%x bytes\n", name, e[i].fileSize);
        }

        cluster = get_next_cluster(cluster, fat_ram);
        safety++;
    }

    vga_print("\n");
}

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

                // Leerer Eintrag
                if (e[i].name[0] == 0x00)
                    continue;

                // Gelöscht
                if (e[i].name[0] == 0xE5)
                    continue;

                // LFN-Eintrag
                if (e[i].attr == 0x0F) {
                    FAT32_LFN_Entry* l = (FAT32_LFN_Entry*)&e[i];

                    char temp[14];
                    int p = 0;

                    // Unicode → ASCII (einfach)
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

                    // vorne anhängen (LFN kommt rückwärts)
                    char new_lfn[256];
                    strcpy(new_lfn, temp);
                    strcat(new_lfn, lfn);
                    strcpy(lfn, new_lfn);

                    continue;
                }

                // Volume Label ignorieren
                if (e[i].attr & 0x08)
                    continue;

                // Wenn LFN existiert → das ausgeben
                if (lfn[0] != 0) {
                    if (e[i].attr & 0x10)
                        vga_printf("::%s\n", lfn);
                    else
                        vga_printf("%s\n", lfn);

                    lfn[0] = 0;
                    continue;
                }

                // Kein LFN → 8.3 Name bauen
                char name[13];
                int p = 0;

                for (int k = 0; k < 8; k++) {
                    if (e[i].name[k] == ' ')
                        break;
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
                        if (e[i].name[k] == ' ')
                            break;
                        name[p++] = e[i].name[k];
                    }
                }

                name[p] = 0;

                if (e[i].attr & 0x10)
                    vga_printf("::%s\n", name);
                else
                    vga_printf("%s\n", name);
            }
        }

        cluster = get_next_cluster(cluster, fat_table);
    }
}

bool fat32_find_entry(uint32_t dir_cluster, const char* name, FAT32_DirEntry* out)
{
    char target[256];
    strcpy(target, name);
    fat32_to_upper(target);   // FAT32 ist case-insensitive

    while (dir_cluster < 0x0FFFFFF8) {
        uint32_t lba = fat32_cluster_to_lba(dir_cluster);
        uint8_t sector[512];

        for (int s = 0; s < bpb.sectorsPerCluster; s++) {
            read_sectors_abs(lba + s, sector, 1);

            FAT32_DirEntry* e = (FAT32_DirEntry*)sector;

            char lfn[256];
            lfn[0] = 0;

            for (int i = 0; i < 16; i++) {

                // Ende der Liste
                if (e[i].name[0] == 0x00)
                    return false;

                // Gelöscht
                if (e[i].name[0] == 0xE5)
                    continue;

                // LFN-Eintrag
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

                    // LFN kommt rückwärts → prepend
                    char new_lfn[256];
                    strcpy(new_lfn, temp);
                    strcat(new_lfn, lfn);
                    strcpy(lfn, new_lfn);

                    continue;
                }

                // Volume Label ignorieren
                if (e[i].attr & 0x08)
                    continue;

                // Wenn LFN existiert → vergleichen
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

                // Shortname bauen
                char shortname[13];
                int p = 0;

                for (int k = 0; k < 8; k++) {
                    if (e[i].name[k] == ' ')
                        break;
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
                        if (e[i].name[k] == ' ')
                            break;
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

void fat32_mount(uint32_t fallback_part_lba_start) {
    vga_print(WHITE "<" YELLOW " INFO " WHITE "> " "FAT32: mount start\n");

    // 1) MBR lesen
    uint8_t mbr[512];
    read_sectors_abs(0, mbr, 1);

    uint8_t sig0 = mbr[510];
    uint8_t sig1 = mbr[511];

    vga_printf(WHITE "<" YELLOW " INFO " WHITE "> "
               "MBR signature: %x %x\n", sig0, sig1);

    if (!(sig0 == 0x55 && sig1 == 0xAA)) {
        vga_print(WHITE "<" RED " ERROR " WHITE "> "
                  "No valid MBR, using fallback LBA\n");
        g_part_lba_start = fallback_part_lba_start;
    } else {
        // Partitionseinträge parsen
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

        vga_print(WHITE "<" YELLOW " INFO " WHITE "> "
                  "Scanning MBR partitions: \n");

        for (int i = 0; i < 4; i++) {
            vga_printf(WHITE "<" YELLOW " INFO " WHITE "> " "Part %d: type=%x start=%x size=%x\n",
                       i, p[i].type, p[i].lba_start, p[i].sectors);

            if (p[i].sectors == 0)
                continue; // leer

            // Bootsektor dieser Partition lesen
            uint8_t bs[512];
            read_sectors_abs(p[i].lba_start, bs, 1);

            // Signatur prüfen
            if (!(bs[510] == 0x55 && bs[511] == 0xAA)) {
                vga_printf(WHITE "<" YELLOW " INFO " WHITE "> "
                           "Part %d: invalid boot sig %x %x\n",
                           i, bs[510], bs[511]);
                continue;
            }

            // FS-Typ-Feld bei FAT32: Offset 0x52, 8 Bytes, z.B. "FAT32   "
            if (memcmp(bs + 0x52, "FAT32", 5) == 0) {
                vga_printf(WHITE "<" YELLOW " INFO " WHITE "> "
                           "Part %d: FAT32 detected at LBA %x\n",
                           i, p[i].lba_start);
                found_lba = p[i].lba_start;
                found_idx = i;
                break; // erste passende nehmen
            } else {
                vga_printf(WHITE "<" YELLOW " INFO " WHITE "> "
                           "  Part %d: not FAT32 (fs type: '%.8s')\n",
                           i, bs + 0x52);
            }
        }

        if (found_lba == 0) {
            vga_print(WHITE "<" RED " ERROR " WHITE "> "
                      "No FAT32 partition found via bootsector, using fallback LBA\n");
            g_part_lba_start = fallback_part_lba_start;
        } else {
            g_part_lba_start = found_lba;
            vga_printf(WHITE "<" YELLOW " INFO " WHITE "> "
                       "Using FAT32 partition %d at LBA %x\n",
                       found_idx, g_part_lba_start);
        }
    }

    // 2) Bootsektor der gewählten Partition lesen (relativ mit read_sectors)
    uint8_t bs[512];
    read_sectors(0, bs);   // liest LBA = g_part_lba_start

    vga_printf(WHITE "<" YELLOW " INFO " WHITE "> "
               "Bootsector sig: %x %x\n", bs[510], bs[511]);

    if (!(bs[510] == 0x55 && bs[511] == 0xAA)) {
        vga_print(WHITE "<" RED " ERROR " WHITE "> "
                  "Invalid FAT32 boot sector signature (after select)\n");
        return;
    }

    // 3) BPB übernehmen
    FAT32_BootSector* b = (FAT32_BootSector*)bs;
    memcpy(&bpb, b, sizeof(FAT32_BootSector));

    vga_printf(WHITE "<" YELLOW " INFO " WHITE "> "
               "bytesPerSector: %x\n", bpb.bytesPerSector);
    vga_printf(WHITE "<" YELLOW " INFO " WHITE "> "
               "sectorsPerCluster: %x\n", bpb.sectorsPerCluster);
    vga_printf(WHITE "<" YELLOW " INFO " WHITE "> "
               "reservedSectors: %x\n", bpb.reservedSectors);
    vga_printf(WHITE "<" YELLOW " INFO " WHITE "> "
               "numFATs: %x\n", bpb.numFATs);
    vga_printf(WHITE "<" YELLOW " INFO " WHITE "> "
               "FATSize32: %x\n", bpb.FATSize32);
    vga_printf(WHITE "<" YELLOW " INFO " WHITE "> "
               "rootCluster: %x\n", bpb.rootCluster);

    // 4) FAT in RAM laden, dir_buf allocen
    init_fat32(&bpb);
}