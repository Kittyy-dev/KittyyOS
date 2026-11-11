global isr_stub_table
global isr_common_stub
extern isr_common_handler

section .text

%macro ISR_NOERR 1
isr_stub_%1:
    push qword 0        
    push qword %1        
    jmp isr_common_stub
%endmacro

%macro ISR_ERR 1
isr_stub_%1:
    push qword %1
    jmp isr_common_stub
%endmacro

isr_stub_table:
%assign i 0
%rep 256
    dq isr_stub_%+i
%assign i i+1
%endrep

%assign i 0
%rep 256

%if (i == 8) || (i == 10) || (i == 11) || (i == 12) || (i == 13) || (i == 14) || (i == 17)
    ISR_ERR i
%else
    ISR_NOERR i
%endif

%assign i i+1
%endrep

isr_common_stub:
    push rax
    push rcx
    push rdx
    push rbx
    push rbp
    push rsi
    push rdi
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15

    mov r12, rsp

    mov rdi, [r12 + 0x78]
    mov rsi, [r12 + 0x80]
    mov rdx, r12

    and rsp, -16
    call isr_common_handler

    mov rsp, r12

    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rdi
    pop rsi
    pop rbp
    pop rbx
    pop rdx
    pop rcx
    pop rax

    add rsp, 16

    iretq