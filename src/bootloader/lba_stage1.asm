; KittyyOS Copyright(C)
; Stage1 Bootloader

[org 0x7C00]
[bits 16]

     
db 'KITTYOS'
dw 512               
db 1                 
dw 1                 
db 2                 
dw 224               
dw 0x40, 0x0B
db 0xF0            
dw 9                
dw 18             
dw 2         
dd 0               
dd 0               
db 0                 
db 0                 
db 0x29              
dd 0x12345678        
db 'KITTYOS    '     
db 'FAT12   '        

; dap:
;    db 16
;    db 0
;    dw 1
;    dw 0
;    dw 0
;    dd 0
;    dd 0

start:
    cli
    xor ax, ax
    mov ax, 0x07C0
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7C00

    mov [boot_drive], dl

    mov si, msg - $$

.print_loop:
    lodsb
    cmp al, 0
    je .done
    mov ah, 0x0E
    mov bh, 0
    mov bl, 0x02
    int 0x10
    jmp .print_loop

.done:
    mov ax, 0x0800      
    mov es, ax
    xor bx, bx          

    mov si, 0           

.read_loop:
    mov ah, 0x02        
    mov al, 1           
    mov ch, 0           
    mov dh, 0           
    mov dl, [boot_drive]

    mov cx, si
    add cx, 3        
    mov cl, cl          
    int 0x13
    jc disk_error

    add bx, 512
    inc si
    cmp si, 40
    jl .read_loop

    jmp 0x0800:0x0000

disk_error:
    mov ah, 0x0E
    mov al, 'E'
    int 0x10
    hlt

boot_drive: db 0
msg: db "Welcome to KittyyOS Boootloader", 0x0D, 0x0A, 0x0D, 0x0A, 0

dap: times 16 db 0

times 510 - ($ - $$) db 0
dw 0xAA55
