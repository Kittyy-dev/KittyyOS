[bits 64]

section .text.entry

extern kernel

entry:
    cli
    cld

    mov rax, cr0
    and rax, 0xFFFFFFFFFFFFFFFB
    or  rax, 0x2
    mov cr0, rax

    mov rax, cr4
    or  rax, (1 << 9)
    or  rax, (1 << 10)
    mov cr4, rax

    lea rsp, [rel kstack_top]
    xor rbp, rbp

    jmp kernel

.hang:
    hlt
    jmp .hang

section .bss
align 16
kstack_bottom: resb 16384
kstack_top: