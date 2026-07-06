#include <stdint.h>
#include <vga.h>
#include <panic.h>
#include <colors.h>
#include <isr.h>
#include <idt.h>
#include <process.h>
#include <flush.h>
#include <keyboard.h>
#include <syscall_handler.h>
#include <gdt.h>
#include <string.h>
#include <bios.h>
#include <paging.h>
#include <fat.h>
#include <ata.h>
#include <cd.h>
#include <ahci.h>
#include <storage.h>
#include <kernel.h>
#include <pmm.h>
#include <e820.h>
#include <tsc.h>

// extern void pic_remap();

uint16_t e820_count;

extern bool keyboard_enabled;
extern void shell();
extern uint64_t* pml4;

// gdt start
typedef struct {
    uint16_t limit;
    uint64_t base;
} __attribute__((packed)) GDTR;

typedef struct {
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t  base_mid;
    uint8_t  access;
    uint8_t  granularity;
    uint8_t  base_high;  
} __attribute__((packed)) GDTEntry;

static inline GDTR get_gdtr(void) {
    GDTR gdtr;

    __asm__ volatile("sgdt %0" : "=m"(gdtr));

    return gdtr;
}

void check_gdt(void) {
    GDTR gdtr = get_gdtr();

    vga_printf(WHITE "<" YELLOW " INFO " WHITE "> GDT BASE:  0x%x\n", (uint32_t)gdtr.base);
    vga_printf(WHITE "<" YELLOW " INFO " WHITE "> GDT LIMIT: 0x%x\n", gdtr.limit);

    GDTEntry* gdt = (GDTEntry*)gdtr.base;

    int entries = (gdtr.limit + 1) / sizeof(GDTEntry);

    for (int i = 0; i < entries; i++) {
        uint32_t base = (gdt[i].base_low) | ((uint32_t)gdt[i].base_mid << 16) | ((uint32_t)gdt[i].base_high << 24);

        uint32_t limit = (gdt[i].base_low) | ((uint32_t)(gdt[i].granularity & 0x0F) << 16);

        uint8_t access = gdt[i].access;
        uint8_t flags = (gdt[i].granularity >> 4) & 0x0F;

        vga_printf(WHITE "<" YELLOW " INFO " WHITE "> GDT[%d] BASE: 0x%x LIMIT: 0x%x ACCESS=0%x FLAGS=0x%x\n", i, base, limit, access, flags);
    }
}

// gdt end

// idt start
extern char out[256];

typedef struct {
    uint16_t limit;
    uint64_t base;
} __attribute__((packed)) IDTR;

typedef struct {
    uint16_t offset_low;
    uint16_t selector;
    uint8_t  ist;
    uint8_t  type_attr;
    uint16_t offset_mid;
    uint32_t offset_high;
    uint32_t zero;
} __attribute__((packed)) IDTEntry64;

IDTR get_idtr(void) {
    IDTR idtr;
    __asm__ volatile("sidt %0" : "=m"(idtr));
    return idtr;
}

// idt end

// idt check 
void check_idt(void) { 
    IDTR idtr = get_idtr();

    vga_printf(WHITE "<" YELLOW " INFO " WHITE "> " "IDT BASE: 0x%x\n", (uint32_t)idtr.base);
    vga_printf(WHITE "<" YELLOW " INFO " WHITE "> " "IDT LIMIT: 0x%x\n", idtr.base);

    IDTEntry64* idt = (IDTEntry64*)idtr.base;

    for (int i = 0; i < 5; i++) {
        uint64_t offset =
            ((uint64_t)idt[i].offset_high << 32) |
            ((uint64_t)idt[i].offset_mid << 16) |
            idt[i].offset_low;

        vga_printf(
            WHITE "<" YELLOW " INFO " WHITE "> " "IDT[%d] OFF=0x%x SEL=0x%x FLAGS=0x%x\n",
            i,
            (uint32_t)offset,
            idt[i].selector,
            idt[i].type_attr
        );
    }
}

// idt main function
void idt_start() {
    isr_install();
    idt_init();
    // register_interrupt_handler(33, keyboard_handler);
}

void init_system() {
    // paging

    paging_init();
    acpi_map_bios_region();

    // gdt part
    gdt_init();
    vga_printf(WHITE "<" GREEN " KERNEL " WHITE "> " "GDT Loaded!\n\n");
    check_gdt();

    // idt part
    idt_start();
    vga_printf("\n" WHITE "<" GREEN " KERNEL " WHITE "> " "IDT Loaded!\n\n");
    check_idt();
    vga_printf("\n");

    // e820_scan();

    vga_printf(WHITE "<" GREEN " KERNEL " WHITE "> " "Finding usable RAM\n");
    uint64_t mem_size = get_ram_size();

    pmm_init(mem_size);

    // time

    vga_printf(WHITE "<" GREEN " KERNEL " WHITE "> " "Calibrating TSC!\n");
    calibrate_tsc();

    // Boot device check
    storage_init();

    /*
    if (g_use_usb) {
        vga_printf(WHITE "<" GREEN " KERNEL " WHITE "> " "USB storage selected.\n");
    } else if (g_use_ahci) {
        // vga_printf(WHITE "<" GREEN " KERNEL " WHITE "> " "AHCI storage selected.\n");
    } else {
        vga_printf(WHITE "<" GREEN " KERNEL " WHITE "> " "Using IDE fallback.\n");
    }
    */

    // fat32
    vga_printf("\n" WHITE "<" GREEN " KERNEL " WHITE "> " "Loading FAT32...\n\n");
    ata_dma_init();
    fat32_mount(6144);
    fat32_set_current_dir(bpb.rootCluster);
    vga_printf("INIT DEBUG: rootCluster=%d\n", (int)bpb.rootCluster);

    /*
    uint8_t test[512];
    read_sectors_abs(0, test, 1);

    vga_print("TEST MBR DUMP:\n");
    for (int i = 0; i < 64; i++) {
        vga_printf("%x ", test[i]);
        vga_print("\n");
    }
    */


    // scroll_screen();

    // enable interrputs
    enable_interrupts();
}

void kernel (void) {
    // welcome
    clear_screen();
    vga_printf(WHITE "<" GREEN " KERNEL " WHITE "> " "KittyyOS Copyright(C)\n\n");
    init_system();

    keyboard_enabled = true; // Turn keyboard on or off

    vga_printf("\nUsing fallback shell!\n\n");
    while (1) {
        shell();
    }


    // cli();
    // cld();
    // sti();

    // volatile int *ptr = (int*)0xDEADBEEF;
    // int x = *ptr;

    // flush(out, sizeof(out));


    // __asm__ volatile ("hlt");
}