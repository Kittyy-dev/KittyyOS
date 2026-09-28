global enter_userspace

enter_userspace:
    ; RDI = user RIP
    ; RSI = user RSP

    cli

    push 0x23        ; SS
    push rsi         ; RSP

    pushfq
    pop rax
    or rax, 0x200    ; IF = 1
    push rax         ; RFLAGS

    push 0x1B        ; CS
    push rdi         ; RIP

    iretq