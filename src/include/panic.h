#include <isr.h>

#pragma once

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

void panic(const char* msg);
void panic_ex(const char* msg, uint64_t vector, uint64_t error_code, struct isr_frame* f);