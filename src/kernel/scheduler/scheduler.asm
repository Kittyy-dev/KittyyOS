global task_trampoline

extern task_exit

section .text

task_trampoline:
    pop rax
    
    and rsp, -16

    call rax

    call task_exit

.hang:
    cli
    hlt
    jmp .hang