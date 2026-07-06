#include <stdint.h>
#include <ports.h>
#include <vga.h>
#include <colors.h>
#include <ahci.h>

#define PCI_CONFIG_ADDRESS 0xCF8
#define PCI_CONFIG_DATA    0xCFC

// PCI Config Read
static inline uint32_t pci_read32(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset) {
    uint32_t address =
        (1u << 31) |
        ((uint32_t)bus  << 16) |
        ((uint32_t)slot << 11) |
        ((uint32_t)func << 8)  |
        (offset & 0xFC);
    outl(PCI_CONFIG_ADDRESS, address);
    return inl(PCI_CONFIG_DATA);
}

static volatile HBA_MEM*  g_hba   = 0;
static volatile HBA_PORT* g_port  = 0;
static HBA_CMD_HEADER*    g_clb   = 0;
static HBA_CMD_TBL*       g_ctbl  = 0;

static void print_hex32(uint32_t v) {
    const char* hex = "0123456789ABCDEF";
    char buf[9];
    buf[8] = 0;
    for (int i = 7; i >= 0; --i) {
        buf[i] = hex[v & 0xF];
        v >>= 4;
    }
    vga_print(buf);
}

static int ahci_find_controller(void) {
    for (uint8_t bus = 0; bus < 32; bus++) {
        for (uint8_t slot = 0; slot < 32; slot++) {
            uint32_t id = pci_read32(bus, slot, 0, 0x00);
            if (id == 0xFFFFFFFF) continue;

            uint32_t classcode = pci_read32(bus, slot, 0, 0x08);
            uint8_t base  = (classcode >> 24) & 0xFF;
            uint8_t sub   = (classcode >> 16) & 0xFF;
            uint8_t prog  = (classcode >> 8)  & 0xFF;

            // Mass Storage, SATA, AHCI
            if (base == 0x01 && sub == 0x06 && prog == 0x01) {
                uint32_t bar5 = pci_read32(bus, slot, 0, 0x24);
                uint64_t addr = bar5 & ~0xF;
                vga_printf(WHITE "<" YELLOW " INFO " WHITE "> " "AHCI: found at BUS=%x SLOT=%x", bus, slot);
                vga_printf(" BAR5=");
                print_hex32(bar5);
                vga_printf(" ADDR=");
                print_hex32(addr);
                vga_printf("\n");
                g_hba = (volatile HBA_MEM*)(uintptr_t)addr;
                return 0;
            }
        }
    }
    vga_printf(WHITE "<" RED " ERROR " WHITE "> "
               "AHCI: no controller found\n");
    return -1;
}

static int ahci_find_port(void) {
    uint32_t pi = g_hba->head.pi;
    for (int i = 0; i < 32; i++) {
        if (!(pi & (1u << i))) continue;

        volatile HBA_PORT* p = &g_hba->ports[i];
        uint32_t sig = p->sig;

        // SATA / ATAPI / etc. – hier sehr grob
        if (sig == 0x00000101 || sig == 0xEB140101 || sig == 0x96690101) {
            g_port = p;
            vga_printf(WHITE "<" YELLOW " INFO " WHITE "> "
                       "AHCI: using port %d (sig=%x)\n", i, sig);
            return 0;
        }
    }
    vga_printf(WHITE "<" RED " ERROR " WHITE "> "
               "AHCI: no SATA port with valid signature\n");
    return -1;
}

int ahci_init(void) {
    if (ahci_find_controller() != 0) return -1;
    if (ahci_find_port() != 0) return -1;

    // AHCI enable
    g_hba->head.ghc |= (1 << 31);

    // Port stoppen
    g_port->cmd &= ~1;        // ST=0
    g_port->cmd &= ~(1 << 4); // FRE=0
    while (g_port->cmd & (1 << 15)) {} // CR
    while (g_port->cmd & (1 << 14)) {} // FR

    static uint8_t clb_mem[1024] __attribute__((aligned(1024)));
    static uint8_t ctbl_mem[256] __attribute__((aligned(128)));

    g_clb  = (HBA_CMD_HEADER*)clb_mem;
    g_ctbl = (HBA_CMD_TBL*)ctbl_mem;

    g_port->clb  = (uint32_t)(uintptr_t)g_clb;
    g_port->clbu = 0;
    g_port->fb   = 0;
    g_port->fbu  = 0;

    // Ein Command Header benutzen (Slot 0)
    for (int i = 0; i < 32; i++) {
        g_clb[i].ctba  = (uint32_t)(uintptr_t)g_ctbl;
        g_clb[i].ctbau = 0;
        g_clb[i].prdtl = 1;
        g_clb[i].prdbc = 0;
        g_clb[i].flags = 0;
    }

    // Port starten
    g_port->cmd |= (1 << 4); // FRE=1
    g_port->cmd |= 1;        // ST=1

    vga_printf(WHITE "<" GREEN " INFO " WHITE "> "
               "AHCI: init done\n");
    return 0;
}

int ahci_read_lba(uint64_t lba, uint32_t count, void* buf) {
    if (!g_port) return -1;

    g_port->is = (uint32_t)-1;

    int slot = 0;
    HBA_CMD_HEADER* cmd = &g_clb[slot];
    cmd->flags = 0;      // keine besonderen Flags
    cmd->prdtl = 1;
    cmd->prdbc = 0;
    cmd->ctba  = (uint32_t)(uintptr_t)g_ctbl;
    cmd->ctbau = 0;

    HBA_CMD_TBL* tbl = g_ctbl;
    for (int i = 0; i < 64; i++) tbl->cfis[i] = 0;

    tbl->prdt[0].dba  = (uint32_t)(uintptr_t)buf;
    tbl->prdt[0].dbau = 0;
    tbl->prdt[0].dbc  = (count * 512) - 1;
    tbl->prdt[0].rsv0 = 0;

    uint8_t* cfis = tbl->cfis;
    cfis[0] = 0x27; // Host to device FIS
    cfis[1] = 1;    // Command
    cfis[2] = 0x25; // READ DMA EXT

    cfis[4] = (uint8_t)(lba);
    cfis[5] = (uint8_t)(lba >> 8);
    cfis[6] = (uint8_t)(lba >> 16);
    cfis[7] = (uint8_t)(lba >> 24);
    cfis[8] = (uint8_t)(lba >> 32);
    cfis[9] = (uint8_t)(lba >> 40);

    cfis[12] = (uint8_t)(count);
    cfis[13] = (uint8_t)(count >> 8);

    // Warten bis nicht busy
    while (g_port->tfd & (0x80 | 0x08)) {}

    g_port->ci = 1u << slot;

    while (g_port->ci & (1u << slot)) {
        if (g_port->is & (1u << 30)) {
            vga_printf(WHITE "<" RED " ERROR " WHITE "> "
                       "AHCI: read error\n");
            return -1;
        }
    }

    if (g_port->is & (1u << 30)) {
        vga_printf(WHITE "<" RED " ERROR " WHITE "> "
                   "AHCI: read error (post)\n");
        return -1;
    }

    return 0;
}

int ahci_has_valid_fat32() {
    uint8_t sector[512];

    ahci_read_lba(0, 1, sector);

    if (sector[510] == 0x55 && sector[511] == 0xAA) {
        return 1; 
    }

    return 0;
}