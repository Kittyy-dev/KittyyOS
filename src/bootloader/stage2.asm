DEFAULT ABS

[org 0x0000]
[bits 16]

BOOT_INFO_ADDR equ 0x3000
VBE_OEM_ADDR equ 0x4400

DAP2: times 16 db 0

start:
    cli
    cld
    mov ax, cs
    mov ds, ax
    mov dl, 0x80

    call get_e820
    call get_vbe_info
    call vesa_string
    call get_vbe_mode_info

    mov ax, cs
    mov ds, ax
    mov si, vbe_mode_info_block

    mov ax, 0x0000
    mov es, ax
    mov di, 0x4000

    mov cx, 256
    rep movsb

    call set_vbe_mode ; set vbe mode on

    ; call set_text_mode

    call copy_boot_info

    ; hlt

    ; call copy_e820_to_kernel

    mov ax, 0x2000
    mov es, ax
    mov bx, 0

    mov ax, 2085
    mov cx, 40
    call load_file

    mov ah, 0x0E
    mov al, 'O'
    int 0x10

    ; mov cx, [e820_entries]   
    mov ax, 0x0000
    mov es, ax
    mov di, 0x5000 

    jmp 0x2000:0x0000

    mov ah, 0x0E
    mov al, 'F'

    int 0x10

; Get e820

vesa_string:
    mov ax, cs
    mov ds, ax
    
    mov si, [vbe_ctrl_info_block + 0x06]
    mov ax, [vbe_ctrl_info_block + 0x08]
    mov ds, ax

    mov ax, 0x0000
    mov es, ax
    mov di, VBE_OEM_ADDR

.copy_oem:
    lodsb
    stosb

    test al, al
    jnz .copy_oem

    mov ax, cs
    mov ds, ax

copy_boot_info:

    mov ax, cs
    mov ds, ax

    mov al, [video_mode]
    mov [boot_info+0], al

    xor ax, ax
    mov es, ax

    mov si, boot_info
    mov di, BOOT_INFO_ADDR

    mov cx, 14
    rep movsw

    ret

get_vbe_info:
    mov ax, cs
    mov es, ax
    mov ax, 0x4F00
    mov di, vbe_ctrl_info_block
    int 0x10
    cmp ax, 0x004F
    jne vbe_fail
    ret

get_vbe_mode_info:
    mov ax, cs
    mov es, ax
    mov ax, 0x4F01
    mov cx, 0x118
    mov di, vbe_mode_info_block
    int 0x10

    cmp ax, 0x004F
    jne vbe_fail

    ret

vbe_fail:
    mov byte [video_mode], 0
    ret

set_text_mode:
    mov ax, 0x0003
    int 0x10

    mov byte [video_mode], 0

    ret

set_vbe_mode:
    mov ax, 0x4F02
    mov bx, 0x118
    int 0x10

    cmp ax, 0x004F
    jne vbe_fail

    mov byte [video_mode], 1

    ret

get_e820:
    pusha
    push ds
    push es

    xor ax, ax
    mov [e820_entries], ax

    mov ax, 0x0000
    mov es, ax
    mov di, 0x5000
    xor ebx, ebx

.e820_loop:
    mov eax, 0xE820
    mov edx, 0x534D4150
    mov ecx, 24

    int 0x15
    
    jc .e820_done

    cmp eax, 0x534D4150
    jne .e820_done

    inc word [e820_entries]
    add di, 24

    test ebx, ebx
    jnz .e820_loop

.e820_done:
    mov ax, 0x0000
    mov es, ax
    mov ax, [e820_entries]
    mov [es:0x4FFE], ax
    pop es
    pop ds
    popa
    ret

; copy e820

read_sectors_to_buffer:
    push ax
    push cx
    mov bx, buffer

.next_sector:
    mov byte [DAP2], 16
    mov byte [DAP2+1], 0
    mov word [DAP2+2], 1

    mov word [DAP2+4], bx
    mov word [DAP2+6], es

    mov word [DAP2+8], ax
    mov word [DAP2+10], 0
    mov dword [DAP2+12], 0

    mov ah, 0x42
    mov si, DAP2
    int 0x13
    jc fail

    inc ax
    add bx, 512
    loop .next_sector

    pop cx
    pop ax
    ret

load_file:

.load_next:
    mov byte [DAP2], 16
    mov byte [DAP2+1], 0
    mov word [DAP2+2], 1

    mov word [DAP2+4], bx      
    mov word [DAP2+6], es      

    mov word [DAP2+8], ax     
    mov word [DAP2+10], 0
    mov dword [DAP2+12], 0

    mov ah, 0x42
    mov dl, 0x80
    mov si, DAP2
    int 0x13
    jc fail

    inc ax
    add bx, 512
    dec cx
    jnz .load_next

    ret

fail:
    mov ah, 0x0E
    mov al, 'E'
    int 0x10
.hang:
    jmp .hang


e820_buffer:  times 64*24 db 0
e820_entries: dw 0
vbe_ctrl_info_block: times 512 db 0
vbe_mode_info_block: times 256 db 0

video_mode: db 0

boot_info:
    dd 0          ; video_mode
    dd 0          ; width
    dd 0          ; height
    dd 0          ; pitch
    dd 0          ; framebuffer low
    dd 0          ; framebuffer high

buffer:          times 32768 db 0