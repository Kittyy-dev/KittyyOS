section .text.initrd

global set_stack
; extern kernel_main

set_stack:
    mov rsp, 0x100000
    and rsp, - 16
    ret
    ; jmp kernel_main