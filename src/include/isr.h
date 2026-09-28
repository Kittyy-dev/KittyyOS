#ifndef ISR_H
#define ISR_H

#include <stdint.h>

typedef struct {
    uint64_t ds;
    uint64_t rdi, rsi, rbp, rsp, rbx, rdx, rcx, rax;
    uint64_t int_no, err_code;
    uint64_t rip, cs, rflags, useresp, ss;
} registers_t;

typedef void (*isr_handler_t)(void);

void isr_install();

void enable_interrupts();

void register_interrupt_handler(uint8_t n, isr_handler_t handler);

void isr_common_handler(uint64_t vector, uint64_t error_code, void* frame_ptr);

#endif