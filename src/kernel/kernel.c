// KittyyOS Copyright(C) OPSEC LEVEL = UNTER DAS OS

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
#include <e820.h>
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
#include <kernel_api.h>
#include <cpu.h>
#include <pci.h>
#include <gop.h>
#include <devfs.h>

extern void irq0_stub(void);    

// extern void pic_remap();

extern void check_video_mode(void);

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

uint64_t get_memory_mb(void) {
    return mb;
}

void init_system() {
    // gop_init();
    // gop_putchar_at(0, 0, 'T',  0xFFFFFF);

    heap_init();

    acpi_map_bios_region();

    // gdt part
    gdt_init();
    kprintf(WHITE "<" GREEN " KERNEL " WHITE "> " "GDT Loaded!\n");

    pit_init(100);

    // idt part
    idt_start();
    kprintf(WHITE "<" GREEN " KERNEL " WHITE "> " "IDT Loaded!\n");

    if (is_bios_boot()) {
        kprintf(WHITE "<" GREEN " KERNEL " WHITE "> " "KittyyOS BIOS Version\n");
        mem_size = get_ram_size();
        mb = mem_size / (1024 * 1024);
        kprintf(WHITE "<" GREEN " KERNEL " WHITE "> " "Found Usable RAM: %u MB\n", mb);
    } else {
        kprintf(WHITE "<" GREEN " KERNEL " WHITE "> " "KittyyOS UEFI Version\n");
    }

    pmm_init(mem_size);

    // time

    kprintf(WHITE "<" GREEN " KERNEL " WHITE "> " "Calibrating TSC...\n");

    uint32_t eax;

    calibrate_tsc();

    int size = heap_size();

    kprintf(WHITE "<" GREEN " KERNEL " WHITE "> " "Heap Size: %d\n", size);

    // Boot device check
    storage_init();

    // ramfs root
    ramfs_init();
    vfs_register(&ramfs_fs);
    vfs_mount("/", "ramfs_fs");
    kprintf(WHITE "<" GREEN " KERNEL " WHITE "> " "RAMFS mounted!\n");

    // fat32
    kprintf(WHITE "<" GREEN " KERNEL " WHITE "> " "Loading FAT32...\n");
    ata_dma_init();
    fat32_mount(6144);
    vfs_register(&fat32_fs);
    vfs_mount("/", "fat32");

    // Devfs
    vfs_register(&devfs);
    vfs_mkdir(vfs_get_root(), "dev");
    vfs_mount("/dev", "devfs");
    devfs_init();

    vfs_node_t *null = vfs_resolve_path("/dev/null");
    
    kprintf(WHITE "<" GREEN " KERNEL " WHITE "> " "DevFS mounted!\n");

    // vfs_node_t* root = fat32_fs.mount();

    if (vfs_root) {
        kprintf(WHITE "<" GREEN " KERNEL " WHITE "> " "Fat32 mounted!\n");
    } else {
        kprintf(WHITE "<" RED " ERROR " WHITE "> " "Fat32 not mounted!\n");
    }

    // tmp fs
    vfs_register(&ramfs_fs);
    vfs_mkdir(vfs_root, "tmp");
    vfs_mount("/tmp", "ramfs_fs");
    kprintf(WHITE "<" GREEN " KERNEL " WHITE "> " "TMPFS mounted!\n");

    fat32_set_current_dir(bpb.rootCluster);

    // enable interrputs

    vfs_cwd = vfs_root;

    enable_interrupts(); // Crash in uefi mode

    keyboard_enabled = true;

    kernel_load_module("/bin/hh.bin"); // Higher Half Entry

    // register_interrupt_handler(32, irq0_stub);

    // pit_init(100);
}

/*
void crash(void) {
    volatile int zero = 0;
    volatile int result = 10 / zero;
    (void)result;
}
*/

void kernel (void) {
    check_video_mode();

    // int cx = vbe.width / 2;
    // int cy = vbe.height / 2;

    // vbe_draw_circle(cx, cy, 200, 0x00FF00);

    init_system();

    // crash(); // Turn keyboard on or off
    // uint64_t last = 0;

    while (1) {
        /*
        uint64_t ticks = timer_get_ticks();

        if (ticks != last) {
            last = ticks;

            if (ticks % 100 == 0) {
                kprintf("TIMER: %llu\n", ticks);
            }
        } */

        // asm volatile ("hlt");
    }
}
