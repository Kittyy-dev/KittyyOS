#include <gdt.h>

extern void gdt_load_and_jump(void *desc);

static struct gdt_entry gdt[3];
static struct gdt_ptr   gdt_desc;

static void gdt_set_entry(int i, uint32_t base, uint32_t limit, uint8_t access, uint8_t gran) {
    gdt[i].limit_low   = limit & 0xFFFF;
    gdt[i].base_low    = base & 0xFFFF;
    gdt[i].base_mid    = (base >> 16) & 0xFF;
    gdt[i].access      = access;
    gdt[i].granularity = ((limit >> 16) & 0x0F) | (gran & 0xF0);
    gdt[i].base_high   = (base >> 24) & 0xFF;
}

extern void gdt_load_and_jump(void *desc);

void gdt_init(void) {
    gdt_set_entry(0, 0, 0, 0, 0);
    gdt_set_entry(1, 0, 0xFFFFF, 0x9A, 0x20 | 0x80);
    gdt_set_entry(2, 0, 0xFFFFF, 0x92, 0xC0);

    gdt_desc.limit = sizeof(gdt) - 1;
    gdt_desc.base  = (uint64_t)&gdt;

    gdt_load_and_jump(&gdt_desc);
}