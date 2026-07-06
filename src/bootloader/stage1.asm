; KittyyOS Copyright(C)
; Stage1 Bootloader

[org 0x7C00] ; Offset
[bits 16] ; 16 Bit Code

DAP_ADDR equ 0x0200 ; DAP ADDR = 0x0200

start: ; Code entry
    cli ; Clear interrupts and BIOS shit
    xor ax, ax
    mov ax, 0x07C0
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7C00

    mov [boot_drive], dl ; Variable to move dl into stage2

    mov si, msg - $$ ; Put the msg in si to print si

.print_loop: ; Print function
    lodsb
    cmp al, 0
    je .done
    mov ah, 0x0E
    mov bh, 0
    mov bl, 0x02
    int 0x10
    jmp .print_loop

.done: ; If printing is done 
    mov ax, 0x0800      
    mov es, ax
    xor bx, bx          
    xor si, si

    ; mov si, 0        

.read_loop: ; Loop to read more sectors than 1
    mov byte [DAP_ADDR], 16     
    mov byte [DAP_ADDR+1], 0    
    mov word [DAP_ADDR+2], 1    
    mov word [DAP_ADDR+4], bx   
    mov word [DAP_ADDR+6], es   

    mov ax, si
    add ax, 3
    mov [DAP_ADDR+8], ax      
    mov word [DAP_ADDR+10], 0    
    mov dword [DAP_ADDR+12], 0   

    mov ah, 0x42            
    mov [boot_drive], dl
    mov si, DAP_ADDR
    int 0x13
    jc disk_error

    add bx, 512             
    inc si
    cmp si, 104       
    jl .read_loop

    jmp 0x0800:0x0000

disk_error: ; If Error this function prints E
    mov ah, 0x0E
    mov al, 'E'
    int 0x10
    hlt

boot_drive: db 0 ; Uninitialized Variable for dl 
msg: db "KittyyOS: Loading Kernel!", 0x0D, 0x0A, 0x0D, 0x0A, 0 ; Hello Msg
