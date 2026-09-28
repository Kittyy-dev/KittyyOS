#pragma once
#include <stddef.h>
#include <stdint.h>
#include <vfs.h>

#define HBA_PORT_CMD_ST   (1U << 0)
#define HBA_PORT_CMD_FRE  (1U << 4)
#define HBA_PORT_CMD_FR   (1U << 14)
#define HBA_PORT_CMD_CR   (1U << 15)
#define HBA_PxIS_TFES    (1U << 30)
#define ATA_CMD_READ_DMA_EXT 0x25
#define MBR_SIGNATURE 0xAA55
#define ATA_CMD_WRITE_DMA_EXT 0x35

typedef struct {
    uint8_t boot;
    uint8_t chs_start[3];
    uint8_t type;
    uint8_t chs_end[3];
    uint32_t lba_start;
    uint32_t sector_count;
} __attribute__((packed)) MBR_PARTITION;

typedef struct {
    uint8_t boot_code[446];
    MBR_PARTITION partitions[4];
    uint16_t signature;
} __attribute__((packed)) MBR;

// AHCI Memory Registers (HBA)
typedef struct {
    uint32_t cap;       // 0x00
    uint32_t ghc;       // 0x04
    uint32_t is;        // 0x08
    uint32_t pi;        // 0x0C
    uint32_t vs;        // 0x10
    uint32_t ccc_ctl;   // 0x14
    uint32_t ccc_pts;   // 0x18
    uint32_t em_loc;    // 0x1C
    uint32_t em_ctl;    // 0x20
    uint32_t cap2;      // 0x24
    uint32_t bohc;      // 0x28
    uint8_t  rsv[0xA0 - 0x2C];
    uint8_t  vendor[0x100 - 0xA0];
} __attribute__((packed)) HBA_MEM_HEAD;

// AHCI Port
typedef struct {
    uint32_t clb;       // 0x00
    uint32_t clbu;      // 0x04
    uint32_t fb;        // 0x08
    uint32_t fbu;       // 0x0C
    uint32_t is;        // 0x10
    uint32_t ie;        // 0x14
    uint32_t cmd;       // 0x18
    uint32_t rsv0;      // 0x1C
    uint32_t tfd;       // 0x20
    uint32_t sig;       // 0x24
    uint32_t ssts;      // 0x28
    uint32_t sctl;      // 0x2C
    uint32_t serr;      // 0x30
    uint32_t sact;      // 0x34
    uint32_t ci;        // 0x38
    uint32_t sntf;      // 0x3C
    uint32_t fbs;       // 0x40
    uint32_t rsv1[11];  // 0x44–0x6F
    uint32_t vendor[4]; // 0x70–0x7F
} __attribute__((packed)) HBA_PORT;

typedef struct {
    HBA_MEM_HEAD head;
    HBA_PORT     ports[32];
} __attribute__((packed)) HBA_MEM;

// Command Header
typedef struct {
    uint16_t flags;
    uint16_t prdtl;
    uint32_t prdbc;
    uint32_t ctba;
    uint32_t ctbau;
    uint32_t rsv1[4];
} __attribute__((packed)) HBA_CMD_HEADER;

// PRDT
typedef struct {
    uint32_t dba;
    uint32_t dbau;
    uint32_t rsv0;
    uint32_t dbc;
} __attribute__((packed)) HBA_PRDT_ENTRY;

// Command Table
typedef struct {
    uint8_t  cfis[64];
    uint8_t  acmd[16];
    uint8_t  rsv[48];
    HBA_PRDT_ENTRY prdt[1];
} __attribute__((packed)) HBA_CMD_TBL;

typedef struct {
    uint8_t fis_type;

    uint8_t pmport_c;
    uint8_t command;
    uint8_t featurel;

    uint8_t lba0;
    uint8_t lba1;
    uint8_t lba2;
    uint8_t device;

    uint8_t lba3;
    uint8_t lba4;
    uint8_t lba5;
    uint8_t featureh;

    uint8_t count;
    uint8_t count_high;
    uint8_t icc;
    uint8_t control;

    uint8_t rsv[4];
} __attribute__((packed)) FIS_REG_H2D;

_Static_assert(sizeof(HBA_MEM_HEAD) == 0x100, "HBA_MEM_HEAD size wrong!\n");
_Static_assert(sizeof(HBA_PORT) == 0x80, "HBA_PORT size wrong!\n");
_Static_assert(offsetof(HBA_MEM, ports) == 0x100, "HBA Ports offset wrong!\n");

extern int ahci_disk_port;

// AHCI API
int ahci_init(void);
int ahci_read(uint8_t port_num, uint64_t lba, uint16_t count, void *buffer);
int fat32_check_partition(uint8_t port);
int ahci_find_port(void);
int ahci_write(uint8_t port_num, uint64_t lba, uint16_t count, const void *buffer);
size_t devsda_write(vfs_node_t *node, const void *buf, size_t len);
size_t devsda_read(vfs_node_t *node, void *buf, size_t len);