#include <idt.h>
#include <vga.h>
#include <colors.h>
#include <ports.h>
#include <isr.h>

extern void default_interrupt_stub();
extern void* isr_stub_table[];

void remap() {
    outb(0x20, 0x11);
    outb(0xA0, 0x11);
    outb(0x21, 0x20);
    outb(0xA1, 0x28);
    outb(0x21, 0x04);
    outb(0xA1, 0x02);
    outb(0x21, 0x01);
    outb(0xA1, 0x01);
    outb(0x21, 0x0);
    outb(0xA1, 0x0);
}

void idt_init() {
    for (int i = 0; i < 256; i++) {
        set_idt_entry(i, isr_stub_table[i]);
    }

    set_idt_entry(33, isr_stub_table[33]);

    // remap();

    load_idt();
}

struct idt_entry idt[IDT_ENTRIES];

void set_idt_entry(int vector, void (*handler)(void)) {
    uint64_t addr = (uint64_t)handler;

    idt[vector].offset_low  = addr & 0xFFFF;
    idt[vector].selector    = 0x08;
    idt[vector].ist         = 0;
    idt[vector].type_attr   = 0x8E;
    idt[vector].offset_mid  = (addr >> 16) & 0xFFFF;
    idt[vector].offset_high = (addr >> 32) & 0xFFFFFFFF;
    idt[vector].zero        = 0;
}

void load_idt() {
    struct idt_ptr idtp;
    idtp.limit = sizeof(idt) - 1;
    idtp.base  = (uint64_t)&idt;

    __asm__ volatile ("lidt %0" : : "m"(idtp));
}