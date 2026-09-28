global task_trampoline

section .text

task_trampoline:
    pop rax        ; entry
    pop rdi        ; KernelAPI*
    call rax

.hang:
    cli
    hlt
    jmp .hang