// Opsec Level: Unter das OS!!!

#include <vfs.h>
#include <dma.h>
#include <pci.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <ahci.h>
#include <kprint.h>
#include <colors.h>
#include <paging.h>
#include <storage.h>

#define PAGE_PWT (1ULL << 3)
#define PAGE_PCD (1ULL << 4)
#define AHCI_CLB_PHYS 0x00800000ULL
#define AHCI_FB_PHYS  0x00801000ULL
#define AHCI_CT_PHYS  0x00802000ULL

#define PHYS_PTR(p) ((void *)(HHDM_BASE + (uint64_t)(p)))

uint64_t abar = 0;
static volatile HBA_MEM *hba = 0;
int ahci_disk_port = -1;

int ahci_init(void) {
    if (!pci_ahci_found) {
        log_error("No AHCI controller!\n");
        return 1;
    }

    abar = pci_get_bar5(pci_ahci_bus, pci_ahci_slot, pci_ahci_func);
    abar &= ~0xFULL;

    for (uint64_t off = 0; off < 0x2000; off += 0x1000) {
        map_page(HHDM_BASE + abar + off, abar + off, PAGE_RW | PAGE_PCD | PAGE_PWT);
    }

    hba = (volatile HBA_MEM *)(HHDM_BASE + abar);

    hba->head.ghc |= (1U << 31);
    hba->head.ghc &= ~(1U << 1);

    return 0;
}

static int ahci_stop_engine(volatile HBA_PORT *port) {
    port->cmd &= ~HBA_PORT_CMD_ST;
    port->cmd &= ~HBA_PORT_CMD_FRE;

    uint32_t timeout = 1000000;

    while (port->cmd & (HBA_PORT_CMD_CR | HBA_PORT_CMD_FR)) {
        if (--timeout == 0) {
            return -1;
        }
    }

    return 0;
}

int ahci_find_port(void) {
    uint8_t mbr[512];

    for (int i = 0; i < 32; i++) {
        volatile HBA_PORT *p = &hba->ports[i];

        if ((p->ssts & 0x0F) != 3) {
            continue;
        }

        if (((p->ssts >> 8) & 0x0F) != 1) {
            continue;
        }

        if (p->sig != 0x00000101) {
            continue;
        }

        log_kernel("Checking AHCI port %d...\n", i);

        if (ahci_read(i, 0, 1, mbr) != 0) {
            continue;
        }

        if (mbr_matches_boot_disk(mbr)) {
            return i;
        }
    }

    return -1;
}

static uint64_t ahci_buf_phys(const void *buf, uint32_t bytes) {
    uint64_t v     = (uint64_t)buf;
    uint64_t first = virt_to_phys(v);

    if (!first) {
        return 0;
    }

    for (uint32_t off = 0x1000 - (v & 0xFFF); off < bytes; off += 0x1000) {
        if (virt_to_phys(v + off) != first + off) {
            return 0;
        }
    }

    return first;
}

static int ahci_rw(uint8_t port_num, uint64_t lba, uint16_t count, void *buffer, int write) {
    if (count == 0 || count > 16 || buffer == NULL) {
        return -1;
    }

    volatile HBA_PORT *port = &hba->ports[port_num];

    uint32_t bytes = (uint32_t)count * 512;
    uint64_t buf_phys = ahci_buf_phys(buffer, bytes);

    if (!buf_phys) {
        log_error("AHCI: buffer %p not mapped/contiguous!\n", buffer);
        return -6;
    }

    port->ie = 0;

    if (ahci_stop_engine(port) != 0) {
        log_error("AHCI timeout stopping engine! CMD: %08x\n", port->cmd);
        return -2;
    }

    port->clb  = (uint32_t)AHCI_CLB_PHYS;
    port->clbu = (uint32_t)(AHCI_CLB_PHYS >> 32);
    port->fb   = (uint32_t)AHCI_FB_PHYS;
    port->fbu  = (uint32_t)(AHCI_FB_PHYS >> 32);

    HBA_CMD_HEADER *cmd_list  = (HBA_CMD_HEADER *)PHYS_PTR(AHCI_CLB_PHYS);
    uint8_t        *fis_area  = (uint8_t *)PHYS_PTR(AHCI_FB_PHYS);
    uint8_t        *cmd_table = (uint8_t *)PHYS_PTR(AHCI_CT_PHYS);

    memset(cmd_list, 0, 1024);
    memset(fis_area, 0, 256);
    memset(cmd_table, 0, 256);

    HBA_CMD_HEADER *cmd = &cmd_list[0];

    cmd->flags = 5 | (write ? (1 << 6) : 0);
    cmd->prdtl = 1;
    cmd->ctba  = (uint32_t)AHCI_CT_PHYS;
    cmd->ctbau = (uint32_t)(AHCI_CT_PHYS >> 32);

    FIS_REG_H2D *fis = (FIS_REG_H2D *)cmd_table;

    fis->fis_type = 0x27;
    fis->pmport_c = (1 << 7);
    fis->command  = write ? ATA_CMD_WRITE_DMA_EXT : ATA_CMD_READ_DMA_EXT;

    fis->lba0 = (uint8_t)(lba);
    fis->lba1 = (uint8_t)(lba >> 8);
    fis->lba2 = (uint8_t)(lba >> 16);
    fis->lba3 = (uint8_t)(lba >> 24);
    fis->lba4 = (uint8_t)(lba >> 32);
    fis->lba5 = (uint8_t)(lba >> 40);

    fis->device     = (1 << 6);
    fis->count      = count & 0xFF;
    fis->count_high = (count >> 8) & 0xFF;

    HBA_PRDT_ENTRY *prdt = (HBA_PRDT_ENTRY *)(cmd_table + 0x80);

    prdt->dba  = (uint32_t)buf_phys;
    prdt->dbau = (uint32_t)(buf_phys >> 32);
    prdt->dbc  = bytes - 1;

    port->serr = 0xFFFFFFFF;
    port->is   = 0xFFFFFFFF;

    port->cmd |= HBA_PORT_CMD_FRE;
    port->cmd |= HBA_PORT_CMD_ST;

    asm volatile("" ::: "memory");

    port->ci |= 1;

    uint32_t timeout = 10000000;

    while (port->ci & 1) {
        if (port->is & HBA_PxIS_TFES) {
            port->cmd &= ~HBA_PORT_CMD_ST;
            return -3;
        }

        if (--timeout == 0) {
            log_error("AHCI %s timeout! CI: %08x IS: %08x TFD: %08x\n", write ? "write" : "read", port->ci, port->is, port->tfd);

            port->cmd &= ~HBA_PORT_CMD_ST;
            return -4;
        }
    }

    port->cmd &= ~HBA_PORT_CMD_ST;

    timeout = 10000000;

    while (port->cmd & HBA_PORT_CMD_CR) {
        if (--timeout == 0) {
            log_error("AHCI timeout waiting for CR!\n");
            return -5;
        }
    }

    return 0;
}

int ahci_read(uint8_t port_num, uint64_t lba, uint16_t count, void *buffer) {
    return ahci_rw(port_num, lba, count, buffer, 0);
}

int ahci_write(uint8_t port_num, uint64_t lba, uint16_t count, const void *buffer) {
    return ahci_rw(port_num, lba, count, (void *)buffer, 1);
}

size_t devsda_read(vfs_node_t *node, void *buf, size_t len) {
    (void)node;

    if (!buf || len == 0) {
        return 0;
    }

    if (ahci_disk_port < 0) {
        return 0;
    }

    if (len % 512 != 0) {
        return 0;
    }

    uint16_t sectors = len / 512;
    size_t done = 0;

    while (sectors > 0) {
        uint16_t chunk = sectors > 16 ? 16 : sectors;

        if (ahci_read((uint8_t)ahci_disk_port, done / 512, chunk, (uint8_t *)buf + done) != 0) {
            return done;
        }

        done += chunk * 512;
        sectors -= chunk;
    }

    return done;
}

size_t devsda_write(vfs_node_t *node, const void *buf, size_t len) {
    (void)node;

    if (!buf || len == 0) {
        return 0;
    }

    if (ahci_disk_port < 0) {
        return 0;
    }

    if (len % 512 != 0) {
        return 0;
    }

    uint32_t sectors = len / 512;
    size_t done = 0;

    while (sectors > 0) {
        uint16_t count = sectors > 16 ? 16 : sectors;

        if (ahci_write((uint8_t)ahci_disk_port, done / 512, count, (const uint8_t *)buf + done) != 0) {
            break;
        }

        done += (size_t)count * 512;
        sectors -= count;
    }

    return done;
}