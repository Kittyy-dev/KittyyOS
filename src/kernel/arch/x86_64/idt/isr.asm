global isr_stub_table
global isr_common_stub
extern isr_common_handler

section .text

%macro ISR_NOERR 1
isr_stub_%1:
    push qword 0        
    push qword %1        
    jmp isr_common_stub
%endmacro

; Exceptions MIT Error-Code
%macro ISR_ERR 1
isr_stub_%1:
    push qword %1
    jmp isr_common_stub
%endmacro

; ------------------------------------------------------------
;  Stub table (256 entries)
; ------------------------------------------------------------

isr_stub_table:
%assign i 0
%rep 256
    dq isr_stub_%+i
%assign i i+1
%endrep

; ------------------------------------------------------------
;  Generate all 256 stubs
; ------------------------------------------------------------

%assign i 0
%rep 256

%if (i == 8) || (i == 10) || (i == 11) || (i == 12) || (i == 13) || (i == 14) || (i == 17)
    ISR_ERR i
%else
    ISR_NOERR i
%endif

%assign i i+1
%endrep

; ------------------------------------------------------------
;  Common stub
; ------------------------------------------------------------

isr_common_stub:
    push rax
    push rcx
    push rdx
    push rbx
    push rbp
    push rsi
    push rdi
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15

    mov rax, rsp

    ; mov rsi, [rsp + 15*8]    
    ; mov rdi, [rsp + 16*8]    
    ; mov rdx, rsp          
    
    mov rdi, [rsp + 0x78]    
    mov rsi, [rsp + 0x80]    

    lea rdx, [rsp + 0x78]

    ; lea rdx, [rsp + 17*8] 

    ; mov rdx, rsp  

    call isr_common_handler

    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rdi
    pop rsi
    pop rbp
    pop rbx
    pop rdx
    pop rcx
    pop rax

    add rsp, 16 ; 8

    iretq