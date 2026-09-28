#include <vga.h>
#include <colors.h>
#include <panic.h>
#include <isr.h>
#include <kprint.h>

#define REG_R15 0
#define REG_R14 1
#define REG_R13 2
#define REG_R12 3
#define REG_R11 4
#define REG_R10 5
#define REG_R9  6
#define REG_R8  7
#define REG_RDI 8
#define REG_RSI 9
#define REG_RBP 10
#define REG_RBX 11
#define REG_RDX 12
#define REG_RCX 13
#define REG_RAX 14 

static inline uint64_t read_cr2(void) {
    uint64_t value;
    __asm__ volatile("mov %%cr2, %0" : "=r"(value));
    return value;
}

__attribute__((noreturn))
void panic_ex(const char* msg, uint64_t vector, uint64_t error_code, struct isr_frame* f) {
    kprintf(WHITE "<" RED " PANIC " WHITE "> " "KERNEL PANIC!\n\n"); 
    kprintf(WHITE "<" YELLOW " INFO " WHITE "> ");
    kprintf("INT %x EC %x RIP %x CS %x RFLAGS %x\n", (unsigned)vector, (unsigned)error_code, f->regs[2], f->regs[3], f->regs[4]);

    uint64_t addr = read_cr2();
    uint32_t high = (uint32_t)(addr >> 32);
    uint32_t low  = (uint32_t)(addr & 0xFFFFFFFF);

    kprintf(WHITE "<" YELLOW " INFO " WHITE "> " "Fault at: %x%x\n\n", high, low);

    for (int i = 0; i < 15; i++) {
        kprintf(WHITE "<" YELLOW " INFO " WHITE "> " "REG[%d]: %x\n", i, f->regs[i]);
    }
    
    kprintf("\n");

    kprintf(WHITE "<" RED " ERROR " WHITE "> %s\n", msg);
    kprintf("\nSystem halted!");

    for (;;) {
        __asm__("hlt");
    }
}

void panic(const char* msg) {
    kclear_screen();

    kprintf(WHITE "<" RED " PANIC " WHITE "> " "KERNEL PANIC!\n\n"); 
    kprintf(WHITE "<" RED " ERROR " WHITE "> %s\n", msg);
    kprintf("\nSystem halted!");

    for (;;) {
        __asm__("hlt");
    }
}