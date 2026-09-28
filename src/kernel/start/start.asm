[bits 64]

section .text.entry

extern __bss_start
extern __bss_end
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

    mov rsp, 0x90000
    and rsp, -16

    call kernel

.hang:
    hlt
    jmp .hang
