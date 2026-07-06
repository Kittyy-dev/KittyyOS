[BITS 64]

section .text

global context_switch

context_switch:
    test rdi, rdi
    jz .skip_save

    mov qword [rdi + 0x00], rsp
    mov qword [rdi + 0x08], rbx
    mov qword [rdi + 0x10], rbp
    mov qword [rdi + 0x18], r12
    mov qword [rdi + 0x20], r13
    mov qword [rdi + 0x28], r14
    mov qword [rdi + 0x30], r15

.skip_save:
    mov rsp, qword [rsi + 0x00]
    mov rbx, qword [rsi + 0x08]
    mov rbp, qword [rsi + 0x10]
    mov r12, qword [rsi + 0x18]
    mov r13, qword [rsi + 0x20]
    mov r14, qword [rsi + 0x28]
    mov r15, qword [rsi + 0x30]

    ret
