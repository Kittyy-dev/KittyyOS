[bits 64]

section .text.entry

extern __bss_start
extern __bss_end
extern __stack_end
extern kernel

entry:
    cli
    cld

    mov rax, cr0
    and rax, 0xFFFFFFFFFFFFFFFB
    or rax, 0x2
    mov cr0, rax
    mov rax, cr4
    or rax, (1 << 9)
    or rax, (1 << 10)
    mov cr4, rax

    ; BSS nullen
    lea rdi, [rel __bss_start]
    lea rcx, [rel __bss_end]
    sub rcx, rdi
    xor rax, rax
    rep stosb 

    mov rsp, 0x90000
    and rsp, -16
    
    call kernel

.hang:
    hlt
    jmp .hang
