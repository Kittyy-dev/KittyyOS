#include "pmm.h"
#include <gdt.h>
#include <stdint.h>
#include <string.h>
#include <tss.h>
#include <panic.h>

extern void gdt_load_and_jump(void *desc);

struct __attribute__((packed)) {
    struct gdt_entry entries[5];
    struct gdt_tss_entry tss;
} gdt;

static struct gdt_ptr   gdt_desc;

static void gdt_set_entry(int i, uint32_t base, uint32_t limit, uint8_t access, uint8_t gran) {
    gdt.entries[i].limit_low   = limit & 0xFFFF;
    gdt.entries[i].base_low    = base & 0xFFFF;
    gdt.entries[i].base_mid    = (base >> 16) & 0xFF;
    gdt.entries[i].access      = access;
    gdt.entries[i].granularity = ((limit >> 16) & 0x0F) | (gran & 0xF0);
    gdt.entries[i].base_high   = (base >> 24) & 0xFF;
}

extern void gdt_load_and_jump(void *desc);

void gdt_set_tss(struct gdt_tss_entry *entry, struct tss *tss) {
    uint64_t base = (uint64_t)tss;
    uint32_t limit = sizeof(struct tss) - 1;

    entry->limit_low = limit & 0xFFFF;
    entry->base_low = base & 0xFFFF;
    entry->base_mid = (base >> 16) & 0xFF;
    entry->access = 0x89;              // Present | Available 64-bit TSS
    entry->granularity = (limit >> 16) & 0x0F;
    entry->base_high = (base >> 24) & 0xFF;
    entry->base_upper = (base >> 32) & 0xFFFFFFFF;
    entry->reserved = 0;
}

void gdt_init(void) {
    static struct tss kernel_tss;
    gdt_set_entry(0, 0, 0, 0, 0);

    // Kernel
    gdt_set_entry(1, 0, 0xFFFFF, 0x9A, 0x20 | 0x80);
    gdt_set_entry(2, 0, 0xFFFFF, 0x92, 0xC0);

    // User
    gdt_set_entry(3, 0, 0xFFFFF, 0xFA, 0xCF);
    gdt_set_entry(4, 0, 0xFFFFF, 0xF2, 0xCF);

    // Stack

    uint64_t stack = pmm_alloc_pages(4);

    if (stack == 0) {
        panic("No memory for kernel stack!\n");
    } 

    memset(&kernel_tss, 0, sizeof(kernel_tss));

    kernel_tss.rsp0 = stack + (4 * PAGE_SIZE);
    kernel_tss.iomap_base = sizeof(struct tss);

    gdt_set_tss(&gdt.tss, &kernel_tss);

    gdt_desc.limit = sizeof(gdt) - 1;
    gdt_desc.base  = (uint64_t)&gdt;

    gdt_load_and_jump(&gdt_desc);
}