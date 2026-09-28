/*
KittyyOS Copyright(C)
ISR-Handler
*/

#include <stdint.h>
#include <ports.h>
#include <idt.h>
#include <vga.h>
#include <colors.h>
#include <isr.h>
#include <panic.h>
#include <pit.h>

extern void irq0_handler(void);
extern void xhci_irq_handler(void);
extern void keyboard_handler(void);

struct isr_frame {
    uint64_t regs[15];     

    uint64_t error_code;   
    uint64_t vector;       

    uint64_t rip;
    uint64_t cs;
    uint64_t rflags;
    uint64_t rsp;
    uint64_t ss;
};

struct raw_isr_frame {
    uint64_t regs[15];   

    uint64_t vector;    
    uint64_t error_code; 
    uint64_t rip;        
    uint64_t cs;         
    uint64_t rflags;     
};

static isr_handler_t interrupt_handlers[256] = { 0 };

void register_interrupt_handler(uint8_t n, isr_handler_t handler) {
    interrupt_handlers[n] = handler;
}

void pic_remap() {
    // Remap
    outb(0x20, 0x11);
    outb(0xA0, 0x11);
    outb(0x21, 0x20);
    outb(0xA1, 0x28);
    outb(0x21, 0x04);
    outb(0xA1, 0x02);
    outb(0x21, 0x01);
    outb(0xA1, 0x01);

    outb(0x21, 0xFF);
    outb(0xA1, 0xFF);
}

/*
void gpf(uint64_t vector, uint64_t error_code, void* frame_ptr_raw) {
    panic_ex("GENERAL PROTECTION FAULT", vector, error_code, &f);
}

void page_fault(uint64_t vector, uint64_t error_code, void* frame_ptr_raw) {
    panic_ex("PAGE FAULT", vector, error_code, &f);
}

void invalid_opcode(uint64_t vector, uint64_t error_code, void* frame_ptr_raw) {
    panic_ex("INVALID OPCODE", vector, error_code, &f);
}

void double_fault(uint64_t vector, uint64_t error_code, void* frame_ptr_raw) {
    panic_ex("DOUBLE FAULT", vector, error_code, &f);
}

void de(uint64_t vector, uint64_t error_code, void* frame_ptr_raw) {
    panic_ex("DEVIDE ERROR", vector, error_code, &f);
} */

void isr_install() {
    pic_remap();

    uint8_t mask = inb(0x21);
    mask &= ~(1 << 0);
    mask &= ~(1 << 1);
    outb(0x21, mask);

    // register_interrupt_handler(13, gpf);
    register_interrupt_handler(33, keyboard_handler);
    register_interrupt_handler(32, irq0_handler);
}

void isr_common_handler(uint64_t vector, uint64_t error_code, void* frame_ptr_raw) {
    struct raw_isr_frame* r = (struct raw_isr_frame*)frame_ptr_raw;

    struct isr_frame f;

    for (int i = 0; i < 15; i++) {
        f.regs[i] = r->regs[i];
    }

    f.error_code = r->error_code;
    f.vector     = r->vector;
    f.rip        = r->rip;
    f.cs         = r->cs;
    f.rflags     = r->rflags;

    f.rsp = 0;
    f.ss  = 0;

    if (vector == 0) {
        panic_ex("DIVIDE ERROR", vector, error_code, &f);
    }

    if (vector == 13) {
        panic_ex("GENERAL PROTECTION FAULT", vector, error_code, &f);
    }

    if (vector == 14) {
        panic_ex("PAGE FAULT", vector, error_code, &f);
    }

    if (vector == 6) {
        panic_ex("INVALID OPCODE", vector, error_code, &f);
    }

    if (vector == 8) {
        panic_ex("DOUBLE FAULT", vector, error_code, &f);
    }

    if (interrupt_handlers[vector]) {
        interrupt_handlers[vector]();
    } else {
        panic_ex("UNHANDLED INTERRUPT", vector, error_code, &f);
        return;
    }

    if (vector >= 0x20 && vector <= 0x2F) {
        if (vector >= 0x28) {
            outb(0xA0, 0x20);
        }
        outb(0x20, 0x20);
    }
}

void enable_interrupts() {
    asm volatile ("sti");
}