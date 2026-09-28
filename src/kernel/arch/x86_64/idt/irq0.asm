global irq0_stub
 
extern irq0_handler

irq0_stub:
    push rax
    push rbx
    push rcx
    push rdx

    push rsi
    push rdi

    push r8
    push r9
    push r10
    
    call irq0_handler

    pop r11
    pop r10
    pop r9
    pop r8

    pop rdi
    pop rsi
    
    pop rdx
    pop rcx
    pop rbx
    pop rax

    iretq