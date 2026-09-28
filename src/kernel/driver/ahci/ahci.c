// Opsec Level: Unter das OS!!!

#include <vfs.h>
#include <dma.h>
#include <pci.h>
#include <stddef.h>
#include <stdint.h>
#include <ahci.h>
#include <kprint.h>
#include <colors.h>
#include <paging.h>

uint64_t abar = 0;
int ahci_disk_port;

int ahci_init(void) {
    if (!pci_ahci_found) {
        kprintf(WHITE "<" RED " ERROR " WHITE "> " "No AHCI controller!\n");
        return 1;
    }

    abar = pci_get_bar5(pci_ahci_bus, pci_ahci_slot, pci_ahci_func);

    abar &= ~0xFULL;

    /*
        AHCI MMIO:
     
        0x0000 - 0x00FF = HBA registers
        0x0100 - 0x10FF = port registers
    */

    for (uint64_t off = 0; off < 0x2000; off += 0x1000) {
        map_page(abar + off, abar + off, PAGE_RW);
    }

    volatile HBA_MEM *hba = (HBA_MEM *)(uintptr_t)abar;

    /*
        Enable AHCI
    
        GHC bit 31 = AE
     */
    
    hba->head.ghc |= (1U << 31);
    hba->head.ghc &= ~(1U << 1);

    return 0;
}


int ahci_find_port(void) {
    volatile HBA_MEM *hba = (HBA_MEM *)(uintptr_t)abar;

    for (int i = 0; i < 32; i++) {

        if (!(hba->head.pi & (1U << i)))
            continue;

        volatile HBA_PORT *port = &hba->ports[i];

        uint32_t ssts = port->ssts;
        uint32_t det  = ssts & 0x0F;
        uint32_t ipm  = (ssts >> 8) & 0x0F;

        if (det == 3 && ipm == 1) {
            kprintf(WHITE "<" GREEN " KERNEL " WHITE "> " "SATA device found on port %u!\n", i);
            
            // Disable interrupts
            port->ie = 0;

            // Stop command engine
            port->cmd &= ~HBA_PORT_CMD_ST;
            port->cmd &= ~HBA_PORT_CMD_FRE;

            uint32_t timeout = 1000000;

            while (port->cmd & HBA_PORT_CMD_CR) {
                if (--timeout == 0) {
                    kprintf(WHITE "<" RED " ERROR " WHITE "> " "CR won't clear! CMD: %08x\n", port->cmd);

                    return -1;
                }
            }

            timeout = 1000000;

            while (port->cmd & HBA_PORT_CMD_FR) {
                if (--timeout == 0) {
                    kprintf(WHITE "<" RED " ERROR " WHITE "> " "FR won't clear! CMD: %08x\n", port->cmd);

                    return -2;
                }
            }

            kprintf(WHITE "<" GREEN " KERNEL " WHITE "> " "Port %u stopped. CMD: %08x\n", i, port->cmd);

            ahci_disk_port = i;

            return i;
        }
    }

    kprintf(WHITE "<" RED " ERROR " WHITE "> " "No SATA device found!\n");

    return -1;
}

int ahci_read(uint8_t port_num, uint64_t lba, uint16_t count, void *buffer) {
    if (count == 0 || count > 16 || buffer == NULL) {
        return -1;
    }

    volatile HBA_MEM *hba = (HBA_MEM*)(uintptr_t)abar;
    volatile HBA_PORT *port = &hba->ports[port_num];

    port->ie = 0;

    // Stop command engine
    port->cmd &= ~HBA_PORT_CMD_ST;
    port->cmd &= ~HBA_PORT_CMD_FRE;

    // Wait until command engine stopped
    uint32_t timeout = 1000000;

    while (port->cmd & (HBA_PORT_CMD_CR | HBA_PORT_CMD_FR)) {
        if (--timeout == 0) {
            kprintf(WHITE "<" RED " ERROR " WHITE "> " "AHCI timeout stopping engine! CMD: %08x\n", port->cmd);
            return -2;
        }
    }

    // Command list
    uint32_t clb = 0x00800000;
    uint32_t fb  = 0x00801000;
    uint32_t ct  = 0x00802000;

    port->clb = clb;
    port->clbu = 0;

    port->fb = fb;
    port->fbu = 0;

    // Clear command list
    HBA_CMD_HEADER *cmd_list = (HBA_CMD_HEADER*)(uintptr_t)clb;

    for (uint32_t i = 0; i < 32; i++) {
        ((uint32_t*)cmd_list)[i] = 0;
    }

    // Command slot 0
    HBA_CMD_HEADER *cmd = &cmd_list[0];

    cmd->flags = 5; // CFL = 5 DWORDs = 20 byte
    cmd->prdtl = 1;
    cmd->ctba = ct;
    cmd->ctbau = 0;

    // Clear command table
    uint8_t *cmd_table = (uint8_t*)(uintptr_t)ct;

    for (uint32_t i = 0; i < 256; i++) {
        cmd_table[i] = 0;
    }

    // FIS
    FIS_REG_H2D *fis = (FIS_REG_H2D*)cmd_table;

    fis->fis_type = 0x27;

    // C = 1
    fis->pmport_c = (1 << 7);

    fis->command = ATA_CMD_READ_DMA_EXT;

    // LBA
    fis->lba0 = (uint8_t)(lba);
    fis->lba1 = (uint8_t)(lba >> 8);
    fis->lba2 = (uint8_t)(lba >> 16);

    fis->lba3 = (uint8_t)(lba >> 24);
    fis->lba4 = (uint8_t)(lba >> 32);
    fis->lba5 = (uint8_t)(lba >> 40);

    // LBA mode
    fis->device = (1 << 6);

    // Sector count
    fis->count = count & 0xFF;
    fis->count_high = (count >> 8) & 0xFF;

    // PRDT
    HBA_PRDT_ENTRY *prdt = (HBA_PRDT_ENTRY*)(cmd_table + 0x80);

    uint32_t bytes = (uint32_t)count * 512;

    prdt->dba = (uint32_t)(uintptr_t)buffer;
    prdt->dbau = 0;

    // DBC = number of bytes - 1
    prdt->dbc = bytes - 1;

    // Clear errors
    port->serr = 0xFFFFFFFF;
    port->is = 0xFFFFFFFF;

    // Start command engine
    port->cmd |= HBA_PORT_CMD_FRE;
    port->cmd |= HBA_PORT_CMD_ST;

    // Execute slot 0
    // kprintf("CLB=%08x FB=%08x CT=%08x BUFFER=%p\n", clb, fb, ct, buffer);
    port->ci |= 1;
    // kprintf("CI=%08x CMD=%08x\n", port->ci, port->cmd);

    // Wait
    timeout = 10000000;

    while (port->ci & 1) {
        if (port->is & HBA_PxIS_TFES) {
            port->cmd &= ~HBA_PORT_CMD_ST;

            return -3;
        }

        if (--timeout == 0) {
            kprintf(WHITE "<" RED " ERROR " WHITE "> " "AHCI read timeout! CI: %08x IS: %08x TFD: %08x\n", port->ci, port->is, port->tfd);

            port->cmd &= ~HBA_PORT_CMD_ST;

            return -4;
        }
    }

    // Stop command engine
    port->cmd &= ~HBA_PORT_CMD_ST;

    timeout = 10000000;

    while (port->cmd & HBA_PORT_CMD_CR) {
        if (--timeout == 0) {
            kprintf(WHITE "<" RED " ERROR " WHITE "> " "AHCI timeout waiting for CR: 0\n");

            return -5;
        }
    }

    return 0;
}

int ahci_write(uint8_t port_num, uint64_t lba, uint16_t count, const void *buffer) {
    if (count == 0 || count > 16 || buffer == NULL) {
        return -1;
    }

    volatile HBA_MEM *hba = (HBA_MEM*)(uintptr_t)abar;
    volatile HBA_PORT *port = &hba->ports[port_num];

    port->ie = 0;

    // Stop command engine
    port->cmd &= ~HBA_PORT_CMD_ST;
    port->cmd &= ~HBA_PORT_CMD_FRE;

    uint32_t timeout = 1000000;

    while (port->cmd & (HBA_PORT_CMD_CR | HBA_PORT_CMD_FRE)) {
        if (--timeout == 0) {
            kprintf(WHITE "<" RED " ERROR " WHITE "> " "AHCI timeout stopping engine! CMD: %08x\n", port->cmd);
            return -2;
        }
    }

    // Command list / FIS / Command table

    uint32_t clb = 0x00800000;
    uint32_t fb  = 0x00801000;
    uint32_t ct  = 0x00802000;

    port->clb = clb;
    port->clbu = 0;

    port->fb = fb;
    port->fbu = 0;

    HBA_CMD_HEADER *cmd_list = (HBA_CMD_HEADER*)(uintptr_t)clb;

    for (uint32_t i = 0; i < 32; i++) {
        ((uint32_t*)cmd_list)[i] = 0;
    }

    // Command slot 0
    HBA_CMD_HEADER *cmd = &cmd_list[0];

    cmd->flags = 5;
    cmd->prdtl = 1;
    cmd->ctba = ct;
    cmd->ctbau = 0;

    // Command table
    uint8_t *cmd_table = (uint8_t*)(uintptr_t)ct;

    for (uint32_t i = 0; i < 256; i++) {
        cmd_table[i] = 0;
    }

    // Register H2D FIS
    FIS_REG_H2D *fis = (FIS_REG_H2D*)cmd_table;

    fis->fis_type = 0x27;

    // Command + C bit
    fis->pmport_c = (1 << 7);
    fis->command = ATA_CMD_WRITE_DMA_EXT;

    // LBA
    fis->lba0 = (uint8_t)(lba);
    fis->lba1 = (uint8_t)(lba >> 8);
    fis->lba2 = (uint8_t)(lba >> 16);

    fis->lba3 = (uint8_t)(lba >> 24);
    fis->lba4 = (uint8_t)(lba >> 32);
    fis->lba5 = (uint8_t)(lba >> 40);

    // LBA mode
    fis->device = (1 << 6);

    // Sector count
    fis->count = count & 0xFF;
    fis->count_high = (count >> 8) & 0xFF;

    // PRDT
    HBA_PRDT_ENTRY *prdt = (HBA_PRDT_ENTRY*)(cmd_table + 0x80);

    uint32_t bytes = (uint32_t)count * 512;

    prdt->dba = (uint32_t)(uintptr_t)buffer;
    prdt->dbau = 0;

    // Bytes count - 1
    prdt->dbc = bytes - 1;

    // Remove error
    port->serr = 0xFFFFFFFF;
    port->is = 0xFFFFFFFF;

    // Start command engine
    port->cmd |= HBA_PORT_CMD_FRE;
    port->cmd |= HBA_PORT_CMD_ST;

    // Command slot 0
    port->ci |= 1;

    // Wait
    timeout = 10000000;

    while (port->ci & 1) {
        if (port->is & HBA_PxIS_TFES) {
            port->cmd &= ~HBA_PORT_CMD_ST;
            return -3;
        }

        if (--timeout == 0) {
            kprintf(WHITE "<" RED " ERROR " WHITE "> " "AHCI write timeout! CI: %08x IS: %08x TFD: %08x\n", port->ci, port->is, port->tfd);

            port->cmd &= ~HBA_PORT_CMD_ST;

            return -4;
        }
    }

    // Stop command engine
    port->cmd &= ~HBA_PORT_CMD_ST;

    timeout = 10000000;

    while (port->cmd & HBA_PORT_CMD_CR) {
        if (--timeout == 0) {
            kprintf(WHITE "<" RED " ERROR " WHITE "> " "AHCI timeout waiting for CR!\n");

            return -5;
        }
    }

    return 0;
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

        if (ahci_read((uint8_t)ahci_disk_port, done / 512, chunk, (uint8_t*)buf + done) != 0) {
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

        if (ahci_write((uint8_t)ahci_disk_port, done / 512, count, (const uint8_t*)buf + done) != 0) {
            break;
        }

        done += (size_t)count * 512;
        sectors -= count;
    }

    return done;
}