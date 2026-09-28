global enter_userspace

enter_userspace:
    push rcx
    push rsi

    pushfq
    pop rax
    or rax, 0x200
    push rax

    push rdx
    push rdi

    iretq