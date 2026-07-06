DEFAULT ABS

[org 0x0000]
[bits 16]


DAP2: times 16 db 0

start:
    cli
    mov dl, 0x80

    call get_e820

    call copy_e820_to_kernel

    mov ax, 0x2000
    mov es, ax
    mov bx, 0

    mov ax, 2085
    mov cx, 40
    call load_file

    mov ah, 0x0E
    mov al, 'O'
    int 0x10

    mov cx, [e820_entries]   
    mov ax, 0x0000
    mov es, ax
    mov di, 0x5000 

    jmp 0x2000:0x0000

    mov ah, 0x0E
    mov al, 'F'

    int 0x10

; Get e820

get_e820:
    pusha
    push ds
    push es

    xor ax, ax
    mov [e820_entries], ax

    mov di, e820_buffer
    xor ebx, ebx

.e820_loop:
    mov eax, 0xE820
    mov edx, 0x534D4150
    mov ecx, 24

    mov ax, cs
    mov es, ax

    int 0x15
    
    jc .e820_done

    cmp eax, 0x534D4150
    jne .e820_done

    inc word [e820_entries]
    add di, 24

    test ebx, ebx
    jnz .e820_loop

.e820_done:
    pop es
    pop ds
    popa
    ret

; copy e820

copy_e820_to_kernel:
    pusha

    mov ax, cs
    mov ds, ax

    mov ax, 0x0000
    mov es, ax

    mov si, e820_buffer

    mov di, 0x5000

    mov cx, [e820_entries]
    mov bx, cx
    shl cx, 4
    shl bx, 3
    add cx, bx

    rep movsb

    popa

    ret

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


e820_buffer:  times 32*24 db 0
e820_entries: dw 0
buffer:          times 32768 db 0
