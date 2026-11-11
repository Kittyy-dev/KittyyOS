global context_switch

section .text

context_switch:
    ; RDI = old
    ; RSI = new

    push rbx
    push rbp
    push r12
    push r13
    push r14
    push r15

    test rdi, rdi
    jz .no_old

    mov [rdi + 0x10], rsp

.no_old:
    mov rsp, [rsi + 0x10]

    pop r15
    pop r14
    pop r13
    pop r12
    pop rbp
    pop rbx

    ret