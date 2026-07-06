#include <stdint.h>
#include <ports.h>
#include <vga.h>
#include <ata.h>
#include <string.h>

// ---------------------------------------------------------
// Globale BMIDE-Basisadresse (wird per PCI ermittelt)
// ---------------------------------------------------------
static uint32_t bmide_base = 0;

// PRDT-Struktur
typedef struct {
    uint32_t base;
    uint16_t count;
    uint16_t flags;
} __attribute__((packed)) PRDEntry;

// PRDT + DMA Buffer
static PRDEntry prdt __attribute__((aligned(4)));
static uint8_t dma_buf[128 * 512] __attribute__((aligned(4)));


// ---------------------------------------------------------
// PCI CONFIG SPACE LESEN
// ---------------------------------------------------------
static uint32_t pci_read32(uint8_t bus, uint8_t dev, uint8_t func, uint8_t offset) {
    uint32_t addr = 0x80000000
                  | ((uint32_t)bus  << 16)
                  | ((uint32_t)dev  << 11)
                  | ((uint32_t)func << 8)
                  | (offset & 0xFC);

    outl(0xCF8, addr);
    return inl(0xCFC);
}


// ---------------------------------------------------------
// DMA INITIALISIEREN (BMIDE Base holen)
// ---------------------------------------------------------
void ata_dma_init(void) {
    // QEMU Standard-PC (i440FX/PIIX3):
    // IDE-Controller: Bus 0, Device 1, Function 1, BAR4 = Bus-Master-IDE
    uint32_t bar4 = pci_read32(0, 1, 1, 0x20);
    bmide_base = bar4 & ~0x3;
}


// ---------------------------------------------------------
// ATA Register
// ---------------------------------------------------------
#define ATA_DATA     0x1F0
#define ATA_SECCOUNT 0x1F2
#define ATA_LBA_LOW  0x1F3
#define ATA_LBA_MID  0x1F4
#define ATA_LBA_HIGH 0x1F5
#define ATA_DRIVE    0x1F6
#define ATA_CMD      0x1F7
#define ATA_CMD_WRITE_DMA 0xCA

#define ATA_CMD_READ_DMA 0xC8

// Bus-Master-IDE Register (dynamisch!)
#define BM_CMD      (bmide_base + 0x00)
#define BM_STATUS   (bmide_base + 0x02)
#define BM_PRDT     (bmide_base + 0x04)


// ---------------------------------------------------------
// Hilfsfunktionen
// ---------------------------------------------------------
static inline uint32_t virt_to_phys(void* v) {
    return (uint64_t)v;   // Identity Mapping
}

static inline void ata_wait_ready(void) {
    while (inb(ATA_CMD) & 0x80) { }
}


// ---------------------------------------------------------
// DMA READ
// ---------------------------------------------------------
int ata_dma_read(uint32_t lba, uint16_t count, void* out) {
    if (count == 0 || count > 128 || bmide_base == 0)
        return -1;

    // PRDT vorbereiten
    prdt.base  = virt_to_phys(dma_buf);
    prdt.count = (count * 512) - 1;
    prdt.flags = 0x8000;

    outl(BM_PRDT, virt_to_phys(&prdt));

    // Status löschen
    uint8_t st = inb(BM_STATUS);
    outb(BM_STATUS, st | 0x06);

    ata_wait_ready();

    // LBA + Count setzen
    outb(ATA_SECCOUNT, (uint8_t)count);
    outb(ATA_LBA_LOW,  (uint8_t)(lba));
    outb(ATA_LBA_MID,  (uint8_t)(lba >> 8));
    outb(ATA_LBA_HIGH, (uint8_t)(lba >> 16));
    outb(ATA_DRIVE,    0xE0 | ((lba >> 24) & 0x0F));

    // DMA Richtung + Start
    outb(BM_CMD, 0x08); // read
    outb(BM_CMD, 0x09); // start

    // READ DMA Command
    outb(ATA_CMD, ATA_CMD_READ_DMA);

    // Warten auf DMA-Fertig (Interrupt-Bit)
    uint32_t timeout = 0;
    while (1) {
        st = inb(BM_STATUS);

        if (st & 0x04)   // Interrupt/Fertig
            break;

        if (++timeout > 20000000) {
            vga_print("DMA TIMEOUT\n");
            outb(BM_CMD, 0x08);
            return -1;
        }
    }

    // DMA stoppen
    outb(BM_CMD, 0x08);

    // Fehler?
    if (st & 0x02) {
        // vga_print("DMA ERROR\n"); // wenn's nervt, auskommentieren
        return -1;
    }

    // Daten kopieren
    memcpy(out, dma_buf, count * 512);
    return 0;
}

int ata_dma_write(uint32_t lba, uint16_t count, const void* in) {
    if (count == 0 || count > 128 || bmide_base == 0)
        return -1;

    // Daten in DMA-Puffer kopieren
    memcpy(dma_buf, in, count * 512);

    prdt.base  = virt_to_phys(dma_buf);
    prdt.count = (count * 512) - 1;
    prdt.flags = 0x8000;

    outl(BM_PRDT, virt_to_phys(&prdt));

    uint8_t st = inb(BM_STATUS);
    outb(BM_STATUS, st | 0x06);

    ata_wait_ready();

    outb(ATA_SECCOUNT, (uint8_t)count);
    outb(ATA_LBA_LOW,  (uint8_t)(lba));
    outb(ATA_LBA_MID,  (uint8_t)(lba >> 8));
    outb(ATA_LBA_HIGH, (uint8_t)(lba >> 16));
    outb(ATA_DRIVE,    0xE0 | ((lba >> 24) & 0x0F));

    // DMA Richtung: Write (Bit3 = 0), dann Start
    outb(BM_CMD, 0x00);
    outb(BM_CMD, 0x01);

    outb(ATA_CMD, ATA_CMD_WRITE_DMA);

    uint32_t timeout = 0;
    while (1) {
        st = inb(BM_STATUS);
        if (st & 0x04)
            break;
        if (++timeout > 20000000) {
            vga_print("DMA WRITE TIMEOUT\n");
            outb(BM_CMD, 0x00);
            return -1;
        }
    }

    outb(BM_CMD, 0x00);

    if (st & 0x02) {
        // vga_print("DMA WRITE ERROR\n");
        return -1;
    }

    return 0;
}