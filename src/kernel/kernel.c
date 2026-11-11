// KittyyOS Copyright(C) OPSEC LEVEL = UNTER DAS OS

#include <net.h>
#include <stdint.h>
#include <vga.h>
#include <panic.h>
#include <colors.h>
#include <isr.h>
#include <idt.h>
#include <flush.h>
#include <keyboard.h>
#include <gdt.h>
#include <bios.h>
#include <paging.h>
#include <fat.h>
#include <ata.h>
#include <cd.h>
#include <storage.h>
#include <kernel.h>
#include <pmm.h>
#include <mem.h>
#include <tsc.h>
#include <types.h>
#include <sound.h>
#include <vfs.h>
#include <ramfs.h>
#include <tools.h>
#include <vbe.h>
#include <vbecolors.h>
#include <heap.h>
#include <pit.h>
#include <kprint.h>
#include <cpu.h>
#include <pci.h>
#include <devfs.h>
#include <memtest.h>
#include <e1000.h>

const uint8_t gateway_ip[4] = {10, 0, 2, 2};

extern void irq0_stub(void);    

extern void check_video_mode(void);
extern void scheduler_start(void);

extern VBEInfo vbe;

vfs_node_t* vfs_cwd;

uint16_t e820_count;

uint64_t mem_size;
uint64_t mb;

extern bool keyboard_enabled;
extern void shell();
extern uint64_t* pml4;

// idt main function
void idt_start() {
    isr_install();
    idt_init();
}

void init_system() {
    mem_size = get_ram_size();
    mb = mem_size / (1024 * 1024);
    log_kernel("RAM: %uMB\n", mb);

    // PMM
    pmm_init(get_ram_size());

    // HEAP
    heap_init();

    // Test
    log_kernel("Testing memory...\n");
    test_hhdm();
    test_pmm();

    acpi_map_bios_region();

    // GDT
    gdt_init();
    log_kernel("GDT Loaded!\n");

    // IDT
    idt_start();
    log_kernel("IDT Loaded!\n");

    // PIT
    pit_init(100);

    // TSC
    uint32_t eax;
    calibrate_tsc();
    log_kernel("TSC calibrated!\n");

    // Boot device check
    storage_init();

    // WiFi
    if (pci_network_found && pci_network_vendor == 0x8086 && pci_network_device == 0x100E) {
        log_kernel("NET: Initailizing e1000 driver...\n");

        // __asm__ volatile ("int $14");

        if (e1000_map_mmio()) {
            e1000_test_mmio();
            pci_enable_mmio_busmaster(pci_network_bus, pci_network_slot, pci_network_func);
            e1000_reset();
            e1000_read_mac();
            e1000_init_rx();
            e1000_init_tx();
            arp_init();
            log_kernel("e1000: Network device ready!\n");

            // arp_request(gateway_ip);
            // e1000_test_send();

            for (volatile unsigned int i = 0; i < 1000000; i++) {
                asm volatile("pause");
            }
        } else {
            log_error("e1000: MMIO setup failed!\n");
        }
    }

    // VFS Root
    // vfs_init();

    // fat32
    ata_dma_init();
    fat32_mount(6144);
    vfs_register(&fat32_fs);
    vfs_mount("/", "fat32");
    log_kernel("FAT32 loaded!\n");

    if (vfs_root) {
        log_kernel("FAT32 mounted!\n");
    } else {
        log_error("FAT32 not mounted!\n");
    }

    // tmp fs
    vfs_register(&ramfs_fs);
    vfs_mkdir(vfs_root, "tmp");
    vfs_mount("/tmp", "ramfs_fs");
    log_kernel("TmpFS mounted!\n");

    // Devfs
    vfs_register(&devfs);
    vfs_node_t *dev_dir = vfs_mkdir(vfs_root, "dev");
    if (!dev_dir) {
        log_error("Failed to create /dev!\n");
        return;
    }

    if (!vfs_mount("/dev", "devfs")) {
        log_error("Failed to mount DevFS");
        return;
    }

    devfs_init();
    log_kernel("DevFS mounted!\n");

    // vfs_node_t* root = fat32_fs.mount();

    fat32_set_current_dir(bpb.rootCluster);

    enable_interrupts();
    log_kernel("Enabled interrupts!\n");

    vfs_cwd = vfs_root;

    scheduler_start();
}

void kernel (void) {
    check_video_mode();

    // int cx = vbe.width / 2;
    // int cy = vbe.height / 2;

    init_system();
    // vbe_draw_circle(cx, cy, 200, 0x00FF00);

    while (1) {}
}
