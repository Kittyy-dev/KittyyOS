#pragma once
#include <stdint.h>

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

// Gesamte HBA-Struktur (Header + Ports)
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

// AHCI API
int ahci_init(void);
int ahci_read_lba(uint64_t lba, uint32_t count, void* buf);
int ahci_has_valid_fat32();