global gdt_load_and_jump
global tss_load

gdt_load_and_jump:
    lgdt [rdi]

    push qword 0x08
    lea rax, [rel .reload_cs]
    push rax
    retfq

.reload_cs:
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov ss, ax
    ret

tss_load:
    mov ax, 0x28
    ltr ax
    ret